#include "task-list-model.h"
#include "task-object.h"
#include <sqlite3.h>
#include <time.h>

struct _GTimerTaskListModel {
  GObject parent_instance;

  GTimerDBManager *db_manager;
  GListStore *store;
};

G_DEFINE_TYPE ( GTimerTaskListModel, gtimer_task_list_model, G_TYPE_OBJECT )
static void gtimer_task_list_model_finalize ( GObject * object )
{
  GTimerTaskListModel *self = GTIMER_TASK_LIST_MODEL ( object );
  g_clear_object ( &self->db_manager );
  g_object_unref ( self->store );
  G_OBJECT_CLASS ( gtimer_task_list_model_parent_class )->finalize ( object );
}

static void
gtimer_task_list_model_class_init ( GTimerTaskListModelClass * klass )
{
  GObjectClass *object_class = G_OBJECT_CLASS ( klass );
  object_class->finalize = gtimer_task_list_model_finalize;
}

static void gtimer_task_list_model_init ( GTimerTaskListModel * self )
{
  self->store = g_list_store_new ( GTIMER_TYPE_TASK );
}

GTimerTaskListModel *gtimer_task_list_model_new ( GTimerDBManager *
    db_manager )
{
  GTimerTaskListModel *self =
      g_object_new ( GTIMER_TYPE_TASK_LIST_MODEL, NULL );
  self->db_manager = g_object_ref ( db_manager );
  return self;
}

GListModel *gtimer_task_list_model_get_model ( GTimerTaskListModel * self )
{
  return G_LIST_MODEL ( self->store );
}

void gtimer_task_list_model_refresh ( GTimerTaskListModel * self )
{
  sqlite3 *db = gtimer_db_manager_get_db ( self->db_manager );
  if ( !db )
    return;

  sqlite3_stmt *stmt;
  int rc;
  GDateTime *now_dt = g_date_time_new_now_local (  );
  char *today = g_date_time_format ( now_dt, "%Y-%m-%d" );
  gint64 now = g_date_time_to_unix ( now_dt );
  g_date_time_unref ( now_dt );

  const char *sql =
      "SELECT t.id, t.name, t.project_id, p.name, t.is_timing, t.is_hidden, "
      "  (SELECT seconds FROM daily_time WHERE task_id = t.id AND date = ?) as today_time, "
      "  (SELECT SUM(seconds) FROM daily_time WHERE task_id = t.id) as total_time, "
      "  t.last_start_time, t.created_at "
      "FROM tasks t "
      "LEFT JOIN projects p ON t.project_id = p.id " "WHERE t.is_hidden = 0;";

  rc = sqlite3_prepare_v2 ( db, sql, -1, &stmt, NULL );
  if ( rc != SQLITE_OK ) {
    g_free ( today );
    return;
  }

  sqlite3_bind_text ( stmt, 1, today, -1, SQLITE_TRANSIENT );

  GHashTable *found_ids = g_hash_table_new ( g_direct_hash, g_direct_equal );

  while ( sqlite3_step ( stmt ) == SQLITE_ROW ) {
    int id = sqlite3_column_int ( stmt, 0 );
    const char *name = ( const char * ) sqlite3_column_text ( stmt, 1 );
    int project_id = sqlite3_column_int ( stmt, 2 );
    const char *project_name =
        ( const char * ) sqlite3_column_text ( stmt, 3 );
    gboolean is_timing = sqlite3_column_int ( stmt, 4 ) != 0;
    gboolean is_hidden = sqlite3_column_int ( stmt, 5 ) != 0;
    gint64 db_today_time = sqlite3_column_int64 ( stmt, 6 );
    gint64 db_total_time = sqlite3_column_int64 ( stmt, 7 );
    gint64 last_start = sqlite3_column_int64 ( stmt, 8 );
    gint64 created_at = sqlite3_column_int64 ( stmt, 9 );

    gint64 today_time = db_today_time;
    gint64 total_time = db_total_time;
    if ( is_timing && last_start > 0 ) {
      gint64 elapsed = now - last_start;
      if ( elapsed > 0 ) {
        today_time += elapsed;
        total_time += elapsed;
      }
    }

    g_hash_table_add ( found_ids, GINT_TO_POINTER ( id ) );

    GTimerTask *task = NULL;
    guint n = g_list_model_get_n_items ( G_LIST_MODEL ( self->store ) );
    for ( guint i = 0; i < n; i++ ) {
      GTimerTask *t =
          GTIMER_TASK ( g_list_model_get_item ( G_LIST_MODEL ( self->store ),
              i ) );
      if ( gtimer_task_get_id ( t ) == id ) {
        task = t;
        break;
      }
      g_object_unref ( t );
    }

    if ( task ) {
      gtimer_task_set_name ( task, name );
      gtimer_task_set_project_id ( task, project_id );
      gtimer_task_set_project_name ( task, project_name );
      gtimer_task_set_today_time ( task, today_time );
      gtimer_task_set_total_time ( task, total_time );
      gtimer_task_set_is_timing ( task, is_timing );
      gtimer_task_set_is_hidden ( task, is_hidden );
      gtimer_task_set_last_start_time ( task, last_start );
      gtimer_task_set_created_at ( task, created_at );
      g_object_unref ( task );
    } else {
      task =
          gtimer_task_new ( id, name, project_id, project_name, today_time,
          total_time, is_timing, is_hidden, last_start, created_at );
      g_list_store_append ( self->store, task );
      g_object_unref ( task );
    }
  }

  for ( int i =
      ( int )g_list_model_get_n_items ( G_LIST_MODEL ( self->store ) ) - 1;
      i >= 0; i-- ) {
    GTimerTask *t =
        GTIMER_TASK ( g_list_model_get_item ( G_LIST_MODEL ( self->store ),
            i ) );
    if ( !g_hash_table_contains ( found_ids,
            GINT_TO_POINTER ( gtimer_task_get_id ( t ) ) ) ) {
      g_list_store_remove ( self->store, i );
    }
    g_object_unref ( t );
  }

  g_hash_table_destroy ( found_ids );
  sqlite3_finalize ( stmt );
  g_free ( today );
}
