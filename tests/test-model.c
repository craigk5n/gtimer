#include <glib.h>
#include "../core/db-manager.h"
#include "../core/task-list-model.h"
#include "../core/task-object.h"

static void
test_model_refresh (void)
{
  GTimerDBManager *db_manager;
  GTimerTaskListModel *model;
  GError *error = NULL;

  db_manager = gtimer_db_manager_new (":memory:", &error);
  g_assert_no_error (error);

  model = gtimer_task_list_model_new (db_manager);
  GListModel *list_model = gtimer_task_list_model_get_model (model);
  g_assert_cmpint (g_list_model_get_n_items (list_model), ==, 0);

  // Add a task manually to DB
  sqlite3 *db = gtimer_db_manager_get_db (db_manager);
  sqlite3_exec (db, "INSERT INTO tasks (name) VALUES ('Task 1');", NULL, NULL, NULL);

  gtimer_task_list_model_refresh (model);
  g_assert_cmpint (g_list_model_get_n_items (list_model), ==, 1);

  GTimerTask *task = GTIMER_TASK (g_list_model_get_item (list_model, 0));
  g_assert_cmpstr (gtimer_task_get_name (task), ==, "Task 1");
  g_object_unref (task);

  g_object_unref (model);
  g_object_unref (db_manager);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/model/refresh", test_model_refresh);

  return g_test_run ();
}
