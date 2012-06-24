/******************************************************************************
 * $Id: osm_pi.cpp,v 1.0 2011/02/26 01:54:37 nohal Exp $
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

#include "osm_pi.h"

// the class factories, used to create and destroy instances of the PlugIn
extern "C" DECL_EXP opencpn_plugin* create_pi(void *ppimgr)
{
    return new osm_pi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p)
{
    delete p;
}

void appendOSDirSlash(wxString* pString)
{
    wxChar sep = wxFileName::GetPathSeparator();
    if (pString->Last() != sep)
        pString->Append(sep);
}

// write downloaded data
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}
 

//---------------------------------------------------------------------------------------------------------
//
//    Osm PlugIn Implementation
//
//---------------------------------------------------------------------------------------------------------

#include "icons.h"

//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

osm_pi::osm_pi(void *ppimgr)
      :opencpn_plugin_18(ppimgr)
{
    // Create the PlugIn icons
    initialize_images();
}

int osm_pi::Init(void)
{
//    int db_ver = 1;
    spatialite_init(0);
/* opening the DB */
    int cache_size = 0;
    open_db (m_dbpath.mb_str(), &m_database, cache_size);
    
    err_msg = NULL;
    wxString sql;

    m_bshuttingDown = false;
    m_lat = 999.0;
    m_lon = 999.0;

    AddLocaleCatalog( _T("opencpn-osm_pi") );

    // Set some default private member parameters
    m_osm_dialog_x = 0;
    m_osm_dialog_y = 0;

    ::wxDisplaySize(&m_display_width, &m_display_height);

    //    Get a pointer to the opencpn display canvas, to use as a parent for the POI Manager dialog
    m_parent_window = GetOCPNCanvasWindow();

    //    Get a pointer to the opencpn configuration object
    m_pconfig = GetOCPNConfigObject();

    //    And load the configuration items
    LoadConfig();

    //      Establish the location of the config file
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

      //    This PlugIn needs a toolbar icon, so request its insertion
    m_leftclick_tool_id  = InsertPlugInTool(_T(""), _img_osm, _img_osm, 
        wxITEM_NORMAL,_("OpenSeaMap"), _T(""), NULL,
        OSM_TOOL_POSITION, 0, this);

    m_pOsmDialog = NULL;

    return (WANTS_TOOLBAR_CALLBACK    |
          INSTALLS_TOOLBAR_TOOL     |
          WANTS_CURSOR_LATLON       |
          WANTS_PREFERENCES         |
          WANTS_OVERLAY_CALLBACK    |
          WANTS_ONPAINT_VIEWPORT    |
          //WANTS_OPENGL_OVERLAY_CALLBACK |
          //WANTS_DYNAMIC_OPENGL_OVERLAY_CALLBACK |
          WANTS_CONFIG
       );
}

bool osm_pi::DeInit(void)
{
    m_bshuttingDown = true;
    //    Record the dialog position
    if (NULL != m_pOsmDialog)
    {
        wxPoint p = m_pOsmDialog->GetPosition();
        SetOsmDialogX(p.x);
        SetOsmDialogY(p.y);

        m_pOsmDialog->Close();
        delete m_pOsmDialog;
        m_pOsmDialog = NULL;
    }
    SaveConfig();
/* closing the DB connection */
    if (m_database)
        sqlite3_close (m_database);
    spatialite_cleanup();
    return true;
}

int osm_pi::GetAPIVersionMajor()
{
    return MY_API_VERSION_MAJOR;
}

int osm_pi::GetAPIVersionMinor()
{
    return MY_API_VERSION_MINOR;
}

int osm_pi::GetPlugInVersionMajor()
{
    return PLUGIN_VERSION_MAJOR;
}

int osm_pi::GetPlugInVersionMinor()
{
    return PLUGIN_VERSION_MINOR;
}

wxBitmap *osm_pi::GetPlugInBitmap()
{
    return _img_osm_pi;
}

wxString osm_pi::GetCommonName()
{
    return _("OpenSeaMap");
}

wxString osm_pi::GetShortDescription()
{
    return _("Osm PlugIn for OpenCPN");
}

wxString osm_pi::GetLongDescription()
{
    return _("Retrieves data from the Open Street Map\n\
    database and displays it on the chart.");
}

wxString osm_pi::GetApiUrl(float lon_min, float lat_min, float lon_max, float lat_max)
{
    wxString url;
    url = _("http://open.mapquestapi.com/xapi/api/0.6/*[seamark:type=*][bbox=%f,%f,%f,%f]");
    //url = _("http://www.overpass-api.de/api/xapi?*[bbox=[%f,%f,%f,%f][seamark:type=*]]");
    //url = _("http://overpass.osm.rambler.ru/cgi/xapi?*[bbox=[%f,%f,%f,%f][seamark:type=*]]");

    return wxString::Format(url, lon_min, lat_min, lon_max, lat_max);
}

void osm_pi::SetCursorLatLon(double lat, double lon)
{
    //m_cursor_lon = lon;
    //m_cursor_lat = lat;
    //wxLogMessage (_T("OSM_PI: OnToolbarToolCallback %d,%d\n"), lat,lon);
}

int osm_pi::GetToolbarToolCount(void)
{
    return 1;
}

void osm_pi::SetColorScheme(PI_ColorScheme cs)
{
    if (NULL == m_pOsmDialog)
        return;

    DimeWindow(m_pOsmDialog);
}

bool osm_pi::LoadConfig(void)
{
    wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

    if(pConf)
    {
        pConf->SetPath ( _T( "/Settings/Osm" ) );
        //pConf->Read ( _T ( "Api Url" ),  &m_sApi_url, API_URL );

        m_osm_dialog_x =  pConf->Read ( _T ( "DialogPosX" ), 20L );
        m_osm_dialog_y =  pConf->Read ( _T ( "DialogPosY" ), 20L );

        if((m_osm_dialog_x < 0) || (m_osm_dialog_x > m_display_width))
              m_osm_dialog_x = 5;
        if((m_osm_dialog_y < 0) || (m_osm_dialog_y > m_display_height))
              m_osm_dialog_y = 5;
        return true;
    }
    else
        return false;
}

bool osm_pi::SaveConfig(void)
{
    wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

    if(pConf)
    {
        pConf->SetPath ( _T ( "/Settings/Osm" ) );
    //            pConf->Write ( _T ( "Api Url" ), m_sApi_url );

        pConf->Write ( _T ( "DialogPosX" ),   m_osm_dialog_x );
        pConf->Write ( _T ( "DialogPosY" ),   m_osm_dialog_y );

        return true;
    }
    else
        return false;
}

void osm_pi::ShowPreferencesDialog( wxWindow* parent )
{
    OsmCfgDlg *dialog = new OsmCfgDlg( parent, wxID_ANY, _("OSM Preferences"), wxPoint( m_osm_dialog_x, m_osm_dialog_y), wxDefaultSize, wxDEFAULT_DIALOG_STYLE );
    dialog->Fit();
    wxColour cl;
    DimeWindow(dialog);
    //      dialog->m_tApi_url->SetValue(wxString::Format(wxT("%s"), m_sApi_url));

    if(dialog->ShowModal() == wxID_OK)
    {
    //            m_sApi_url = dialog->m_cpApi_url->GetUrl().GetAsString();
        SaveConfig();
    }

    delete dialog;
}

void osm_pi::SetCurrentViewPort(PlugIn_ViewPort &vp)
{
    if (m_bshuttingDown)
        return;
    if (vp.clat == m_pastVp.clat && vp.clon == m_pastVp.clon && vp.pix_height == m_pastVp.pix_height && vp.pix_width == m_pastVp.pix_width && vp.rotation == m_pastVp.rotation && vp.chart_scale == m_pastVp.chart_scale && 
        vp.lat_max == m_pastVp.lat_max && vp.lat_min == m_pastVp.lat_min && vp.lon_max == m_pastVp.lon_max && vp.lon_min == m_pastVp.lon_min && vp.view_scale_ppm == m_pastVp.view_scale_ppm)
    {
        return; //Prevents event storm killing the responsiveness. At least in course-up it looks needed.
    }
    m_pastVp = vp;
    m_api_url = GetApiUrl(m_pastVp.lon_min, m_pastVp.lat_min, m_pastVp.lon_max, m_pastVp.lat_max);
    //wxLogMessage (_T("OSM_PI: SetCurrentViewPort api_url = [%s]"), m_api_url.c_str());

    /*
    //[seamark:type=
    anchorage|anchor_berth|berth|building|
    beacon_cardinal|beacon_isolated_danger|beacon_lateral|beacon_safe_water|beacon_special_purpose|
    buoy_cardinal|buoy_installation|buoy_isolated_danger|buoy_lateral|buoy_safe_water|buoy_special_purpose|
    cable_area|cable_submarine|causway|coastguard_station|
    daymark|fog_signal|
    gate|harbour|landmark|
    light|light_major|light_minor|light_float|light_vessel|
    lock_basin|
    mooring|navigation_line|notice|
    pile|
    pilot_boarding|
    platform|production_area|
    radar_reflector|radar_transponder|radar_station|radio_station|
    recommended_track|rescue_station|restricted_area|sandwaves|seabed_area|
    separation_boundary|separation_crossing|separation_lane|separation_line|separation_roundabout|separation_zone|
    shoreline_construction|signal_station_traffic|signal_station_warning|small_craft_facility|topmark|wreck]
    */

    // TODO: Query local database for seamarks
}

void osm_pi::OnToolbarToolCallback(int id)
{
    wxLogMessage (_T("OSM_PI: OnToolbarToolCallback\n"));
    /* OSM Dialog (here we can select what types of objects to show)
    if(NULL == m_pOsmDialog)
    {
        m_pOsmDialog = new OsmDlg(m_parent_window);
        m_pOsmDialog->plugin = this;
        m_pOsmDialog->Move(wxPoint(m_osm_dialog_x, m_osm_dialog_y));
    }
    m_pOsmDialog->Show(!m_pOsmDialog->IsShown());
    */
    // TODO: this needs to show a dialog which allow the user
    // to update their local database by pressing a button? Maybe?
    DownloadUrl(m_api_url);
}


//---------------------------------------------------------------------------------------------------------
//
//          Overpass/OSM methods
//
//---------------------------------------------------------------------------------------------------------

const char *osm_pi::m_osm_path = "/tmp/features.osm";

void osm_pi::DownloadUrl(wxString url)
{
    wxLogMessage (_T("OSM_PI: DownloadUrl [%s]"), url.c_str());
    CURL *curl;
    FILE *fp;
    CURLcode res;
    
    curl = curl_easy_init();
    if (curl) {
        fp = fopen(m_osm_path,"wb");
        curl_easy_setopt(curl, CURLOPT_URL, url.mb_str().data() );
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        fclose(fp);
    }

	if(res == CURLE_OK)
	{
        wxLogMessage (_T("OSM_PI: File downloaded"));
	    // Download completed ok
	    OnDownloadComplete();
	    /*
		TiXmlDocument doc( "/tmp/features.xml" );
		bool loadOkay = doc.LoadFile();
		if (loadOkay)
		{
		    wxLogMessage (_T("OSM_PI: Sweet!! Just downloaded this file: [%s]"), url.c_str());
			ParseOsm(doc.FirstChildElement());
		}
		else
		{
			wxLogMessage (_T("OSM_PI: Failed to load file: /tmp/features.xml"));
		}
		*/
	}
	else
	{
		//wxString m_errorString = wxT(curl_easy_strerror(res));
		//wxMessageBox(m_errorString);
		//wxLogMessage (_T("OSM_PI: Dang!! Couldnt download url because %s : [%s]"), m_errorString.c_str(), url.c_str());
	}
}

//---------------------------------------------------------------------------------------------------------
//
//          Database methods
//
//---------------------------------------------------------------------------------------------------------

int osm_pi::OnDownloadComplete()
{
    wxLogMessage (_T("OSM_PI: OnDownloadComplete"));
    //sqlite3 *handle;
    //const char *osm_path = NULL;
    int journal_off = 0;
    //struct aux_params params;
    const void *osm_handle;

/* initializing the aux-structs */
    m_params.db_handle = NULL;
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

    if (!m_database)
	return -1;
    m_params.db_handle = m_database;

/* creating SQL prepared statements */
    create_sql_stmts (&m_params, journal_off);

/* parsing the input OSM-file */
    if (readosm_open (m_osm_path, &osm_handle) != READOSM_OK)
    {
        wxLogMessage (_T("OSM_PI: Cant open file"));
        fprintf (stderr, "cannot open %s\n", m_osm_path);
        finalize_sql_stmts (&m_params);
        readosm_close (osm_handle);
        return -1;
    }

    if (readosm_parse
	(osm_handle, &m_params, consume_node, consume_way,
	    consume_relation) != READOSM_OK)
    {
//        wxLogMessage (_T("OSM_PI: unrecoverable error while parsing %s"), m_osm_path);
        fprintf (stderr, "unrecoverable error while parsing %s\n", m_osm_path);
        finalize_sql_stmts (&m_params);
        readosm_close (osm_handle);
        return -1;
	}
    readosm_close (osm_handle);

/* finalizing SQL prepared statements */
    finalize_sql_stmts (&m_params);

/* printing out statistics */
    printf ("inserted %d nodes\n", m_params.wr_nodes);
    printf ("\t%d tags\n", m_params.wr_node_tags);
    printf ("inserted %d ways\n", m_params.wr_ways);
    printf ("\t%d tags\n", m_params.wr_way_tags);
    printf ("\t%d node-refs\n", m_params.wr_way_refs);
    printf ("inserted %d relations\n", m_params.wr_relations);
    printf ("\t%d tags\n", m_params.wr_rel_tags);
    printf ("\t%d refs\n", m_params.wr_rel_refs);

    return 0;
}

// OSM Consumers
int osm_pi::consume_node (const void *user_data, const readosm_node * node)
{
    wxLogMessage (_T("OSM_PI: processing an OSM Node (ReadOSM callback function)"));
/* processing an OSM Node (ReadOSM callback function) */
    struct aux_params *params = (struct aux_params *) user_data;
    if (!insert_node (params, node))
    	return READOSM_ABORT;
    return READOSM_OK;
}

int osm_pi::consume_way (const void *user_data, const readosm_way * way)
{
    wxLogMessage (_T("OSM_PI: processing an OSM Way (ReadOSM callback function)"));
/* processing an OSM Way (ReadOSM callback function) */
    struct aux_params *params = (struct aux_params *) user_data;
    if (!insert_way (params, way))
    	return READOSM_ABORT;
    return READOSM_OK;
}

int osm_pi::consume_relation (const void *user_data, const readosm_relation * relation)
{
    wxLogMessage (_T("OSM_PI: processing an OSM Relation (ReadOSM callback function)"));
/* processing an OSM Relation (ReadOSM callback function) */
    struct aux_params *params = (struct aux_params *) user_data;
    if (!insert_relation (params, relation))
    	return READOSM_ABORT;
    return READOSM_OK;
}

int osm_pi::insert_node (struct aux_params *params, const readosm_node * node)
{
    wxLogMessage (_T("OSM_PI: insert_node"));
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
	  geom->Srid = 4326;
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
	  fprintf (stderr, "sqlite3_step() error: INSERT INTO osm_nodes\n");
	  return 0;
      }
    params->wr_nodes += 1;

    for (i_tag = 0; i_tag < node->tag_count; i_tag++)
      {
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
		fprintf (stderr,
			 "sqlite3_step() error: INSERT INTO osm_node_tags\n");
		return 0;
	    }
	  params->wr_node_tags += 1;
      }
    return 1;
}

int osm_pi::insert_way (struct aux_params *params, const readosm_way * way)
{
    wxLogMessage (_T("OSM_PI: insert_way"));
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
	  fprintf (stderr, "sqlite3_step() error: INSERT INTO osm_ways\n");
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
		fprintf (stderr,
			 "sqlite3_step() error: INSERT INTO osm_way_tags\n");
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
		fprintf (stderr,
			 "sqlite3_step() error: INSERT INTO osm_way_refs\n");
		return 0;
	    }
	  params->wr_way_refs += 1;
      }
    return 1;
}

int osm_pi::insert_relation (struct aux_params *params, const readosm_relation * relation)
{
    wxLogMessage (_T("OSM_PI: insert_relation"));
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
	  fprintf (stderr, "sqlite3_step() error: INSERT INTO osm_relations\n");
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
		fprintf (stderr,
			 "sqlite3_step() error: INSERT INTO osm_relation_tags\n");
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
		fprintf (stderr,
			 "sqlite3_step() error: INSERT INTO osm_relation_refs\n");
		return 0;
	    }
	  params->wr_rel_refs += 1;
      }
    return 1;
}

void osm_pi::finalize_sql_stmts (struct aux_params *params)
{
    wxLogMessage (_T("OSM_PI: finalize_sql_stmts"));
    int ret;
    char *sql_err = NULL;

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

/* committing the still pending SQL Transaction */
    ret = sqlite3_exec (params->db_handle, "COMMIT", NULL, NULL, &sql_err);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "COMMIT TRANSACTION error: %s\n", sql_err);
	  sqlite3_free (sql_err);
	  return;
      }
}

void osm_pi::create_sql_stmts (struct aux_params *params, int journal_off)
{
    wxLogMessage (_T("OSM_PI: create_sql_stmts"));
    sqlite3_stmt *ins_nodes_stmt;
    sqlite3_stmt *ins_node_tags_stmt;
    sqlite3_stmt *ins_ways_stmt;
    sqlite3_stmt *ins_way_tags_stmt;
    sqlite3_stmt *ins_way_refs_stmt;
    sqlite3_stmt *ins_relations_stmt;
    sqlite3_stmt *ins_relation_tags_stmt;
    sqlite3_stmt *ins_relation_refs_stmt;
    char sql[1024];
    int ret;
    char *sql_err = NULL;

    if (journal_off)
      {
	  /* disabling the journal: unsafe but faster */
	  ret =
	      sqlite3_exec (params->db_handle, "PRAGMA journal_mode = OFF",
			    NULL, NULL, &sql_err);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "PRAGMA journal_mode=OFF error: %s\n",
			 sql_err);
		sqlite3_free (sql_err);
		return;
	    }
      }

/* the complete operation is handled as an unique SQL Transaction */
    ret = sqlite3_exec (params->db_handle, "BEGIN", NULL, NULL, &sql_err);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "BEGIN TRANSACTION error: %s\n", sql_err);
	  sqlite3_free (sql_err);
	  return;
      }
    strcpy (sql,
	    "INSERT OR REPLACE INTO osm_nodes (node_id, version, timestamp, uid, user, changeset, filtered, Geometry) ");
    strcat (sql, "VALUES (?, ?, ?, ?, ?, ?, 0, ?)");
    ret =
	sqlite3_prepare_v2 (params->db_handle, sql, strlen (sql),
			    &ins_nodes_stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql,
		   sqlite3_errmsg (params->db_handle));
	  finalize_sql_stmts (params);
	  return;
      }
    strcpy (sql, "INSERT OR REPLACE INTO osm_node_tags (node_id, sub, k, v) ");
    strcat (sql, "VALUES (?, ?, ?, ?)");
    ret =
	sqlite3_prepare_v2 (params->db_handle, sql, strlen (sql),
			    &ins_node_tags_stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql,
		   sqlite3_errmsg (params->db_handle));
	  finalize_sql_stmts (params);
	  return;
      }
    strcpy (sql,
	    "INSERT OR REPLACE INTO osm_ways (way_id, version, timestamp, uid, user, changeset, filtered) ");
    strcat (sql, "VALUES (?, ?, ?, ?, ?, ?, 0)");
    ret =
	sqlite3_prepare_v2 (params->db_handle, sql, strlen (sql),
			    &ins_ways_stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql,
		   sqlite3_errmsg (params->db_handle));
	  finalize_sql_stmts (params);
	  return;
      }
    strcpy (sql, "INSERT OR REPLACE INTO osm_way_tags (way_id, sub, k, v) ");
    strcat (sql, "VALUES (?, ?, ?, ?)");
    ret =
	sqlite3_prepare_v2 (params->db_handle, sql, strlen (sql),
			    &ins_way_tags_stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql,
		   sqlite3_errmsg (params->db_handle));
	  finalize_sql_stmts (params);
	  return;
      }
    strcpy (sql, "INSERT OR REPLACE INTO osm_way_refs (way_id, sub, node_id) ");
    strcat (sql, "VALUES (?, ?, ?)");
    ret =
	sqlite3_prepare_v2 (params->db_handle, sql, strlen (sql),
			    &ins_way_refs_stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql,
		   sqlite3_errmsg (params->db_handle));
	  finalize_sql_stmts (params);
	  return;
      }
    strcpy (sql,
	    "INSERT OR REPLACE INTO osm_relations (rel_id, version, timestamp, uid, user, changeset, filtered) ");
    strcat (sql, "VALUES (?, ?, ?, ?, ?, ?, 0)");
    ret =
	sqlite3_prepare_v2 (params->db_handle, sql, strlen (sql),
			    &ins_relations_stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql,
		   sqlite3_errmsg (params->db_handle));
	  finalize_sql_stmts (params);
	  return;
      }
    strcpy (sql, "INSERT OR REPLACE INTO osm_relation_tags (rel_id, sub, k, v) ");
    strcat (sql, "VALUES (?, ?, ?, ?)");
    ret =
	sqlite3_prepare_v2 (params->db_handle, sql, strlen (sql),
			    &ins_relation_tags_stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql,
		   sqlite3_errmsg (params->db_handle));
	  finalize_sql_stmts (params);
	  return;
      }
    strcpy (sql,
	    "INSERT OR REPLACE INTO osm_relation_refs (rel_id, sub, type, ref, role) ");
    strcat (sql, "VALUES (?, ?, ?, ?, ?)");
    ret =
	sqlite3_prepare_v2 (params->db_handle, sql, strlen (sql),
			    &ins_relation_refs_stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql,
		   sqlite3_errmsg (params->db_handle));
	  finalize_sql_stmts (params);
	  return;
      }

    params->ins_nodes_stmt = ins_nodes_stmt;
    params->ins_node_tags_stmt = ins_node_tags_stmt;
    params->ins_ways_stmt = ins_ways_stmt;
    params->ins_way_tags_stmt = ins_way_tags_stmt;
    params->ins_way_refs_stmt = ins_way_refs_stmt;
    params->ins_relations_stmt = ins_relations_stmt;
    params->ins_relation_tags_stmt = ins_relation_tags_stmt;
    params->ins_relation_refs_stmt = ins_relation_refs_stmt;
}

void osm_pi::spatialite_autocreate (sqlite3 * db)
{
    wxLogMessage (_T("OSM_PI: spatialite_autocreate"));
/* attempting to perform self-initialization for a newly created DB */
    int ret;
    char sql[1024];
    char *err_msg = NULL;
    int count;
    int i;
    char **results;
    int rows;
    int columns;

/* checking if this DB is really empty */
    strcpy (sql, "SELECT Count(*) from sqlite_master");
    ret = sqlite3_get_table (db, sql, &results, &rows, &columns, NULL);
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

/* all right, it's empty: proceding to initialize */
    strcpy (sql, "SELECT InitSpatialMetadata()");
    ret = sqlite3_exec (db, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return;
      }
}

void osm_pi::open_db (const char *path, sqlite3 ** handle, int cache_size)
{
    wxLogMessage (_T("OSM_PI: open_db"));
/* opening the DB */
    sqlite3 *db_handle;
    int ret;
    char sql[1024];
    char *err_msg = NULL;
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
    printf ("SQLite version: %s\n", sqlite3_libversion ());
    printf ("SpatiaLite version: %s\n\n", spatialite_version ());

    ret =
	sqlite3_open_v2 (path, &db_handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open '%s': %s\n", path,
		   sqlite3_errmsg (db_handle));
	  return;
      }
      
    // see if we've already created the database
    strcpy (sql, "SELECT value FROM settings WHERE key = 'DBVersion'");
    ret = sqlite3_get_table (db_handle, sql, &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	;
	else
	{
	    // get DBVersion
        sqlite3_free_table (results);
        *handle = db_handle;
        return;
	}
    sqlite3_free_table (results);

    spatialite_autocreate (db_handle);
    if (cache_size > 0)
      {
	  /* setting the CACHE-SIZE */
	  sprintf (sql, "PRAGMA cache_size=%d", cache_size);
	  sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
      }

/* checking the GEOMETRY_COLUMNS table */
    strcpy (sql, "PRAGMA table_info(geometry_columns)");
    ret = sqlite3_get_table (db_handle, sql, &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	goto unknown;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
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

/* checking the SPATIAL_REF_SYS table */
    strcpy (sql, "PRAGMA table_info(spatial_ref_sys)");
    ret = sqlite3_get_table (db_handle, sql, &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	goto unknown;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
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
/* verifying the MetaData format */
    if (spatialite_gc && spatialite_rs)
	;
    else
	goto unknown;

/* creating the OSM "raw" nodes */
    strcpy (sql, "CREATE TABLE osm_nodes (\n");
    strcat (sql, "node_id INTEGER NOT NULL PRIMARY KEY,\n");
    strcat (sql, "version INTEGER,\n");
    strcat (sql, "timestamp TEXT,\n");
    strcat (sql, "uid INTEGER,\n");
    strcat (sql, "user TEXT,\n");
    strcat (sql, "changeset INTEGER,\n");
    strcat (sql, "filtered INTEGER NOT NULL)\n");
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE 'osm_nodes' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return;
      }
    strcpy (sql,
	    "SELECT AddGeometryColumn('osm_nodes', 'Geometry', 4326, 'POINT', 'XY')");
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE 'osm_nodes' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return;
      }
/* creating the OSM "raw" node tags */
    strcpy (sql, "CREATE TABLE osm_node_tags (\n");
    strcat (sql, "node_id INTEGER NOT NULL,\n");
    strcat (sql, "sub INTEGER NOT NULL,\n");
    strcat (sql, "k TEXT,\n");
    strcat (sql, "v TEXT,\n");
    strcat (sql, "CONSTRAINT pk_osm_nodetags PRIMARY KEY (node_id, sub),\n");
    strcat (sql, "CONSTRAINT fk_osm_nodetags FOREIGN KEY (node_id) ");
    strcat (sql, "REFERENCES osm_nodes (node_id))\n");
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE 'osm_node_tags' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return;
      }
/* creating the OSM "raw" ways */
    strcpy (sql, "CREATE TABLE osm_ways (\n");
    strcat (sql, "way_id INTEGER NOT NULL PRIMARY KEY,\n");
    strcat (sql, "version INTEGER,\n");
    strcat (sql, "timestamp TEXT,\n");
    strcat (sql, "uid INTEGER,\n");
    strcat (sql, "user TEXT,\n");
    strcat (sql, "changeset INTEGER,\n");
    strcat (sql, "filtered INTEGER NOT NULL)\n");
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE 'osm_ways' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return;
      }
/* creating the OSM "raw" way tags */
    strcpy (sql, "CREATE TABLE osm_way_tags (\n");
    strcat (sql, "way_id INTEGER NOT NULL,\n");
    strcat (sql, "sub INTEGER NOT NULL,\n");
    strcat (sql, "k TEXT,\n");
    strcat (sql, "v TEXT,\n");
    strcat (sql, "CONSTRAINT pk_osm_waytags PRIMARY KEY (way_id, sub),\n");
    strcat (sql, "CONSTRAINT fk_osm_waytags FOREIGN KEY (way_id) ");
    strcat (sql, "REFERENCES osm_ways (way_id))\n");
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE 'osm_way_tags' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return;
      }
/* creating the OSM "raw" way-node refs */
    strcpy (sql, "CREATE TABLE osm_way_refs (\n");
    strcat (sql, "way_id INTEGER NOT NULL,\n");
    strcat (sql, "sub INTEGER NOT NULL,\n");
    strcat (sql, "node_id INTEGER NOT NULL,\n");
    strcat (sql, "CONSTRAINT pk_osm_waynoderefs PRIMARY KEY (way_id, sub),\n");
    strcat (sql, "CONSTRAINT fk_osm_waynoderefs FOREIGN KEY (way_id) ");
    strcat (sql, "REFERENCES osm_ways (way_id))\n");
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE 'osm_way_refs' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return;
      }
/* creating an index supporting osm_way_refs.node_id */
    strcpy (sql, "CREATE INDEX idx_osm_ref_way ON osm_way_refs (node_id)");
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE INDEX 'idx_osm_node_way' error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  return;
      }
/* creating the OSM "raw" relations */
    strcpy (sql, "CREATE TABLE osm_relations (\n");
    strcat (sql, "rel_id INTEGER NOT NULL PRIMARY KEY,\n");
    strcat (sql, "version INTEGER,\n");
    strcat (sql, "timestamp TEXT,\n");
    strcat (sql, "uid INTEGER,\n");
    strcat (sql, "user TEXT,\n");
    strcat (sql, "changeset INTEGER,\n");
    strcat (sql, "filtered INTEGER NOT NULL)\n");
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE 'osm_relations' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return;
      }
/* creating the OSM "raw" relation tags */
    strcpy (sql, "CREATE TABLE osm_relation_tags (\n");
    strcat (sql, "rel_id INTEGER NOT NULL,\n");
    strcat (sql, "sub INTEGER NOT NULL,\n");
    strcat (sql, "k TEXT,\n");
    strcat (sql, "v TEXT,\n");
    strcat (sql, "CONSTRAINT pk_osm_reltags PRIMARY KEY (rel_id, sub),\n");
    strcat (sql, "CONSTRAINT fk_osm_reltags FOREIGN KEY (rel_id) ");
    strcat (sql, "REFERENCES osm_relations (rel_id))\n");
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE 'osm_relation_tags' error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  return;
      }
/* creating the OSM "raw" relation-node refs */
    strcpy (sql, "CREATE TABLE osm_relation_refs (\n");
    strcat (sql, "rel_id INTEGER NOT NULL,\n");
    strcat (sql, "sub INTEGER NOT NULL,\n");
    strcat (sql, "type TEXT NOT NULL,\n");
    strcat (sql, "ref INTEGER NOT NULL,\n");
    strcat (sql, "role TEXT,");
    strcat (sql, "CONSTRAINT pk_osm_relnoderefs PRIMARY KEY (rel_id, sub),\n");
    strcat (sql, "CONSTRAINT fk_osm_relnoderefs FOREIGN KEY (rel_id) ");
    strcat (sql, "REFERENCES osm_relations (rel_id))\n");
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE 'osm_relation_refs' error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  return;
      }
/* creating an index supporting osm_relation_refs.ref */
    strcpy (sql,
	    "CREATE INDEX idx_osm_ref_relation ON osm_relation_refs (type, ref)");
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE INDEX 'idx_osm_relation' error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  return;
      }

/* creating the OSM db settings */
    strcpy (sql, "CREATE TABLE settings (\n");
    strcat (sql, "key TEXT NOT NULL UNIQUE,\n");
    strcat (sql, "value TEXT)\n");
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE 'settings' error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  return;
      }
    strcpy (sql,
	    "INSERT INTO settings (key, value) VALUES ('DBVersion', '1')");
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT into 'settings' error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return;
      }
    //db_ver = 1;

    *handle = db_handle;
    return;

  unknown:
    fprintf (stderr, "DB '%s'\n", path);
    fprintf (stderr, "doesn't seems to contain valid Spatial Metadata ...\n\n");
    fprintf (stderr, "Please, initialize Spatial Metadata\n\n");
    return;
}


