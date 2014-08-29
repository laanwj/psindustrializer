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

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

#include <alsa/asoundlib.h>
#include <glib.h>

#include "alsa.h"

char *device = "plughw:0,0";	/* playback device */
snd_pcm_format_t format = SND_PCM_FORMAT_S16;	/* sample format */
unsigned int rate = 44100;	/* stream rate */
unsigned int channels = 1;	/* count of channels */

snd_output_t *output = NULL;
snd_pcm_t *handle;

static int alsa_open(void)
{
    unsigned int rrate = rate;
    snd_pcm_uframes_t bufsize;
    int err;

    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;

    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);

    if ((err = snd_output_stdio_attach(&output, stdout, 0)) < 0) {
	return err;
    }
    if ((err =
	 snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
	return err;
    }
    if ((err = snd_pcm_hw_params_any(handle, hwparams)) < 0) {
	return err;
    }
    if ((err =
	 snd_pcm_hw_params_set_access(handle, hwparams,
				      SND_PCM_ACCESS_RW_INTERLEAVED)) <
	0) {
	return err;
    }
    if ((err = snd_pcm_hw_params_set_format(handle, hwparams, format)) < 0) {
	return err;
    }
    if ((err = snd_pcm_hw_params_set_channels(handle, hwparams, 1)) < 0) {
	return err;
    }
    if ((err =
	 snd_pcm_hw_params_set_rate_near(handle, hwparams, &rrate,
					 0)) < 0) {
	return err;
    }
    if ((float) rrate / (float) rate > 1.1
	|| (float) rrate / (float) rate < .9) {
	fprintf(stderr, "Supported rate is too far from 44100Hz\n");
	return G_MININT;
    }
    if ((err =
	 snd_pcm_hw_params_get_buffer_size_max(hwparams, &bufsize)) < 0) {
	return err;
    }
    if (bufsize < 2 * 2048) {
	fprintf(stderr, "Buffer size is too small, sorry...\n");
	return G_MININT;
    }
    if ((err =
	 snd_pcm_hw_params_set_buffer_size(handle, hwparams, bufsize =
					   2 * 2048)) < 0) {
	return err;
    }
    if ((err = snd_pcm_hw_params_set_periods(handle, hwparams, 2, 0)) < 0) {
	return err;
    }
    if ((err = snd_pcm_hw_params(handle, hwparams)) < 0) {
	return err;
    }
    if ((err = snd_pcm_sw_params_current(handle, swparams)) < 0) {
	return err;
    }
    if ((err =
	 snd_pcm_sw_params_set_start_threshold(handle, swparams,
					       bufsize)) < 0) {
	return err;
    }
    if ((err =
	 snd_pcm_sw_params_set_avail_min(handle, swparams, 2048)) < 0) {
	return err;
    }
    if ((err = snd_pcm_sw_params_set_xfer_align(handle, swparams, 1)) < 0) {
	return err;
    }
    if ((err = snd_pcm_sw_params(handle, swparams)) < 0) {
	return err;
    }
    return 0;
}

static int xrun_recovery(snd_pcm_t * handle, int err)
{
    if (err == -EPIPE) {	/* under-run */
	err = snd_pcm_prepare(handle);
	if (err < 0)
	    fprintf(stderr,
		    "Can't recovery from underrun, prepare failed: %s\n",
		    snd_strerror(err));
	return 0;
    } else if (err == -ESTRPIPE) {
	while ((err = snd_pcm_resume(handle)) == -EAGAIN)
	    sleep(1);		/* wait until the suspend flag is released */
	if (err < 0) {
	    err = snd_pcm_prepare(handle);
	    if (err < 0)
		fprintf(stderr,
			"Can't recovery from suspend, prepare failed: %s\n",
			snd_strerror(err));
	}
	return 0;
    }
    return err;
}

static int alsa_play(gint16 * ptr, int n)
{
    int err = 0;

    while (err <= 0) {
	err = snd_pcm_writei(handle, ptr, n);

	if (err == -EAGAIN)
	    continue;
	if (err < 0) {
	    if (xrun_recovery(handle, err) < 0) {
		return err;
	    }
	    break;		/* skip one period */
	}
    }
    return err;
}

static void alsa_close(void)
{
    snd_pcm_close(handle);
}

static const char *alsa_err(int errnum)
{
    static const char *message =
	N_("PSIndustrializer internal error\nSee stderr for details");

    if (errno == G_MININT)
	return message;

    return snd_strerror(errnum);
}

drv driver_alsa = {
    N_("ALSA output"),
    alsa_open,
    alsa_play,
    alsa_close,
    alsa_err
};
