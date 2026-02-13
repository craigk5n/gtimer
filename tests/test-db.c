#include <glib.h>
#include "../core/db-manager.h"
#include <sqlite3.h>

static void
test_db_init (void)
{
  GTimerDBManager *db_manager;
  GError *error = NULL;

  db_manager = gtimer_db_manager_new (":memory:", &error);
  g_assert_no_error (error);
  g_assert_nonnull (db_manager);

  sqlite3 *db = gtimer_db_manager_get_db (db_manager);
  g_assert_nonnull (db);

  // Verify tables exist
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2 (db, "SELECT name FROM sqlite_master WHERE type='table' AND name='projects';", -1, &stmt, NULL);
  g_assert_cmpint (rc, ==, SQLITE_OK);
  g_assert_cmpint (sqlite3_step (stmt), ==, SQLITE_ROW);
  sqlite3_finalize (stmt);

  g_object_unref (db_manager);
}

static void
test_db_save_task (void)
{
  GTimerDBManager *db_manager;
  GError *error = NULL;

  db_manager = gtimer_db_manager_new (":memory:", &error);
  g_assert_no_error (error);

  sqlite3 *db = gtimer_db_manager_get_db (db_manager);
  
  // Insert a task (project_id = NULL to avoid FK failure)
  sqlite3_exec (db, "INSERT INTO tasks (name, project_id) VALUES ('Test Task', NULL);", NULL, NULL, NULL);

  // Insert daily time
  sqlite3_exec (db, "INSERT INTO daily_time (task_id, date, seconds) VALUES (1, '2024-02-09', 3600);", NULL, NULL, NULL);

  // Verify task was saved
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2 (db, "SELECT name, project_id FROM tasks;", -1, &stmt, NULL);
  g_assert_cmpint (rc, ==, SQLITE_OK);
  g_assert_cmpint (sqlite3_step (stmt), ==, SQLITE_ROW);
  g_assert_cmpstr ((const char *)sqlite3_column_text (stmt, 0), ==, "Test Task");
  g_assert_true (sqlite3_column_type (stmt, 1) == SQLITE_NULL);
  sqlite3_finalize (stmt);

  // Verify entry was saved (in daily_time table now)
  rc = sqlite3_prepare_v2 (db, "SELECT seconds FROM daily_time;", -1, &stmt, NULL);
  g_assert_cmpint (rc, ==, SQLITE_OK);
  g_assert_cmpint (sqlite3_step (stmt), ==, SQLITE_ROW);
  g_assert_cmpint (sqlite3_column_int (stmt, 0), ==, 3600);
  sqlite3_finalize (stmt);

  g_object_unref (db_manager);
}

static void
test_db_report (void)
{
  GTimerDBManager *db_manager;
  GError *error = NULL;

  db_manager = gtimer_db_manager_new (":memory:", &error);
  
  // Insert some data
  sqlite3 *db = gtimer_db_manager_get_db (db_manager);
  sqlite3_exec (db, "INSERT INTO tasks (id, name) VALUES (1, 'Task A'), (2, 'Task B');", NULL, NULL, NULL);
  
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (db, "INSERT INTO daily_time (task_id, date, seconds) VALUES (?, ?, ?);", -1, &stmt, NULL);
  
  // Task A: 1 hour
  sqlite3_bind_int (stmt, 1, 1); sqlite3_bind_text (stmt, 2, "2024-02-09", -1, SQLITE_STATIC); sqlite3_bind_int (stmt, 3, 3600);
  sqlite3_step (stmt); sqlite3_reset (stmt);
  
  // Task B: 2 hours
  sqlite3_bind_int (stmt, 1, 2); sqlite3_bind_text (stmt, 2, "2024-02-09", -1, SQLITE_STATIC); sqlite3_bind_int (stmt, 3, 7200);
  sqlite3_step (stmt);
  
  sqlite3_finalize (stmt);

  GList *report = gtimer_db_manager_get_daily_report (db_manager, 2024, 2, 9);
  g_assert_cmpint (g_list_length (report), ==, 2);
  
  GTimerReportRow *r1 = report->data;
  g_assert_cmpstr (r1->task_name, ==, "Task B"); // Highest duration first
  g_assert_cmpint (r1->total_duration, ==, 7200);

  g_list_free_full (report, (GDestroyNotify)gtimer_report_row_free);
  g_object_unref (db_manager);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/db/init", test_db_init);
  g_test_add_func ("/db/save_task", test_db_save_task);
  g_test_add_func ("/db/report", test_db_report);

  return g_test_run ();
}
