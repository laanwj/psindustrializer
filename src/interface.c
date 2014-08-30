/*  Power Station Industrializer
 *  Copyright (c) 2000 David A. Bartold
 *  Copyrigth (c) 2005 Yury Aliaev
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "callbacks.h"
#include "interface.h"
#include "main.h"

GtkWidget *status_label, *progressbar1;
GSList *driver_list;

static GtkWidget *check_decay, *size_label;
static GtkObject *adj_decay;
static GtkWidget *topologynotebook, *height_spinbutton, *circum_spinbutton, *length_spinbutton,
		 *plane_length_spinbutton, *plane_width_spinbutton;
static GtkWidget *speed_hscale, *damping_hscale, *actuation_combo, *velocity_hscale, *tension_hscale;
static GtkWidget *render, *play, *save;

static guint	preselected_driver;
static gboolean	autocorrect_ext, overwarning;

void gui_set_sensitive(gboolean sens)
{
    gtk_widget_set_sensitive(play, sens);
    gtk_widget_set_sensitive(save, sens);
}


inline void gui_set_topology (gint obj_type)
{
    gtk_notebook_set_current_page(GTK_NOTEBOOK(topologynotebook), obj_type);
}

inline void gui_configure_tube (gint height, gint circum)
{
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(height_spinbutton), (gfloat)height);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(circum_spinbutton), (gfloat)circum);
}

inline void gui_configure_rod (gint length)
{
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(length_spinbutton), (gfloat)length);
}

inline void gui_configure_plane (gint length, gint width)
{
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(plane_length_spinbutton), (gfloat)length);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(plane_width_spinbutton), (gfloat)width);
}

inline void gui_set_values (gdouble tension, gdouble speed, gdouble damping, gint actuation, gdouble velocity)
{
    gtk_range_set_value(GTK_RANGE(tension_hscale), tension);
    gtk_range_set_value(GTK_RANGE(speed_hscale), speed);
    gtk_range_set_value(GTK_RANGE(damping_hscale), damping);
    gtk_range_set_value(GTK_RANGE(velocity_hscale), velocity);

    gtk_combo_box_set_active(GTK_COMBO_BOX(actuation_combo), actuation);
}

void gui_set_size_label (gfloat value)
{
    static gchar *size_str;

    size_str = g_strdup_printf(_("Real length: %.1f s"), value);
    gtk_label_set_text(GTK_LABEL(size_label), size_str);
    g_free(size_str);
}

inline gboolean gui_decay_is_used(void)
{
    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_decay));
}

inline gfloat gui_get_decay(void)
{
    return GTK_ADJUSTMENT(adj_decay)->value;
}

static inline
GtkWidget* dialog_add_stock_button(GtkDialog* dialog, const gchar* stock_id, gint response)
{
    GtkWidget *thing;

    thing = gtk_button_new_from_stock(stock_id);
    gtk_dialog_add_action_widget(dialog, thing, response);
    gtk_widget_show(thing);
    
    return thing;
}

static void hang_tooltip(GtkWidget * widget, const gchar * text)
{
    static GtkTooltips *tips = NULL;

    if (!tips) {
	tips = gtk_tooltips_new();
    }
    gtk_tooltips_set_tip(tips, widget, text, NULL);
}

static void use_decay_toggled(GtkToggleButton * button, GtkWidget * target)
{
    gtk_widget_set_sensitive(target, gtk_toggle_button_get_active(button));
}

static void setup_clicked(GtkWidget * setup_win, gint response, gpointer data)
{
    if (response == GTK_RESPONSE_OK) {
	psi_set_driver(preselected_driver);
	conf_autoext = autocorrect_ext;
	conf_overwrite_warning = overwarning;
    }
    
    gtk_widget_hide(setup_win);
}

static void
driver_selected(GtkComboBox * combobox, gpointer data)
{
    preselected_driver = gtk_combo_box_get_active(combobox);
}

static void descr_fill(drv * ldriver, GtkWidget *combo)
{
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo), ldriver->description);
}

static void checkbutton_changed(GtkToggleButton * tbutton, gboolean *value)
{
    *value = gtk_toggle_button_get_active(tbutton);
}

static void setup_dialog(void)
{
    static GtkWidget *combo, *check_ext, *check_overwrite;
    static GtkWidget *setup_window = NULL;

    if(setup_window && GTK_IS_WIDGET(setup_window)) {
	if(!GTK_WIDGET_VISIBLE(setup_window)) {
	    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), psi_get_current_driver());
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_ext), conf_autoext);
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_overwrite), conf_overwrite_warning);
	    gtk_widget_show(setup_window);
	}
    } else {
	setup_window = gtk_dialog_new();
	g_signal_connect(setup_window, "response",
		         G_CALLBACK(setup_clicked), NULL);

	gtk_window_set_title(GTK_WINDOW(setup_window), _("Industrializer configuration"));
	gtk_window_set_policy(GTK_WINDOW(setup_window), FALSE, FALSE, FALSE);

	dialog_add_stock_button(GTK_DIALOG(setup_window), GTK_STOCK_OK, GTK_RESPONSE_OK);
	dialog_add_stock_button(GTK_DIALOG(setup_window), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

	combo = gtk_combo_box_entry_new_text();
	g_slist_foreach(driver_list, (GFunc) descr_fill, combo);
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), psi_get_current_driver());

	gtk_entry_set_editable(GTK_ENTRY(GTK_BIN(combo)->child), FALSE);
	
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(setup_window)->vbox), combo,
		       TRUE, TRUE, 0);
	gtk_widget_show(combo);
	g_signal_connect(combo, "changed",
			 G_CALLBACK(driver_selected),
			 NULL);

	check_ext = gtk_check_button_new_with_label(_("Add extension to files without it"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_ext),
				     autocorrect_ext = conf_autoext); /* innocent C hack ;-) */
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(setup_window)->vbox), check_ext,
		       TRUE, TRUE, 0);
	gtk_widget_show(check_ext);
	g_signal_connect(check_ext, "toggled",
			 G_CALLBACK(checkbutton_changed),
			 &autocorrect_ext);

	check_overwrite = gtk_check_button_new_with_label(_("File overwrite warning"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_overwrite),
				     overwarning = conf_overwrite_warning);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(setup_window)->vbox), check_overwrite,
		       TRUE, TRUE, 0);
	gtk_widget_show(check_overwrite);
	g_signal_connect(check_overwrite, "toggled",
			 G_CALLBACK(checkbutton_changed),
			 &overwarning);

	gtk_widget_show_all(setup_window);
    }
}

static void
dialog_clicked (GtkWidget *dialog)
{
    gtk_widget_hide(dialog);
}

static void
create_AboutWindow (void)
{
    static GtkWidget *AboutWindow = NULL;
    GtkWidget *thing;
    GtkWidget *vbox7;
    GtkWidget *about_ok;

    if(AboutWindow && GTK_IS_WIDGET(AboutWindow)) {
	if(!GTK_WIDGET_VISIBLE(AboutWindow))
	    gtk_widget_show(AboutWindow);
    } else {
	AboutWindow = gtk_dialog_new();
	g_signal_connect(AboutWindow, "response",
		         G_CALLBACK(dialog_clicked), NULL);

	gtk_window_set_title(GTK_WINDOW(AboutWindow),
			     _("About Industrializer"));
	gtk_window_set_modal(GTK_WINDOW(AboutWindow), TRUE);
	gtk_window_set_policy(GTK_WINDOW(AboutWindow), FALSE, FALSE, FALSE);

	vbox7 = GTK_DIALOG(AboutWindow)->vbox;

	thing = gtk_image_new_from_file(PREFIX"share/"PACKAGE"/power_station_logo.xpm");
	gtk_widget_show (thing);
	gtk_box_pack_start(GTK_BOX(vbox7), thing, FALSE, FALSE, 0);

	thing =
	    gtk_label_new(_("Power Station Industrializer\nA Percussion Synthesizer\n\nCopyright (c) 2000\nDavid A. Bartold\n\nLicensed under the\nGNU GPL v2"));
	gtk_widget_show(thing);
	gtk_box_pack_start(GTK_BOX(vbox7), thing, FALSE, FALSE, 0);

	about_ok = dialog_add_stock_button(GTK_DIALOG(AboutWindow), GTK_STOCK_OK, 0);/* Responce means nothing here */
	gtk_widget_grab_focus(about_ok);
    
	gtk_widget_show(AboutWindow);
    }
}

GtkWidget *gui_create_AppWindow(void)
{
    GtkWidget *AppWindow;
    GtkWidget *vbox3;
    GtkWidget *hbox3;
    GtkWidget *vbox6;
    GtkWidget *table, *table1;
    GtkWidget *label2;
    GtkWidget *label1;
    GtkObject *height_spinbutton_adj;
    GtkObject *circum_spinbutton_adj;
    GtkWidget *tubelabel;
    GtkWidget *table2;
    GtkWidget *label15;
    GtkObject *length_spinbutton_adj;
    GtkWidget *rodlabel;
    GtkWidget *table4;
    GtkWidget *label17;
    GtkWidget *label18;
    GtkObject *plane_length_spinbutton_adj;
    GtkObject *plane_width_spinbutton_adj;
    GtkWidget *planelabel;
    GtkWidget *table3;
    GtkWidget *label3;
    GtkWidget *label4;
    GtkWidget *label5;
    GtkWidget *label6;
    GtkWidget *label7;
    GtkWidget *label8;
    GtkWidget *length_hscale;
    GtkWidget *openglframe;
    GtkWidget *opengl;
    GtkWidget *about;
    GtkWidget *close;
    GtkWidget *frame1;
    GtkWidget *hbox4, *thing, *btn, *lvbox;
    GtkTooltips *tooltips;
    GtkObject *adj;
    gchar *size_str;
    GdkPixbuf *icon;

    tooltips = gtk_tooltips_new();

    AppWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(AppWindow),
			 _("Power Station Industrializer"));
    gtk_window_set_policy(GTK_WINDOW(AppWindow), FALSE, FALSE, FALSE);
    icon = gdk_pixbuf_new_from_file(PREFIX"share/pixmaps/"PACKAGE".png", NULL);
    gtk_window_set_icon(GTK_WINDOW(AppWindow), icon);

    vbox3 = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(vbox3);
    gtk_container_add(GTK_CONTAINER(AppWindow), vbox3);

    hbox3 = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox3);
    gtk_box_pack_start(GTK_BOX(vbox3), hbox3, TRUE, TRUE, 0);

    vbox6 = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(vbox6);
    gtk_box_pack_start(GTK_BOX(hbox3), vbox6, TRUE, TRUE, 0);

    topologynotebook = gtk_notebook_new();
    gtk_widget_show(topologynotebook);
    gtk_box_pack_start(GTK_BOX(vbox6), topologynotebook, TRUE, TRUE, 0);

    table1 = gtk_table_new(2, 2, TRUE);
    gtk_widget_show(table1);
    gtk_container_add(GTK_CONTAINER(topologynotebook), table1);
    gtk_container_set_border_width(GTK_CONTAINER(table1), 10);
    gtk_table_set_row_spacings(GTK_TABLE(table1), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table1), 10);

    label2 = gtk_label_new(_("Tube Circumference"));
    gtk_widget_show(label2);
    gtk_table_attach(GTK_TABLE(table1), label2, 0, 1, 1, 2,
		     (GtkAttachOptions) (0), (GtkAttachOptions) (0), 0, 0);

    label1 = gtk_label_new(_("Tube Height"));
    gtk_widget_show(label1);
    gtk_table_attach(GTK_TABLE(table1), label1, 0, 1, 0, 1,
		     (GtkAttachOptions) (0), (GtkAttachOptions) (0), 0, 0);

    height_spinbutton_adj = gtk_adjustment_new(10, 3, 30, 1, 10, 10);
    height_spinbutton =
	gtk_spin_button_new(GTK_ADJUSTMENT(height_spinbutton_adj), 1, 0);
    gtk_widget_show(height_spinbutton);
    gtk_table_attach(GTK_TABLE(table1), height_spinbutton, 1, 2, 0, 1,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);

    circum_spinbutton_adj = gtk_adjustment_new(5, 3, 30, 1, 10, 10);
    circum_spinbutton =
	gtk_spin_button_new(GTK_ADJUSTMENT(circum_spinbutton_adj), 1, 0);
    gtk_widget_show(circum_spinbutton);
    gtk_table_attach(GTK_TABLE(table1), circum_spinbutton, 1, 2, 1, 2,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);

    tubelabel = gtk_label_new(_("Tube"));
    gtk_widget_show(tubelabel);
    gtk_notebook_set_tab_label(GTK_NOTEBOOK(topologynotebook),
			       gtk_notebook_get_nth_page(GTK_NOTEBOOK
							 (topologynotebook),
							 0), tubelabel);

    table2 = gtk_table_new(1, 2, TRUE);
    gtk_widget_show(table2);
    gtk_container_add(GTK_CONTAINER(topologynotebook), table2);
    gtk_container_set_border_width(GTK_CONTAINER(table2), 10);
    gtk_table_set_row_spacings(GTK_TABLE(table2), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table2), 10);

    label15 = gtk_label_new(_("Rod Length"));
    gtk_widget_show(label15);
    gtk_table_attach(GTK_TABLE(table2), label15, 0, 1, 0, 1,
		     (GtkAttachOptions) (0), (GtkAttachOptions) (0), 0, 0);

    length_spinbutton_adj = gtk_adjustment_new(5, 3, 200, 1, 10, 10);
    length_spinbutton =
	gtk_spin_button_new(GTK_ADJUSTMENT(length_spinbutton_adj), 1, 0);
    gtk_widget_show(length_spinbutton);
    gtk_table_attach(GTK_TABLE(table2), length_spinbutton, 1, 2, 0, 1,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);

    rodlabel = gtk_label_new(_("Rod"));
    gtk_widget_show(rodlabel);
    gtk_notebook_set_tab_label(GTK_NOTEBOOK(topologynotebook),
			       gtk_notebook_get_nth_page(GTK_NOTEBOOK
							 (topologynotebook),
							 1), rodlabel);

    table4 = gtk_table_new(2, 2, TRUE);
    gtk_widget_show(table4);
    gtk_container_add(GTK_CONTAINER(topologynotebook), table4);
    gtk_container_set_border_width(GTK_CONTAINER(table4), 10);
    gtk_table_set_row_spacings(GTK_TABLE(table4), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table4), 10);

    label17 = gtk_label_new(_("Plane Length"));
    gtk_widget_show(label17);
    gtk_table_attach(GTK_TABLE(table4), label17, 0, 1, 0, 1,
		     (GtkAttachOptions) (0), (GtkAttachOptions) (0), 0, 0);

    label18 = gtk_label_new(_("Plane Width"));
    gtk_widget_show(label18);
    gtk_table_attach(GTK_TABLE(table4), label18, 0, 1, 1, 2,
		     (GtkAttachOptions) (0), (GtkAttachOptions) (0), 0, 0);

    plane_length_spinbutton_adj = gtk_adjustment_new(7, 3, 30, 1, 10, 10);
    plane_length_spinbutton =
	gtk_spin_button_new(GTK_ADJUSTMENT(plane_length_spinbutton_adj), 1,
			    0);
    gtk_widget_show(plane_length_spinbutton);
    gtk_table_attach(GTK_TABLE(table4), plane_length_spinbutton, 1, 2, 0,
		     1, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);

    plane_width_spinbutton_adj = gtk_adjustment_new(9, 3, 39, 1, 10, 10);
    plane_width_spinbutton =
	gtk_spin_button_new(GTK_ADJUSTMENT(plane_width_spinbutton_adj), 1,
			    0);
    gtk_widget_show(plane_width_spinbutton);
    gtk_table_attach(GTK_TABLE(table4), plane_width_spinbutton, 1, 2, 1, 2,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);

    planelabel = gtk_label_new(_("Plane"));
    gtk_widget_show(planelabel);
    gtk_notebook_set_tab_label(GTK_NOTEBOOK(topologynotebook),
			       gtk_notebook_get_nth_page(GTK_NOTEBOOK
							 (topologynotebook),
							 2), planelabel);

    table3 = gtk_table_new(7, 2, FALSE);
    gtk_widget_show(table3);
    gtk_box_pack_start(GTK_BOX(vbox6), table3, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(table3), 10);
    gtk_table_set_row_spacings(GTK_TABLE(table3), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table3), 10);

    label3 = gtk_label_new(_("Spring Tension"));
    gtk_widget_show(label3);
    gtk_table_attach(GTK_TABLE(table3), label3, 0, 1, 0, 1,
		     (GtkAttachOptions) (0), (GtkAttachOptions) (0), 0, 0);

    label4 = gtk_label_new(_("Speed"));
    gtk_widget_show(label4);
    gtk_table_attach(GTK_TABLE(table3), label4, 0, 1, 1, 2,
		     (GtkAttachOptions) (0), (GtkAttachOptions) (0), 0, 0);

    label5 = gtk_label_new(_("Damping time"));
    gtk_widget_show(label5);
    gtk_table_attach(GTK_TABLE(table3), label5, 0, 1, 2, 3,
		     (GtkAttachOptions) (0), (GtkAttachOptions) (0), 0, 0);

    label6 = gtk_label_new(_("Type of Actuation"));
    gtk_widget_show(label6);
    gtk_table_attach(GTK_TABLE(table3), label6, 0, 1, 3, 4,
		     (GtkAttachOptions) (0), (GtkAttachOptions) (0), 0, 0);

    label7 = gtk_label_new(_("Velocity"));
    gtk_widget_show(label7);
    gtk_table_attach(GTK_TABLE(table3), label7, 0, 1, 4, 5,
		     (GtkAttachOptions) (0), (GtkAttachOptions) (0), 0, 0);

    label8 = gtk_label_new(_("Sample Length"));
    gtk_widget_show(label8);
    gtk_table_attach(GTK_TABLE(table3), label8, 0, 1, 5, 6,
		     (GtkAttachOptions) (0), (GtkAttachOptions) (0), 0, 0);

    thing = gtk_label_new(_("Final Decay"));
    gtk_widget_show(thing);
    gtk_table_attach(GTK_TABLE(table3), thing, 0, 1, 6, 7,
		     (GtkAttachOptions) (0), (GtkAttachOptions) (0), 0, 0);

    speed_hscale =
	gtk_hscale_new(GTK_ADJUSTMENT
		       (adj =
			gtk_adjustment_new(0.2, 0, 0.5, 0.01, 0.02, 0)));
    gtk_widget_show(speed_hscale);
    g_signal_connect(adj, "value-changed",
		     G_CALLBACK(on_speed_hscale_focus_out_event), NULL);
    gtk_table_attach(GTK_TABLE(table3), speed_hscale, 1, 2, 1, 2,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_FILL), 0, 0);
    gtk_scale_set_digits(GTK_SCALE(speed_hscale), 3);

    damping_hscale =
	gtk_hscale_new(GTK_ADJUSTMENT
		       (adj =
			gtk_adjustment_new(0.05, 0.005, 0.5, 0.001, 0.01,
					   0)));
    gtk_widget_show(damping_hscale);
    g_signal_connect(adj, "value-changed",
		     G_CALLBACK(on_damping_hscale_focus_out_event), NULL);
    gtk_table_attach(GTK_TABLE(table3), damping_hscale, 1, 2, 2, 3,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_FILL), 0, 0);
    gtk_scale_set_digits(GTK_SCALE(damping_hscale), 3);

    actuation_combo = gtk_combo_box_entry_new_text();
    gtk_widget_show(actuation_combo);
    gtk_table_attach(GTK_TABLE(table3), actuation_combo, 1, 2, 3, 4,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_combo_box_append_text(GTK_COMBO_BOX(actuation_combo), _("Compression"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(actuation_combo), _("Perpendicular Hit"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(actuation_combo), 0);

    gtk_entry_set_editable(GTK_ENTRY(GTK_BIN(actuation_combo)->child), FALSE);

    velocity_hscale =
	gtk_hscale_new(GTK_ADJUSTMENT
		       (adj = gtk_adjustment_new(1, 0, 1, 0.1, 0.05, 0)));
    gtk_widget_show(velocity_hscale);
    g_signal_connect(adj, "value_changed",
		     G_CALLBACK(on_velocity_hscale_focus_out_event), NULL);
    gtk_table_attach(GTK_TABLE(table3), velocity_hscale, 1, 2, 4, 5,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_FILL), 0, 0);
    gtk_scale_set_digits(GTK_SCALE(velocity_hscale), 3);

    length_hscale =
	gtk_hscale_new(GTK_ADJUSTMENT
		       (adj = gtk_adjustment_new(1, 0, 10, 0.1, 0.2, 0)));
    gtk_widget_show(length_hscale);
    g_signal_connect(adj, "value-changed",
		     G_CALLBACK(on_length_hscale_focus_out_event), NULL);
    gtk_table_attach(GTK_TABLE(table3), length_hscale, 1, 2, 5, 6,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_FILL), 0, 0);
    gtk_scale_set_digits(GTK_SCALE(length_hscale), 3);

    hbox4 = gtk_hbox_new(FALSE, 0);
    check_decay = gtk_check_button_new();
    hang_tooltip(check_decay,
		 _("Check this if you want to stop rendering at given decay"));
    gtk_widget_show(check_decay);
    gtk_box_pack_start(GTK_BOX(hbox4), check_decay, FALSE, TRUE, 0);

    thing =
	gtk_hscale_new(GTK_ADJUSTMENT
		       (adj_decay =
			gtk_adjustment_new(-50.0, -80.0, -20.0, 1.0, 5.0, 0)));
    hang_tooltip(thing,
		 _("Relative signal amplitude at which the rendering is stopped"));
    gtk_widget_set_sensitive(thing, 0);
    gtk_widget_show(thing);
    gtk_box_pack_start(GTK_BOX(hbox4), thing, TRUE, TRUE, 0);
    gtk_scale_set_digits(GTK_SCALE(thing), 2);
    g_signal_connect(check_decay, "toggled",
		     G_CALLBACK(use_decay_toggled), thing);
    gtk_widget_show(hbox4);
    gtk_table_attach(GTK_TABLE(table3), hbox4, 1, 2, 6, 7,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_FILL), 0, 0);

    tension_hscale =
	tension_hscale_create("tension_hscale", NULL, NULL, 0, 0);
    gtk_widget_show(tension_hscale);
    gtk_table_attach(GTK_TABLE(table3), tension_hscale, 1, 2, 0, 1,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_FILL), 0, 0);
    GTK_WIDGET_SET_FLAGS(tension_hscale, GTK_CAN_FOCUS);
    GTK_WIDGET_UNSET_FLAGS(tension_hscale, GTK_CAN_DEFAULT);

    openglframe = gtk_aspect_frame_new(_("3D View"), 0.5, 0.5, 1, FALSE);
    gtk_widget_show(openglframe);
    gtk_box_pack_start(GTK_BOX(hbox3), openglframe, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(openglframe), 4);

    opengl = opengl_create("opengl", NULL, NULL, 0, 0);
    gtk_widget_show(opengl);
    gtk_container_add(GTK_CONTAINER(openglframe), opengl);
#ifdef HAVE_OPENGL
    GTK_WIDGET_SET_FLAGS(opengl, GTK_CAN_FOCUS);
    GTK_WIDGET_UNSET_FLAGS(opengl, GTK_CAN_DEFAULT);
    gtk_widget_set_events(opengl,
			  GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_HINT_MASK
			  | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_PRESS_MASK
			  | GDK_BUTTON_RELEASE_MASK);
#endif

    table = gtk_table_new(2, 5, TRUE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table), 10);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(vbox3), table, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(table), 10);

    render = gtk_button_new_with_label(_("Render"));
    gtk_widget_show(render);
    gtk_table_attach_defaults(GTK_TABLE(table), render, 0, 1, 0, 1);
    gtk_tooltips_set_tip(tooltips, render, _("Render a sound"), NULL);
    
    /* render, play, save are made inactive when rendering */
    play = gtk_button_new_with_label(_("Play"));
    gtk_widget_show(play);
    gtk_table_attach_defaults(GTK_TABLE(table), play, 1, 2, 0, 1);
    gtk_widget_set_sensitive(play, FALSE);
    gtk_tooltips_set_tip(tooltips, play, _("Play rendered sound"), NULL);

    save = gtk_button_new_with_label(_("Save..."));
    gtk_widget_show(save);
    gtk_table_attach_defaults(GTK_TABLE(table), save, 2, 3, 0, 1);
    gtk_widget_set_sensitive(save, FALSE);
    gtk_tooltips_set_tip(tooltips, save, _("Save sound to wave file"),
			 NULL);

    about = gtk_button_new_with_label(_("About..."));
    gtk_widget_show(about);
    gtk_table_attach_defaults(GTK_TABLE(table), about, 3, 4, 0, 1);
    gtk_tooltips_set_tip(tooltips, about, _("About this program"), NULL);

    close = gtk_button_new_with_label(_("Close"));
    gtk_widget_show(close);
    gtk_table_attach_defaults(GTK_TABLE(table), close, 4, 5, 0, 1);
    gtk_tooltips_set_tip(tooltips, close, _("Close program"), NULL);

    thing = gtk_button_new_with_label(_("Save ins..."));
    gtk_widget_show(thing);
    gtk_table_attach_defaults(GTK_TABLE(table), thing, 0, 1, 1, 2);
    gtk_tooltips_set_tip(tooltips, thing, _("Save presets as instrument"), NULL);
    g_signal_connect(thing, "clicked", G_CALLBACK(save_ins_clicked), NULL);

    thing = gtk_button_new_with_label(_("Load ins..."));
    gtk_widget_show(thing);
    gtk_table_attach_defaults(GTK_TABLE(table), thing, 1, 2, 1, 2);
    gtk_tooltips_set_tip(tooltips, thing, _("Load instrument as presets"), NULL);
    g_signal_connect(thing, "clicked", G_CALLBACK(load_ins_clicked), NULL);

    thing = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(thing);
    gtk_container_set_border_width(GTK_CONTAINER(thing), 5);
    gtk_box_pack_start(GTK_BOX(vbox3), thing, TRUE, TRUE, 0);

    frame1 = gtk_frame_new(_("Status"));
    gtk_widget_show(frame1);
    gtk_box_pack_start(GTK_BOX(thing), frame1, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(frame1), 10);

    hbox4 = gtk_hbox_new(TRUE, 10);
    gtk_widget_show(hbox4);
    gtk_container_add(GTK_CONTAINER(frame1), hbox4);
    gtk_container_set_border_width(GTK_CONTAINER(hbox4), 10);

    status_label = gtk_label_new(_("Ready..."));
    gtk_widget_show(status_label);
    gtk_box_pack_start(GTK_BOX(hbox4), status_label, FALSE, FALSE, 0);

    progressbar1 = gtk_progress_bar_new();
    gtk_widget_show(progressbar1);
    gtk_box_pack_start(GTK_BOX(hbox4), progressbar1, FALSE, FALSE, 0);

    size_str = g_strdup_printf(_("Real length: %.1f s"), 0.0);
    size_label = gtk_label_new(size_str);
    gtk_widget_show(size_label);
    g_free(size_str);
    gtk_box_pack_start(GTK_BOX(hbox4), size_label, FALSE, FALSE, 0);

    lvbox = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(lvbox);
    btn = gtk_button_new_with_label(_("Setup..."));
    gtk_widget_show(btn);
    g_signal_connect(btn, "clicked",
		     G_CALLBACK(setup_dialog), NULL);

    gtk_box_pack_start(GTK_BOX(lvbox), btn, TRUE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(thing), lvbox, FALSE, FALSE, 0);

    g_signal_connect(AppWindow, "delete-event",
		     G_CALLBACK(on_AppWindow_delete_event), NULL);
    g_signal_connect(AppWindow, "destroy",
		     G_CALLBACK(on_AppWindow_destroy), opengl);
    g_signal_connect(topologynotebook, "switch-page",
		     G_CALLBACK(on_topologynotebook_switch_page), NULL);
    g_signal_connect(height_spinbutton, "value-changed",
		     G_CALLBACK(on_height_spinbutton_changed), NULL);
    g_signal_connect(circum_spinbutton, "value-changed",
		     G_CALLBACK(on_circum_spinbutton_changed), NULL);
    g_signal_connect(length_spinbutton, "value-changed",
		     G_CALLBACK(on_length_spinbutton_changed), NULL);
    g_signal_connect(plane_length_spinbutton, "value-changed",
		     G_CALLBACK(on_plane_length_spinbutton_changed), NULL);
    g_signal_connect(plane_width_spinbutton, "value-changed",
		     G_CALLBACK(on_plane_width_spinbutton_changed),  NULL);
    g_signal_connect(actuation_combo, "changed",
		     G_CALLBACK(on_actuation_comboentry_changed), NULL);
    g_signal_connect(render, "clicked",
		     G_CALLBACK(on_render_clicked), NULL);
    g_signal_connect(play, "clicked",
		     G_CALLBACK(on_play_clicked), NULL);
    g_signal_connect(save, "clicked",
		     G_CALLBACK(on_save_clicked), NULL);
    g_signal_connect(about, "clicked",
		     G_CALLBACK(create_AboutWindow), NULL);
    g_signal_connect(close, "clicked",
		     G_CALLBACK(on_close_clicked), NULL);

    return AppWindow;
}

static gboolean
on_FileSelector_delete_event(GtkWidget * widget,
			     GdkEvent * event, gpointer user_data)
{
    gtk_widget_hide(widget);
    return TRUE;
}

GtkWidget *gui_create_FileSelector(void(*callback)(), const gchar* title, const gchar *path)
{
    GtkWidget *FileSelector;
    GtkWidget *ok_button;
    GtkWidget *cancel_button;
    gchar *glib_path;

    FileSelector = gtk_file_selection_new(title);
    gtk_container_set_border_width(GTK_CONTAINER(FileSelector), 10);
    gtk_window_set_modal(GTK_WINDOW(FileSelector), TRUE);
    
    if(path) {
	glib_path = g_filename_from_utf8(path, -1, NULL, NULL, NULL);
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(FileSelector), glib_path);
	g_free(glib_path);
    }

    ok_button = GTK_FILE_SELECTION(FileSelector)->ok_button;
    gtk_widget_show(ok_button);
    GTK_WIDGET_SET_FLAGS(ok_button, GTK_CAN_DEFAULT);

    cancel_button = GTK_FILE_SELECTION(FileSelector)->cancel_button;
    gtk_widget_show(cancel_button);
    GTK_WIDGET_SET_FLAGS(cancel_button, GTK_CAN_DEFAULT);

    g_signal_connect(FileSelector, "delete-event",
		     G_CALLBACK(on_FileSelector_delete_event), NULL);
    g_signal_connect_swapped(ok_button, "clicked",
			     G_CALLBACK(callback),
			     FileSelector);
    g_signal_connect_swapped(cancel_button, "clicked",
			     G_CALLBACK(on_cancel_button_clicked),
			     FileSelector);

    return FileSelector;
}

void
gui_error_msg (gchar *msg)
{
    static GtkWidget *ErrorWindow = NULL;
    static GtkWidget *error_label;
    GtkWidget *vbox8;
    GtkWidget *error_ok_button;

    if(ErrorWindow && GTK_IS_WIDGET(ErrorWindow)) {
	if(!GTK_WIDGET_VISIBLE(ErrorWindow)) {
	    gtk_widget_show(ErrorWindow);
	    gtk_label_set_text(GTK_LABEL(error_label), msg);
	}
    } else {
	ErrorWindow = gtk_dialog_new();
	g_signal_connect(ErrorWindow, "response",
		         G_CALLBACK(dialog_clicked), NULL);

	gtk_window_set_title(GTK_WINDOW(ErrorWindow), _("Error"));
	gtk_window_set_modal(GTK_WINDOW(ErrorWindow), TRUE);
	gtk_window_set_policy(GTK_WINDOW(ErrorWindow), FALSE, FALSE, FALSE);

	vbox8 = GTK_DIALOG(ErrorWindow)->vbox;

	error_label = gtk_label_new(msg);
	gtk_widget_show(error_label);
	gtk_box_pack_start(GTK_BOX(vbox8), error_label, TRUE, FALSE, 0);
	gtk_misc_set_padding(GTK_MISC(error_label), 20, 20);

	error_ok_button = dialog_add_stock_button(GTK_DIALOG(ErrorWindow), GTK_STOCK_CLOSE, 0);
	gtk_widget_grab_focus(error_ok_button);

	gtk_widget_show(ErrorWindow);
    }
}

/* Message can only be set during the first call of this fuction on the given widget instance */
/* default_button = TRUE means Ok, FALSE -- Cancel */
GtkWidget*
gui_ok_cancel_dialog (GtkWidget *dialog, const gchar *title, const gchar *msg,
		      GCallback handler, gpointer data, gboolean default_button)
{
    GtkWidget *vbox, *label, *ok_button, *cancel_button;

    if(dialog && GTK_IS_WIDGET(dialog)) {
	if(!GTK_WIDGET_VISIBLE(dialog))
	    gtk_widget_show(dialog);
    } else {
	dialog = gtk_dialog_new();
	g_signal_connect(dialog, "response",
		         handler, data);

	gtk_window_set_title(GTK_WINDOW(dialog), title);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_policy(GTK_WINDOW(dialog), FALSE, FALSE, FALSE);

	vbox = GTK_DIALOG(dialog)->vbox;

	label = gtk_label_new(msg);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, FALSE, 0);
	gtk_misc_set_padding(GTK_MISC(label), 20, 20);

	ok_button = dialog_add_stock_button(GTK_DIALOG(dialog), GTK_STOCK_OK, GTK_RESPONSE_OK);
	cancel_button = dialog_add_stock_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_widget_grab_focus(default_button ? ok_button : cancel_button);

	gtk_widget_show(dialog);
    }
    return dialog;
}
