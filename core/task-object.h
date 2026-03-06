#ifndef GTIMER_TASK_OBJECT_H
#define GTIMER_TASK_OBJECT_H

#include <glib-object.h>

#define GTIMER_TYPE_TASK (gtimer_task_get_type ())
G_DECLARE_FINAL_TYPE (GTimerTask, gtimer_task, GTIMER, TASK, GObject)

GTimerTask *gtimer_task_new (int id, const char *name, int project_id, const char *project_name, gint64 today_time, gint64 total_time, gboolean is_timing, gboolean is_hidden, gint64 last_start_time, gint64 created_at);

int         gtimer_task_get_id           (GTimerTask *self);
const char *gtimer_task_get_name         (GTimerTask *self);
int         gtimer_task_get_project_id   (GTimerTask *self);
const char *gtimer_task_get_project_name (GTimerTask *self);
gint64      gtimer_task_get_today_time   (GTimerTask *self);
gint64      gtimer_task_get_total_time   (GTimerTask *self);
gboolean    gtimer_task_is_timing      (GTimerTask *self);
gboolean    gtimer_task_is_hidden      (GTimerTask *self);
gint64      gtimer_task_get_last_start_time (GTimerTask *self);
gint64      gtimer_task_get_created_at (GTimerTask *self);
const char *gtimer_task_get_tags       (GTimerTask *self);

void        gtimer_task_set_name       (GTimerTask *self, const char *name);
void        gtimer_task_set_project_id (GTimerTask *self, int project_id);
void        gtimer_task_set_project_name (GTimerTask *self, const char *project_name);
void        gtimer_task_set_today_time (GTimerTask *self, gint64 today_time);
void        gtimer_task_set_total_time (GTimerTask *self, gint64 total_time);
void        gtimer_task_set_is_timing  (GTimerTask *self, gboolean is_timing);
void        gtimer_task_set_is_hidden  (GTimerTask *self, gboolean is_hidden);
void        gtimer_task_set_last_start_time (GTimerTask *self, gint64 last_start_time);
void        gtimer_task_set_created_at (GTimerTask *self, gint64 created_at);
void        gtimer_task_set_tags       (GTimerTask *self, const char *tags);

#endif

