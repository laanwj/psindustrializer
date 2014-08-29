/* api-wrapper.h - Power Station hi-level functions implementation
 * based on PhyMod Library (header)
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

#ifndef __API_WRAPPER_H_
#define __API_WRAPPER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "psmetalobj.h"
#include <glib.h>

typedef void PSPercentCallback (gfloat percent, gpointer userdata);

guint ps_metal_obj_render_tube (gint rate, gint height, gint circum, gdouble tension, gdouble speed, gdouble damp, gint compress, gdouble velocity, gint len, gdouble *samples, PSPercentCallback *cb, gdouble att, gpointer userdata);
guint ps_metal_obj_render_rod (gint rate, gint length, gdouble tension, gdouble speed, gdouble damp, gint compress, gdouble velocity, gint len, gdouble *samples, PSPercentCallback *cb, gdouble att,  gpointer userdata);
guint ps_metal_obj_render_plane (gint rate, gint length, gint width, gdouble tension, gdouble speed, gdouble damp, gint compress, gdouble velocity, gint len, gdouble *samples, PSPercentCallback *cb, gdouble att,  gpointer userdata);

#ifdef __cplusplus
}
#endif

#endif
