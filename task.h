/*
 * Definition for a task
 *
 * Copyright:
 *	(C) 1999-2023 Craig Knudsen, craig@k5n.us
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
 *	17-Apr-2005	Add support for subtracting a particular offset
 *			off of timers.  (Russ Allbery)
 */


#ifndef _TASK_H
#define _TASK_H

#define TASK_DIRECTORY		".gtimer"	/* from $HOME */

/* Errors */
#define TASK_ERROR_SYSTEM_ERROR	1	/* check errno value */
#define TASK_ERROR_BAD_FILE	2	/* bad file format */

typedef struct {
  int seconds;		/* time in seconds */
  int mon, mday, year;	/* MM/DD/YYYY */
  int marked_seconds;	/* time in seconds - used by taskMark() */
} TaskTimeEntry;

typedef struct {
  char *text;		/* text of annotiation */
  time_t text_time;	/* GMT of annotation */
} TaskAnnotation;

typedef struct {
  char *name;			/* name of task */
  TaskTimeEntry **entries;	/* entries */
  int num_entries;		/* number entries (dates) */
  time_t created;		/* time created */
  int number;			/* unique task id number */
  int project_id;		/* id of parent project (-1=no project) */
  unsigned int options;		/* app-defined bit-or options */
  TaskAnnotation **annotations;	/* annotations */
  int num_annotations;		/* size of above array */
} Task;

/*
 * Functions
 */


void taskAdd ( Task *task );
int taskSave ( Task *task, char *taskdir );
int taskSaveAll ( char *taskdir );
/* rra 2005-04-17 - add an offset to subtract from running times */
void taskMark ( Task *task, int offset );
void taskMarkAll ( int offset );
void taskRestore ( Task *task );
void taskRestoreAll ();
void taskClearAll ();
int taskLoad ( char *file, Task **task );
int taskLoadAll ( char *taskdir );
Task *taskCreate ( char *name );
int taskDelete ( Task *task, char *taskdir );
void taskFree ();
int taskCount ();
Task *taskGet ( int number );
Task *taskGetFirst ();
Task *taskGetNext ();
TaskTimeEntry *taskGetTimeEntry ( Task *task, int year, int month, int day );
TaskTimeEntry *taskNewTimeEntry ( Task *task, int year, int month, int day );
unsigned int taskOptions ( Task *task );
unsigned int taskOptionEnabled ( Task *task, unsigned int option );
void taskSetOption ( Task *task, unsigned int option );
void taskUnsetOption ( Task *task, unsigned int option );
void taskAddAnnotation ( Task *task, char *taskdir, char *text );
TaskAnnotation **TaskGetAnnotationEntries ( Task *task, int year,
  int month, int day, int time_offset, int *num_ret );
char *taskErrorString ( int task_error );

#endif /* _TASK_H */
