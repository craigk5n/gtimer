/*
 * Copyright (C) 1999 Craig Knudsen, cknudsen@cknudsen.com
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * Description:
 *	Helps you keep track of time spent on different tasks.
 *
 * Author:
 *	Craig Knudsen, cknudsen@cknudsen.com, http://www.cknudsen.com
 *
 * Home Page:
 *	http://www.cknudsen.com/gtimer/
 *
 * History:
 *	17-Apr-2005	Added configurability of the browser. (Russ Allbery)
 *	04-Apr-98	Created
 *			(Code stolen from another project/program I wrote.)
 *
 ****************************************************************************/


#ifndef _CONFIG_H
#define _CONFIG_H

/* config file parameters */

#define CONFIG_DEFAULT_FILE		".gtimerrc"

#define CONFIG_VERSION			"version"
#define CONFIG_SORT			"sort"
#define CONFIG_SORT_FORWARD		"sort-dir"
#define CONFIG_PRINT			"print"
#define CONFIG_IDLE			"idle"
#define CONFIG_IDLE_ON			"idle-on"
#define CONFIG_TOOLBAR_STATUS		"toolbar-status"
#define CONFIG_AUTOSAVE			"autosave"
#define CONFIG_ANIMATE			"animate"
#define CONFIG_USE_PROJECTS		"use-projects"
#define CONFIG_MAIN_WINDOW_WIDTH	"win_width"
#define CONFIG_MAIN_WINDOW_HEIGHT	"win_height"
#define CONFIG_MAIN_WINDOW_PROJECT_WIDTH	"list_project_width"
#define CONFIG_MAIN_WINDOW_TASK_WIDTH	"list_task_width"
#define CONFIG_MAIN_WINDOW_TODAY_WIDTH	"list_today_width"
#define CONFIG_MAIN_WINDOW_TOTAL_WIDTH	"list_total_width"
#define CONFIG_NEXT_VERSION_CHECK	"next_version_check"
#define CONFIG_LAST_TIMED_TASKS		"timed_tasks"
#define CONFIG_BROWSER			"browser"

/* default values */
#ifdef CONFIG_DEFAULTS
static char *default_config[] = {
  CONFIG_SORT, "0",
  CONFIG_SORT_FORWARD, "1",
  CONFIG_PRINT, "lpr",
  CONFIG_IDLE, "300",
  CONFIG_IDLE_ON, "1",
  CONFIG_AUTOSAVE, "1",
  CONFIG_ANIMATE, "1",
  CONFIG_USE_PROJECTS, "1",
  CONFIG_TOOLBAR_STATUS, "1",
  CONFIG_MAIN_WINDOW_WIDTH, "500",
  CONFIG_MAIN_WINDOW_HEIGHT, "400",
  CONFIG_NEXT_VERSION_CHECK, "0",

  /* rra 2005-07-15: Changed to sensible-browser for Debian. */
  CONFIG_BROWSER, "sensible-browser",
  NULL,
};
#endif

int configReadAttributes (
#ifndef _NO_PROTO
  char *attrfile
#endif
);
int configGetAttribute (
#ifndef _NO_PROTO
  char *attribute, char **value
#endif
);
int configGetAttributeInt (
#ifndef _NO_PROTO
  char *attribute, int *value
#endif
);
int configSetAttribute (
#ifndef _NO_PROTO
  char *attribute, char *value
#endif
);
int configSetAttributeInt (
#ifndef _NO_PROTO
  char *attribute, int value
#endif
);
int configSaveAttributes (
#ifndef _NO_PROTO
  char *attrfile
#endif
);
int configModified ();
void configClear ();

#endif /* _CONFIG_H */
