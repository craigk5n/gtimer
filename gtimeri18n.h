/*
 * GTimer
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
 * Description:
 *	Helps you keep track of time spent on different tasks.
 *
 * Author:
 *	Craig Knudsen, craig@k5n.us https://www.k5n.us/gtimer/
 *
 * Home Page:
 *	https://www.k5n.us/gtimer/
 *
 * History:
 *	2008-08-31	Header file created
 */


#ifndef _GTIMER_I18N_H
#define _GTIMER_I18N_H


// PV: Internationalization
#ifdef HAVE_LIBINTL_H
#  ifndef ENABLE_NLS
#    define ENABLE_NLS	1
#  endif
#  define DEFAULT_TEXT_DOMAIN	"gtimer"
//#  ifndef WIN32
//#    define LOCALE_DIR	"/usr/share/locale"
//#warning: FIX if locale directory is different from /usr/share/locale
//#  endif
#  include "gettext.h"
#  include <locale.h>
#else
#  define gettext(a)	a
#  define gettext_noop(a)	a
#endif

#endif   /* _GTIMER_I18N_H */
