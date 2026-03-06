#include "../core/db-manager.h"
#include "../core/task-object.h"
#include "../core/project-object.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <sqlite3.h>
#include <time.h>
#include <string.h>

/* Test fixture: each test gets a fresh in-memory DB */
typedef struct {
  GTimerDBManager *db;
} DbFixture;

static void
db_fixture_setup (DbFixture *f, gconstpointer data)
{
  (void)data;
  GError *error = NULL;
  f->db = gtimer_db_manager_new (":memory:", &error);
  g_assert_no_error (error);
  g_assert_nonnull (f->db);
}

static void
db_fixture_teardown (DbFixture *f, gconstpointer data)
{
  (void)data;
  g_object_unref (f->db);
}

static void
test_db_add_project (DbFixture *f, gconstpointer data)
{
  (void)data;
  GError *error = NULL;

  GList *projects = gtimer_db_manager_get_projects (f->db);
  g_assert_cmpint (g_list_length (projects), ==, 0);
  g_list_free_full (projects, g_object_unref);

  gtimer_db_manager_create_project (f->db, "Test Project", &error);
  g_assert_no_error (error);

  projects = gtimer_db_manager_get_projects (f->db);
  g_assert_cmpint (g_list_length (projects), ==, 1);

  GTimerProject *p = projects->data;
  g_assert_cmpstr (gtimer_project_get_name (p), ==, "Test Project");

  g_list_free_full (projects, g_object_unref);
}

static void
test_db_add_task (DbFixture *f, gconstpointer data)
{
  (void)data;
  GError *error = NULL;

  GList *tasks = gtimer_db_manager_get_all_tasks (f->db);
  g_assert_cmpint (g_list_length (tasks), ==, 0);
  g_list_free_full (tasks, g_object_unref);

  gtimer_db_manager_create_task (f->db, "Test Task", -1, &error);
  g_assert_no_error (error);

  tasks = gtimer_db_manager_get_all_tasks (f->db);
  g_assert_cmpint (g_list_length (tasks), ==, 1);

  GTimerTask *t = tasks->data;
  g_assert_cmpstr (gtimer_task_get_name (t), ==, "Test Task");

  g_list_free_full (tasks, g_object_unref);
}

static void
test_db_hide_unhide_task (DbFixture *f, gconstpointer data)
{
  (void)data;
  GError *error = NULL;

  gtimer_db_manager_create_task (f->db, "Hide Test Task", -1, &error);
  g_assert_no_error (error);

  GList *tasks = gtimer_db_manager_get_all_tasks (f->db);
  GTimerTask *task = GTIMER_TASK (tasks->data);
  int task_id = gtimer_task_get_id (task);
  g_list_free_full (tasks, g_object_unref);

  /* Hide it */
  gtimer_db_manager_hide_task (f->db, task_id, TRUE, &error);
  g_assert_no_error (error);

  tasks = gtimer_db_manager_get_all_tasks (f->db);
  gboolean found = FALSE;
  for (GList *l = tasks; l != NULL; l = l->next) {
    GTimerTask *t = GTIMER_TASK (l->data);
    if (gtimer_task_get_id (t) == task_id) {
      found = TRUE;
      g_assert_true (gtimer_task_is_hidden (t));
    }
  }
  g_list_free_full (tasks, g_object_unref);
  g_assert_true (found);

  /* Unhide it */
  gtimer_db_manager_hide_task (f->db, task_id, FALSE, &error);
  g_assert_no_error (error);

  tasks = gtimer_db_manager_get_all_tasks (f->db);
  for (GList *l = tasks; l != NULL; l = l->next) {
    GTimerTask *t = GTIMER_TASK (l->data);
    if (gtimer_task_get_id (t) == task_id)
      g_assert_false (gtimer_task_is_hidden (t));
  }
  g_list_free_full (tasks, g_object_unref);
}

static void
test_db_annotations (DbFixture *f, gconstpointer data)
{
  (void)data;
  GError *error = NULL;

  gtimer_db_manager_create_task (f->db, "Annotation Task", -1, &error);
  g_assert_no_error (error);

  GList *tasks = gtimer_db_manager_get_all_tasks (f->db);
  int task_id = gtimer_task_get_id (GTIMER_TASK (tasks->data));
  g_list_free_full (tasks, g_object_unref);

  gtimer_db_manager_add_annotation (f->db, task_id, "Test annotation");

  GList *annotations = gtimer_db_manager_get_annotations (f->db, task_id);
  g_assert_nonnull (annotations);
  g_assert_cmpint (g_list_length (annotations), ==, 1);

  g_list_free_full (annotations, (GDestroyNotify)gtimer_annotation_free);
}

static void
test_db_update_task (DbFixture *f, gconstpointer data)
{
  (void)data;
  GError *error = NULL;

  gtimer_db_manager_create_project (f->db, "Update Project", &error);
  g_assert_no_error (error);

  GList *projects = gtimer_db_manager_get_projects (f->db);
  int project_id = gtimer_project_get_id (projects->data);
  g_list_free_full (projects, g_object_unref);

  gtimer_db_manager_create_task (f->db, "Before Update", project_id, &error);
  g_assert_no_error (error);

  GList *tasks = gtimer_db_manager_get_all_tasks (f->db);
  int task_id = gtimer_task_get_id (tasks->data);
  g_assert_cmpint (gtimer_task_get_project_id (GTIMER_TASK (tasks->data)), ==, project_id);
  g_list_free_full (tasks, g_object_unref);

  /* Update name and remove project */
  gtimer_db_manager_update_task (f->db, task_id, "After Update", -1, &error);
  g_assert_no_error (error);

  tasks = gtimer_db_manager_get_all_tasks (f->db);
  GTimerTask *t = GTIMER_TASK (tasks->data);
  g_assert_cmpstr (gtimer_task_get_name (t), ==, "After Update");
  g_assert_null (gtimer_task_get_project_name (t));
  g_list_free_full (tasks, g_object_unref);

  /* Reassign project */
  gtimer_db_manager_update_task (f->db, task_id, "After Update", project_id, &error);
  g_assert_no_error (error);

  tasks = gtimer_db_manager_get_all_tasks (f->db);
  t = GTIMER_TASK (tasks->data);
  g_assert_cmpint (gtimer_task_get_project_id (t), ==, project_id);
  g_assert_cmpstr (gtimer_task_get_project_name (t), ==, "Update Project");
  g_list_free_full (tasks, g_object_unref);
}

static void
test_db_update_project (DbFixture *f, gconstpointer data)
{
  (void)data;
  GError *error = NULL;

  gtimer_db_manager_create_project (f->db, "Old Name", &error);
  g_assert_no_error (error);

  GList *projects = gtimer_db_manager_get_projects (f->db);
  int project_id = gtimer_project_get_id (projects->data);
  g_list_free_full (projects, g_object_unref);

  gtimer_db_manager_update_project (f->db, project_id, "New Name", &error);
  g_assert_no_error (error);

  projects = gtimer_db_manager_get_projects (f->db);
  g_assert_cmpstr (gtimer_project_get_name (projects->data), ==, "New Name");
  g_list_free_full (projects, g_object_unref);

  /* Verify tasks see updated project name */
  gtimer_db_manager_create_task (f->db, "Project Name Task", project_id, &error);
  g_assert_no_error (error);

  GList *tasks = gtimer_db_manager_get_all_tasks (f->db);
  for (GList *l = tasks; l != NULL; l = l->next) {
    GTimerTask *t = l->data;
    if (g_strcmp0 (gtimer_task_get_name (t), "Project Name Task") == 0)
      g_assert_cmpstr (gtimer_task_get_project_name (t), ==, "New Name");
  }
  g_list_free_full (tasks, g_object_unref);
}

static void
test_db_midnight_rollover (DbFixture *f, gconstpointer data)
{
  (void)data;
  GError *error = NULL;

  gtimer_db_manager_create_task (f->db, "Rollover Task", -1, &error);
  g_assert_no_error (error);

  GList *tasks = gtimer_db_manager_get_all_tasks (f->db);
  int task_id = gtimer_task_get_id (tasks->data);
  g_list_free_full (tasks, g_object_unref);

  /* Set is_timing and last_start_time to 11:30 PM yesterday */
  time_t now = time (NULL);
  struct tm tm;
  localtime_r (&now, &tm);
  tm.tm_mday--;
  tm.tm_hour = 23;
  tm.tm_min = 30;
  tm.tm_sec = 0;
  time_t yesterday_night = mktime (&tm);

  sqlite3 *sql_db = gtimer_db_manager_get_db (f->db);
  char *sql = g_strdup_printf (
      "UPDATE tasks SET is_timing = 1, last_start_time = %ld WHERE id = %d;",
      (long)yesterday_night, task_id);
  sqlite3_exec (sql_db, sql, NULL, NULL, NULL);
  g_free (sql);

  /* Stop timing now. Should split 30 mins to yesterday and ~N mins to today */
  gtimer_db_manager_stop_task_timing (f->db, task_id);

  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (sql_db,
      "SELECT date, seconds FROM daily_time WHERE task_id = ? ORDER BY date ASC;",
      -1, &stmt, NULL);
  sqlite3_bind_int (stmt, 1, task_id);

  /* Yesterday's entry: 1800 seconds */
  g_assert_cmpint (sqlite3_step (stmt), ==, SQLITE_ROW);
  g_assert_cmpint (sqlite3_column_int (stmt, 1), ==, 1800);

  /* Today's entry: > 0 seconds */
  g_assert_cmpint (sqlite3_step (stmt), ==, SQLITE_ROW);
  g_assert_cmpint (sqlite3_column_int (stmt, 1), >, 0);

  sqlite3_finalize (stmt);
}

static void
test_db_project_deletion_fk (DbFixture *f, gconstpointer data)
{
  (void)data;
  GError *error = NULL;

  gtimer_db_manager_create_project (f->db, "FK Project", &error);
  g_assert_no_error (error);

  GList *projects = gtimer_db_manager_get_projects (f->db);
  int project_id = gtimer_project_get_id (projects->data);
  g_list_free_full (projects, g_object_unref);

  gtimer_db_manager_create_task (f->db, "FK Task", project_id, &error);
  g_assert_no_error (error);

  GList *tasks = gtimer_db_manager_get_all_tasks (f->db);
  int task_id = gtimer_task_get_id (tasks->data);
  g_assert_cmpint (gtimer_task_get_project_id (GTIMER_TASK (tasks->data)), ==, project_id);
  g_list_free_full (tasks, g_object_unref);

  /* Delete project via SQL to test ON DELETE behavior */
  sqlite3 *sql_db = gtimer_db_manager_get_db (f->db);
  char *sql = g_strdup_printf ("DELETE FROM projects WHERE id = %d;", project_id);
  sqlite3_exec (sql_db, sql, NULL, NULL, NULL);
  g_free (sql);

  /* Verify task's project is now NULL */
  tasks = gtimer_db_manager_get_all_tasks (f->db);
  for (GList *l = tasks; l != NULL; l = l->next) {
    GTimerTask *t = l->data;
    if (gtimer_task_get_id (t) == task_id)
      g_assert_null (gtimer_task_get_project_name (t));
  }
  g_list_free_full (tasks, g_object_unref);
}

static void
test_db_migration_schema (DbFixture *f, gconstpointer data)
{
  (void)f; (void)data;

  /* Create a raw SQLite DB with an old schema */
  char *db_path = g_build_filename (g_get_tmp_dir (), "gtimer-migration-test.db", NULL);
  g_remove (db_path);

  sqlite3 *raw_db;
  sqlite3_open (db_path, &raw_db);
  sqlite3_exec (raw_db, "CREATE TABLE tasks (id INTEGER PRIMARY KEY, name TEXT);", NULL, NULL, NULL);
  sqlite3_close (raw_db);

  /* Open with DB Manager - should trigger migration */
  GError *error = NULL;
  GTimerDBManager *mig_db = gtimer_db_manager_new (db_path, &error);
  g_assert_no_error (error);
  g_assert_nonnull (mig_db);

  /* Verify new columns exist */
  sqlite3 *sql_db = gtimer_db_manager_get_db (mig_db);
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2 (sql_db,
      "SELECT is_hidden, is_timing, last_start_time FROM tasks;", -1, &stmt, NULL);
  g_assert_cmpint (rc, ==, SQLITE_OK);
  sqlite3_finalize (stmt);

  g_object_unref (mig_db);
  g_remove (db_path);
  g_free (db_path);
}

static void
test_db_get_hidden_tasks (DbFixture *f, gconstpointer data)
{
  (void)data;
  GError *error = NULL;

  gtimer_db_manager_create_task (f->db, "Visible Task", -1, &error);
  g_assert_no_error (error);
  gtimer_db_manager_create_task (f->db, "Hidden Task", -1, &error);
  g_assert_no_error (error);

  /* Find the task to hide */
  GList *tasks = gtimer_db_manager_get_all_tasks (f->db);
  int hidden_id = 0;
  for (GList *l = tasks; l != NULL; l = l->next) {
    GTimerTask *t = l->data;
    if (g_strcmp0 (gtimer_task_get_name (t), "Hidden Task") == 0)
      hidden_id = gtimer_task_get_id (t);
  }
  g_list_free_full (tasks, g_object_unref);
  g_assert_cmpint (hidden_id, >, 0);

  gtimer_db_manager_hide_task (f->db, hidden_id, TRUE, &error);
  g_assert_no_error (error);

  GList *hidden = gtimer_db_manager_get_hidden_tasks (f->db);
  g_assert_nonnull (hidden);
  g_assert_cmpint (g_list_length (hidden), ==, 1);

  GTimerTask *ht = hidden->data;
  g_assert_true (gtimer_task_is_hidden (ht));
  g_assert_cmpint (gtimer_task_get_id (ht), ==, hidden_id);

  g_list_free_full (hidden, g_object_unref);
}

static void
test_db_task_time_queries (DbFixture *f, gconstpointer data)
{
  (void)data;
  GError *error = NULL;

  gtimer_db_manager_create_task (f->db, "Time Query Task", -1, &error);
  g_assert_no_error (error);

  GList *tasks = gtimer_db_manager_get_all_tasks (f->db);
  int task_id = gtimer_task_get_id (tasks->data);
  g_list_free_full (tasks, g_object_unref);

  /* No time yet */
  g_assert_cmpint (gtimer_db_manager_get_task_total_time (f->db, task_id), ==, 0);
  g_assert_cmpint (gtimer_db_manager_get_task_today_time (f->db, task_id), ==, 0);

  /* Add time for today */
  gtimer_db_manager_add_task_time (f->db, task_id, 3600);
  g_assert_cmpint (gtimer_db_manager_get_task_today_time (f->db, task_id), ==, 3600);
  g_assert_cmpint (gtimer_db_manager_get_task_total_time (f->db, task_id), ==, 3600);

  /* Add more time (accumulates) */
  gtimer_db_manager_add_task_time (f->db, task_id, 1800);
  g_assert_cmpint (gtimer_db_manager_get_task_today_time (f->db, task_id), ==, 5400);
  g_assert_cmpint (gtimer_db_manager_get_task_total_time (f->db, task_id), ==, 5400);

  /* Add time for a different date (only affects total) */
  gtimer_db_manager_add_task_time_for_date (f->db, task_id, "2020-01-01", 900);
  g_assert_cmpint (gtimer_db_manager_get_task_today_time (f->db, task_id), ==, 5400);
  g_assert_cmpint (gtimer_db_manager_get_task_total_time (f->db, task_id), ==, 6300);

  /* set_task_today_time overwrites */
  gtimer_db_manager_set_task_today_time (f->db, task_id, 100);
  g_assert_cmpint (gtimer_db_manager_get_task_today_time (f->db, task_id), ==, 100);
  g_assert_cmpint (gtimer_db_manager_get_task_total_time (f->db, task_id), ==, 1000);

  /* Nonexistent task returns 0 */
  g_assert_cmpint (gtimer_db_manager_get_task_total_time (f->db, 99999), ==, 0);
  g_assert_cmpint (gtimer_db_manager_get_task_today_time (f->db, 99999), ==, 0);
}

static void
test_db_start_stop_timing (DbFixture *f, gconstpointer data)
{
  (void)data;
  GError *error = NULL;

  gtimer_db_manager_create_task (f->db, "Timing Task", -1, &error);
  g_assert_no_error (error);

  GList *tasks = gtimer_db_manager_get_all_tasks (f->db);
  int task_id = gtimer_task_get_id (tasks->data);
  g_list_free_full (tasks, g_object_unref);

  g_assert_false (gtimer_db_manager_is_task_timing (f->db, task_id));

  gtimer_db_manager_start_task_timing (f->db, task_id);
  g_assert_true (gtimer_db_manager_is_task_timing (f->db, task_id));

  gtimer_db_manager_stop_task_timing (f->db, task_id);
  g_assert_false (gtimer_db_manager_is_task_timing (f->db, task_id));

  g_assert_false (gtimer_db_manager_is_task_timing (f->db, 99999));
}

static void
test_db_concurrent_timing (DbFixture *f, gconstpointer data)
{
  (void)data;
  GError *error = NULL;

  gtimer_db_manager_create_task (f->db, "Task A", -1, &error);
  g_assert_no_error (error);
  gtimer_db_manager_create_task (f->db, "Task B", -1, &error);
  g_assert_no_error (error);

  GList *tasks = gtimer_db_manager_get_all_tasks (f->db);
  int id_a = 0, id_b = 0;
  for (GList *l = tasks; l != NULL; l = l->next) {
    GTimerTask *t = l->data;
    if (g_strcmp0 (gtimer_task_get_name (t), "Task A") == 0)
      id_a = gtimer_task_get_id (t);
    else if (g_strcmp0 (gtimer_task_get_name (t), "Task B") == 0)
      id_b = gtimer_task_get_id (t);
  }
  g_list_free_full (tasks, g_object_unref);
  g_assert_cmpint (id_a, >, 0);
  g_assert_cmpint (id_b, >, 0);

  /* Start both tasks simultaneously */
  gtimer_db_manager_start_task_timing (f->db, id_a);
  gtimer_db_manager_start_task_timing (f->db, id_b);

  g_assert_true (gtimer_db_manager_is_task_timing (f->db, id_a));
  g_assert_true (gtimer_db_manager_is_task_timing (f->db, id_b));

  /* Add time to simulate elapsed duration */
  gtimer_db_manager_add_task_time (f->db, id_a, 600);
  gtimer_db_manager_add_task_time (f->db, id_b, 300);

  /* Stop task A - should not affect task B */
  gtimer_db_manager_stop_task_timing (f->db, id_a);
  g_assert_false (gtimer_db_manager_is_task_timing (f->db, id_a));
  g_assert_true (gtimer_db_manager_is_task_timing (f->db, id_b));

  /* Add more time to B while A is stopped */
  gtimer_db_manager_add_task_time (f->db, id_b, 200);

  /* Stop task B */
  gtimer_db_manager_stop_task_timing (f->db, id_b);
  g_assert_false (gtimer_db_manager_is_task_timing (f->db, id_b));

  /* Verify independent time tracking */
  gint64 total_a = gtimer_db_manager_get_task_total_time (f->db, id_a);
  gint64 total_b = gtimer_db_manager_get_task_total_time (f->db, id_b);

  g_assert_cmpint (total_a, >=, 600);
  g_assert_cmpint (total_b, >=, 500);
}

static void
test_db_tags (DbFixture *f, gconstpointer data)
{
  (void)data;
  GError *error = NULL;

  /* Create two tasks */
  gtimer_db_manager_create_task (f->db, "Tagged Task A", -1, &error);
  g_assert_no_error (error);
  gtimer_db_manager_create_task (f->db, "Tagged Task B", -1, &error);
  g_assert_no_error (error);

  /* Add tags */
  gtimer_db_manager_add_tag_to_task (f->db, 1, "meeting");
  gtimer_db_manager_add_tag_to_task (f->db, 1, "billable");
  gtimer_db_manager_add_tag_to_task (f->db, 2, "meeting");

  /* Verify task tags */
  GList *tags1 = gtimer_db_manager_get_task_tags (f->db, 1);
  g_assert_cmpint (g_list_length (tags1), ==, 2);
  /* Tags are ordered by name: billable, meeting */
  g_assert_cmpstr ((char *)tags1->data, ==, "billable");
  g_assert_cmpstr ((char *)tags1->next->data, ==, "meeting");
  g_list_free_full (tags1, g_free);

  GList *tags2 = gtimer_db_manager_get_task_tags (f->db, 2);
  g_assert_cmpint (g_list_length (tags2), ==, 1);
  g_assert_cmpstr ((char *)tags2->data, ==, "meeting");
  g_list_free_full (tags2, g_free);

  /* Verify all tags */
  GList *all_tags = gtimer_db_manager_get_all_tags (f->db);
  g_assert_cmpint (g_list_length (all_tags), ==, 2);
  g_list_free_full (all_tags, g_free);

  /* Verify tasks by tag */
  GList *meeting_tasks = gtimer_db_manager_get_tasks_by_tag (f->db, "meeting");
  g_assert_cmpint (g_list_length (meeting_tasks), ==, 2);
  g_list_free (meeting_tasks);

  GList *billable_tasks = gtimer_db_manager_get_tasks_by_tag (f->db, "billable");
  g_assert_cmpint (g_list_length (billable_tasks), ==, 1);
  g_list_free (billable_tasks);

  /* Remove tag */
  gtimer_db_manager_remove_tag_from_task (f->db, 1, "billable");
  tags1 = gtimer_db_manager_get_task_tags (f->db, 1);
  g_assert_cmpint (g_list_length (tags1), ==, 1);
  g_assert_cmpstr ((char *)tags1->data, ==, "meeting");
  g_list_free_full (tags1, g_free);

  /* Adding duplicate tag is idempotent */
  gtimer_db_manager_add_tag_to_task (f->db, 1, "meeting");
  tags1 = gtimer_db_manager_get_task_tags (f->db, 1);
  g_assert_cmpint (g_list_length (tags1), ==, 1);
  g_list_free_full (tags1, g_free);

  /* Tags appear in get_all_tasks via task object */
  GList *tasks = gtimer_db_manager_get_all_tasks (f->db);
  for (GList *l = tasks; l != NULL; l = l->next) {
    GTimerTask *task = GTIMER_TASK (l->data);
    if (gtimer_task_get_id (task) == 1) {
      g_assert_nonnull (gtimer_task_get_tags (task));
      g_assert_nonnull (strstr (gtimer_task_get_tags (task), "meeting"));
    }
  }
  g_list_free_full (tasks, g_object_unref);
}

static void
test_db_tag_cascade_delete (DbFixture *f, gconstpointer data)
{
  (void)data;
  GError *error = NULL;

  gtimer_db_manager_create_task (f->db, "Doomed Task", -1, &error);
  g_assert_no_error (error);

  gtimer_db_manager_add_tag_to_task (f->db, 1, "temp-tag");

  /* Delete the task - should cascade delete task_tags */
  gtimer_db_manager_delete_task (f->db, 1, &error);
  g_assert_no_error (error);

  /* Tag should still exist in tags table but no task_tags rows */
  GList *all_tags = gtimer_db_manager_get_all_tags (f->db);
  g_assert_cmpint (g_list_length (all_tags), ==, 1);
  g_list_free_full (all_tags, g_free);

  GList *tasks_for_tag = gtimer_db_manager_get_tasks_by_tag (f->db, "temp-tag");
  g_assert_cmpint (g_list_length (tasks_for_tag), ==, 0);
  g_list_free (tasks_for_tag);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/db/add_project", DbFixture, NULL,
      db_fixture_setup, test_db_add_project, db_fixture_teardown);
  g_test_add ("/db/add_task", DbFixture, NULL,
      db_fixture_setup, test_db_add_task, db_fixture_teardown);
  g_test_add ("/db/hide_unhide_task", DbFixture, NULL,
      db_fixture_setup, test_db_hide_unhide_task, db_fixture_teardown);
  g_test_add ("/db/annotations", DbFixture, NULL,
      db_fixture_setup, test_db_annotations, db_fixture_teardown);
  g_test_add ("/db/update_task", DbFixture, NULL,
      db_fixture_setup, test_db_update_task, db_fixture_teardown);
  g_test_add ("/db/update_project", DbFixture, NULL,
      db_fixture_setup, test_db_update_project, db_fixture_teardown);
  g_test_add ("/db/midnight_rollover", DbFixture, NULL,
      db_fixture_setup, test_db_midnight_rollover, db_fixture_teardown);
  g_test_add ("/db/project_deletion_fk", DbFixture, NULL,
      db_fixture_setup, test_db_project_deletion_fk, db_fixture_teardown);
  g_test_add ("/db/migration_schema", DbFixture, NULL,
      db_fixture_setup, test_db_migration_schema, db_fixture_teardown);
  g_test_add ("/db/get_hidden_tasks", DbFixture, NULL,
      db_fixture_setup, test_db_get_hidden_tasks, db_fixture_teardown);
  g_test_add ("/db/task_time_queries", DbFixture, NULL,
      db_fixture_setup, test_db_task_time_queries, db_fixture_teardown);
  g_test_add ("/db/start_stop_timing", DbFixture, NULL,
      db_fixture_setup, test_db_start_stop_timing, db_fixture_teardown);
  g_test_add ("/db/concurrent_timing", DbFixture, NULL,
      db_fixture_setup, test_db_concurrent_timing, db_fixture_teardown);
  g_test_add ("/db/tags", DbFixture, NULL,
      db_fixture_setup, test_db_tags, db_fixture_teardown);
  g_test_add ("/db/tag_cascade_delete", DbFixture, NULL,
      db_fixture_setup, test_db_tag_cascade_delete, db_fixture_teardown);

  return g_test_run ();
}
