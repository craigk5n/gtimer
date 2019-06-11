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
 *	Craig Knudsen, craig@k5n.us, http://www.k5n.us
 *
 * Home Page:
 *	http://www.cknudsen.com/gtimer/
 *
 * History:
 *      07-Aug-2008	Ver 1.1.7 - Support UTF-8 using GTK+ ver. > 2.4.0
 *	15-Jul-2005	Add -weekstart to configure the first day of the
 *			week.  (Russ Allbery)
 *	31-May-2005	When exiting, only change the window size
 *			configuration if gdk_window_get_size succeeds.
 *			(Russ Allbery)
 *	18-Apr-2005	Fully stop timing and update the icons properly
 *			after a reset from the idle window.
 *			Subtract off the five minute initial period when
 *			reverting after an idle.  (Russ Allbery)
 *	17-Apr-2005	Added configurability of the browser. (Russ Allbery)
 *	02-Jan-2004	Add time increment and decrement by one minute.
 *			Add more key shortcuts for increment/decrement.
 *			64-bit fixes for time changes.  (Klaus Ethgen)
 *	02-Jan-2004	Add a callback to clear the paste buffer.  (Klaus
 *			Ethgen)
 *	13-Mar-2003	Added cut/copy/paste in new Edit menu.
 *	06-Mar-2003	Added support for -resume option which will start
 *			timing the same task that was being timed last
 *			time gtimer exited.
 *	01-Mar-2003	Added projects
 *	09-Mar-2000	Improved idle handling.  On revert, the tasks are
 *			no longer reloaded from the data files and the
 *			display will not be resorted.
 *			Added a "resume" option for idles that continues
 *			timing but throws out all the idle time.
 *	21-May-1999	Moved around menu (added tools and help)
 *	19-May-1999	Added ability to check for new version.
 *			This is a really COOL feature :-)
 *	13-May-1999	Allow users to view ChangeLog.
 *	13-May-1999	Added handle box to menu.
 *	04-May-1999	Oops.  Fixed label that said "inc 5 seconds" to
 *			say "inc 5 minutes".
 *	30-Apr-1999	Added support help options: -h, -help, --help
 *	30-Apr-1999	Added support version options: -v, -version, --version
 *	30-Apr-1999	Added support for -start option.
 *	25-Mar-1999	Made animated clock in clist optional.
 *	25-Mar-1999	Applied some WIN32 patches provided by
 *			Thomas Epperly <Thomas.Epperly@aspentech.com>
 *	25-Mar-1999	Fixed bug where -nosplash would cause the main window
 *			to not remember the correct window size.
 *	25-Mar-1999	Fixed bugs when using start/stop/etc. before any
 *			tasks have been created.
 *	24-Mar-1999	Made toolbar optional
 *	24-Mar-1999	Added tearoff menus
 *	18-Mar-1999	Internationalization
 *	18-Mar-1999	Added support for X11 screen saver extension
 *			for idle detect.  (This will detect keyboard
 *			usage rather than just mouse usage.)
 *	16-Mar-1999	Added back in support for GTK 1.0 via autoconf.
 *			(Stole some of the autoconf stuff from xhippo-0.7.)
 *			GTK 1.1/1.2 handles clist double-click events
 *			completely different than GTK 1.0.
 *	24-Feb-1999	Added unhide function (finally... have had the
 *			hide function for a while now.)
 *	11-Feb-1999	Modified accelerator key code.
 *			Fixed to have task pulldown menu with right mouse
 *			button (API changed from GTK+1.0)
 *	08-Feb-1999	Added changes for accelerator keys provided by
 *			Matt Martin <mmartin@Calvin.SFC.Lehigh.Edu>
 *	02-Feb-1999	Remember main window width & height
 *	21-Jan-1999	Added gtk-1.1 support (received patches from
 *			Stephen Webb & Jim Bray).
 *	11-Nov-1998	Added support for hiding tasks
 *	13-Jul-1998	Make double-click stop timing all other tasks and
 *			start timing the selected task.
 *	09-May-1998	Added autosave option
 *	07-May-1998	Finished idle detect option
 *	08-Apr-1998	Set the application icon within the application.
 *			Display a different icon depening on whether or not
 *			we are timing any tasks.
 *	06-Apr-1998	Began adding code for idle detect
 *	05-Apr-1998	Fixed vertical resize (status was resizing)
 *			code submitted by Zach Beane (xach@mint.net)
 *	05-Apr-1998	Added splash screen.
 *	01-Apr-1998	Added status bar and total hours for today at
 *			the bottom of the main window.
 *	18-Mar-1998	Reduce flicker in task list when changing sort
 *			code submitted by Zach Beane (xach@mint.net)
 *	18-Mar-1998	Release 0.95
 *	18-Mar-1998	Added calls to gtk_window_set_wmclass so the windows
 *			behave better for window managers.
 *			code submitted by ObiTuarY (Obituary@cybernet.be)
 *	17-Mar-1998	Fixed handing if $HOME is not defined.
 *	16-Mar-1998	Changed name to "GTimer"
 *			updated application icon
 *	15-Mar-1998	Added memory debugging calls (memdebug.h).
 *			(The memdebug library is something I wrote myself
 *			a few years ago for another project.  It keeps
 *			track of all malloc/realloc/free calls and can
 *			show you what's been allocated but not freed with
 *			the md_print_all () function.  Email me if you
 *			would like this library.)
 *	15-Mar-1998	Added annotate icon and ability to add annotations
 *			(dated comments) to tasks.
 *	13-Mar-1998	Click on column header to sort by that column.
 *	13-Mar-1998	Did some code redesign.  Pulldown menus setup
 *			from the TTPulldown structure.
 *	13-Mar-1998	Add pulldown menu for right mouse click on a task.
 *			Added functions to add time and remove time from
 *			a task.
 *	13-Mar-1998	Add pulldown menu for right mouse click on a task.
 *			code submitted by Zach Beane (xach@mint.net)
 *	13-Mar-1998	Double-click on a task brings up the edit window
 *			code submitted by Zach Beane (xach@mint.net)
 *	13-Mar-1998	Fixed handling of date change when time passes
 *			midnight.
 *	11-Mar-1998	Rearranged UI.  Added toolbar with icons.
 *			Added "Report" to menubar.
 *	25-Feb-1998	Created
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <pwd.h>
#include <time.h>
#include <memory.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glib.h>

#include <gtk/gtk.h>
#ifdef HAVE_APP_INDICATOR
#include <libappindicator/app-indicator.h>
#endif

#ifdef HAVE_SCREEN_SAVER_EXT
#include <gdk/gdkx.h>
#endif

#include "project.h"
#include "task.h"
#include "gtimer.h"
#include "config.h"
#include "tcpt.h"
#include "http.h"
// PV:
#include "custom-list.h"


#ifdef GTIMER_MEMDEBUG
#include "memdebug/memdebug.h"
#endif

/* check for a new version every 30 days */
#define VERSION_CHECK_INTERVAL		(3600 * 24 * 30)

/* splash icon */
#include "icons/splash.xpm"

/* app icons */
#include "icons/gtimer.xpm"
#include "icons/gtimer2.xpm"

/* timer icon */
#include "icons/clock1.xpm"
#include "icons/clock2.xpm"
#include "icons/clock3.xpm"
#include "icons/clock4.xpm"
#include "icons/clock5.xpm"
#include "icons/clock6.xpm"
#include "icons/clock7.xpm"
#include "icons/clock8.xpm"
#include "icons/blank.xpm"

/* toolbar icons */
#include "icons/start.xpm"
#include "icons/stop.xpm"
#include "icons/stop_all.xpm"
#include "icons/annotate.xpm"
#include "icons/new.xpm"
#include "icons/edit.xpm"

GtkWidget *main_window = NULL;
static GtkWidget *splash_window = NULL;
static int move_to_task = -1;
static GtkWidget *idle_prompt_window = NULL;
static GtkWidget *option_menu_items[4];
static time_t splash_until, last_save;
static int modified_since_save = 0;
static int splash_seconds = 2;
GtkWidget *toolbar = NULL;
GtkWidget *task_list = NULL;
GtkWidget *status = NULL;
guint status_id = 0;
static time_t lastMessageTime = 0;
GtkWidget *total_label = NULL;
static char total_str[20];
GdkPixmap *icons[8], *blankicon, *appicon, *appicon2;
GdkBitmap *icon_masks[8], *blankicon_mask, *appicon_mask, *appicon2_mask;
#if OLD_GTK
#else
GtkAccelGroup* mainag;
#endif
#ifdef HAVE_APP_INDICATOR
static AppIndicator *indicator;
#endif
static sockfd connection = -1;
static gint gdk_input_id = -1;
static int version_check_is_auto = 0;
static int cutBuffer = 0; /* Seconds from cut/copy/set-to-zero/revert/resume*/
static int messageDisplayTime = 15; /* how long to leave messages up */

typedef struct {
  char *name;
  int width;
  int max_width;
  GtkJustification justify;
  gboolean resizeable;
  GtkWidget *widget;
} list_column_def;

static int sort_forward = 1;
static int last_sort = 0;
static int rebuilding_list = 0;

int today_year, today_mon, today_mday;
int config_midnight_offset = 0;
int config_max_idle = 0;
int config_idle_enabled = 0;
int config_autosave_enabled = 1;
int config_toolbar_enabled = 1;
int config_animate_enabled = 1;
int config_autosave_interval = (60*15); /* 15 minutes */
int config_start_of_week = 0;

char *taskdir = NULL;
char *config_file = NULL;
char *gtkrc = NULL;
int selected_task = -1;
int pulldown_selected_task = -1; /* task selected with right mouse button */

TaskData **tasks;
int num_tasks = 0;
int num_timing = 0;
TaskData **visible_tasks; /* not hidden */
int num_visible_tasks;

list_column_def task_list_columns[4] = {
  { "Project",	150, 0,	GTK_JUSTIFY_LEFT,	(gboolean)1,	NULL },
  { "Task",	150, 0,	GTK_JUSTIFY_LEFT,	(gboolean)1,	NULL },
  { "Today",	70, 70,	GTK_JUSTIFY_RIGHT,	(gboolean)0,	NULL },
  { "Total",	70, 70,	GTK_JUSTIFY_RIGHT,	(gboolean)0,	NULL }
};

/*
** Local functions
*/
void update_list ();
static void build_list ();
static void about_callback ( GtkAction *act );
static void website_callback ( GtkAction *act );
static void changelog_callback ( GtkAction *act );
static void save_callback ( GtkAction *act );
static void exit_callback ( GtkAction *act );
static void start_callback ( GtkAction *act );
static void switch_to_callback ( GtkWidget *widget, gpointer data );
static void stop_callback ( GtkAction *act );
static void stop_all_callback ( GtkAction *act );
static void task_add_callback ( GtkAction *act );
static void task_edit_callback ( GtkAction *act );
static void task_hide_callback ( GtkAction *act );
static void task_unhide_callback ( GtkAction *act );
static void task_delete_callback ( GtkAction *act );
static void task_cut_callback ( GtkAction *act );
static void task_copy_callback ( GtkAction *act );
static void task_paste_callback ( GtkAction *act );
static void task_clear_callback ( GtkAction *act );
static void increment_time_callback ( GtkWidget *widget, gpointer data );
static void decrement_time_callback ( GtkWidget *widget, gpointer data );
static void project_add_callback ( GtkAction *act );
static void project_edit_callback ( GtkAction *act );
//static void report_callback ( GtkWidget *widget, gpointer data );
static void annotate_callback ( GtkAction *act );
static void idle_reset_callback ( GtkWidget *widget, gpointer data );
static void idle_cancel_callback ( GtkWidget *widget, gpointer data );
static void idle_resume_callback ( GtkWidget *widget, gpointer data );
static void column_selected_callback ( GtkWidget *widget, int col );
static void toolbar_toggle_callback ( GtkToggleAction *act );
static void idle_toggle_callback ( GtkToggleAction *act );
static void autosave_toggle_callback ( GtkToggleAction *act );
static void animate_toggle_callback ( GtkToggleAction *act );
static void check_version_callback ( GtkAction *act );
static void set_browser_callback ( GtkAction *act );

// PV:
static void shift_time_callback ( GtkAction *act );
static void report2_callback ( GtkAction *act );


/*
** Structure for defining the the pulldown menus.
*/
typedef struct {
  int label_index;
  void (*callback)();
  gpointer data;
  gint acckey;
  GdkModifierType accmod;
} TTPulldown;


// PV: move main menu to GtkActionEntry Structures (see GTK+ Reference Manual,
// Example 55 - 59)

// globally accessible UI Manager object
GtkUIManager *uimanager;

// Menu Entries - because it is neccessary to translate each menu item as separate word, I decide
// to assign unique prefix to each menu column and merge these prefixes to menu items.
// Prefix will be hidden during translation (see function mainmenu_translate).
// Last character in prefix is '|', I hope this character never appears as a part of displayed word.
// If I'll not be right in future, please change it to another character in menu and function
// mainmenu_translate.
// Recommendation: It will be better _do_not_translate_ these prefixes.

static const GtkActionEntry MM_NormalEntries[] = {
  { "FileMenu", NULL, gettext_noop("MM|_File") },
  { "EditMenu", NULL, gettext_noop("MM|_Edit") },
  { "OptionsMenu", NULL, gettext_noop("MM|_Options") },
  { "TaskMenu", NULL, gettext_noop( "MM|_Task") },
  { "ProjectMenu", NULL, gettext_noop("MM|_Project") },
  { "ReportMenu", NULL, gettext_noop("MM|_Report") },
  { "ToolsMenu", NULL, gettext_noop("MM|_Tools") },
  { "HelpMenu", NULL, gettext_noop("MM|_Help") },
  { "FM_Save", GTK_STOCK_SAVE, gettext_noop("FM|_Save"), "<control>S",
	gettext_noop("Save actual state"), G_CALLBACK(save_callback) },
  { "FM_Exit", GTK_STOCK_QUIT, gettext_noop("FM|_Exit"), "<control>Q",
	gettext_noop("Exit program"), G_CALLBACK(exit_callback) },
  { "EM_Cut", GTK_STOCK_CUT, gettext_noop("EM|C_ut Time"), "<control>X",
	gettext_noop("Cut actual time from line to buffer"), G_CALLBACK(task_cut_callback) },
  { "EM_Copy", GTK_STOCK_COPY, gettext_noop("EM|_Copy Time"), "<control>C",
	gettext_noop("Copy actual time from line to buffer"), G_CALLBACK(task_copy_callback) },
  { "EM_Paste", GTK_STOCK_PASTE, gettext_noop("EM|_Paste Time"), "<control>V",
	gettext_noop("Paste actual time from buffer to actual line"), G_CALLBACK(task_paste_callback) },
  { "EM_Clear", GTK_STOCK_CLEAR, gettext_noop("EM|Clear _Buffer"), NULL,
	gettext_noop("Clear buffer"), G_CALLBACK(task_clear_callback) },
  { "OM_Browser", NULL, gettext_noop("OM|_Browser ..."), NULL,
	gettext_noop("Set your preferred web browser"), G_CALLBACK(set_browser_callback) },
  { "TM_Start", NULL, gettext_noop("TM|_Start Timing"), "<alt>S",
	gettext_noop("Start timing of selected task"), G_CALLBACK(start_callback) },
  { "TM_Stop", NULL, gettext_noop("TM|S_top Timing"), "<alt>X",
	gettext_noop("Stop timing of selected task"), G_CALLBACK(stop_callback) },
  { "TM_Stopall", NULL, gettext_noop("TM|Stop A_ll Timing"), "<alt>T",
	gettext_noop("Stop timing of all running tasks"), G_CALLBACK(stop_all_callback) },
  { "TM_New", NULL, gettext_noop("TM|_New..."), "<control>N",
	gettext_noop("Create new task"), G_CALLBACK(task_add_callback) },
  { "TM_Edit", NULL, gettext_noop("TM|_Edit... "), "<control>E",
	gettext_noop("Edit selected task"), G_CALLBACK(task_edit_callback) },
  { "TM_Annotate", NULL, gettext_noop("TM|_Annotate..."), "<control>A",
	gettext_noop("Add annotation"), G_CALLBACK(annotate_callback) },
  { "TM_Hide", NULL, gettext_noop("TM|_Hide"), "<control>H",
	gettext_noop("Hide task"), G_CALLBACK(task_hide_callback) },
  { "TM_Unhide", NULL, gettext_noop("TM|_Unhide... "), "<control>U",
	gettext_noop("Unhide tasks"), G_CALLBACK(task_unhide_callback) },
  { "TM_Delete", NULL, gettext_noop("TM|_Delete"), "<control>R",
	gettext_noop("Add annotation"), G_CALLBACK(task_delete_callback) },
  { "TM_Inc1", NULL, gettext_noop("TM|_Increment 1 minute"), "<shift><control>I",
	gettext_noop("Increment time"), G_CALLBACK(shift_time_callback) },
  { "TM_Inc5", NULL, gettext_noop("TM|I_ncrement 5 minutes"), "<control>I",
	gettext_noop("Increment time"), G_CALLBACK(shift_time_callback) },
  { "TM_Inc30", NULL, gettext_noop("TM|In_crement 30 minute"), "<control><alt>I",
	gettext_noop("Increment time"), G_CALLBACK(shift_time_callback) },
  { "TM_Dec1", NULL, gettext_noop("TM|Decrement _1 minute"), "<shift><control>D",
	gettext_noop("Decrement time"), G_CALLBACK(shift_time_callback) },
  { "TM_Dec5", NULL, gettext_noop("TM|Decrement _5 minutes"), "<control>D",
	gettext_noop("Decrement time"), G_CALLBACK(shift_time_callback) },
  { "TM_Dec30", NULL, gettext_noop("TM|Decrement _30 minutes"), "<control><alt>D",
	gettext_noop("Decrement time"), G_CALLBACK(shift_time_callback) },
  { "TM_Zero", NULL, gettext_noop("TM|Set to _Zero"), "<control><alt>0",
	gettext_noop("Clear time of selected task"), G_CALLBACK(shift_time_callback) },
  { "PM_New", NULL, gettext_noop("PM|_New..."), NULL,
	gettext_noop("New Project"), G_CALLBACK(project_add_callback) },
  { "PM_Edit", NULL, gettext_noop("PM|_Edit..."), NULL,
	gettext_noop("Edit Project"), G_CALLBACK(project_edit_callback) },
  { "RM_Daily", NULL, gettext_noop("RM|_Daily..."), NULL,
	gettext_noop("Daily Report"), G_CALLBACK(report2_callback) },
  { "RM_Weekly", NULL, gettext_noop("RM|_Weekly..."), NULL,
	gettext_noop("Weekly Report"), G_CALLBACK(report2_callback) },
  { "RM_Monthly", NULL, gettext_noop("RM|_Monthly..."), NULL,
	gettext_noop("Monthly Report"), G_CALLBACK(report2_callback) },
  { "RM_Yearly", NULL, gettext_noop("RM|_Yearly..."), NULL,
	gettext_noop("Yearly Report"), G_CALLBACK(report2_callback) },
  { "TL_Check", NULL, gettext_noop("TL|_Check for New Version..."), NULL,
	gettext_noop("Keep your program updated"), G_CALLBACK(check_version_callback) },
  { "HM_About", NULL, gettext_noop("HM|_About..."), NULL,
	gettext_noop("About"), G_CALLBACK(about_callback) },
  { "HM_ViewChL", NULL, gettext_noop("HM|View _Change log..."), NULL,
	gettext_noop("Changelog"), G_CALLBACK(changelog_callback) },
  { "HM_VisitWeb", NULL, gettext_noop("HM|Visit _Website..."), NULL,
	gettext_noop("Open web browser"), G_CALLBACK(website_callback) }
};

/* PV: Add Toggle Entries
 *   This structure is initialized with FALSE initial values. Before activating it will
 *   be modified according to configuration settings. This is why this structure is not
 *   declared as 'const'
 */

//static const GtkToggleActionEntry MM_ToggleEntries[] = {
static GtkToggleActionEntry MM_ToggleEntries[] = {
  { "OM_Toolbar", NULL, gettext_noop("OM|_Toolbar"), NULL,
	NULL, G_CALLBACK(toolbar_toggle_callback), FALSE },
  { "OM_Animate", NULL, gettext_noop("OM|_Animate"), NULL,
	NULL, G_CALLBACK(animate_toggle_callback), FALSE },
  { "OM_Idle", NULL, gettext_noop("OM|_Idle Detect"), NULL,
	NULL, G_CALLBACK(idle_toggle_callback), FALSE },
  { "OM_Autosave", NULL, gettext_noop("OM|Auto _Save"), NULL,
	NULL, G_CALLBACK(autosave_toggle_callback), FALSE },
};

// according to MM_ToggleEntries set following constants:
#define	MM_TOGGLE_TOOLBAR	0
#define	MM_TOGGLE_ANIMATE	1
#define	MM_TOGGLE_IDLE		2
#define	MM_TOGGLE_AUTOSAVE	3




//PV: XML description:

static const char * menustring =
"<ui>"
"  <menubar name='MainMenu'>"
"    <menu action='FileMenu'>"
"      <menuitem action='FM_Save'/>"
"      <separator/>"
"      <menuitem action='FM_Exit'/>"
"    </menu>"
"    <menu action='EditMenu'>"
"      <menuitem action='EM_Cut'/>"
"      <menuitem action='EM_Copy'/>"
"      <menuitem action='EM_Paste'/>"
"      <menuitem action='EM_Clear'/>"
"    </menu>"
"    <menu action='OptionsMenu'>"
"      <menuitem action='OM_Toolbar'/>"
"      <menuitem action='OM_Animate'/>"
"      <menuitem action='OM_Idle'/>"
"      <menuitem action='OM_Autosave'/>"
"      <menuitem action='OM_Browser'/>"
"    </menu>"
"    <menu action='TaskMenu'>"
"      <menuitem action='TM_Start'/>"
"      <menuitem action='TM_Stop'/>"
"      <menuitem action='TM_Stopall'/>"
"      <separator/>"
"      <menuitem action='TM_New'/>"
"      <menuitem action='TM_Edit'/>"
"      <menuitem action='TM_Annotate'/>"
"      <menuitem action='TM_Hide'/>"
"      <menuitem action='TM_Unhide'/>"
"      <menuitem action='TM_Delete'/>"
"      <separator/>"
"      <menuitem action='TM_Inc1'/>"
"      <menuitem action='TM_Inc5'/>"
"      <menuitem action='TM_Inc30'/>"
"      <menuitem action='TM_Dec1'/>"
"      <menuitem action='TM_Dec5'/>"
"      <menuitem action='TM_Dec30'/>"
"      <menuitem action='TM_Zero'/>"
"    </menu>"
"    <menu action='ProjectMenu'>"
"      <menuitem action='PM_New'/>"
"      <menuitem action='PM_Edit'/>"
"    </menu>"
"    <menu action='ReportMenu'>"
"      <menuitem action='RM_Daily'/>"
"      <menuitem action='RM_Weekly'/>"
"      <menuitem action='RM_Monthly'/>"
"      <menuitem action='RM_Yearly'/>"
"    </menu>"
"    <menu action='ToolsMenu'>"
"      <menuitem action='TL_Check'/>"
"    </menu>"
"    <separator/>"
"    <menu action='HelpMenu'>"
"      <menuitem action='HM_About'/>"
"      <menuitem action='HM_ViewChL'/>"
"      <menuitem action='HM_VisitWeb'/>"
"    </menu>"
"  </menubar>"
"  <popup name='PopupMenu'>"
"      <menuitem action='TM_Start'/>"
"      <menuitem action='TM_Stop'/>"
"      <menuitem action='TM_Stopall'/>"
"      <separator/>"
"      <menuitem action='TM_New'/>"
"      <menuitem action='TM_Edit'/>"
"      <menuitem action='TM_Annotate'/>"
"      <menuitem action='TM_Hide'/>"
"      <menuitem action='TM_Unhide'/>"
"      <menuitem action='TM_Delete'/>"
"      <separator/>"
"      <menuitem action='TM_Inc1'/>"
"      <menuitem action='TM_Inc5'/>"
"      <menuitem action='TM_Inc30'/>"
"      <menuitem action='TM_Dec1'/>"
"      <menuitem action='TM_Dec5'/>"
"      <menuitem action='TM_Dec30'/>"
"      <menuitem action='TM_Zero'/>"
"  </popup>"
"</ui>";



/*
** Structure for defining the toolbar
*/
// PV: Move it to UI manager too? Later ...
typedef struct {
  char *label;
  char *tooltip_index;
  gchar **icon_data;
  void (*callback)();
  GtkWidget *widget;
  gpointer data;
} TTToolButton;
#define TOOLBAR_START_BUTTON		0
#define TOOLBAR_STOP_BUTTON		1
#define TOOLBAR_STOP_ALL_BUTTON		2
TTToolButton main_toolbar[] = {
  { gettext_noop("Start"),
    gettext_noop("Start Timing the Selected Task"),
    start_xpm,
    start_callback, NULL, NULL },
  { gettext_noop("Stop"),
    gettext_noop("Stop Timing the Selected Task"),
    stop_xpm,
    stop_callback, NULL, NULL },
  { gettext_noop("Stop All"),
    gettext_noop("Stop Timing All Tasks"),
    stop_all_xpm,
    stop_all_callback, NULL, NULL },
  { gettext_noop("Annotate"),
    gettext_noop("Add Annotation to Selected Task"),
    annotate_xpm,
    annotate_callback, NULL, NULL },
  { gettext_noop("Add"),
    gettext_noop("Add New Task"),
    new_xpm,
    task_add_callback, NULL, NULL },
  { gettext_noop("Edit"),
    gettext_noop("Edit Name of the Selected Task") ,
    edit_xpm,
    task_edit_callback, NULL, NULL },
  { NULL, 0, NULL, NULL, NULL, NULL }
};



#ifdef WIN32
static void convert_backslash ( filename )
char *filename;
{
  while ( *filename ){
    if ( *filename == '\\' )
      *filename = '/';
    filename++;
  }
}
#endif


/*
** Get the currently selected task.  If the user has used the right mouse
** button to create a pulldown menu, then pulldown_selected_task will
** be set.  This routine should called only once per callback since
** we reset the value of pulldown_selected_task.
*/
static int get_selected_task ()
{
  int ret;

  if ( pulldown_selected_task >= 0 ) {
    ret = pulldown_selected_task;
    pulldown_selected_task = -1;
  } else
    ret = selected_task;

  return ret;
}


static int my_strcasecmp ( char *str1, char *str2 )
{
  char *ptr1, *ptr2, *ptr;
  int ret;

  ptr1 = strdup ( str1 );
  ptr2 = strdup ( str2 );

  for ( ptr = ptr1; *ptr != '\0'; ptr++ ) {
    *ptr = toupper ( *ptr );
  }
  for ( ptr = ptr2; *ptr != '\0'; ptr++ ) {
    *ptr = toupper ( *ptr );
  }

  ret = strcmp ( ptr1, ptr2 );
  free ( ptr1 );
  free ( ptr2 );
  return ( ret );
}


/*
 * Sort by project id, which will put oldest projects first, newest
 * projects last.
 */
static int sort_task_by_project_id ( td1, td2 )
TaskData **td1;
TaskData **td2;
{
  TaskData *tda = *td1;
  TaskData *tdb = *td2;
  int ret;

  /* put tasks with no projects (-1) at the end of the list, most
     recent projects at the top */
  if ( tda->task->project_id > tdb->task->project_id )
    ret = -1;
  else if ( tda->task->project_id < tdb->task->project_id )
    ret = 1;
  else
    ret = 0;

  if ( sort_forward )
    return ( ret );
  else
    return ( - ret );
}


/*
 * Sort by project name.
 */
static int sort_task_by_project_name ( td1, td2 )
TaskData **td1;
TaskData **td2;
{
  TaskData *tda = *td1;
  TaskData *tdb = *td2;
  int ret;

  /* put tasks with no projects (-1) at the end of the list, most
     recent projects at the top */
  if ( tda->task->project_id < 0 )
    ret = -1;
  else if ( tdb->task->project_id < 0 )
    ret = 1;
  else
    ret = my_strcasecmp ( tda->project_name, tdb->project_name );

  if ( sort_forward )
    return ( ret );
  else
    return ( - ret );
}

static int sort_task_by_name ( td1, td2 )
TaskData **td1;
TaskData **td2;
{
  TaskData *tda = *td1;
  TaskData *tdb = *td2;
  int ret;

  ret = ( my_strcasecmp ( tda->task->name, tdb->task->name ) );
  if ( sort_forward )
    return ( ret );
  else
    return ( - ret );
}

static int sort_task_by_id ( td1, td2 )
TaskData **td1;
TaskData **td2;
{
  TaskData *tda = *td1;
  TaskData *tdb = *td2;
  int ret;

  /* put most recent tasks at the top */
  if ( tda->task->number > tdb->task->number )
    ret = -1;
  else if ( tda->task->number < tdb->task->number )
    ret = 1;
  else
    ret = 0;

  if ( sort_forward )
    return ( ret );
  else
    return ( - ret );
}

static int sort_task_by_today ( td1, td2 )
TaskData **td1;
TaskData **td2;
{
  TaskData *tda = *td1;
  TaskData *tdb = *td2;
  int ret = 0;

  if ( tda->last_today_int > tdb->last_today_int )
    ret = 1;
  else if ( tda->last_today_int < tdb->last_today_int )
    ret = -1;
  else if ( tda->last_today_int == tdb->last_today_int )
    ret = ( (void *)tda < (void *)td2 );

  if ( sort_forward )
    return ( - ret );
  else
    return ( ret );
}


static int sort_task_by_total ( td1, td2 )
TaskData **td1;
TaskData **td2;
{
  TaskData *tda = *td1;
  TaskData *tdb = *td2;
  int ret = 0;

  if ( tda->last_total_int > tdb->last_total_int )
    ret = 1;
  else if ( tda->last_total_int < tdb->last_total_int )
    ret = -1;
  else if ( tda->last_total_int == tdb->last_total_int )
    ret = ( (void *)tda < (void *)td2 );

  if ( sort_forward )
    return ( - ret );
  else
    return ( ret );
}




/*
** Transfer all the time for tasks currently being timed into the
** Task data structure so the reports will have access to it easily.
*/
static void update_tasks ()
{
  int i;
  time_t now, diff;

  time ( &now );
  for ( i = 0; i < num_visible_tasks; i++ ) {
    if ( visible_tasks[i]->timer_on ) {
      diff = now - visible_tasks[i]->on_since;
      visible_tasks[i]->todays_entry->seconds += diff;
      visible_tasks[i]->on_since = now;
    }
  }
}




/*
** Save all the tasks to their files.
*/
void save_all ()
{
  update_tasks ();
  taskSaveAll ( taskdir );
  projectSaveAll ( taskdir );
  time ( &last_save );
  modified_since_save = 0;
}



/* Delete window handler */
gint delete_event ( widget, event, data )
GtkWidget *widget;
GdkEvent *event;
gpointer data;
{
  save_all ();
  configSaveAttributes ( config_file );
#ifdef GTIMER_MEMDEBUG
  configClear ();
#endif
  return ( TRUE );
}

static void exit_callback ( GtkAction *act )
{
  gint w, h;
  char temp[128];
  int loop;

  /* save task data */
  save_all ();

  /* save window size */
  w = 0;
  h = 0;
  gdk_window_get_size ( GTK_WIDGET ( main_window )->window, &w, &h );
  if ( w != 0 && h != 0 ) {
    configSetAttributeInt ( CONFIG_MAIN_WINDOW_WIDTH, w );
    configSetAttributeInt ( CONFIG_MAIN_WINDOW_HEIGHT, h );
  }

  /* get columns widths in main window */
  w = GTK_CLIST ( task_list )->column[0].width;
  configSetAttributeInt ( CONFIG_MAIN_WINDOW_PROJECT_WIDTH, w );
  w = GTK_CLIST ( task_list )->column[1].width;
  configSetAttributeInt ( CONFIG_MAIN_WINDOW_TASK_WIDTH, w );
  w = GTK_CLIST ( task_list )->column[2].width;
  configSetAttributeInt ( CONFIG_MAIN_WINDOW_TODAY_WIDTH, w );
  w = GTK_CLIST ( task_list )->column[3].width;
  configSetAttributeInt ( CONFIG_MAIN_WINDOW_TOTAL_WIDTH, w );

  /* keep track of which tasks were being timed in case the user starts up
     with -resume next time */
  temp[0] = '\0';
  for ( loop = 0; loop < num_visible_tasks; loop++ ) {
    if ( visible_tasks[loop]->timer_on ) {
      if ( strlen ( temp ) )
        strcat ( temp, "," );
      sprintf ( temp + strlen ( temp ), "%d",
        visible_tasks[loop]->task->number );
    }
  }
  configSetAttribute ( CONFIG_LAST_TIMED_TASKS, temp );

  /* save config settings */
  configSaveAttributes ( config_file );

#ifdef GTIMER_MEMDEBUG
  free ( config_file );
  configClear ();
#endif
  gtk_main_quit ();
}

/*
** Set toolbar buttons to sensitive/insensitive based
** on our current status.
*/
static void update_toolbar_buttons () {
  if ( num_timing ) {
    gtk_widget_set_sensitive (
      GTK_WIDGET ( main_toolbar[TOOLBAR_STOP_BUTTON].widget ), 1 );
    gtk_widget_set_sensitive (
      GTK_WIDGET ( main_toolbar[TOOLBAR_STOP_ALL_BUTTON].widget ), 1 );
  } else {
    gtk_widget_set_sensitive (
      GTK_WIDGET ( main_toolbar[TOOLBAR_STOP_BUTTON].widget ), 0 );
    gtk_widget_set_sensitive (
      GTK_WIDGET ( main_toolbar[TOOLBAR_STOP_ALL_BUTTON].widget ), 0 );
  }
  if ( num_visible_tasks ) {
    gtk_widget_set_sensitive (
      GTK_WIDGET ( main_toolbar[TOOLBAR_START_BUTTON].widget ), 1 );
  } else {
    gtk_widget_set_sensitive (
      GTK_WIDGET ( main_toolbar[TOOLBAR_START_BUTTON].widget ), 0 );
  }
}

static void save_callback ( GtkAction *act )
{
  showMessage ( gettext("All data saved") );
  save_all ();
}

static void about_callback ( GtkAction *act )
{
  char text[1024];

  sprintf ( text,
    "GTimer\n%s\n%s: %s (%s)\n%s\nGTK %s: %d.%d.%d\n\n",
    GTIMER_COPYRIGHT, gettext("Version"), GTIMER_VERSION, GTIMER_VERSION_DATE,
    GTIMER_URL, gettext("Version"),
    gtk_major_version, gtk_minor_version, gtk_micro_version );
  strcat ( text, gettext("Author") );
  sprintf ( text + strlen ( text ),
     ":\nCraig Knudsen\ncraig@k5n.us\n\n" );
  create_confirm_window ( CONFIRM_ABOUT,
    gettext("About"),
    text,
    gettext("Ok"), NULL, NULL, NULL, NULL, NULL, NULL );
}

static void changelog_callback ( GtkAction *act )
{
  display_changelog ();
}


static void website_callback ( GtkAction *act )
{
  char *path, *command;

  if ( configGetAttribute ( CONFIG_BROWSER, &path ) < 0 )
    path = "mozilla";

  command = (char *) malloc ( strlen ( path ) + 128 );
  if ( strstr ( path, "%s" ) )
    sprintf ( command, path, GTIMER_URL );
  else
    sprintf ( command, "%s %s", path, GTIMER_URL );

  if ( system ( command ) != 0 ) {
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"), gettext("Error communicating with browser."),
      gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL, NULL );
  }

  free ( command );


}



static void task_add_callback ( GtkAction *act )
{
  create_task_edit_window ( NULL );
}

static void task_edit_callback ( GtkAction *act )
{
  int st = get_selected_task ();

  if ( st < 0 || ! num_visible_tasks ) {
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"),
      gettext("You have not selected\na task to edit."),
      gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL,
      NULL );
  } else {
    create_task_edit_window ( visible_tasks[st] );
  }
}



static void task_hide_callback ( GtkAction *act )
{
  TaskData *td;
  int i, st;

  st = get_selected_task ();
  if ( st < 0 || ! num_visible_tasks ) {
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"),
      gettext("You have not selected\na task to hide."),
      gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL,
      NULL );
  } else {
    td = visible_tasks[st];
    taskSetOption ( td->task, GTIMER_TASK_OPTION_HIDDEN );
    td->timer_on = 0;
    for ( i = st; i < num_visible_tasks; i++ ) {
      if ( i + 1 < num_visible_tasks )
        visible_tasks[i] = visible_tasks[i + 1];
    }
    num_visible_tasks--;
    update_list ();
    gtk_clist_remove ( GTK_CLIST ( task_list ), st );
    showMessage ( gettext("Task hidden") );
  }
}



static void task_unhide_callback ( GtkAction *act )
{
  int st;

  st = get_selected_task ();
  if ( num_tasks == num_visible_tasks ) {
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"),
      gettext("There are no hidden tasks."),
      gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL,
      NULL );
  } else {
    update_list ();
    create_unhide_window ();
  }
}



static void delete_confirm_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  TaskData *td = (TaskData *)data;
  int ret, loop, tasknumber;
  char msg[500];

  if ( ( ret = taskDelete ( td->task, taskdir ) ) ) {
    sprintf ( msg, "%s:\n%s",
      gettext("Error deleting task"), taskErrorString ( ret ) );
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"),
      msg,
      gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL, NULL );
  }

  /* delete from visible_tasks[] and the list window */
  tasknumber = -1;
  for ( loop = 0; loop < num_visible_tasks && tasknumber < 0; loop++ ) {
    if ( visible_tasks[loop] == td )
      tasknumber = loop;
  }
  if ( tasknumber >= 0 ) {
    gtk_clist_remove ( GTK_CLIST ( task_list ), tasknumber );
    for ( loop = tasknumber; loop < num_visible_tasks; loop++ ) {
      if ( loop + 1 < num_visible_tasks )
        visible_tasks[loop] = visible_tasks[loop + 1];
    }
  }

  /* delete from tasks[] */
  tasknumber = -1;
  for ( loop = 0; loop < num_tasks && tasknumber < 0; loop++ ) {
    if ( tasks[loop] == td )
      tasknumber = loop;
  }
  if ( tasknumber >= 0 ) {
    for ( loop = tasknumber; loop < num_tasks; loop++ ) {
      if ( loop + 1 < num_tasks )
        tasks[loop] = tasks[loop + 1];
    }
  }

  free ( td );
  num_tasks--;
  num_visible_tasks--;
  showMessage ( gettext("Task removed") );

  update_list ();
}


static void task_delete_callback ( GtkAction *act )
{
  int st = get_selected_task ();

  if ( st < 0 || ! num_visible_tasks ) {
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"),
      gettext("You have not selected\na task to delete."),
      gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL,
      NULL );
  } else {
    create_confirm_window ( CONFIRM_CONFIRM,
      gettext("Delete Task?"),
      gettext("Are you sure you want\nto delete this task?"),
      gettext("Ok"), gettext("Cancel"), NULL,
      delete_confirm_callback, NULL, NULL,
      (char *)visible_tasks[st] );
  }
}

static void start_callback ( GtkAction *act )
{
  TaskData *td;
  int st = get_selected_task ();

  if ( st < 0 || ! num_visible_tasks ) {
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"),
      gettext("You have not selected\na task to start timing."),
      gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL,
      NULL );
  } else {
    td = visible_tasks[st];
    if ( td->timer_on ) {
      create_confirm_window ( CONFIRM_ERROR,
        gettext("Error"),
        gettext("Task is already being timed."),
        gettext("Ok"), NULL, NULL,
        NULL, NULL, NULL,
        NULL );
    } else {
      td->timer_on = 1;
      time ( &td->on_since );
      if ( td->todays_entry == NULL )
        td->todays_entry = taskNewTimeEntry ( td->task, today_year,
          today_mon, today_mday );
      update_list ();
      num_timing++;
      if ( num_timing == 1 )
        gdk_window_set_icon ( GTK_WIDGET ( main_window )->window,
          NULL, appicon, appicon_mask );
    }
  }
  update_toolbar_buttons ();
}

static void stop_callback ( GtkAction *act )
{
  TaskData *td;
  time_t now, diff;
  int st = get_selected_task ();

  if ( st < 0 || ! num_visible_tasks ) {
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"),
      gettext("You have not selected\na task to stop timing."),
      gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL,
      NULL );
  } else {
    td = visible_tasks[st];
    if ( ! td->timer_on ) {
      create_confirm_window ( CONFIRM_ERROR,
        gettext("Error"),
        gettext("Task is not being timed."),
        gettext("Ok"), NULL, NULL,
        NULL, NULL, NULL,
        NULL );
    } else {
      td->timer_on = 0;
      time ( &now );
      diff = now - td->on_since;
      td->todays_entry->seconds += diff;
      td->on_since = 0;
      update_list ();
      num_timing--;
      if ( num_timing == 0 )
        gdk_window_set_icon ( GTK_WIDGET ( main_window )->window,
          NULL, appicon2, appicon2_mask );
    }
  }
  update_toolbar_buttons ();
}




static void annotate_callback ( GtkAction *act )
{
  int st = get_selected_task ();

  if ( st < 0 || ! num_visible_tasks ) {
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"),
      gettext("You have not selected\na task to annotate."),
      gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL,
      NULL );
  } else {
    create_annotate_window ( visible_tasks[st] );
  }
}


static void stop_all_callback ( GtkAction *act )
{
  TaskData *td;
  time_t now, diff;
  int loop;

  get_selected_task (); /* reset pulldown task selection */

  for ( loop = 0; loop < num_visible_tasks; loop++ ) {
    td = visible_tasks[loop];
    if ( td->timer_on ) {
      td->timer_on = 0;
      time ( &now );
      diff = now - td->on_since;
      td->todays_entry->seconds += diff;
      td->on_since = 0;
    }
  }
  if ( num_timing )
    gdk_window_set_icon ( GTK_WIDGET ( main_window )->window,
       NULL, appicon2, appicon2_mask );
  num_timing = 0;
  update_list ();
  update_toolbar_buttons ();
}



static void switch_to_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  TaskData *td;
  time_t now, diff;
  int loop;
  int new_icon = 1;
  int st = get_selected_task ();

  for ( loop = 0; loop < num_visible_tasks; loop++ ) {
    td = visible_tasks[loop];
    if ( td->timer_on ) {
      td->timer_on = 0;
      time ( &now );
      diff = now - td->on_since;
      td->todays_entry->seconds += diff;
      td->on_since = 0;
      new_icon = 0;
      num_timing--;
    }
  }
  td = visible_tasks[st];
  td->timer_on = 1;
  time ( &td->on_since );
  if ( td->todays_entry == NULL )
    td->todays_entry = taskNewTimeEntry ( td->task, today_year,
      today_mon, today_mday );
  update_list ();
  num_timing = 1;
  if ( new_icon )
    gdk_window_set_icon ( GTK_WIDGET ( main_window )->window,
       NULL, appicon, appicon_mask );
  update_list ();
  update_toolbar_buttons ();
}


static void toolbar_toggle_callback ( GtkToggleAction *act )
{
  int active = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION( act ));
  if ( active ) {
    configSetAttributeInt ( CONFIG_TOOLBAR_STATUS, 1 );
    gtk_widget_show ( toolbar );
  } else {
    configSetAttributeInt ( CONFIG_TOOLBAR_STATUS, 0 );
    gtk_widget_hide ( toolbar );
  }
  config_toolbar_enabled = active;
}


static void animate_toggle_callback ( GtkToggleAction *act )
{
  int active = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION( act ));
  if ( active )
    configSetAttributeInt ( CONFIG_ANIMATE, 1 );
  else
    configSetAttributeInt ( CONFIG_ANIMATE, 0 );
  config_animate_enabled = active;
}


static void idle_toggle_callback ( GtkToggleAction *act )
{
  int active = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION( act ));
  if ( active )
    configSetAttributeInt ( CONFIG_IDLE_ON, 1 );
  else
    configSetAttributeInt ( CONFIG_IDLE_ON, 0 );
  config_idle_enabled = active;
}


static void autosave_toggle_callback ( GtkToggleAction *act )
{
  int active = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION( act ));
  if ( active )
    configSetAttributeInt ( CONFIG_AUTOSAVE, 1 );
  else
    configSetAttributeInt ( CONFIG_AUTOSAVE, 0 );
  config_autosave_enabled = active;
}


static void set_browser_callback ( GtkAction *act )
{
  create_browser_edit_window ();
}


static void project_add_callback ( GtkAction *act )
{
  create_project_edit_window ( NULL );
}

static void project_edit_callback ( GtkAction *act )
{
  int st = get_selected_task ();
  Project *p;

  if ( st < 0 || ! num_visible_tasks ) {
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"),
      gettext("You have not selected\na task."),
      gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL,
      NULL );
  } else {
    if ( visible_tasks[st]->task->project_id < 0 ) {
      create_confirm_window ( CONFIRM_ERROR,
        gettext("Error"),
        gettext("The selected task does not\nhave a project."),
        gettext("Ok"), NULL, NULL,
        NULL, NULL, NULL,
        NULL );
    } else {
      p = projectGet ( visible_tasks[st]->task->project_id );
      create_project_edit_window ( p );
    }
  }
}

static void report2_callback ( GtkAction *act )
{
//  report_type rt = (report_type) data;

  typedef struct {
	gchar *name;
	report_type rt;
  } TTarray;

  const TTarray acts[] = {
	{"RM_Daily", REPORT_TYPE_DAILY},
	{"RM_Weekly", REPORT_TYPE_WEEKLY},
	{"RM_MOnthly", REPORT_TYPE_MONTHLY},
	{"RM_Yearly", REPORT_TYPE_YEARLY}
   };
  const gchar *aname = gtk_action_get_name(GTK_ACTION(act));
  report_type rt = REPORT_TYPE_NONE ;
  int i;

  for ( i = 0 ; i <= 3 ; i++ ) {
    if ( !strcmp ( acts[i].name, aname ) ) {
	rt = acts[i].rt;
	break;
    }
  }
  if ( rt == REPORT_TYPE_NONE ) {
	fprintf( stderr, "Gtimer: Unknown report (%s)!\n", aname); return;
  }

#if PV_DEBUG
  g_message("Report 2 cback start");
#endif
  update_tasks ();
  create_report_window ( rt );

#if PV_DEBUG
  g_message("Report 2 cback end");
#endif

}

/*
 * Display the time in the cutBuffer in the status area.
 */
static void showCutBuffer ()
{
  char temp[48];
  int cb, h, m, s;

  cb = cutBuffer;
  h = cb / 3600;
  cb %= 3600;
  m = cb / 60;
  cb %= 60;
  s = cb;

  sprintf ( temp, "%s %02d:%02d:%02d",
    gettext ("Cut/Paste buffer is" ), h, m, s );
  showMessage ( temp );
}

static void adjust_task_time ( offset )
int offset;
{
  TaskData *td;
  int st = get_selected_task ();

  if ( st < 0 ) {
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"),
      gettext("You have not selected\na task to adjust the time for."),
      gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL,
      NULL );
  } else {
    update_tasks ();
    td = visible_tasks[st];
    if ( td->todays_entry == NULL )
      td->todays_entry = taskNewTimeEntry ( td->task,
        today_year, today_mon, today_mday );
    if ( offset == 0 ) {
      /* set to zero */
      cutBuffer = td->todays_entry->seconds;
      td->todays_entry->seconds = 0;
    } else if ( offset < 0 ) {
      /* decrement */
      if ( td->todays_entry->seconds < ( 0 - offset ) ) {
        cutBuffer += td->todays_entry->seconds;
        td->todays_entry->seconds = 0;
      } else {
        cutBuffer += - offset;
        td->todays_entry->seconds += offset;
      }
    } else {
      td->todays_entry->seconds += offset;
    }
    modified_since_save = 1;
    update_list ();
  }
}



/*
** Revert to where we were when we first noticed that the user was idle.
*/
static void idle_reset_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  int loop;
  int timeBeforeReset, timeAfterReset;
  time_t now;

  idle_prompt_window = NULL;

  time ( &now );

  /* add up total time for today */
  for ( timeBeforeReset = loop = 0; loop < num_tasks; loop++ )  {
    if ( tasks[loop]->todays_entry ) {
      timeBeforeReset += tasks[loop]->todays_entry->seconds;
      if ( tasks[loop]->timer_on ) {
        timeBeforeReset += ( now - tasks[loop]->on_since );
      }
    }
  }

  taskRestoreAll ();

  for ( timeAfterReset = loop = 0; loop < num_tasks; loop++ )  {
    if ( tasks[loop]->timer_on ) {
      tasks[loop]->timer_on = 0;
      tasks[loop]->on_since = 0;
      num_timing--;
    }
    if ( tasks[loop]->todays_entry ) {
      timeAfterReset += tasks[loop]->todays_entry->seconds;
    }
  }
  update_list ();
  update_toolbar_buttons ();

  /* reset icon */
  gdk_window_set_icon ( GTK_WIDGET ( main_window )->window,
    NULL, appicon2, appicon2_mask );

  /*
   * Store however much time we just removed in the cut buffer, so the
   * user can Edit->Paste to apply the time to a different task
   */
  cutBuffer = timeBeforeReset - timeAfterReset;
  if ( cutBuffer < 0 )
    cutBuffer = 0;
  showCutBuffer ();
}

static void task_clear_callback ( GtkAction *act )
{
  /* set to zero */
  cutBuffer = 0;
  showCutBuffer ();
}

static void idle_cancel_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  idle_prompt_window = NULL;
}

/*
** Revert to the data that was last saved to file (which we did when
** we first noticed the user was idle.) and continue timing.
** Save the idle time in the cut/paste buffer.
*/
static void idle_resume_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  int loop;
  int timeBeforeReset, timeAfterReset;
  time_t now;

  time ( &now );

  idle_prompt_window = NULL;

  /* add up total time for today */
  for ( timeBeforeReset = loop = 0; loop < num_tasks; loop++ )  {
    if ( tasks[loop]->todays_entry ) {
      timeBeforeReset += tasks[loop]->todays_entry->seconds;
      if ( tasks[loop]->timer_on ) {
        timeBeforeReset += ( now - tasks[loop]->on_since );
      }
    }
  }

  taskRestoreAll ();

  for ( timeAfterReset = loop = 0; loop < num_visible_tasks; loop++ ) {
    if ( visible_tasks[loop]->timer_on )
      time ( &visible_tasks[loop]->on_since );
    if ( tasks[loop]->todays_entry )
      timeAfterReset += tasks[loop]->todays_entry->seconds;
  }
  update_list ();

  /*
   * Store however much time we just removed in the cut buffer, so the
   * user can Edit->Paste to apply the time to a different task
   */
  cutBuffer = timeBeforeReset - timeAfterReset;
  if ( cutBuffer < 0 )
    cutBuffer = 0;
  showCutBuffer ();
}


static void task_cut_callback ( GtkAction *act )
{
  adjust_task_time ( 0 );
  showCutBuffer ();
}

static void task_copy_callback ( GtkAction *act )
{
  TaskData *td;
  int st = get_selected_task ();

  cutBuffer = 0;

  if ( st < 0 ) {
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"),
      gettext("You have not selected a task."),
      gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL,
      NULL );
  } else {
    update_tasks ();
    td = visible_tasks[st];
    if ( td->todays_entry != NULL ) {
      cutBuffer = td->todays_entry->seconds;
    }
    showCutBuffer ();
  }
}

static void task_paste_callback ( GtkAction *act )
{
  if ( cutBuffer > 0 ) {
    adjust_task_time ( cutBuffer );
  }
  /* note that we don't reset the cut buffer back to zero here.
     this allows them to paste more than once. */
  showCutBuffer ();
}


static void increment_time_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  int offset = GPOINTER_TO_INT(data);
  adjust_task_time ( offset );
}


static void decrement_time_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  int offset = GPOINTER_TO_INT(data);
  adjust_task_time ( 0 - offset );
  showCutBuffer ();
}


static void shift_time_callback ( GtkAction *act )
{
  typedef struct {
	gchar *name;
	int shift;
  } TTarray;

  int time_shift;  //Shift in seconds
  int i;

  const TTarray acts[] = {
	{"TM_Inc1",    60 },
	{"TM_Inc5",   300 },
	{"TM_Inc30", 1800 },
	{"TM_Dec1",   -60 },
	{"TM_Dec5",  -300 },
	{"TM_Dec30",-1800 },
	{"TM_Zero",     0 }
  };

  const gchar *aname = gtk_action_get_name(GTK_ACTION(act));

  time_shift = -1;

  for ( i = 0 ; i <= 6 ; i++ ) {
//  g_message( "%s x %s", acts[i].name, aname);
    if ( !strcmp ( acts[i].name, aname ) ) {
	time_shift = acts[i].shift;
	break;
    }
  }
  if ( time_shift == -1 ) {
	fprintf( stderr, "Gtimer: Unknown action (%s)!\n", aname); return;
  }
#if PV_DEBUG
  g_message("Shift_time: %d s", time_shift);
#endif
  adjust_task_time(time_shift);
  if (time_shift < 0 ) showCutBuffer();
}


static void read_http_socket ( data, source, condition )
gpointer data;
gint source;
GdkInputCondition condition;
{
  httpProcessRead ( connection );
}


/*
** This function will be called when the HTTP data is ready.
** It could actually be called multiple times each time a read()
** completes, but since the version info is so short, this will most
** likely never happen.
*/
static void read_version ( data, len )
char *data;
int len;	/* NOT NULL-terminated! */
{
  char *ptr, *data2, *msg, version[30];
  time_t now;
  char timestr[20];

  if ( len == 0 ) {
    gdk_input_remove ( gdk_input_id );
    httpKillConnection ( connection );
    connection = -1;
  } else {
    data2 = (char *) malloc ( len + 1 );
    strncpy ( data2, data, len );
    data2[len] = '\0';
    if ( strstr ( data2, "Not Found" ) != NULL ||
      strstr ( data2, "Not found" ) != NULL ||
      strstr ( data2, "not found" ) != NULL ) {
      if ( ! version_check_is_auto )
        create_confirm_window ( CONFIRM_ERROR,
          gettext("Error"),
          gettext("This service is no longer available."),
          gettext("Ok"), NULL, NULL, NULL, NULL, NULL, NULL );
    } else {
      ptr = strstr ( data2, "GTimer Version" );
      if ( ptr == NULL ) {
        if ( ! version_check_is_auto )
          create_confirm_window ( CONFIRM_ERROR,
            gettext("Error"),
            gettext("Unable to determine available GTimer version."),
            gettext("Ok"), NULL, NULL, NULL, NULL, NULL, NULL );
      } else {
        ptr += 15; /* skip over "GTimer Version" */
        sprintf ( version, "%s (%s)", GTIMER_VERSION, GTIMER_VERSION_DATE );
        if ( strncmp ( version, ptr, strlen ( version ) ) == 0 ) {
          if ( ! version_check_is_auto )
            create_confirm_window ( CONFIRM_ABOUT,
              gettext("Version"),
              gettext("You have the most recent version of GTimer."),
              gettext("Ok"), NULL, NULL, NULL, NULL, NULL, NULL );
        } else if ( strncmp ( version, ptr, strlen ( version ) ) > 0 ) {
          create_confirm_window ( CONFIRM_ABOUT,
            gettext("Version"),
            gettext("Strange....  You seem to have a more recent version\nof GTimer than is available on the server."),
            gettext("Ok"), NULL, NULL, NULL, NULL, NULL, NULL );
        } else {
          msg = (char *) malloc ( 200 + strlen ( ptr ) );
          sprintf ( msg, "%s:\n\n%s\n%s:\n\n%s",
            gettext("There is a new version of GTimer available"),
            ptr,
            gettext("You can download it at"),
            GTIMER_URL );
          create_confirm_window ( CONFIRM_ABOUT,
            gettext("Version"),
            msg, gettext("Ok"), NULL, NULL, NULL, NULL, NULL, NULL );
          free ( msg );
        }
      }
    }
    gdk_input_remove ( gdk_input_id );
    httpKillConnection ( connection );
    connection = -1;
  }

  /* calcuate the next time we will need to do this check */
  time ( &now );
  now += VERSION_CHECK_INTERVAL;
  sprintf ( timestr, "%ul", (unsigned int)now );
  configSetAttribute ( CONFIG_NEXT_VERSION_CHECK, timestr );
  version_check_is_auto = FALSE;
}

static void check_version_callback ( GtkAction *act )
{
  httpError ret;
  char msg[400];

  if ( connection >= 0 )
    httpKillConnection ( connection );

  ret = httpOpenConnection ( GTIMER_VERSION_CHECK_SERVER,
    GTIMER_VERSION_CHECK_PORT, &connection );
  if ( ret ) {
    /* only report errors if the user asked for a version check */
    if ( ! version_check_is_auto ) {
      strcpy ( msg,
        gettext("An error occurred while\nchecking for a new version.") );
      strcat ( msg, "\n\n" );
      sprintf ( msg + strlen ( msg ), "%s:\n\n%s", gettext("HTTP Error"),
        httpErrorString ( ret ) );
      create_confirm_window ( CONFIRM_ERROR,
        gettext("Error"), msg, gettext("Ok"), NULL, NULL,
        NULL, NULL, NULL, NULL );
    }
    version_check_is_auto = FALSE;
  } else {
    gdk_input_id = gdk_input_add ( (gint) connection,
      GDK_INPUT_READ, read_http_socket, NULL );
    ret = httpGet ( connection, GTIMER_VERSION_CHECK_SERVER,
      GTIMER_VERSION_CHECK_PATH, NULL, NULL, 0, read_version );
    if ( ret ) {
      if ( ! version_check_is_auto ) {
        sprintf ( msg, "%s:\n\n%s", gettext("HTTP Error"),
          httpErrorString ( ret ) );
        create_confirm_window ( CONFIRM_ERROR,
          gettext("Error"), msg, gettext("Ok"), NULL, NULL,
          NULL, NULL, NULL, NULL );
      }
      httpKillConnection ( connection );
      gdk_input_remove ( gdk_input_id );
      gdk_input_id = -1;
      version_check_is_auto = FALSE;
    }
  }
}




/*
** A generic event handler for the task pulldown menu (when created from
** a right mouse button in the task list).  If the user doesn't select
** anything from the pulldown, we need to forget about which task they
** selected for the pulldown.
*/
static gint pulldown_event ( widget, event )
GtkWidget *widget;
GdkEvent *event;
{
  if ( event->type == GDK_UNMAP )
    get_selected_task ();
  return ( FALSE );
}


/*
** Create a pulldown menu (from the user selecting a task with the
** right mouse button);
*/
static GtkWidget *create_task_pulldown ( int is_main ) {
  return  gtk_ui_manager_get_widget( uimanager, "/PopupMenu" );
}



/*
** Callback for selecting the column header.
** Sort the list by the selected column.
*/
static void column_selected_callback ( widget, col )
GtkWidget *widget;
int col;
{
  int i;
  static int same_col = 0;

  /*
   * Keep track of how many times they click on the same column header.
   * When they do this with the project column, it will sort as follows:
   *   1) project id
   *   2) project id (reverse)
   *   3) project name
   *   4) project name (reverse)
   *   5) back to 1...
   */
  if ( col == last_sort ) {
    same_col++;
  } else {
    same_col = 0;
  }

  if ( col == last_sort )
    sort_forward = ! sort_forward;
  else
    sort_forward = 1;
  last_sort = col;
  switch ( col ) {
    case 0:
      if ( same_col % 4 < 2 ) {
        qsort ( visible_tasks, num_visible_tasks, sizeof ( TaskData * ),
          sort_task_by_project_id );
        showMessage ( gettext("Sorted by project creation date") );
      } else {
        qsort ( visible_tasks, num_visible_tasks, sizeof ( TaskData * ),
          sort_task_by_project_name );
        showMessage ( gettext("Sorted by project name") );
      }
      break;
    case 1:
      if ( same_col % 4 < 2 ) {
        qsort ( visible_tasks, num_visible_tasks, sizeof ( TaskData * ),
          sort_task_by_name );
        showMessage ( gettext("Sorted by task name") );
      } else {
        qsort ( visible_tasks, num_visible_tasks, sizeof ( TaskData * ),
          sort_task_by_id );
        showMessage ( gettext("Sorted by task creation date") );
      }
      break;
    case 2:
      qsort ( visible_tasks, num_visible_tasks, sizeof ( TaskData * ),
        sort_task_by_today );
      showMessage ( gettext("Sorted by time for today") );
      break;
    default:
    case 3:
      qsort ( visible_tasks, num_visible_tasks, sizeof ( TaskData * ),
        sort_task_by_total );
      showMessage ( gettext("Sorted by total time") );
      break;
  }
  rebuilding_list = 1;
  gtk_clist_freeze( GTK_CLIST (task_list) );
  build_list ();
  update_list ();
  rebuilding_list = 0;
  for ( i = 0; i < num_visible_tasks; i++ ) {
    if ( visible_tasks[i]->selected )
      gtk_clist_select_row ( GTK_CLIST (task_list), i, 0 );
  }
  gtk_clist_thaw( GTK_CLIST (task_list) );
  configSetAttributeInt ( CONFIG_SORT, col );
  configSetAttributeInt ( CONFIG_SORT_FORWARD, sort_forward );
}






/*
** General event handler for the task list clist widget.  Catch
** right mouse button events and create the pulldown menu.
*/
static gint task_list_event ( widget, event )
GtkWidget *widget;
GdkEvent *event;
{
  GdkEventButton *eb;
  GtkWidget *menu;
  int row, col;

  if ( event->type == GDK_BUTTON_PRESS ) {
    eb = (GdkEventButton *)event;
    if ( eb->button == 3 ) {
      gtk_clist_get_selection_info ( GTK_CLIST ( task_list ),
        eb->x, eb->y, &row, &col );
      pulldown_selected_task = row;
      menu = create_task_pulldown ( FALSE );
      gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL,
         NULL, 3, eb->time);
    }
  }
  return ( FALSE );
}


/*
** Callback for user selecting a task (single-click, double-click,
** right-mouse, etc.)
*/
static gint task_selected_callback ( widget, col, row, bevent, user_data )
GtkWidget *widget;
int col;
int row;
GdkEventButton *bevent;
gpointer user_data;
{
  int i;

  if ( rebuilding_list )
    return ( TRUE );

  for ( i = 0; i < num_visible_tasks; i++ )
    visible_tasks[i]->selected = 0;

  selected_task = col;
  if ( selected_task >= 0 )
    visible_tasks[selected_task]->selected = 1;

  if ( bevent != NULL ) {
    /* double-click ? */
    if ( bevent->type == GDK_2BUTTON_PRESS )
      switch_to_callback ( widget, user_data );
  }

  return ( TRUE );
}

/*
** PV: Main Menu translations - because of duplicated items (e.g. "New" in menu Task and Project)
**     we have to know which one item is translated. Each string in menu can contain prefix in form
**     <prefix><delimiter><text>. Delimiter is now hardcoded as '|' (or any character not contained
**     in menu description).
*/


gchar *mainmenu_translate(const gchar *str, gpointer data)
{
	gchar *retval;
	gchar **split;

	retval = gettext(str);

        if ( strchr(str, '|') ) {

          split = g_strsplit(retval,"|",2);


	  //debug:
#if PV_DEBUG
          printf("\nPrefix: %s", split[0]);
          printf("\nText  : %s", split[1]);
#endif
          retval = g_strdup(split[1]);
          g_strfreev(split);
#if PV_DEBUG
          printf("\n*** Retval *** %s", retval);
#endif
	}

	return retval;
}

/*
** Create the main window's menu bar.
** PV: For GTK 2.x --- Actions menus
**
*/
static GtkWidget *create_main_window_menu_bar()
{
  GtkWidget *menu;
  GtkWidget *root_menu;
  GtkWidget *menu_item;
  GtkWidget *menu_bar;
//  int loop;
//  PV: GTK2
//  GtkUIManager *uimanager;
  GtkActionGroup *actgroup;
  GtkAction *item;
  GError *err = NULL;

  actgroup = gtk_action_group_new("ActionMain");
  // set translation domain and function
  gtk_action_group_set_translation_domain(GTK_ACTION_GROUP(actgroup), DEFAULT_TEXT_DOMAIN);
  gtk_action_group_set_translate_func(GTK_ACTION_GROUP(actgroup), mainmenu_translate, NULL, NULL);
  // set entries
  gtk_action_group_add_actions  (GTK_ACTION_GROUP(actgroup), MM_NormalEntries,
				G_N_ELEMENTS(MM_NormalEntries), main_window);
  gtk_action_group_add_toggle_actions (GTK_ACTION_GROUP(actgroup), MM_ToggleEntries,
				G_N_ELEMENTS(MM_ToggleEntries), main_window);


  uimanager = gtk_ui_manager_new();
  gtk_ui_manager_set_add_tearoffs(uimanager, TRUE);
  gtk_ui_manager_insert_action_group(GTK_UI_MANAGER(uimanager),
                                     GTK_ACTION_GROUP(actgroup),
				     0);
  mainag = gtk_ui_manager_get_accel_group (uimanager);
  gtk_window_add_accel_group (GTK_WINDOW(main_window), GTK_ACCEL_GROUP(mainag));

  gtk_ui_manager_add_ui_from_string (uimanager, menustring, -1, &err);

    if (!(err == NULL))
    {
	g_message(gettext("Building menu Failed: %s \n"), err->message);
	g_error_free(err);
    }

  return ( gtk_ui_manager_get_widget (uimanager, "/MainMenu") );
}


static GtkWidget *create_list_column_def (num, cols)
int num;
list_column_def *cols;
{
  GtkWidget *clist;
  GtkWidget *alignment;
  GtkWidget *label;
  int i;

  clist = gtk_clist_new (num);
#ifdef GTK_CLIST_SET_FLAGS /* GTK 1.0.1 */
  GTK_CLIST_SET_FLAGS (clist, CLIST_SHOW_TITLES);
#else
  GTK_CLIST_SET_FLAG (clist, CLIST_SHOW_TITLES);
#endif

  for (i = 0; i < num; i++) {
    gtk_clist_set_column_width (GTK_CLIST (clist), i, cols[i].width);
/*
    gtk_clist_set_column_resizeable (GTK_CLIST (clist), i, cols[i].resizeable);
    gtk_clist_set_column_auto_resize (GTK_CLIST (clist), i, (gboolean)1);
    if ( cols[i].max_width )
      gtk_clist_set_column_max_width ( GTK_CLIST (clist), i,
        cols[i].max_width );
*/
    if (cols[i].justify != GTK_JUSTIFY_LEFT) {
      gtk_clist_set_column_justification (GTK_CLIST (clist), i,
        cols[i].justify);
    }

    alignment = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);

    /*label = gtk_label_new (cols[i].name);*/
    label = gtk_label_new ( gettext(cols[i].name) );
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
    gtk_container_add (GTK_CONTAINER (alignment), label);
    gtk_widget_show (label);

    cols[i].widget = label;

    gtk_clist_set_column_widget (GTK_CLIST (clist), i, alignment);
    /*gtk_clist_column_title_passive ( GTK_CLIST(clist), i );*/
    gtk_widget_show (alignment);
  }

  return clist;
}



/*
** Update the time values shown in the list.
*/
void update_list () {
  TaskData *taskdata;
  int i;
  int h, m, s;
  char text[100];
  time_t now, diff, total, today;
  GdkPixmap *icon;
  GdkBitmap *mask;
  char *row[4];
  int total_today = 0;
  char today_test[20];
  char *project_name;
  Project *p;
  static char *noproject = "";

  time ( &now );
  if ( config_animate_enabled ) {
    icon = icons[now%8];
    mask = icon_masks[now%8];
  } else {
    icon = icons[0];
    mask = icon_masks[0];
  }

  /*gtk_clist_freeze ( GTK_CLIST(task_list) );*/
  for ( i = 0; i < num_visible_tasks; i++ ) {
    taskdata = visible_tasks[i];
    /* new task ? */
    if ( taskdata->new_task ) {
      modified_since_save = 1;
      taskdata->new_task = 0;
      if ( taskdata->task->project_id > 0 ) {
        p = projectGet ( taskdata->task->project_id );
        project_name = p->name;
      } else {
        project_name = noproject;
      }
      row[0] = project_name;
      row[1] = taskdata->task->name;
      row[2] = "00:00:00";
      row[3] = "00:00:00";
      gtk_clist_append ( GTK_CLIST(task_list), row );
      gtk_clist_set_pixtext (GTK_CLIST (task_list), i, 0,
        taskdata->project_name, 2, blankicon, blankicon_mask);
      continue;
    }
    /* update the name ? */
    if ( taskdata->name_updated || taskdata->moved ) {
      modified_since_save = 1;
      taskdata->name_updated = 0;
      if ( taskdata->timer_on ) {
        gtk_clist_set_pixtext (GTK_CLIST (task_list), i, 0,
          taskdata->project_name, 2, icon, mask);
        taskdata->last_on = 1;
      } else {
        gtk_clist_set_pixtext (GTK_CLIST (task_list), i, 0,
          taskdata->project_name, 2, blankicon, blankicon_mask);
        taskdata->last_on = 0;
      }
    }
    gtk_clist_set_text ( GTK_CLIST(task_list), i, 1, taskdata->task->name );
    /* calc total */
    total = taskdata->total;
    if ( taskdata->todays_entry )
      total += taskdata->todays_entry->seconds;
    if ( taskdata->timer_on ) {
      time ( &now );
      diff = now - taskdata->on_since;
      total += diff;
    }
    h = total / 3600;
    m = ( total - h * 3600 ) / 60;
    s = total % 60;
    sprintf ( text, "%d:%02d:%02d", h, m, s );
    if ( strcmp ( text, taskdata->last_total ) || taskdata->moved ) {
      gtk_clist_set_text ( GTK_CLIST(task_list), i, 3, text );
      strcpy ( taskdata->last_total, text );
    }

    taskdata->last_total_int = total;
    today = 0;
    if ( taskdata->todays_entry )
      today = taskdata->todays_entry->seconds;
    if ( taskdata->timer_on ) {
      time ( &now );
      diff = now - taskdata->on_since;
      today += diff;
    }
    h = today / 3600;
    m = ( today - h * 3600 ) / 60;
    s = today % 60;
    sprintf ( text, "%d:%02d:%02d", h, m, s );
    if ( strcmp ( text, taskdata->last_today ) || taskdata->moved ) {
      gtk_clist_set_text ( GTK_CLIST(task_list), i, 2, text );
      strcpy ( taskdata->last_today, text );
    }
    taskdata->last_today_int = today;
    /* draw the icon ? */
    if ( taskdata->timer_on ) {
      modified_since_save = 1;
      gtk_clist_set_pixtext (GTK_CLIST (task_list), i, 0,
        taskdata->project_name, 2, icon, mask);
      taskdata->last_on = 1;
    } else if ( ! taskdata->timer_on && taskdata->last_on ) {
      gtk_clist_set_pixtext (GTK_CLIST (task_list), i, 0,
        taskdata->project_name, 2, blankicon, blankicon_mask);
      taskdata->last_on = 0;
    }
    taskdata->moved = 0;
    total_today += today;
  }
  /*gtk_clist_thaw ( GTK_CLIST(task_list) );*/

  h = total_today / 3600;
  m = ( total_today - h * 3600 ) / 60;
  s = total_today % 60;
  sprintf ( today_test, "%s: %d:%02d:%02d", gettext("Today"), h, m, s );
  if ( strcmp ( today_test, total_str ) ) {
    strcpy ( total_str, today_test );
    gtk_label_set ( GTK_LABEL ( total_label ), total_str );
  }
#ifdef HAVE_APP_INDICATOR
  app_indicator_set_label(indicator, today_test + 7, "Gtimer Indicator");
#endif
}


/*
** Create the task list.
*/
static void build_list () {
  Task *task;
  TaskData *taskdata;
  Project *p;
  char *project_name;
  char today_str[100], total_str[100];
  char *row[4];
  int i, j;
  GdkPixmap *icon;
  GdkBitmap *mask;
  time_t now;
  static int first = 1;
  GtkWidget *win;
  static char *noproject = "";

  if ( splash_window )
    win = splash_window;
  else
    win = main_window;

  /* gtk_clist_freeze ( GTK_CLIST(task_list) ); */
  gtk_clist_clear ( GTK_CLIST(task_list) );

  if ( first ) {
    icons[0] = gdk_pixmap_create_from_xpm_d (
      GTK_WIDGET ( win )->window, &icon_masks[0],
      &win->style->white, clock1_xpm);
    icons[1] = gdk_pixmap_create_from_xpm_d (
      GTK_WIDGET ( win )->window, &icon_masks[1],
      &win->style->white, clock2_xpm);
    icons[2] = gdk_pixmap_create_from_xpm_d (
      GTK_WIDGET ( win )->window, &icon_masks[2],
      &win->style->white, clock3_xpm);
    icons[3] = gdk_pixmap_create_from_xpm_d (
      GTK_WIDGET ( win )->window, &icon_masks[3],
      &win->style->white, clock4_xpm);
    icons[4] = gdk_pixmap_create_from_xpm_d (
      GTK_WIDGET ( win )->window, &icon_masks[4],
      &win->style->white, clock5_xpm);
    icons[5] = gdk_pixmap_create_from_xpm_d (
      GTK_WIDGET ( win )->window, &icon_masks[5],
      &win->style->white, clock6_xpm);
    icons[6] = gdk_pixmap_create_from_xpm_d (
      GTK_WIDGET ( win )->window, &icon_masks[6],
      &win->style->white, clock7_xpm);
    icons[7] = gdk_pixmap_create_from_xpm_d (
      GTK_WIDGET ( win )->window, &icon_masks[7],
      &win->style->white, clock8_xpm);
    blankicon = gdk_pixmap_create_from_xpm_d (
      GTK_WIDGET ( win )->window, &blankicon_mask,
      &win->style->white, blank_xpm);
  }

  if ( tasks == NULL ) {
    tasks = (TaskData **) malloc ( taskCount() * sizeof ( TaskData * ) );
    visible_tasks = (TaskData **) malloc ( taskCount() *
      sizeof ( TaskData * ) );
    for ( i = 0, task = taskGetFirst(); task != NULL;
      i++, task = taskGetNext () ) {
      taskdata = (TaskData *) malloc ( sizeof ( TaskData ) );
      memset ( taskdata, '\0', sizeof ( TaskData ) );
      taskdata->task = task;
      taskdata->todays_entry = taskGetTimeEntry ( taskdata->task, today_year,
        today_mon, today_mday );
      for ( j = 0; j < taskdata->task->num_entries; j++ ) {
        if ( taskdata->task->entries[j] != taskdata->todays_entry )
          taskdata->total += taskdata->task->entries[j]->seconds;
      }
      strcpy ( taskdata->last_today, "" );
      strcpy ( taskdata->last_total, "" );
      taskdata->project_name = "";
      if ( taskdata->task->project_id >= 0 ) {
        p = projectGet ( task->project_id );
        if ( p != NULL )
          taskdata->project_name = p->name;
      }
      tasks[num_tasks++] = taskdata;
      if ( ! taskOptionEnabled ( taskdata->task, GTIMER_TASK_OPTION_HIDDEN ) )
        visible_tasks[num_visible_tasks++] = taskdata;
    }
    /* sort the list of tasks */
    qsort ( tasks, num_tasks, sizeof ( TaskData * ), sort_task_by_name );
    qsort ( visible_tasks, num_visible_tasks, sizeof ( TaskData * ),
      sort_task_by_name );
  }

  time ( &now );
  icon = icons[now%8];
  mask = icon_masks[now%8];
  for ( i = 0; i < num_visible_tasks; i++ ) {
    visible_tasks[i]->moved = 1;
    task = visible_tasks[i]->task;
    row[0] = noproject;
    if ( task->project_id >= 0 ) {
      p = projectGet ( task->project_id );
      row[0] = p->name;
    }
    row[1] = task->name;
    sprintf ( today_str, "00:00:00" );
    row[2] = today_str;
    sprintf ( total_str, "00:00:00" );
    row[3] = total_str;
    gtk_clist_append ( GTK_CLIST(task_list), row );
    if ( tasks[i]->timer_on )
      gtk_clist_set_pixtext (GTK_CLIST (task_list), i, 0,
        visible_tasks[i]->project_name, 2, icon, mask);
    else
      gtk_clist_set_pixtext (GTK_CLIST (task_list), i, 0,
        visible_tasks[i]->project_name, 2, blankicon, blankicon_mask);
  }
  /* gtk_clist_thaw ( GTK_CLIST(task_list) ); */

  first = 0;
}

/*
** Check for a new version of GTimer.
** Notice that, Unlike Xt, you don't have to add the timeout again :-)
*/
static gint version_timeout_handler ( gpointer data ) {
  time_t now;
  char *next_check, now_str[20];
  int do_check = FALSE;

  /**
   * 2003/05/09 Chad Walstrom <chewie@debian.org>
   *   Since this auto version checking is not configurable -- it can't be shut
   *   off -- we'll just shortcut the whole process and return TRUE like a
   *   happy camper.
   */
  return ( TRUE );

  time ( &now );
  sprintf ( now_str, "%ul", (unsigned int)now );
  if ( configGetAttribute ( CONFIG_NEXT_VERSION_CHECK, &next_check ) == 0 ) {
    if ( strcmp ( now_str, next_check ) > 0 ) {
      /* time for another check! */
      do_check = TRUE;
    }
  } else {
    /* have never checked! */
    do_check = TRUE;
  }

  if ( do_check ) {
    version_check_is_auto = TRUE;
    check_version_callback ( NULL );
  }

  /* return TRUE to so this timeout happens again */
  return ( TRUE );
}


/*
** Handle the update.  This gets called every 1 second.
** Notice that, Unlike Xt, you don't have to add the timeout again :-)
*/
static gint timeout_handler ( gpointer data ) {
  time_t now;
  struct tm *tm;
  int loop;
  gint w, h, x, y;
  static gint last_x, last_y;
  static time_t last_move = 0;
  GdkModifierType mask;
  char *ptr;
  int idle;

  time ( &now );

  /* remove splash window ? */
  if ( splash_window && now > splash_until ) {
    if ( GTK_IS_WIDGET ( splash_window ) )
      gtk_widget_destroy ( splash_window );
    splash_window = NULL;
    gtk_widget_show ( main_window );
    if ( configGetAttributeInt ( CONFIG_MAIN_WINDOW_WIDTH, &w ) == 0 &&
      configGetAttributeInt ( CONFIG_MAIN_WINDOW_HEIGHT, &h ) == 0 ) {
      gdk_window_resize ( GTK_WIDGET ( main_window )->window, w, h );
    }
    gdk_window_set_icon ( GTK_WIDGET ( main_window )->window,
      NULL, appicon2, appicon2_mask );
    if ( move_to_task >= 0 ) {
      gtk_clist_moveto ( GTK_CLIST ( task_list ), move_to_task, -1, 0.5, 0 );
      move_to_task = -1;
    }
  } else {
    gdk_window_get_pointer ( GTK_WIDGET ( main_window )->window,
      &x, &y, &mask );
    if ( x != last_x || y != last_y ) {
      last_x = x;
      last_y = y;
      last_move = now;
    }
  }

  /* Check to see if the date has changed. */
  now -= config_midnight_offset;
  tm = localtime ( &now );
  if ( today_mday != tm->tm_mday ) {
    update_tasks ();
    today_year = tm->tm_year + 1900;
    today_mon = tm->tm_mon + 1;
    today_mday = tm->tm_mday;
    for ( loop = 0; loop < num_tasks; loop++ ) {
      if ( tasks[loop]->todays_entry )
        tasks[loop]->total += tasks[loop]->todays_entry->seconds;
      tasks[loop]->todays_entry = taskGetTimeEntry ( tasks[loop]->task,
        today_year, today_mon, today_mday );
      if ( tasks[loop]->timer_on ) {
        if ( ! tasks[loop]->todays_entry )
          tasks[loop]->todays_entry = taskNewTimeEntry ( tasks[loop]->task,
            today_year, today_mon, today_mday );
        time ( &tasks[loop]->on_since );
      }
    }
  }

  /* Update the list */
  update_list ();

  /* have we been idle for too long? */
  if ( num_timing && configGetAttribute ( CONFIG_IDLE, &ptr ) == 0 &&
    ! idle_prompt_window && config_idle_enabled ) {
#ifdef HAVE_SCREEN_SAVER_EXT
    idle = (int) get_x_idle_time ( GDK_DISPLAY() );
#else
    time ( &now );
    idle = (int) ( now - last_move );
#endif
    if ( idle > config_max_idle) {
      /* we've been idle too long. mark time, save to file, then popup window */
      update_tasks ();
      save_all ();
      for ( loop = 0; loop < num_tasks; loop++ ) {
        if ( tasks[loop]->timer_on ) {
          taskMark ( tasks[loop]->task, idle );
        } else {
          taskMark ( tasks[loop]->task, 0 );
        }
      }
      time ( &now );
      now -= idle;
      tm = localtime ( &now );
      ptr = (char *) malloc ( 500 );
      sprintf ( ptr, "%s\n%d %s (%s %d:%02d)\n\n",
        gettext("You have been idle for"),
        config_max_idle / 60, gettext("minutes"), gettext("since"),
        tm->tm_hour, tm->tm_min );
      strcat ( ptr, gettext("You may now:") );
      strcat ( ptr, "\n\n" );
      strcat ( ptr, gettext("Revert to back to before the idle") );
      strcat ( ptr, "\n\n" );
      strcat ( ptr, gettext("Continue timing, ignoring the idle") );
      strcat ( ptr, "\n\n" );
      strcat ( ptr, gettext("Resume timing from when the idle started") );
      idle_prompt_window = create_confirm_toplevel ( CONFIRM_WARNING,
        gettext("Idle Detect"), ptr,
        gettext("Revert"), gettext("Continue"), gettext("Resume"),
         idle_reset_callback, idle_cancel_callback, idle_resume_callback,
         NULL );
      free ( ptr );
    }
  }

  /* autosave ? (do not autosave if idle) */
  time ( &now );
  if ( modified_since_save &&
    ( now > ( last_save + config_autosave_interval ) ) &&
    config_autosave_enabled && ( idle_prompt_window == NULL ) ) {
    save_all ();
  }

  /* clear message area? */
  if ( lastMessageTime && ( now - lastMessageTime > messageDisplayTime ) ) {
    showMessage ( NULL ); /* clear msg area */
  }

  /* return TRUE to so this timeout happens again in 1 second */
  return ( TRUE );
}




void create_splash_window () {
  GtkWidget *table, *pixmap, *label;
  GdkPixmap *icon;
  GdkBitmap *mask;
  char msg[500];
  GtkStyle *style;

  splash_window = gtk_window_new ( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_wmclass ( GTK_WINDOW ( splash_window ), "GTimer", "gtimer" );
  gtk_window_set_title ( GTK_WINDOW ( splash_window ), "GTimer" );
  gtk_widget_set_usize ( splash_window, 450, 180 );
  gtk_window_position ( GTK_WINDOW ( splash_window ), GTK_WIN_POS_CENTER );
  gtk_widget_realize ( splash_window );
  gdk_window_set_decorations ( GTK_WIDGET ( splash_window )->window,
    GDK_DECOR_BORDER );

  table = gtk_table_new ( 2, 2, FALSE );
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 8);
  gtk_container_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add ( GTK_CONTAINER ( splash_window ), table );

  icon = gdk_pixmap_create_from_xpm_d (
    GTK_WIDGET ( splash_window )->window, &mask,
    &splash_window->style->white, splash_xpm );
  pixmap = gtk_pixmap_new ( icon, mask );
  gtk_misc_set_alignment (GTK_MISC (pixmap), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), pixmap, 0, 1, 0, 2,
    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show ( pixmap );

  style = gtk_style_new ();

  sprintf ( msg, "GTimer v%s", GTIMER_VERSION );
  label = gtk_label_new ( msg );
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1,
    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show ( label );

  sprintf ( msg, "%s\nGTK Version: %d.%d.%d\n",
    GTIMER_COPYRIGHT,
    gtk_major_version, gtk_minor_version, gtk_micro_version );
  label = gtk_label_new ( msg );
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2,
    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show ( label );

  gtk_widget_show ( table );
  gtk_widget_show ( splash_window );

  time ( &splash_until );
  splash_until += splash_seconds;
}


void create_main_window () {
  GtkWidget *vbox;
  GtkWidget *menu_bar, *toolbutton, *iconw, *table, *scroll, *handlebox;
  GdkPixmap *icon;
  GdkBitmap *mask;
  int loop;
  gint w;

  main_window = gtk_window_new ( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_wmclass( GTK_WINDOW ( main_window ), "GTimer", "gtimer" );
  gtk_signal_connect ( GTK_OBJECT ( main_window ), "delete_event",
    GTK_SIGNAL_FUNC ( exit_callback ), NULL );
  gtk_signal_connect ( GTK_OBJECT ( main_window ), "destroy",
    GTK_SIGNAL_FUNC ( exit_callback ), NULL );
  gtk_window_set_title (GTK_WINDOW (main_window), "GTimer" );
  gtk_widget_realize ( main_window );

  vbox = gtk_vbox_new ( FALSE, 0 );
  gtk_container_add ( GTK_CONTAINER ( main_window ), vbox );

  menu_bar = create_main_window_menu_bar();
  handlebox = gtk_handle_box_new ();
  gtk_container_add ( GTK_CONTAINER ( handlebox ), menu_bar );
  gtk_widget_show ( handlebox );

  gtk_box_pack_start ( GTK_BOX ( vbox ), handlebox, FALSE, FALSE, 0 );
  gtk_widget_show ( menu_bar );

  /* create toolbar */
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_append_space ( GTK_TOOLBAR ( toolbar ) );

  gtk_box_pack_start ( GTK_BOX ( vbox ), toolbar, FALSE, FALSE, 0 );
/*
  toolbar_handlebox = gtk_handle_box_new ();
  gtk_container_add ( GTK_CONTAINER ( toolbar_handlebox ), toolbar );
  gtk_widget_show ( toolbar_handlebox );

  gtk_box_pack_start ( GTK_BOX ( vbox ), toolbar_handlebox, FALSE, FALSE, 0 );
*/

  for ( loop = 0; main_toolbar[loop].label != NULL; loop++ ) {
    icon = gdk_pixmap_create_from_xpm_d (
      GTK_WIDGET ( main_window )->window, &mask,
      &main_window->style->white, main_toolbar[loop].icon_data );
    iconw = gtk_pixmap_new ( icon, mask );
    toolbutton = gtk_toolbar_append_item ( GTK_TOOLBAR ( toolbar ),
      gettext ( main_toolbar[loop].label ),
      gettext ( main_toolbar[loop].tooltip_index ), "Private",
      iconw, GTK_SIGNAL_FUNC ( main_toolbar[loop].callback ), NULL );
    main_toolbar[loop].widget = toolbutton;
    gtk_toolbar_append_space ( GTK_TOOLBAR ( toolbar ) );
    gtk_widget_show ( toolbutton );
  }

  if ( config_toolbar_enabled )
    gtk_widget_show ( toolbar );

  /* add in list here */
  if ( configGetAttributeInt ( CONFIG_MAIN_WINDOW_PROJECT_WIDTH, &w ) == 0 )
    task_list_columns[0].width = w;
  if ( configGetAttributeInt ( CONFIG_MAIN_WINDOW_TASK_WIDTH, &w ) == 0 )
    task_list_columns[1].width = w;
  if ( configGetAttributeInt ( CONFIG_MAIN_WINDOW_TODAY_WIDTH, &w ) == 0 )
    task_list_columns[2].width = w;
  if ( configGetAttributeInt ( CONFIG_MAIN_WINDOW_TOTAL_WIDTH, &w ) == 0 )
    task_list_columns[3].width = w;
  task_list = create_list_column_def ( 4, task_list_columns );
  gtk_clist_set_selection_mode (GTK_CLIST (task_list), GTK_SELECTION_BROWSE);
  gtk_widget_set_usize (GTK_WIDGET (task_list), 350, 150);
#if OLD_GTK
  gtk_clist_set_policy (GTK_CLIST (task_list),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#endif
  gtk_signal_connect (GTK_OBJECT (task_list), "click_column",
    GTK_SIGNAL_FUNC (column_selected_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (task_list), "event",
    GTK_SIGNAL_FUNC (task_list_event), NULL);
  gtk_signal_connect_after (GTK_OBJECT (task_list), "select_row",
    GTK_SIGNAL_FUNC (task_selected_callback), NULL);

#if OLD_GTK
  gtk_box_pack_start ( GTK_BOX ( vbox ), task_list, TRUE, TRUE, 0 );
#else
  scroll = gtk_scrolled_window_new ( NULL, NULL );
  gtk_widget_set_usize (GTK_WIDGET (scroll), 400, 150);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_container_add (GTK_CONTAINER ( scroll ), task_list );
  gtk_box_pack_start ( GTK_BOX ( vbox ), scroll, TRUE, TRUE, 0 );
  gtk_widget_show ( scroll );
#endif
  gtk_widget_show ( task_list );

  /* add a status area and a place for the total time for today */
  table = gtk_table_new ( 10, 1, FALSE );
  gtk_box_pack_start ( GTK_BOX ( vbox ), table, FALSE, TRUE, 2 );

  status = gtk_statusbar_new ();
  gtk_table_attach_defaults ( GTK_TABLE (table), status, 0, 9, 0, 1 );
  gtk_widget_show (status);
  status_id = gtk_statusbar_get_context_id ( GTK_STATUSBAR ( status ),
    GTIMER_STATUS_ID );

  total_label = gtk_label_new ( "Total: 0:00:00" );
  gtk_table_attach_defaults ( GTK_TABLE (table), total_label, 9, 10, 0, 1 );
  gtk_label_set_justify ( GTK_LABEL ( total_label ), GTK_JUSTIFY_RIGHT );
  gtk_widget_show (total_label);

  gtk_widget_show (table);

  gtk_widget_show (vbox);
}



static void print_version () {
  /*int gtkmajor, gtkminor, gtkmicro;*/
  printf ( "GTimer v%s (%s)\n", GTIMER_VERSION, GTIMER_VERSION_DATE );
  /*
  gtkmajor = GTK_VERSION / 10000;
  gtkminor = ( GTK_VERSION / 100 ) % 100;
  gtkmicro = GTK_VERSION % 100;
  printf ( "%s %d.%d.%d\n", gettext ( "Compiled with GTK+" ),
    gtkmajor, gtkminor, gtkmicro );
  */
  printf ( "%s: %d.%d.%d\n",
    gettext ( "GTK+ runtime" ),
    gtk_major_version, gtk_minor_version, gtk_micro_version );
  printf ( "%s: %s\n", gettext ( "Home page" ), GTIMER_URL );
  printf ( "%s\n", GTIMER_COPYRIGHT );
}


static void print_help () {
  printf ( "GTimer %s:\n", gettext ( "options" ) );
  printf ( "%-20s %s\n", "-nosplash",
    gettext ( "don't display the splash screen" ) );
  printf ( "%-20s %s\n", "-help",
    gettext ( "display this help info" ) );
  printf ( "%-20s %s\n", "-version",
    gettext ( "display the version" ) );
  printf ( "%-20s %s\n", "-midnight N",
    gettext ( "specify the midnight offset" ) );
  printf ( "%-20s %s\n", "-start taskname",
    gettext ( "start timing the specified task" ) );
  printf ( "%-20s %s\n", "-weekstart N",
    gettext ( "use N as the first day of weeks" ) );
}



int main ( int argc, char *argv[] ) {
  GtkWidget *indicator_menu;
  GtkWidget *menu_item;
  char *home = "";
#ifndef WIN32
  uid_t uid;
  struct passwd *passwd;
#endif
  time_t now;
  struct tm *tm;
  int loop, loop2, offset, lastTaskNumber;
  char *ptr, *ptr2;
  struct stat buf;
  int display_splash = 1;
  int resume = 0; /* start time tasks from last exit */
  GtkWidget *win;
  int w = 0, h = 0;
#ifdef HAVE_LIBINTL_H
#ifdef WIN32
  char *localedir;
#endif
#endif
  char *matches[100];
  int nmatches = 0, found;
  TaskData *td;
  char msg[128];

#if PV_DEBUG
  g_message ( "PV: DEBUG version");
#endif

  if ( getenv ( "HOME" ) ) {
    home = getenv ( "HOME" );
  }
#ifndef WIN32
  else {
    uid = getuid ();
    passwd = getpwuid ( uid );
    if ( passwd )
      home = passwd->pw_dir;
  }
#endif

  taskdir = (char *) malloc ( strlen ( home ) +
    strlen ( TASK_DIRECTORY ) + 10 );
  sprintf ( taskdir, "%s/%s", home, TASK_DIRECTORY );
#ifdef WIN32
  convert_backslash ( taskdir );
#endif

  if ( stat ( taskdir, &buf ) != 0 ) {
    /* check for ".tasktimer" directory for backwards compatiblity */
    sprintf ( taskdir, "%s/%s", home, ".tasktimer" );
    /* Note: in 2.0 it seems to be good idea to create new command-line option '--config' */
#ifdef WIN32
    convert_backslash ( taskdir );
#endif
    if ( stat ( taskdir, &buf ) != 0 ) {
      sprintf ( taskdir, "%s/%s", home, TASK_DIRECTORY );
#ifdef WIN32
      convert_backslash ( taskdir );
      if ( _mkdir ( taskdir ) ) {
#else
      if ( mkdir ( taskdir, 0777 ) ) {
#endif
        fprintf ( stderr, "Error: unable to create directory %s\n", taskdir );
        exit ( 1 );
      }
    }
  }

#ifdef HAVE_LIBINTL_H
  /* internationalization stuff */
  setlocale ( LC_MESSAGES, "" );
#ifdef WIN32
  localedir = (char *) malloc ( strlen ( taskdir ) + 8 );
  sprintf ( localedir, "%s/locale", taskdir );
  convert_backslash(localedir);
  bindtextdomain ( DEFAULT_TEXT_DOMAIN, localedir );
  free ( localedir );
#else
  bindtextdomain(DEFAULT_TEXT_DOMAIN, LOCALEDIR);
#endif
//  textdomain ( "gtimer" );
#endif
  bind_textdomain_codeset(DEFAULT_TEXT_DOMAIN, "UTF-8");

  /* Init GTK */
  gtk_init ( &argc, &argv );
#if OLD_GTK
#else
  gtkrc = (char *) malloc ( strlen ( taskdir ) +
    strlen ( "gtkrc" ) + 2 );
  sprintf ( gtkrc, "%s/%s", taskdir, "gtkrc" );
  gtk_rc_parse ( gtkrc );
#endif

  /* Examine command line args */
  for ( loop = 1; loop < argc; loop++ ) {
    if ( strcmp ( argv[loop], "-dir" ) == 0 ) {
      if ( ! argv[loop+1] ) {
        fprintf ( stderr, "%s: -dir %s.\n",
          gettext("Error"), gettext("requires an argument") );
        exit ( 1 );
      }
      taskdir = argv[++loop];
#ifdef HAVE_LIBINTL_H
#ifdef WIN32
      localedir = (char *) malloc ( strlen ( taskdir ) + 8 );
      sprintf ( localedir, "%s/locale", taskdir );
      convertbackslash(localedir);
      bindtextdomain ( "gtimer", localedir );
      free ( localedir );
#endif
#endif
#if OLD_GTK
#else
      if ( gtkrc )
        free ( gtkrc );
      gtkrc = (char *) malloc ( strlen ( taskdir ) +
        strlen ( "gtkrc" ) + 2 );
      sprintf ( gtkrc, "%s/%s", taskdir, "gtkrc" );
      gtk_rc_parse ( gtkrc );
#endif
    } else if ( strcmp ( argv[loop], "-nosplash" ) == 0 ) {
      display_splash = 0;
    } else if ( strcmp ( argv[loop], "-resume" ) == 0 ) {
      resume = 1;
    } else if ( strcmp ( argv[loop], "-midnight" ) == 0 ) {
      if ( ! argv[loop+1] ) {
        fprintf ( stderr, "%s: -midnight %s.\n",
          gettext("Error"), gettext("requires an argument") );
        exit ( 1 );
      }
      for ( ptr = argv[++loop]; *ptr != '\0'; ptr++ ) {
        if ( ! isdigit ( *ptr ) && *ptr != '-' ) {
          fprintf ( stderr, "%s: -midnight %s (%s %s)\n",
            gettext("Error"), gettext("requires a number"),
            gettext("not"), argv[loop] );
          exit ( 1 );
        }
      }
      ptr = argv[loop];
      if ( *ptr == '-' )
        offset = atoi ( ptr + 1 );
      else
        offset = atoi ( ptr );
      if ( offset > 2359 ) {
        fprintf ( stderr, "%s -midnight: %s\n",
          gettext("Invalid offset for"), argv[loop] );
        fprintf ( stderr, "%s HHMM (<2359)\n",
          gettext("Format should be") );
        exit ( 1 );
      }
      config_midnight_offset = ( offset / 100 * 3600 ) + ( offset % 100 * 60 );
      if ( *ptr == '-' )
        config_midnight_offset *= -1;
    } else if ( strcmp ( argv[loop], "-weekstart" ) == 0 ) {
      if ( ! argv[loop+1] ) {
        fprintf ( stderr, "%s: -weekstart %s.\n",
          gettext("Error"), gettext("requires an argument") );
        exit ( 1 );
      }
      for ( ptr = argv[++loop]; *ptr != '\0'; ptr++ ) {
        if ( ! isdigit ( *ptr ) ) {
          fprintf ( stderr, "%s: -weekstart %s (%s %s)\n",
            gettext("Error"), gettext("requires a number"),
            gettext("not"), argv[loop] );
          exit ( 1 );
        }
      }
      config_start_of_week = atoi ( argv[loop] );
      if ( config_start_of_week > 6 ) {
        fprintf ( stderr, "%s -weekstart: %s\n",
          gettext("Invalid day of week number for"), argv[loop] );
        exit ( 1 );
      }
    } else if ( strcmp ( argv[loop], "-start" ) == 0 ) {
      if ( nmatches < 99 )
        matches[nmatches++] = argv[++loop];
    } else if ( strcmp ( argv[loop], "-v" ) == 0 ||
      strcmp ( argv[loop], "-version" ) == 0 ||
      strcmp ( argv[loop], "--version" ) == 0 ) {
      print_version ();
      exit ( 0 );
    } else if ( strcmp ( argv[loop], "-h" ) == 0 ||
      strcmp ( argv[loop], "-help" ) == 0 ||
      strcmp ( argv[loop], "--help" ) == 0 ) {
      print_version ();
      print_help ();
      exit ( 0 );
    } else {
      fprintf ( stderr, "%s: %s\n",
        gettext("Ingoring unknown option"), argv[loop] );
    }
  }

  /* read config values */
  config_file = (char *) malloc ( strlen ( taskdir ) +
    strlen ( CONFIG_DEFAULT_FILE ) + 2 );
  sprintf ( config_file, "%s/%s", taskdir, CONFIG_DEFAULT_FILE );
  configReadAttributes ( config_file );

  /* Get the toolbar setting */
  configGetAttributeInt ( CONFIG_TOOLBAR_STATUS, &config_toolbar_enabled );

  /* Get the animate setting */
  configGetAttributeInt ( CONFIG_ANIMATE, &config_animate_enabled );

  /* Get the autosave setting */
  configGetAttributeInt ( CONFIG_AUTOSAVE, &config_autosave_enabled );

  /* Get the idle delay */
  if ( configGetAttributeInt ( CONFIG_IDLE_ON, &config_idle_enabled ) < 0 )
    config_idle_enabled = 1;
  if ( configGetAttributeInt ( CONFIG_IDLE, &config_max_idle ) < 0 )
    config_max_idle = 15 * 60; /* default */

  /* PV: Set menu check boxes according to config */
  if ( config_toolbar_enabled )
    MM_ToggleEntries[MM_TOGGLE_TOOLBAR].is_active = TRUE;

  if ( config_animate_enabled )
    MM_ToggleEntries[MM_TOGGLE_ANIMATE].is_active = TRUE;

   if ( config_autosave_enabled )
    MM_ToggleEntries[MM_TOGGLE_AUTOSAVE].is_active = TRUE;

   if ( config_idle_enabled )
    MM_ToggleEntries[MM_TOGGLE_IDLE].is_active = TRUE;

  /* in the future check version number and pop up license and/or
  ** release notes if a new version
  */
  configSetAttribute ( CONFIG_VERSION, GTIMER_VERSION );

  /* load all projects */
  projectLoadAll ( taskdir );

  /* load all tasks */
  taskLoadAll ( taskdir );

  /* Create splash window */
  if ( display_splash )
    create_splash_window ();

  /* Create window */
  create_main_window ();

  /* move main window */
  if ( configGetAttributeInt ( CONFIG_MAIN_WINDOW_WIDTH, &w ) == 0 &&
    configGetAttributeInt ( CONFIG_MAIN_WINDOW_HEIGHT, &h ) == 0 ) {
    /* wait until after gtk_widget_show() to resize */
  }

  if ( ! splash_window ) {
    gtk_widget_show ( main_window );
    if ( w )
      gdk_window_resize ( GTK_WIDGET ( main_window )->window, w, h );
  }

  /* set application icon */
  win = splash_window ? splash_window : main_window;
  appicon = gdk_pixmap_create_from_xpm_d (
    GTK_WIDGET ( win )->window, &appicon_mask,
    &win->style->white, gtimer_xpm);
  appicon2 = gdk_pixmap_create_from_xpm_d (
    GTK_WIDGET ( win )->window, &appicon2_mask,
    &win->style->white, gtimer2_xpm);
  if ( ! splash_window )
    gdk_window_set_icon ( GTK_WIDGET ( main_window )->window,
      NULL, num_timing ? appicon : appicon2,
      num_timing ? appicon_mask : appicon2_mask );

  /* set the current date */
  time ( &now );
  now -= config_midnight_offset;
  tm = localtime ( &now );
  today_year = tm->tm_year + 1900;
  today_mon = tm->tm_mon + 1;
  today_mday = tm->tm_mday;

#ifdef HAVE_APP_INDICATOR
  indicator_menu = gtk_menu_new();
  indicator = app_indicator_new ("gtimer_indicator", "/usr/share/pixmaps/gtimer.xpm", APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
  app_indicator_set_status (indicator, APP_INDICATOR_STATUS_ACTIVE);
  app_indicator_set_menu (indicator, GTK_MENU (indicator_menu));
#endif

  menu_item = gtk_menu_item_new_with_label ("Start current task");
  gtk_menu_shell_append (GTK_MENU_SHELL (indicator_menu), menu_item);
  g_signal_connect_swapped (menu_item, "activate",
                      G_CALLBACK (start_callback),
                                  (gpointer) g_strdup ("Start current task"));
  gtk_widget_show (menu_item);

  menu_item = gtk_menu_item_new_with_label ("Stop all tasks");
  gtk_menu_shell_append (GTK_MENU_SHELL (indicator_menu), menu_item);
  g_signal_connect_swapped (menu_item, "activate",
                      G_CALLBACK (stop_all_callback),
                                  (gpointer) g_strdup ("Stop all tasks"));
  gtk_widget_show (menu_item);

  /* build and update the task list */
  build_list ();
  update_list ();
  update_toolbar_buttons ();

  /* sort list like it was last time */
  if ( configGetAttribute ( CONFIG_SORT, &ptr ) == 0 ) {
    last_sort = atoi ( ptr );
    if ( configGetAttribute ( CONFIG_SORT_FORWARD, &ptr ) == 0 )
      sort_forward = ! atoi ( ptr );
    else
      sort_forward = 0;
    column_selected_callback ( NULL, last_sort );
  } else {
    sort_forward = 0;
    column_selected_callback ( NULL, 0 );
  }

  /* handle tasks specified with -start */
  for ( loop = 0; ! resume && loop < nmatches; loop++ ) {
    found = 0;
    for ( loop2 = 0; loop2 < num_visible_tasks && ! found; loop2++ ) {
      td =  visible_tasks[loop2];
      if ( strcmp ( td->task->name, matches[loop] ) == 0 ) {
        found = 1;
        num_timing++;
        td->timer_on = 1;
        time ( &td->on_since );
        if ( td->todays_entry == NULL )
          td->todays_entry = taskNewTimeEntry ( td->task, today_year,
            today_mon, today_mday );
        /* select the task */
        gtk_clist_select_row ( GTK_CLIST ( task_list ), loop2, 0 );
        /* make task visible */
        move_to_task = loop2;
        if ( ! splash_window )
          gtk_clist_moveto ( GTK_CLIST ( task_list ), loop2, -1, 0.5, 0 );
      }
    }
    if ( ! found ) {
      fprintf ( stderr, "%s \"%s\" %s.\n",
        gettext ( "Task" ), matches[loop], gettext ( "not found" ) );
    }
  }

  if ( resume &&
    ( configGetAttribute ( CONFIG_LAST_TIMED_TASKS, &ptr ) == 0 ) ) {
    ptr = strdup ( ptr );
    for ( ptr2 = strtok ( ptr, "," ); ptr2 != NULL;
      ptr2 = strtok ( NULL, "," ) ) {
      lastTaskNumber = atoi ( ptr2 );
      for ( loop = 0; loop < num_visible_tasks; loop++ ) {
        td =  visible_tasks[loop];
        if ( td->task->number == lastTaskNumber ) {
          num_timing++;
          td->timer_on = 1;
          time ( &td->on_since );
          if ( td->todays_entry == NULL )
            td->todays_entry = taskNewTimeEntry ( td->task, today_year,
              today_mon, today_mday );
          /* select the task */
          gtk_clist_select_row ( GTK_CLIST ( task_list ), loop, 0 );
          /* make task visible */
          move_to_task = loop2;
          if ( ! splash_window )
            gtk_clist_moveto ( GTK_CLIST ( task_list ), loop, -1, 0.5, 0 );
          break;
        }
      }
    }
    free ( ptr );
  }

  /* Add a timeout to update the display once a second */
  gtk_timeout_add ( 1000, timeout_handler, NULL );

  /* Add a timeout to check for a new version in 30 seconds
     (We will just check every 30 seconds to see if we should check */
  gtk_timeout_add ( 30 * 1000, version_timeout_handler, NULL );

  /* record time for use with autosave */
  time ( &last_save );
  modified_since_save = 0;

  /* set x error handler... */
#ifdef WIN32
  set_x_error_handler ();
#endif

  sprintf ( msg, "%s GTimer %s", gettext("Welcome to"),
     GTIMER_VERSION );
  showMessage ( msg );

  /* Loop endlessly */
  gtk_main ();

#ifdef GTIMER_MEMDEBUG
  /* memory debugging... make sure md_print_all gets linked in so we can
  ** call it from gdb.
  */
  for ( loop = 0; loop < num_tasks; loop++ ) {
    taskFree ( tasks[loop]->task );
    free ( tasks[loop] );
  }
  free ( tasks );
  free ( visible_tasks );
  md_print_all ();
#endif

  return ( 0 );
}


/*
 * Update the message area at the bottom of the screen.
 * If msg == NULL, the area will be cleared.
 * The message will be cleared automatically after a while.
 */
void showMessage ( msg )
char *msg;
{
  gtk_statusbar_push ( GTK_STATUSBAR ( status ), status_id, msg ? msg : "" );
  if ( msg == NULL )
    lastMessageTime = 0;
  else
    time ( &lastMessageTime );
}



/*
** Determine the path to an executable using the $PATH environment
** variable.
** Return value should be freed (unless it's NULL!)
*/
char *get_client_path ( char *file )
{
  char *path;
  static char *path2 = "/usr/bin/X11:/usr/local/bin:/usr/bin:/opt/local/bin";
  char *env;
  struct stat buf;
  char *ret, *ptr;

  env = getenv ( "PATH" );
  if ( env ) {
    path = (char *) malloc ( strlen ( env ) + strlen ( path2 ) + 2 );
    sprintf ( path, "%s:%s", env, path2 );
  } else {
    path = (char *) malloc ( strlen ( path2 ) + 1 );
    strcpy ( path, path2 );
  }

  ptr = strtok ( path, ":" );
  while ( ptr ) {
    ret = (char *) malloc ( strlen ( ptr ) + strlen ( file ) + 2 );
    sprintf ( ret, "%s/%s", ptr, file );
    if ( stat ( ret, &buf ) == 0 ) {
      /* found it */
      free ( path );
      return ( ret );
    }
    ptr = strtok ( NULL, ":" );
  }

  /* not found */
  free ( path );
  return ( NULL );
}
