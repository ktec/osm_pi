/* 
/ osmxml.c
/
/ OSM-XML (.osm) implementation
/
/ version  1.0, 2012 April 21
/
/ Author: Sandro Furieri a.furieri@lqt.it
/
/ ------------------------------------------------------------------------------
/ 
/ Version: MPL 1.1/GPL 2.0/LGPL 2.1
/ 
/ The contents of this file are subject to the Mozilla Public License Version
/ 1.1 (the "License"); you may not use this file except in compliance with
/ the License. You may obtain a copy of the License at
/ http://www.mozilla.org/MPL/
/ 
/ Software distributed under the License is distributed on an "AS IS" basis,
/ WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
/ for the specific language governing rights and limitations under the
/ License.
/
/ The Original Code is the ReadOSM library
/
/ The Initial Developer of the Original Code is Alessandro Furieri
/ 
/ Portions created by the Initial Developer are Copyright (C) 2012
/ the Initial Developer. All Rights Reserved.
/ 
/ Contributor(s):
/ 
/ Alternatively, the contents of this file may be used under the terms of
/ either the GNU General Public License Version 2 or later (the "GPL"), or
/ the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
/ in which case the provisions of the GPL or the LGPL are applicable instead
/ of those above. If you wish to allow use of your version of this file only
/ under the terms of either the GPL or the LGPL, and not to allow others to
/ use your version of this file under the terms of the MPL, indicate your
/ decision by deleting the provisions above and replace them with the notice
/ and other provisions required by the GPL or the LGPL. If you do not delete
/ the provisions above, a recipient may use your version of this file under
/ the terms of any one of the MPL, the GPL or the LGPL.
/ 
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <expat.h>

#include "readosm.h"
#include "readosm_internals.h"

#if defined(_WIN32)
#define atol_64		_atoi64
#else
#define atol_64		atoll
#endif

#define BUFFSIZE	8192

struct xml_params
{
/* an helper struct supporting XML parsing */
    int current_tag;
    const void *user_data;
    readosm_node_callback node_callback;
    readosm_way_callback way_callback;
    readosm_relation_callback relation_callback;
    readosm_internal_node node;
    readosm_internal_way way;
    readosm_internal_relation relation;
    int stop;
};

static void
xml_init_params (struct xml_params *params, const void *user_data,
		 readosm_node_callback node_fnct, readosm_way_callback way_fnct,
		 readosm_relation_callback relation_fnct, int stop)
{
/* initializing an empty XML helper structure */
    params->current_tag = READOSM_CURRENT_TAG_UNKNOWN;
    params->user_data = user_data;
    params->node_callback = node_fnct;
    params->way_callback = way_fnct;
    params->relation_callback = relation_fnct;

    params->node.id = READOSM_UNDEFINED;
    params->node.latitude = READOSM_UNDEFINED;
    params->node.longitude = READOSM_UNDEFINED;
    params->node.version = READOSM_UNDEFINED;
    params->node.changeset = READOSM_UNDEFINED;
    params->node.user = NULL;
    params->node.uid = READOSM_UNDEFINED;
    params->node.timestamp = NULL;
    params->node.tag_count = 0;
    params->node.first_tag = NULL;
    params->node.last_tag = NULL;

    params->way.id = READOSM_UNDEFINED;
    params->way.version = READOSM_UNDEFINED;
    params->way.changeset = READOSM_UNDEFINED;
    params->way.user = NULL;
    params->way.uid = READOSM_UNDEFINED;
    params->way.timestamp = NULL;
    params->way.ref_count = 0;
    params->way.first_ref = NULL;
    params->way.last_ref = NULL;
    params->way.tag_count = 0;
    params->way.first_tag = NULL;
    params->way.last_tag = NULL;

    params->relation.id = READOSM_UNDEFINED;
    params->relation.version = READOSM_UNDEFINED;
    params->relation.changeset = READOSM_UNDEFINED;
    params->relation.user = NULL;
    params->relation.uid = READOSM_UNDEFINED;
    params->relation.timestamp = NULL;
    params->relation.member_count = 0;
    params->relation.first_member = NULL;
    params->relation.last_member = NULL;
    params->relation.tag_count = 0;
    params->relation.first_tag = NULL;
    params->relation.last_tag = NULL;

    params->stop = stop;
}

static void
xml_reset_params (struct xml_params *params)
{
/* resetting the XML helper structure to initial empty state */
    readosm_internal_tag *tag;
    readosm_internal_tag *tag_n;
    readosm_internal_ref *ref;
    readosm_internal_ref *ref_n;
    readosm_internal_member *member;
    readosm_internal_member *member_n;

    if (params->node.user)
	free (params->node.user);
    if (params->node.timestamp)
	free (params->node.timestamp);
    tag = params->node.first_tag;
    while (tag)
      {
	  tag_n = tag->next;
	  destroy_internal_tag (tag);
	  tag = tag_n;
      }

    if (params->way.user)
	free (params->way.user);
    if (params->way.timestamp)
	free (params->way.timestamp);
    ref = params->way.first_ref;
    while (ref)
      {
	  ref_n = ref->next;
	  destroy_internal_ref (ref);
	  ref = ref_n;
      }
    tag = params->way.first_tag;
    while (tag)
      {
	  tag_n = tag->next;
	  destroy_internal_tag (tag);
	  tag = tag_n;
      }

    if (params->relation.user)
	free (params->relation.user);
    if (params->relation.timestamp)
	free (params->relation.timestamp);
    member = params->relation.first_member;
    while (member)
      {
	  member_n = member->next;
	  destroy_internal_member (member);
	  member = member_n;
      }
    tag = params->relation.first_tag;
    while (tag)
      {
	  tag_n = tag->next;
	  destroy_internal_tag (tag);
	  tag = tag_n;
      }

    xml_init_params (params, params->user_data, params->node_callback,
		     params->way_callback, params->relation_callback,
		     params->stop);
}

static void
xml_start_node (struct xml_params *params, const char **attr)
{
/* an XML Node starts here */
    int i;
    int len;
    xml_reset_params (params);
    for (i = 0; attr[i]; i += 2)
      {
	  if (strcmp (attr[i], "id") == 0)
	      params->node.id = atol_64 (attr[i + 1]);
	  if (strcmp (attr[i], "lat") == 0)
	      params->node.latitude = atof (attr[i + 1]);
	  if (strcmp (attr[i], "lon") == 0)
	      params->node.longitude = atof (attr[i + 1]);
	  if (strcmp (attr[i], "version") == 0)
	      params->node.version = atoi (attr[i + 1]);
	  if (strcmp (attr[i], "changeset") == 0)
	      params->node.changeset = atol_64 (attr[i + 1]);
	  if (strcmp (attr[i], "user") == 0)
	    {
		len = strlen (attr[i + 1]);
		params->node.user = malloc (len + 1);
		strcpy (params->node.user, attr[i + 1]);
	    }
	  if (strcmp (attr[i], "uid") == 0)
	      params->node.uid = atoi (attr[i + 1]);
	  if (strcmp (attr[i], "timestamp") == 0)
	    {
		len = strlen (attr[i + 1]);
		params->node.timestamp = malloc (len + 1);
		strcpy (params->node.timestamp, attr[i + 1]);
	    }
      }
    params->current_tag = READOSM_CURRENT_TAG_IS_NODE;
}

static void
xml_end_node (struct xml_params *params)
{
/* an XML Node ends here */
    if (params->node_callback != NULL && params->stop == 0)
      {
	  int ret =
	      call_node_callback (params->node_callback, params->user_data,
				  &(params->node));
	  if (ret != READOSM_OK)
	      params->stop = 1;
      }
    xml_reset_params (params);
}

static void
xml_start_way (struct xml_params *params, const char **attr)
{
/* an XML Way starts here */
    int i;
    int len;
    xml_reset_params (params);
    for (i = 0; attr[i]; i += 2)
      {
	  if (strcmp (attr[i], "id") == 0)
	      params->way.id = atol_64 (attr[i + 1]);
	  if (strcmp (attr[i], "version") == 0)
	      params->way.version = atoi (attr[i + 1]);
	  if (strcmp (attr[i], "changeset") == 0)
	      params->way.changeset = atol_64 (attr[i + 1]);
	  if (strcmp (attr[i], "user") == 0)
	    {
		len = strlen (attr[i + 1]);
		params->way.user = malloc (len + 1);
		strcpy (params->way.user, attr[i + 1]);
	    }
	  if (strcmp (attr[i], "uid") == 0)
	      params->way.uid = atoi (attr[i + 1]);
	  if (strcmp (attr[i], "timestamp") == 0)
	    {
		len = strlen (attr[i + 1]);
		params->way.timestamp = malloc (len + 1);
		strcpy (params->way.timestamp, attr[i + 1]);
	    }
      }
    params->current_tag = READOSM_CURRENT_TAG_IS_WAY;
}

static void
xml_end_way (struct xml_params *params)
{
/* an XML Way ends here */
    if (params->way_callback != NULL && params->stop == 0)
      {
	  int ret = call_way_callback (params->way_callback, params->user_data,
				       &(params->way));
	  if (ret != READOSM_OK)
	      params->stop = 1;
      }
    xml_reset_params (params);
}

static void
xml_start_relation (struct xml_params *params, const char **attr)
{
/* an XML Relation starts here */
    int i;
    int len;
    xml_reset_params (params);
    for (i = 0; attr[i]; i += 2)
      {
	  if (strcmp (attr[i], "id") == 0)
	      params->relation.id = atol_64 (attr[i + 1]);
	  if (strcmp (attr[i], "version") == 0)
	      params->relation.version = atoi (attr[i + 1]);
	  if (strcmp (attr[i], "changeset") == 0)
	      params->relation.changeset = atol_64 (attr[i + 1]);
	  if (strcmp (attr[i], "user") == 0)
	    {
		len = strlen (attr[i + 1]);
		params->relation.user = malloc (len + 1);
		strcpy (params->relation.user, attr[i + 1]);
	    }
	  if (strcmp (attr[i], "uid") == 0)
	      params->relation.uid = atoi (attr[i + 1]);
	  if (strcmp (attr[i], "timestamp") == 0)
	    {
		len = strlen (attr[i + 1]);
		params->relation.timestamp = malloc (len + 1);
		strcpy (params->relation.timestamp, attr[i + 1]);
	    }
      }
    params->current_tag = READOSM_CURRENT_TAG_IS_RELATION;
}

static void
xml_end_relation (struct xml_params *params)
{
/* an XML Relation ends here */
    if (params->relation_callback != NULL && params->stop == 0)
      {
	  int ret = call_relation_callback (params->relation_callback,
					    params->user_data,
					    &(params->relation));
	  if (ret != READOSM_OK)
	      params->stop = 1;
      }
    xml_reset_params (params);
}

static void
xml_start_xtag (struct xml_params *params, const char **attr)
{
/* an XML Tag starts here */
    readosm_internal_tag *tag;
    int i;
    int len;

    if (params->current_tag == READOSM_CURRENT_TAG_IS_NODE
	|| params->current_tag == READOSM_CURRENT_TAG_IS_WAY
	|| params->current_tag == READOSM_CURRENT_TAG_IS_RELATION)
      {
	  tag = alloc_internal_tag ();
	  for (i = 0; attr[i]; i += 2)
	    {
		if (strcmp (attr[i], "k") == 0)
		  {
		      len = strlen (attr[i + 1]);
		      tag->key = malloc (len + 1);
		      strcpy (tag->key, attr[i + 1]);
		  }
		if (strcmp (attr[i], "v") == 0)
		  {
		      len = strlen (attr[i + 1]);
		      tag->value = malloc (len + 1);
		      strcpy (tag->value, attr[i + 1]);
		  }
	    }
	  if (params->current_tag == READOSM_CURRENT_TAG_IS_NODE)
	    {
		if (params->node.first_tag == NULL)
		    params->node.first_tag = tag;
		if (params->node.last_tag != NULL)
		    params->node.last_tag->next = tag;
		params->node.last_tag = tag;
	    }
	  if (params->current_tag == READOSM_CURRENT_TAG_IS_WAY)
	    {
		if (params->way.first_tag == NULL)
		    params->way.first_tag = tag;
		if (params->way.last_tag != NULL)
		    params->way.last_tag->next = tag;
		params->way.last_tag = tag;
	    }
	  if (params->current_tag == READOSM_CURRENT_TAG_IS_RELATION)
	    {
		if (params->relation.first_tag == NULL)
		    params->relation.first_tag = tag;
		if (params->relation.last_tag != NULL)
		    params->relation.last_tag->next = tag;
		params->relation.last_tag = tag;
	    }
      }
}

static void
xml_start_nd (struct xml_params *params, const char **attr)
{
/* an XML Nd starts here */
    readosm_internal_ref *ref;
    int i;

    if (params->current_tag == READOSM_CURRENT_TAG_IS_WAY)
      {
	  ref = alloc_internal_ref ();
	  for (i = 0; attr[i]; i += 2)
	    {
		if (strcmp (attr[i], "ref") == 0)
		    ref->node_ref = atol_64 (attr[i + 1]);
	    }
	  if (params->way.first_ref == NULL)
	      params->way.first_ref = ref;
	  if (params->way.last_ref != NULL)
	      params->way.last_ref->next = ref;
	  params->way.last_ref = ref;
      }
}

static void
xml_start_member (struct xml_params *params, const char **attr)
{
/* an XML Member starts here */
    readosm_internal_member *member;
    int i;
    int len;

    if (params->current_tag == READOSM_CURRENT_TAG_IS_RELATION)
      {
	  member = alloc_internal_member ();
	  for (i = 0; attr[i]; i += 2)
	    {
		if (strcmp (attr[i], "ref") == 0)
		    member->id = atol_64 (attr[i + 1]);
		if (strcmp (attr[i], "type") == 0)
		  {
		      if (strcmp (attr[i + 1], "node") == 0)
			  member->member_type = READOSM_MEMBER_NODE;
		      if (strcmp (attr[i + 1], "way") == 0)
			  member->member_type = READOSM_MEMBER_WAY;
		      if (strcmp (attr[i + 1], "relation") == 0)
			  member->member_type = READOSM_MEMBER_RELATION;
		  }
		if (strcmp (attr[i], "role") == 0)
		  {
		      len = strlen (attr[i + 1]);
		      member->role = malloc (len + 1);
		      strcpy (member->role, attr[i + 1]);
		  }
	    }
	  if (params->relation.first_member == NULL)
	      params->relation.first_member = member;
	  if (params->relation.last_member != NULL)
	      params->relation.last_member->next = member;
	  params->relation.last_member = member;
      }
}

static void
xml_start_tag (void *data, const char *el, const char **attr)
{
/* some generic XML tag starts here */
    struct xml_params *params = (struct xml_params *) data;
    if (strcmp (el, "node") == 0)
	xml_start_node (params, attr);
    if (strcmp (el, "tag") == 0)
	xml_start_xtag (params, attr);
    if (strcmp (el, "way") == 0)
	xml_start_way (params, attr);
    if (strcmp (el, "nd") == 0)
	xml_start_nd (params, attr);
    if (strcmp (el, "relation") == 0)
	xml_start_relation (params, attr);
    if (strcmp (el, "member") == 0)
	xml_start_member (params, attr);
}

static void
xml_end_tag (void *data, const char *el)
{
/* some generic XML tag ends here */
    struct xml_params *params = (struct xml_params *) data;
    if (strcmp (el, "node") == 0)
	xml_end_node (params);
    if (strcmp (el, "way") == 0)
	xml_end_way (params);
    if (strcmp (el, "relation") == 0)
	xml_end_relation (params);
}

int
parse_osm_xml (readosm_file * input, const void *user_data,
	       readosm_node_callback node_fnct, readosm_way_callback way_fnct,
	       readosm_relation_callback relation_fnct)
{
/* parsing the input file [OSM XML format] */
    XML_Parser parser;
    char xml_buff[BUFFSIZE];
    int done = 0;
    int len;
    struct xml_params params;

    xml_init_params (&params, user_data, node_fnct, way_fnct, relation_fnct, 0);

    parser = XML_ParserCreate (NULL);
    if (!parser)
	return READOSM_CREATE_XML_PARSER_ERROR;

    XML_SetUserData (parser, &params);
    XML_SetElementHandler (parser, xml_start_tag, xml_end_tag);
    while (!done)
      {
	  len = fread (xml_buff, 1, BUFFSIZE, input->in);
	  if (ferror (input->in))
	      return READOSM_READ_ERROR;
	  done = feof (input->in);
	  if (!XML_Parse (parser, xml_buff, len, done))
	      return READOSM_XML_ERROR;
	  if (params.stop)
	      return READOSM_ABORT;
      }
    XML_ParserFree (parser);

    return READOSM_OK;
}

