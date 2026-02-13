#include "timer-utils.h"
#include <stdio.h>

char *gtimer_utils_format_duration ( gint64 seconds )
{
  gint64 h, m, s;

  h = seconds / 3600;
  m = ( seconds % 3600 ) / 60;
  s = seconds % 60;

  return g_strdup_printf ( "%02" G_GINT64_FORMAT ":%02" G_GINT64_FORMAT ":%02"
      G_GINT64_FORMAT, h, m, s );
}

gint64
gtimer_utils_calculate_current_duration ( gint64 start_time,
    gint64 current_time, gint64 paused_duration )
{
  if ( current_time < start_time )
    return 0;

  gint64 diff = current_time - start_time;
  if ( diff < paused_duration )
    return 0;

  return diff - paused_duration;
}
