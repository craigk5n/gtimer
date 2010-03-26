/*
 * GTimer
 *
 * Copyright:
 *	(C) 1999 Craig Knudsen, cknudsen@cknudsen.com
 *	See accompanying file "COPYING".
 * 
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version 2
 *	of the License, or (at your option) any later version.
 * 
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 * 
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the
 *	Free Software Foundation, Inc., 59 Temple Place,
 *	Suite 330, Boston, MA  02111-1307, USA
 *
 * Description:
 *	Helps you keep track of time spent on different tasks.
 *
 * Author:
 *	Craig Knudsen, cknudsen@cknudsen.com http://www.cknudsen.com/
 *
 * Home Page:
 *	http://www.cknudsen.com/gtimer/
 *
 * History:
 *	09-Mar-2000	Changed release to 1.1.5
 *	09-Mar-2000	Added args to confirm_create_window()
 *	04-May-1999	Changed release to 1.1.4
 *	03-May-1999	Changed release to 1.1.3
 *	17-Mar-1999	Changed release to 1.1.2
 *	24-Feb-1999	Changed release to 1.1.1
 *	02-Feb-1999	Changed release to 1.1.0
 *	04-Jan-1999	Changed release to 0.99B
 *	11-Nov-1998	Added GTIMER_TASK_OPTION_HIDDEN
 *			Changed release to 0.99
 *	10-May-1998	Changed release to 0.98
 *	10-May-1998	Changed release to 0.97
 *	05-Apr-1998	Changed release to 0.96
 *	24-Mar-1998	Changed release to 0.95
 *	18-Mar-1998	Changed release to 0.94
 *	13-Mar-1998	Changed release to 0.93
 *	10-Mar-1998	Changed release to 0.92
 *	10-Mar-1998	Changed release to 0.91
 *	25-Feb-1998	Created
 *
 ****************************************************************************/

#ifndef _GTIMER_H
#define _GTIMER_H

/* Note: we could use "VERSION" produced from autoconf, but I like
   to also have the version date, so I'll just edit this by hand. */
#define GTIMER_VERSION		"2.0.0"
#define GTIMER_VERSION_DATE	"25 Mar 2010" /*"19 Mar 2003"*/
#define GTIMER_URL		"http://www.k5n.us/gtimer.php"
#define GTIMER_COPYRIGHT	"Copyright (C) 2000-2010 Craig Knudsen"

#define GTIMER_STATUS_ID	"status-update"

#define GTIMER_TASK_OPTION_HIDDEN	0x0001

/* URL to check for new version */
#define GTIMER_VERSION_CHECK_SERVER	"www.k5n.us"
#define GTIMER_VERSION_CHECK_PORT	80
#define GTIMER_VERSION_CHECK_PATH	"/gtimer_latest_version.txt"

// PV:
#define MAX_NUMBER_OF_ACTIONS	30


// PV: Internationalization
#include "gtimeri18n.h"

typedef enum {
  CONFIRM_ABOUT,
  CONFIRM_ERROR,
  CONFIRM_WARNING,
  CONFIRM_CONFIRM,
  CONFIRM_MESSAGE
} enum_confirm_type;

typedef enum {
  REPORT_TYPE_NONE,        /* PV: + */
  REPORT_TYPE_DAILY,
  REPORT_TYPE_WEEKLY,
  REPORT_TYPE_MONTHLY,
  REPORT_TYPE_YEARLY,
  REPORT_TYPE_TOTAL
} report_type;

typedef struct {
  Task *task;
  char *project_name;		/* name of parent project */
  TaskTimeEntry *todays_entry;
  int timer_on;
  time_t on_since;
  time_t total;			/* except for today */
  int name_updated;		/* flag to update name on next draw */
  int new_task;			/* flag to add this to the clist */
  char last_total[15];
  time_t last_total_int;
  char last_today[15];
  time_t last_today_int;
  int last_on;			/* was the icon drawn last time? */
  int selected;			/* item is selected */
  int moved;			/* item was moved */
} TaskData;

void save_all ();

void update_list ();

#ifdef __GTK_H__

#ifndef WIN32
void set_x_error_handler ();
#endif

GtkWidget *create_task_edit_window ( TaskData *taskdata );

GtkWidget *create_project_edit_window ( Project *project );

GtkWidget *create_annotate_window ( TaskData *taskdata );

GtkWidget *create_report_window ( report_type type );

GtkWidget *create_unhide_window ( );

void display_changelog ( );

void showMessage ( char *msg );

GtkWidget *create_confirm_window (
  enum_confirm_type type,
  char *title,
  char *text,
  char *ok_text,
  char *cancel_text,
  char *third_text,
  void (*ok_callback)(),
  void (*cancel_callback)(),
  void (*third_callback)(),
  char *callback_data
);

GtkWidget *create_confirm_toplevel (
  enum_confirm_type type,
  char *title,
  char *text,
  char *ok_text,
  char *cancel_text,
  char *third_text,
  void (*ok_callback)(),
  void (*cancel_callback)(),
  void (*third_callback)(),
  char *callback_data
);

char *get_client_path ( char *file );

#endif /* __GTK_H__ */

#ifdef GTK_MAJOR_VERSION
#if ( GTK_MAJOR_VERSION < 1 ) || \
  ( GTK_MAJOR_VERSION == 1 && GTK_MINOR_VERSION <= 1 )
#define OLD_GTK         1
#else
#define OLD_GTK         0
#endif
#endif


// PV: debug version
// #define PV_DEBUG	1



#endif /* _GTIMER_H */

