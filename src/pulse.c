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

#include <pulse/simple.h>
#include <glib.h>
#include <unistd.h>

#include "pulse.h"

pa_simple *s;

static int pulse_open(void)
{
    pa_sample_spec ss;
    ss.format = PA_SAMPLE_S16NE;
    ss.channels = 1;
    ss.rate = 44100;
    s = pa_simple_new(NULL, "PSIndustrializer", PA_STREAM_PLAYBACK, NULL, "sound", &ss,
            NULL, NULL, NULL);
    if (!s)
        return -1;
    return 0;
}

static int pulse_play(gint16 * ptr, int n)
{
    if (!s)
        return -1;
    int nbytes = pa_simple_write(s, (const void *) ptr, n << 1, NULL);
    if(nbytes < 0)
        return -1;
    return n;
}

static void pulse_close(void)
{
    pa_simple_free(s);
}

static const char *pulse_err(int errno)
{
    static const char *message =
	N_("Sorry, no diagnostics is available in Pulseaudio driver");

    return message;
}

drv driver_pulse = {
    N_("Pulseaudio output"),
    pulse_open,
    pulse_play,
    pulse_close,
    pulse_err
};
