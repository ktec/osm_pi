/******************************************************************************
 * $Id: osmdb.h,v 1.0 2011/02/26 01:54:37 ktec Exp $
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

#ifndef _OSMDB_H_
#define _OSMDB_H_

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#include <wx/stdpaths.h>
#include <wx/fileconf.h>
#include <wx/filename.h>

//#include <readosm.h>
#include "readosm/readosm.h"

//#include <spatialite/gaiageo.h>
//#include <spatialite.h>
#include "libspatialite-amalgamation-3.0.1/headers/spatialite/sqlite3.h"
#include "libspatialite-amalgamation-3.0.1/headers/spatialite/gaiageo.h"
#include "libspatialite-amalgamation-3.0.1/headers/spatialite.h"

#define DATABASE_NAME "osm.sqlite"

struct aux_params
{
/* an auxiliary struct used for XML parsing */
    sqlite3 *db_handle;
    sqlite3_stmt *select_nodes_stmt;
    sqlite3_stmt *ins_nodes_stmt;
    sqlite3_stmt *ins_node_tags_stmt;
    sqlite3_stmt *ins_ways_stmt;
    sqlite3_stmt *ins_way_tags_stmt;
    sqlite3_stmt *ins_way_refs_stmt;
    sqlite3_stmt *ins_relations_stmt;
    sqlite3_stmt *ins_relation_tags_stmt;
    sqlite3_stmt *ins_relation_refs_stmt;
    int wr_nodes;
    int wr_node_tags;
    int wr_ways;
    int wr_way_tags;
    int wr_way_refs;
    int wr_relations;
    int wr_rel_tags;
    int wr_rel_refs;
};

class OsmDb
{
    public:

        OsmDb();
        ~OsmDb();

        // ReadOSM stuff
        struct aux_params m_params;
        void ConsumeOsm(const char *osm_path);

    private:

        static int consume_node (const void *user_data, const readosm_node * node);
        static int consume_way (const void *user_data, const readosm_way * way);
        static int consume_relation (const void *user_data, const readosm_relation * relation);

        // Database stuff
        wxString m_dbpath;
        sqlite3 *m_database;
        sqlite3_stmt *m_stmt;
        int ret;
        char *err_msg;
        bool b_dbUsable;

        static int insert_node (struct aux_params *params, const readosm_node * node);
        static int insert_way (struct aux_params *params, const readosm_way * way);
        static int insert_relation (struct aux_params *params, const readosm_relation * relation);

        static int select_nodes (struct aux_params *params, 
            double lat, double lon, double lat_max, double lon_max);

        static void begin_sql_transaction (struct aux_params *params);
        static void commit_sql_transaction (struct aux_params *params);
        static void finalize_sql_stmts (struct aux_params *params);
        static void create_sql_stmts (struct aux_params *params, int journal_off);
        static void spatialite_autocreate (sqlite3 * db);
        static void open_db (const char *path, sqlite3 ** handle, int cache_size);

};

#endif
