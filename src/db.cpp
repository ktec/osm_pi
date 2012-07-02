/******************************************************************************
 * $Id: osmdb.cpp,v 1.0 2011/02/26 01:54:37 ktec Exp $
 *
 * Project:  OpenCPN
 * Purpose:  OSM Plugin
 * Author:   Keith Salisbury
 *
 ***************************************************************************
 *   Copyright (C) 2012 by Keith Salisbury                                 *
 *   $EMAIL$                                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 ***************************************************************************
 */

#include "../include/db.h"
#include "../include/osm.h"

static void profile(void *context, const char *sql, sqlite3_uint64 ns) {
    wxLogMessage (_T("OSM_PI: Query: %s"), sql);
    wxLogMessage (_T("OSM_PI: Execution Time: %llu ms"), ns / 1000000);
}

void appendOSDirSlash(wxString* pString)
{
    wxChar sep = wxFileName::GetPathSeparator();
    if (pString->Last() != sep)
        pString->Append(sep);
}

OsmDatabase::OsmDatabase()
{
    // constructor
    //      Establish the location of the database file
    //wxString dbpath; // moved to header

    //      Establish a "home" location
    wxStandardPathsBase& std_path = wxStandardPaths::Get();

    wxString pHome_Locn;
#ifdef __WXMSW__
    pHome_Locn.Append(std_path.GetConfigDir());          // on w98, produces "/windows/Application Data"
#else
    pHome_Locn.Append(std_path.GetUserConfigDir());
#endif
    appendOSDirSlash(&pHome_Locn) ;

#ifdef __WXMSW__
    m_dbpath = _T(DATABASE_NAME);
    m_dbpath.Prepend(pHome_Locn);
#elif defined __WXOSX__
    m_dbpath = std_path.GetUserConfigDir(); // should be ~/Library/Preferences
    appendOSDirSlash(&m_dbpath) ;
    m_dbpath.Append(_T(DATABASE_NAME));
#else
    m_dbpath = std_path.GetUserDataDir(); // should be ~/.opencpn
    appendOSDirSlash(&m_dbpath) ;
    m_dbpath.Append(_T(DATABASE_NAME));
#endif

    wxLogMessage (_T("OSM_PI: Database path %s"), m_dbpath.c_str());

    // initializing the aux-structs
    m_params.db_handle = NULL;
    m_params.select_nodes_stmt = NULL;
    m_params.ins_nodes_stmt = NULL;
    m_params.ins_node_tags_stmt = NULL;
    m_params.ins_ways_stmt = NULL;
    m_params.ins_way_tags_stmt = NULL;
    m_params.ins_way_refs_stmt = NULL;
    m_params.ins_relations_stmt = NULL;
    m_params.ins_relation_tags_stmt = NULL;
    m_params.ins_relation_refs_stmt = NULL;
    m_params.wr_nodes = 0;
    m_params.wr_node_tags = 0;
    m_params.wr_ways = 0;
    m_params.wr_way_tags = 0;
    m_params.wr_way_refs = 0;
    m_params.wr_relations = 0;
    m_params.wr_rel_tags = 0;
    m_params.wr_rel_refs = 0;

//    err_msg = NULL;
//    int db_ver = 1;
    spatialite_init(0);
    // opening the DB
    int cache_size = 0;
    b_dbUsable = OpenDb (m_dbpath.mb_str(), &m_database, cache_size);

    if (!b_dbUsable)
        wxLogMessage (_T("OSM_PI: Database Error"));

    //if (!m_database)
	//return -1;
    m_params.db_handle = m_database;

    int journal_off = 0;
    // creating SQL prepared statements
    CreateSqlStatements (&m_params, journal_off);

}

OsmDatabase::~OsmDatabase()
{
    // destructor
    // finalizing SQL prepared statements
    FinalizeSqlStatements (&m_params);
    
    // closing the DB connection
    if (m_database)
        sqlite3_close (m_database);
    spatialite_cleanup();
}

void OsmDatabase::ConsumeOsm(const char *osm_path)
{
    wxLogMessage (_T("OSM_PI: OnDownloadComplete"));
    //sqlite3 *handle;
    //const char *osm_path = OsmDownloader::m_osm_path;
    //struct aux_params params;
    const void *osm_handle;

    // parsing the input OSM-file
    if (readosm_open (osm_path, &osm_handle) != READOSM_OK)
    {
        wxLogMessage (_T("OSM_PI: cannot open %s"), osm_path);
        readosm_close (osm_handle);
        return;
    }

    // begin transaction
    Exec( m_params.db_handle, _T("BEGIN") );

    wxLogMessage (_T("OSM_PI: Starting osm file parse operation..."));
    if (readosm_parse
	(osm_handle, &m_params, ConsumeNode, ConsumeWay,
	    ConsumeRelation) != READOSM_OK)
    {
        wxLogMessage (_T("OSM_PI: unrecoverable error while parsing %s"), osm_path);
        return;
	}
    readosm_close (osm_handle);
    Exec( m_params.db_handle, _T("COMMIT") );

    wxLogMessage (_T("OSM_PI: Osm file parse complete..."));

    // printing out statistics
    wxLogMessage (_T("OSM_PI: inserted %d nodes"), m_params.wr_nodes);
    wxLogMessage (_T("OSM_PI: \t%d tags"), m_params.wr_node_tags);
    wxLogMessage (_T("OSM_PI: inserted %d ways"), m_params.wr_ways);
    wxLogMessage (_T("OSM_PI: \t%d tags"), m_params.wr_way_tags);
    wxLogMessage (_T("OSM_PI: \t%d node-refs"), m_params.wr_way_refs);
    wxLogMessage (_T("OSM_PI: inserted %d relations"), m_params.wr_relations);
    wxLogMessage (_T("OSM_PI: \t%d tags"), m_params.wr_rel_tags);
    wxLogMessage (_T("OSM_PI: \t%d refs"), m_params.wr_rel_refs);

}

// OSM Consumers
int OsmDatabase::ConsumeNode (const void *user_data, const readosm_node * node)
{
    // processing an OSM Node (ReadOSM callback function)
    struct aux_params *params = (struct aux_params *) user_data;
    if (!InsertNode (params, node))
    	return READOSM_ABORT;
    return READOSM_OK;
}

int OsmDatabase::ConsumeWay (const void *user_data, const readosm_way * way)
{
    // processing an OSM Way (ReadOSM callback function)
    struct aux_params *params = (struct aux_params *) user_data;
    if (!InsertWay (params, way))
    	return READOSM_ABORT;
    return READOSM_OK;
}

int OsmDatabase::ConsumeRelation (const void *user_data, const readosm_relation * relation)
{
    // processing an OSM Relation (ReadOSM callback function)
    struct aux_params *params = (struct aux_params *) user_data;
    if (!InsertRelation (params, relation))
    	return READOSM_ABORT;
    return READOSM_OK;
}

int OsmDatabase::SelectNodes (double lat, double lon, double lat_max, double lon_max, std::vector<Node> &nodes)
{
    wxLogMessage (_T("OSM_PI: SelectNodes %f,%f,%f,%f"),(float)lat,(float)lon,(float)lat_max,(float)lon_max);
    if (!b_dbUsable)
    {
        wxLogMessage (_T("OSM_PI: Error Database unuseable."));
        return -1;
    }

    sqlite3_reset (m_params.select_nodes_stmt);
    sqlite3_clear_bindings (m_params.select_nodes_stmt);
    
	sqlite3_bind_double (m_params.select_nodes_stmt, 1, (float)lat);
	sqlite3_bind_double (m_params.select_nodes_stmt, 2, (float)lon);
	sqlite3_bind_double (m_params.select_nodes_stmt, 3, (float)lat_max);
	sqlite3_bind_double (m_params.select_nodes_stmt, 4, (float)lon_max);

    int ret = sqlite3_step (m_params.select_nodes_stmt);

    int count = 0;
    while (ret == SQLITE_ROW) {
        Node node;
        node.id = sqlite3_column_int64(m_params.select_nodes_stmt, 0);
        node.seamark_type = SeamarkTypes.find(sqlite3_column_text(m_params.select_nodes_stmt, 1))->second;
        node.latitude = sqlite3_column_double(m_params.select_nodes_stmt, 2);
        node.longitude = sqlite3_column_double(m_params.select_nodes_stmt, 3);
	    nodes.push_back(node);
        count++;
        ret = sqlite3_step (m_params.select_nodes_stmt);
    }

    if (ret == SQLITE_DONE)
       ; // wxLogMessage (_T("OSM_PI: select nodes complete"));
    else
    {
        wxLogMessage (_T("OSM_PI: sqlite3_step() error: SELECT osm_nodes"));
        return 0;
    }
    return 1;
}

int OsmDatabase::InsertNode (struct aux_params *params, const readosm_node * node)
{
    //wxLogMessage (_T("OSM_PI: InsertNode"));
    int ret;
    unsigned char *blob;
    int blob_size;
    int i_tag;
    const readosm_tag *p_tag;
    gaiaGeomCollPtr geom = NULL;
    if (node->longitude != READOSM_UNDEFINED
	            && node->latitude != READOSM_UNDEFINED)
    {
        geom = gaiaAllocGeomColl ();
        geom->Srid = SPATIAL_REFERENCE_ID;
        gaiaAddPointToGeomColl (geom, node->longitude, node->latitude);
    }
    sqlite3_reset (params->ins_nodes_stmt);
    sqlite3_clear_bindings (params->ins_nodes_stmt);
    sqlite3_bind_int64 (params->ins_nodes_stmt, 1, node->id);
    if (node->version == READOSM_UNDEFINED)
    	sqlite3_bind_null (params->ins_nodes_stmt, 2);
    else
    	sqlite3_bind_int64 (params->ins_nodes_stmt, 2, node->version);
    if (node->timestamp == NULL)
    	sqlite3_bind_null (params->ins_nodes_stmt, 3);
    else
    	sqlite3_bind_text (params->ins_nodes_stmt, 3, node->timestamp,
			   strlen (node->timestamp), SQLITE_STATIC);
    if (node->uid == READOSM_UNDEFINED)
    	sqlite3_bind_null (params->ins_nodes_stmt, 4);
    else
    	sqlite3_bind_int64 (params->ins_nodes_stmt, 4, node->uid);
    if (node->user == NULL)
    	sqlite3_bind_null (params->ins_nodes_stmt, 5);
    else
    	sqlite3_bind_text (params->ins_nodes_stmt, 5, node->user,
			   strlen (node->user), SQLITE_STATIC);
    if (node->changeset == READOSM_UNDEFINED)
    	sqlite3_bind_null (params->ins_nodes_stmt, 6);
    else
    	sqlite3_bind_int64 (params->ins_nodes_stmt, 6, node->changeset);
    if (!geom)
    	sqlite3_bind_null (params->ins_nodes_stmt, 7);
    else
    {
        gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
        gaiaFreeGeomColl (geom);
        sqlite3_bind_blob (params->ins_nodes_stmt, 7, blob, blob_size, free);
    }
    ret = sqlite3_step (params->ins_nodes_stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
    {
        wxLogMessage (_T("OSM_PI: sqlite3_step() error: INSERT INTO osm_nodes"));
        return 0;
    }
    params->wr_nodes += 1;

    for (i_tag = 0; i_tag < node->tag_count; i_tag++) {
        p_tag = node->tags + i_tag;
        sqlite3_reset (params->ins_node_tags_stmt);
        sqlite3_clear_bindings (params->ins_node_tags_stmt);
        sqlite3_bind_int64 (params->ins_node_tags_stmt, 1, node->id);
        sqlite3_bind_int (params->ins_node_tags_stmt, 2, i_tag);
        if (p_tag->key == NULL)
            sqlite3_bind_null (params->ins_node_tags_stmt, 3);
        else
            sqlite3_bind_text (params->ins_node_tags_stmt, 3, p_tag->key,
                 strlen (p_tag->key), SQLITE_STATIC);
        if (p_tag->value == NULL)
            sqlite3_bind_null (params->ins_node_tags_stmt, 4);
        else
            sqlite3_bind_text (params->ins_node_tags_stmt, 4, p_tag->value,
                 strlen (p_tag->value), SQLITE_STATIC);
        ret = sqlite3_step (params->ins_node_tags_stmt);
        if (ret == SQLITE_DONE || ret == SQLITE_ROW)
          ;
        else
        {
        wxLogMessage (_T("OSM_PI: sqlite3_step() error: INSERT INTO osm_node_tags"));
        return 0;
        }
        params->wr_node_tags += 1;
    }
    return 1;
}

int OsmDatabase::InsertWay (struct aux_params *params, const readosm_way * way)
{
    //wxLogMessage (_T("OSM_PI: InsertWay"));
    int ret;
    int i_tag;
    int i_ref;
    const readosm_tag *p_tag;
    sqlite3_reset (params->ins_ways_stmt);
    sqlite3_clear_bindings (params->ins_ways_stmt);
    sqlite3_bind_int64 (params->ins_ways_stmt, 1, way->id);
    if (way->version == READOSM_UNDEFINED)
    	sqlite3_bind_null (params->ins_ways_stmt, 2);
    else
    	sqlite3_bind_int64 (params->ins_ways_stmt, 2, way->version);
    if (way->timestamp == NULL)
    	sqlite3_bind_null (params->ins_ways_stmt, 3);
    else
	    sqlite3_bind_text (params->ins_ways_stmt, 3, way->timestamp,
			   strlen (way->timestamp), SQLITE_STATIC);
    if (way->uid == READOSM_UNDEFINED)
    	sqlite3_bind_null (params->ins_ways_stmt, 4);
    else
    	sqlite3_bind_int64 (params->ins_ways_stmt, 4, way->uid);
    if (way->user == NULL)
    	sqlite3_bind_null (params->ins_ways_stmt, 5);
    else
    	sqlite3_bind_text (params->ins_ways_stmt, 5, way->user,
			   strlen (way->user), SQLITE_STATIC);
    if (way->changeset == READOSM_UNDEFINED)
    	sqlite3_bind_null (params->ins_ways_stmt, 6);
    else
    	sqlite3_bind_int64 (params->ins_ways_stmt, 6, way->changeset);
    ret = sqlite3_step (params->ins_ways_stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  wxLogMessage (_T("OSM_PI: sqlite3_step() error: INSERT INTO osm_ways"));
	  return 0;
      }
    params->wr_ways += 1;

    for (i_tag = 0; i_tag < way->tag_count; i_tag++)
      {
	  p_tag = way->tags + i_tag;
	  sqlite3_reset (params->ins_way_tags_stmt);
	  sqlite3_clear_bindings (params->ins_way_tags_stmt);
	  sqlite3_bind_int64 (params->ins_way_tags_stmt, 1, way->id);
	  sqlite3_bind_int (params->ins_way_tags_stmt, 2, i_tag);
	  if (p_tag->key == NULL)
	      sqlite3_bind_null (params->ins_way_tags_stmt, 3);
	  else
	      sqlite3_bind_text (params->ins_way_tags_stmt, 3, p_tag->key,
				 strlen (p_tag->key), SQLITE_STATIC);
	  if (p_tag->value == NULL)
	      sqlite3_bind_null (params->ins_way_tags_stmt, 4);
	  else
	      sqlite3_bind_text (params->ins_way_tags_stmt, 4, p_tag->value,
				 strlen (p_tag->value), SQLITE_STATIC);
	  ret = sqlite3_step (params->ins_way_tags_stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		wxLogMessage (_T("OSM_PI: sqlite3_step() error: INSERT INTO osm_way_tags"));
		return 0;
	    }
	  params->wr_way_tags += 1;
      }

    for (i_ref = 0; i_ref < way->node_ref_count; i_ref++)
      {
	  sqlite3_int64 node_id = *(way->node_refs + i_ref);
	  sqlite3_reset (params->ins_way_refs_stmt);
	  sqlite3_clear_bindings (params->ins_way_refs_stmt);
	  sqlite3_bind_int64 (params->ins_way_refs_stmt, 1, way->id);
	  sqlite3_bind_int (params->ins_way_refs_stmt, 2, i_ref);
	  sqlite3_bind_int64 (params->ins_way_refs_stmt, 3, node_id);
	  ret = sqlite3_step (params->ins_way_refs_stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		wxLogMessage (_T("OSM_PI: sqlite3_step() error: INSERT INTO osm_way_refs"));
		return 0;
	    }
	  params->wr_way_refs += 1;
      }
    return 1;
}

int OsmDatabase::InsertRelation (struct aux_params *params, const readosm_relation * relation)
{
    //wxLogMessage (_T("OSM_PI: InsertRelation"));
    int ret;
    int i_tag;
    int i_member;
    const readosm_tag *p_tag;
    const readosm_member *p_member;
    sqlite3_reset (params->ins_relations_stmt);
    sqlite3_clear_bindings (params->ins_relations_stmt);
    sqlite3_bind_int64 (params->ins_relations_stmt, 1, relation->id);
    if (relation->version == READOSM_UNDEFINED)
    	sqlite3_bind_null (params->ins_relations_stmt, 2);
    else
    	sqlite3_bind_int64 (params->ins_relations_stmt, 2, relation->version);
    if (relation->timestamp == NULL)
    	sqlite3_bind_null (params->ins_relations_stmt, 3);
    else
    	sqlite3_bind_text (params->ins_relations_stmt, 3, relation->timestamp,
			   strlen (relation->timestamp), SQLITE_STATIC);
    if (relation->uid == READOSM_UNDEFINED)
    	sqlite3_bind_null (params->ins_relations_stmt, 4);
    else
    	sqlite3_bind_int64 (params->ins_relations_stmt, 4, relation->uid);
    if (relation->user == NULL)
    	sqlite3_bind_null (params->ins_relations_stmt, 5);
    else
    	sqlite3_bind_text (params->ins_relations_stmt, 5, relation->user,
			   strlen (relation->user), SQLITE_STATIC);
    if (relation->changeset == READOSM_UNDEFINED)
    	sqlite3_bind_null (params->ins_relations_stmt, 6);
    else
    	sqlite3_bind_int64 (params->ins_relations_stmt, 6, relation->changeset);
    ret = sqlite3_step (params->ins_relations_stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
    {
        wxLogMessage (_T("OSM_PI: sqlite3_step() error: INSERT INTO osm_relations"));
        return 0;
    }
    params->wr_relations += 1;

    for (i_tag = 0; i_tag < relation->tag_count; i_tag++)
      {
	  p_tag = relation->tags + i_tag;
	  sqlite3_reset (params->ins_relation_tags_stmt);
	  sqlite3_clear_bindings (params->ins_relation_tags_stmt);
	  sqlite3_bind_int64 (params->ins_relation_tags_stmt, 1, relation->id);
	  sqlite3_bind_int (params->ins_relation_tags_stmt, 2, i_tag);
	  if (p_tag->key == NULL)
	      sqlite3_bind_null (params->ins_relation_tags_stmt, 3);
	  else
	      sqlite3_bind_text (params->ins_relation_tags_stmt, 3, p_tag->key,
				 strlen (p_tag->key), SQLITE_STATIC);
	  if (p_tag->value == NULL)
	      sqlite3_bind_null (params->ins_relation_tags_stmt, 4);
	  else
	      sqlite3_bind_text (params->ins_relation_tags_stmt, 4,
				 p_tag->value, strlen (p_tag->value),
				 SQLITE_STATIC);
	  ret = sqlite3_step (params->ins_relation_tags_stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		wxLogMessage (_T("OSM_PI: sqlite3_step() error: INSERT INTO osm_relation_tags"));
		return 0;
	    }
	  params->wr_rel_tags += 1;
      }

    for (i_member = 0; i_member < relation->member_count; i_member++)
      {
	  p_member = relation->members + i_member;
	  sqlite3_reset (params->ins_relation_refs_stmt);
	  sqlite3_clear_bindings (params->ins_relation_refs_stmt);
	  sqlite3_bind_int64 (params->ins_relation_refs_stmt, 1, relation->id);
	  sqlite3_bind_int (params->ins_relation_refs_stmt, 2, i_member);
	  if (p_member->member_type == READOSM_MEMBER_NODE)
	      sqlite3_bind_text (params->ins_relation_refs_stmt, 3, "N", 1,
				 SQLITE_STATIC);
	  else if (p_member->member_type == READOSM_MEMBER_WAY)
	      sqlite3_bind_text (params->ins_relation_refs_stmt, 3, "W", 1,
				 SQLITE_STATIC);
	  else if (p_member->member_type == READOSM_MEMBER_RELATION)
	      sqlite3_bind_text (params->ins_relation_refs_stmt, 3, "R", 1,
				 SQLITE_STATIC);
	  else
	      sqlite3_bind_text (params->ins_relation_refs_stmt, 3, "?", 1,
				 SQLITE_STATIC);
	  sqlite3_bind_int64 (params->ins_relation_refs_stmt, 4, p_member->id);
	  if (p_member->role == NULL)
	      sqlite3_bind_null (params->ins_relation_refs_stmt, 5);
	  else
	      sqlite3_bind_text (params->ins_relation_refs_stmt, 5,
				 p_member->role, strlen (p_member->role),
				 SQLITE_STATIC);
	  ret = sqlite3_step (params->ins_relation_refs_stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		wxLogMessage (_T("OSM_PI: sqlite3_step() error: INSERT INTO osm_relation_refs"));
		return 0;
	    }
	  params->wr_rel_refs += 1;
      }
    return 1;
}

void OsmDatabase::FinalizeSqlStatements (struct aux_params *params)
{
    wxLogMessage (_T("OSM_PI: FinalizeSqlStatements"));

    if (params->ins_nodes_stmt != NULL)
    	sqlite3_finalize (params->ins_nodes_stmt);
    if (params->ins_node_tags_stmt != NULL)
    	sqlite3_finalize (params->ins_node_tags_stmt);
    if (params->ins_ways_stmt != NULL)
    	sqlite3_finalize (params->ins_ways_stmt);
    if (params->ins_way_tags_stmt != NULL)
    	sqlite3_finalize (params->ins_way_tags_stmt);
    if (params->ins_way_refs_stmt != NULL)
    	sqlite3_finalize (params->ins_way_refs_stmt);
    if (params->ins_relations_stmt != NULL)
    	sqlite3_finalize (params->ins_relations_stmt);
    if (params->ins_relation_tags_stmt != NULL)
    	sqlite3_finalize (params->ins_relation_tags_stmt);
    if (params->ins_relation_refs_stmt != NULL)
    	sqlite3_finalize (params->ins_relation_refs_stmt);

}

void OsmDatabase::CreateSqlStatements (struct aux_params *params, int journal_off)
{
    wxLogMessage (_T("OSM_PI: CreateSqlStatements"));
    wxString sql;
    //int ret;

    if (journal_off)
    {
        /* disabling the journal: unsafe but faster */
        Exec( params->db_handle, _T("PRAGMA journal_mode = OFF") );
    }

    Exec( params->db_handle, _T("BEGIN") );

    sql = _T("SELECT n.node_id, n.timestamp, X(Geometry), Y(Geometry), t.k, t.v ")
          _T("FROM osm_nodes AS n JOIN osm_node_tags AS t ON (t.node_id = n.node_id) ")
          _T("WHERE k='seamark:type' ") 
          //_T("AND n.node_id NOT IN (?)");
          _T("AND MBRContains(BuildMBR(?, ?, ?, ?), Geometry)");
    sqlite3_prepare_v2 (params->db_handle, sql.mb_str(), sql.length(),
			    &params->select_nodes_stmt, NULL);
    //params->select_nodes_stmt = PrepareStatement(params->db_handle,sql);

    sql = _T("INSERT OR REPLACE INTO osm_nodes (node_id, version, timestamp, uid, user, changeset, filtered, Geometry) ")
          _T("VALUES (?, ?, ?, ?, ?, ?, 0, ?)");
    sqlite3_prepare_v2 (params->db_handle, sql.mb_str(), sql.length(),
			    &params->ins_nodes_stmt, NULL);
    //params->ins_nodes_stmt = PrepareStatement(params->db_handle,sql);

    sql = _T("INSERT OR REPLACE INTO osm_node_tags (node_id, sub, k, v) ")
          _T("VALUES (?, ?, ?, ?)");
    sqlite3_prepare_v2 (params->db_handle, sql.mb_str(), sql.length(),
			    &params->ins_node_tags_stmt, NULL);
    //params->ins_node_tags_stmt = PrepareStatement(params->db_handle,sql);

	sql = _T("INSERT OR REPLACE INTO osm_ways (way_id, version, timestamp, uid, user, changeset, filtered) ")
          _T("VALUES (?, ?, ?, ?, ?, ?, 0)");
    sqlite3_prepare_v2 (params->db_handle, sql.mb_str(), sql.length(),
			    &params->ins_ways_stmt, NULL);
    //params->ins_ways_stmt = PrepareStatement(params->db_handle,sql);

    sql = _T("INSERT OR REPLACE INTO osm_way_tags (way_id, sub, k, v) ")
          _T("VALUES (?, ?, ?, ?)");
    sqlite3_prepare_v2 (params->db_handle, sql.mb_str(), sql.length(),
			    &params->ins_way_tags_stmt, NULL);
    //params->ins_way_tags_stmt = PrepareStatement(params->db_handle,sql);

    sql = _T("INSERT OR REPLACE INTO osm_way_refs (way_id, sub, node_id) ")
          _T("VALUES (?, ?, ?)");
    sqlite3_prepare_v2 (params->db_handle, sql.mb_str(), sql.length(),
			    &params->ins_way_refs_stmt, NULL);
    //params->ins_way_refs_stmt = PrepareStatement(params->db_handle,sql);

    sql = _T("INSERT OR REPLACE INTO osm_relations (rel_id, version, timestamp, uid, user, changeset, filtered) ")
          _T("VALUES (?, ?, ?, ?, ?, ?, 0)");
    sqlite3_prepare_v2 (params->db_handle, sql.mb_str(), sql.length(),
			    &params->ins_relations_stmt, NULL);
    //params->ins_relations_stmt = PrepareStatement(params->db_handle,sql);

    sql = _T("INSERT OR REPLACE INTO osm_relation_tags (rel_id, sub, k, v) ")
          _T("VALUES (?, ?, ?, ?)");
    sqlite3_prepare_v2 (params->db_handle, sql.mb_str(), sql.length(),
			    &params->ins_relation_tags_stmt, NULL);
    //params->ins_relation_tags_stmt = PrepareStatement(params->db_handle,sql);

	sql = _T("INSERT OR REPLACE INTO osm_relation_refs (rel_id, sub, type, ref, role) ")
          _T("VALUES (?, ?, ?, ?, ?)");
    sqlite3_prepare_v2 (params->db_handle, sql.mb_str(), sql.length(),
			    &params->ins_relation_refs_stmt, NULL);
    //params->ins_relation_refs_stmt = PrepareStatement(params->db_handle,sql);

    Exec( params->db_handle, _T("COMMIT") );

}

/*
sqlite3_stmt OsmDatabase::PrepareStatement (sqlite3 * db_handle, wxString sql)
{
    sqlite3_stmt *stmt = NULL;
    wxLogMessage (_T("OSM_PI: PrepareStatement: %s"), sql.c_str());
    int ret = sqlite3_prepare_v2 (db_handle, sql.mb_str(), sql.length(),
			    &stmt, NULL);
    if (ret != SQLITE_OK)
    {
        wxLogMessage (_T("OSM_PI: SQL error: %s\n%s"), sql.c_str(),
                                       sqlite3_errmsg (db_handle));
        //Exec( params->db_handle, _T("COMMIT") );
        return NULL;
    }
    return stmt;
}
*/

void OsmDatabase::GetTable(struct aux_params *params, wxString sql, char ***results, int &n_rows, int &n_columns)
{
    int ret = sqlite3_get_table (params->db_handle, sql.mb_str(), results, &n_rows, &n_columns, NULL);
    if (ret != SQLITE_OK)
    {
        wxLogMessage (_T("OSM_PI: SQL error: %s\n%s"), sql.c_str(),
                                       sqlite3_errmsg (params->db_handle));
        //sqlite3_free (err_msg);
    }
}

void OsmDatabase::Exec(sqlite3 * db_handle, wxString sql)
{
    int ret = sqlite3_exec (db_handle, sql.mb_str(), NULL, NULL, NULL);
    if (ret != SQLITE_OK)
    {
        wxLogMessage (_T("OSM_PI: SQL error: %s\n%s"), sql.c_str(),
                                       sqlite3_errmsg (db_handle));
    }
}

void OsmDatabase::SpatialiteAutocreate (sqlite3 * db)
{
    wxLogMessage (_T("OSM_PI: SpatialiteAutocreate"));
    // attempting to perform self-initialization for a newly created DB
    int ret;
    wxString sql;
    //char sql[1024];
    char *err_msg = NULL;
    int count;
    int i;
    char **results;
    int rows;
    int columns;

    // checking if this DB is really empty
    sql = _T("SELECT Count(*) from sqlite_master");
    ret = sqlite3_get_table (db, sql.mb_str(), &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	return;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	      count = atoi (results[(i * columns) + 0]);
      }
    sqlite3_free_table (results);

    if (count > 0)
	return;

    // all right, it's empty: proceding to initialize
    sql = _T("SELECT InitSpatialMetadata()");
    ret = sqlite3_exec (db, sql.mb_str(), NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  wxLogMessage (_T("OSM_PI: InitSpatialMetadata() error: %s"), err_msg);
	  sqlite3_free (err_msg);
	  return;
      }
}

bool OsmDatabase::OpenDb (const char *path, sqlite3 ** handle, int cache_size)
{
    wxLogMessage (_T("OSM_PI: OpenDb"));
    // opening the DB
    sqlite3 *db_handle;
    int ret;
    wxString sql;
    //char sql[1024];
    //char *err_msg = NULL;
    int spatialite_rs = 0;
    int spatialite_gc = 0;
    int rs_srid = 0;
    int auth_name = 0;
    int auth_srid = 0;
    int ref_sys_name = 0;
    int proj4text = 0;
    int f_table_name = 0;
    int f_geometry_column = 0;
    int coord_dimension = 0;
    int gc_srid = 0;
    int type = 0;
    int spatial_index_enabled = 0;
    const char *name;
    int i;
    char **results;
    int rows;
    int columns;

    *handle = NULL;
    spatialite_init (0);
    wxLogMessage (_T("OSM_PI: SQLite version: %s"), sqlite3_libversion());
    wxLogMessage (_T("OSM_PI: SpatiaLite version: %s"), spatialite_version());

    ret =
	sqlite3_open_v2 (path, &db_handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
    {
        wxLogMessage (_T("OSM_PI: cannot open '%s': %s"), path,
            sqlite3_errmsg (db_handle));
        return false;
    }
      
    sqlite3_profile(db_handle, &profile, NULL);
      
    // see if we've already created the database
    sql = _T("SELECT value FROM settings WHERE key = 'DBVersion'");
    ret = sqlite3_get_table (db_handle, sql.mb_str(), &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	;
	else
	{
	    // get DBVersion
        sqlite3_free_table (results);
        *handle = db_handle;
        return true;
	}
    sqlite3_free_table (results);
    
    // Ensure this is a Spatialite enabled database
    SpatialiteAutocreate (db_handle);
    if (cache_size > 0)
    {
        /* setting the CACHE-SIZE */
        wxLogMessage (_T("OSM_PI: PRAGMA cache_size=%d"), cache_size);
        Exec( db_handle, sql );
    }

    // checking the GEOMETRY_COLUMNS table
    sql = _T("PRAGMA table_info(geometry_columns)");
    ret = sqlite3_get_table (db_handle, sql.mb_str(), &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	goto unknown;
    if (rows < 1)
	;
    else
    {
        for (i = 1; i <= rows; i++) {
            name = results[(i * columns) + 1];
            if (strcasecmp (name, "f_table_name") == 0)
                f_table_name = 1;
            if (strcasecmp (name, "f_geometry_column") == 0)
                f_geometry_column = 1;
            if (strcasecmp (name, "coord_dimension") == 0)
                coord_dimension = 1;
            if (strcasecmp (name, "srid") == 0)
                gc_srid = 1;
            if (strcasecmp (name, "type") == 0)
                type = 1;
            if (strcasecmp (name, "spatial_index_enabled") == 0)
                spatial_index_enabled = 1;
        }
    }
    sqlite3_free_table (results);
    if (f_table_name && f_geometry_column && type && coord_dimension
	&& gc_srid && spatial_index_enabled)
	spatialite_gc = 1;

    // checking the SPATIAL_REF_SYS table
    sql = _T("PRAGMA table_info(spatial_ref_sys)");
    ret = sqlite3_get_table (db_handle, sql.mb_str(), &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	goto unknown;
    if (rows < 1)
	;
    else
    {
        for (i = 1; i <= rows; i++) {
		    name = results[(i * columns) + 1];
		    if (strcasecmp (name, "srid") == 0)
		        rs_srid = 1;
		    if (strcasecmp (name, "auth_name") == 0)
		        auth_name = 1;
		    if (strcasecmp (name, "auth_srid") == 0)
		        auth_srid = 1;
		    if (strcasecmp (name, "ref_sys_name") == 0)
		        ref_sys_name = 1;
		    if (strcasecmp (name, "proj4text") == 0)
		        proj4text = 1;
	    }
    }
    sqlite3_free_table (results);
    if (rs_srid && auth_name && auth_srid && ref_sys_name && proj4text)
	spatialite_rs = 1;

    // verifying the MetaData format
    if (spatialite_gc && spatialite_rs)
	;
    else
	goto unknown;

    // creating the OSM "raw" nodes
    sql = _T("CREATE TABLE osm_nodes (\n")
          _T("node_id INTEGER NOT NULL PRIMARY KEY,\n")
          _T("version INTEGER,\n")
          _T("timestamp TEXT,\n")
          _T("uid INTEGER,\n")
          _T("user TEXT,\n")
          _T("changeset INTEGER,\n")
          _T("filtered INTEGER NOT NULL)\n");
    Exec( db_handle, sql );
    
    sql = _T("SELECT AddGeometryColumn('osm_nodes', 'Geometry', 4326, 'POINT', 'XY')");
    Exec( db_handle, sql );

    // creating the OSM "raw" node tags
    sql = _T("CREATE TABLE osm_node_tags (\n")
          _T("node_id INTEGER NOT NULL,\n")
          _T("sub INTEGER NOT NULL,\n")
          _T("k TEXT,\n")
          _T("v TEXT,\n")
          _T("CONSTRAINT pk_osm_nodetags PRIMARY KEY (node_id, sub),\n")
          _T("CONSTRAINT fk_osm_nodetags FOREIGN KEY (node_id) ")
          _T("REFERENCES osm_nodes (node_id))\n");
    Exec( db_handle, sql );

    // creating the OSM "raw" ways
    sql = _T("CREATE TABLE osm_ways (\n")
          _T("way_id INTEGER NOT NULL PRIMARY KEY,\n")
          _T("version INTEGER,\n")
          _T("timestamp TEXT,\n")
          _T("uid INTEGER,\n")
          _T("user TEXT,\n")
          _T("changeset INTEGER,\n")
          _T("filtered INTEGER NOT NULL)\n");
    Exec( db_handle, sql );

    // creating the OSM "raw" way tags
    sql = _T("CREATE TABLE osm_way_tags (\n")
          _T("way_id INTEGER NOT NULL,\n")
          _T("sub INTEGER NOT NULL,\n")
          _T("k TEXT,\n")
          _T("v TEXT,\n")
          _T("CONSTRAINT pk_osm_waytags PRIMARY KEY (way_id, sub),\n")
          _T("CONSTRAINT fk_osm_waytags FOREIGN KEY (way_id) ")
          _T("REFERENCES osm_ways (way_id))\n");
    Exec( db_handle, sql );

    // creating the OSM "raw" way-node refs
    sql = _T("CREATE TABLE osm_way_refs (\n")
          _T("way_id INTEGER NOT NULL,\n")
          _T("sub INTEGER NOT NULL,\n")
          _T("node_id INTEGER NOT NULL,\n")
          _T("CONSTRAINT pk_osm_waynoderefs PRIMARY KEY (way_id, sub),\n")
          _T("CONSTRAINT fk_osm_waynoderefs FOREIGN KEY (way_id) ")
          _T("REFERENCES osm_ways (way_id))\n");
    Exec( db_handle, sql );

    // creating an index supporting osm_way_refs.node_id
    sql = _T("CREATE INDEX idx_osm_ref_way ON osm_way_refs (node_id)");
    Exec( db_handle, sql );

    // creating the OSM "raw" relations
    sql = _T("CREATE TABLE osm_relations (\n")
          _T("rel_id INTEGER NOT NULL PRIMARY KEY,\n")
          _T("version INTEGER,\n")
          _T("timestamp TEXT,\n")
          _T("uid INTEGER,\n")
          _T("user TEXT,\n")
          _T("changeset INTEGER,\n")
          _T("filtered INTEGER NOT NULL)\n");
    Exec( db_handle, sql );

    // creating the OSM "raw" relation tags
    sql = _T("CREATE TABLE osm_relation_tags (\n")
          _T("rel_id INTEGER NOT NULL,\n")
          _T("sub INTEGER NOT NULL,\n")
          _T("k TEXT,\n")
          _T("v TEXT,\n")
          _T("CONSTRAINT pk_osm_reltags PRIMARY KEY (rel_id, sub),\n")
          _T("CONSTRAINT fk_osm_reltags FOREIGN KEY (rel_id) ")
          _T("REFERENCES osm_relations (rel_id))\n");
    Exec( db_handle, sql );

    // creating the OSM "raw" relation-node refs
    sql = _T("CREATE TABLE osm_relation_refs (\n")
          _T("rel_id INTEGER NOT NULL,\n")
          _T("sub INTEGER NOT NULL,\n")
          _T("type TEXT NOT NULL,\n")
          _T("ref INTEGER NOT NULL,\n")
          _T("role TEXT,")
          _T("CONSTRAINT pk_osm_relnoderefs PRIMARY KEY (rel_id, sub),\n")
          _T("CONSTRAINT fk_osm_relnoderefs FOREIGN KEY (rel_id) ")
          _T("REFERENCES osm_relations (rel_id))\n");
    Exec( db_handle, sql );

    // creating an index supporting osm_relation_refs.ref
    sql = _T("CREATE INDEX idx_osm_ref_relation ON osm_relation_refs (type, ref)");
    Exec( db_handle, sql );

    // creating the OSM db settings
    sql = _T("CREATE TABLE settings (\n")
          _T("key TEXT NOT NULL UNIQUE,\n")
          _T("value TEXT)\n");
    Exec( db_handle, sql );
    
    // insert DBversion
    sql = _T("INSERT INTO settings (key, value) VALUES ('DBVersion', '1')");
    Exec( db_handle, sql );

    //db_ver = 1;

    *handle = db_handle;
    return true;

  unknown:
    wxLogMessage (_T("OSM_PI: DB '%s'"), path);
    wxLogMessage (_T("OSM_PI: doesn't seems to contain valid Spatial Metadata ..."));
    wxLogMessage (_T("OSM_PI: Please, initialize Spatial Metadata"));
    return false;
}



