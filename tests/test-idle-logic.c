#include <glib.h>
#include "../core/timer-service.h"
#include "../core/db-manager.h"
#include "../core/task-object.h"

/* Extern helpers from mock-idle-monitor.c */
void mock_idle_monitor_trigger_idle (GTimerIdleMonitor *self);
void mock_idle_monitor_trigger_resume (GTimerIdleMonitor *self);

static void
test_idle_detection (void)
{
  GTimerDBManager *db_manager;
  GTimerTimerService *service;
  GTimerTask *task;
  GTimerIdleMonitor *monitor;

  db_manager = gtimer_db_manager_new (":memory:", NULL);
  service = gtimer_timer_service_new (db_manager);
  
  /* Must use the mocked monitor */
  monitor = gtimer_idle_monitor_new ();
  gtimer_timer_service_set_idle_monitor (service, monitor);
  
  /* Create and start a task */
  sqlite3 *db = gtimer_db_manager_get_db (db_manager);
  sqlite3_exec (db, "INSERT INTO tasks (id, name, is_timing) VALUES (1, 'Idle Test Task', 0);", NULL, NULL, NULL);
  task = gtimer_task_new (1, "Idle Test Task", -1, NULL, 0, 0, FALSE, FALSE, 0, 0);
  
  gtimer_timer_service_start (service, task);
  g_assert_true (gtimer_task_is_timing (task));
  g_assert_false (gtimer_timer_service_is_paused (service));

  /* Trigger Idle */
  mock_idle_monitor_trigger_idle (monitor);
  g_assert_true (gtimer_timer_service_is_paused (service));
  
  /* Trigger Resume */
  mock_idle_monitor_trigger_resume (monitor);
  g_assert_false (gtimer_timer_service_is_paused (service));
  
  g_object_unref (task);
  g_object_unref (monitor);
  g_object_unref (service);
  g_object_unref (db_manager);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/idle/detection", test_idle_detection);
  return g_test_run ();
}
