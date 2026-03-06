#include "../core/db-manager.h"
#include "../core/task-object.h"
#include "../core/project-object.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <sqlite3.h>
#include <time.h>

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

static void
test_db_update_task(void)
{
  GError *error = NULL;

  /* Create a project and a task */
  gtimer_db_manager_create_project(db, "Update Project", &error);
  g_assert_no_error(error);

  GList *projects = gtimer_db_manager_get_projects(db);
  int project_id = 0;
  for (GList *l = projects; l != NULL; l = l->next) {
    GTimerProject *p = l->data;
    if (g_strcmp0(gtimer_project_get_name(p), "Update Project") == 0)
      project_id = gtimer_project_get_id(p);
  }
  g_list_free_full(projects, g_object_unref);
  g_assert_cmpint(project_id, >, 0);

  gtimer_db_manager_create_task(db, "Before Update", project_id, &error);
  g_assert_no_error(error);

  /* Find the task we just created */
  GList *tasks = gtimer_db_manager_get_all_tasks(db);
  int task_id = 0;
  for (GList *l = tasks; l != NULL; l = l->next) {
    GTimerTask *t = l->data;
    if (g_strcmp0(gtimer_task_get_name(t), "Before Update") == 0) {
      task_id = gtimer_task_get_id(t);
      g_assert_cmpint(gtimer_task_get_project_id(t), ==, project_id);
    }
  }
  g_list_free_full(tasks, g_object_unref);
  g_assert_cmpint(task_id, >, 0);

  /* Update the task name and remove project */
  gtimer_db_manager_update_task(db, task_id, "After Update", -1, &error);
  g_assert_no_error(error);

  /* Verify the update */
  tasks = gtimer_db_manager_get_all_tasks(db);
  gboolean found = FALSE;
  for (GList *l = tasks; l != NULL; l = l->next) {
    GTimerTask *t = l->data;
    if (gtimer_task_get_id(t) == task_id) {
      found = TRUE;
      g_assert_cmpstr(gtimer_task_get_name(t), ==, "After Update");
      g_assert_null(gtimer_task_get_project_name(t));
    }
  }
  g_list_free_full(tasks, g_object_unref);
  g_assert_true(found);

  /* Update with a project assignment */
  gtimer_db_manager_update_task(db, task_id, "After Update", project_id, &error);
  g_assert_no_error(error);

  tasks = gtimer_db_manager_get_all_tasks(db);
  for (GList *l = tasks; l != NULL; l = l->next) {
    GTimerTask *t = l->data;
    if (gtimer_task_get_id(t) == task_id) {
      g_assert_cmpint(gtimer_task_get_project_id(t), ==, project_id);
      g_assert_cmpstr(gtimer_task_get_project_name(t), ==, "Update Project");
    }
  }
  g_list_free_full(tasks, g_object_unref);
}

static void
test_db_update_project(void)
{
  GError *error = NULL;

  gtimer_db_manager_create_project(db, "Old Name", &error);
  g_assert_no_error(error);

  /* Find the project */
  GList *projects = gtimer_db_manager_get_projects(db);
  int project_id = 0;
  for (GList *l = projects; l != NULL; l = l->next) {
    GTimerProject *p = l->data;
    if (g_strcmp0(gtimer_project_get_name(p), "Old Name") == 0)
      project_id = gtimer_project_get_id(p);
  }
  g_list_free_full(projects, g_object_unref);
  g_assert_cmpint(project_id, >, 0);

  /* Rename it */
  gtimer_db_manager_update_project(db, project_id, "New Name", &error);
  g_assert_no_error(error);

  /* Verify */
  projects = gtimer_db_manager_get_projects(db);
  gboolean found = FALSE;
  for (GList *l = projects; l != NULL; l = l->next) {
    GTimerProject *p = l->data;
    if (gtimer_project_get_id(p) == project_id) {
      found = TRUE;
      g_assert_cmpstr(gtimer_project_get_name(p), ==, "New Name");
    }
  }
  g_list_free_full(projects, g_object_unref);
  g_assert_true(found);

  /* Verify tasks with this project see the updated name */
  gtimer_db_manager_create_task(db, "Project Name Task", project_id, &error);
  g_assert_no_error(error);

  GList *tasks = gtimer_db_manager_get_all_tasks(db);
  for (GList *l = tasks; l != NULL; l = l->next) {
    GTimerTask *t = l->data;
    if (g_strcmp0(gtimer_task_get_name(t), "Project Name Task") == 0) {
      g_assert_cmpstr(gtimer_task_get_project_name(t), ==, "New Name");
    }
  }
  g_list_free_full(tasks, g_object_unref);
}

static void
test_db_midnight_rollover(void)
{
  GError *error = NULL;
  gtimer_db_manager_create_task(db, "Rollover Task", -1, &error);
  g_assert_no_error(error);
  
  /* Manually set is_timing and last_start_time to 11:30 PM yesterday */
  time_t now = time(NULL);
  struct tm tm;
  localtime_r(&now, &tm);
  tm.tm_mday--;
  tm.tm_hour = 23;
  tm.tm_min = 30;
  tm.tm_sec = 0;
  time_t yesterday_night = mktime(&tm);
  
  sqlite3 *sql_db = gtimer_db_manager_get_db(db);
  char *sql = g_strdup_printf("UPDATE tasks SET is_timing = 1, last_start_time = %ld WHERE id = 1;", (long)yesterday_night);
  sqlite3_exec(sql_db, sql, NULL, NULL, NULL);
  g_free(sql);
  
  /* Stop timing now. It should split 30 mins to yesterday and ~N mins to today */
  gtimer_db_manager_stop_task_timing(db, 1);
  
  /* Verify daily_time entries */
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2(sql_db, "SELECT date, seconds FROM daily_time WHERE task_id = 1 ORDER BY date ASC;", -1, &stmt, NULL);
  
  /* Yesterday's entry: 1800 seconds */
  g_assert_cmpint(sqlite3_step(stmt), ==, SQLITE_ROW);
  g_assert_cmpint(sqlite3_column_int(stmt, 1), ==, 1800);
  
  /* Today's entry: > 0 seconds */
  g_assert_cmpint(sqlite3_step(stmt), ==, SQLITE_ROW);
  g_assert_cmpint(sqlite3_column_int(stmt, 1), >, 0);
  
  sqlite3_finalize(stmt);
}

static void
test_db_project_deletion_fk(void)
{
  GError *error = NULL;
  
  /* Create project and task */
  gtimer_db_manager_create_project(db, "FK Project", &error);
  g_assert_no_error(error);
  
  /* Get project ID (should be 2, as 1 was created in previous test) */
  GList *projects = gtimer_db_manager_get_projects(db);
  int project_id = 0;
  for (GList *l = projects; l != NULL; l = l->next) {
    GTimerProject *p = l->data;
    if (g_strcmp0(gtimer_project_get_name(p), "FK Project") == 0) {
      project_id = gtimer_project_get_id(p);
    }
    g_object_unref(p);
  }
  g_list_free(projects);
  g_assert_cmpint(project_id, >, 0);
  
  gtimer_db_manager_create_task(db, "FK Task", project_id, &error);
  g_assert_no_error(error);
  
  /* Get task ID */
  GList *tasks = gtimer_db_manager_get_all_tasks(db);
  int task_id = 0;
  for (GList *l = tasks; l != NULL; l = l->next) {
    GTimerTask *t = l->data;
    if (g_strcmp0(gtimer_task_get_name(t), "FK Task") == 0) {
      task_id = gtimer_task_get_id(t);
      g_assert_cmpint(gtimer_task_get_project_id(t), ==, project_id);
    }
    g_object_unref(t);
  }
  g_list_free(tasks);
  g_assert_cmpint(task_id, >, 0);
  
  /* Delete project - requires manual SQL as delete_project isn't exposed in public API yet, 
     but we want to test the DB schema ON DELETE behavior */
  sqlite3 *sql_db = gtimer_db_manager_get_db(db);
  char *sql = g_strdup_printf("DELETE FROM projects WHERE id = %d;", project_id);
  sqlite3_exec(sql_db, sql, NULL, NULL, NULL);
  g_free(sql);
  
  /* Verify task's project_id is now NULL (or -1 in our GTimerTask representation) */
  tasks = gtimer_db_manager_get_all_tasks(db);
  for (GList *l = tasks; l != NULL; l = l->next) {
    GTimerTask *t = l->data;
    if (gtimer_task_get_id(t) == task_id) {
      /* In GTimerTask, a NULL project in DB maps to project_id -1 or 0? 
         Let's check task-object.c or just check project_name is NULL */
      g_assert_null(gtimer_task_get_project_name(t));
    }
    g_object_unref(t);
  }
  g_list_free(tasks);
}

static void
test_db_migration_schema(void)
{
  /* Create a raw SQLite DB with an old schema */
  char *db_path = g_build_filename(g_get_tmp_dir(), "gtimer-migration-test.db", NULL);
  g_remove(db_path);
  
  sqlite3 *raw_db;
  sqlite3_open(db_path, &raw_db);
  sqlite3_exec(raw_db, "CREATE TABLE tasks (id INTEGER PRIMARY KEY, name TEXT);", NULL, NULL, NULL);
  sqlite3_close(raw_db);
  
  /* Open with DB Manager - should trigger migration */
  GError *error = NULL;
  GTimerDBManager *mig_db = gtimer_db_manager_new(db_path, &error);
  g_assert_no_error(error);
  g_assert_nonnull(mig_db);
  
  /* Verify new columns exist by trying to use them */
  sqlite3 *sql_db = gtimer_db_manager_get_db(mig_db);
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(sql_db, "SELECT is_hidden, is_timing, last_start_time FROM tasks;", -1, &stmt, NULL);
  g_assert_cmpint(rc, ==, SQLITE_OK);
  sqlite3_finalize(stmt);
  
  g_object_unref(mig_db);
  g_remove(db_path);
  g_free(db_path);
}

static void
test_db_get_hidden_tasks(void)
{
  GError *error = NULL;

  /* Create two tasks, hide one */
  gtimer_db_manager_create_task(db, "Visible Task", -1, &error);
  g_assert_no_error(error);
  gtimer_db_manager_create_task(db, "Hidden Task", -1, &error);
  g_assert_no_error(error);

  /* Find the task to hide */
  GList *tasks = gtimer_db_manager_get_all_tasks(db);
  int hidden_id = 0;
  for (GList *l = tasks; l != NULL; l = l->next) {
    GTimerTask *t = l->data;
    if (g_strcmp0(gtimer_task_get_name(t), "Hidden Task") == 0)
      hidden_id = gtimer_task_get_id(t);
  }
  g_list_free_full(tasks, g_object_unref);
  g_assert_cmpint(hidden_id, >, 0);

  gtimer_db_manager_hide_task(db, hidden_id, TRUE, &error);
  g_assert_no_error(error);

  /* get_hidden_tasks should return only hidden tasks */
  GList *hidden = gtimer_db_manager_get_hidden_tasks(db);
  g_assert_nonnull(hidden);

  gboolean found = FALSE;
  for (GList *l = hidden; l != NULL; l = l->next) {
    GTimerTask *t = l->data;
    g_assert_true(gtimer_task_is_hidden(t));
    if (gtimer_task_get_id(t) == hidden_id)
      found = TRUE;
  }
  g_list_free_full(hidden, g_object_unref);
  g_assert_true(found);

  /* Unhide for cleanup */
  gtimer_db_manager_hide_task(db, hidden_id, FALSE, &error);
  g_assert_no_error(error);
}

static void
test_db_task_time_queries(void)
{
  GError *error = NULL;

  gtimer_db_manager_create_task(db, "Time Query Task", -1, &error);
  g_assert_no_error(error);

  /* Find the task ID */
  GList *tasks = gtimer_db_manager_get_all_tasks(db);
  int task_id = 0;
  for (GList *l = tasks; l != NULL; l = l->next) {
    GTimerTask *t = l->data;
    if (g_strcmp0(gtimer_task_get_name(t), "Time Query Task") == 0)
      task_id = gtimer_task_get_id(t);
  }
  g_list_free_full(tasks, g_object_unref);
  g_assert_cmpint(task_id, >, 0);

  /* No time yet */
  g_assert_cmpint(gtimer_db_manager_get_task_total_time(db, task_id), ==, 0);
  g_assert_cmpint(gtimer_db_manager_get_task_today_time(db, task_id), ==, 0);

  /* Add time for today */
  gtimer_db_manager_add_task_time(db, task_id, 3600);
  g_assert_cmpint(gtimer_db_manager_get_task_today_time(db, task_id), ==, 3600);
  g_assert_cmpint(gtimer_db_manager_get_task_total_time(db, task_id), ==, 3600);

  /* Add more time for today (accumulates) */
  gtimer_db_manager_add_task_time(db, task_id, 1800);
  g_assert_cmpint(gtimer_db_manager_get_task_today_time(db, task_id), ==, 5400);
  g_assert_cmpint(gtimer_db_manager_get_task_total_time(db, task_id), ==, 5400);

  /* Add time for a different date (only affects total, not today) */
  gtimer_db_manager_add_task_time_for_date(db, task_id, "2020-01-01", 900);
  g_assert_cmpint(gtimer_db_manager_get_task_today_time(db, task_id), ==, 5400);
  g_assert_cmpint(gtimer_db_manager_get_task_total_time(db, task_id), ==, 6300);

  /* set_task_today_time overwrites (not accumulates) */
  gtimer_db_manager_set_task_today_time(db, task_id, 100);
  g_assert_cmpint(gtimer_db_manager_get_task_today_time(db, task_id), ==, 100);
  g_assert_cmpint(gtimer_db_manager_get_task_total_time(db, task_id), ==, 1000);

  /* Nonexistent task returns 0 */
  g_assert_cmpint(gtimer_db_manager_get_task_total_time(db, 99999), ==, 0);
  g_assert_cmpint(gtimer_db_manager_get_task_today_time(db, 99999), ==, 0);
}

static void
test_db_start_stop_timing(void)
{
  GError *error = NULL;

  gtimer_db_manager_create_task(db, "Timing Task", -1, &error);
  g_assert_no_error(error);

  GList *tasks = gtimer_db_manager_get_all_tasks(db);
  int task_id = 0;
  for (GList *l = tasks; l != NULL; l = l->next) {
    GTimerTask *t = l->data;
    if (g_strcmp0(gtimer_task_get_name(t), "Timing Task") == 0)
      task_id = gtimer_task_get_id(t);
  }
  g_list_free_full(tasks, g_object_unref);
  g_assert_cmpint(task_id, >, 0);

  /* Not timing initially */
  g_assert_false(gtimer_db_manager_is_task_timing(db, task_id));

  /* Start timing */
  gtimer_db_manager_start_task_timing(db, task_id);
  g_assert_true(gtimer_db_manager_is_task_timing(db, task_id));

  /* Stop timing */
  gtimer_db_manager_stop_task_timing(db, task_id);
  g_assert_false(gtimer_db_manager_is_task_timing(db, task_id));

  /* Nonexistent task returns FALSE */
  g_assert_false(gtimer_db_manager_is_task_timing(db, 99999));
}

static void
test_db_concurrent_timing(void)
{
  /* Use a fresh in-memory DB to avoid interference from shared state */
  GError *error = NULL;
  GTimerDBManager *cdb = gtimer_db_manager_new(":memory:", &error);
  g_assert_no_error(error);

  /* Create two tasks */
  gtimer_db_manager_create_task(cdb, "Task A", -1, &error);
  g_assert_no_error(error);
  gtimer_db_manager_create_task(cdb, "Task B", -1, &error);
  g_assert_no_error(error);

  GList *tasks = gtimer_db_manager_get_all_tasks(cdb);
  int id_a = 0, id_b = 0;
  for (GList *l = tasks; l != NULL; l = l->next) {
    GTimerTask *t = l->data;
    if (g_strcmp0(gtimer_task_get_name(t), "Task A") == 0)
      id_a = gtimer_task_get_id(t);
    else if (g_strcmp0(gtimer_task_get_name(t), "Task B") == 0)
      id_b = gtimer_task_get_id(t);
  }
  g_list_free_full(tasks, g_object_unref);
  g_assert_cmpint(id_a, >, 0);
  g_assert_cmpint(id_b, >, 0);

  /* Start both tasks simultaneously */
  gtimer_db_manager_start_task_timing(cdb, id_a);
  gtimer_db_manager_start_task_timing(cdb, id_b);

  g_assert_true(gtimer_db_manager_is_task_timing(cdb, id_a));
  g_assert_true(gtimer_db_manager_is_task_timing(cdb, id_b));

  /* Manually add time to simulate elapsed duration (avoid sleeps) */
  gtimer_db_manager_add_task_time(cdb, id_a, 600);
  gtimer_db_manager_add_task_time(cdb, id_b, 300);

  /* Stop task A - should not affect task B */
  gtimer_db_manager_stop_task_timing(cdb, id_a);
  g_assert_false(gtimer_db_manager_is_task_timing(cdb, id_a));
  g_assert_true(gtimer_db_manager_is_task_timing(cdb, id_b));

  /* Add more time to B while A is stopped */
  gtimer_db_manager_add_task_time(cdb, id_b, 200);

  /* Stop task B */
  gtimer_db_manager_stop_task_timing(cdb, id_b);
  g_assert_false(gtimer_db_manager_is_task_timing(cdb, id_b));

  /* Verify independent time tracking */
  gint64 total_a = gtimer_db_manager_get_task_total_time(cdb, id_a);
  gint64 total_b = gtimer_db_manager_get_task_total_time(cdb, id_b);

  /* Task A: 600s from manual add (stop_task_timing adds 0 because
     last_start_time is set by strftime('%s','now') and stop is immediate) */
  g_assert_cmpint(total_a, >=, 600);
  /* Task B: 300 + 200 = 500s minimum */
  g_assert_cmpint(total_b, >=, 500);

  g_object_unref(cdb);
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
  g_test_add_func("/db/update_task", test_db_update_task);
  g_test_add_func("/db/update_project", test_db_update_project);
  g_test_add_func("/db/midnight_rollover", test_db_midnight_rollover);
  g_test_add_func("/db/project_deletion_fk", test_db_project_deletion_fk);
  g_test_add_func("/db/migration_schema", test_db_migration_schema);
  g_test_add_func("/db/get_hidden_tasks", test_db_get_hidden_tasks);
  g_test_add_func("/db/task_time_queries", test_db_task_time_queries);
  g_test_add_func("/db/start_stop_timing", test_db_start_stop_timing);
  g_test_add_func("/db/concurrent_timing", test_db_concurrent_timing);

  int result = g_test_run();
  
  g_object_unref(db);
  
  return result;
}
