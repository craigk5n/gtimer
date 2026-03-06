#include <glib.h>
#include <sqlite3.h>
#include "../core/db-manager.h"
#include "../core/report-generator.h"

static GTimerDBManager *db = NULL;

static void
seed_data (void)
{
  sqlite3 *sql_db = gtimer_db_manager_get_db (db);

  sqlite3_exec (sql_db,
      "INSERT INTO projects (id, name) VALUES (1, 'Work');",
      NULL, NULL, NULL);
  sqlite3_exec (sql_db,
      "INSERT INTO tasks (id, name, project_id) VALUES (1, 'Coding', 1);",
      NULL, NULL, NULL);
  sqlite3_exec (sql_db,
      "INSERT INTO tasks (id, name) VALUES (2, 'Meetings');",
      NULL, NULL, NULL);
  sqlite3_exec (sql_db,
      "INSERT INTO daily_time (task_id, date, seconds) VALUES (1, '2026-03-01', 3600);",
      NULL, NULL, NULL);
  sqlite3_exec (sql_db,
      "INSERT INTO daily_time (task_id, date, seconds) VALUES (2, '2026-03-01', 1800);",
      NULL, NULL, NULL);
  sqlite3_exec (sql_db,
      "INSERT INTO daily_time (task_id, date, seconds) VALUES (1, '2026-03-02', 7200);",
      NULL, NULL, NULL);
  sqlite3_exec (sql_db,
      "INSERT INTO daily_time (task_id, date, seconds) VALUES (2, '2026-03-05', 900);",
      NULL, NULL, NULL);
}

static void
test_report_text_daily (void)
{
  GDateTime *start = g_date_time_new_local (2026, 3, 1, 0, 0, 0);
  GDateTime *end = g_date_time_new_local (2026, 3, 1, 0, 0, 0);

  char *report = gtimer_report_generate (db, GTIMER_REPORT_DAILY,
      GTIMER_REPORT_TEXT, start, end, NULL, 0);

  g_assert_nonnull (report);
  /* Header */
  g_assert_true (g_str_has_prefix (report, "GTimer Report\n"));
  g_assert_nonnull (strstr (report, "2026-03-01"));
  /* Coding: 1h, Meetings: 30m */
  g_assert_nonnull (strstr (report, "01:00:00"));
  g_assert_nonnull (strstr (report, "00:30:00"));
  g_assert_nonnull (strstr (report, "Coding"));
  g_assert_nonnull (strstr (report, "Meetings"));
  /* Project name shown for Coding */
  g_assert_nonnull (strstr (report, "(Work)"));
  /* Total line: 01:30:00 */
  g_assert_nonnull (strstr (report, "01:30:00"));
  g_assert_nonnull (strstr (report, "Total"));

  g_free (report);
  g_date_time_unref (start);
  g_date_time_unref (end);
}

static void
test_report_text_multiday (void)
{
  GDateTime *start = g_date_time_new_local (2026, 3, 1, 0, 0, 0);
  GDateTime *end = g_date_time_new_local (2026, 3, 5, 0, 0, 0);

  char *report = gtimer_report_generate (db, GTIMER_REPORT_WEEKLY,
      GTIMER_REPORT_TEXT, start, end, NULL, 0);

  g_assert_nonnull (report);
  /* Coding: 3600 + 7200 = 10800 = 03:00:00 */
  g_assert_nonnull (strstr (report, "03:00:00"));
  g_assert_nonnull (strstr (report, "Coding"));
  /* Meetings: 1800 + 900 = 2700 = 00:45:00 */
  g_assert_nonnull (strstr (report, "00:45:00"));
  /* Total: 13500 = 03:45:00 */
  g_assert_nonnull (strstr (report, "03:45:00"));

  g_free (report);
  g_date_time_unref (start);
  g_date_time_unref (end);
}

static void
test_report_html (void)
{
  GDateTime *start = g_date_time_new_local (2026, 3, 1, 0, 0, 0);
  GDateTime *end = g_date_time_new_local (2026, 3, 1, 0, 0, 0);

  char *report = gtimer_report_generate (db, GTIMER_REPORT_DAILY,
      GTIMER_REPORT_HTML, start, end, NULL, 0);

  g_assert_nonnull (report);
  g_assert_true (g_str_has_prefix (report, "<!DOCTYPE html>"));
  g_assert_nonnull (strstr (report, "</html>"));
  g_assert_nonnull (strstr (report, "<table>"));
  g_assert_nonnull (strstr (report, "01:00:00"));
  g_assert_nonnull (strstr (report, "Coding"));
  g_assert_nonnull (strstr (report, "class='project'>Work</td>"));
  /* Task without project gets a dash */
  g_assert_nonnull (strstr (report, "class='project'>-</td>"));
  g_assert_nonnull (strstr (report, "Total"));

  g_free (report);
  g_date_time_unref (start);
  g_date_time_unref (end);
}

static void
test_report_empty_period (void)
{
  GDateTime *start = g_date_time_new_local (2020, 1, 1, 0, 0, 0);
  GDateTime *end = g_date_time_new_local (2020, 1, 31, 0, 0, 0);

  char *text = gtimer_report_generate (db, GTIMER_REPORT_MONTHLY,
      GTIMER_REPORT_TEXT, start, end, NULL, 0);
  g_assert_nonnull (text);
  g_assert_nonnull (strstr (text, "No data found"));
  g_assert_null (strstr (text, "Total"));

  char *html = gtimer_report_generate (db, GTIMER_REPORT_MONTHLY,
      GTIMER_REPORT_HTML, start, end, NULL, 0);
  g_assert_nonnull (html);
  g_assert_nonnull (strstr (html, "No data found"));

  g_free (text);
  g_free (html);
  g_date_time_unref (start);
  g_date_time_unref (end);
}

static void
test_report_rounding (void)
{
  GDateTime *start = g_date_time_new_local (2026, 3, 1, 0, 0, 0);
  GDateTime *end = g_date_time_new_local (2026, 3, 1, 0, 0, 0);

  /* Round to 15 minutes (900s).
     Coding: 3600s -> 3600 (exact multiple, stays)
     Meetings: 1800s -> 1800 (exact multiple, stays) */
  char *report = gtimer_report_generate (db, GTIMER_REPORT_DAILY,
      GTIMER_REPORT_TEXT, start, end, NULL, 15);
  g_assert_nonnull (report);
  g_assert_nonnull (strstr (report, "01:00:00"));
  g_assert_nonnull (strstr (report, "00:30:00"));
  g_free (report);

  /* Now test with data that actually rounds.
     Add a task with 500s (8m20s). Round to 15min:
     (500 + 450) / 900 * 900 = 1 * 900 = 900 = 00:15:00 */
  sqlite3 *sql_db = gtimer_db_manager_get_db (db);
  sqlite3_exec (sql_db,
      "INSERT INTO tasks (id, name) VALUES (3, 'Quick');",
      NULL, NULL, NULL);
  sqlite3_exec (sql_db,
      "INSERT INTO daily_time (task_id, date, seconds) VALUES (3, '2026-03-01', 500);",
      NULL, NULL, NULL);

  report = gtimer_report_generate (db, GTIMER_REPORT_DAILY,
      GTIMER_REPORT_TEXT, start, end, NULL, 15);
  g_assert_nonnull (report);
  g_assert_nonnull (strstr (report, "Quick"));
  g_assert_nonnull (strstr (report, "00:15:00"));
  g_free (report);

  g_date_time_unref (start);
  g_date_time_unref (end);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  GError *error = NULL;
  db = gtimer_db_manager_new (":memory:", &error);
  g_assert_no_error (error);
  g_assert_nonnull (db);
  seed_data ();

  g_test_add_func ("/report/text_daily", test_report_text_daily);
  g_test_add_func ("/report/text_multiday", test_report_text_multiday);
  g_test_add_func ("/report/html", test_report_html);
  g_test_add_func ("/report/empty_period", test_report_empty_period);
  g_test_add_func ("/report/rounding", test_report_rounding);

  int result = g_test_run ();
  g_object_unref (db);
  return result;
}
