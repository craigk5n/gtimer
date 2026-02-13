#include "report-generator.h"
#include "timer-utils.h"
#include <stdio.h>

char *gtimer_report_generate ( GTimerDBManager * db_manager,
    GTimerReportType type,
    GTimerReportFormat format,
    GDateTime * start_date,
    GDateTime * end_date, GList * task_ids, int rounding_minutes )
{
  ( void ) type;
  ( void ) task_ids;

  sqlite3 *db = gtimer_db_manager_get_db ( db_manager );
  sqlite3_stmt *stmt;
  GString *output = g_string_new ( NULL );

  char *start_str = g_date_time_format ( start_date, "%Y-%m-%d" );
  char *end_str = g_date_time_format ( end_date, "%Y-%m-%d" );

  if ( format == GTIMER_REPORT_TEXT ) {
    g_string_append_printf ( output, "GTimer Report\n" );
    g_string_append_printf ( output, "Period: %s to %s\n\n", start_str,
        end_str );
  } else {
    g_string_append ( output, "<!DOCTYPE html><html><head><style>" );
    g_string_append ( output,
        "body { font-family: sans-serif; margin: 2em; }" );
    g_string_append ( output,
        "table { border-collapse: collapse; width: 100%; }" );
    g_string_append ( output,
        "th, td { border-bottom: 1px solid #ddd; padding: 8px; text-align: left; }" );
    g_string_append ( output, "th { background-color: #f2f2f2; }" );
    g_string_append ( output, ".duration { font-family: monospace; }" );
    g_string_append ( output, ".project { color: #666; font-size: 0.9em; }" );
    g_string_append ( output, "</style></head><body>" );
    g_string_append_printf ( output,
        "<h1>GTimer Report</h1><h3>Period: %s to %s</h3>", start_str,
        end_str );
    g_string_append ( output,
        "<table><tr><th>Duration</th><th>Task</th><th>Project</th></tr>" );
  }

  const char *sql =
      "SELECT t.name, COALESCE(p.name, '') as project_name, SUM(d.seconds) as total "
      "FROM tasks t "
      "JOIN daily_time d ON t.id = d.task_id "
      "LEFT JOIN projects p ON t.project_id = p.id "
      "WHERE d.date >= ? AND d.date <= ? "
      "GROUP BY t.id " "ORDER BY total DESC;";

  if ( sqlite3_prepare_v2 ( db, sql, -1, &stmt, NULL ) == SQLITE_OK ) {
    sqlite3_bind_text ( stmt, 1, start_str, -1, SQLITE_TRANSIENT );
    sqlite3_bind_text ( stmt, 2, end_str, -1, SQLITE_TRANSIENT );

    gint64 grand_total = 0;
    int count = 0;
    while ( sqlite3_step ( stmt ) == SQLITE_ROW ) {
      count++;
      const char *name = ( const char * ) sqlite3_column_text ( stmt, 0 );
      const char *project_name =
          ( const char * ) sqlite3_column_text ( stmt, 1 );
      gint64 seconds = sqlite3_column_int64 ( stmt, 2 );

      if ( rounding_minutes > 0 ) {
        gint64 round_secs = ( gint64 ) rounding_minutes * 60;
        seconds = ( ( seconds + round_secs / 2 ) / round_secs ) * round_secs;
      }

      char *duration = gtimer_utils_format_duration ( seconds );
      if ( format == GTIMER_REPORT_TEXT ) {
        if ( project_name && strlen ( project_name ) > 0 ) {
          g_string_append_printf ( output, "%-10s  %s (%s)\n", duration, name,
              project_name );
        } else {
          g_string_append_printf ( output, "%-10s  %s\n", duration, name );
        }
      } else {
        if ( project_name && strlen ( project_name ) > 0 ) {
          g_string_append_printf ( output,
              "<tr><td class='duration'>%s</td><td>%s</td><td class='project'>%s</td></tr>",
              duration, name, project_name );
        } else {
          g_string_append_printf ( output,
              "<tr><td class='duration'>%s</td><td>%s</td><td class='project'>-</td></tr>",
              duration, name );
        }
      }
      grand_total += seconds;
      g_free ( duration );
    }

    if ( count > 0 ) {
      char *total_str = gtimer_utils_format_duration ( grand_total );
      if ( format == GTIMER_REPORT_TEXT ) {
        g_string_append_printf ( output, "\n%-10s  Total\n", total_str );
      } else {
        g_string_append_printf ( output,
            "<tr><th class='duration'>%s</th><th colspan='2'>Total</th></tr>",
            total_str );
      }
      g_free ( total_str );
    } else {
      if ( format == GTIMER_REPORT_TEXT ) {
        g_string_append ( output, "No data found for this period.\n" );
      } else {
        g_string_append ( output,
            "<tr><td colspan='3'>No data found for this period.</td></tr>" );
      }
    }
    sqlite3_finalize ( stmt );
  }

  if ( format == GTIMER_REPORT_HTML ) {
    g_string_append ( output, "</table></body></html>" );
  }

  g_free ( start_str );
  g_free ( end_str );
  return g_string_free ( output, FALSE );
}
