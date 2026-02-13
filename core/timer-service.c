#include "timer-service.h"
#include "timer-utils.h"
#include <time.h>
#include <gio/gio.h>

struct _GTimerTimerService {
  GObject parent_instance;

  GTimerDBManager *db_manager;
  GTimerTask *active_task;
  gint64 start_time;
  gint64 paused_duration;
  guint timeout_id;

  gint64 initial_today_time;
  gint64 initial_total_time;

  GTimerIdleMonitor *idle_monitor;
  gint64 pause_start_time;
  gboolean is_paused;
  guint auto_save_id;
  gint64 idle_start_time;
};

G_DEFINE_TYPE ( GTimerTimerService, gtimer_timer_service, G_TYPE_OBJECT )
static void gtimer_timer_service_auto_save ( GTimerTimerService * self )
{
  GSettings *settings = g_settings_new ( "us.k5n.GTimer" );
  gboolean enabled = g_settings_get_boolean ( settings, "auto-save" );
  g_object_unref ( settings );

  if ( !enabled )
    return;

  sqlite3 *db = gtimer_db_manager_get_db ( self->db_manager );
  sqlite3_stmt *stmt;
  gint64 now = time ( NULL );

  // Find all timing tasks
  const char *sql =
      "SELECT id, last_start_time FROM tasks WHERE is_timing = 1;";
  if ( sqlite3_prepare_v2 ( db, sql, -1, &stmt, NULL ) == SQLITE_OK ) {
    while ( sqlite3_step ( stmt ) == SQLITE_ROW ) {
      int id = sqlite3_column_int ( stmt, 0 );
      gint64 last_start = sqlite3_column_int64 ( stmt, 1 );
      gint64 elapsed = now - last_start;

      if ( elapsed > 0 ) {
        // Flush elapsed to daily_time
        gtimer_db_manager_add_task_time ( self->db_manager, id, elapsed );
        // Update last_start_time to now so we don't double count
        sqlite3_stmt *upd;
        const char *upd_sql =
            "UPDATE tasks SET last_start_time = ? WHERE id = ?;";
        if ( sqlite3_prepare_v2 ( db, upd_sql, -1, &upd, NULL ) == SQLITE_OK ) {
          sqlite3_bind_int64 ( upd, 1, now );
          sqlite3_bind_int ( upd, 2, id );
          sqlite3_step ( upd );
          sqlite3_finalize ( upd );
        }
      }
    }
    sqlite3_finalize ( stmt );
  }
}

static gboolean auto_save_timeout_cb ( gpointer user_data )
{
  GTimerTimerService *self = GTIMER_TIMER_SERVICE ( user_data );
  gtimer_timer_service_auto_save ( self );
  return TRUE;
}

enum {
  SIGNAL_TICK,
  SIGNAL_PAUSED,
  SIGNAL_RESUMED,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

static void gtimer_timer_service_finalize ( GObject * object )
{
  GTimerTimerService *self = GTIMER_TIMER_SERVICE ( object );
  gtimer_timer_service_stop ( self );
  if ( self->auto_save_id )
    g_source_remove ( self->auto_save_id );
  g_clear_object ( &self->idle_monitor );
  G_OBJECT_CLASS ( gtimer_timer_service_parent_class )->finalize ( object );
}

static void
gtimer_timer_service_class_init ( GTimerTimerServiceClass * klass )
{
  GObjectClass *object_class = G_OBJECT_CLASS ( klass );
  object_class->finalize = gtimer_timer_service_finalize;

  signals[SIGNAL_TICK] =
      g_signal_new ( "tick",
      G_TYPE_FROM_CLASS ( klass ),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_INT64 );

  signals[SIGNAL_PAUSED] =
      g_signal_new ( "paused",
      G_TYPE_FROM_CLASS ( klass ),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0 );

  signals[SIGNAL_RESUMED] =
      g_signal_new ( "resumed",
      G_TYPE_FROM_CLASS ( klass ),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0 );
}

static void gtimer_timer_service_init ( GTimerTimerService * self )
{
  self->auto_save_id =
      g_timeout_add_seconds ( 60, auto_save_timeout_cb, self );
}

static void
on_idle_monitor_idle ( GTimerIdleMonitor * monitor, gpointer user_data )
{
  ( void ) monitor;
  GTimerTimerService *self = GTIMER_TIMER_SERVICE ( user_data );
  self->idle_start_time = time ( NULL );
  gtimer_timer_service_pause ( self );
}

static void
on_idle_monitor_resume ( GTimerIdleMonitor * monitor, gpointer user_data )
{
  ( void ) monitor;
  GTimerTimerService *self = GTIMER_TIMER_SERVICE ( user_data );
  gtimer_timer_service_resume ( self );
}

GTimerTimerService *gtimer_timer_service_new ( GTimerDBManager * db_manager )
{
  GTimerTimerService *self = g_object_new ( GTIMER_TYPE_TIMER_SERVICE, NULL );
  self->db_manager = db_manager;
  return self;
}

static gboolean on_timeout ( gpointer user_data )
{
  GTimerTimerService *self = GTIMER_TIMER_SERVICE ( user_data );
  gint64 elapsed = gtimer_timer_service_get_elapsed ( self );

  if ( self->active_task ) {
    gtimer_task_set_today_time ( self->active_task,
        self->initial_today_time + elapsed );
    gtimer_task_set_total_time ( self->active_task,
        self->initial_total_time + elapsed );
    g_signal_emit ( self, signals[SIGNAL_TICK], 0, elapsed );
  }

  return TRUE;
}

void
gtimer_timer_service_start ( GTimerTimerService * self, GTimerTask * task )
{
  g_return_if_fail ( GTIMER_IS_TIMER_SERVICE ( self ) );
  g_return_if_fail ( GTIMER_IS_TASK ( task ) );

  if ( self->active_task )
    gtimer_timer_service_stop ( self );

  self->active_task = g_object_ref ( task );
  self->start_time = time ( NULL );
  self->paused_duration = 0;
  self->is_paused = FALSE;
  self->pause_start_time = 0;

  self->initial_today_time = gtimer_task_get_today_time ( task );
  self->initial_total_time = gtimer_task_get_total_time ( task );

  gtimer_task_set_is_timing ( self->active_task, TRUE );

  self->timeout_id = g_timeout_add_seconds ( 1, on_timeout, self );

  // Start idle monitoring if available
  if ( self->idle_monitor
      && gtimer_idle_monitor_is_available ( self->idle_monitor ) ) {
    GSettings *settings = g_settings_new ( "us.k5n.GTimer" );
    int threshold = g_settings_get_int ( settings, "idle-threshold" );
    gtimer_idle_monitor_start ( self->idle_monitor, threshold * 60 );
    g_object_unref ( settings );
  }
  // Emit initial tick
  g_signal_emit ( self, signals[SIGNAL_TICK], 0, ( gint64 ) 0 );
}

void gtimer_timer_service_stop ( GTimerTimerService * self )
{
  g_return_if_fail ( GTIMER_IS_TIMER_SERVICE ( self ) );

  if ( self->timeout_id ) {
    g_source_remove ( self->timeout_id );
    self->timeout_id = 0;
  }
  // Stop idle monitoring
  if ( self->idle_monitor ) {
    gtimer_idle_monitor_stop ( self->idle_monitor );
  }

  if ( self->active_task ) {
    gint64 elapsed = gtimer_timer_service_get_elapsed ( self );
    if ( elapsed > 0 ) {
      gtimer_db_manager_add_task_time ( self->db_manager,
          gtimer_task_get_id ( self->active_task ), elapsed );
    }
    gtimer_task_set_is_timing ( self->active_task, FALSE );
    g_clear_object ( &self->active_task );
  }

  self->is_paused = FALSE;
  self->pause_start_time = 0;
}

void gtimer_timer_service_pause ( GTimerTimerService * self )
{
  g_return_if_fail ( GTIMER_IS_TIMER_SERVICE ( self ) );

  if ( !self->active_task || self->is_paused )
    return;

  self->is_paused = TRUE;
  self->pause_start_time = time ( NULL );

  // Stop the tick timer while paused
  if ( self->timeout_id ) {
    g_source_remove ( self->timeout_id );
    self->timeout_id = 0;
  }

  g_signal_emit ( self, signals[SIGNAL_PAUSED], 0 );
}

void gtimer_timer_service_resume ( GTimerTimerService * self )
{
  g_return_if_fail ( GTIMER_IS_TIMER_SERVICE ( self ) );

  if ( !self->active_task || !self->is_paused )
    return;

  // Add the paused time to total paused duration
  self->paused_duration += time ( NULL ) - self->pause_start_time;
  self->is_paused = FALSE;
  self->pause_start_time = 0;

  // Restart the tick timer
  self->timeout_id = g_timeout_add_seconds ( 1, on_timeout, self );

  // Emit current elapsed time
  gint64 elapsed = gtimer_timer_service_get_elapsed ( self );
  g_signal_emit ( self, signals[SIGNAL_TICK], 0, elapsed );

  g_signal_emit ( self, signals[SIGNAL_RESUMED], 0 );
}

GTimerTask *gtimer_timer_service_get_active_task ( GTimerTimerService * self )
{
  return self->active_task;
}

GTimerDBManager *gtimer_timer_service_get_db_manager ( GTimerTimerService *
    self )
{
  return self->db_manager;
}

gint64 gtimer_timer_service_get_elapsed ( GTimerTimerService * self )
{
  if ( !self->active_task )
    return 0;
  return gtimer_utils_calculate_current_duration ( self->start_time,
      time ( NULL ), self->paused_duration );
}

gboolean gtimer_timer_service_is_paused ( GTimerTimerService * self )
{
  return self->is_paused;
}

void
gtimer_timer_service_remove_time ( GTimerTimerService * self,
    GTimerTask * task, gint64 seconds )
{
  g_return_if_fail ( GTIMER_IS_TIMER_SERVICE ( self ) );
  g_return_if_fail ( GTIMER_IS_TASK ( task ) );

  gtimer_db_manager_add_task_time ( self->db_manager,
      gtimer_task_get_id ( task ), -seconds );
}

gint64 gtimer_timer_service_get_idle_duration ( GTimerTimerService * self )
{
  if ( self->idle_start_time == 0 )
    return 0;
  return time ( NULL ) - self->idle_start_time;
}

void
gtimer_timer_service_set_idle_monitor ( GTimerTimerService * self,
    GTimerIdleMonitor * monitor )
{
  g_return_if_fail ( GTIMER_IS_TIMER_SERVICE ( self ) );

  if ( self->idle_monitor ) {
    g_signal_handlers_disconnect_by_func ( self->idle_monitor,
        on_idle_monitor_idle, self );
    g_signal_handlers_disconnect_by_func ( self->idle_monitor,
        on_idle_monitor_resume, self );
  }

  g_clear_object ( &self->idle_monitor );

  if ( monitor ) {
    self->idle_monitor = g_object_ref ( monitor );
    g_signal_connect ( monitor, "idle", G_CALLBACK ( on_idle_monitor_idle ),
        self );
    g_signal_connect ( monitor, "resume",
        G_CALLBACK ( on_idle_monitor_resume ), self );
  }
}
