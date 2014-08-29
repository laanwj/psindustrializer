/* psphymod.h - Power Station Glib PhyMod Library
 * Copyright (c) 2000 David A. Bartold
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

#ifndef __PS_PHYMOD_H_
#define __PS_PHYMOD_H_

#include <stdio.h>
#include <psphymod/psmetalobj.h>

/* Utility functions. */
inline gint16
double_to_s16 (gdouble d)
{
  if (d >= 1.0)
    return 32767;
  else if (d <= -1.0)
    return -32768;

  return (gint16) ((d + 1.0) * 32767.5 - 32768.0);
}

static inline gdouble
s16_to_double (gint16 i)
{
  return (((double) i) + 32768.0) / 32767.5 - 1.0;
}

static inline gint16
get_s16 (FILE *in)
{
  gint16 a;

  a = fgetc (in);

  return (fgetc (in) << 8) | a;
}

static inline void
put_s16 (FILE *out, gint16 i)
{
  fputc (i, out);
  fputc (i >> 8, out);
}

#ifdef __cplusplus
}
#endif

#endif
