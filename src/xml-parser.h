/* xml-parser.h - xml files manipulating functions (header)
 * Copyright (c) 2000 David A. Bartold
 * Copyright (c) 2005 Yury G. Aliaev
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

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <glib.h>

typedef struct _xmlpContext
{
    xmlDocPtr doc;
    xmlXPathContextPtr context;
    gchar *name;
    gchar *type;
} xmlpContext;

xmlpContext*	xmlp_new_doc		(const gchar *docname, const gchar *keyword);
xmlpContext*	xmlp_get_doc		(const gchar *docname, const gchar *keyword);
void		xmlp_sync		(xmlpContext *cntxt);

void		xmlp_free		(xmlpContext *cntxt);
inline void	xmlp_free_string	(gchar *str);

xmlChar*	xmlp_get		(xmlpContext *cntxt, const gchar *xpath,
					 const gchar *type, const gchar *name);
inline gchar*	xmlp_get_string		(xmlpContext *cntxt, const gchar *xpath,
					 const gchar *name);
inline gchar*	xmlp_get_string_default	(xmlpContext *cntxt, const gchar *xpath,
					 const gchar *name, const gchar *deflt);
inline gchar*	xmlp_get_set_string_default	(xmlpContext *cntxt, const gchar *xpath,
						 const gchar *name, const gchar *deflt);
inline gboolean	xmlp_get_int		(xmlpContext *cntxt, const gchar *xpath,
					 const gchar *name, gint *value);
inline gint	xmlp_get_int_default	(xmlpContext *cntxt, const gchar *xpath,
					 const gchar *name, const gint value);
inline gboolean	xmlp_get_double		(xmlpContext *cntxt, const gchar *xpath,
					 const gchar *name, gdouble *value);
inline gdouble	xmlp_get_double_default	(xmlpContext *cntxt, const gchar *xpath,
					 const gchar *name, const gdouble value);
inline gboolean xmlp_get_boolean_default	(xmlpContext *cntxt, const gchar *xpath,
						 const gchar *name, const gboolean value);

void		xmlp_set		(xmlpContext *cntxt, const gchar *xpath,
					 const gchar *type, const gchar *name, const gchar *value);
inline void	xmlp_set_string		(xmlpContext *cntxt, const gchar *xpath,
					 const gchar *name, const gchar *value);
inline void	xmlp_set_int		(xmlpContext *cntxt, const gchar *xpath,
					 const gchar *name, gint value);
inline void	xmlp_set_double		(xmlpContext *cntxt, const gchar *xpath,
					 const gchar *name, gdouble value);
inline void	xmlp_set_boolean	(xmlpContext *cntxt, const gchar *xpath,
					 const gchar *name, gboolean value);
