/* api-wrapper.c - Power Station hi-level functions implementation
 * based on PhyMod Library
 * Copyright (c) 2000 David A. Bartold
 * Copyright (c) 2004 Yury G. Aliaev
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <math.h>

#include "api-wrapper.h"

/* Now len means _maximal_ lenght if the given attenuation will not be reached;
   for disabling stopping at given attenuation, use attenuation = 0.0.
   Attenuation is given in dB, att = 60.0 means render will be stopped after
   the mean amplitude reach the value of -60 dB. */
static guint
ps_metal_obj_render(gint rate, PSMetalObj * obj, gint innode, gint outnode,
		    gdouble speed, gdouble damp, gint compress,
		    gdouble velocity, gint len, gdouble * samples,
		    PSPercentCallback * cb, gdouble att, gpointer userdata)
{
    gint i, real_len;
    gdouble maxvol;
    gdouble stasis;
    gdouble sample, hipass, hipass_coeff, lowpass_coeff, lowpass, maxamp;

    gdouble curr_att = 0.0;

    if (compress) {
	stasis = obj->nodes[outnode]->pos.z;
	obj->nodes[innode]->pos.z += velocity;
    } else {
	stasis = obj->nodes[outnode]->pos.x;
	obj->nodes[innode]->pos.x += velocity;
    }

    hipass = lowpass = maxamp = 0.0;
    hipass_coeff = pow(0.5, 5.0 / rate);
    lowpass_coeff = 1 - 20.0 / rate;	/* 50 ms integrator */
    damp = pow(0.5, 1.0 / (damp * rate));

    maxvol = 0.001;
    for (i = 0; i < len; i++) {
	ps_metal_obj_perturb(obj, speed, damp);

	if (compress)
	    sample = obj->nodes[outnode]->pos.z - stasis;
	else
	    sample = obj->nodes[outnode]->pos.x - stasis;

	hipass = hipass_coeff * hipass + (1.0 - hipass_coeff) * sample;
	samples[i] = sample - hipass;

	if (fabs(samples[i]) > maxvol)
	    maxvol = fabs(samples[i]);

	lowpass =
	    lowpass_coeff * lowpass + (1.0 -
				       lowpass_coeff) * fabs(samples[i]);
	if (maxamp < lowpass)
	    maxamp = lowpass;

	if (att < 0) {
	    if (maxamp > 0.0)
		curr_att = 20 * log10(lowpass / maxamp);

	    if (!(i & 1023))
		if (cb != NULL) {
		    float p1, p2;

		    p1 = ((float) i) / len;
		    p2 = curr_att / att;
		    cb(MAX(p1, p2), userdata);
		}

	    if (curr_att <= att)
		break;
	} else {
	    if (!(i & 1023))
		if (cb != NULL)
		    cb(((float) i) / len, userdata);
	}
    }
    real_len = i;

    maxvol = 1.0 / maxvol;
    for (i = 0; i < real_len; i++)
	samples[i] *= maxvol;

    return real_len;
}

guint
ps_metal_obj_render_tube(gint rate, gint height, gint circum,
			 gdouble tension, gdouble speed, gdouble damp,
			 gint compress, gdouble velocity, gint len,
			 gdouble * samples, PSPercentCallback * cb,
			 gdouble att, gpointer userdata)
{
    PSMetalObj *obj;
    gint innode, outnode;
    guint lgth;

    obj = ps_metal_obj_new_tube(height, circum, tension);

    innode = circum + circum / 2;
    outnode = (height - 2) * circum;

    lgth =
	ps_metal_obj_render(rate, obj, innode, outnode, speed, damp,
			    compress, velocity, len, samples, cb, att,
			    userdata);

    ps_metal_obj_free(obj);
    return lgth;
}

guint
ps_metal_obj_render_rod(int rate, int length, double tension, double speed,
			double damp, int compress, double velocity,
			int len, double *samples, PSPercentCallback * cb,
			gdouble att, gpointer userdata)
{
    PSMetalObj *obj;
    gint innode, outnode;
    guint lgth;

    obj = ps_metal_obj_new_rod(length, tension);

    innode = 1;
    outnode = length - 2;

    lgth =
	ps_metal_obj_render(rate, obj, innode, outnode, speed, damp,
			    compress, velocity, len, samples, cb, att,
			    userdata);

    ps_metal_obj_free(obj);
    return lgth;
}

guint
ps_metal_obj_render_plane(gint rate, gint length, gint width,
			  gdouble tension, gdouble speed, gdouble damp,
			  gint compress, gdouble velocity, gint len,
			  gdouble * samples, PSPercentCallback * cb,
			  gdouble att, gpointer userdata)
{
    PSMetalObj *obj;
    gint innode, outnode;
    guint lgth;

    obj = ps_metal_obj_new_plane(length, width, tension);

    innode = 1;
    outnode = (length - 1) * width - 1;

    lgth =
	ps_metal_obj_render(rate, obj, innode, outnode, speed, damp,
			    compress, velocity, len, samples, cb, att,
			    userdata);

    ps_metal_obj_free(obj);
    return lgth;
}
