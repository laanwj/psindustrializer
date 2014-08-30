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

#ifndef _PSI_CALLBACKS
#define _PSI_CALLBACKS

#include <gtk/gtk.h>

gboolean
on_AppWindow_delete_event              (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_AppWindow_destroy                   (GtkObject       *object,
                                	gpointer         user_data);

void
on_height_spinbutton_changed           (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_circum_spinbutton_changed           (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_tension_hscale_changed              (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_speed_hscale_focus_out_event        (GtkAdjustment   *adj,
                                        gpointer         user_data);

void
on_damping_hscale_focus_out_event      (GtkAdjustment   *adj,
                                        gpointer         user_data);

void
on_velocity_hscale_focus_out_event     (GtkAdjustment   *adj,
                                        gpointer         user_data);

void
on_length_hscale_focus_out_event       (GtkAdjustment   *adj,
                                        gpointer         user_data);

void
on_actuation_comboentry_changed        (GtkComboBox	*combobox,
                                        gpointer         user_data);

void
on_play_clicked                        (GtkButton       *button,
                                        gpointer         user_data);

void
on_save_clicked                        (GtkButton       *button,
                                        gpointer         user_data);

void
on_close_clicked                       (GtkButton       *button,
                                        gpointer         user_data);

void
on_about_clicked                       (GtkButton       *button,
                                        gpointer         user_data);

void
on_cancel_button_clicked               (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_about_ok_clicked                    (GtkWidget       *widget,
                                        gpointer         user_data);

GtkWidget*
opengl_create (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2);

void
on_length_spinbutton_changed           (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_topologynotebook_switch_page        (GtkNotebook     *notebook,
                                        GtkNotebookPage *page,
                                        gint             page_num,
                                        gpointer         user_data);

GtkWidget*
tension_hscale_create (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2);

void
on_plane_length_spinbutton_changed     (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_plane_width_spinbutton_changed      (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_error_ok_button_clicked             (GtkWidget       *widget,
                                        gpointer         user_data);

void
save_ins_clicked		       (GtkWidget       *widget,
                                        gpointer         user_data);
void
load_ins_clicked		       (GtkWidget       *widget,
                                        gpointer         user_data);
#endif
