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

#include <esd.h>
#include <glib.h>
#include <unistd.h>

#include "esnd.h"

static int h;

static int esnd_open(void)
{
    h = esd_play_stream_fallback(ESD_BITS16 | ESD_MONO | ESD_STREAM |
				 ESD_PLAY, 44100, NULL, NULL);
    return 0;
}

static int esnd_play(gint16 * ptr, int n)
{
    int nbytes;

    nbytes = write(h, (guint8 *) ptr, n << 1);
    return nbytes >> 1;
}

static void esnd_close(void)
{
    close(h);
}

static const char *esnd_err(int errno)
{
    static const char *message =
	N_("Sorry, no diagnostics is available in EsounD driver");

    return message;
}

drv driver_esd = {
    N_("Esound output"),
    esnd_open,
    esnd_play,
    esnd_close,
    esnd_err
};
