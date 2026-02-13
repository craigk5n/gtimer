#include "gtimer-window.h"
#include "report-window.h"
#include "../core/db-manager.h"
#include "../core/task-list-model.h"
#include "../core/timer-service.h"
#include "../core/idle-monitor.h"
#include "../core/timer-utils.h"
#include "../core/report-generator.h"
#include <glib/gi18n.h>
#include <locale.h>
#include <stdlib.h>

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
  gboolean verbose;
  gboolean quiet;
} GTimerCLIOptions;

static void
print_status(GTimerDBManager *db)
{
  GList *tasks = gtimer_db_manager_get_all_tasks(db);
  GList *l;
  int running_count = 0;

  if (tasks) {
    for (l = tasks; l != NULL; l = g_list_next(l)) {
      GTimerTask *task = GTIMER_TASK(l->data);
      if (!task)
        continue;
      if (gtimer_task_is_timing(task)) {
        running_count++;
        g_print("[%d] %s", gtimer_task_get_id(task), gtimer_task_get_name(task));
        if (gtimer_task_get_project_name(task)) {
          g_print(" (%s)", gtimer_task_get_project_name(task));
        }
        g_print("\n");
      }
    }
    g_list_free_full(tasks, g_object_unref);
  }

  if (running_count == 0) {
    g_print("No tasks are currently timing.\n");
  }
}

static void
list_all_tasks(GTimerDBManager *db)
{
  GList *tasks = gtimer_db_manager_get_all_tasks(db);
  GList *l;

  g_print("ID   Project                     Task Name\n");
  g_print("---- --------------------------- --------------------------------\n");

  if (tasks) {
    for (l = tasks; l != NULL; l = g_list_next(l)) {
      GTimerTask *task = GTIMER_TASK(l->data);
      if (!task)
        continue;
      const char *project = gtimer_task_get_project_name(task) ? gtimer_task_get_project_name(task) : "-";
      g_print("%-4d %-27s %s\n",
              gtimer_task_get_id(task),
              project,
              gtimer_task_get_name(task));
    }
    g_list_free_full(tasks, g_object_unref);
  }

  g_print("\n%d tasks total.\n", tasks ? g_list_length(tasks) : 0);
}

static void
handle_cli_options(GTimerCLIOptions *opts, GTimerDBManager *db)
{
  if (opts->show_version) {
    g_print("GTimer %s\n", VERSION);
    return;
  }

  if (opts->list_tasks) {
    list_all_tasks(db);
    return;
  }

  if (opts->show_status) {
    print_status(db);
    return;
  }

  if (opts->stop_all_timers) {
    GList *tasks = gtimer_db_manager_get_all_tasks(db);
    GList *l;
    int count = 0;

    for (l = tasks; l != NULL; l = l->next) {
      GTimerTask *task = GTIMER_TASK(l->data);
      if (gtimer_task_is_timing(task)) {
        gtimer_db_manager_stop_task_timing(db, gtimer_task_get_id(task));
        count++;
        g_print("Stopped: %s\n", gtimer_task_get_name(task));
      }
    }
    g_list_free_full(tasks, g_object_unref);
    g_print("Stopped %d task(s).\n", count);
    return;
  }

  if (opts->stop_timer) {
    GList *tasks = gtimer_db_manager_get_all_tasks(db);
    GList *l;
    int count = 0;
    char *task_name = NULL;

    for (l = tasks; l != NULL; l = l->next) {
      GTimerTask *task = GTIMER_TASK(l->data);
      if (gtimer_task_is_timing(task)) {
        task_name = g_strdup(gtimer_task_get_name(task));
        gtimer_db_manager_stop_task_timing(db, gtimer_task_get_id(task));
        count++;
        g_print("Stopped: %s\n", task_name);
        g_free(task_name);
      }
    }
    g_list_free_full(tasks, g_object_unref);
    if (count == 0) {
      g_print("No task is currently timing.\n");
    }
    return;
  }

  if (opts->task_id > 0) {
    GList *tasks = gtimer_db_manager_get_all_tasks(db);
    GList *l;
    GTimerTask *found = NULL;

    for (l = tasks; l != NULL; l = l->next) {
      GTimerTask *task = GTIMER_TASK(l->data);
      if (gtimer_task_get_id(task) == opts->task_id) {
        found = task;
        break;
      }
    }
    g_list_free_full(tasks, g_object_unref);

    if (!found) {
      g_printerr("Error: Task with ID %d not found\n", opts->task_id);
      return;
    }

    gtimer_db_manager_start_task_timing(db, opts->task_id);
    g_print("Started timing: %s (ID: %d)\n", gtimer_task_get_name(found), opts->task_id);
    g_object_unref(found);
    return;
  }

  if (opts->add_task) {
    GError *error = NULL;
    gtimer_db_manager_create_task(db, opts->add_task, opts->add_task_project > 0 ? opts->add_task_project - 1 : -1, &error);
    if (error) {
      g_printerr("Error creating task: %s\n", error->message);
      g_error_free(error);
      return;
    }
    g_print("Created task: %s\n", opts->add_task);
    return;
  }

  if (opts->report_type) {
    GDateTime *now = g_date_time_new_now_local();
    GDateTime *start_date = g_date_time_ref(now);
    GDateTime *end_date = g_date_time_ref(now);

    if (g_str_equal(opts->report_type, "weekly") || g_str_equal(opts->report_type, "w")) {
      start_date = g_date_time_add_days(now, -7);
    } else if (g_str_equal(opts->report_type, "monthly") || g_str_equal(opts->report_type, "m")) {
      start_date = g_date_time_add_months(now, -1);
    } else if (g_str_equal(opts->report_type, "yearly") || g_str_equal(opts->report_type, "y")) {
      start_date = g_date_time_add_years(now, -1);
    } else if (!g_str_equal(opts->report_type, "daily") && !g_str_equal(opts->report_type, "d")) {
      g_printerr("Error: Unknown report type '%s'. Use daily, weekly, or monthly\n", opts->report_type);
      g_date_time_unref(now);
      g_date_time_unref(start_date);
      g_date_time_unref(end_date);
      return;
    }

    char *report = gtimer_report_generate(db, GTIMER_REPORT_DAILY, GTIMER_REPORT_TEXT,
                                          start_date, end_date, NULL, 0);

    if (opts->report_file) {
      if (!g_file_set_contents(opts->report_file, report, -1, NULL)) {
        g_printerr("Error: Failed to write report to %s\n", opts->report_file);
      } else {
        g_print("Report saved to: %s\n", opts->report_file);
      }
    } else {
      g_print("%s\n", report);
    }

    g_free(report);
    g_date_time_unref(now);
    g_date_time_unref(start_date);
    g_date_time_unref(end_date);
    return;
  }

  if (opts->export_csv) {
    GList *tasks = gtimer_db_manager_get_all_tasks(db);
    GList *l;
    GString *csv = g_string_new("task_id,task_name,project,is_timing,is_hidden,total_seconds,today_seconds\n");

    for (l = tasks; l != NULL; l = l->next) {
      GTimerTask *task = GTIMER_TASK(l->data);
      const char *project = gtimer_task_get_project_name(task) ? gtimer_task_get_project_name(task) : "";
      g_string_append_printf(csv, "%d,\"%s\",\"%s\",%d,%d,%ld,%ld\n",
                            gtimer_task_get_id(task),
                            gtimer_task_get_name(task),
                            project,
                            gtimer_task_is_timing(task) ? 1 : 0,
                            gtimer_task_is_hidden(task) ? 1 : 0,
                            gtimer_task_get_total_time(task),
                            gtimer_task_get_today_time(task));
    }
    g_list_free_full(tasks, g_object_unref);

    if (!g_file_set_contents(opts->export_csv, csv->str, -1, NULL)) {
      g_printerr("Error: Failed to write CSV to %s\n", opts->export_csv);
    } else {
      g_print("Exported %d tasks to: %s\n", g_list_length(tasks), opts->export_csv);
    }
    g_string_free(csv, TRUE);
    return;
  }

  if (opts->import_csv) {
    g_printerr("Error: CSV import not yet implemented\n");
    return;
  }
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
  GTimerApp *gtimer_app = g_object_get_data (G_OBJECT (app), "gtimer-app");
  
  // Stop all timers
  GListModel *model = gtimer_task_list_model_get_model (gtimer_app->task_list_model);
  guint n_items = g_list_model_get_n_items (model);
  for (guint i = 0; i < n_items; i++) {
    GTimerTask *task = GTIMER_TASK (g_list_model_get_item (model, i));
    if (gtimer_task_is_timing (task)) {
      gtimer_db_manager_stop_task_timing (gtimer_app->db_manager, gtimer_task_get_id (task));
    }
    g_object_unref (task);
  }
  
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
    { "verbose", 0, 0, G_OPTION_ARG_NONE, &cli_opts.verbose, "Enable verbose output", NULL },
    { "quiet", 'q', 0, G_OPTION_ARG_NONE, &cli_opts.quiet, "Suppress non-essential output", NULL },
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
  if (cli_opts.datadir) {
    data_dir = cli_opts.datadir;
  } else {
    data_dir = g_get_user_data_dir();
  }

  char *gtimer_dir = g_build_filename(data_dir, "gtimer", NULL);
  g_mkdir_with_parents(gtimer_dir, 0755);
  char *db_path = g_build_filename(gtimer_dir, "gtimer.db", NULL);

  gtimer_app.db_manager = gtimer_db_manager_new(db_path, &error);
  if (!gtimer_app.db_manager) {
    g_printerr("Failed to initialize database: %s\n", error->message);
    return 1;
  }

  if (cli_opts.show_version || cli_opts.show_status || cli_opts.list_tasks ||
      cli_opts.stop_timer || cli_opts.stop_all_timers || cli_opts.task_id > 0 ||
      cli_opts.add_task || cli_opts.report_type || cli_opts.export_csv || cli_opts.import_csv) {
    if (!cli_opts.quiet) {
      g_print("GTimer %s\n", VERSION);
    }
    handle_cli_options(&cli_opts, gtimer_app.db_manager);
    g_object_unref(gtimer_app.db_manager);
    g_free(db_path);
    g_free(gtimer_dir);
    return 0;
  }

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
