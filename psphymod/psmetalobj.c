/* gmetalobj.c - Power Station Glib PhyMod Library
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

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "psmetalobj.h"

/* Crappy azz M$ software doesn't define this. */
#ifndef M_PI
#define M_PI 3.14159265358979
#endif

#undef TRUE
#define TRUE (1)

#undef FALSE
#define FALSE (0)

PSMetalObjNode *
ps_metal_obj_node_new (int neighbors)
{
  PSMetalObjNode *n;
  int size;

  size = sizeof (PSMetalObjNode) + sizeof (PSMetalObjNode*) * (neighbors - 1);

  n = (PSMetalObjNode*) malloc (size);
  if (n == NULL)
    return NULL;

  memset (n, 0, sizeof (PSMetalObjNode));
  n->num_neighbors = neighbors;

  return n;
} 

void
ps_metal_obj_node_free (PSMetalObjNode *n)
{
  if (n != NULL)
    free (n);
}

PSMetalObj *
ps_metal_obj_new (int size)
{
  PSMetalObj *obj;
  int byte_size;

  byte_size = sizeof (PSMetalObj) + sizeof (PSMetalObjNode*) * (size - 1);

  obj = (PSMetalObj*) malloc (byte_size);
  if (obj == NULL)
    return NULL;

  memset (obj, 0, sizeof (PSMetalObj));
  obj->num_nodes = size;

  return obj;
}

void
ps_metal_obj_free (PSMetalObj *obj)
{
  if (obj != NULL)
    {
      int i;

      for (i = 0; i < obj->num_nodes; i++)
        ps_metal_obj_node_free (obj->nodes[i]);

      free (obj);
    }
}

PSMetalObj *
ps_metal_obj_new_tube (int height, int circum, double tension)
{
  int x, y;
  int n;
  PSMetalObj *obj;
  double radius;
  double angle;
  PSMetalObjNode *inode = NULL;

  obj = ps_metal_obj_new (height * circum);
  if (obj == NULL)
    return NULL;

  radius = 0.5 / cos ((M_PI * (circum - 2)) / circum / 2.0);

  n = 0;
  for (y = 0; y < height; y++)
    for (x = 0; x < circum; x++)
      {
        angle = x * 2.0 * M_PI / circum;
        inode = ps_metal_obj_node_new ((y == 0 || y == height - 1) ? 3 : 4);
        if (inode == NULL)
          {
            ps_metal_obj_free (obj);
            return NULL;
          }

        obj->nodes[n] = inode;

        inode->pos.x = cos (angle) * radius;
        inode->pos.y = sin (angle) * radius;
        inode->pos.z = y * tension;

        if (y == height-1 || y == 0)
          inode->anchor = TRUE;

        n++;
      }

  n = 0;
  for (y = 0; y < height; y++)
    for (x = 0; x < circum; x++)
      {
        inode = obj->nodes[n];

        if (x == 0)
          inode->neighbors[0] = obj->nodes[y * circum + circum - 1];
        else
          inode->neighbors[0] = obj->nodes[n - 1];

        if (x == circum - 1)
          inode->neighbors[1] = obj->nodes[y * circum];
        else
          inode->neighbors[1] = obj->nodes[n + 1];

        if (y == 0)
          inode->neighbors[2] = obj->nodes[n + circum];
        else if (y == height - 1)
          inode->neighbors[2] = obj->nodes[n - circum];
        else
          {
            inode->neighbors[2] = obj->nodes[n + circum];
            inode->neighbors[3] = obj->nodes[n - circum];
          }

        n++;
      }

  return obj;
}

PSMetalObj *
ps_metal_obj_new_rod (int height, double tension)
{
  PSMetalObj *obj;
  PSMetalObjNode *inode = NULL;
  int i;

  obj = ps_metal_obj_new (height);
  if (obj == NULL)
    return NULL;

  for (i = 0; i < height; i++)
    {
      inode = ps_metal_obj_node_new ((i == 0 || i == height - 1) ? 1 : 2);
      if (inode == NULL)
        {
          ps_metal_obj_free (obj);
          return NULL;
        }
      obj->nodes[i] = inode;

      inode->pos.x = inode->pos.y = 0.0;
      inode->pos.z = i * tension;
    }

  for (i = 0; i < height; i++)
    {
      inode = obj->nodes[i];

      if (i == 0)
        {
          inode->neighbors[0] = obj->nodes[1];
          inode->anchor = TRUE;
        }
      else if (i == height - 1)
        {
          inode->neighbors[0] = obj->nodes[i - 1];
          inode->anchor = TRUE;
        }
      else
        {
          inode->neighbors[0] = obj->nodes[i - 1];
          inode->neighbors[1] = obj->nodes[i + 1];
        }
    }

  return obj;
}

PSMetalObj *
ps_metal_obj_new_plane (int length, int width, double tension)
{
  int x, y, dx, dy;
  int n, count;
  PSMetalObj *obj;
  PSMetalObjNode *inode = NULL;

  obj = ps_metal_obj_new (length * width);
  if (obj == NULL)
    return NULL;

  n = 0;
  for (y = 0; y < length; y++)
    for (x = 0; x < width; x++)
      {
        count = 8;

        if (y == 0 || y == length - 1)
          count = 5;

        if (x == 0 || x == width - 1)
          {
            if (count == 5)
              count = 3;
            else
              count = 5;
          }

        inode = ps_metal_obj_node_new (count);
        if (inode == NULL)
          {
            ps_metal_obj_free (obj);
            return NULL;
          }

        obj->nodes[n] = inode;

        inode->pos.x = 0.0;
        inode->pos.y = x;
        inode->pos.z = y * tension;

        n++;
      }

  obj->nodes [0]->anchor = TRUE;
  obj->nodes [width - 1]->anchor = TRUE;
  obj->nodes [(length - 1) * width]->anchor = TRUE;
  obj->nodes [(length - 1) * width + width - 1]->anchor = TRUE;

  n = 0;
  for (y = 0; y < length; y++)
    for (x = 0; x < width; x++)
      {
        inode = obj->nodes[n];

        count = 0;
        for (dy = -1; dy <= 1; dy++)
          for (dx = -1; dx <= 1; dx++)
            {
              if (x + dx >= 0 && x + dx < width &&
                  y + dy >= 0 && y + dy < length &&
                  (dx != 0 || dy != 0))
                inode->neighbors [count++] = obj->nodes [(y + dy) * width + x + dx];
            }

        n++;
      }

  return obj;
}

#if 0
PSMetalObj *
ps_metal_obj_new_hypercube (int    dimensions,
                            int    size,
                            double tension)
{
  int        i, count;
  PSMetalObj *obj;

  count = 1;
  for (i = 0; i < dimensions; i++)
    count *= size;

  obj = ps_metal_obj_new (count);

  for (i = 0; i < count; i++)
    {
      int j, value;
      int neighbors;

      value = i;
      neighbors = 0;
      for (j = 0; j < dimensions; j++)
        {
          int pos;
          pos = value % size;

          if (pos > 0)
            neighbors++;

          if (pos < size - 1)
            neighbors++;

          value /= size;
        }

      obj->nodes[i] = ps_metal_obj_node_new (neighbors);
    }

  for (i = 0; i < count; i++)
    {
      int     j, offset, value;
      int     neighbors;
      gboolean anchor;

      anchor = FALSE;
      neighbors = 0;
      offset = 1;
      value = i;
      for (j = 0; j < dimensions; j++)
        {
          int pos;
          pos = value % size;

          if (pos > 0)
            obj->nodes[i]->neighbors[neighbors++] =
              obj->nodes[i - offset];
          else
            anchor = TRUE;

          if (pos < size - 1)
            obj->nodes[i]->neighbors[neighbors++] =
              obj->nodes[i + offset];
          else
            anchor = TRUE;

          offset *= size;
          value /= size;
        }

      obj->nodes[i]->anchor = FALSE;
    }

  return obj;
}
#endif

inline void
ps_metal_obj_perturb (PSMetalObj *obj, double speed, double damp)
{
  int i, j;
  vector3 sum;
  vector3 dif;
  PSMetalObjNode *inode;
  double temp;
  double sprinps_k;

  for (i = 0; i < obj->num_nodes; i++)
    {
      inode = obj->nodes[i];
      if (!inode->anchor)
        {
          sum.x = sum.y = sum.z = 0.0;

          for (j = 0; j < inode->num_neighbors; j++)
            {
              dif.x = inode->pos.x - inode->neighbors[j]->pos.x;
              dif.y = inode->pos.y - inode->neighbors[j]->pos.y;
              dif.z = inode->pos.z - inode->neighbors[j]->pos.z;

              temp = 1.0 - sqrt ((dif.x * dif.x) + (dif.y * dif.y) + (dif.z * dif.z));

              sum.x += dif.x * temp;
              sum.y += dif.y * temp;
              sum.z += dif.z * temp;
            }

          sprinps_k = 1.0;
          inode->vel.x = (inode->vel.x + sprinps_k * sum.x * speed) * damp;
          inode->vel.y = (inode->vel.y + sprinps_k * sum.y * speed) * damp;
          inode->vel.z = (inode->vel.z + sprinps_k * sum.z * speed) * damp;
        }
    }

  for (i = 0; i < obj->num_nodes; i++)
    {
      inode = obj->nodes[i];

      if (!inode->anchor)
        {
          inode->pos.x += inode->vel.x * speed;
          inode->pos.y += inode->vel.y * speed;
          inode->pos.z += inode->vel.z * speed;
        }
    }
}
