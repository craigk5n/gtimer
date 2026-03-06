#ifndef GTIMER_DB_MANAGER_H
#define GTIMER_DB_MANAGER_H

#include <sqlite3.h>
#include <glib.h>
#include <glib-object.h>

#define GTIMER_TYPE_DB_MANAGER (gtimer_db_manager_get_type())
#define GTIMER_DB_MANAGER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GTIMER_TYPE_DB_MANAGER, GTimerDBManager))
#define GTIMER_IS_DB_MANAGER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTIMER_TYPE_DB_MANAGER))

#define GTIMER_DB_ERROR (gtimer_db_error_quark())
#define GTIMER_DB_ERROR_SQL 1

GQuark gtimer_db_error_quark(void);

typedef struct _GTimerDBManager GTimerDBManager;
typedef struct _GTimerDBManagerClass GTimerDBManagerClass;

struct _GTimerDBManager {
	GObject parent_instance;
	sqlite3 *db;
};

struct _GTimerDBManagerClass {
	GObjectClass parent_class;
};

GTimerDBManager *gtimer_db_manager_new(const char *path, GError **error);

sqlite3 *gtimer_db_manager_get_db(GTimerDBManager *self);

void gtimer_db_manager_create_task(GTimerDBManager *self, const char *name,
				    int project_id, GError **error);
void gtimer_db_manager_update_task(GTimerDBManager *self, int task_id,
				    const char *name, int project_id,
				    GError **error);
void gtimer_db_manager_delete_task(GTimerDBManager *self, int task_id,
				    GError **error);
void gtimer_db_manager_hide_task(GTimerDBManager *self, int task_id,
				  gboolean hidden, GError **error);
GList *gtimer_db_manager_get_hidden_tasks(GTimerDBManager *self);
GList *gtimer_db_manager_get_all_tasks(GTimerDBManager *self);

void gtimer_db_manager_create_project(GTimerDBManager *self,
				       const char *name, GError **error);
void gtimer_db_manager_update_project(GTimerDBManager *self, int project_id,
				       const char *name, GError **error);
GList *gtimer_db_manager_get_projects(GTimerDBManager *self);

gint64 gtimer_db_manager_get_task_total_time(GTimerDBManager *self,
					      int task_id);
gint64 gtimer_db_manager_get_task_today_time(GTimerDBManager *self,
					      int task_id);
void gtimer_db_manager_add_task_time(GTimerDBManager *self, int task_id,
				      gint64 seconds);
void gtimer_db_manager_add_task_time_for_date(GTimerDBManager *self,
					       int task_id,
					       const char *date_str,
					       gint64 seconds);
void gtimer_db_manager_set_task_today_time(GTimerDBManager *self,
					   int task_id, gint64 seconds);

void gtimer_db_manager_start_task_timing(GTimerDBManager *self,
					  int task_id);
void gtimer_db_manager_stop_task_timing(GTimerDBManager *self, int task_id);
void gtimer_db_manager_flush_task_elapsed(GTimerDBManager *self, int task_id,
					   gint64 start_time, gint64 end_time);
gboolean gtimer_db_manager_is_task_timing(GTimerDBManager *self, int task_id);

typedef struct {
	char *task_name;
	gint64 total_duration;
} GTimerReportRow;

GList *gtimer_db_manager_get_daily_report(GTimerDBManager *self, int year,
					   int month, int day);
void gtimer_report_row_free(GTimerReportRow *row);

typedef struct {
	gint64 id;
	gint64 task_id;
	gint64 created_at;
	char *text;
} GTimerAnnotation;

GList *gtimer_db_manager_get_annotations(GTimerDBManager *self, int task_id);
void gtimer_db_manager_add_annotation(GTimerDBManager *self, int task_id,
				      const char *text);
void gtimer_annotation_free(GTimerAnnotation *annotation);

/* Tags */
void gtimer_db_manager_add_tag_to_task(GTimerDBManager *self, int task_id,
				       const char *tag_name);
void gtimer_db_manager_remove_tag_from_task(GTimerDBManager *self, int task_id,
					    const char *tag_name);
GList *gtimer_db_manager_get_task_tags(GTimerDBManager *self, int task_id);
GList *gtimer_db_manager_get_all_tags(GTimerDBManager *self);
GList *gtimer_db_manager_get_tasks_by_tag(GTimerDBManager *self,
					  const char *tag_name);

#endif
