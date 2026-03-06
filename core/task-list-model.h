#ifndef GTIMER_TASK_LIST_MODEL_H
#define GTIMER_TASK_LIST_MODEL_H

#include <gio/gio.h>
#include "db-manager.h"

#define GTIMER_TYPE_TASK_LIST_MODEL (gtimer_task_list_model_get_type ())
G_DECLARE_FINAL_TYPE (GTimerTaskListModel, gtimer_task_list_model, GTIMER, TASK_LIST_MODEL, GObject)

GTimerTaskListModel *gtimer_task_list_model_new (GTimerDBManager *db_manager);
GListModel *gtimer_task_list_model_get_model (GTimerTaskListModel *self);
void gtimer_task_list_model_refresh (GTimerTaskListModel *self);
void gtimer_task_list_model_set_view_date (GTimerTaskListModel *self, const char *date);
const char *gtimer_task_list_model_get_view_date (GTimerTaskListModel *self);

#endif
