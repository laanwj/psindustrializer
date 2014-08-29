/* psmetalobj.h - Power Station Glib PhyMod Library
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

#ifndef __PS_METAL_OBJ_H_
#define __PS_METAL_OBJ_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef
struct _vector3
{
  double x, y, z;
} vector3;

typedef
struct _PSMetalObjNode
{
  int anchor;
  vector3  pos;
  vector3  vel;

  int                    num_neighbors;
  struct _PSMetalObjNode *neighbors[1];
} PSMetalObjNode;

typedef
struct _PSMetalObj
{
  int            num_nodes;
  PSMetalObjNode *nodes[1];
} PSMetalObj;

static PSMetalObjNode *ps_metal_obj_node_new (int neighbors);
void ps_metal_obj_node_free (PSMetalObjNode *n);
PSMetalObj *ps_metal_obj_new (int size);
void ps_metal_obj_free (PSMetalObj *obj);
PSMetalObj *ps_metal_obj_new_tube (int height, int circum, double tension);
PSMetalObj *ps_metal_obj_new_rod (int height, double tension);
PSMetalObj *ps_metal_obj_new_plane (int length, int width, double tension);
// PSMetalObj *ps_metal_obj_new_hypercube (int dimensions, int size, double tension);
void ps_metal_obj_perturb (PSMetalObj *obj, double speed, double damp);

#ifdef __cplusplus
}
#endif

#endif
