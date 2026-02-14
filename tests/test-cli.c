#include "../core/db-manager.h"
#include "../core/task-object.h"
#include "../core/project-object.h"
#include <glib.h>
#include <stdio.h>

static GTimerDBManager *db = NULL;

static void
test_cli_add_project(void)
{
  GError *error = NULL;
  int count_before = g_list_length(gtimer_db_manager_get_projects(db));
  
  gtimer_db_manager_create_project(db, "CLI Test Project", &error);
  g_assert_no_error(error);
  
  GList *projects = gtimer_db_manager_get_projects(db);
  int count_after = g_list_length(projects);
  
  g_assert_cmpint(count_after, ==, count_before + 1);
  
  g_list_free_full(projects, g_object_unref);
}

static void
test_cli_add_task(void)
{
  GError *error = NULL;
  int count_before = g_list_length(gtimer_db_manager_get_all_tasks(db));
  
  gtimer_db_manager_create_task(db, "CLI Test Task", -1, &error);
  g_assert_no_error(error);
  
  GList *tasks = gtimer_db_manager_get_all_tasks(db);
  int count_after = g_list_length(tasks);
  
  g_assert_cmpint(count_after, ==, count_before + 1);
  
  g_list_free_full(tasks, g_object_unref);
}

static void
test_cli_hide_unhide_task(void)
{
  GError *error = NULL;
  
  /* Create a task first */
  gtimer_db_manager_create_task(db, "Hide Test Task", -1, &error);
  g_assert_no_error(error);
  
  GList *tasks = gtimer_db_manager_get_all_tasks(db);
  GTimerTask *task = GTIMER_TASK(tasks->data);
  int task_id = gtimer_task_get_id(task);
  g_list_free_full(tasks, g_object_unref);
  
  /* Hide it */
  gtimer_db_manager_hide_task(db, task_id, TRUE, &error);
  g_assert_no_error(error);
  
  tasks = gtimer_db_manager_get_all_tasks(db);
  gboolean found = FALSE;
  for (GList *l = tasks; l != NULL; l = l->next) {
    GTimerTask *t = GTIMER_TASK(l->data);
    if (gtimer_task_get_id(t) == task_id) {
      found = TRUE;
      g_assert_true(gtimer_task_is_hidden(t));
    }
    g_object_unref(t);
  }
  g_list_free(tasks);
  g_assert_true(found);
  
  /* Unhide it */
  gtimer_db_manager_hide_task(db, task_id, FALSE, &error);
  g_assert_no_error(error);
}

static void
test_cli_annotations(void)
{
  GError *error = NULL;
  
  /* Create a task */
  gtimer_db_manager_create_task(db, "Annotation Test Task", -1, &error);
  g_assert_no_error(error);
  
  GList *tasks = gtimer_db_manager_get_all_tasks(db);
  GTimerTask *task = GTIMER_TASK(tasks->data);
  int task_id = gtimer_task_get_id(task);
  g_list_free_full(tasks, g_object_unref);
  
  /* Add annotation */
  gtimer_db_manager_add_annotation(db, task_id, "Test annotation");
  
  /* Get annotations */
  GList *annotations = gtimer_db_manager_get_annotations(db, task_id);
  g_assert_nonnull(annotations);
  g_assert_cmpint(g_list_length(annotations), >, 0);
  
  g_list_free_full(annotations, (GDestroyNotify)gtimer_annotation_free);
}

int
main(int argc, char **argv)
{
  g_test_init(&argc, &argv, NULL);
  
  GError *error = NULL;
  db = gtimer_db_manager_new(":memory:", &error);
  g_assert_no_error(error);
  g_assert_nonnull(db);
  
  g_test_add_func("/cli/add_project", test_cli_add_project);
  g_test_add_func("/cli/add_task", test_cli_add_task);
  g_test_add_func("/cli/hide_unhide_task", test_cli_hide_unhide_task);
  g_test_add_func("/cli/annotations", test_cli_annotations);
  
  int result = g_test_run();
  
  g_object_unref(db);
  
  return result;
}
