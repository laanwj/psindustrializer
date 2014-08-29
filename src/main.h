/*  Power Station Industrializer
 *  Copyright (c) 2000 David A. Bartold
 *  Copyright (c) 2003 Yury Aliaev
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _PSI_MAIN
#define _PSI_MAIN

#include <glib.h>

#ifndef _
#if defined(ENABLE_NLS)
#  include <libintl.h>
#  define _(x) gettext(x)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define N_(String) (String)
#  define _(x) (x)
#  define gettext(x) (x)
#endif
#endif

typedef struct _drv
{
    const char *description;
    int (*open)(void);
    int (*play)(gint16*, int);
    void (*close)(void);
    const char* (*err)(int);
} drv;

drv		*driver;

/* global configuration variables */
gboolean	conf_autoext, conf_overwrite_warning;
gchar		*conf_instr_path, *conf_sample_path;

inline guint				psi_get_current_driver	(void);
void					psi_set_driver		(guint driver);

#endif
