#include <glib.h>
#include "../core/task-object.h"
#include "../core/project-object.h"

/* --- Task Object Tests --- */

static void
test_task_new (void)
{
  GTimerTask *task = gtimer_task_new (42, "My Task", 7, "My Project",
      100, 5000, TRUE, FALSE, 1000000, 999999);

  g_assert_cmpint (gtimer_task_get_id (task), ==, 42);
  g_assert_cmpstr (gtimer_task_get_name (task), ==, "My Task");
  g_assert_cmpint (gtimer_task_get_project_id (task), ==, 7);
  g_assert_cmpstr (gtimer_task_get_project_name (task), ==, "My Project");
  g_assert_cmpint (gtimer_task_get_today_time (task), ==, 100);
  g_assert_cmpint (gtimer_task_get_total_time (task), ==, 5000);
  g_assert_true (gtimer_task_is_timing (task));
  g_assert_false (gtimer_task_is_hidden (task));
  g_assert_cmpint (gtimer_task_get_last_start_time (task), ==, 1000000);
  g_assert_cmpint (gtimer_task_get_created_at (task), ==, 999999);

  g_object_unref (task);
}

static void
test_task_new_defaults (void)
{
  GTimerTask *task = gtimer_task_new (1, "Minimal", -1, NULL, 0, 0,
      FALSE, FALSE, 0, 0);

  g_assert_cmpint (gtimer_task_get_id (task), ==, 1);
  g_assert_cmpstr (gtimer_task_get_name (task), ==, "Minimal");
  g_assert_cmpint (gtimer_task_get_project_id (task), ==, -1);
  g_assert_null (gtimer_task_get_project_name (task));
  g_assert_cmpint (gtimer_task_get_today_time (task), ==, 0);
  g_assert_cmpint (gtimer_task_get_total_time (task), ==, 0);
  g_assert_false (gtimer_task_is_timing (task));
  g_assert_false (gtimer_task_is_hidden (task));
  g_assert_cmpint (gtimer_task_get_last_start_time (task), ==, 0);
  g_assert_cmpint (gtimer_task_get_created_at (task), ==, 0);

  g_object_unref (task);
}

static void
test_task_setters (void)
{
  GTimerTask *task = gtimer_task_new (1, "Original", -1, NULL, 0, 0,
      FALSE, FALSE, 0, 0);

  gtimer_task_set_name (task, "Renamed");
  g_assert_cmpstr (gtimer_task_get_name (task), ==, "Renamed");

  gtimer_task_set_project_id (task, 5);
  g_assert_cmpint (gtimer_task_get_project_id (task), ==, 5);

  gtimer_task_set_project_name (task, "New Project");
  g_assert_cmpstr (gtimer_task_get_project_name (task), ==, "New Project");

  gtimer_task_set_today_time (task, 3600);
  g_assert_cmpint (gtimer_task_get_today_time (task), ==, 3600);

  gtimer_task_set_total_time (task, 86400);
  g_assert_cmpint (gtimer_task_get_total_time (task), ==, 86400);

  gtimer_task_set_is_timing (task, TRUE);
  g_assert_true (gtimer_task_is_timing (task));

  gtimer_task_set_is_hidden (task, TRUE);
  g_assert_true (gtimer_task_is_hidden (task));

  gtimer_task_set_last_start_time (task, 1234567890);
  g_assert_cmpint (gtimer_task_get_last_start_time (task), ==, 1234567890);

  gtimer_task_set_created_at (task, 1111111111);
  g_assert_cmpint (gtimer_task_get_created_at (task), ==, 1111111111);

  g_object_unref (task);
}

static void
test_task_string_setter_replaces (void)
{
  GTimerTask *task = gtimer_task_new (1, "First", -1, "Proj A", 0, 0,
      FALSE, FALSE, 0, 0);

  /* Overwrite name and project_name multiple times to verify no leaks */
  gtimer_task_set_name (task, "Second");
  gtimer_task_set_name (task, "Third");
  g_assert_cmpstr (gtimer_task_get_name (task), ==, "Third");

  gtimer_task_set_project_name (task, "Proj B");
  gtimer_task_set_project_name (task, "Proj C");
  g_assert_cmpstr (gtimer_task_get_project_name (task), ==, "Proj C");

  /* Set to NULL */
  gtimer_task_set_project_name (task, NULL);
  g_assert_null (gtimer_task_get_project_name (task));

  g_object_unref (task);
}

static int notify_count = 0;
static const char *last_notified_property = NULL;

static void
on_notify (GObject *object, GParamSpec *pspec, gpointer user_data)
{
  (void)object; (void)user_data;
  notify_count++;
  last_notified_property = g_param_spec_get_name (pspec);
}

static void
test_task_property_notify (void)
{
  GTimerTask *task = gtimer_task_new (1, "Test", -1, NULL, 0, 0,
      FALSE, FALSE, 0, 0);

  g_signal_connect (task, "notify", G_CALLBACK (on_notify), NULL);

  notify_count = 0;
  gtimer_task_set_name (task, "Changed");
  g_assert_cmpint (notify_count, ==, 1);
  g_assert_cmpstr (last_notified_property, ==, "name");

  notify_count = 0;
  gtimer_task_set_today_time (task, 60);
  g_assert_cmpint (notify_count, ==, 1);
  g_assert_cmpstr (last_notified_property, ==, "today-time");

  notify_count = 0;
  gtimer_task_set_is_timing (task, TRUE);
  g_assert_cmpint (notify_count, ==, 1);
  g_assert_cmpstr (last_notified_property, ==, "is-timing");

  notify_count = 0;
  gtimer_task_set_is_hidden (task, TRUE);
  g_assert_cmpint (notify_count, ==, 1);
  g_assert_cmpstr (last_notified_property, ==, "is-hidden");

  notify_count = 0;
  gtimer_task_set_total_time (task, 999);
  g_assert_cmpint (notify_count, ==, 1);
  g_assert_cmpstr (last_notified_property, ==, "total-time");

  notify_count = 0;
  gtimer_task_set_last_start_time (task, 12345);
  g_assert_cmpint (notify_count, ==, 1);
  g_assert_cmpstr (last_notified_property, ==, "last-start-time");

  g_object_unref (task);
}

static void
test_task_gobject_properties (void)
{
  GTimerTask *task = gtimer_task_new (10, "GObj Test", 3, "Project X",
      200, 8000, FALSE, TRUE, 500, 600);

  /* Read properties via g_object_get */
  int id = 0;
  g_autofree char *name = NULL;
  int project_id = 0;
  g_autofree char *project_name = NULL;
  gint64 today_time = 0, total_time = 0;
  gboolean is_timing = TRUE, is_hidden = FALSE;

  g_object_get (task,
      "id", &id,
      "name", &name,
      "project-id", &project_id,
      "project-name", &project_name,
      "today-time", &today_time,
      "total-time", &total_time,
      "is-timing", &is_timing,
      "is-hidden", &is_hidden,
      NULL);

  g_assert_cmpint (id, ==, 10);
  g_assert_cmpstr (name, ==, "GObj Test");
  g_assert_cmpint (project_id, ==, 3);
  g_assert_cmpstr (project_name, ==, "Project X");
  g_assert_cmpint (today_time, ==, 200);
  g_assert_cmpint (total_time, ==, 8000);
  g_assert_false (is_timing);
  g_assert_true (is_hidden);

  /* Write properties via g_object_set */
  g_object_set (task,
      "name", "Updated",
      "today-time", (gint64)999,
      "is-timing", TRUE,
      NULL);

  g_assert_cmpstr (gtimer_task_get_name (task), ==, "Updated");
  g_assert_cmpint (gtimer_task_get_today_time (task), ==, 999);
  g_assert_true (gtimer_task_is_timing (task));

  g_object_unref (task);
}

/* --- Project Object Tests --- */

static void
test_project_new (void)
{
  GTimerProject *project = gtimer_project_new (5, "Work");

  g_assert_cmpint (gtimer_project_get_id (project), ==, 5);
  g_assert_cmpstr (gtimer_project_get_name (project), ==, "Work");

  g_object_unref (project);
}

static void
test_project_null_name (void)
{
  GTimerProject *project = gtimer_project_new (1, NULL);

  g_assert_cmpint (gtimer_project_get_id (project), ==, 1);
  g_assert_null (gtimer_project_get_name (project));

  g_object_unref (project);
}

static void
test_project_gobject_properties (void)
{
  GTimerProject *project = gtimer_project_new (99, "Personal");

  int id = 0;
  g_autofree char *name = NULL;

  g_object_get (project, "id", &id, "name", &name, NULL);

  g_assert_cmpint (id, ==, 99);
  g_assert_cmpstr (name, ==, "Personal");

  g_object_unref (project);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/task/new", test_task_new);
  g_test_add_func ("/task/new_defaults", test_task_new_defaults);
  g_test_add_func ("/task/setters", test_task_setters);
  g_test_add_func ("/task/string_setter_replaces", test_task_string_setter_replaces);
  g_test_add_func ("/task/property_notify", test_task_property_notify);
  g_test_add_func ("/task/gobject_properties", test_task_gobject_properties);
  g_test_add_func ("/project/new", test_project_new);
  g_test_add_func ("/project/null_name", test_project_null_name);
  g_test_add_func ("/project/gobject_properties", test_project_gobject_properties);

  return g_test_run ();
}
