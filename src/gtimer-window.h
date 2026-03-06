#ifndef GTIMER_WINDOW_H
#define GTIMER_WINDOW_H

#include <adwaita.h>
#include "../core/task-list-model.h"
#include "../core/timer-service.h"

#define GTIMER_TYPE_WINDOW (gtimer_window_get_type ())
G_DECLARE_FINAL_TYPE (GTimerWindow, gtimer_window, GTIMER, WINDOW, AdwApplicationWindow)

GTimerWindow *gtimer_window_new (GtkApplication *app);
void gtimer_window_set_task_list_model (GTimerWindow *self, GTimerTaskListModel *model);
void gtimer_window_set_timer_service (GTimerWindow *self, GTimerTimerService *service);
void gtimer_window_check_stale_timers (GTimerWindow *self);

#endif
