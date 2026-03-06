#include "task-object.h"
struct _GTimerTask {
  GObject parent_instance;
  int id;
  char *name;
  int project_id;
  char *project_name;
  gint64 today_time;
  gint64 total_time;
  gboolean is_timing;
  gboolean is_hidden;
  gint64 last_start_time;
  gint64 created_at;
  char *tags;
};
G_DEFINE_TYPE ( GTimerTask, gtimer_task, G_TYPE_OBJECT )
enum {
  PROP_ID = 1,
  PROP_NAME,
  PROP_PROJECT_ID,
  PROP_PROJECT_NAME,
  PROP_TODAY_TIME,
  PROP_TOTAL_TIME,
  PROP_IS_TIMING,
  PROP_IS_HIDDEN,
  PROP_LAST_START_TIME,
  PROP_CREATED_AT,
  PROP_TAGS,
  N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

static void gtimer_task_finalize ( GObject * object )
{
  GTimerTask *self = GTIMER_TASK ( object );
  g_free ( self->name );
  g_free ( self->project_name );
  g_free ( self->tags );
  G_OBJECT_CLASS ( gtimer_task_parent_class )->finalize ( object );
}

static void
gtimer_task_get_property ( GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec )
{
  GTimerTask *self = GTIMER_TASK ( object );
  switch ( property_id ) {
    case PROP_ID:
      g_value_set_int ( value, self->id );
      break;
    case PROP_NAME:
      g_value_set_string ( value, self->name );
      break;
    case PROP_PROJECT_ID:
      g_value_set_int ( value, self->project_id );
      break;
    case PROP_PROJECT_NAME:
      g_value_set_string ( value, self->project_name );
      break;
    case PROP_TODAY_TIME:
      g_value_set_int64 ( value, self->today_time );
      break;
    case PROP_TOTAL_TIME:
      g_value_set_int64 ( value, self->total_time );
      break;
    case PROP_IS_TIMING:
      g_value_set_boolean ( value, self->is_timing );
      break;
    case PROP_IS_HIDDEN:
      g_value_set_boolean ( value, self->is_hidden );
      break;
    case PROP_LAST_START_TIME:
      g_value_set_int64 ( value, self->last_start_time );
      break;
    case PROP_CREATED_AT:
      g_value_set_int64 ( value, self->created_at );
      break;
    case PROP_TAGS:
      g_value_set_string ( value, self->tags );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, property_id, pspec );
      break;
  }
}

static void
gtimer_task_set_property ( GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec )
{
  GTimerTask *self = GTIMER_TASK ( object );
  switch ( property_id ) {
    case PROP_ID:
      self->id = g_value_get_int ( value );
      break;
    case PROP_NAME:
      g_free ( self->name );
      self->name = g_value_dup_string ( value );
      break;
    case PROP_PROJECT_ID:
      self->project_id = g_value_get_int ( value );
      break;
    case PROP_PROJECT_NAME:
      g_free ( self->project_name );
      self->project_name = g_value_dup_string ( value );
      break;
    case PROP_TODAY_TIME:
      self->today_time = g_value_get_int64 ( value );
      break;
    case PROP_TOTAL_TIME:
      self->total_time = g_value_get_int64 ( value );
      break;
    case PROP_IS_TIMING:
      self->is_timing = g_value_get_boolean ( value );
      break;
    case PROP_IS_HIDDEN:
      self->is_hidden = g_value_get_boolean ( value );
      break;
    case PROP_LAST_START_TIME:
      self->last_start_time = g_value_get_int64 ( value );
      break;
    case PROP_CREATED_AT:
      self->created_at = g_value_get_int64 ( value );
      break;
    case PROP_TAGS:
      g_free ( self->tags );
      self->tags = g_value_dup_string ( value );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, property_id, pspec );
      break;
  }
}

static void gtimer_task_class_init ( GTimerTaskClass * klass )
{
  GObjectClass *object_class = G_OBJECT_CLASS ( klass );
  object_class->finalize = gtimer_task_finalize;
  object_class->get_property = gtimer_task_get_property;
  object_class->set_property = gtimer_task_set_property;
  properties[PROP_ID] =
      g_param_spec_int ( "id", "ID", "Task ID", -1, G_MAXINT, -1,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY );
  properties[PROP_NAME] =
      g_param_spec_string ( "name", "Name", "Task Name", NULL,
      G_PARAM_READWRITE );
  properties[PROP_PROJECT_ID] =
      g_param_spec_int ( "project-id", "Project ID", "Project ID", -1,
      G_MAXINT, -1, G_PARAM_READWRITE );
  properties[PROP_PROJECT_NAME] =
      g_param_spec_string ( "project-name", "Project Name", "Project Name",
      NULL, G_PARAM_READWRITE );
  properties[PROP_TODAY_TIME] =
      g_param_spec_int64 ( "today-time", "Today Time",
      "Time spent today in seconds", 0, G_MAXINT64, 0, G_PARAM_READWRITE );
  properties[PROP_TOTAL_TIME] =
      g_param_spec_int64 ( "total-time", "Total Time",
      "Total time in seconds", 0, G_MAXINT64, 0, G_PARAM_READWRITE );
  properties[PROP_IS_TIMING] =
      g_param_spec_boolean ( "is-timing", "Is Timing",
      "Whether task is currently being timed", FALSE, G_PARAM_READWRITE );
  properties[PROP_IS_HIDDEN] =
      g_param_spec_boolean ( "is-hidden", "Is Hidden",
      "Whether task is hidden", FALSE, G_PARAM_READWRITE );
  properties[PROP_LAST_START_TIME] =
      g_param_spec_int64 ( "last-start-time", "Last Start Time",
      "Unix timestamp of last start", 0, G_MAXINT64, 0, G_PARAM_READWRITE );
  properties[PROP_CREATED_AT] =
      g_param_spec_int64 ( "created-at", "Created At",
      "Unix timestamp of creation", 0, G_MAXINT64, 0,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY );
  properties[PROP_TAGS] =
      g_param_spec_string ( "tags", "Tags",
      "Comma-separated tag names", NULL, G_PARAM_READWRITE );
  g_object_class_install_properties ( object_class, N_PROPERTIES,
      properties );
}

static void gtimer_task_init ( GTimerTask * self )
{
  ( void ) self;
}

GTimerTask *gtimer_task_new ( int id, const char *name, int project_id,
    const char *project_name, gint64 today_time, gint64 total_time,
    gboolean is_timing, gboolean is_hidden, gint64 last_start_time,
    gint64 created_at )
{
  GTimerTask *task = g_object_new ( GTIMER_TYPE_TASK,
      "id", id,
      "name", name,
      "project-id", project_id,
      "project-name", project_name,
      "today-time", today_time,
      "total-time", total_time,
      "is-timing", is_timing,
      "is-hidden", is_hidden,
      "last-start-time", last_start_time, "created-at", created_at, NULL );
  g_object_ref_sink(task);
  return task;
}

int gtimer_task_get_id ( GTimerTask * self )
{
  return self->id;
}

const char *gtimer_task_get_name ( GTimerTask * self )
{
  return self->name;
}

int gtimer_task_get_project_id ( GTimerTask * self )
{
  return self->project_id;
}

const char *gtimer_task_get_project_name ( GTimerTask * self )
{
  return self->project_name;
}

gint64 gtimer_task_get_today_time ( GTimerTask * self )
{
  return self->today_time;
}

gint64 gtimer_task_get_total_time ( GTimerTask * self )
{
  return self->total_time;
}

gboolean gtimer_task_is_timing ( GTimerTask * self )
{
  return self->is_timing;
}

gboolean gtimer_task_is_hidden ( GTimerTask * self )
{
  return self->is_hidden;
}

gint64 gtimer_task_get_last_start_time ( GTimerTask * self )
{
  return self->last_start_time;
}

gint64 gtimer_task_get_created_at ( GTimerTask * self )
{
  return self->created_at;
}

void gtimer_task_set_name ( GTimerTask * self, const char *name )
{
  g_free ( self->name );
  self->name = g_strdup ( name );
  g_object_notify_by_pspec ( G_OBJECT ( self ), properties[PROP_NAME] );
}

void gtimer_task_set_project_id ( GTimerTask * self, int project_id )
{
  self->project_id = project_id;
  g_object_notify_by_pspec ( G_OBJECT ( self ), properties[PROP_PROJECT_ID] );
}

void gtimer_task_set_project_name ( GTimerTask * self,
    const char *project_name )
{
  g_free ( self->project_name );
  self->project_name = g_strdup ( project_name );
  g_object_notify_by_pspec ( G_OBJECT ( self ),
      properties[PROP_PROJECT_NAME] );
}

void gtimer_task_set_today_time ( GTimerTask * self, gint64 today_time )
{
  self->today_time = today_time;
  g_object_notify_by_pspec ( G_OBJECT ( self ), properties[PROP_TODAY_TIME] );
}

void gtimer_task_set_total_time ( GTimerTask * self, gint64 total_time )
{
  self->total_time = total_time;
  g_object_notify_by_pspec ( G_OBJECT ( self ), properties[PROP_TOTAL_TIME] );
}

void gtimer_task_set_is_timing ( GTimerTask * self, gboolean is_timing )
{
  self->is_timing = is_timing;
  g_object_notify_by_pspec ( G_OBJECT ( self ), properties[PROP_IS_TIMING] );
}

void gtimer_task_set_is_hidden ( GTimerTask * self, gboolean is_hidden )
{
  self->is_hidden = is_hidden;
  g_object_notify_by_pspec ( G_OBJECT ( self ), properties[PROP_IS_HIDDEN] );
}

void gtimer_task_set_last_start_time ( GTimerTask * self,
    gint64 last_start_time )
{
  self->last_start_time = last_start_time;
  g_object_notify_by_pspec ( G_OBJECT ( self ),
      properties[PROP_LAST_START_TIME] );
}

void gtimer_task_set_created_at ( GTimerTask * self, gint64 created_at )
{
  self->created_at = created_at;
  g_object_notify_by_pspec ( G_OBJECT ( self ), properties[PROP_CREATED_AT] );
}

const char *gtimer_task_get_tags ( GTimerTask * self )
{
  return self->tags;
}

void gtimer_task_set_tags ( GTimerTask * self, const char *tags )
{
  g_free ( self->tags );
  self->tags = g_strdup ( tags );
  g_object_notify_by_pspec ( G_OBJECT ( self ), properties[PROP_TAGS] );
}
