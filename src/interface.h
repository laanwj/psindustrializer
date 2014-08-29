/*  Power Station Industrializer
 *  Copyright (c) 2000 David A. Bartold
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

#ifndef _PSI_INTERFACE
#define _PSI_INTERFACE

GtkWidget*		gui_create_AppWindow (void);
GtkWidget* 		gui_create_FileSelector (void(*callback)(), const gchar* title, const gchar *path);
void			gui_error_msg (gchar *msg);
GtkWidget*		gui_ok_cancel_dialog (GtkWidget *dialog, const gchar *title, const gchar *msg,
					      GCallback handler, gpointer data, gboolean default_button);

inline gboolean		gui_decay_is_used (void);
inline gfloat		gui_get_decay (void);
void			gui_set_size_label (gfloat value);

void			gui_set_sensitive(gboolean sens);

inline void		gui_set_topology (gint obj_type);
inline void		gui_configure_tube (gint height, gint circum);
inline void		gui_configure_rod (gint length);
inline void		gui_configure_plane (gint length, gint width);
inline void 		gui_set_values (gdouble tension, gdouble speed, gdouble damping,
				    gint actuation, gdouble velocity);

#endif
