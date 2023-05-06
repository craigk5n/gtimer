/*
 * Definition for a project
 *
 * Copyright:
 *	(C) 2003 Craig Knudsen, craig@k5n.us
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
 */


#ifndef _PROJECT_H
#define _PROJECT_H

#define PROJECT_DIRECTORY		".gtimer"	/* from $HOME */

/* Errors */
#define PROJECT_ERROR_SYSTEM_ERROR	1	/* check errno value */
#define PROJECT_ERROR_BAD_FILE	2	/* bad file format */

typedef struct {
  char *name;			/* name of project */
  time_t created;		/* time created */
  int number;			/* unique project id number */
  unsigned int options;		/* app-defined bit-or options */
} Project;

/*
 * Functions
 */


void projectAdd ( Project *project );
int projectSave ( Project *project, char *projectdir );
int projectSaveAll ( char *projectdir );
int projectLoad ( char *file, Project **project );
int projectLoadAll ( char *projectdir );
Project *projectCreate ( char *name );
int projectDelete ( Project *project, char *projectdir );
void projectFree ();
int projectCount ();
Project *projectGet ( int number );
Project *projectGetFirst ();
Project *projectGetNext ();
unsigned int projectOptions ( Project *project );
unsigned int projectOptionEnabled ( Project *project, unsigned int option );
void projectSetOption ( Project *project, unsigned int option );
void projectUnsetOption ( Project *project, unsigned int option );
char *projectErrorString ( int project_error );

#endif /* _PROJECT_H */
