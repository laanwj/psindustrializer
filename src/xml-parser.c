/* xml-parser.c - xml files manipulating functions
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

#include <stdlib.h>
#include <string.h>
#include <glib/gprintf.h>

#include "xml-parser.h"

static gchar buf[256];

xmlpContext*
xmlp_new_doc (const gchar *docname, const gchar *keyword)
{
    xmlpContext *cntxt;
    xmlNodePtr	cur;

    cntxt = g_new(xmlpContext, 1);
    cntxt->name = g_strdup(docname);
    cntxt->type = g_strdup(keyword);

    cntxt->doc = xmlNewDoc("1.0");
    cur = xmlNewDocNode(cntxt->doc, NULL, keyword, NULL);
    xmlDocSetRootElement(cntxt->doc, cur);

    cntxt->context = xmlXPathNewContext(cntxt->doc);
    return cntxt;
}

xmlpContext*
xmlp_get_doc (const gchar *docname, const gchar *keyword)
{
    xmlpContext *cntxt;
    xmlNodePtr cur;

    cntxt = g_new(xmlpContext, 1);
    cntxt->name = g_strdup(docname);
    cntxt->type = g_strdup(keyword);
    xmlKeepBlanksDefault(0);

    if((cntxt->doc = xmlParseFile(docname)) == NULL)
	return NULL;

    if(!(cur = xmlDocGetRootElement(cntxt->doc))) {
	xmlFreeDoc(cntxt->doc);
	return NULL;
    }
    
    if(xmlStrcasecmp(cur->name, (const xmlChar*)keyword)){
	xmlFreeDoc(cntxt->doc);
	return NULL;
    }

    cntxt->context = xmlXPathNewContext(cntxt->doc);
    return cntxt;
}

static xmlXPathObjectPtr
xmlp_get_nodeset (xmlpContext *cntxt, gchar *xpath)
{
    xmlXPathObjectPtr result;

    result = xmlXPathEvalExpression((xmlChar *)xpath, cntxt->context);
    if(xmlXPathNodeSetIsEmpty(result->nodesetval))
	return NULL;

    return result;
}

void
xmlp_free (xmlpContext *cntxt)
{
    xmlXPathFreeContext(cntxt->context);
    xmlFreeDoc(cntxt->doc);
    g_free(cntxt->name);
    g_free(cntxt->type);
    g_free(cntxt);
    xmlCleanupParser();
}

inline void
xmlp_free_string (gchar *str)
{
    xmlFree((xmlChar *)str);
}

/* These fucntions return only the first value occurens in the case of multiple */
xmlChar*
xmlp_get (xmlpContext *cntxt, const gchar *xpath, const gchar *type, const gchar *name)
{
    xmlXPathObjectPtr	result;
    xmlNodeSetPtr	nodeset;
    xmlChar		*keyword;
    gchar		*path;
    
    path = g_strconcat("//", xpath, type, "[attribute::name=\"", name, "\"]", NULL);
    result = xmlp_get_nodeset(cntxt, path);
    g_free(path);

    if(result) {
	nodeset = result->nodesetval;
	keyword = xmlNodeListGetString(cntxt->doc, nodeset->nodeTab[0]->xmlChildrenNode, 1);
	xmlXPathFreeObject (result);
	
	return keyword;
    }
    
    return NULL;
}

inline gchar*
xmlp_get_string (xmlpContext *cntxt, const gchar *xpath, const gchar *name)
{
    return (gchar *)xmlp_get(cntxt, xpath, "string", name);
}

gchar*
xmlp_get_string_default (xmlpContext *cntxt, const gchar *xpath, const gchar *name, const gchar *deflt)
{
    gchar	*str;
    
    if((str = xmlp_get_string(cntxt, xpath, name)))
	return str;
    else {
	str = (gchar *)xmlStrdup((xmlChar *)deflt);
	return str;
    }
}

gchar*
xmlp_get_set_string_default (xmlpContext *cntxt, const gchar *xpath, const gchar *name, const gchar *deflt)
{
    gchar	*str;
    
    if((str = xmlp_get_string(cntxt, xpath, name)))
	return str;
    else {
	str = (gchar *)xmlStrdup((xmlChar *)deflt);
	xmlp_set_string(cntxt, xpath, name, str);
	return str;
    }
}

inline gboolean
xmlp_get_int (xmlpContext *cntxt, const gchar *xpath, const gchar *name, gint *value)
{
    gchar	*strvalue;
    
    if((strvalue = (gchar *)xmlp_get(cntxt, xpath, "int", name))) {
	*value = atoi(strvalue);
	xmlp_free_string(strvalue);
	return TRUE;
    }
    
    return FALSE;
}

inline gint
xmlp_get_int_default (xmlpContext *cntxt, const gchar *xpath, const gchar *name, const gint value)
{
    gint	result = value;

    xmlp_get_int (cntxt, xpath, name, &result);
    return result;
}

inline gboolean
xmlp_get_double (xmlpContext *cntxt, const gchar *xpath, const gchar *name, gdouble *value)
{
    gchar	*strvalue;
    
    if((strvalue = (gchar *)xmlp_get(cntxt, xpath, "double", name))) {
	*value = strtod(strvalue, NULL);
	xmlp_free_string(strvalue);
	return TRUE;
    }
    
    return FALSE;
}

inline gdouble
xmlp_get_double_default (xmlpContext *cntxt, const gchar *xpath, const gchar *name, const gdouble value)
{
    gdouble	result = value;

    xmlp_get_double (cntxt, xpath, name, &result);
    return result;
}

inline gboolean
xmlp_get_boolean (xmlpContext *cntxt, const gchar *xpath, const gchar *name, gboolean *value)
{
    gchar	*strvalue;
    
    if((strvalue = (gchar *)xmlp_get(cntxt, xpath, "bool", name))) {
	if(!xmlStrcasecmp(strvalue, "true"))
	    *value = TRUE;
	else
	    *value = FALSE;
	xmlp_free_string(strvalue);
	return TRUE;
    }
    
    return FALSE;
}

inline gboolean
xmlp_get_boolean_default (xmlpContext *cntxt, const gchar *xpath, const gchar *name, const gboolean value)
{
    gboolean	result = value;

    xmlp_get_boolean (cntxt, xpath, name, &result);
    return result;
}

static xmlNodePtr
lookup_node (xmlNodePtr cur, const gchar *name, const gchar *aname, const gchar *avalue)
{
    while(cur) {
	if(!xmlStrcasecmp(cur->name, (const xmlChar *)name)) {
	    if(aname) {
		if(!xmlStrcasecmp(xmlGetProp(cur, (const xmlChar *)aname), avalue))
		    return cur;
	    } else
		return cur;
	}

	cur = cur->next;
    }
    
    return NULL;
}

void
xmlp_set (xmlpContext *cntxt, const gchar *xpath, const gchar *type, const gchar *name, const gchar *value)
{
    xmlXPathObjectPtr	result;
    xmlNodeSetPtr	nodeset;
    gchar		*path;
    gchar		**path_split, **na;
    xmlNodePtr		cur, next, sub;
    
    path = g_strconcat("/", cntxt->type, "/", xpath, type, "[attribute::name=\"", name, "\"]", NULL);
    result = xmlp_get_nodeset(cntxt, path);

    if(result) { /* The value already presents */
	nodeset = result->nodesetval;
	xmlNodeSetContent(nodeset->nodeTab[0], (xmlChar *)value);
	xmlXPathFreeObject (result);
	g_free(path);
    } else { /* We need to create several nodes */
	guint		i = 1; /* The 0-th and 1-st element are empty */
	gboolean	exist = TRUE;

	path_split = g_strsplit(path, "/", 0);
	cur = xmlDocGetRootElement(cntxt->doc);

	while(path_split[i]) {
	    gchar *av = NULL;
	    gchar **attr = NULL;
	    
	    na = g_strsplit(path_split[i], "[attribute::", 0);
	    if(na[1]) {
		attr = g_strsplit(na[1], "=\"", -1);
		av = g_strndup(attr[1], strlen(attr[1]) - 2);
	    }

	    if(exist) {
		if((next = lookup_node(cur, na[0], attr ? attr[0] : NULL, av))) {
		    sub = next;
		    cur = next->xmlChildrenNode;
		} else {
		    exist = FALSE;
		    cur = sub;
		}
	    }

	    if(!exist) {
		next = xmlNewTextChild(cur, NULL, na[0], "");
		if(attr)
		    xmlNewProp(next, attr[0], av);

		cur = next;
	    }

	    g_strfreev(na);
	    if(attr) {
		g_free(av);
		g_strfreev(attr);
	    }
	    i++;
	}
	
	xmlNodeSetContent(cur, (xmlChar*)value);
	
	g_strfreev(path_split);
    }
}

inline void
xmlp_set_string (xmlpContext *cntxt, const gchar *xpath, const gchar *name, const gchar *value)
{
    xmlp_set(cntxt, xpath, "string", name, value);
}

inline void
xmlp_set_int (xmlpContext *cntxt, const gchar *xpath, const gchar *name, gint value)
{
    g_sprintf(buf, "%i", value);
    xmlp_set(cntxt, xpath, "int", name, buf);
}

inline void
xmlp_set_double (xmlpContext *cntxt, const gchar *xpath, const gchar *name, gdouble value)
{
    g_sprintf(buf, "%f", value);
    xmlp_set(cntxt, xpath, "double", name, buf);
}

inline void
xmlp_set_boolean (xmlpContext *cntxt, const gchar *xpath, const gchar *name, gboolean value)
{
    xmlp_set(cntxt, xpath, "bool", name, value ? "TRUE" : "FALSE");
}

void
xmlp_sync (xmlpContext *cntxt)
{
    xmlSaveFormatFile(cntxt->name, cntxt->doc, 1);
}
