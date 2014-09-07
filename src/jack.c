/*  Power Station Industrializer
 *  Copyright (c) 2000 David A. Bartold
 *  Copyright (c) 2003 Yury Aliaev
 *  Copyright (c) 2014 Wladimir J. van der Laan
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
#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <glib.h>
#include <unistd.h>
#include <string.h>

#include "jack.h"

/* Size of ringbuffer */
#define BUFFER_SIZE 16384
/* Max length of error message */
#define ERROR_SIZE 128

static jack_port_t *output_port;
static jack_client_t *client;
static gchar jack_error[ERROR_SIZE];
static jack_ringbuffer_t *buffer;
static GMutex buffer_cond_mutex;
static GCond buffer_cond;

/* JACK audio callback */
static int JACK_AudioCallback(jack_nframes_t nframes, void *userdata)
{
    unsigned char *out = jack_port_get_buffer(output_port, nframes);
    size_t outsize = nframes * sizeof(float);
    size_t nread = jack_ringbuffer_read(buffer, (char*)out, outsize);
    // Pad with zeros if necessary
    if (nread < outsize)
        memset(out + nread, 0, outsize - nread);
    g_cond_signal(&buffer_cond);
    return 0;
}

/* JACK shutdown callback */
static void JACK_ShutdownCallback(void *userdata)
{
}

static int jack_open(void)
{
    jack_options_t options = JackNullOption;
    jack_status_t status;
    const char *server_name = NULL;
    const char **ports = NULL;
    GString *errstr = NULL;
    if ((errstr = g_string_new(NULL)) == NULL)
        goto error;

    if ((client = jack_client_open("psindustrializer", options, &status, server_name)) == 0) {
        g_string_append_printf(errstr, "jack_client_open() failed, status = 0x%2.0x\n", status);
        if (status & JackServerFailed) {
            g_string_append_printf(errstr, "Unable to connect to JACK server\n");
        }
        goto error;
    }

    g_string_append_printf(errstr, "Unable to connect to JACK server\n");
    jack_set_process_callback(client, JACK_AudioCallback, 0);
    jack_on_shutdown(client, JACK_ShutdownCallback, 0);

    //unsigned int jack_rate = (unsigned int)jack_get_sample_rate(client);
    //if(jack_rate != 44100)
    //    UI.InitMessage(-1, "(warning: this differs from PCM rate, %u)", (unsigned)PCM_RATE);

    /* Create one port for mono audio */
    output_port = jack_port_register(client, "out",
                                     JACK_DEFAULT_AUDIO_TYPE,
                                     JackPortIsOutput, 0);
    if (output_port == NULL) {
        g_string_append_printf(errstr, "no more JACK ports available\n");
        return -1;
    }

    if (jack_activate(client)) {
        g_string_append_printf(errstr, "JACK: cannot activate client\n");
        return -1;
    }

    /* Attempt to auto-connect to system output */
    ports = jack_get_ports(client, NULL, NULL,
                           JackPortIsPhysical|JackPortIsInput);
    if (ports != NULL)
    {
        int port;
        for(port=0; port<2; ++port)
        {
            if (jack_connect(client, jack_port_name(output_port), ports[port])) {
                g_string_append_printf(errstr, "JACK: cannot connect output ports\n");
                goto error;
            }
        }
        jack_free(ports);
    }

    /* Create ringbuffer for communication with playback thread */
    buffer = jack_ringbuffer_create(BUFFER_SIZE * sizeof(float));
    if (buffer == NULL)
    {
        g_string_append_printf(errstr, "JACK: cannot allocate ring buffer\n");
        goto error;
    }

    g_string_free(errstr, TRUE);
    return 0;
error:
    if (client)
        jack_client_close(client);
    if (ports)
        jack_free(ports);
    if (errstr)
    {
        g_strlcpy(jack_error, errstr->str, ERROR_SIZE);
        g_string_free(errstr, TRUE);
    }
    return -1;
}

static int jack_play(gint16 * ptr, int n)
{
    if (buffer == NULL || n > 2048)
    {
        g_strlcpy(jack_error, "Internal playback error", ERROR_SIZE);
        return -1;
    }
    /* TODO ideally we'd use a callback-based mechanism for sound rendering instead of this temporary buffer */
    /* Wait until there is place in the buffer */
    g_mutex_lock(&buffer_cond_mutex);
    while (jack_ringbuffer_write_space(buffer) < (n*sizeof(float)))
        g_cond_wait(&buffer_cond, &buffer_cond_mutex);
    g_mutex_unlock(&buffer_cond_mutex);

    /* Convert samples to float */
    float playbuffer[2048];
    int x;
    for (x=0; x<n; ++x)
        playbuffer[x] = ptr[x] / 32768.0;
    jack_ringbuffer_write(buffer, (const char*)&playbuffer, n * sizeof(float));
    return n;
}

/* Close and clean up */
static void jack_close(void)
{
    jack_deactivate(client);
    jack_port_unregister(client, output_port);
    jack_client_close(client);
    jack_ringbuffer_free(buffer);

    client = 0;
    output_port = 0;
}

static const char *jack_err(int errno)
{
    return jack_error;
}

drv driver_jack = {
    N_("JACK output"),
    jack_open,
    jack_play,
    jack_close,
    jack_err
};
