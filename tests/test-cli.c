#include <glib.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../core/db-manager.h"
#include "../core/task-object.h"

/* GTIMER_BINARY is defined in meson.build */

static gboolean
run_gtimer (const char *args, char **output, int *exit_status)
{
  char *command = g_strdup_printf ("%s %s", GTIMER_BINARY, args);
  char *stdout_text = NULL;
  char *stderr_text = NULL;
  GError *error = NULL;
  
  gboolean success = g_spawn_command_line_sync (command, &stdout_text, &stderr_text, exit_status, &error);
  g_free (command);
  
  if (error) {
    g_printerr ("Spawn error: %s\n", error->message);
    g_error_free (error);
    return FALSE;
  }
  
  /* Combine output */
  GString *combined = g_string_new (NULL);
  if (stdout_text) g_string_append (combined, stdout_text);
  if (stderr_text) g_string_append (combined, stderr_text);
  
  *output = g_string_free (combined, FALSE);
  g_free (stdout_text);
  g_free (stderr_text);
  
  return success;
}

static void
test_cli_version (void)
{
  char *output = NULL;
  int status = 0;
  
  g_assert_true (run_gtimer ("--version", &output, &status));
  g_assert_cmpint (status, ==, 0);
  g_assert_nonnull (strstr (output, "GTimer"));
  g_free (output);
}

static void
test_cli_help (void)
{
  char *output = NULL;
  int status = 0;
  
  g_assert_true (run_gtimer ("--help", &output, &status));
  g_assert_cmpint (status, ==, 0);
  g_assert_nonnull (strstr (output, "Usage:"));
  g_free (output);
}

static void
test_cli_database_override (void)
{
  char *output = NULL;
  int status = 0;
  char *db_path = g_build_filename (g_get_tmp_dir (), "gtimer-test.db", NULL);
  
  /* Remove if exists */
  g_remove (db_path);
  
  char *args = g_strdup_printf ("--database %s --add-project 'Test Project'", db_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args);
  g_assert_cmpint (status, ==, 0);
  g_free (output);
  
  /* Verify file exists */
  g_assert_true (g_file_test (db_path, G_FILE_TEST_EXISTS));
  
  /* Verify project was added */
  args = g_strdup_printf ("--database %s --list-projects", db_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args);
  g_assert_cmpint (status, ==, 0);
  g_assert_nonnull (strstr (output, "Test Project"));
  g_free (output);
  
  g_remove (db_path);
  g_free (db_path);
}

static void
test_cli_add_list_task (void)
{
  char *output = NULL;
  int status = 0;
  char *db_path = g_build_filename (g_get_tmp_dir (), "gtimer-test-tasks.db", NULL);
  g_remove (db_path);
  
  /* Add task */
  char *args = g_strdup_printf ("--database %s --add-task 'New Task'", db_path);
  run_gtimer (args, &output, &status);
  g_free (args);
  g_free (output);
  
  /* List tasks */
  args = g_strdup_printf ("--database %s --list-tasks", db_path);
  run_gtimer (args, &output, &status);
  g_free (args);
  g_assert_nonnull (strstr (output, "New Task"));
  g_free (output);
  
  g_remove (db_path);
  g_free (db_path);
}

static void
test_cli_seed_and_list (void)
{
  char *output = NULL;
  int status = 0;
  char *db_path = g_build_filename (g_get_tmp_dir (), "gtimer-seed-test.db", NULL);
  g_remove (db_path);

  /* Seed the database using DB manager */
  GError *error = NULL;
  GTimerDBManager *db = gtimer_db_manager_new (db_path, &error);
  g_assert_no_error (error);
  
  gtimer_db_manager_create_project (db, "Seeded Project", &error);
  g_assert_no_error (error);
  
  gtimer_db_manager_create_task (db, "Seeded Task", 1, &error);
  g_assert_no_error (error);
  
  g_object_unref (db);

  /* Verify with CLI */
  char *args = g_strdup_printf ("--database %s --list-tasks", db_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args);
  g_assert_cmpint (status, ==, 0);
  g_assert_nonnull (strstr (output, "Seeded Task"));
  g_assert_nonnull (strstr (output, "Seeded Project"));
  g_free (output);

  /* Verify project listing */
  args = g_strdup_printf ("--database %s --list-projects", db_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args);
  g_assert_cmpint (status, ==, 0);
  g_assert_nonnull (strstr (output, "Seeded Project"));
  g_free (output);

  g_remove (db_path);
  g_free (db_path);
}

static void
test_cli_task_management (void)
{
  char *output = NULL;
  int status = 0;
  char *db_path = g_build_filename (g_get_tmp_dir (), "gtimer-mgmt-test.db", NULL);
  g_remove (db_path);

  /* Create task */
  char *args = g_strdup_printf ("--database %s --add-task 'To Be Deleted'", db_path);
  run_gtimer (args, &output, &status);
  g_free (args); g_free (output);

  /* Get task ID (it should be 1) */
  args = g_strdup_printf ("--database %s --list-tasks", db_path);
  run_gtimer (args, &output, &status);
  g_free (args);
  g_assert_nonnull (strstr (output, "1"));
  g_free (output);

  /* Delete task */
  args = g_strdup_printf ("--database %s --delete-task 1", db_path);
  run_gtimer (args, &output, &status);
  g_free (args); g_free (output);

  /* Verify deleted */
  args = g_strdup_printf ("--database %s --list-tasks", db_path);
  run_gtimer (args, &output, &status);
  g_free (args);
  g_assert_null (strstr (output, "To Be Deleted"));
  g_free (output);

  g_remove (db_path);
  g_free (db_path);
}

static void
test_cli_json_output (void)
{
  char *output = NULL;
  int status = 0;
  char *db_path = g_build_filename (g_get_tmp_dir (), "gtimer-json-test.db", NULL);
  g_remove (db_path);

  /* Add task */
  char *args = g_strdup_printf ("--database %s --add-task 'JSON Task'", db_path);
  run_gtimer (args, &output, &status);
  g_free (args); g_free (output);

  /* List tasks in JSON */
  args = g_strdup_printf ("--database %s --list-tasks --json", db_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args);
  g_assert_cmpint (status, ==, 0);
  g_assert_nonnull (strstr (output, "["));
  g_assert_nonnull (strstr (output, "\"name\": \"JSON Task\""));
  g_assert_nonnull (strstr (output, "]"));
  g_free (output);

  g_remove (db_path);
  g_free (db_path);
}

static void
test_cli_report (void)
{
  char *output = NULL;
  int status = 0;
  char *db_path = g_build_filename (g_get_tmp_dir (), "gtimer-report-test.db", NULL);
  g_remove (db_path);

  /* Add some data */
  char *args = g_strdup_printf ("--database %s --add-task 'Report Task'", db_path);
  run_gtimer (args, &output, &status);
  g_free (args); g_free (output);

  /* Seed time so it shows in report */
  GError *error = NULL;
  GTimerDBManager *db = gtimer_db_manager_new (db_path, &error);
  gtimer_db_manager_add_task_time (db, 1, 3600);
  g_object_unref (db);

  /* Generate daily report */
  args = g_strdup_printf ("--database %s --report daily", db_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args);
  g_assert_cmpint (status, ==, 0);
  g_assert_nonnull (strstr (output, "Report Task"));
  g_free (output);

  g_remove (db_path);
  g_free (db_path);
}

static void
test_cli_csv_export (void)
{
  char *output = NULL;
  int status = 0;
  char *db_path = g_build_filename (g_get_tmp_dir (), "gtimer-csv-test.db", NULL);
  char *csv_path = g_build_filename (g_get_tmp_dir (), "gtimer-export.csv", NULL);
  g_remove (db_path);
  g_remove (csv_path);

  /* Add task */
  char *args = g_strdup_printf ("--database %s --add-task 'CSV Task'", db_path);
  run_gtimer (args, &output, &status);
  g_free (args); g_free (output);

  /* Export to CSV */
  args = g_strdup_printf ("--database %s --export-csv %s", db_path, csv_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args);
  g_assert_cmpint (status, ==, 0);
  g_free (output);

  /* Verify CSV file */
  g_assert_true (g_file_test (csv_path, G_FILE_TEST_EXISTS));
  char *csv_content = NULL;
  g_file_get_contents (csv_path, &csv_content, NULL, NULL);
  g_assert_nonnull (strstr (csv_content, "CSV Task"));
  g_free (csv_content);

  g_remove (db_path);
  g_remove (csv_path);
  g_free (db_path);
  g_free (csv_path);
}

static void
test_cli_annotations (void)
{
  char *output = NULL;
  int status = 0;
  char *db_path = g_build_filename (g_get_tmp_dir (), "gtimer-ann-test.db", NULL);
  g_remove (db_path);

  /* Add task */
  char *args = g_strdup_printf ("--database %s --add-task 'Ann Task'", db_path);
  run_gtimer (args, &output, &status);
  g_free (args); g_free (output);

  /* Add annotation */
  args = g_strdup_printf ("--database %s --annotate 'Test Note' --annotate-task 1", db_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args); g_free (output);

  /* List annotations */
  args = g_strdup_printf ("--database %s --list-annotations 1", db_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args);
  g_assert_nonnull (strstr (output, "Test Note"));
  g_free (output);

  g_remove (db_path);
  g_free (db_path);
}

static void
test_cli_backup_restore (void)
{
  char *output = NULL;
  int status = 0;
  char *db_path = g_build_filename (g_get_tmp_dir (), "gtimer-orig.db", NULL);
  char *bak_path = g_build_filename (g_get_tmp_dir (), "gtimer-bak.db", NULL);
  g_remove (db_path);
  g_remove (bak_path);

  /* Create and add task to original */
  char *args = g_strdup_printf ("--database %s --add-task 'Orig Task'", db_path);
  run_gtimer (args, &output, &status);
  g_free (args); g_free (output);

  /* Backup */
  args = g_strdup_printf ("--database %s --backup %s", db_path, bak_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args); g_free (output);
  g_assert_true (g_file_test (bak_path, G_FILE_TEST_EXISTS));

  /* Delete original and restore */
  g_remove (db_path);
  args = g_strdup_printf ("--database %s --restore %s", db_path, bak_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args); g_free (output);
  
  /* Verify restored task */
  args = g_strdup_printf ("--database %s --list-tasks", db_path);
  run_gtimer (args, &output, &status);
  g_free (args);
  g_assert_nonnull (strstr (output, "Orig Task"));
  g_free (output);

  g_remove (db_path);
  g_remove (bak_path);
  g_free (db_path);
  g_free (bak_path);
}

static void
test_cli_datadir_override (void)
{
  char *output = NULL;
  int status = 0;
  char *tmp_dir = g_dir_make_tmp ("gtimer-datadir-XXXXXX", NULL);
  
  /* Running with --datadir <tmp_dir> should create <tmp_dir>/gtimer/gtimer.db */
  char *args = g_strdup_printf ("--datadir %s --add-task 'Datadir Task'", tmp_dir);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args);
  g_assert_cmpint (status, ==, 0);
  g_free (output);
  
  char *expected_db = g_build_filename (tmp_dir, "gtimer", "gtimer.db", NULL);
  g_assert_true (g_file_test (expected_db, G_FILE_TEST_EXISTS));
  
  /* Cleanup */
  g_remove (expected_db);
  char *gtimer_dir = g_build_filename (tmp_dir, "gtimer", NULL);
  g_rmdir (gtimer_dir);
  g_rmdir (tmp_dir);
  
  g_free (expected_db);
  g_free (gtimer_dir);
  g_free (tmp_dir);
}

static void
test_cli_negative_cases (void)
{
  char *output = NULL;
  int status = 0;
  char *db_path = g_build_filename (g_get_tmp_dir (), "gtimer-neg-test.db", NULL);
  g_remove (db_path);

  /* Delete non-existent task */
  char *args = g_strdup_printf ("--database %s --delete-task 999", db_path);
  run_gtimer (args, &output, &status);
  g_free (args);
  /* Current implementation reports success even if ID not found (SQL DELETE is idempotent-ish) */
  g_assert_nonnull (strstr (output, "Deleted task ID 999"));
  g_free (output);

  /* Restore non-existent file */
  args = g_strdup_printf ("--database %s --restore /non/existent/file.db", db_path);
  run_gtimer (args, &output, &status);
  g_free (args);
  g_assert_nonnull (strstr (output, "Restore failed"));
  g_free (output);

  g_remove (db_path);
  g_free (db_path);
}

static void
test_cli_export_json (void)
{
  char *output = NULL;
  int status = 0;
  char *db_path = g_build_filename (g_get_tmp_dir (), "gtimer-json-export.db", NULL);
  char *json_path = g_build_filename (g_get_tmp_dir (), "gtimer-export.json", NULL);
  g_remove (db_path);
  g_remove (json_path);

  /* Add project and task */
  char *args = g_strdup_printf ("--database %s --add-project 'JSON Project'", db_path);
  run_gtimer (args, &output, &status);
  g_free (args); g_free (output);

  args = g_strdup_printf ("--database %s --add-task 'JSON Export Task' --project 1", db_path);
  run_gtimer (args, &output, &status);
  g_free (args); g_free (output);

  /* Add annotation */
  args = g_strdup_printf ("--database %s --annotate 'A test note' --annotate-task 1", db_path);
  run_gtimer (args, &output, &status);
  g_free (args); g_free (output);

  /* Export to JSON */
  args = g_strdup_printf ("--database %s --export-json %s", db_path, json_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args);
  g_assert_cmpint (status, ==, 0);
  g_free (output);

  /* Verify JSON file */
  g_assert_true (g_file_test (json_path, G_FILE_TEST_EXISTS));
  char *json_content = NULL;
  g_file_get_contents (json_path, &json_content, NULL, NULL);
  g_assert_nonnull (json_content);
  g_assert_nonnull (strstr (json_content, "\"projects\""));
  g_assert_nonnull (strstr (json_content, "\"tasks\""));
  g_assert_nonnull (strstr (json_content, "JSON Project"));
  g_assert_nonnull (strstr (json_content, "JSON Export Task"));
  g_assert_nonnull (strstr (json_content, "\"annotations\""));
  g_assert_nonnull (strstr (json_content, "A test note"));
  g_assert_nonnull (strstr (json_content, "\"total_time\""));
  g_assert_nonnull (strstr (json_content, "\"is_timing\""));
  g_free (json_content);

  g_remove (db_path);
  g_remove (json_path);
  g_free (db_path);
  g_free (json_path);
}

static void
test_cli_summary_json (void)
{
  char *output = NULL;
  int status = 0;
  char *db_path = g_build_filename (g_get_tmp_dir (), "gtimer-summary-json.db", NULL);
  g_remove (db_path);

  /* Add task */
  char *args = g_strdup_printf ("--database %s --add-task 'Summary Task'", db_path);
  run_gtimer (args, &output, &status);
  g_free (args); g_free (output);

  /* Summary in JSON */
  args = g_strdup_printf ("--database %s --summary --json", db_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args);
  g_assert_cmpint (status, ==, 0);
  g_assert_nonnull (strstr (output, "\"running\""));
  g_assert_nonnull (strstr (output, "\"total_tasks\""));
  g_assert_nonnull (strstr (output, "\"today_seconds\""));
  g_free (output);

  /* Total time in JSON */
  args = g_strdup_printf ("--database %s --total-time --json", db_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args);
  g_assert_cmpint (status, ==, 0);
  g_assert_nonnull (strstr (output, "\"total_seconds\""));
  g_free (output);

  g_remove (db_path);
  g_free (db_path);
}

static void
test_cli_tags (void)
{
  char *output = NULL;
  int status = 0;
  char *db_path = g_build_filename (g_get_tmp_dir (), "gtimer-tags-test.db", NULL);
  g_remove (db_path);

  /* Create task */
  char *args = g_strdup_printf ("--database %s --add-task 'Tag Test'", db_path);
  run_gtimer (args, &output, &status);
  g_free (args); g_free (output);

  /* Add tags */
  args = g_strdup_printf ("--database %s --add-tag meeting --add-tag-task 1", db_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args);
  g_assert_cmpint (status, ==, 0);
  g_assert_nonnull (strstr (output, "Added tag 'meeting'"));
  g_free (output);

  args = g_strdup_printf ("--database %s --add-tag billable --add-tag-task 1", db_path);
  run_gtimer (args, &output, &status);
  g_free (args); g_free (output);

  /* List tags */
  args = g_strdup_printf ("--database %s --list-tags", db_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args);
  g_assert_nonnull (strstr (output, "billable"));
  g_assert_nonnull (strstr (output, "meeting"));
  g_free (output);

  /* List tasks shows tags */
  args = g_strdup_printf ("--database %s --list-tasks", db_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args);
  g_assert_nonnull (strstr (output, "[billable, meeting]"));
  g_free (output);

  /* List tasks with JSON shows tags */
  args = g_strdup_printf ("--database %s --list-tasks --json", db_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args);
  g_assert_nonnull (strstr (output, "\"tags\""));
  g_assert_nonnull (strstr (output, "billable"));
  g_free (output);

  /* Filter by tag */
  args = g_strdup_printf ("--database %s --add-task 'No Tags'", db_path);
  run_gtimer (args, &output, &status);
  g_free (args); g_free (output);

  args = g_strdup_printf ("--database %s --list-tasks --tag meeting", db_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args);
  g_assert_nonnull (strstr (output, "Tag Test"));
  g_assert_null (strstr (output, "No Tags"));
  g_free (output);

  /* Remove tag */
  args = g_strdup_printf ("--database %s --remove-tag billable --remove-tag-task 1", db_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args);
  g_assert_nonnull (strstr (output, "Removed tag"));
  g_free (output);

  /* Verify removal */
  args = g_strdup_printf ("--database %s --task-details 1", db_path);
  g_assert_true (run_gtimer (args, &output, &status));
  g_free (args);
  g_assert_null (strstr (output, "billable"));
  g_assert_nonnull (strstr (output, "meeting"));
  g_free (output);

  g_remove (db_path);
  g_free (db_path);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/cli/version", test_cli_version);
  g_test_add_func ("/cli/help", test_cli_help);
  g_test_add_func ("/cli/database-override", test_cli_database_override);
  g_test_add_func ("/cli/datadir-override", test_cli_datadir_override);
  g_test_add_func ("/cli/add-list-task", test_cli_add_list_task);
  g_test_add_func ("/cli/seed-and-list", test_cli_seed_and_list);
  g_test_add_func ("/cli/task-management", test_cli_task_management);
  g_test_add_func ("/cli/json-output", test_cli_json_output);
  g_test_add_func ("/cli/export-json", test_cli_export_json);
  g_test_add_func ("/cli/summary-json", test_cli_summary_json);
  g_test_add_func ("/cli/report", test_cli_report);
  g_test_add_func ("/cli/csv-export", test_cli_csv_export);
  g_test_add_func ("/cli/annotations", test_cli_annotations);
  g_test_add_func ("/cli/backup-restore", test_cli_backup_restore);
  g_test_add_func ("/cli/negative", test_cli_negative_cases);
  g_test_add_func ("/cli/tags", test_cli_tags);

  return g_test_run ();
}
