#include "../core/idle-monitor.h"

struct _GTimerIdleMonitor {
  GObject parent_instance;
  guint timeout_id;
};

G_DEFINE_TYPE (GTimerIdleMonitor, gtimer_idle_monitor, G_TYPE_OBJECT)

enum {
  SIGNAL_IDLE,
  SIGNAL_RESUME,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

static void
gtimer_idle_monitor_class_init (GTimerIdleMonitorClass *klass)
{
  signals[SIGNAL_IDLE] = g_signal_new ("idle",
                                       G_TYPE_FROM_CLASS (klass),
                                       G_SIGNAL_RUN_LAST,
                                       0, NULL, NULL, NULL,
                                       G_TYPE_NONE, 0);

  signals[SIGNAL_RESUME] = g_signal_new ("resume",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL, NULL,
                                         G_TYPE_NONE, 0);
}

static void
gtimer_idle_monitor_init (GTimerIdleMonitor *self)
{
  (void)self;
}

GTimerIdleMonitor *
gtimer_idle_monitor_new (void)
{
  return g_object_new (GTIMER_TYPE_IDLE_MONITOR, NULL);
}

void
gtimer_idle_monitor_start (GTimerIdleMonitor *self, guint timeout_seconds)
{
  (void)self;
  (void)timeout_seconds;
}

void
gtimer_idle_monitor_stop (GTimerIdleMonitor *self)
{
  (void)self;
}

gboolean
gtimer_idle_monitor_is_available (GTimerIdleMonitor *self)
{
  (void)self;
  return TRUE;
}

/* Helper for tests to trigger signals manually */
void
mock_idle_monitor_trigger_idle (GTimerIdleMonitor *self)
{
  g_signal_emit (self, signals[SIGNAL_IDLE], 0);
}

void
mock_idle_monitor_trigger_resume (GTimerIdleMonitor *self)
{
  g_signal_emit (self, signals[SIGNAL_RESUME], 0);
}
