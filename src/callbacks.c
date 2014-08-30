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
#  include <config.h>
#endif

#ifdef WIN32
#  include <io.h>
#  include <windows.h>
#  include <windowsx.h>
#  include <mmsystem.h>
#else
#  include <math.h>
#ifdef HAVE_OPENGL
#  include <gtk/gtkgl.h>
#  include <GL/gl.h>
#endif
#  include <pthread.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <audiofile.h>

#include "callbacks.h"
#include "interface.h"
#include "api-wrapper.h"
#include "main.h"
#include "xml-parser.h"

GtkWidget *status_label, *progressbar1;

static const int rate = 44100;

static int size;
static int height = 10, circum = 5, length = 5;
static int plane_length = 7, plane_width = 9;
static double tenseness = 4.0, speed = 0.2, damping = 0.05;
static int actuation = 0;
static double velocity = 1.0;
static double sample_length = 1.0;
gint16 *samples = NULL;
static PSMetalObj *object = NULL;
static GMutex render_mutex;
static GtkWidget *area = NULL;
static int obj_type = 0;
static float x_angle = 90.0, y_angle = 0.0;
static gboolean decay_is_used = FALSE;
static double decay_value = 0.0;

static gfloat percent = 0.0;

typedef void (*CallbackFunc)(gpointer data);
static gboolean need_render = TRUE;
static CallbackFunc render_done_callback = NULL;
static CallbackFunc render_done_userdata = NULL;

static const gchar *types[] = {"tube", "rod", "plane"};
#define OBJ_NUM 3 /* Last object index */

static void save_wav_callback(GtkWidget * widget, gpointer user_data);

#ifdef HAVE_OPENGL
static void glarea_update(GtkWidget * widget);
#endif

static
gchar* filename_correct_ext(const gchar* fname, const gchar* ext)
{
    gchar *name_buf, *path_buf, *name_ret;
    
    name_buf = g_path_get_basename(fname);
    path_buf = g_path_get_dirname(fname);
    
    if(!strchr(name_buf, '.'))
	name_ret = g_strconcat(path_buf, "/", name_buf, ".", ext, NULL);
    else
	name_ret = g_strdup(fname); /* to make the returned string free'able anycase */
    
    g_free(name_buf);
    g_free(path_buf);
    
    return name_ret;
}

static int double_to_s16(double d)
{
    int out;

    if (d >= 1.0)
	out = 32767;
    else if (d <= -1.0)
	out = -32768;
    else
	out = (int) ((d + 1.0) * 32768.0 - 32768.0);

    return out;
}


static void percent_callback(gfloat p, gpointer userdata)
{
    percent = p;
}


static void render_object()
{
    ps_metal_obj_free(object);

    switch (obj_type) {
    case 0:
	object = ps_metal_obj_new_tube(height, circum, tenseness);
	break;

    case 1:
	object = ps_metal_obj_new_rod(length, tenseness);
	break;

    case 2:
	object =
	    ps_metal_obj_new_plane(plane_length, plane_width, tenseness);
	break;
    }

#ifdef HAVE_OPENGL
    gtk_widget_queue_draw(area);
#endif    
}

static void instrument_changed()
{
    need_render = TRUE;
}

gboolean
on_AppWindow_delete_event(GtkWidget *widget,
			  GdkEvent *event, gpointer user_data)
{
    return FALSE;
}

void
on_AppWindow_destroy(GtkObject *object, gpointer user_data)
{
    gtk_main_quit();
}


void
on_height_spinbutton_changed(GtkEditable * editable, gpointer user_data)
{
    height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
    instrument_changed();
    render_object();
}


void
on_circum_spinbutton_changed(GtkEditable * editable, gpointer user_data)
{
    circum = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
    instrument_changed();
    render_object();
}


void on_tension_hscale_changed(GtkWidget * widget, gpointer user_data)
{
    tenseness = GTK_ADJUSTMENT(widget)->value;
    instrument_changed();
    render_object();
}


void
on_speed_hscale_focus_out_event(GtkAdjustment * adj, gpointer user_data)
{
    speed = adj->value;
    instrument_changed();
}


void
on_damping_hscale_focus_out_event(GtkAdjustment * adj, gpointer user_data)
{
    damping = adj->value;
    instrument_changed();
}


void
on_velocity_hscale_focus_out_event(GtkAdjustment * adj, gpointer user_data)
{
    velocity = adj->value;
    instrument_changed();
}


void
on_length_hscale_focus_out_event(GtkAdjustment * adj, gpointer user_data)
{
    sample_length = adj->value;
    instrument_changed();
}


void
on_actuation_comboentry_changed(GtkComboBox * combobox, gpointer user_data)
{
    actuation = gtk_combo_box_get_active(combobox);
    instrument_changed();
}

void on_use_decay_toggled(GtkToggleButton * button, GtkWidget * target)
{
    decay_is_used = gtk_toggle_button_get_active(button);
    gtk_widget_set_sensitive(target, decay_is_used);
    instrument_changed();
}

void on_adj_decay_value_changed(GtkAdjustment * adj, gpointer user_data)
{
    decay_value = adj->value;
    instrument_changed();
}

static void set_status_message(gchar * str)
{
    gtk_label_set_text(GTK_LABEL(status_label), str);
}


static void set_percent(gfloat percent)
{
    gtk_progress_set_percentage(GTK_PROGRESS(progressbar1), percent);
}

static void *do_render(void *appwin)
{
    int i;
    gfloat decay;

    static double *data;
    static unsigned int alloc_length = 0;

    size = (int) (rate * sample_length);
    if (size > alloc_length) {
	data = g_renew(double, data, size);
	samples = g_renew(gint16, samples, size);
	alloc_length = size;
    }

    decay = decay_is_used ? decay_value : 0;

    switch (obj_type) {
    case 0:
	size =
	    ps_metal_obj_render_tube(rate, height, circum, tenseness,
				     speed, damping, actuation, velocity,
				     size, data, percent_callback, decay,
				     NULL);
	break;

    case 1:
	size =
	    ps_metal_obj_render_rod(rate, length, tenseness, speed,
				    damping, actuation, velocity, size,
				    data, percent_callback, decay, NULL);
	break;

    case 2:
	size =
	    ps_metal_obj_render_plane(rate, plane_length, plane_width,
				      tenseness, speed, damping, actuation,
				      velocity, size, data,
				      percent_callback, decay, NULL);
	break;
    }

    for (i = 0; i < size; i++)
	samples[i] = double_to_s16(data[i]);

    g_mutex_unlock(&render_mutex);

    return NULL;
}

static gint render_done(void *widget)
{
    if (g_mutex_trylock(&render_mutex)) {
	gui_set_sensitive(TRUE);
	set_status_message(_("Done..."));
	gui_set_size_label((gfloat) size / rate);
	percent = 0.0;
	set_percent(percent);
	g_mutex_unlock(&render_mutex);
        if (render_done_callback)
            render_done_callback(render_done_userdata);
	return FALSE;
    } else {
	set_percent(percent);
    }

    return TRUE;
}

void start_render(CallbackFunc callback, gpointer userdata)
{
    pthread_t thread;

    if (!g_mutex_trylock(&render_mutex))
	return;

    gui_set_sensitive(FALSE);
    set_status_message(_("Rendering..."));
    percent = 0.0;
    need_render = FALSE;
    render_done_callback = callback;
    render_done_userdata = userdata;

    if (pthread_create(&thread, NULL, do_render, NULL) != 0)
	do_render(NULL);

    g_timeout_add(100, render_done, NULL);
}

static void errmessage(errno)
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

void trigger_play(gpointer user_data)
{
    if (!g_mutex_trylock(&render_mutex))
        return;

    if (samples != NULL) {
#ifdef WIN32
        HWAVEOUT out;
        WAVEFORMATEX format;
        LPWAVEHDR hdr;

        format.wFormatTag = WAVE_FORMAT_PCM;
        format.nChannels = 1;
        format.nSamplesPerSec = 44100;
        format.nBlockAlign = 2;
        format.nAvgBytesPerSec =
            format.nSamplesPerSec * format.nBlockAlign;
        format.wBitsPerSample = 16;
        format.cbSize = 0;

        waveOutOpen(&out, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL);

        hdr = (LPWAVEHDR) GlobalAllocPtr(GMEM_MOVEABLE | GMEM_SHARE,
                                         (DWORD) sizeof(WAVEHDR));

        hdr->lpData =
            GlobalAllocPtr(GMEM_MOVEABLE | GMEM_SHARE, size * 2);
        hdr->dwBufferLength = size * 2;
        hdr->dwFlags = 0;
        memcpy(hdr->lpData, samples, size * 2);
        waveOutPrepareHeader(out, hdr, sizeof(WAVEHDR));
        waveOutWrite(out, hdr, sizeof(WAVEHDR));
        while (!(hdr->dwFlags & WHDR_DONE)) {
            Sleep(100);
        }
        waveOutUnprepareHeader(out, hdr, sizeof(WAVEHDR));
        GlobalFreePtr(hdr->lpData);
        GlobalFree(hdr);

        waveOutClose(out);

#else
        int n, nbytes;
        gint16 *ptr;
        int err;

        ptr = samples;
        if ((err = driver->open()) >= 0) {
            nbytes = size;
            while (nbytes > 0) {
                if (nbytes > 2048)
                    n = 2048;
                else
                    n = nbytes;
                n = driver->play(ptr, n);
                if (n < 0) {
                    errmessage(n);
                    break;
                }
                if (n > 0) {
                    ptr += n;
                    nbytes -= n;
                }
            }
            driver->close();
        } else {
            errmessage(err);
        }
#endif

        g_mutex_unlock(&render_mutex);
    }
}

void on_play_clicked(GtkButton * button, gpointer user_data)
{
    if (need_render)
        start_render(trigger_play, NULL);
    else
        trigger_play(NULL);
}

void on_space_pressed(gpointer user_data)
{
    if (need_render)
        start_render(trigger_play, NULL);
    else
        trigger_play(NULL);
}

void trigger_save(gpointer user_data)
{
    static GtkWidget *fileselector = NULL;
    if(samples != NULL) {
	if(!fileselector)
	    fileselector = gui_create_FileSelector(save_wav_callback,_("Save .WAV file"),
						   conf_sample_path);
	else {
	    gtk_widget_show(fileselector);
	    gtk_widget_grab_focus(fileselector);
	}

	gtk_widget_show(fileselector);
    }
}

void on_save_clicked(GtkButton * button, gpointer user_data)
{
    if (need_render)
        start_render(trigger_save, NULL);
    else
        trigger_save(NULL);
}


void on_close_clicked(GtkButton * button, gpointer user_data)
{
    gtk_main_quit();
}

static
void save_wav_do(GtkWidget * widget, gint response, gchar *fname)
{
    gchar	*path, *path1;

    if(widget)
	gtk_widget_hide(widget);
    if(response != GTK_RESPONSE_OK)
	return;

    path = g_path_get_dirname(fname);
    if(conf_sample_path)
	g_free(conf_sample_path);
    path1 = g_filename_to_utf8(path, -1, NULL, NULL, NULL);
    conf_sample_path = g_strconcat(path1, "/", NULL);
    g_free(path1);
    g_free(path);

    if (samples != NULL) {
#ifdef WIN32
	/* Microsoft is such a b*stard... they couldn't have made this
	   operation more opaque. */

	HMMIO wav;
	MMIOINFO info;
	MMCKINFO out, outRiff;
	PCMWAVEFORMAT format;
	gint32 total = size * 2;
	gint32 i;

	format.wf.wFormatTag = WAVE_FORMAT_PCM;
	format.wf.nChannels = 1;
	format.wf.nSamplesPerSec = 44100;
	format.wf.nBlockAlign = 2;
	format.wf.nAvgBytesPerSec =
	    format.wf.nSamplesPerSec * format.wf.nBlockAlign;
	format.wBitsPerSample = 16;

	unlink(fname);
	wav =
	    mmioOpen(fname, NULL,
		     MMIO_ALLOCBUF | MMIO_READWRITE | MMIO_CREATE);

	outRiff.fccType = mmioFOURCC('W', 'A', 'V', 'E');
	outRiff.cksize = 0;
	mmioCreateChunk(wav, &outRiff, MMIO_CREATERIFF);

	out.ckid = mmioFOURCC('f', 'm', 't', ' ');
	out.cksize = sizeof(PCMWAVEFORMAT);
	mmioCreateChunk(wav, &out, 0);
	mmioWrite(wav, (HPSTR) & format, sizeof(PCMWAVEFORMAT));

	mmioAscend(wav, &out, 0);

	out.ckid = mmioFOURCC('d', 'a', 't', 'a');
	out.cksize = 0;
	mmioCreateChunk(wav, &out, 0);
	mmioGetInfo(wav, &info, 0);

	for (i = 0; i < total; i++) {
	    if (info.pchNext == info.pchEndWrite) {
		info.dwFlags |= MMIO_DIRTY;
		mmioAdvance(wav, &info, MMIO_WRITE);
	    }

	    *((BYTE *) info.pchNext)++ = ((BYTE *) samples)[i];
	}

	info.dwFlags |= MMIO_DIRTY;
	mmioSetInfo(wav, &info, 0);
	mmioAscend(wav, &out, 0);
	mmioAscend(wav, &outRiff, 0);
	mmioClose(wav, 0);
#else
	AFfilehandle wav;
	AFfilesetup setup;

	setup = afNewFileSetup();
	afInitFileFormat(setup, AF_FILE_WAVE);
	afInitSampleFormat(setup, AF_DEFAULT_TRACK, AF_SAMPFMT_TWOSCOMP,
			   16);
	afInitByteOrder(setup, AF_DEFAULT_TRACK,
			AF_BYTEORDER_LITTLEENDIAN);
	afInitChannels(setup, AF_DEFAULT_TRACK, 1);
	afInitRate(setup, AF_DEFAULT_TRACK, 44100.0);

	wav = afOpenFile(fname, "w", setup);
	if (wav != NULL) {
	    afWriteFrames(wav, AF_DEFAULT_TRACK, samples, size);
	    afCloseFile(wav);
	} else {
	    g_print("Could not write file %s\n", fname);
	    gui_error_msg(_("Could not write file."));
	}

	afFreeFileSetup(setup);
#endif
	if(conf_autoext)
	    g_free(fname);
    }
}

static void
save_wav_callback (GtkWidget * widget, gpointer user_data)
{
    static GtkWidget *overwrite_dialog = NULL;
    static gchar *fname;

    gtk_widget_hide(widget);

    /* Hack, but rather harmless... I don't want to add variables without need. */
    fname = (gchar*)gtk_file_selection_get_filename(GTK_FILE_SELECTION(widget));
    if(conf_autoext)
	fname = filename_correct_ext(fname, "wav");
    if(conf_overwrite_warning && g_file_test(fname, G_FILE_TEST_EXISTS))
	overwrite_dialog = gui_ok_cancel_dialog(overwrite_dialog, _("File overwrite warning"),
				_("Warning! File exists!\nWould you like to overwrite it?"),
				G_CALLBACK(save_wav_do), fname, FALSE);
    else
	save_wav_do(NULL, GTK_RESPONSE_OK, fname);
}

void on_cancel_button_clicked(GtkWidget * widget, gpointer user_data)
{
    gtk_widget_hide(GTK_WIDGET(widget));
}

#ifdef HAVE_OPENGL
static gint glarea_draw(GtkWidget * widget, GdkEventExpose * event);
static gint glarea_realize(GtkWidget * widget);

static gdouble basex, basey;

static gint glarea_press(GtkWidget * widget, GdkEventMotion * event)
{
    basex = event->x;
    basey = event->y;

    return TRUE;
}

static gint glarea_release(GtkWidget * widget, GdkEventMotion * event)
{
    return TRUE;
}

static void
cube(float x1, float x2, float y1, float y2, float z1, float z2)
{
    /* Face 1 */
    glBegin(GL_TRIANGLE_STRIP);
    glNormal3f(0.0, 0.0, -1.0);
    glVertex3f(x1, y1, z1);
    glVertex3f(x1, y2, z1);
    glVertex3f(x2, y1, z1);
    glVertex3f(x2, y2, z1);
    glEnd();

    /* Face 2 */
    glBegin(GL_TRIANGLE_STRIP);
    glNormal3f(1.0, 0.0, 0.0);
    glVertex3f(x2, y1, z1);
    glVertex3f(x2, y2, z1);
    glVertex3f(x2, y1, z2);
    glVertex3f(x2, y2, z2);
    glEnd();

    /* Face 3 */
    glBegin(GL_TRIANGLE_STRIP);
    glNormal3f(0.0, 0.0, 1.0);
    glVertex3f(x2, y1, z2);
    glVertex3f(x2, y2, z2);
    glVertex3f(x1, y1, z2);
    glVertex3f(x1, y2, z2);
    glEnd();

    /* Face 4 */
    glBegin(GL_TRIANGLE_STRIP);
    glNormal3f(-1.0, 0.0, 0.0);
    glVertex3f(x1, y1, z2);
    glVertex3f(x1, y2, z2);
    glVertex3f(x1, y1, z1);
    glVertex3f(x1, y2, z1);
    glEnd();

    /* Face 5 */
    glBegin(GL_TRIANGLE_STRIP);
    glNormal3f(0.0, -1.0, 0.0);
    glVertex3f(x1, y1, z2);
    glVertex3f(x1, y1, z1);
    glVertex3f(x2, y1, z2);
    glVertex3f(x2, y1, z1);
    glEnd();

    /* Face 6 */
    glBegin(GL_TRIANGLE_STRIP);
    glNormal3f(0.0, 1.0, 0.0);
    glVertex3f(x1, y2, z1);
    glVertex3f(x1, y2, z2);
    glVertex3f(x2, y2, z1);
    glVertex3f(x2, y2, z2);
    glEnd();
}

static void glarea_update(GtkWidget * widget)
{
    int i;
    vector3 *v3;
    float maxx, maxy, maxz;
    float minx, miny, minz;
    float max, min;
    float cenx, ceny, cenz;
    float size;
    float ptsize;

    GLfloat light_ambient[] = { 0.5, 0.5, 0.5, 1.0 };
    GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light_position[] = { 10.0, 10.0, 10.0, 0.0 };
    GLfloat global_ambient[] = { 0.75, 0.75, 0.75, 1.0 };

    GLfloat red_mat_diffuse[] = { 1.0, 0.0, 0.0, 1.0 };
    GLfloat white_mat_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };

    if (widget == NULL || !GTK_WIDGET_REALIZED(widget))
	return;

    GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (widget);
    GdkGLContext *glcontext = gtk_widget_get_gl_context (widget);
    if (gdk_gl_drawable_gl_begin (gldrawable, glcontext)) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (object != NULL) {
	    minx = maxx = 0.0;
	    miny = maxy = 0.0;
	    minz = maxz = 0.0;

	    for (i = 0; i < object->num_nodes; i++) {
		v3 = &object->nodes[i]->pos;

		if (v3->x < minx)
		    minx = v3->x;
		if (v3->y < miny)
		    miny = v3->y;
		if (v3->z < minz)
		    minz = v3->z;
		if (v3->x > maxx)
		    maxx = v3->x;
		if (v3->y > maxy)
		    maxy = v3->y;
		if (v3->z > maxz)
		    maxz = v3->z;
	    }

	    min = minx;
	    if (miny < min)
		min = miny;
	    if (minz < min)
		min = minz;
	    max = maxx;
	    if (maxy > max)
		max = maxy;
	    if (maxz > max)
		max = maxz;

	    cenx = (minx + maxx) / 2.0;
	    ceny = (miny + maxy) / 2.0;
	    cenz = (minz + maxz) / 2.0;
	    size = (max - min) / 2.0 * 1.2;

	    glEnable(GL_LIGHTING);
	    glEnable(GL_LIGHT0);
	    glDepthRange(0.4, 0.6);

	    glFrontFace(GL_CCW);
	    glDepthFunc(GL_LESS);
	    glEnable(GL_DEPTH_TEST);
	    glCullFace(GL_BACK);
	    glEnable(GL_CULL_FACE);
	    glViewport(0, 0, widget->allocation.width,
		       widget->allocation.height);
	    glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	    glFrustum(-1.0, 1.0, -1.0, 1.0, 1.0, 200.0);
	    glScalef(95.0 / size, 95.0 / size, 1.0);

	    glMatrixMode(GL_MODELVIEW);
	    glLoadIdentity();

	    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

	    glTranslatef(0.0, 0.0, -100.0);
	    glRotatef(y_angle, 1.0, 0.0, 0.0);
	    glRotatef(x_angle, 0.0, 1.0, 0.0);
	    glTranslatef(-cenx, -ceny, -cenz);

	    ptsize = tenseness * 0.4;
	    if (ptsize > 0.4)
		ptsize = 0.4;

	    for (i = 0; i < object->num_nodes; i++) {
		v3 = &object->nodes[i]->pos;

		if (object->nodes[i]->anchor)
		    glMaterialfv(GL_FRONT, GL_DIFFUSE, red_mat_diffuse);
		else
		    glMaterialfv(GL_FRONT, GL_DIFFUSE, white_mat_diffuse);

		cube(v3->x - ptsize, v3->x + ptsize, v3->y - ptsize,
		     v3->y + ptsize, v3->z - ptsize, v3->z + ptsize);
	    }
	}

        if (gdk_gl_drawable_is_double_buffered (gldrawable))
            gdk_gl_drawable_swap_buffers (gldrawable);
        else
            glFlush ();
        gdk_gl_drawable_gl_end (gldrawable);
    }
}

static gint glarea_motion(GtkWidget * widget, GdkEventMotion * event)
{
    /* This code is borrowed from the planetmm GtkGLArea-- sample app. */

    gdouble x, y;
    gfloat xChange, yChange;
    GdkModifierType state;

    /* Calculate the relative motion of the cursor */
    if (event->is_hint) {
	gint tx, ty;
	gdk_window_get_pointer(event->window, &tx, &ty, &state);
	x = (gdouble) tx;
	y = (gdouble) ty;
    } else {
	x = event->x;
	y = event->y;
	state = (GdkModifierType) event->state;
    }

    if (!(state & GDK_BUTTON1_MASK))
	return TRUE;

    xChange = 0.25 * (GLfloat) (basex - x);
    yChange = 0.25 * (GLfloat) (basey - y);
    basex = event->x;
    basey = event->y;

    /* Transform curser motions into object rotations */
    x_angle += xChange;
    y_angle += yChange;

    /* Limit the values of rotation */
    while (x_angle < 0.0)
	x_angle += 360.0;
    while (x_angle > 360.0)
	x_angle -= 360.0;
    while (y_angle < 0.0)
	y_angle += 360.0;
    while (y_angle > 360.0)
	y_angle -= 360.0;

    gtk_widget_queue_draw(widget);

    return TRUE;
}
#endif

GtkWidget *opengl_create(gchar * widget_name, gchar * string1,
			 gchar * string2, gint int1, gint int2)
{
#ifdef HAVE_OPENGL
    GdkGLContext *glcontext = NULL;
    GdkGLConfig *glconfig = NULL;
    if (glconfig == NULL)
    {
        glconfig = gdk_gl_config_new_by_mode (GDK_GL_MODE_RGB | GDK_GL_MODE_DEPTH | GDK_GL_MODE_DOUBLE);

        if (glconfig == NULL)
        {
            glconfig = gdk_gl_config_new_by_mode (GDK_GL_MODE_RGB | GDK_GL_MODE_DEPTH);
        }
    }
    if (glconfig == NULL)
        return gtk_label_new(_("<OpenGL is buggy!>"));
    area = gtk_drawing_area_new ();
    gtk_widget_set_gl_capability (area, glconfig, glcontext, TRUE, GDK_GL_RGBA_TYPE);

    gtk_signal_connect(GTK_OBJECT(area), "expose_event",
		       GTK_SIGNAL_FUNC(glarea_draw), NULL);

    gtk_signal_connect(GTK_OBJECT(area), "realize",
		       GTK_SIGNAL_FUNC(glarea_realize), NULL);

    gtk_signal_connect(GTK_OBJECT(area), "button_press_event",
		       GTK_SIGNAL_FUNC(glarea_press), NULL);

    gtk_signal_connect(GTK_OBJECT(area), "button_release_event",
		       GTK_SIGNAL_FUNC(glarea_release), NULL);

    gtk_signal_connect(GTK_OBJECT(area), "motion_notify_event",
		       GTK_SIGNAL_FUNC(glarea_motion), NULL);

    gtk_drawing_area_size(GTK_DRAWING_AREA(area), 300, 300);

    return area;
#else
    return gtk_label_new(_("<OpenGL not supported!>"));
#endif
}

#ifdef HAVE_OPENGL
static gint glarea_draw (GtkWidget * widget, GdkEventExpose * event)
{
    if (event->count > 0)
	return TRUE;

    glarea_update(widget);

    return TRUE;
}

static gint glarea_realize (GtkWidget * widget)
{
    render_object();

    return TRUE;
}
#endif

void
on_length_spinbutton_changed (GtkEditable * editable, gpointer user_data)
{
    length = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
    render_object();
    instrument_changed();
}


void
on_topologynotebook_switch_page (GtkNotebook * notebook,
				GtkNotebookPage * page,
				gint page_num, gpointer user_data)
{
    obj_type = page_num;
    render_object();
    instrument_changed();
}


GtkWidget *tension_hscale_create (gchar * widget_name, gchar * string1,
				 gchar * string2, gint int1, gint int2)
{
    GtkAdjustment *adj;
    GtkHScale *hscale;

    adj =
	GTK_ADJUSTMENT(gtk_adjustment_new
		       (4.0, 0.1, 16.0, 0.005, 0.1, 0.0));

    hscale = GTK_HSCALE(gtk_hscale_new(adj));

    gtk_signal_connect(GTK_OBJECT(adj), "value_changed",
		       GTK_SIGNAL_FUNC(on_tension_hscale_changed), NULL);

    gtk_scale_set_digits(GTK_SCALE(hscale), 3);

    return GTK_WIDGET(hscale);
}


void
on_plane_length_spinbutton_changed (GtkEditable * editable,
				   gpointer user_data)
{
    plane_length =
	gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
    instrument_changed();
    render_object();
}


void
on_plane_width_spinbutton_changed (GtkEditable * editable,
				  gpointer user_data)
{
    plane_width =
	gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
    instrument_changed();
    render_object();
}


void
on_error_ok_button_clicked (GtkWidget * widget, gpointer user_data)
{
    gtk_widget_destroy(widget);
}

static void
save_ins_do (GtkWidget *widget, gint response, gchar* fname)
{
    xmlpContext	*instr;
    gchar	*path, *path1;

    if(widget)
	gtk_widget_hide(widget);
    if(response != GTK_RESPONSE_OK)
	return;

    path = g_path_get_dirname(fname);
    if(conf_instr_path)
	g_free(conf_instr_path);
    path1 = g_filename_to_utf8(path, -1, NULL, NULL, NULL);
    conf_instr_path = g_strconcat(path1, "/", NULL);
    g_free(path1);
    g_free(path);

    instr = xmlp_new_doc(fname, "psiinstr");
    
    xmlp_set_string(instr, "", "type", types[obj_type]);

    switch(obj_type) {
    case 0: /* tube */
	xmlp_set_int(instr, "", "height", height);
	xmlp_set_int(instr, "", "circumference", circum);
	break;
    case 1: /* rod */
	xmlp_set_int(instr, "", "length", length);
	break;
    case 2: /* plane */
	xmlp_set_int(instr, "", "length", plane_length);
	xmlp_set_int(instr, "", "width", plane_width);
	break;
    default:
	break;
    }
    
    xmlp_set_double(instr, "", "tension", tenseness);
    xmlp_set_double(instr, "", "speed", speed);
    xmlp_set_double(instr, "", "damping", damping);
    xmlp_set_int(instr, "", "actuation", actuation);
    xmlp_set_double(instr, "", "velocity", velocity);
    
    xmlp_sync(instr);
    xmlp_free(instr);
    if(conf_autoext)
	g_free(fname);
    
}

static void
save_ins_callback (GtkWidget * widget, gpointer user_data)
{
    static GtkWidget *overwrite_dialog = NULL;
    static gchar *fname;

    gtk_widget_hide(widget);

    fname = (gchar*)gtk_file_selection_get_filename(GTK_FILE_SELECTION(widget));
    if(conf_autoext)
	fname = filename_correct_ext(fname, "psii");
    if(conf_overwrite_warning && g_file_test(fname, G_FILE_TEST_EXISTS))
	overwrite_dialog = gui_ok_cancel_dialog(overwrite_dialog, _("File overwrite warning"),
				_("Warning! File exists!\nWould you like to overwrite it?"),
				G_CALLBACK(save_ins_do), fname, FALSE);
    else
	save_ins_do(NULL, GTK_RESPONSE_OK, fname);
}

void
save_ins_clicked (GtkWidget *widget, gpointer user_data)
{
    static GtkWidget *fileselector = NULL;

    if(!fileselector)
	fileselector = gui_create_FileSelector(save_ins_callback,_("Save presets as instrument"),
					       conf_instr_path);
    else {
	gtk_widget_show(fileselector);
	gtk_widget_grab_focus(fileselector);
    }

    gtk_widget_show(fileselector);
}

static
void load_ins_callback (GtkWidget * widget, gpointer user_data)
{
    G_CONST_RETURN gchar *fname;
    xmlpContext *instr;
    gchar *type, *path, *path1;
    guint i, itype = 0;

    fname = gtk_file_selection_get_filename(GTK_FILE_SELECTION(widget));
    path = g_path_get_dirname(fname);
    if(conf_instr_path)
	g_free(conf_instr_path);
    path1 = g_filename_to_utf8(path, -1, NULL, NULL, NULL);
    conf_instr_path = g_strconcat(path1, "/", NULL);
    g_free(path1);
    g_free(path);

    instr = xmlp_get_doc(fname, "psiinstr");
    if(!instr)
	return;
    
    type = xmlp_get_string_default(instr, "", "type", "tube");
    for(i = 0; i < OBJ_NUM; i++) {
	if(!g_ascii_strcasecmp(type, types[i]))
	    itype = i;
    }
    obj_type = itype;
    gui_set_topology(itype);
    
    switch(itype) {
    case 0: /* tube */
	height = xmlp_get_int_default(instr, "", "height", 10);
	circum = xmlp_get_int_default(instr, "", "circumference", 5);
	gui_configure_tube(height, circum);
	break;
    case 1: /* rod */
	length = xmlp_get_int_default(instr, "", "length", 5);
	gui_configure_rod(length);
	break;
    case 2: /* plane */
	plane_length = xmlp_get_int_default(instr, "", "length", 7);
	plane_width = xmlp_get_int_default(instr, "", "width", 9);
	gui_configure_plane(plane_length, plane_width);
	break;
    default:
	break;
    }
    
    tenseness = xmlp_get_double_default(instr, "", "tension", 4.0);
    speed = xmlp_get_double_default(instr, "", "speed", 0.2);
    damping = xmlp_get_double_default(instr, "", "damping", 0.05);
    actuation = xmlp_get_int_default(instr, "", "actuation", 0);
    velocity = xmlp_get_double_default(instr, "", "velocity", 1.0);
    gui_set_values(tenseness, speed, damping, actuation, velocity);
    xmlp_sync(instr);
    xmlp_free(instr);
    
    gtk_widget_hide(widget);
    instrument_changed();
}

void
load_ins_clicked (GtkWidget *widget, gpointer user_data)
{
    static GtkWidget *fileselector = NULL;

    if(!fileselector)
	fileselector = gui_create_FileSelector(load_ins_callback,_("Load instrument as presets"),
					       conf_instr_path);
    else {
	gtk_widget_show(fileselector);
	gtk_widget_grab_focus(fileselector);
    }

    gtk_widget_show(fileselector);
}
