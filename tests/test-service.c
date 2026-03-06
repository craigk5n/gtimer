#include <glib.h>
#include "../core/timer-service.h"
#include "../core/task-object.h"

static int tick_count = 0;

static void
on_tick (GTimerTimerService *service, gint64 elapsed, gpointer user_data)
{
  (void)service; (void)elapsed;
  GMainLoop *loop = user_data;
  tick_count++;
  if (tick_count >= 2) {
    g_main_loop_quit (loop);
  }
}

static void
test_service_tick (void)
{
  GTimerDBManager *db_manager;
  GTimerTimerService *service;
  GTimerTask *task;
  GMainLoop *loop;

  db_manager = gtimer_db_manager_new (":memory:", NULL);
  service = gtimer_timer_service_new (db_manager);

  sqlite3 *db = gtimer_db_manager_get_db (db_manager);
  sqlite3_exec (db, "INSERT INTO tasks (id, name) VALUES (1, 'Test Task');", NULL, NULL, NULL);

  task = gtimer_task_new (1, "Test Task", -1, NULL, 0, 0, FALSE, FALSE, 0, 0);
  loop = g_main_loop_new (NULL, FALSE);

  g_signal_connect (service, "tick", G_CALLBACK (on_tick), loop);

  gtimer_timer_service_start (service, task);
  
  // Run loop until 2 ticks happen
  g_main_loop_run (loop);

  g_assert_cmpint (tick_count, >=, 2);
  g_assert_nonnull (gtimer_timer_service_get_active_task (service));

  gtimer_timer_service_stop (service);
  g_assert_null (gtimer_timer_service_get_active_task (service));

  g_main_loop_unref (loop);
  g_object_unref (task);
  g_object_unref (service);
  g_object_unref (db_manager);
}

static void
test_service_save (void)
{
  GTimerDBManager *db_manager;
  GTimerTimerService *service;
  GTimerTask *task;

  db_manager = gtimer_db_manager_new (":memory:", NULL);
  service = gtimer_timer_service_new (db_manager);
  
  // Must insert task into DB first so daily_time FK is valid
  sqlite3 *db = gtimer_db_manager_get_db (db_manager);
  sqlite3_exec (db, "INSERT INTO tasks (id, name) VALUES (1, 'Test Task');", NULL, NULL, NULL);
  
  task = gtimer_task_new (1, "Test Task", -1, NULL, 0, 0, FALSE, FALSE, 0, 0);

  gtimer_timer_service_start (service, task);
  // Manual override of start_time to 10 seconds ago
  // We'll just wait a bit instead since we can't easily reach into the struct
  g_usleep (1100000); // 1.1 seconds

  gtimer_timer_service_stop (service);

  // Verify DB has the entry
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (db, "SELECT seconds FROM daily_time WHERE task_id = 1;", -1, &stmt, NULL);
  g_assert_cmpint (sqlite3_step (stmt), ==, SQLITE_ROW);
  g_assert_cmpint (sqlite3_column_int (stmt, 0), >=, 1);
  sqlite3_finalize (stmt);

  g_object_unref (task);
  g_object_unref (service);
  g_object_unref (db_manager);
}

static void
test_service_task_switching (void)
{
  GTimerDBManager *db_manager;
  GTimerTimerService *service;
  GTimerTask *task1, *task2;

  db_manager = gtimer_db_manager_new (":memory:", NULL);
  service = gtimer_timer_service_new (db_manager);
  
  sqlite3 *db = gtimer_db_manager_get_db (db_manager);
  sqlite3_exec (db, "INSERT INTO tasks (id, name) VALUES (1, 'Task A');", NULL, NULL, NULL);
  sqlite3_exec (db, "INSERT INTO tasks (id, name) VALUES (2, 'Task B');", NULL, NULL, NULL);
  
  task1 = gtimer_task_new (1, "Task A", -1, NULL, 0, 0, FALSE, FALSE, 0, 0);
  task2 = gtimer_task_new (2, "Task B", -1, NULL, 0, 0, FALSE, FALSE, 0, 0);
  
  /* Start Task A */
  gtimer_timer_service_start (service, task1);
  g_assert_true (gtimer_task_is_timing (task1));
  g_assert_nonnull (gtimer_timer_service_get_active_task (service));
  g_assert_cmpint (gtimer_task_get_id (gtimer_timer_service_get_active_task (service)), ==, 1);
  
  /* Start Task B - should stop Task A */
  gtimer_timer_service_start (service, task2);
  g_assert_true (gtimer_task_is_timing (task2));
  g_assert_false (gtimer_task_is_timing (task1));
  g_assert_cmpint (gtimer_task_get_id (gtimer_timer_service_get_active_task (service)), ==, 2);
  
  /* Stop Task B */
  gtimer_timer_service_stop (service);
  g_assert_false (gtimer_task_is_timing (task2));
  g_assert_null (gtimer_timer_service_get_active_task (service));
  
  g_object_unref (task1);
  g_object_unref (task2);
  g_object_unref (service);
  g_object_unref (db_manager);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/service/tick", test_service_tick);
  g_test_add_func ("/service/save", test_service_save);
  g_test_add_func ("/service/switching", test_service_task_switching);

  return g_test_run ();
}
