#include "gtimer-window.h"
#include "../core/task-object.h"
#include "../core/project-object.h"
#include "../core/timer-service.h"
#include "../core/timer-utils.h"
#include "../core/db-manager.h"
#include <time.h>
#include <glib/gi18n.h>

struct _GTimerWindow
{
  AdwApplicationWindow parent_instance;

  AdwHeaderBar *header_bar;
  AdwWindowTitle *window_title;
  AdwToastOverlay *toast_overlay;
  GtkColumnView *column_view;
  GtkLabel *footer_label;
  GTimerTaskListModel *model;
  GTimerTimerService *timer_service;
  GTimerDBManager *db_manager;

  GtkWidget *start_stop_button;
  GtkWidget *stop_all_button;
  GtkWidget *new_task_button;
  GtkWidget *edit_task_button;
  GtkWidget *annotate_button;
  GtkSearchEntry *search_entry;
  GtkStringFilter *string_filter;

  GSettings *settings;
  gint64 time_buffer;
  guint tick_timeout_id;
};

G_DEFINE_TYPE (GTimerWindow, gtimer_window, ADW_TYPE_APPLICATION_WINDOW)

static void
show_toast (GTimerWindow *self, const char *format, ...)
{
  va_list args;
  va_start (args, format);
  char *msg = g_strdup_vprintf (format, args);
  va_end (args);

  adw_toast_overlay_add_toast (self->toast_overlay, adw_toast_new (msg));
  g_free (msg);
}

static void
save_window_state (GTimerWindow *self)
{
  int width, height;
  gtk_window_get_default_size (GTK_WINDOW (self), &width, &height);
  g_settings_set_int (self->settings, "window-width", width);
  g_settings_set_int (self->settings, "window-height", height);
}

static void
on_window_size_notify (GObject *object, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec; (void)user_data;
  save_window_state (GTIMER_WINDOW (object));
}

static void
update_window_title (GTimerWindow *self)
{
  if (!self->model) return;
  GListModel *model = gtimer_task_list_model_get_model (self->model);
  guint n_items = g_list_model_get_n_items (model);
  int running_count = 0;
  char *last_running_name = NULL;
  gint64 today_total = 0;

  for (guint i = 0; i < n_items; i++) {
    GTimerTask *task = GTIMER_TASK (g_list_model_get_item (model, i));
    today_total += gtimer_task_get_today_time (task);
    if (gtimer_task_is_timing (task)) {
      running_count++;
      g_free (last_running_name);
      last_running_name = g_strdup (gtimer_task_get_name (task));
    }
    g_object_unref (task);
  }

  if (running_count == 0) {
    adw_window_title_set_subtitle (self->window_title, "");
    gtk_widget_set_sensitive (self->stop_all_button, FALSE);
  } else if (running_count == 1) {
    adw_window_title_set_subtitle (self->window_title, last_running_name);
    gtk_widget_set_sensitive (self->stop_all_button, TRUE);
  } else {
    char *subtitle = g_strdup_printf ("%d tasks running", running_count);
    adw_window_title_set_subtitle (self->window_title, subtitle);
    g_free (subtitle);
    gtk_widget_set_sensitive (self->stop_all_button, TRUE);
  }
  g_free (last_running_name);

  char *total_str = gtimer_utils_format_duration (today_total);
  char *footer_text = g_strdup_printf ("Today: %s", total_str);
  gtk_label_set_text (self->footer_label, footer_text);
  g_free (footer_text);
  g_free (total_str);
}

static gboolean
on_tick (gpointer user_data)
{
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  gtimer_task_list_model_refresh (self->model);
  update_window_title (self);
  return TRUE;
}

static void
toggle_task_timer (GTimerWindow *self, GTimerTask *task)
{
  int task_id = gtimer_task_get_id (task);
  gboolean is_timing = gtimer_task_is_timing (task);
  
  if (is_timing) {
    gtimer_db_manager_stop_task_timing (self->db_manager, task_id);
  } else {
    gtimer_db_manager_start_task_timing (self->db_manager, task_id);
    if (self->tick_timeout_id == 0) {
      self->tick_timeout_id = g_timeout_add_seconds (1, on_tick, self);
    }
  }
  
  gtimer_task_list_model_refresh (self->model);
  update_window_title (self);
}

static void
on_add_task_response (GtkDialog *dialog, int response_id, gpointer user_data)
{
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  if (response_id == GTK_RESPONSE_OK) {
    GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    GtkWidget *entry = g_object_get_data (G_OBJECT (content_area), "entry");
    GtkDropDown *dropdown = g_object_get_data (G_OBJECT (content_area), "dropdown");
    
    const char *name = gtk_editable_get_text (GTK_EDITABLE (entry));
    int project_id = -1;
    
    guint selected = gtk_drop_down_get_selected (dropdown);
    if (selected != GTK_INVALID_LIST_POSITION) {
      GListModel *id_model = g_object_get_data (G_OBJECT (dropdown), "id-model");
      GObject *item = g_list_model_get_item (id_model, selected);
      if (g_object_get_data (item, "is-none")) {
        project_id = -1;
      } else {
        project_id = GPOINTER_TO_INT (g_object_get_data (item, "project-id"));
      }
      g_object_unref (item);
    }

    if (name && strlen (name) > 0) {
      gtimer_db_manager_create_task (self->db_manager, name, project_id, NULL);
      show_toast (self, "Task '%s' added", name);
      if (self->model) {
        gtimer_task_list_model_refresh (self->model);
        update_window_title (self);
      }
    }
  }
  gtk_window_destroy (GTK_WINDOW (dialog));
}

static void
on_new_task_dialog (GTimerWindow *self)
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons ("New Task", GTK_WINDOW (self),
                                                   GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   "_Add", GTK_RESPONSE_OK, NULL);
  
  GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_box_set_spacing (GTK_BOX (content_area), 12);
  gtk_widget_set_margin_start (content_area, 12);
  gtk_widget_set_margin_end (content_area, 12);
  gtk_widget_set_margin_top (content_area, 12);
  gtk_widget_set_margin_bottom (content_area, 12);

  // Project Dropdown
  GList *projects = gtimer_db_manager_get_projects (self->db_manager);
  GtkStringList *string_list = gtk_string_list_new (NULL);
  GListStore *project_obj_store = g_list_store_new (G_TYPE_OBJECT);
  
  GObject *none_obj = g_object_new (G_TYPE_OBJECT, NULL);
  g_object_set_data (none_obj, "is-none", GINT_TO_POINTER (TRUE));
  gtk_string_list_append (string_list, "None");
  g_list_store_append (project_obj_store, none_obj);
  g_object_unref (none_obj);
  
  for (GList *l = projects; l != NULL; l = l->next) {
    GTimerProject *p = l->data;
    gtk_string_list_append (string_list, gtimer_project_get_name (p));
    GObject *p_obj = g_object_new (G_TYPE_OBJECT, NULL);
    g_object_set_data (p_obj, "project-id", GINT_TO_POINTER (gtimer_project_get_id (p)));
    g_list_store_append (project_obj_store, p_obj);
    g_object_unref (p_obj);
    g_object_unref (p);
  }
  g_list_free (projects);

  GtkDropDown *dropdown = GTK_DROP_DOWN (gtk_drop_down_new (G_LIST_MODEL (string_list), NULL));
  gtk_box_append (GTK_BOX (content_area), gtk_label_new ("Project:"));
  gtk_box_append (GTK_BOX (content_area), GTK_WIDGET (dropdown));
  
  // Task Name Entry
  GtkWidget *entry = gtk_entry_new ();
  gtk_entry_set_placeholder_text (GTK_ENTRY (entry), "Task Name");
  gtk_box_append (GTK_BOX (content_area), gtk_label_new ("Task Name:"));
  gtk_box_append (GTK_BOX (content_area), entry);
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  
  g_object_set_data (G_OBJECT (content_area), "entry", entry);
  g_object_set_data (G_OBJECT (content_area), "dropdown", dropdown);
  g_object_set_data_full (G_OBJECT (dropdown), "id-model", project_obj_store, g_object_unref);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  
  g_signal_connect (dialog, "response", G_CALLBACK (on_add_task_response), self);
  gtk_widget_show (dialog);
}

static void
on_start_stop_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  (void)action; (void)parameter;
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  GtkSelectionModel *selection = gtk_column_view_get_model (self->column_view);
  GObject *item = gtk_single_selection_get_selected_item (GTK_SINGLE_SELECTION (selection));
  
  if (GTIMER_IS_TASK (item)) {
    toggle_task_timer (self, GTIMER_TASK (item));
  }
}

static void
on_stop_all_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  (void)action; (void)parameter;
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  GListModel *model = gtimer_task_list_model_get_model (self->model);
  guint n_items = g_list_model_get_n_items (model);
  
  for (guint i = 0; i < n_items; i++) {
    GTimerTask *task = GTIMER_TASK (g_list_model_get_item (model, i));
    if (gtimer_task_is_timing (task)) {
      gtimer_db_manager_stop_task_timing (self->db_manager, gtimer_task_get_id (task));
    }
    g_object_unref (task);
  }
  gtimer_task_list_model_refresh (self->model);
  update_window_title (self);
}

static void
on_new_task_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  (void)action; (void)parameter;
  on_new_task_dialog (GTIMER_WINDOW (user_data));
}

static void
on_edit_task_response (GtkDialog *dialog, int response_id, gpointer user_data)
{
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  if (response_id == GTK_RESPONSE_OK) {
    GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    GtkWidget *entry = g_object_get_data (G_OBJECT (content_area), "entry");
    GtkDropDown *dropdown = g_object_get_data (G_OBJECT (content_area), "dropdown");
    GTimerTask *task = g_object_get_data (G_OBJECT (content_area), "task");
    
    const char *name = gtk_editable_get_text (GTK_EDITABLE (entry));
    int project_id = -1;
    
    guint selected = gtk_drop_down_get_selected (dropdown);
    if (selected != GTK_INVALID_LIST_POSITION) {
      GListModel *id_model = g_object_get_data (G_OBJECT (dropdown), "id-model");
      GObject *item = g_list_model_get_item (id_model, selected);
      if (g_object_get_data (item, "is-none")) {
        project_id = -1;
      } else {
        project_id = GPOINTER_TO_INT (g_object_get_data (item, "project-id"));
      }
      g_object_unref (item);
    }

    if (name && strlen (name) > 0) {
      gtimer_db_manager_update_task (self->db_manager, gtimer_task_get_id (task), name, project_id, NULL);
      show_toast (self, "Task '%s' updated", name);
      gtimer_task_list_model_refresh (self->model);
      update_window_title (self);
    }
  }
  gtk_window_destroy (GTK_WINDOW (dialog));
}

static void
on_edit_task_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  (void)action; (void)parameter;
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  GtkSelectionModel *selection = gtk_column_view_get_model (self->column_view);
  GObject *item = gtk_single_selection_get_selected_item (GTK_SINGLE_SELECTION (selection));
  
  if (GTIMER_IS_TASK (item)) {
    GTimerTask *task = GTIMER_TASK (item);
    
    GtkWidget *dialog = gtk_dialog_new_with_buttons ("Edit Task", GTK_WINDOW (self),
                                                     GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR,
                                                     "_Cancel", GTK_RESPONSE_CANCEL,
                                                     "_Save", GTK_RESPONSE_OK, NULL);
    
    GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    gtk_box_set_spacing (GTK_BOX (content_area), 12);
    gtk_widget_set_margin_start (content_area, 12);
    gtk_widget_set_margin_end (content_area, 12);
    gtk_widget_set_margin_top (content_area, 12);
    gtk_widget_set_margin_bottom (content_area, 12);

    GList *projects = gtimer_db_manager_get_projects (self->db_manager);
    GtkStringList *string_list = gtk_string_list_new (NULL);
    GListStore *project_obj_store = g_list_store_new (G_TYPE_OBJECT);
    
    GObject *none_obj = g_object_new (G_TYPE_OBJECT, NULL);
    g_object_set_data (none_obj, "is-none", GINT_TO_POINTER (TRUE));
    gtk_string_list_append (string_list, "None");
    g_list_store_append (project_obj_store, none_obj);
    g_object_unref (none_obj);
    
    int selected_idx = 0;
    int i = 1;
    for (GList *l = projects; l != NULL; l = l->next) {
      GTimerProject *p = l->data;
      gtk_string_list_append (string_list, gtimer_project_get_name (p));
      GObject *p_obj = g_object_new (G_TYPE_OBJECT, NULL);
      g_object_set_data (p_obj, "project-id", GINT_TO_POINTER (gtimer_project_get_id (p)));
      g_list_store_append (project_obj_store, p_obj);
      if (gtimer_project_get_id (p) == gtimer_task_get_project_id (task)) selected_idx = i;
      i++;
      g_object_unref (p_obj);
      g_object_unref (p);
    }
    g_list_free (projects);

    GtkDropDown *dropdown = GTK_DROP_DOWN (gtk_drop_down_new (G_LIST_MODEL (string_list), NULL));
    gtk_drop_down_set_selected (dropdown, selected_idx);
    gtk_box_append (GTK_BOX (content_area), gtk_label_new ("Project:"));
    gtk_box_append (GTK_BOX (content_area), GTK_WIDGET (dropdown));
    
    GtkWidget *entry = gtk_entry_new ();
    gtk_editable_set_text (GTK_EDITABLE (entry), gtimer_task_get_name (task));
    gtk_box_append (GTK_BOX (content_area), gtk_label_new ("Task Name:"));
    gtk_box_append (GTK_BOX (content_area), entry);
    gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
    
    g_object_set_data (G_OBJECT (content_area), "entry", entry);
    g_object_set_data (G_OBJECT (content_area), "dropdown", dropdown);
    g_object_set_data_full (G_OBJECT (dropdown), "id-model", project_obj_store, g_object_unref);
    g_object_set_data (G_OBJECT (content_area), "task", task);

    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
    g_signal_connect (dialog, "response", G_CALLBACK (on_edit_task_response), self);
    gtk_widget_show (dialog);
  }
}

static void
on_annotate_response (GtkDialog *dialog, int response_id, gpointer user_data)
{
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  if (response_id == GTK_RESPONSE_OK) {
    GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    GtkTextView *text_view = g_object_get_data (G_OBJECT (content_area), "text-view");
    GTimerTask *task = g_object_get_data (G_OBJECT (content_area), "task");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (text_view);
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds (buffer, &start, &end);
    char *text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
    if (text && strlen (text) > 0) gtimer_db_manager_add_annotation (self->db_manager, gtimer_task_get_id (task), text);
    g_free (text);
  }
  gtk_window_destroy (GTK_WINDOW (dialog));
}

static void
on_annotate_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  (void)action; (void)parameter;
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  GtkSelectionModel *selection = gtk_column_view_get_model (self->column_view);
  GObject *item = gtk_single_selection_get_selected_item (GTK_SINGLE_SELECTION (selection));
  
  if (GTIMER_IS_TASK (item)) {
    GTimerTask *task = GTIMER_TASK (item);
    char *title = g_strdup_printf ("Annotate: %s", gtimer_task_get_name (task));
    GtkWidget *dialog = gtk_dialog_new_with_buttons (title, GTK_WINDOW (self),
                                                     GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR,
                                                     "_Cancel", GTK_RESPONSE_CANCEL,
                                                     "_Save", GTK_RESPONSE_OK, NULL);
    g_free (title);
    GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    gtk_box_set_spacing (GTK_BOX (content_area), 12);
    gtk_widget_set_margin_start (content_area, 12);
    gtk_widget_set_margin_end (content_area, 12);
    gtk_widget_set_margin_top (content_area, 12);
    gtk_widget_set_margin_bottom (content_area, 12);
    GtkWidget *scrolled = gtk_scrolled_window_new ();
    gtk_widget_set_size_request (scrolled, 300, 200);
    gtk_box_append (GTK_BOX (content_area), scrolled);
    GtkWidget *text_view = gtk_text_view_new ();
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD_CHAR);
    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled), text_view);
    g_object_set_data (G_OBJECT (content_area), "text-view", text_view);
    g_object_set_data (G_OBJECT (content_area), "task", task);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
    g_signal_connect (dialog, "response", G_CALLBACK (on_annotate_response), self);
    gtk_widget_show (dialog);
  }
}

static void
on_adjust_time_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  (void)action;
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  gint32 seconds = g_variant_get_int32 (parameter);
  GtkSelectionModel *selection = gtk_column_view_get_model (self->column_view);
  GObject *item = gtk_single_selection_get_selected_item (GTK_SINGLE_SELECTION (selection));
  if (GTIMER_IS_TASK (item)) {
    gtimer_db_manager_add_task_time (self->db_manager, gtimer_task_get_id (GTIMER_TASK (item)), seconds);
    gtimer_task_list_model_refresh (self->model);
    update_window_title (self);
  }
}

static void
on_delete_task_response (GtkDialog *dialog, int response_id, gpointer user_data)
{
  if (response_id == GTK_RESPONSE_YES) {
    GTimerWindow *self = GTIMER_WINDOW (user_data);
    GtkSelectionModel *selection = gtk_column_view_get_model (self->column_view);
    GObject *item = gtk_single_selection_get_selected_item (GTK_SINGLE_SELECTION (selection));
    if (GTIMER_IS_TASK (item)) {
      gtimer_db_manager_delete_task (self->db_manager, gtimer_task_get_id (GTIMER_TASK (item)), NULL);
      show_toast (self, "Task deleted");
      gtimer_task_list_model_refresh (self->model);
      update_window_title (self);
    }
  }
  gtk_window_destroy (GTK_WINDOW (dialog));
}

static void
on_delete_task_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  (void)action; (void)parameter;
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  GtkSelectionModel *selection = gtk_column_view_get_model (self->column_view);
  GObject *item = gtk_single_selection_get_selected_item (GTK_SINGLE_SELECTION (selection));
  if (GTIMER_IS_TASK (item)) {
    GTimerTask *task = GTIMER_TASK (item);
    char *text = g_strdup_printf ("This will permanently delete '%s' and all its time entries and annotations.", gtimer_task_get_name (task));
    GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (self),
                                                GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_USE_HEADER_BAR,
                                                GTK_MESSAGE_QUESTION,
                                                GTK_BUTTONS_YES_NO,
                                                "Delete Task?");
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", text);
    g_free (text);
    g_signal_connect (dialog, "response", G_CALLBACK (on_delete_task_response), self);
    gtk_widget_show (dialog);
  }
}

static void
on_hide_task_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  (void)action; (void)parameter;
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  GtkSelectionModel *selection = gtk_column_view_get_model (self->column_view);
  GObject *item = gtk_single_selection_get_selected_item (GTK_SINGLE_SELECTION (selection));
  if (GTIMER_IS_TASK (item)) {
    gtimer_db_manager_hide_task (self->db_manager, gtimer_task_get_id (GTIMER_TASK (item)), TRUE, NULL);
    show_toast (self, "Task hidden");
    gtimer_task_list_model_refresh (self->model);
    update_window_title (self);
  }
}

static void
on_unhide_response (GtkDialog *dialog, int response_id, gpointer user_data)
{
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  if (response_id == GTK_RESPONSE_OK) {
    GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    GtkListBox *list_box = g_object_get_data (G_OBJECT (content_area), "list-box");
    GtkWidget *row = gtk_widget_get_first_child (GTK_WIDGET (list_box));
    while (row) {
      GtkWidget *check = g_object_get_data (G_OBJECT (row), "check");
      if (gtk_check_button_get_active (GTK_CHECK_BUTTON (check))) {
        int task_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (row), "task-id"));
        gtimer_db_manager_hide_task (self->db_manager, task_id, FALSE, NULL);
      }
      row = gtk_widget_get_next_sibling (row);
    }
    show_toast (self, "Tasks unhidden");
    gtimer_task_list_model_refresh (self->model);
    update_window_title (self);
  }
  gtk_window_destroy (GTK_WINDOW (dialog));
}

static void
on_unhide_tasks_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  (void)action; (void)parameter;
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  GList *hidden_tasks = gtimer_db_manager_get_hidden_tasks (self->db_manager);
  if (!hidden_tasks) {
    adw_toast_overlay_add_toast (self->toast_overlay, adw_toast_new ("No hidden tasks."));
    return;
  }
  GtkWidget *dialog = gtk_dialog_new_with_buttons ("Unhide Tasks", GTK_WINDOW (self),
                                                   GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   "_Unhide", GTK_RESPONSE_OK, NULL);
  GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_box_set_spacing (GTK_BOX (content_area), 12);
  gtk_widget_set_margin_start (content_area, 12); gtk_widget_set_margin_end (content_area, 12);
  gtk_widget_set_margin_top (content_area, 12); gtk_widget_set_margin_bottom (content_area, 12);
  GtkWidget *scrolled = gtk_scrolled_window_new ();
  gtk_widget_set_size_request (scrolled, 300, 300);
  gtk_box_append (GTK_BOX (content_area), scrolled);
  GtkWidget *list_box = gtk_list_box_new ();
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (list_box), GTK_SELECTION_NONE);
  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled), list_box);
  for (GList *l = hidden_tasks; l != NULL; l = l->next) {
    GTimerTask *task = l->data;
    GtkWidget *row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *check = gtk_check_button_new ();
    gtk_check_button_set_active (GTK_CHECK_BUTTON (check), TRUE);
    gtk_box_append (GTK_BOX (row), check);
    const char *project_name = gtimer_task_get_project_name (task);
    char *label_text = project_name ? g_strdup_printf ("[%s] %s", project_name, gtimer_task_get_name (task)) : g_strdup (gtimer_task_get_name (task));
    gtk_box_append (GTK_BOX (row), gtk_label_new (label_text));
    g_free (label_text);
    gtk_list_box_append (GTK_LIST_BOX (list_box), row);
    g_object_set_data (G_OBJECT (row), "check", check);
    g_object_set_data (G_OBJECT (row), "task-id", GINT_TO_POINTER (gtimer_task_get_id (task)));
    g_object_unref (task);
  }
  g_list_free (hidden_tasks);
  g_object_set_data (G_OBJECT (content_area), "list-box", list_box);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  g_signal_connect (dialog, "response", G_CALLBACK (on_unhide_response), self);
  gtk_widget_show (dialog);
}

static void
on_set_zero_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  (void)action; (void)parameter;
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  GtkSelectionModel *selection = gtk_column_view_get_model (self->column_view);
  GObject *item = gtk_single_selection_get_selected_item (GTK_SINGLE_SELECTION (selection));
  if (GTIMER_IS_TASK (item)) {
    GTimerTask *task = GTIMER_TASK (item);
    self->time_buffer = gtimer_task_get_today_time (task);
    gtimer_db_manager_set_task_today_time (self->db_manager, gtimer_task_get_id (task), 0);
    gtimer_task_list_model_refresh (self->model);
    char *msg = g_strdup_printf ("Cut/Paste buffer: %s", gtimer_utils_format_duration (self->time_buffer));
    adw_toast_overlay_add_toast (self->toast_overlay, adw_toast_new (msg));
    g_free (msg);
    update_window_title (self);
  }
}

static void
on_copy_time_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  (void)action; (void)parameter;
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  GtkSelectionModel *selection = gtk_column_view_get_model (self->column_view);
  GObject *item = gtk_single_selection_get_selected_item (GTK_SINGLE_SELECTION (selection));
  if (GTIMER_IS_TASK (item)) {
    GTimerTask *task = GTIMER_TASK (item);
    self->time_buffer = gtimer_task_get_today_time (task);
    char *duration = gtimer_utils_format_duration (self->time_buffer);
    show_toast (self, "Copied %s to buffer", duration);
    g_free (duration);
  }
}

static void
on_paste_time_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  (void)action; (void)parameter;
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  GtkSelectionModel *selection = gtk_column_view_get_model (self->column_view);
  GObject *item = gtk_single_selection_get_selected_item (GTK_SINGLE_SELECTION (selection));
  if (GTIMER_IS_TASK (item)) {
    gtimer_db_manager_add_task_time (self->db_manager, gtimer_task_get_id (GTIMER_TASK (item)), self->time_buffer);
    char *duration = gtimer_utils_format_duration (self->time_buffer);
    show_toast (self, "Pasted %s from buffer", duration);
    g_free (duration);
    gtimer_task_list_model_refresh (self->model);
    update_window_title (self);
  }
}

static void on_clear_buffer_action (GSimpleAction *action, GVariant *parameter, gpointer user_data) { 
  (void)action; (void)parameter; 
  GTimerWindow *self = GTIMER_WINDOW (user_data); 
  self->time_buffer = 0; 
  show_toast (self, "Time buffer cleared");
}

static void
on_new_project_response (GtkDialog *dialog, int response_id, gpointer user_data)
{
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  if (response_id == GTK_RESPONSE_OK) {
    GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    GtkWidget *entry = g_object_get_data (G_OBJECT (content_area), "entry");
    const char *name = gtk_editable_get_text (GTK_EDITABLE (entry));
    if (name && strlen (name) > 0) {
      gtimer_db_manager_create_project (self->db_manager, name, NULL);
      show_toast (self, "Project '%s' added", name);
    }
  }
  gtk_window_destroy (GTK_WINDOW (dialog));
}

static void
on_new_project_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  (void)action; (void)parameter;
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  GtkWidget *dialog = gtk_dialog_new_with_buttons ("New Project", GTK_WINDOW (self), GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR, "_Cancel", GTK_RESPONSE_CANCEL, "_Add", GTK_RESPONSE_OK, NULL);
  GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  GtkWidget *entry = gtk_entry_new ();
  gtk_entry_set_placeholder_text (GTK_ENTRY (entry), "Project Name");
  gtk_widget_set_margin_start (entry, 12); gtk_widget_set_margin_end (entry, 12); gtk_widget_set_margin_top (entry, 12); gtk_widget_set_margin_bottom (entry, 12);
  gtk_box_append (GTK_BOX (content_area), entry);
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  g_object_set_data (G_OBJECT (content_area), "entry", entry);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  g_signal_connect (dialog, "response", G_CALLBACK (on_new_project_response), self);
  gtk_widget_show (dialog);
}

static void
on_edit_project_response (GtkDialog *dialog, int response_id, gpointer user_data)
{
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  if (response_id == GTK_RESPONSE_OK) {
    GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    GtkWidget *entry = g_object_get_data (G_OBJECT (content_area), "entry");
    GtkDropDown *dropdown = g_object_get_data (G_OBJECT (content_area), "dropdown");
    const char *name = gtk_editable_get_text (GTK_EDITABLE (entry));
    guint selected = gtk_drop_down_get_selected (dropdown);
    if (selected != GTK_INVALID_LIST_POSITION && name && strlen (name) > 0) {
      GListModel *id_model = g_object_get_data (G_OBJECT (dropdown), "id-model");
      GObject *item = g_list_model_get_item (id_model, selected);
      int project_id = GPOINTER_TO_INT (g_object_get_data (item, "project-id"));
      g_object_unref (item);
      gtimer_db_manager_update_project (self->db_manager, project_id, name, NULL);
      show_toast (self, "Project '%s' updated", name);
      gtimer_task_list_model_refresh (self->model);
    }
  }
  gtk_window_destroy (GTK_WINDOW (dialog));
}

static void
on_edit_project_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  (void)action; (void)parameter;
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  GList *projects = gtimer_db_manager_get_projects (self->db_manager);
  if (!projects) { adw_toast_overlay_add_toast (self->toast_overlay, adw_toast_new ("No projects to edit.")); return; }
  GtkWidget *dialog = gtk_dialog_new_with_buttons ("Edit Project", GTK_WINDOW (self), GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR, "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_OK, NULL);
  GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_box_set_spacing (GTK_BOX (content_area), 12);
  gtk_widget_set_margin_start (content_area, 12); gtk_widget_set_margin_end (content_area, 12); gtk_widget_set_margin_top (content_area, 12); gtk_widget_set_margin_bottom (content_area, 12);
  GtkStringList *string_list = gtk_string_list_new (NULL);
  GListStore *project_obj_store = g_list_store_new (G_TYPE_OBJECT);
  for (GList *l = projects; l != NULL; l = l->next) {
    GTimerProject *p = l->data;
    gtk_string_list_append (string_list, gtimer_project_get_name (p));
    GObject *p_obj = g_object_new (G_TYPE_OBJECT, NULL);
    g_object_set_data (p_obj, "project-id", GINT_TO_POINTER (gtimer_project_get_id (p)));
    g_list_store_append (project_obj_store, p_obj);
    g_object_unref (p_obj); g_object_unref (p);
  }
  g_list_free (projects);
  GtkDropDown *dropdown = GTK_DROP_DOWN (gtk_drop_down_new (G_LIST_MODEL (string_list), NULL));
  gtk_box_append (GTK_BOX (content_area), gtk_label_new ("Select Project:"));
  gtk_box_append (GTK_BOX (content_area), GTK_WIDGET (dropdown));
  GtkWidget *entry = gtk_entry_new ();
  gtk_box_append (GTK_BOX (content_area), gtk_label_new ("New Name:"));
  gtk_box_append (GTK_BOX (content_area), entry);
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  g_object_set_data (G_OBJECT (content_area), "entry", entry);
  g_object_set_data (G_OBJECT (content_area), "dropdown", dropdown);
  g_object_set_data_full (G_OBJECT (dropdown), "id-model", project_obj_store, g_object_unref);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  g_signal_connect (dialog, "response", G_CALLBACK (on_edit_project_response), self);
  gtk_widget_show (dialog);
}

static void
on_shortcuts_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  (void)action; (void)parameter;
  GtkWindow *self = GTK_WINDOW (user_data);
  GtkWidget *window = g_object_new (GTK_TYPE_SHORTCUTS_WINDOW,
                                    "transient-for", self,
                                    "modal", TRUE,
                                    NULL);
  gtk_window_present (GTK_WINDOW (window));
}

static void setup_date_label_cb (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data) { (void)factory; (void)user_data; GtkWidget *label = gtk_label_new (NULL); gtk_label_set_xalign (GTK_LABEL (label), 0.5); gtk_widget_set_margin_start (label, 12); gtk_widget_set_margin_end (label, 12); gtk_list_item_set_child (list_item, label); }



static void

on_task_created_at_notify (GObject *object, GParamSpec *pspec, gpointer user_data)

{

  (void)pspec;

  GtkLabel *label = GTK_LABEL (user_data);

  GTimerTask *task = GTIMER_TASK (object);

  GDateTime *dt = g_date_time_new_from_unix_local (gtimer_task_get_created_at (task));

  char *formatted = g_date_time_format (dt, "%x");

  gtk_label_set_text (label, formatted);

  g_free (formatted);

  g_date_time_unref (dt);

}



static void

bind_created_at_cb (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data)

{

  (void)factory; (void)user_data;

  GTimerTask *task = GTIMER_TASK (gtk_list_item_get_item (list_item));

  GtkWidget *label = gtk_list_item_get_child (list_item);

  GDateTime *dt = g_date_time_new_from_unix_local (gtimer_task_get_created_at (task));

  char *formatted = g_date_time_format (dt, "%x");

  gtk_label_set_text (GTK_LABEL (label), formatted);

  g_free (formatted);

  g_date_time_unref (dt);

  g_signal_connect_object (task, "notify::created-at", G_CALLBACK (on_task_created_at_notify), label, 0);

}



static void setup_label_cb (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data)

 { (void)factory; (void)user_data; GtkWidget *label = gtk_label_new (NULL); gtk_label_set_xalign (GTK_LABEL (label), 0.0); gtk_widget_set_margin_start (label, 12); gtk_widget_set_margin_end (label, 12); gtk_list_item_set_child (list_item, label); }
static void setup_time_label_cb (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data) { (void)factory; (void)user_data; GtkWidget *label = gtk_label_new (NULL); gtk_label_set_xalign (GTK_LABEL (label), 1.0); gtk_widget_add_css_class (label, "monospace"); gtk_widget_set_margin_start (label, 12); gtk_widget_set_margin_end (label, 12); gtk_list_item_set_child (list_item, label); }
static void setup_project_cb (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data) { 
  (void)factory; (void)user_data; 
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6); 
  gtk_widget_set_margin_start (box, 12); 
  gtk_widget_set_margin_end (box, 12); 
  GtkWidget *spinner = gtk_spinner_new (); 
  gtk_widget_set_visible (spinner, FALSE); 
  gtk_box_append (GTK_BOX (box), spinner); 
  GtkWidget *label = gtk_label_new (NULL); 
  gtk_label_set_xalign (GTK_LABEL (label), 0.0); 
  gtk_box_append (GTK_BOX (box), label); 
  gtk_list_item_set_child (list_item, box); 
}

static void
on_task_project_name_notify (GObject *object, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec;
  GtkLabel *label = GTK_LABEL (user_data);
  GTimerTask *task = GTIMER_TASK (object);
  gtk_label_set_text (label, gtimer_task_get_project_name (task));
}

static void
on_task_is_timing_notify (GObject *object, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec;
  GtkWidget *box = GTK_WIDGET (user_data);
  GtkSpinner *spinner = GTK_SPINNER (gtk_widget_get_first_child (box));
  GTimerTask *task = GTIMER_TASK (object);
  if (gtimer_task_is_timing (task)) {
    gtk_widget_set_visible (GTK_WIDGET (spinner), TRUE);
    gtk_spinner_start (spinner);
    gtk_widget_add_css_class (box, "timing");
  } else {
    gtk_spinner_stop (spinner);
    gtk_widget_set_visible (GTK_WIDGET (spinner), FALSE);
    gtk_widget_remove_css_class (box, "timing");
  }
}

static void
on_task_name_notify (GObject *object, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec;
  GtkLabel *label = GTK_LABEL (user_data);
  GTimerTask *task = GTIMER_TASK (object);
  gtk_label_set_text (label, gtimer_task_get_name (task));
}

static void
on_task_today_time_notify (GObject *object, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec;
  GtkLabel *label = GTK_LABEL (user_data);
  GTimerTask *task = GTIMER_TASK (object);
  char *formatted = gtimer_utils_format_duration (gtimer_task_get_today_time (task));
  gtk_label_set_text (label, formatted);
  g_free (formatted);
}

static void
on_task_total_time_notify (GObject *object, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec;
  GtkLabel *label = GTK_LABEL (user_data);
  GTimerTask *task = GTIMER_TASK (object);
  char *formatted = gtimer_utils_format_duration (gtimer_task_get_total_time (task));
  gtk_label_set_text (label, formatted);
  g_free (formatted);
}

static void bind_project_cb (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data) { 
  (void)factory; (void)user_data; 
  GTimerTask *task = GTIMER_TASK (gtk_list_item_get_item (list_item)); 
  GtkWidget *box = gtk_list_item_get_child (list_item); 
  GtkSpinner *spinner = GTK_SPINNER (gtk_widget_get_first_child (box)); 
  GtkWidget *label = gtk_widget_get_next_sibling (GTK_WIDGET (spinner)); 
  const char *project_name = gtimer_task_get_project_name (task); 
  gtk_label_set_text (GTK_LABEL (label), project_name ? project_name : ""); 
  if (gtimer_task_is_timing (task)) { 
    gtk_widget_set_visible (GTK_WIDGET (spinner), TRUE); 
    gtk_spinner_start (spinner); 
    gtk_widget_add_css_class (box, "timing"); 
  } else { 
    gtk_spinner_stop (spinner); 
    gtk_widget_set_visible (GTK_WIDGET (spinner), FALSE); 
    gtk_widget_remove_css_class (box, "timing"); 
  } 
  g_signal_connect_object (task, "notify::project-name", G_CALLBACK (on_task_project_name_notify), label, 0); 
  g_signal_connect_object (task, "notify::is-timing", G_CALLBACK (on_task_is_timing_notify), box, 0); 
}
static void bind_task_name_cb (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data) { (void)factory; (void)user_data; GTimerTask *task = GTIMER_TASK (gtk_list_item_get_item (list_item)); GtkWidget *label = gtk_list_item_get_child (list_item); gtk_label_set_text (GTK_LABEL (label), gtimer_task_get_name (task)); g_signal_connect_object (task, "notify::name", G_CALLBACK (on_task_name_notify), label, 0); }
static void bind_today_time_cb (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data) { (void)factory; (void)user_data; GTimerTask *task = GTIMER_TASK (gtk_list_item_get_item (list_item)); GtkWidget *label = gtk_list_item_get_child (list_item); char *formatted = gtimer_utils_format_duration (gtimer_task_get_today_time (task)); gtk_label_set_text (GTK_LABEL (label), formatted); g_free (formatted); g_signal_connect_object (task, "notify::today-time", G_CALLBACK (on_task_today_time_notify), label, 0); }
static void bind_total_time_cb (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data) { (void)factory; (void)user_data; GTimerTask *task = GTIMER_TASK (gtk_list_item_get_item (list_item)); GtkWidget *label = gtk_list_item_get_child (list_item); char *formatted = gtimer_utils_format_duration (gtimer_task_get_total_time (task)); gtk_label_set_text (GTK_LABEL (label), formatted); g_free (formatted); g_signal_connect_object (task, "notify::total-time", G_CALLBACK (on_task_total_time_notify), label, 0); }

static void

on_right_click_cb (GtkGestureClick *gesture,

                   int              n_press,

                   double           x,

                   double           y,

                   gpointer         user_data)

{

  (void)gesture; (void)n_press;



  GTimerWindow *self = GTIMER_WINDOW (user_data);

  GtkSelectionModel *selection = gtk_column_view_get_model (self->column_view);

  GObject *item = gtk_single_selection_get_selected_item (GTK_SINGLE_SELECTION (selection));



  if (!item) return;



    GMenu *menu = g_menu_new ();



    g_menu_append (menu, _("Start/Stop"), "win.start-stop");



    g_menu_append (menu, _("Edit..."), "win.edit-task");



    g_menu_append (menu, _("Annotate..."), "win.annotate");



    g_menu_append (menu, _("Hide"), "win.hide-task");



    g_menu_append (menu, _("Delete"), "win.delete-task");



  



  GtkWidget *popover = gtk_popover_menu_new_from_model (G_MENU_MODEL (menu));

  gtk_widget_set_parent (popover, GTK_WIDGET (self->column_view));

  

  GdkRectangle rect = { (int)x, (int)y, 1, 1 };

  gtk_popover_set_pointing_to (GTK_POPOVER (popover), &rect);

  gtk_popover_popup (GTK_POPOVER (popover));

  

  g_object_unref (menu);

}



static void on_selection_changed (GtkSelectionModel *selection, guint position, guint n_items, gpointer user_data)

 { (void)position; (void)n_items; GTimerWindow *self = GTIMER_WINDOW (user_data); GObject *item = gtk_single_selection_get_selected_item (GTK_SINGLE_SELECTION (selection)); gboolean has_selection = (item != NULL); gtk_widget_set_sensitive (self->start_stop_button, has_selection); gtk_widget_set_sensitive (self->edit_task_button, has_selection); gtk_widget_set_sensitive (self->annotate_button, has_selection); if (has_selection && GTIMER_IS_TASK (item)) { GTimerTask *task = GTIMER_TASK (item); gtk_button_set_icon_name (GTK_BUTTON (self->start_stop_button), gtimer_task_is_timing (task) ? "media-playback-pause-symbolic" : "media-playback-start-symbolic"); } }
static void on_task_activated (GtkColumnView *column_view, guint position, gpointer user_data) { 
  (void)column_view; 
  GTimerWindow *self = GTIMER_WINDOW (user_data); 
  GtkSelectionModel *selection = gtk_column_view_get_model (column_view);
  GListModel *model = G_LIST_MODEL (selection);
  GTimerTask *task = GTIMER_TASK (g_list_model_get_item (model, position)); 
  int task_id = gtimer_task_get_id (task);

  // Stop all other tasks
  GListModel *base_model = gtimer_task_list_model_get_model (self->model);
  guint n_items = g_list_model_get_n_items (base_model);
  for (guint i = 0; i < n_items; i++) {
    GTimerTask *t = GTIMER_TASK (g_list_model_get_item (base_model, i));
    if (gtimer_task_get_id (t) != task_id && gtimer_task_is_timing (t)) {
      gtimer_db_manager_stop_task_timing (self->db_manager, gtimer_task_get_id (t));
    }
    g_object_unref (t);
  }

  toggle_task_timer (self, task); 
  g_object_unref (task); 
}

static void gtimer_window_dispose (GObject *object) { GTimerWindow *self = GTIMER_WINDOW (object); if (self->tick_timeout_id) { g_source_remove (self->tick_timeout_id); self->tick_timeout_id = 0; } g_clear_object (&self->settings); G_OBJECT_CLASS (gtimer_window_parent_class)->dispose (object); }
static void gtimer_window_class_init (GTimerWindowClass *klass) { GObjectClass *object_class = G_OBJECT_CLASS (klass); object_class->dispose = gtimer_window_dispose; }

static void
setup_actions (GTimerWindow *self)
{
  const GActionEntry entries[] = {
    { .name = "start-stop", .activate = on_start_stop_action },
    { .name = "stop-all", .activate = on_stop_all_action },
    { .name = "new-task", .activate = on_new_task_action },
    { .name = "edit-task", .activate = on_edit_task_action },
    { .name = "annotate", .activate = on_annotate_action },
    { .name = "hide-task", .activate = on_hide_task_action },
    { .name = "unhide-tasks", .activate = on_unhide_tasks_action },
    { .name = "delete-task", .activate = on_delete_task_action },
    { .name = "adjust-time", .activate = on_adjust_time_action, .parameter_type = "i" },
    { .name = "set-zero", .activate = on_set_zero_action },
    { .name = "cut-time", .activate = on_set_zero_action },
    { .name = "copy-time", .activate = on_copy_time_action },
    { .name = "paste-time", .activate = on_paste_time_action },
    { .name = "clear-buffer", .activate = on_clear_buffer_action },
    { .name = "new-project", .activate = on_new_project_action },
    { .name = "edit-project", .activate = on_edit_project_action },
    { .name = "shortcuts", .activate = on_shortcuts_action },
  };
  g_action_map_add_action_entries (G_ACTION_MAP (self), entries, G_N_ELEMENTS (entries), self);
}

static void
setup_header_bar (GTimerWindow *self, GtkWidget *main_box)
{
  self->header_bar = ADW_HEADER_BAR (adw_header_bar_new ());
  gtk_box_append (GTK_BOX (main_box), GTK_WIDGET (self->header_bar));
  self->window_title = ADW_WINDOW_TITLE (adw_window_title_new ("GTimer", ""));
  adw_header_bar_set_title_widget (self->header_bar, GTK_WIDGET (self->window_title));

  GtkWidget *header_start_stop = gtk_button_new_from_icon_name ("media-playback-start-symbolic");
  gtk_actionable_set_action_name (GTK_ACTIONABLE (header_start_stop), "win.start-stop");
  gtk_widget_set_tooltip_text (header_start_stop, "Start/Stop Timing");
  adw_header_bar_pack_start (self->header_bar, header_start_stop);

  self->search_entry = GTK_SEARCH_ENTRY (gtk_search_entry_new ());
  gtk_widget_set_hexpand (GTK_WIDGET (self->search_entry), TRUE);
  gtk_widget_set_halign (GTK_WIDGET (self->search_entry), GTK_ALIGN_CENTER);
  gtk_widget_set_size_request (GTK_WIDGET (self->search_entry), 250, -1);
  adw_header_bar_set_title_widget (self->header_bar, GTK_WIDGET (self->search_entry));

  self->new_task_button = gtk_button_new_from_icon_name ("list-add-symbolic");
  gtk_actionable_set_action_name (GTK_ACTIONABLE (self->new_task_button), "win.new-task");
  gtk_widget_set_tooltip_text (self->new_task_button, "New Task");
  adw_header_bar_pack_end (self->header_bar, self->new_task_button);
}

static void
setup_toolbar (GTimerWindow *self, GtkWidget *main_box)
{
  GtkWidget *action_bar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_add_css_class (action_bar, "toolbar");
  gtk_widget_set_margin_start (action_bar, 6);
  gtk_widget_set_margin_end (action_bar, 6);
  gtk_widget_set_margin_top (action_bar, 3);
  gtk_widget_set_margin_bottom (action_bar, 3);
  gtk_box_append (GTK_BOX (main_box), action_bar);

  struct { const char *icon; const char *action; const char *tooltip; } bar_buttons[] = {
    { "media-playback-start-symbolic", "win.start-stop", _("Start/Stop Timing") },
    { "media-playback-stop-symbolic", "win.stop-all", _("Stop All Timers") },
    { "list-add-symbolic", "win.new-task", _("New Task") },
    { "document-edit-symbolic", "win.edit-task", _("Edit Task") },
    { "edit-paste-symbolic", "win.annotate", _("Annotate Task") },
    { "x-office-document-symbolic", "app.report", _("Daily Report") },
  };

  for (guint i = 0; i < G_N_ELEMENTS (bar_buttons); i++) {
    GtkWidget *btn = gtk_button_new_from_icon_name (bar_buttons[i].icon);
    gtk_actionable_set_action_name (GTK_ACTIONABLE (btn), bar_buttons[i].action);
    gtk_widget_set_tooltip_text (btn, bar_buttons[i].tooltip);
    gtk_box_append (GTK_BOX (action_bar), btn);

    if (g_strcmp0 (bar_buttons[i].action, "win.start-stop") == 0) self->start_stop_button = btn;
    if (g_strcmp0 (bar_buttons[i].action, "win.stop-all") == 0) self->stop_all_button = btn;
    if (g_strcmp0 (bar_buttons[i].action, "win.edit-task") == 0) self->edit_task_button = btn;
    if (g_strcmp0 (bar_buttons[i].action, "win.annotate") == 0) self->annotate_button = btn;
  }
}

static void
setup_menus (GTimerWindow *self)
{
  GMenu *menu = g_menu_new ();

  GMenu *task_section = g_menu_new ();
  g_menu_append (task_section, _("Start Timing"), "win.start-stop");
  g_menu_append (task_section, _("Stop All Timing"), "win.stop-all");
  g_menu_append (task_section, _("Edit Task..."), "win.edit-task");
  g_menu_append (task_section, _("Annotate..."), "win.annotate");
  g_menu_append (task_section, _("Hide"), "win.hide-task");
  g_menu_append (task_section, _("Unhide..."), "win.unhide-tasks");
  g_menu_append (task_section, _("Delete"), "win.delete-task");
  g_menu_append_section (menu, _("Task"), G_MENU_MODEL (task_section));

  GMenu *report_section = g_menu_new ();
  g_menu_append (report_section, _("Daily Report..."), "app.report");
  g_menu_append_section (menu, _("Data"), G_MENU_MODEL (report_section));

  GMenu *time_section = g_menu_new ();
  g_menu_append (time_section, _("+1 Minute"), "win.adjust-time(60)");
  g_menu_append (time_section, _("+5 Minutes"), "win.adjust-time(300)");
  g_menu_append (time_section, _("+30 Minutes"), "win.adjust-time(1800)");
  g_menu_append (time_section, _("-1 Minute"), "win.adjust-time(-60)");
  g_menu_append (time_section, _("-5 Minutes"), "win.adjust-time(-300)");
  g_menu_append (time_section, _("-30 Minutes"), "win.adjust-time(-1800)");
  g_menu_append (time_section, _("Set to Zero"), "win.set-zero");
  g_menu_append_section (menu, _("Time Adjustment"), G_MENU_MODEL (time_section));

  GMenu *edit_section = g_menu_new ();
  g_menu_append (edit_section, _("Cut Time"), "win.cut-time");
  g_menu_append (edit_section, _("Copy Time"), "win.copy-time");
  g_menu_append (edit_section, _("Paste Time"), "win.paste-time");
  g_menu_append (edit_section, _("Clear Buffer"), "win.clear-buffer");
  g_menu_append_section (menu, _("Edit"), G_MENU_MODEL (edit_section));

  GMenu *project_section = g_menu_new ();
  g_menu_append (project_section, _("New Project..."), "win.new-project");
  g_menu_append (project_section, _("Edit Project..."), "win.edit-project");
  g_menu_append_section (menu, _("Project"), G_MENU_MODEL (project_section));

  GMenu *app_section = g_menu_new ();
  g_menu_append (app_section, _("Keyboard Shortcuts"), "win.shortcuts");
  g_menu_append (app_section, _("About GTimer"), "app.about");
  g_menu_append_section (menu, NULL, G_MENU_MODEL (app_section));

  GtkMenuButton *menu_button = GTK_MENU_BUTTON (gtk_menu_button_new ());
  gtk_menu_button_set_icon_name (menu_button, "view-more-symbolic");
  gtk_menu_button_set_menu_model (menu_button, G_MENU_MODEL (menu));
  adw_header_bar_pack_end (self->header_bar, GTK_WIDGET (menu_button));
}

static void
setup_column_view (GTimerWindow *self, GtkWidget *main_box)
{
  self->toast_overlay = ADW_TOAST_OVERLAY (adw_toast_overlay_new ());
  gtk_widget_set_vexpand (GTK_WIDGET (self->toast_overlay), TRUE);
  gtk_box_append (GTK_BOX (main_box), GTK_WIDGET (self->toast_overlay));

  GtkWidget *scrolled = gtk_scrolled_window_new ();
  adw_toast_overlay_set_child (self->toast_overlay, scrolled);
  self->column_view = GTK_COLUMN_VIEW (gtk_column_view_new (NULL));
  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled), GTK_WIDGET (self->column_view));
  g_signal_connect (self->column_view, "activate", G_CALLBACK (on_task_activated), self);

  GtkGesture *right_click = gtk_gesture_click_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (right_click), GDK_BUTTON_SECONDARY);
  g_signal_connect (right_click, "pressed", G_CALLBACK (on_right_click_cb), self);
  gtk_widget_add_controller (GTK_WIDGET (self->column_view), GTK_EVENT_CONTROLLER (right_click));

  /* Project column */
  GtkListItemFactory *factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_project_cb), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_project_cb), NULL);
  GtkColumnViewColumn *col = gtk_column_view_column_new (_("Project"), factory);
  gtk_column_view_column_set_fixed_width (col, 180);
  gtk_column_view_column_set_resizable (col, TRUE);
  GtkExpression *prop_expr = gtk_property_expression_new (GTIMER_TYPE_TASK, NULL, "project-name");
  GtkStringSorter *string_sorter = gtk_string_sorter_new (prop_expr);
  gtk_column_view_column_set_sorter (col, GTK_SORTER (string_sorter));
  g_object_unref (string_sorter);
  gtk_column_view_append_column (self->column_view, col);

  /* Task column */
  factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_label_cb), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_task_name_cb), NULL);
  col = gtk_column_view_column_new (_("Task"), factory);
  gtk_column_view_column_set_expand (col, TRUE);
  gtk_column_view_column_set_resizable (col, TRUE);
  prop_expr = gtk_property_expression_new (GTIMER_TYPE_TASK, NULL, "name");
  string_sorter = gtk_string_sorter_new (prop_expr);
  gtk_column_view_column_set_sorter (col, GTK_SORTER (string_sorter));
  g_object_unref (string_sorter);
  gtk_column_view_append_column (self->column_view, col);

  /* Today column */
  factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_time_label_cb), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_today_time_cb), NULL);
  col = gtk_column_view_column_new (_("Today"), factory);
  gtk_column_view_column_set_fixed_width (col, 110);
  gtk_column_view_column_set_resizable (col, TRUE);
  prop_expr = gtk_property_expression_new (GTIMER_TYPE_TASK, NULL, "today-time");
  GtkNumericSorter *num_sorter = gtk_numeric_sorter_new (prop_expr);
  gtk_column_view_column_set_sorter (col, GTK_SORTER (num_sorter));
  g_object_unref (num_sorter);
  gtk_column_view_append_column (self->column_view, col);

  /* Total column */
  factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_time_label_cb), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_total_time_cb), NULL);
  col = gtk_column_view_column_new (_("Total"), factory);
  gtk_column_view_column_set_fixed_width (col, 110);
  gtk_column_view_column_set_resizable (col, TRUE);
  prop_expr = gtk_property_expression_new (GTIMER_TYPE_TASK, NULL, "total-time");
  num_sorter = gtk_numeric_sorter_new (prop_expr);
  gtk_column_view_column_set_sorter (col, GTK_SORTER (num_sorter));
  g_object_unref (num_sorter);
  gtk_column_view_append_column (self->column_view, col);

  /* Created column */
  factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_date_label_cb), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_created_at_cb), NULL);
  col = gtk_column_view_column_new (_("Created"), factory);
  gtk_column_view_column_set_fixed_width (col, 130);
  gtk_column_view_column_set_resizable (col, TRUE);
  prop_expr = gtk_property_expression_new (GTIMER_TYPE_TASK, NULL, "created-at");
  num_sorter = gtk_numeric_sorter_new (prop_expr);
  gtk_column_view_column_set_sorter (col, GTK_SORTER (num_sorter));
  g_object_unref (num_sorter);
  gtk_column_view_append_column (self->column_view, col);
}

static void
setup_footer (GTimerWindow *self, GtkWidget *main_box)
{
  GtkWidget *footer_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_margin_start (footer_box, 12); gtk_widget_set_margin_end (footer_box, 12);
  gtk_widget_set_margin_top (footer_box, 6); gtk_widget_set_margin_bottom (footer_box, 6);
  gtk_box_append (GTK_BOX (main_box), footer_box);
  self->footer_label = GTK_LABEL (gtk_label_new ("Today: 0:00:00"));
  gtk_widget_set_hexpand (GTK_WIDGET (self->footer_label), TRUE);
  gtk_label_set_xalign (self->footer_label, 1.0);
  gtk_box_append (GTK_BOX (footer_box), GTK_WIDGET (self->footer_label));
}

static void
gtimer_window_init (GTimerWindow *self)
{
  GtkWidget *main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  adw_application_window_set_content (ADW_APPLICATION_WINDOW (self), main_box);

  setup_actions (self);
  setup_header_bar (self, main_box);
  setup_toolbar (self, main_box);
  setup_menus (self);
  setup_column_view (self, main_box);
  setup_footer (self, main_box);

  self->tick_timeout_id = g_timeout_add_seconds (1, on_tick, self);
  gtk_window_set_icon_name (GTK_WINDOW (self), "us.k5n.GTimer");

  self->settings = g_settings_new ("us.k5n.GTimer");
  int width = g_settings_get_int (self->settings, "window-width");
  int height = g_settings_get_int (self->settings, "window-height");
  gtk_window_set_default_size (GTK_WINDOW (self), width, height);

  g_signal_connect (self, "notify::default-width", G_CALLBACK (on_window_size_notify), NULL);
  g_signal_connect (self, "notify::default-height", G_CALLBACK (on_window_size_notify), NULL);
}

void
gtimer_window_set_task_list_model (GTimerWindow *self, GTimerTaskListModel *model)
{
  self->model = model;
  
  // Create Multi-Filter for Name OR Project Name
  GtkAnyFilter *any_filter = gtk_any_filter_new ();
  
  GtkExpression *name_expr = gtk_property_expression_new (GTIMER_TYPE_TASK, NULL, "name");
  GtkStringFilter *name_filter = gtk_string_filter_new (name_expr);
  gtk_multi_filter_append (GTK_MULTI_FILTER (any_filter), GTK_FILTER (name_filter));
  
  GtkExpression *proj_expr = gtk_property_expression_new (GTIMER_TYPE_TASK, NULL, "project-name");
  GtkStringFilter *proj_filter = gtk_string_filter_new (proj_expr);
  gtk_multi_filter_append (GTK_MULTI_FILTER (any_filter), GTK_FILTER (proj_filter));

  // Connect search entry to filter strings
  g_object_bind_property (self->search_entry, "text", name_filter, "search", G_BINDING_SYNC_CREATE);
  g_object_bind_property (self->search_entry, "text", proj_filter, "search", G_BINDING_SYNC_CREATE);

  GtkFilterListModel *filter_model = gtk_filter_list_model_new (gtimer_task_list_model_get_model (model), GTK_FILTER (any_filter));
  
  GtkSortListModel *sort_model = gtk_sort_list_model_new (G_LIST_MODEL (filter_model),
                                                         g_object_ref (gtk_column_view_get_sorter (self->column_view)));
  GtkSelectionModel *selection_model = GTK_SELECTION_MODEL (gtk_single_selection_new (G_LIST_MODEL (sort_model)));
  gtk_column_view_set_model (self->column_view, selection_model);
  g_signal_connect (selection_model, "selection-changed", G_CALLBACK (on_selection_changed), self);
  g_object_unref (selection_model);
  update_window_title (self);

  // Resume on Startup
  if (g_settings_get_boolean (self->settings, "resume-on-startup")) {
    GListModel *base_model = gtimer_task_list_model_get_model (model);
    guint n = g_list_model_get_n_items (base_model);
    for (guint i = 0; i < n; i++) {
      GTimerTask *task = GTIMER_TASK (g_list_model_get_item (base_model, i));
      if (gtimer_task_is_timing (task)) {
        gtimer_timer_service_start (self->timer_service, task);
      }
      g_object_unref (task);
    }
  }
}

static void
on_idle_response (GtkDialog *dialog, int response_id, gpointer user_data)
{
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  GTimerTask *active = gtimer_timer_service_get_active_task (self->timer_service);
  gint64 idle_secs = gtimer_timer_service_get_idle_duration (self->timer_service);
  
  if (response_id == 1) { // Revert
    // Discard idle time and stop
    if (active) {
      gtimer_timer_service_remove_time (self->timer_service, active, idle_secs);
      gtimer_timer_service_stop (self->timer_service);
    }
  } else if (response_id == 3) { // Resume
    // Discard idle time but keep timing (resume now)
    if (active) {
      gtimer_timer_service_remove_time (self->timer_service, active, idle_secs);
      gtimer_timer_service_resume (self->timer_service);
    }
  }
  // Continue (response_id == 2) - just keep timing as is (was already paused, resume it)
  else if (response_id == 2) {
    gtimer_timer_service_resume (self->timer_service);
  }
  
  gtimer_task_list_model_refresh (self->model);
  update_window_title (self);
  gtk_window_destroy (GTK_WINDOW (dialog));
}

static void
on_timer_service_paused (GTimerTimerService *service, gpointer user_data)
{
  (void)service; (void)user_data;
  // This could be used to show a persistent notification
}

static void
on_timer_service_resumed (GTimerTimerService *service, gpointer user_data)
{
  (void)service;
  GTimerWindow *self = GTIMER_WINDOW (user_data);
  int threshold = g_settings_get_int (self->settings, "idle-threshold");

  // User returned from idle!
  GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (self),
                                              GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_USE_HEADER_BAR,
                                              GTK_MESSAGE_INFO,
                                              GTK_BUTTONS_NONE,
                                              "Idle Detected");
  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), 
                                            "You have been idle for at least %d minutes.", threshold);
  
  gtk_dialog_add_button (GTK_DIALOG (dialog), "_Revert", 1);
  gtk_dialog_add_button (GTK_DIALOG (dialog), "_Continue", 2);
  gtk_dialog_add_button (GTK_DIALOG (dialog), "_Resume", 3);
  
  g_signal_connect (dialog, "response", G_CALLBACK (on_idle_response), self);
  gtk_widget_show (dialog);
}

void
gtimer_window_set_timer_service (GTimerWindow *self, GTimerTimerService *service)
{
  self->timer_service = service;
  self->db_manager = gtimer_timer_service_get_db_manager (service);
  
  g_signal_connect (service, "paused", G_CALLBACK (on_timer_service_paused), self);
  g_signal_connect (service, "resumed", G_CALLBACK (on_timer_service_resumed), self);
}

GTimerWindow *
gtimer_window_new (GtkApplication *app)
{
  return g_object_new (GTIMER_TYPE_WINDOW, "application", app, NULL);
}