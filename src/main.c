#include "gtimer-window.h"
#include "report-window.h"
#include "../core/db-manager.h"
#include "../core/task-list-model.h"
#include "../core/timer-service.h"
#include "../core/idle-monitor.h"
#include "../core/timer-utils.h"
#include "../core/report-generator.h"
#include "../core/project-object.h"
#include <glib/gi18n.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  GTimerDBManager *db_manager;
  GTimerTaskListModel *task_list_model;
  GTimerTimerService *timer_service;
  GTimerIdleMonitor *idle_monitor;
} GTimerApp;

typedef struct {
  int task_id;
  gboolean stop_timer;
  gboolean stop_all_timers;
  gboolean show_status;
  gboolean list_tasks;
  char *add_task;
  int add_task_project;
  char *report_type;
  char *report_file;
  char *export_csv;
  char *import_csv;
  gboolean show_version;
  gboolean show_gui;
  char *datadir;
  char *database;
  gboolean verbose;
  gboolean quiet;
  /* Date-based options */
  gboolean show_today;
  gboolean show_week;
  gboolean show_month;
  char *since_date;
  char *until_date;
  char *specific_date;
  /* Task management */
  int delete_task_id;
  int hide_task_id;
  int unhide_task_id;
  char *rename_task;
  int rename_task_id;
  int move_task_id;
  int move_to_project;
  int task_details_id;
  int reset_task_id;
  int duplicate_task_id;
  /* Project management */
  gboolean list_projects;
  char *add_project;
  int delete_project_id;
  int rename_project_id;
  char *rename_project_name;
  /* Annotations */
  char *annotate_text;
  int annotate_task_id;
  int list_annotations_id;
  char *note_text;
  /* Data/export */
  gboolean output_json;
  char *export_json;
  char *backup_file;
  char *restore_file;
  char *import_gtimer2;
  char *export_sqlite;
  /* Tags */
  char *add_tag;
  int add_tag_task_id;
  char *remove_tag;
  int remove_tag_task_id;
  gboolean list_tags;
  char *filter_tag;
  /* Utility */
  gboolean show_total_time;
  gboolean show_active_time;
  gboolean show_summary;
  int merge_source_id;
  int merge_target_id;
  gboolean vacuum_db;
  int edit_task_id;
} GTimerCLIOptions;

/* Escape a string for safe JSON output. Caller must g_free() the result. */
static char *
json_escape (const char *str)
{
  if (!str) return g_strdup ("");
  GString *out = g_string_sized_new (strlen (str) + 16);
  for (const char *p = str; *p; p++) {
    switch (*p) {
      case '"':  g_string_append (out, "\\\""); break;
      case '\\': g_string_append (out, "\\\\"); break;
      case '\n': g_string_append (out, "\\n"); break;
      case '\r': g_string_append (out, "\\r"); break;
      case '\t': g_string_append (out, "\\t"); break;
      default:
        if ((unsigned char)*p < 0x20)
          g_string_append_printf (out, "\\u%04x", (unsigned char)*p);
        else
          g_string_append_c (out, *p);
        break;
    }
  }
  return g_string_free (out, FALSE);
}

static void
print_status(GTimerDBManager *db, gboolean json)
{
  GList *tasks = gtimer_db_manager_get_all_tasks(db);
  GList *l;
  int running_count = 0;
  GString *json_output = NULL;

  if (json) {
    json_output = g_string_new("[\n");
  }

  if (tasks) {
    for (l = tasks; l != NULL; l = g_list_next(l)) {
      GTimerTask *task = GTIMER_TASK(l->data);
      if (!task)
        continue;
      if (gtimer_task_is_timing(task)) {
        running_count++;
        if (json) {
          g_autofree char *ename = json_escape (gtimer_task_get_name (task));
          g_autofree char *eproj = json_escape (gtimer_task_get_project_name (task));
          g_string_append_printf(json_output, "  {\"id\": %d, \"name\": \"%s\", \"project\": \"%s\"}",
                                gtimer_task_get_id(task), ename, eproj);
          if (l->next) g_string_append(json_output, ",");
          g_string_append(json_output, "\n");
        } else {
          g_print("[%d] %s", gtimer_task_get_id(task), gtimer_task_get_name(task));
          if (gtimer_task_get_project_name(task)) {
            g_print(" (%s)", gtimer_task_get_project_name(task));
          }
          g_print("\n");
        }
      }
      g_object_unref(task);
    }
    g_list_free(tasks);
  }

  if (json) {
    g_string_append(json_output, "]\n");
    g_print("%s", json_output->str);
    g_string_free(json_output, TRUE);
  } else if (running_count == 0) {
    g_print("No tasks are currently timing.\n");
  }
}

static void
list_all_tasks(GTimerDBManager *db, gboolean json, const char *filter_tag)
{
  GList *tasks = gtimer_db_manager_get_all_tasks(db);
  GList *l;

  /* If filtering by tag, get the set of matching task IDs */
  GList *tag_task_ids = NULL;
  if (filter_tag)
    tag_task_ids = gtimer_db_manager_get_tasks_by_tag(db, filter_tag);

  if (json) {
    g_print("[\n");
  } else {
    g_print("ID   Project                     Task Name\n");
    g_print("---- --------------------------- --------------------------------\n");
  }

  gboolean first_json = TRUE;
  int displayed = 0;
  if (tasks) {
    for (l = tasks; l != NULL; l = g_list_next(l)) {
      GTimerTask *task = GTIMER_TASK(l->data);
      if (!task)
        continue;

      /* Apply tag filter */
      if (filter_tag) {
        gboolean found = FALSE;
        for (GList *t = tag_task_ids; t != NULL; t = t->next) {
          if (GPOINTER_TO_INT(t->data) == gtimer_task_get_id(task)) { found = TRUE; break; }
        }
        if (!found) continue;
      }

      displayed++;
      const char *project = gtimer_task_get_project_name(task) ? gtimer_task_get_project_name(task) : "";
      const char *tags = gtimer_task_get_tags(task);
      if (json) {
        g_autofree char *ename = json_escape (gtimer_task_get_name (task));
        g_autofree char *eproj = json_escape (project);
        g_autofree char *etags = json_escape (tags);
        if (!first_json) g_print(",\n");
        g_print("  {\"id\": %d, \"name\": \"%s\", \"project\": \"%s\", \"tags\": \"%s\", \"total_time\": %ld, \"today_time\": %ld}",
                gtimer_task_get_id(task), ename, eproj, etags,
                gtimer_task_get_total_time(task),
                gtimer_task_get_today_time(task));
        first_json = FALSE;
      } else {
        if (tags && tags[0]) {
          g_print("%-4d %-27s %s [%s]\n",
                  gtimer_task_get_id(task),
                  project[0] ? project : "-",
                  gtimer_task_get_name(task), tags);
        } else {
          g_print("%-4d %-27s %s\n",
                  gtimer_task_get_id(task),
                  project[0] ? project : "-",
                  gtimer_task_get_name(task));
        }
      }
      g_object_unref(task);
    }
    g_list_free(tasks);
  }

  g_list_free (tag_task_ids);

  if (json) {
    g_print("\n]\n");
  } else {
    g_print("\n%d tasks total.\n", displayed);
  }
}

/* Helper: find a task by ID from the full task list. Caller must unref. */
static GTimerTask *
find_task_by_id (GTimerDBManager *db, int task_id)
{
  GList *tasks = gtimer_db_manager_get_all_tasks (db);
  GTimerTask *found = NULL;

  for (GList *l = tasks; l != NULL; l = l->next) {
    GTimerTask *task = GTIMER_TASK (l->data);
    if (gtimer_task_get_id (task) == task_id) {
      found = g_object_ref (task);
      break;
    }
  }
  g_list_free_full (tasks, g_object_unref);
  return found;
}

/* Copy a file with error reporting. Returns TRUE on success. */
static gboolean
copy_file (const char *src_path, const char *dst_path,
           const char *success_msg, const char *fail_msg)
{
  GFile *src = g_file_new_for_path (src_path);
  GFile *dst = g_file_new_for_path (dst_path);
  GError *err = NULL;

  gboolean ok = g_file_copy (src, dst, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &err);
  if (ok) {
    g_print ("%s: %s\n", success_msg, dst_path);
  } else {
    g_printerr ("%s: %s\n", fail_msg, err->message);
    g_error_free (err);
  }
  g_object_unref (src);
  g_object_unref (dst);
  return ok;
}

static gboolean
handle_cli_timer_control (GTimerCLIOptions *opts, GTimerDBManager *db)
{
  if (opts->stop_all_timers) {
    GList *tasks = gtimer_db_manager_get_all_tasks (db);
    int count = 0;

    for (GList *l = tasks; l != NULL; l = l->next) {
      GTimerTask *task = GTIMER_TASK (l->data);
      if (gtimer_task_is_timing (task)) {
        gtimer_db_manager_stop_task_timing (db, gtimer_task_get_id (task));
        count++;
        g_print ("Stopped: %s\n", gtimer_task_get_name (task));
      }
    }
    g_list_free_full (tasks, g_object_unref);
    g_print ("Stopped %d task(s).\n", count);
    return TRUE;
  }

  if (opts->stop_timer) {
    GList *tasks = gtimer_db_manager_get_all_tasks (db);
    int count = 0;

    for (GList *l = tasks; l != NULL; l = l->next) {
      GTimerTask *task = GTIMER_TASK (l->data);
      if (gtimer_task_is_timing (task)) {
        char *name = g_strdup (gtimer_task_get_name (task));
        gtimer_db_manager_stop_task_timing (db, gtimer_task_get_id (task));
        count++;
        g_print ("Stopped: %s\n", name);
        g_free (name);
      }
    }
    g_list_free_full (tasks, g_object_unref);
    if (count == 0)
      g_print ("No task is currently timing.\n");
    return TRUE;
  }

  if (opts->task_id > 0) {
    GTimerTask *found = find_task_by_id (db, opts->task_id);
    if (!found) {
      g_printerr ("Error: Task with ID %d not found\n", opts->task_id);
      return TRUE;
    }
    gtimer_db_manager_start_task_timing (db, opts->task_id);
    g_print ("Started timing: %s (ID: %d)\n", gtimer_task_get_name (found), opts->task_id);
    g_object_unref (found);
    return TRUE;
  }

  return FALSE;
}

static gboolean
handle_cli_task_management (GTimerCLIOptions *opts, GTimerDBManager *db)
{
  if (opts->add_task) {
    GError *error = NULL;
    gtimer_db_manager_create_task (db, opts->add_task,
        opts->add_task_project > 0 ? opts->add_task_project : -1, &error);
    if (error) {
      g_printerr ("Error creating task: %s\n", error->message);
      g_error_free (error);
    } else {
      g_print ("Created task: %s\n", opts->add_task);
    }
    return TRUE;
  }

  if (opts->delete_task_id > 0) {
    GError *err = NULL;
    gtimer_db_manager_delete_task (db, opts->delete_task_id, &err);
    if (err) {
      g_printerr ("Error deleting task: %s\n", err->message);
      g_error_free (err);
    } else {
      g_print ("Deleted task ID %d\n", opts->delete_task_id);
    }
    return TRUE;
  }

  if (opts->hide_task_id > 0) {
    GError *err = NULL;
    gtimer_db_manager_hide_task (db, opts->hide_task_id, TRUE, &err);
    if (err) {
      g_printerr ("Error hiding task: %s\n", err->message);
      g_error_free (err);
    } else {
      g_print ("Hidden task ID %d\n", opts->hide_task_id);
    }
    return TRUE;
  }

  if (opts->unhide_task_id > 0) {
    GError *err = NULL;
    gtimer_db_manager_hide_task (db, opts->unhide_task_id, FALSE, &err);
    if (err) {
      g_printerr ("Error unhiding task: %s\n", err->message);
      g_error_free (err);
    } else {
      g_print ("Unhidden task ID %d\n", opts->unhide_task_id);
    }
    return TRUE;
  }

  if (opts->rename_task && opts->rename_task_id > 0) {
    GTimerTask *found = find_task_by_id (db, opts->rename_task_id);
    if (!found) {
      g_printerr ("Task ID %d not found\n", opts->rename_task_id);
      return TRUE;
    }
    int project_id = gtimer_task_get_project_id (found);
    g_object_unref (found);

    GError *err = NULL;
    gtimer_db_manager_update_task (db, opts->rename_task_id, opts->rename_task,
        project_id >= 0 ? project_id : -1, &err);
    if (err) {
      g_printerr ("Error renaming task: %s\n", err->message);
      g_error_free (err);
    } else {
      g_print ("Renamed task %d to: %s\n", opts->rename_task_id, opts->rename_task);
    }
    return TRUE;
  }

  if (opts->move_task_id > 0 && opts->move_to_project >= 0) {
    GTimerTask *found = find_task_by_id (db, opts->move_task_id);
    if (!found) {
      g_printerr ("Task ID %d not found\n", opts->move_task_id);
      return TRUE;
    }
    const char *name = gtimer_task_get_name (found);
    GError *err = NULL;
    gtimer_db_manager_update_task (db, opts->move_task_id, name,
        opts->move_to_project, &err);
    g_object_unref (found);
    if (err) {
      g_printerr ("Error moving task: %s\n", err->message);
      g_error_free (err);
    } else {
      g_print ("Moved task %d to project %d\n", opts->move_task_id, opts->move_to_project);
    }
    return TRUE;
  }

  if (opts->task_details_id > 0) {
    GTimerTask *found = find_task_by_id (db, opts->task_details_id);
    if (!found) {
      g_printerr ("Task ID %d not found\n", opts->task_details_id);
      return TRUE;
    }
    if (opts->output_json) {
      g_autofree char *ename = json_escape (gtimer_task_get_name (found));
      g_autofree char *eproj = json_escape (gtimer_task_get_project_name (found));
      g_autofree char *etags = json_escape (gtimer_task_get_tags (found));
      g_print ("{\n");
      g_print ("  \"id\": %d,\n", gtimer_task_get_id (found));
      g_print ("  \"name\": \"%s\",\n", ename);
      g_print ("  \"project\": \"%s\",\n", eproj);
      g_print ("  \"tags\": \"%s\",\n", etags);
      g_print ("  \"total_time\": %ld,\n", gtimer_task_get_total_time (found));
      g_print ("  \"today_time\": %ld,\n", gtimer_task_get_today_time (found));
      g_print ("  \"is_timing\": %s,\n", gtimer_task_is_timing (found) ? "true" : "false");
      g_print ("  \"is_hidden\": %s\n", gtimer_task_is_hidden (found) ? "true" : "false");
      g_print ("}\n");
    } else {
      const char *tags = gtimer_task_get_tags (found);
      g_print ("Task Details:\n");
      g_print ("  ID: %d\n", gtimer_task_get_id (found));
      g_print ("  Name: %s\n", gtimer_task_get_name (found));
      g_print ("  Project: %s\n", gtimer_task_get_project_name (found) ? gtimer_task_get_project_name (found) : "(none)");
      g_print ("  Tags: %s\n", (tags && tags[0]) ? tags : "(none)");
      g_print ("  Total Time: %ld seconds\n", gtimer_task_get_total_time (found));
      g_print ("  Today: %ld seconds\n", gtimer_task_get_today_time (found));
      g_print ("  Status: %s\n", gtimer_task_is_timing (found) ? "timing" : "stopped");
    }
    g_object_unref (found);
    return TRUE;
  }

  if (opts->reset_task_id > 0) {
    g_printerr ("Reset task not yet implemented (requires db function)\n");
    return TRUE;
  }

  if (opts->duplicate_task_id > 0) {
    g_printerr ("Duplicate task not yet implemented (requires db function)\n");
    return TRUE;
  }

  return FALSE;
}

static gboolean
handle_cli_project_management (GTimerCLIOptions *opts, GTimerDBManager *db)
{
  if (opts->list_projects) {
    GList *projects = gtimer_db_manager_get_projects (db);

    if (opts->output_json) {
      g_print ("[\n");
      for (GList *l = projects; l != NULL; l = l->next) {
        GTimerProject *project = GTIMER_PROJECT (l->data);
        g_autofree char *ename = json_escape (gtimer_project_get_name (project));
        g_print ("  {\"id\": %d, \"name\": \"%s\"}",
            gtimer_project_get_id (project), ename);
        if (l->next) g_print (",");
        g_print ("\n");
      }
      g_print ("]\n");
    } else {
      g_print ("ID   Project Name\n");
      g_print ("---- ---------------------------\n");
      for (GList *l = projects; l != NULL; l = l->next) {
        GTimerProject *project = GTIMER_PROJECT (l->data);
        g_print ("%-4d %s\n", gtimer_project_get_id (project), gtimer_project_get_name (project));
      }
      g_print ("\n%d projects total.\n", g_list_length (projects));
    }
    g_list_free_full (projects, g_object_unref);
    return TRUE;
  }

  if (opts->add_project) {
    GError *err = NULL;
    gtimer_db_manager_create_project (db, opts->add_project, &err);
    if (err) {
      g_printerr ("Error creating project: %s\n", err->message);
      g_error_free (err);
    } else {
      g_print ("Created project: %s\n", opts->add_project);
    }
    return TRUE;
  }

  if (opts->delete_project_id > 0) {
    g_printerr ("Delete project not yet implemented (requires db function)\n");
    return TRUE;
  }

  if (opts->rename_project_id > 0 && opts->rename_project_name) {
    GError *err = NULL;
    gtimer_db_manager_update_project (db, opts->rename_project_id, opts->rename_project_name, &err);
    if (err) {
      g_printerr ("Error renaming project: %s\n", err->message);
      g_error_free (err);
    } else {
      g_print ("Renamed project %d to: %s\n", opts->rename_project_id, opts->rename_project_name);
    }
    return TRUE;
  }

  return FALSE;
}

static gboolean
handle_cli_reports (GTimerCLIOptions *opts, GTimerDBManager *db)
{
  if (opts->report_type) {
    GDateTime *now = g_date_time_new_now_local ();
    GDateTime *start_date = g_date_time_ref (now);
    GDateTime *end_date = g_date_time_ref (now);

    if (g_str_equal (opts->report_type, "weekly") || g_str_equal (opts->report_type, "w")) {
      start_date = g_date_time_add_days (now, -7);
    } else if (g_str_equal (opts->report_type, "monthly") || g_str_equal (opts->report_type, "m")) {
      start_date = g_date_time_add_months (now, -1);
    } else if (g_str_equal (opts->report_type, "yearly") || g_str_equal (opts->report_type, "y")) {
      start_date = g_date_time_add_years (now, -1);
    } else if (!g_str_equal (opts->report_type, "daily") && !g_str_equal (opts->report_type, "d")) {
      g_printerr ("Error: Unknown report type '%s'. Use daily, weekly, or monthly\n", opts->report_type);
      g_date_time_unref (now);
      g_date_time_unref (start_date);
      g_date_time_unref (end_date);
      return TRUE;
    }

    char *report = gtimer_report_generate (db, GTIMER_REPORT_DAILY, GTIMER_REPORT_TEXT,
        start_date, end_date, NULL, 0);

    if (opts->report_file) {
      if (!g_file_set_contents (opts->report_file, report, -1, NULL))
        g_printerr ("Error: Failed to write report to %s\n", opts->report_file);
      else
        g_print ("Report saved to: %s\n", opts->report_file);
    } else {
      g_print ("%s\n", report);
    }

    g_free (report);
    g_date_time_unref (now);
    g_date_time_unref (start_date);
    g_date_time_unref (end_date);
    return TRUE;
  }

  if (opts->show_today || opts->show_week || opts->show_month ||
      opts->since_date || opts->until_date || opts->specific_date) {
    GDateTime *start = g_date_time_new_now_local ();
    GDateTime *end = g_date_time_ref (start);

    if (opts->specific_date) {
      g_print ("Report for %s:\n", opts->specific_date);
    } else if (opts->since_date || opts->until_date) {
      g_print ("Report from %s to %s:\n",
          opts->since_date ? opts->since_date : "beginning",
          opts->until_date ? opts->until_date : "now");
    } else if (opts->show_week) {
      start = g_date_time_add_days (start, -7);
      g_print ("Weekly Report:\n");
    } else if (opts->show_month) {
      start = g_date_time_add_months (start, -1);
      g_print ("Monthly Report:\n");
    } else {
      g_print ("Today's Summary:\n");
    }

    char *report = gtimer_report_generate (db, GTIMER_REPORT_DAILY, GTIMER_REPORT_TEXT,
        start, end, NULL, 0);
    g_print ("%s\n", report);
    g_free (report);
    g_date_time_unref (start);
    g_date_time_unref (end);
    return TRUE;
  }

  return FALSE;
}

static gboolean
handle_cli_annotations (GTimerCLIOptions *opts, GTimerDBManager *db)
{
  if (opts->annotate_text && opts->annotate_task_id > 0) {
    gtimer_db_manager_add_annotation (db, opts->annotate_task_id, opts->annotate_text);
    g_print ("Added annotation to task %d\n", opts->annotate_task_id);
    return TRUE;
  }

  if (opts->list_annotations_id > 0) {
    GList *annotations = gtimer_db_manager_get_annotations (db, opts->list_annotations_id);

    if (opts->output_json) {
      g_print ("[\n");
      for (GList *l = annotations; l != NULL; l = l->next) {
        GTimerAnnotation *ann = l->data;
        GDateTime *dt = g_date_time_new_from_unix_local (ann->created_at);
        char *date_str = g_date_time_format (dt, "%Y-%m-%d %H:%M");
        g_autofree char *etext = json_escape (ann->text);
        g_print ("  {\"date\": \"%s\", \"text\": \"%s\"}", date_str, etext);
        if (l->next) g_print (",");
        g_print ("\n");
        g_free (date_str);
        g_date_time_unref (dt);
      }
      g_print ("]\n");
    } else {
      g_print ("Annotations for task %d:\n", opts->list_annotations_id);
      for (GList *l = annotations; l != NULL; l = l->next) {
        GTimerAnnotation *ann = l->data;
        GDateTime *dt = g_date_time_new_from_unix_local (ann->created_at);
        char *date_str = g_date_time_format (dt, "%Y-%m-%d %H:%M");
        g_print ("  [%s] %s\n", date_str, ann->text);
        g_free (date_str);
        g_date_time_unref (dt);
      }
      if (!annotations)
        g_print ("  (no annotations)\n");
    }
    g_list_free_full (annotations, (GDestroyNotify)gtimer_annotation_free);
    return TRUE;
  }

  if (opts->note_text) {
    GList *tasks = gtimer_db_manager_get_all_tasks (db);
    int running_id = 0;

    for (GList *l = tasks; l != NULL; l = l->next) {
      GTimerTask *task = GTIMER_TASK (l->data);
      if (gtimer_task_is_timing (task))
        running_id = gtimer_task_get_id (task);
    }
    g_list_free_full (tasks, g_object_unref);

    if (running_id > 0) {
      gtimer_db_manager_add_annotation (db, running_id, opts->note_text);
      g_print ("Added note to task %d\n", running_id);
    } else {
      g_printerr ("No task is currently running\n");
    }
    return TRUE;
  }

  return FALSE;
}

static gboolean
handle_cli_tags (GTimerCLIOptions *opts, GTimerDBManager *db)
{
  if (opts->add_tag && opts->add_tag_task_id > 0) {
    gtimer_db_manager_add_tag_to_task (db, opts->add_tag_task_id, opts->add_tag);
    g_print ("Added tag '%s' to task %d\n", opts->add_tag, opts->add_tag_task_id);
    return TRUE;
  }

  if (opts->remove_tag && opts->remove_tag_task_id > 0) {
    gtimer_db_manager_remove_tag_from_task (db, opts->remove_tag_task_id, opts->remove_tag);
    g_print ("Removed tag '%s' from task %d\n", opts->remove_tag, opts->remove_tag_task_id);
    return TRUE;
  }

  if (opts->list_tags) {
    GList *tags = gtimer_db_manager_get_all_tags (db);
    if (opts->output_json) {
      g_print ("[\n");
      for (GList *l = tags; l != NULL; l = l->next) {
        g_autofree char *etag = json_escape ((const char *)l->data);
        g_print ("  \"%s\"", etag);
        if (l->next) g_print (",");
        g_print ("\n");
      }
      g_print ("]\n");
    } else {
      for (GList *l = tags; l != NULL; l = l->next)
        g_print ("%s\n", (const char *)l->data);
      g_print ("\n%d tags total.\n", g_list_length (tags));
    }
    g_list_free_full (tags, g_free);
    return TRUE;
  }

  return FALSE;
}

static gboolean
handle_cli_data_ops (GTimerCLIOptions *opts, GTimerDBManager *db, const char *db_path)
{
  if (opts->export_csv) {
    GList *tasks = gtimer_db_manager_get_all_tasks (db);
    GString *csv = g_string_new ("task_id,task_name,project,tags,is_timing,is_hidden,total_seconds,today_seconds\n");
    int count = g_list_length (tasks);

    for (GList *l = tasks; l != NULL; l = l->next) {
      GTimerTask *task = GTIMER_TASK (l->data);
      const char *project = gtimer_task_get_project_name (task) ? gtimer_task_get_project_name (task) : "";
      const char *tags = gtimer_task_get_tags (task) ? gtimer_task_get_tags (task) : "";
      g_string_append_printf (csv, "%d,\"%s\",\"%s\",\"%s\",%d,%d,%ld,%ld\n",
          gtimer_task_get_id (task), gtimer_task_get_name (task), project, tags,
          gtimer_task_is_timing (task) ? 1 : 0, gtimer_task_is_hidden (task) ? 1 : 0,
          gtimer_task_get_total_time (task), gtimer_task_get_today_time (task));
    }
    g_list_free_full (tasks, g_object_unref);

    if (!g_file_set_contents (opts->export_csv, csv->str, -1, NULL))
      g_printerr ("Error: Failed to write CSV to %s\n", opts->export_csv);
    else
      g_print ("Exported %d tasks to: %s\n", count, opts->export_csv);
    g_string_free (csv, TRUE);
    return TRUE;
  }

  if (opts->export_json) {
    GList *tasks = gtimer_db_manager_get_all_tasks (db);
    GList *projects = gtimer_db_manager_get_projects (db);
    GString *out = g_string_new ("{\n  \"projects\": [\n");

    for (GList *l = projects; l != NULL; l = l->next) {
      GTimerProject *proj = GTIMER_PROJECT (l->data);
      g_autofree char *ename = json_escape (gtimer_project_get_name (proj));
      g_string_append_printf (out, "    {\"id\": %d, \"name\": \"%s\"}",
          gtimer_project_get_id (proj), ename);
      if (l->next) g_string_append (out, ",");
      g_string_append (out, "\n");
    }
    g_list_free_full (projects, g_object_unref);

    g_string_append (out, "  ],\n  \"tasks\": [\n");

    for (GList *l = tasks; l != NULL; l = l->next) {
      GTimerTask *task = GTIMER_TASK (l->data);
      g_autofree char *ename = json_escape (gtimer_task_get_name (task));
      g_autofree char *eproj = json_escape (gtimer_task_get_project_name (task));
      int task_id = gtimer_task_get_id (task);

      g_string_append_printf (out,
          "    {\"id\": %d, \"name\": \"%s\", \"project\": \"%s\", "
          "\"total_time\": %ld, \"today_time\": %ld, "
          "\"is_timing\": %s, \"is_hidden\": %s",
          task_id, ename, eproj,
          gtimer_task_get_total_time (task), gtimer_task_get_today_time (task),
          gtimer_task_is_timing (task) ? "true" : "false",
          gtimer_task_is_hidden (task) ? "true" : "false");

      /* Tags */
      const char *tags = gtimer_task_get_tags (task);
      if (tags && tags[0]) {
        g_string_append (out, ", \"tags\": [");
        g_auto(GStrv) tag_arr = g_strsplit (tags, ", ", -1);
        for (int ti = 0; tag_arr[ti]; ti++) {
          g_autofree char *etag = json_escape (tag_arr[ti]);
          if (ti > 0) g_string_append (out, ", ");
          g_string_append_printf (out, "\"%s\"", etag);
        }
        g_string_append (out, "]");
      }

      GList *annotations = gtimer_db_manager_get_annotations (db, task_id);
      if (annotations) {
        g_string_append (out, ", \"annotations\": [");
        for (GList *a = annotations; a != NULL; a = a->next) {
          GTimerAnnotation *ann = a->data;
          GDateTime *dt = g_date_time_new_from_unix_local (ann->created_at);
          g_autofree char *date_str = g_date_time_format (dt, "%Y-%m-%d %H:%M");
          g_autofree char *etext = json_escape (ann->text);
          g_string_append_printf (out, "{\"date\": \"%s\", \"text\": \"%s\"}", date_str, etext);
          if (a->next) g_string_append (out, ", ");
          g_date_time_unref (dt);
        }
        g_string_append (out, "]");
        g_list_free_full (annotations, (GDestroyNotify)gtimer_annotation_free);
      }

      g_string_append (out, "}");
      if (l->next) g_string_append (out, ",");
      g_string_append (out, "\n");
    }
    int count = g_list_length (tasks);
    g_list_free_full (tasks, g_object_unref);

    g_string_append (out, "  ]\n}\n");

    if (!g_file_set_contents (opts->export_json, out->str, -1, NULL))
      g_printerr ("Error: Failed to write JSON to %s\n", opts->export_json);
    else
      g_print ("Exported %d tasks to: %s\n", count, opts->export_json);
    g_string_free (out, TRUE);
    return TRUE;
  }

  if (opts->import_csv) {
    g_printerr ("Error: CSV import not yet implemented\n");
    return TRUE;
  }

  if (opts->backup_file) {
    copy_file (db_path, opts->backup_file, "Database backed up to", "Backup failed");
    return TRUE;
  }

  if (opts->restore_file) {
    copy_file (opts->restore_file, db_path, "Database restored from", "Restore failed");
    return TRUE;
  }

  if (opts->export_sqlite) {
    copy_file (db_path, opts->export_sqlite, "SQLite database exported to", "Export failed");
    return TRUE;
  }

  return FALSE;
}

static gboolean
handle_cli_utility (GTimerCLIOptions *opts, GTimerDBManager *db)
{
  if (opts->show_total_time) {
    GList *tasks = gtimer_db_manager_get_all_tasks (db);
    gint64 total = 0;

    for (GList *l = tasks; l != NULL; l = l->next) {
      GTimerTask *task = GTIMER_TASK (l->data);
      total += gtimer_task_get_total_time (task);
    }
    g_list_free_full (tasks, g_object_unref);

    if (opts->output_json) {
      g_print ("{\"total_seconds\": %ld}\n", (long)total);
    } else {
      int hours = total / 3600;
      int mins = (total % 3600) / 60;
      g_print ("Total time tracked: %d hours %d minutes\n", hours, mins);
    }
    return TRUE;
  }

  if (opts->show_active_time) {
    GList *tasks = gtimer_db_manager_get_all_tasks (db);
    gboolean first = TRUE;

    if (opts->output_json) g_print ("[\n");

    for (GList *l = tasks; l != NULL; l = l->next) {
      GTimerTask *task = GTIMER_TASK (l->data);
      if (gtimer_task_is_timing (task)) {
        gint64 last_start = gtimer_task_get_last_start_time (task);
        if (last_start > 0) {
          gint64 now = time (NULL);
          gint64 elapsed = now - last_start;
          if (opts->output_json) {
            g_autofree char *ename = json_escape (gtimer_task_get_name (task));
            if (!first) g_print (",\n");
            g_print ("  {\"id\": %d, \"name\": \"%s\", \"elapsed_seconds\": %ld}",
                gtimer_task_get_id (task), ename, (long)elapsed);
            first = FALSE;
          } else {
            int hours = elapsed / 3600;
            int mins = (elapsed % 3600) / 60;
            int secs = elapsed % 60;
            g_print ("%s: %02d:%02d:%02d\n", gtimer_task_get_name (task), hours, mins, secs);
          }
        }
      }
    }
    g_list_free_full (tasks, g_object_unref);

    if (opts->output_json) g_print ("\n]\n");
    return TRUE;
  }

  if (opts->show_summary) {
    GList *tasks = gtimer_db_manager_get_all_tasks (db);
    int running = 0;
    int total_tasks = 0;
    gint64 today_total = 0;

    for (GList *l = tasks; l != NULL; l = l->next) {
      GTimerTask *task = GTIMER_TASK (l->data);
      if (gtimer_task_is_timing (task)) running++;
      today_total += gtimer_task_get_today_time (task);
      total_tasks++;
    }
    g_list_free_full (tasks, g_object_unref);

    if (opts->output_json) {
      g_print ("{\"running\": %d, \"total_tasks\": %d, \"today_seconds\": %ld}\n",
          running, total_tasks, (long)today_total);
    } else {
      int hours = today_total / 3600;
      int mins = (today_total % 3600) / 60;
      g_print ("%d tasks running, %d:%02d today\n", running, hours, mins);
    }
    return TRUE;
  }

  if (opts->merge_source_id > 0 && opts->merge_target_id > 0) {
    g_printerr ("Merge task not yet implemented (requires db function)\n");
    return TRUE;
  }

  if (opts->vacuum_db) {
    sqlite3 *sql_db = gtimer_db_manager_get_db (db);
    int rc = sqlite3_exec (sql_db, "VACUUM;", NULL, NULL, NULL);
    if (rc == SQLITE_OK)
      g_print ("Database compacted\n");
    else
      g_printerr ("Vacuum failed\n");
    return TRUE;
  }

  return FALSE;
}

static void
handle_cli_options (GTimerCLIOptions *opts, GTimerDBManager *db, const char *db_path)
{
  if (opts->list_tasks) { list_all_tasks (db, opts->output_json, opts->filter_tag); return; }
  if (opts->show_status) { print_status (db, opts->output_json); return; }
  if (handle_cli_timer_control (opts, db)) return;
  if (handle_cli_task_management (opts, db)) return;
  if (handle_cli_reports (opts, db)) return;
  if (handle_cli_project_management (opts, db)) return;
  if (handle_cli_annotations (opts, db)) return;
  if (handle_cli_tags (opts, db)) return;
  if (handle_cli_data_ops (opts, db, db_path)) return;
  if (handle_cli_utility (opts, db)) return;
}

static void
on_timer_tick (GTimerTimerService *service, gint64 elapsed, gpointer user_data)
{
  (void)elapsed;
  GtkApplication *app = GTK_APPLICATION (user_data);
  static gint64 last_notif_time = 0;
  gint64 now = time (NULL);
  
  // Update notification if app is in background, throttle to once every 10s
  if (!gtk_application_get_active_window (app)) {
    if (now - last_notif_time >= 10) {
      GTimerTask *task = gtimer_timer_service_get_active_task (service);
      if (task) {
        GNotification *notif = g_notification_new (_("GTimer Running"));
        char *body = g_strdup_printf (_("Timing: %s"), gtimer_task_get_name (task));
        g_notification_set_body (notif, body);
        g_free (body);
        g_notification_set_default_action (notif, "app.activate");
        g_application_send_notification (G_APPLICATION (app), "timer-active", notif);
        g_object_unref (notif);
        last_notif_time = now;
      }
    }
  } else {
    // App is active, withdraw notification
    g_application_withdraw_notification (G_APPLICATION (app), "timer-active");
    last_notif_time = 0;
  }
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
  GTimerApp *gtimer_app = user_data;
  GTimerWindow *window;

  // Ensure icons from resources are found by the icon theme
  GtkIconTheme *theme = gtk_icon_theme_get_for_display (gdk_display_get_default ());
  gtk_icon_theme_add_resource_path (theme, "/us/k5n/GTimer/icons");
  
  // Set default icon name for all windows
  gtk_window_set_default_icon_name ("us.k5n.GTimer");

  // Load CSS
  GtkCssProvider *provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (provider, "/us/k5n/GTimer/style.css");
  gtk_style_context_add_provider_for_display (gdk_display_get_default (),
                                             GTK_STYLE_PROVIDER (provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);

  window = gtimer_window_new (app);
  
  gtimer_window_set_task_list_model (window, gtimer_app->task_list_model);
  gtimer_window_set_timer_service (window, gtimer_app->timer_service);

  // Connect to tick for background notifications
  g_signal_connect (gtimer_app->timer_service, "tick", G_CALLBACK (on_timer_tick), app);

  gtimer_task_list_model_refresh (gtimer_app->task_list_model);

  /* Check for tasks that ran for a long time while app was closed */
  gtimer_window_check_stale_timers (window);

  gtk_window_present (GTK_WINDOW (window));
}

static GFile *last_save_folder = NULL;

static void
on_html_save_response (GtkNativeDialog *native, int response_id, gpointer user_data)
{
  char *report_text = user_data;
  if (response_id == GTK_RESPONSE_ACCEPT) {
    GtkFileChooser *chooser = GTK_FILE_CHOOSER (native);
    GFile *file = gtk_file_chooser_get_file (chooser);
    if (file) {
      // Save the folder for next time
      g_clear_object (&last_save_folder);
      last_save_folder = g_file_get_parent (file);

      GError *err = NULL;
      if (g_file_replace_contents (file, report_text, strlen (report_text), NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL, &err)) {
        char *uri = g_file_get_uri (file);
        gtk_show_uri (gtk_native_dialog_get_transient_for (native), uri, GDK_CURRENT_TIME);
        g_free (uri);
      } else {
        g_warning ("Failed to save HTML report: %s", err->message);
        g_clear_error (&err);
      }
      g_object_unref (file);
    }
  }
  g_free (report_text);
  g_object_unref (native);
}

static void
on_report_dialog_response (GtkDialog *dialog, int response_id, gpointer user_data)
{
  if (response_id == GTK_RESPONSE_OK) {
    GtkApplication *app = GTK_APPLICATION (user_data);
    GTimerApp *gtimer_app = g_object_get_data (G_OBJECT (app), "gtimer-app");
    GtkWindow *parent = gtk_application_get_active_window (app);
    GtkWidget *content_area = gtk_dialog_get_content_area (dialog);
    GtkDropDown *round_dropdown = g_object_get_data (G_OBJECT (content_area), "round-dropdown");
    GtkDropDown *format_dropdown = g_object_get_data (G_OBJECT (content_area), "format-dropdown");
    
    int rounding = 0;
    guint selected = gtk_drop_down_get_selected (round_dropdown);
    switch (selected) {
      case 1: rounding = 1; break;
      case 2: rounding = 5; break;
      case 3: rounding = 10; break;
      case 4: rounding = 15; break;
      case 5: rounding = 30; break;
      case 6: rounding = 60; break;
      default: rounding = 0; break;
    }

    GTimerReportFormat format = GTIMER_REPORT_TEXT;
    if (gtk_drop_down_get_selected (format_dropdown) == 1) {
      format = GTIMER_REPORT_HTML;
    }

    GDateTime *now = g_date_time_new_now_local ();
    GDateTime *start_date = g_date_time_ref (now);
    GDateTime *end_date = g_date_time_ref (now);
    
    guint range_idx = gtk_drop_down_get_selected (g_object_get_data (G_OBJECT (content_area), "range-dropdown"));
    
    // Simplistic range calculation
    switch (range_idx) {
      case 0: // Today
        break;
      case 1: // This Week
        start_date = g_date_time_add_days (now, -(int)g_date_time_get_day_of_week (now) + 1);
        break;
      case 2: // Last Week
        {
          GDateTime *last_week = g_date_time_add_days (now, -7);
          start_date = g_date_time_add_days (last_week, -(int)g_date_time_get_day_of_week (last_week) + 1);
          end_date = g_date_time_add_days (start_date, 6);
          g_date_time_unref (last_week);
        }
        break;
      case 5: // This Month
        start_date = g_date_time_new_local (g_date_time_get_year (now), g_date_time_get_month (now), 1, 0, 0, 0);
        break;
      case 7: // This Year
        start_date = g_date_time_new_local (g_date_time_get_year (now), 1, 1, 0, 0, 0);
        break;
      default:
        // Handle other ranges if needed
        break;
    }

    char *report_text = gtimer_report_generate (gtimer_app->db_manager,
                                                GTIMER_REPORT_DAILY, // Not used in engine anymore
                                                format,
                                                start_date, end_date, NULL, rounding);
    
    if (format == GTIMER_REPORT_HTML) {
      GtkFileChooserNative *native = gtk_file_chooser_native_new (_("Save HTML Report"),
                                                                  parent,
                                                                  GTK_FILE_CHOOSER_ACTION_SAVE,
                                                                  _("_Save"),
                                                                  _("_Cancel"));
      
      char *date_slug = g_date_time_format (now, "%Y%m%d");
      char *default_filename = g_strdup_printf ("gtimer-report-%s.html", date_slug);
      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (native), default_filename);
      g_free (default_filename);
      g_free (date_slug);

      if (last_save_folder) {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (native), last_save_folder, NULL);
      }

      g_signal_connect (native, "response", G_CALLBACK (on_html_save_response), g_strdup (report_text));
      gtk_native_dialog_show (GTK_NATIVE_DIALOG (native));
    } else {
      GTimerReportWindow *report_window = gtimer_report_window_new (parent, _("Report"), report_text);
      gtk_window_present (GTK_WINDOW (report_window));
    }
    
    g_free (report_text);
    g_date_time_unref (now);
    g_date_time_unref (start_date);
    g_date_time_unref (end_date);
  }
  gtk_window_destroy (GTK_WINDOW (dialog));
}

static void
on_report_action (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       user_data)
{
  (void)action;
  (void)parameter;
  GtkApplication *app = GTK_APPLICATION (user_data);
  GtkWindow *parent = gtk_application_get_active_window (app);
  GTimerApp *gtimer_app = g_object_get_data (G_OBJECT (app), "gtimer-app");
  
  GtkWidget *dialog = gtk_dialog_new_with_buttons (_("Generate Report"),
                                                   parent,
                                                   GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR,
                                                   _("_Cancel"), GTK_RESPONSE_CANCEL,
                                                   _("_Generate"), GTK_RESPONSE_OK,
                                                   NULL);
  
  GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_box_set_spacing (GTK_BOX (content_area), 12);
  gtk_widget_set_margin_start (content_area, 12);
  gtk_widget_set_margin_end (content_area, 12);
  gtk_widget_set_margin_top (content_area, 12);
  gtk_widget_set_margin_bottom (content_area, 12);

  // Report Type
  GtkWidget *type_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_append (GTK_BOX (type_box), gtk_label_new (_("Report Type:")));
  GtkStringList *type_list = gtk_string_list_new ((const char *[]) {_("Daily"), _("Weekly"), _("Monthly"), _("Yearly"), NULL});
  GtkDropDown *type_dropdown = GTK_DROP_DOWN (gtk_drop_down_new (G_LIST_MODEL (type_list), NULL));
  gtk_box_append (GTK_BOX (type_box), GTK_WIDGET (type_dropdown));
  gtk_box_append (GTK_BOX (content_area), type_box);

  // Time Range
  GtkWidget *range_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_append (GTK_BOX (range_box), gtk_label_new (_("Time Range:")));
  GtkStringList *range_list = gtk_string_list_new ((const char *[]) {
    _("Today"), _("This Week"), _("Last Week"), _("This & Last Week"), _("Last Two Weeks"), 
    _("This Month"), _("Last Month"), _("This Year"), _("Last Year"), NULL
  });
  GtkDropDown *range_dropdown = GTK_DROP_DOWN (gtk_drop_down_new (G_LIST_MODEL (range_list), NULL));
  gtk_box_append (GTK_BOX (range_box), GTK_WIDGET (range_dropdown));
  gtk_box_append (GTK_BOX (content_area), range_box);

  // Rounding
  GtkWidget *round_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_append (GTK_BOX (round_box), gtk_label_new (_("Rounding:")));
  GtkStringList *round_list = gtk_string_list_new ((const char *[]) {
    _("None"), _("1 Minute"), _("5 Minutes"), _("10 Minutes"), _("15 Minutes"), _("30 Minutes"), _("1 Hour"), NULL
  });
  GtkDropDown *round_dropdown = GTK_DROP_DOWN (gtk_drop_down_new (G_LIST_MODEL (round_list), NULL));
  gtk_box_append (GTK_BOX (round_box), GTK_WIDGET (round_dropdown));
  gtk_box_append (GTK_BOX (content_area), round_box);

  // Format
  GtkWidget *format_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_append (GTK_BOX (format_box), gtk_label_new (_("Format:")));
  GtkStringList *format_list = gtk_string_list_new ((const char *[]) {_("Plain Text"), _("HTML"), NULL});
  GtkDropDown *format_dropdown = GTK_DROP_DOWN (gtk_drop_down_new (G_LIST_MODEL (format_list), NULL));
  gtk_box_append (GTK_BOX (format_box), GTK_WIDGET (format_dropdown));
  gtk_box_append (GTK_BOX (content_area), format_box);

  // Task Selection (Checkbox list)
  gtk_box_append (GTK_BOX (content_area), gtk_label_new (_("Tasks:")));
  GtkWidget *scrolled = gtk_scrolled_window_new ();
  gtk_widget_set_size_request (scrolled, -1, 200);
  gtk_box_append (GTK_BOX (content_area), scrolled);
  
  GtkWidget *task_list = gtk_list_box_new ();
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (task_list), GTK_SELECTION_NONE);
  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled), task_list);

  GListModel *model = gtimer_task_list_model_get_model (gtimer_app->task_list_model);
  guint n_items = g_list_model_get_n_items (model);
  for (guint i = 0; i < n_items; i++) {
    GTimerTask *task = GTIMER_TASK (g_list_model_get_item (model, i));
    GtkWidget *row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *check = gtk_check_button_new ();
    gtk_check_button_set_active (GTK_CHECK_BUTTON (check), TRUE);
    gtk_box_append (GTK_BOX (row), check);
    
    // Task info box with name and project
    GtkWidget *task_info_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_valign (task_info_box, GTK_ALIGN_CENTER);
    
    // Task name
    const char *task_name = gtimer_task_get_name (task);
    GtkWidget *task_label = gtk_label_new (task_name);
    gtk_widget_set_halign (task_label, GTK_ALIGN_START);
    gtk_box_append (GTK_BOX (task_info_box), task_label);
    
    // Project name (secondary, dimmed)
    const char *project_name = gtimer_task_get_project_name (task);
    if (project_name && strlen (project_name) > 0) {
      GtkWidget *project_label = gtk_label_new (project_name);
      gtk_widget_set_halign (project_label, GTK_ALIGN_START);
      gtk_widget_add_css_class (project_label, "dim-label");
      gtk_box_append (GTK_BOX (task_info_box), project_label);
    }
    
    gtk_box_append (GTK_BOX (row), task_info_box);
    gtk_list_box_append (GTK_LIST_BOX (task_list), row);
    g_object_set_data (G_OBJECT (row), "task-id", GINT_TO_POINTER (gtimer_task_get_id (task)));
    g_object_set_data (G_OBJECT (row), "check", check);
    g_object_unref (task);
  }

  g_object_set_data (G_OBJECT (content_area), "range-dropdown", range_dropdown);
  g_object_set_data (G_OBJECT (content_area), "round-dropdown", round_dropdown);
  g_object_set_data (G_OBJECT (content_area), "format-dropdown", format_dropdown);
  g_object_set_data (G_OBJECT (content_area), "task-list", task_list);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  
  // For now, reuse the simple daily report generation logic when Generate is clicked
  g_signal_connect (dialog, "response", G_CALLBACK (on_report_dialog_response), app);
  gtk_widget_show (dialog);
}

static void
on_preferences_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  (void)action; (void)parameter;
  GtkApplication *app = GTK_APPLICATION (user_data);
  GtkWindow *parent = gtk_application_get_active_window (app);
  GSettings *settings = g_settings_new ("us.k5n.GTimer");

  GtkWidget *dialog = adw_preferences_window_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);

  // General Page
  AdwPreferencesPage *page = ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
  adw_preferences_page_set_title (page, _("General"));
  adw_preferences_page_set_icon_name (page, "preferences-system-symbolic");
  adw_preferences_window_add (ADW_PREFERENCES_WINDOW (dialog), page);

  AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (group, _("Behavior"));
  adw_preferences_page_add (page, group);

  // Auto Save
  AdwActionRow *auto_save_row = ADW_ACTION_ROW (adw_action_row_new ());
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (auto_save_row), _("Auto Save"));
  GtkWidget *auto_save_switch = gtk_switch_new ();
  gtk_widget_set_valign (auto_save_switch, GTK_ALIGN_CENTER);
  adw_action_row_add_suffix (auto_save_row, auto_save_switch);
  adw_preferences_group_add (group, GTK_WIDGET (auto_save_row));
  g_settings_bind (settings, "auto-save", auto_save_switch, "active", G_SETTINGS_BIND_DEFAULT);

  // Animate
  AdwActionRow *animate_row = ADW_ACTION_ROW (adw_action_row_new ());
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (animate_row), _("Animate Running Tasks"));
  GtkWidget *animate_switch = gtk_switch_new ();
  gtk_widget_set_valign (animate_switch, GTK_ALIGN_CENTER);
  adw_action_row_add_suffix (animate_row, animate_switch);
  adw_preferences_group_add (group, GTK_WIDGET (animate_row));
  g_settings_bind (settings, "animate-running-tasks", animate_switch, "active", G_SETTINGS_BIND_DEFAULT);

  // Resume
  AdwActionRow *resume_row = ADW_ACTION_ROW (adw_action_row_new ());
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (resume_row), _("Resume Timing on Startup"));
  GtkWidget *resume_switch = gtk_switch_new ();
  gtk_widget_set_valign (resume_switch, GTK_ALIGN_CENTER);
  adw_action_row_add_suffix (resume_row, resume_switch);
  adw_preferences_group_add (group, GTK_WIDGET (resume_row));
  g_settings_bind (settings, "resume-on-startup", resume_switch, "active", G_SETTINGS_BIND_DEFAULT);

  // Tracking Reminder
  AdwActionRow *reminder_row = ADW_ACTION_ROW (adw_action_row_new ());
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (reminder_row), _("Remind When Not Tracking"));
  adw_action_row_set_subtitle (reminder_row, _("Notify if no task is running"));
  GtkWidget *reminder_switch = gtk_switch_new ();
  gtk_widget_set_valign (reminder_switch, GTK_ALIGN_CENTER);
  adw_action_row_add_suffix (reminder_row, reminder_switch);
  adw_preferences_group_add (group, GTK_WIDGET (reminder_row));
  g_settings_bind (settings, "enable-tracking-reminder", reminder_switch, "active", G_SETTINGS_BIND_DEFAULT);

  AdwActionRow *reminder_interval_row = ADW_ACTION_ROW (adw_action_row_new ());
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (reminder_interval_row), _("Reminder Interval (minutes)"));
  GtkWidget *reminder_spin = gtk_spin_button_new (gtk_adjustment_new (15, 1, 120, 1, 10, 0), 1, 0);
  gtk_widget_set_valign (reminder_spin, GTK_ALIGN_CENTER);
  adw_action_row_add_suffix (reminder_interval_row, reminder_spin);
  adw_preferences_group_add (group, GTK_WIDGET (reminder_interval_row));
  g_settings_bind (settings, "tracking-reminder-interval", reminder_spin, "value", G_SETTINGS_BIND_DEFAULT);

  // Idle Detection Page
  page = ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
  adw_preferences_page_set_title (page, _("Idle Detection"));
  adw_preferences_page_set_icon_name (page, "preferences-desktop-screensaver-symbolic");
  adw_preferences_window_add (ADW_PREFERENCES_WINDOW (dialog), page);

  group = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_page_add (page, group);

  // Idle Enable
  AdwActionRow *idle_enable_row = ADW_ACTION_ROW (adw_action_row_new ());
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (idle_enable_row), _("Enable Idle Detection"));
  GtkWidget *idle_enable_switch = gtk_switch_new ();
  gtk_widget_set_valign (idle_enable_switch, GTK_ALIGN_CENTER);
  adw_action_row_add_suffix (idle_enable_row, idle_enable_switch);
  adw_preferences_group_add (group, GTK_WIDGET (idle_enable_row));
  g_settings_bind (settings, "enable-idle-detection", idle_enable_switch, "active", G_SETTINGS_BIND_DEFAULT);

  // Idle Threshold
  AdwActionRow *idle_threshold_row = ADW_ACTION_ROW (adw_action_row_new ());
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (idle_threshold_row), _("Idle Threshold (minutes)"));
  GtkWidget *idle_threshold_spin = gtk_spin_button_new (gtk_adjustment_new (15, 1, 120, 1, 10, 0), 1, 0);
  gtk_widget_set_valign (idle_threshold_spin, GTK_ALIGN_CENTER);
  adw_action_row_add_suffix (idle_threshold_row, idle_threshold_spin);
  adw_preferences_group_add (group, GTK_WIDGET (idle_threshold_row));
  g_settings_bind (settings, "idle-threshold", idle_threshold_spin, "value", G_SETTINGS_BIND_DEFAULT);

  // Time Page
  page = ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
  adw_preferences_page_set_title (page, _("Time"));
  adw_preferences_page_set_icon_name (page, "preferences-system-time-symbolic");
  adw_preferences_window_add (ADW_PREFERENCES_WINDOW (dialog), page);

  group = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_page_add (page, group);

  // Midnight Offset
  AdwActionRow *midnight_row = ADW_ACTION_ROW (adw_action_row_new ());
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (midnight_row), _("Day Start (Midnight Offset)"));
  GtkWidget *midnight_spin = gtk_spin_button_new (gtk_adjustment_new (0, 0, 23, 1, 1, 0), 1, 0);
  gtk_widget_set_valign (midnight_spin, GTK_ALIGN_CENTER);
  adw_action_row_add_suffix (midnight_row, midnight_spin);
  adw_preferences_group_add (group, GTK_WIDGET (midnight_row));
  g_settings_bind (settings, "midnight-offset", midnight_spin, "value", G_SETTINGS_BIND_DEFAULT);

  gtk_widget_show (dialog);
  g_object_unref (settings);
}

static void
on_activate_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  (void)action; (void)parameter;
  GtkApplication *app = GTK_APPLICATION (user_data);
  GtkWindow *window = gtk_application_get_active_window (app);
  if (window) {
    gtk_window_present (window);
  }
}

static void
on_about_action (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  (void)action; (void)parameter;
  GtkApplication *app = GTK_APPLICATION (user_data);
  GtkWindow *parent = gtk_application_get_active_window (app);

  gtk_show_about_dialog (parent,
                         "program-name", "GTimer",
                         "logo-icon-name", "us.k5n.GTimer",
                         "version", VERSION,
                         "copyright", "© 1998-2026 Craig Knudsen",
                         "authors", (const char *[]) {"Craig Knudsen", NULL},
                         "license-type", GTK_LICENSE_GPL_2_0,
                         "website", "http://www.k5n.us/gtimer.php",
                         "website-label", _("Website: k5n.us/gtimer"),
                         "comments", _("3.0.0 (2026-02-10)\n"
                                     "• Complete rewrite with GTK 4 and Libadwaita\n"
                                     "• Modernized UI with Search, Secondary Toolbar and Toast notifications\n"
                                     "• Improved data integrity with Auto-save and Midnight Rollover\n"
                                     "• Enhanced reporting with HTML support and search\n"
                                     "• Robust idle detection for Wayland and X11"),
                         NULL);
}

static void
on_save_action (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  (void)action;
  (void)parameter;
  (void)user_data;
}

static void
on_quit_action (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  (void)action;
  (void)parameter;
  GtkApplication *app = GTK_APPLICATION (user_data);
  GtkWindow *window = gtk_application_get_active_window (app);
  if (window)
    gtk_window_close (window);  /* Routes through close-request handler */
  else
    g_application_quit (G_APPLICATION (app));
}

int
main (int argc, char **argv)
{
  AdwApplication *app;
  GTimerApp gtimer_app = {0};
  GError *error = NULL;
  int status;

  GTimerCLIOptions cli_opts = {0};
  GOptionContext *context;
  GOptionEntry entries[] = {
    { "start", 's', 0, G_OPTION_ARG_INT, &cli_opts.task_id, "Start timing task with given ID", "TASK-ID" },
    { "stop", 't', 0, G_OPTION_ARG_NONE, &cli_opts.stop_timer, "Stop current timing task", NULL },
    { "stop-all", 'S', 0, G_OPTION_ARG_NONE, &cli_opts.stop_all_timers, "Stop all timing tasks", NULL },
    { "status", 'u', 0, G_OPTION_ARG_NONE, &cli_opts.show_status, "Show current timer status", NULL },
    { "list-tasks", 'l', 0, G_OPTION_ARG_NONE, &cli_opts.list_tasks, "List all tasks", NULL },
    { "add-task", 'a', 0, G_OPTION_ARG_STRING, &cli_opts.add_task, "Add a new task", "TASK-NAME" },
    { "project", 'p', 0, G_OPTION_ARG_INT, &cli_opts.add_task_project, "Project ID for --add-task (1-based)", "PROJECT-ID" },
    { "report", 'r', 0, G_OPTION_ARG_STRING, &cli_opts.report_type, "Generate report (daily, weekly, monthly)", "TYPE" },
    { "report-file", 'f', 0, G_OPTION_ARG_STRING, &cli_opts.report_file, "Save report to file", "FILENAME" },
    { "export-csv", 'e', 0, G_OPTION_ARG_STRING, &cli_opts.export_csv, "Export tasks to CSV", "FILENAME" },
    { "import-csv", 'i', 0, G_OPTION_ARG_STRING, &cli_opts.import_csv, "Import tasks from CSV", "FILENAME" },
    { "version", 'v', 0, G_OPTION_ARG_NONE, &cli_opts.show_version, "Show version number", NULL },
    { "show", 'w', 0, G_OPTION_ARG_NONE, &cli_opts.show_gui, "Show GUI window", NULL },
    { "datadir", 'd', 0, G_OPTION_ARG_STRING, &cli_opts.datadir, "Use alternate database directory", "PATH" },
    { "database", 'D', 0, G_OPTION_ARG_STRING, &cli_opts.database, "Use specific database file", "FILE" },
    { "verbose", 0, 0, G_OPTION_ARG_NONE, &cli_opts.verbose, "Enable verbose output", NULL },
    { "quiet", 'q', 0, G_OPTION_ARG_NONE, &cli_opts.quiet, "Suppress non-essential output", NULL },
    /* Date-based */
    { "today", 0, 0, G_OPTION_ARG_NONE, &cli_opts.show_today, "Show today's summary", NULL },
    { "week", 0, 0, G_OPTION_ARG_NONE, &cli_opts.show_week, "Show this week's summary", NULL },
    { "month", 0, 0, G_OPTION_ARG_NONE, &cli_opts.show_month, "Show this month's summary", NULL },
    { "since", 0, 0, G_OPTION_ARG_STRING, &cli_opts.since_date, "Report from date (YYYY-MM-DD)", "DATE" },
    { "until", 0, 0, G_OPTION_ARG_STRING, &cli_opts.until_date, "Report until date (YYYY-MM-DD)", "DATE" },
    { "date", 0, 0, G_OPTION_ARG_STRING, &cli_opts.specific_date, "Show report for specific date", "DATE" },
    /* Task management */
    { "delete-task", 0, 0, G_OPTION_ARG_INT, &cli_opts.delete_task_id, "Delete task by ID", "ID" },
    { "hide-task", 0, 0, G_OPTION_ARG_INT, &cli_opts.hide_task_id, "Hide task by ID", "ID" },
    { "unhide-task", 0, 0, G_OPTION_ARG_INT, &cli_opts.unhide_task_id, "Unhide task by ID", "ID" },
    { "rename-task", 0, 0, G_OPTION_ARG_STRING, &cli_opts.rename_task, "New name for task", "NAME" },
    { "rename-task-id", 0, 0, G_OPTION_ARG_INT, &cli_opts.rename_task_id, "Task ID to rename", "ID" },
    { "move-task", 0, 0, G_OPTION_ARG_INT, &cli_opts.move_task_id, "Move task to different project", "ID" },
    { "move-to-project", 0, 0, G_OPTION_ARG_INT, &cli_opts.move_to_project, "Destination project ID", "ID" },
    { "task-details", 0, 0, G_OPTION_ARG_INT, &cli_opts.task_details_id, "Show detailed task info", "ID" },
    { "reset-task", 0, 0, G_OPTION_ARG_INT, &cli_opts.reset_task_id, "Clear all time for task", "ID" },
    { "duplicate", 0, 0, G_OPTION_ARG_INT, &cli_opts.duplicate_task_id, "Duplicate a task", "ID" },
    /* Project management */
    { "list-projects", 0, 0, G_OPTION_ARG_NONE, &cli_opts.list_projects, "List all projects", NULL },
    { "add-project", 0, 0, G_OPTION_ARG_STRING, &cli_opts.add_project, "Create new project", "NAME" },
    { "delete-project", 0, 0, G_OPTION_ARG_INT, &cli_opts.delete_project_id, "Delete project by ID", "ID" },
    { "rename-project", 0, 0, G_OPTION_ARG_INT, &cli_opts.rename_project_id, "Project ID to rename", "ID" },
    { "rename-project-to", 0, 0, G_OPTION_ARG_STRING, &cli_opts.rename_project_name, "New project name", "NAME" },
    /* Annotations */
    { "annotate", 0, 0, G_OPTION_ARG_STRING, &cli_opts.annotate_text, "Add annotation text", "TEXT" },
    { "annotate-task", 0, 0, G_OPTION_ARG_INT, &cli_opts.annotate_task_id, "Task ID to annotate", "ID" },
    { "list-annotations", 0, 0, G_OPTION_ARG_INT, &cli_opts.list_annotations_id, "List annotations for task", "ID" },
    { "note", 0, 0, G_OPTION_ARG_STRING, &cli_opts.note_text, "Add note to current task", "TEXT" },
    /* Tags */
    { "add-tag", 0, 0, G_OPTION_ARG_STRING, &cli_opts.add_tag, "Add tag to task", "TAG" },
    { "add-tag-task", 0, 0, G_OPTION_ARG_INT, &cli_opts.add_tag_task_id, "Task ID for --add-tag", "ID" },
    { "remove-tag", 0, 0, G_OPTION_ARG_STRING, &cli_opts.remove_tag, "Remove tag from task", "TAG" },
    { "remove-tag-task", 0, 0, G_OPTION_ARG_INT, &cli_opts.remove_tag_task_id, "Task ID for --remove-tag", "ID" },
    { "list-tags", 0, 0, G_OPTION_ARG_NONE, &cli_opts.list_tags, "List all tags", NULL },
    { "tag", 0, 0, G_OPTION_ARG_STRING, &cli_opts.filter_tag, "Filter tasks by tag", "TAG" },
    /* Data/export */
    { "json", 'j', 0, G_OPTION_ARG_NONE, &cli_opts.output_json, "Output in JSON format", NULL },
    { "export-json", 'J', 0, G_OPTION_ARG_STRING, &cli_opts.export_json, "Export all data to JSON file", "FILENAME" },
    { "backup", 0, 0, G_OPTION_ARG_STRING, &cli_opts.backup_file, "Backup database to file", "FILE" },
    { "restore", 0, 0, G_OPTION_ARG_STRING, &cli_opts.restore_file, "Restore database from file", "FILE" },
    { "export-sqlite", 0, 0, G_OPTION_ARG_STRING, &cli_opts.export_sqlite, "Export raw SQLite database", "FILE" },
    /* Utility */
    { "total-time", 0, 0, G_OPTION_ARG_NONE, &cli_opts.show_total_time, "Show total time across all tasks", NULL },
    { "active-time", 0, 0, G_OPTION_ARG_NONE, &cli_opts.show_active_time, "Show active timer duration", NULL },
    { "summary", 0, 0, G_OPTION_ARG_NONE, &cli_opts.show_summary, "Show one-line summary", NULL },
    { "merge-source", 0, 0, G_OPTION_ARG_INT, &cli_opts.merge_source_id, "Source task ID for merge", "ID" },
    { "merge-target", 0, 0, G_OPTION_ARG_INT, &cli_opts.merge_target_id, "Target task ID for merge", "ID" },
    { "vacuum", 0, 0, G_OPTION_ARG_NONE, &cli_opts.vacuum_db, "Compact database", NULL },
    { NULL }
  };

  context = g_option_context_new("- GTimer time tracking application");
  g_option_context_add_main_entries(context, entries, NULL);
  g_option_context_set_ignore_unknown_options(context, TRUE);

  if (!g_option_context_parse(context, &argc, &argv, &error)) {
    g_printerr("Error parsing options: %s\n", error ? error->message : "unknown");
    g_clear_error(&error);
  }

  if (cli_opts.verbose) {
    g_setenv("G_MESSAGES_DEBUG", "all", TRUE);
  }

  g_set_prgname ("us.k5n.GTimer");
  g_set_application_name ("GTimer");

  const char *data_dir;
  char *db_path = NULL;
  char *gtimer_dir = NULL;

  if (cli_opts.database) {
    db_path = g_strdup(cli_opts.database);
  } else {
    if (cli_opts.datadir) {
      data_dir = cli_opts.datadir;
    } else {
      data_dir = g_get_user_data_dir();
    }
    gtimer_dir = g_build_filename(data_dir, "gtimer", NULL);
    g_mkdir_with_parents(gtimer_dir, 0755);
    db_path = g_build_filename(gtimer_dir, "gtimer.db", NULL);
  }

  gtimer_app.db_manager = gtimer_db_manager_new(db_path, &error);
  if (!gtimer_app.db_manager) {
    g_printerr("Failed to initialize database: %s\n", error->message);
    return 1;
  }

  if (cli_opts.show_version || cli_opts.show_status || cli_opts.list_tasks ||
      cli_opts.stop_timer || cli_opts.stop_all_timers || cli_opts.task_id > 0 ||
      cli_opts.add_task || cli_opts.report_type || cli_opts.export_csv || cli_opts.export_json || cli_opts.import_csv ||
      cli_opts.show_today || cli_opts.show_week || cli_opts.show_month ||
      cli_opts.since_date || cli_opts.until_date || cli_opts.specific_date ||
      cli_opts.delete_task_id > 0 || cli_opts.hide_task_id > 0 || cli_opts.unhide_task_id > 0 ||
      cli_opts.rename_task || cli_opts.move_task_id > 0 || cli_opts.task_details_id > 0 ||
      cli_opts.reset_task_id > 0 || cli_opts.duplicate_task_id > 0 ||
      cli_opts.list_projects || cli_opts.add_project || cli_opts.delete_project_id > 0 ||
      cli_opts.rename_project_id > 0 || cli_opts.annotate_text || cli_opts.list_annotations_id > 0 ||
      cli_opts.note_text || cli_opts.add_tag || cli_opts.remove_tag || cli_opts.list_tags ||
      cli_opts.backup_file || cli_opts.restore_file || cli_opts.export_sqlite ||
      cli_opts.show_total_time || cli_opts.show_active_time || cli_opts.show_summary ||
      cli_opts.merge_source_id > 0 || cli_opts.vacuum_db) {
    if (!cli_opts.quiet) {
      g_print("GTimer %s\n", VERSION);
    }
    handle_cli_options(&cli_opts, gtimer_app.db_manager, db_path);
    g_object_unref(gtimer_app.db_manager);
    g_free(db_path);
    g_free(gtimer_dir);
    return 0;
  }

  /* The GUI requires the GSettings schema. g_settings_new() aborts the
   * process if it is missing, so check up front and give a useful hint. */
  GSettingsSchemaSource *schema_source = g_settings_schema_source_get_default ();
  GSettingsSchema *schema = schema_source != NULL
    ? g_settings_schema_source_lookup (schema_source, "us.k5n.GTimer", TRUE)
    : NULL;
  if (schema == NULL) {
    g_printerr ("Error: GSettings schema 'us.k5n.GTimer' is not installed.\n"
                "\n"
                "If running from the build directory without installing, use:\n"
                "  GSETTINGS_SCHEMA_DIR=build/data ./build/src/gtimer\n"
                "\n"
                "Or install the application first:\n"
                "  sudo meson install -C build\n");
    g_object_unref (gtimer_app.db_manager);
    g_free (db_path);
    g_free (gtimer_dir);
    return 1;
  }
  g_settings_schema_unref (schema);

  app = adw_application_new ("us.k5n.GTimer", G_APPLICATION_FLAGS_NONE);

  const GActionEntry app_entries[] = {
    { .name = "about", .activate = on_about_action },
    { .name = "report", .activate = on_report_action },
    { .name = "save", .activate = on_save_action },
    { .name = "quit", .activate = on_quit_action },
    { .name = "preferences", .activate = on_preferences_action },
    { .name = "activate", .activate = on_activate_action },
  };
  g_action_map_add_action_entries (G_ACTION_MAP (app), app_entries, G_N_ELEMENTS (app_entries), app);
  
  // Store gtimer_app for callbacks
  g_object_set_data (G_OBJECT (app), "gtimer-app", &gtimer_app);

  gtimer_app.task_list_model = gtimer_task_list_model_new (gtimer_app.db_manager);
  gtimer_task_list_model_refresh (gtimer_app.task_list_model);

  gtimer_app.timer_service = gtimer_timer_service_new (gtimer_app.db_manager);
  
  // Set up idle monitoring
  gtimer_app.idle_monitor = gtimer_idle_monitor_new ();
  if (gtimer_idle_monitor_is_available (gtimer_app.idle_monitor)) {
    gtimer_timer_service_set_idle_monitor (gtimer_app.timer_service, gtimer_app.idle_monitor);
    g_debug ("Idle monitoring enabled");
  } else {
    g_debug ("Idle monitoring not available (not running under GNOME/Mutter)");
  }

  g_signal_connect (app, "activate", G_CALLBACK (activate), &gtimer_app);

  // Register Accelerators
  struct { const char *action; const char *accels[2]; } accels[] = {
    { "app.quit", { "<Control>q", NULL } },
    { "app.save", { "<Control>s", NULL } },
    { "app.report", { "<Control>r", NULL } },
    { "win.start-stop", { "<Alt>s", NULL } },
    { "win.stop-all", { "<Alt>t", NULL } },
    { "win.new-task", { "<Control>n", NULL } },
    { "win.edit-task", { "<Control>e", NULL } },
    { "win.annotate", { "<Control><Shift>a", NULL } },
    { "win.hide-task", { "<Control><Shift>h", NULL } },
    { "win.unhide-tasks", { "<Control><Shift>u", NULL } },
    { "win.delete-task", { "<Control>Delete", NULL } },
    { "win.adjust-time(60)", { "<Control><Shift>i", NULL } },
    { "win.adjust-time(300)", { "<Control>i", NULL } },
    { "win.adjust-time(1800)", { "<Control><Alt>i", NULL } },
    { "win.adjust-time(-60)", { "<Control><Shift>d", NULL } },
    { "win.adjust-time(-300)", { "<Control>d", NULL } },
    { "win.adjust-time(-1800)", { "<Control><Alt>d", NULL } },
    { "win.set-zero", { "<Control><Alt>0", NULL } },
    { "win.cut-time", { "<Control>x", NULL } },
    { "win.copy-time", { "<Control>c", NULL } },
    { "win.paste-time", { "<Control>v", NULL } },
    { "win.quick-entry", { "<Control>space", NULL } },
    { "win.prev-day", { "<Alt>Left", NULL } },
    { "win.next-day", { "<Alt>Right", NULL } },
    { "win.today", { "<Alt>Home", NULL } },
  };

  for (guint i = 0; i < G_N_ELEMENTS (accels); i++) {
    gtk_application_set_accels_for_action (GTK_APPLICATION (app), accels[i].action, accels[i].accels);
  }

  status = g_application_run (G_APPLICATION (app), argc, argv);

  g_object_unref (gtimer_app.timer_service);
  g_object_unref (gtimer_app.task_list_model);
  g_clear_object (&gtimer_app.idle_monitor);
  g_object_unref (gtimer_app.db_manager);
  g_object_unref (app);
  g_free (db_path);
  g_free (gtimer_dir);

  return status;
}
