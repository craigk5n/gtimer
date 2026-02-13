#include "project-object.h"

struct _GTimerProject {
  GObject parent_instance;

  int id;
  char *name;
};

G_DEFINE_TYPE ( GTimerProject, gtimer_project, G_TYPE_OBJECT )
enum {
  PROP_ID = 1,
  PROP_NAME,
  N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

static void gtimer_project_finalize ( GObject * object )
{
  GTimerProject *self = GTIMER_PROJECT ( object );
  g_free ( self->name );
  G_OBJECT_CLASS ( gtimer_project_parent_class )->finalize ( object );
}

static void
gtimer_project_get_property ( GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec )
{
  GTimerProject *self = GTIMER_PROJECT ( object );

  switch ( property_id ) {
    case PROP_ID:
      g_value_set_int ( value, self->id );
      break;
    case PROP_NAME:
      g_value_set_string ( value, self->name );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, property_id, pspec );
      break;
  }
}

static void
gtimer_project_set_property ( GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec )
{
  GTimerProject *self = GTIMER_PROJECT ( object );

  switch ( property_id ) {
    case PROP_ID:
      self->id = g_value_get_int ( value );
      break;
    case PROP_NAME:
      g_free ( self->name );
      self->name = g_value_dup_string ( value );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, property_id, pspec );
      break;
  }
}

static void gtimer_project_class_init ( GTimerProjectClass * klass )
{
  GObjectClass *object_class = G_OBJECT_CLASS ( klass );

  object_class->finalize = gtimer_project_finalize;
  object_class->get_property = gtimer_project_get_property;
  object_class->set_property = gtimer_project_set_property;

  properties[PROP_ID] =
      g_param_spec_int ( "id", "ID", "Project ID", -1, G_MAXINT, -1,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY );
  properties[PROP_NAME] =
      g_param_spec_string ( "name", "Name", "Project Name", NULL,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY );

  g_object_class_install_properties ( object_class, N_PROPERTIES,
      properties );
}

static void gtimer_project_init ( GTimerProject * self )
{
  ( void ) self;
}

GTimerProject *gtimer_project_new ( int id, const char *name )
{
  return g_object_new ( GTIMER_TYPE_PROJECT, "id", id, "name", name, NULL );
}

int gtimer_project_get_id ( GTimerProject * self )
{
  return self->id;
}

const char *gtimer_project_get_name ( GTimerProject * self )
{
  return self->name;
}
