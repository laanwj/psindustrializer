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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <string.h>
#ifdef HAVE_OPENGL
#  include <gtk/gtkgl.h>
#endif

#include "interface.h"
#include "main.h"
#include "xml-parser.h"

#ifdef DRIVER_ALSA
#include "alsa.h"
#endif

#ifdef DRIVER_PULSE
#include "pulse.h"
#endif

#ifdef DRIVER_JACK
#include "jack.h"
#endif

static guint current_driver;

GSList *driver_list = NULL;

inline guint psi_get_current_driver(void)
{
    return current_driver;
}

void psi_driver_errmessage(int errno)
{
    static const char *message = N_("Sound driver error:\n");
    char *buffer, *drv_message;

    drv_message = (char*)_(driver->err(errno));
    buffer = malloc(strlen(message) + strlen(drv_message) + 1);

    strcpy(buffer, message);
    strcat(buffer, drv_message);

    gui_error_msg(buffer);

    free(buffer);
}

void psi_set_driver(guint drv)
{
    int err;
    if (driver)
        driver->close();
    driver = g_slist_nth_data(driver_list, drv);
    current_driver = drv;
    if (driver != NULL)
    {
        if ((err = driver->open()) < 0) {
            psi_driver_errmessage(err);
            driver = NULL;
        }
    }
}

int main(int argc, char *argv[])
{
    GtkWidget		*AppWindow;
    drv			*currd;
    struct stat		st;
    struct passwd	*pw;
    gchar		*confdir, *conffile, *current_driver_string;
    xmlpContext		*cfg;

#ifdef ENABLE_NLS
    gtk_set_locale();
    bindtextdomain(PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(PACKAGE,"UTF-8");
    textdomain(PACKAGE);
#endif

    gtk_init(&argc, &argv);
#ifdef HAVE_OPENGL
    gtk_gl_init (&argc, &argv);
#endif

#ifdef DRIVER_PULSE
    driver_list = g_slist_append(driver_list, &driver_pulse);
#endif
#ifdef DRIVER_JACK
    driver_list = g_slist_append(driver_list, &driver_jack);
#endif
#ifdef DRIVER_ALSA
    driver_list = g_slist_append(driver_list, &driver_alsa);
#endif

    pw = getpwuid(getuid());
    confdir = g_strconcat(pw->pw_dir, "/."PACKAGE, NULL);
    conffile = g_strconcat(confdir, "/config", NULL);

    if(stat(confdir, &st) < 0)
	mkdir(confdir, S_IRUSR | S_IWUSR | S_IXUSR);

    current_driver = 0;
    if(!(cfg = xmlp_get_doc(conffile, "psiconfig")))
	cfg = xmlp_new_doc(conffile, "psiconfig");
    g_free(confdir);
    g_free(conffile);

    if((current_driver_string = xmlp_get_string(cfg, "driver/", "current"))) {
	guint i;
	
	for(i = 0; i < g_slist_length(driver_list); i++) {
	    currd = g_slist_nth_data(driver_list, i);
	    if(!g_ascii_strcasecmp(currd->description, current_driver_string))
		current_driver = i;
	}
	xmlp_free_string(current_driver_string);
    }
    psi_set_driver(current_driver);
    
    conf_autoext = xmlp_get_boolean_default(cfg, "behaviour/", "auto_ext", TRUE);
    conf_overwrite_warning = xmlp_get_boolean_default(cfg, "behaviour/", "overwrite_warning", TRUE);
    conf_instr_path = xmlp_get_string(cfg, "paths/", "instr_path");
    conf_sample_path = xmlp_get_string(cfg, "paths/", "sample_path");

    AppWindow = gui_create_AppWindow();
    gtk_widget_show(AppWindow);

    gtk_main();

    currd = g_slist_nth_data(driver_list, current_driver);
    if(currd)
        xmlp_set_string(cfg, "driver/", "current", (gchar *)currd->description);
    xmlp_set_boolean(cfg, "behaviour/", "auto_ext", conf_autoext);
    xmlp_set_boolean(cfg, "behaviour/", "overwrite_warning", conf_overwrite_warning);
    if(conf_instr_path) {
	xmlp_set_string(cfg, "paths/", "instr_path", conf_instr_path);
	xmlp_free_string(conf_instr_path);
    }
    if(conf_sample_path) {
	xmlp_set_string(cfg, "paths/", "sample_path", conf_sample_path);
	xmlp_free_string(conf_sample_path);
    }
    xmlp_sync(cfg);

    if (driver)
        driver->close();

    return 0;
}
