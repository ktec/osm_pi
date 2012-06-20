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
#define DATABASE_NAME "osm.sqlite"

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#include <wx/stdpaths.h>
#include "osm_pi.h"
#define CURL_STATICLIB
#include <curl/curl.h>
#include <curl/easy.h>

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
      int db_ver = 1;
      spatialite_init(0);
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
      wxString dbpath;

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
      dbpath = _T(DATABASE_NAME);
      dbpath.Prepend(pHome_Locn);
#elif defined __WXOSX__
      dbpath = std_path.GetUserConfigDir(); // should be ~/Library/Preferences
      appendOSDirSlash(&dbpath) ;
      dbpath.Append(_T(DATABASE_NAME));
#else
      dbpath = std_path.GetUserDataDir(); // should be ~/.opencpn
      appendOSDirSlash(&dbpath) ;
      dbpath.Append(_T(DATABASE_NAME));
#endif
      
      bool newDB = !wxFileExists(dbpath);
      b_dbUsable = true;

      ret = sqlite3_open_v2 (dbpath.mb_str(), &m_database, 
                SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
      if (ret != SQLITE_OK)
      {
        wxLogMessage (_T("OSM_PI: cannot open '%s': %s\n"), 
                DATABASE_NAME, sqlite3_errmsg (m_database));
        sqlite3_close (m_database);
        b_dbUsable = false;
      }

      if (newDB && b_dbUsable)
      {
            // TODO: Check if the database is really empty
            // TODO: Set the CACHE-SIZE
            // TODO: Check the GEOMETRY_COLUMNS table      

            //create empty db
            dbQuery(_T("SELECT InitSpatialMetadata()"));
            
            //CREATE OUR TABLES
            // creating the OSM "raw" nodes
            sql = _T("CREATE TABLE osm_nodes (\n")
                  _T("node_id INTEGER NOT NULL PRIMARY KEY,\n")
                  _T("version INTEGER,\n")
                  _T("timestamp TEXT,\n")
                  _T("uid INTEGER,\n")
                  _T("user TEXT,\n")
                  _T("changeset INTEGER,\n")
                  _T("filtered INTEGER NOT NULL)\n");
            dbQuery(sql);
            dbQuery(_T("SELECT AddGeometryColumn('osm_nodes', 'Geometry', 4326, 'POINT', 'XY')"));

            // creating the OSM "raw" node tags
            sql = _T("CREATE TABLE osm_node_tags (\n")
                  _T("node_id INTEGER NOT NULL,\n")
                  _T("sub INTEGER NOT NULL,\n")
                  _T("k TEXT,\n")
                  _T("v TEXT,\n")
                  _T("CONSTRAINT pk_osm_nodetags PRIMARY KEY (node_id, sub),\n")
                  _T("CONSTRAINT fk_osm_nodetags FOREIGN KEY (node_id) ")
                  _T("REFERENCES osm_nodes (node_id))\n");
            dbQuery(sql);

            // creating the OSM "raw" ways
            sql = _T("CREATE TABLE osm_ways (\n")
                  _T("way_id INTEGER NOT NULL PRIMARY KEY,\n")
                  _T("version INTEGER,\n")
                  _T("timestamp TEXT,\n")
                  _T("uid INTEGER,\n")
                  _T("user TEXT,\n")
                  _T("filtered INTEGER NOT NULL)\n");
            dbQuery(sql);

            // creating the OSM "raw" way tags
            sql = _T("CREATE TABLE osm_way_tags (\n")
                  _T("way_id INTEGER NOT NULL,\n")
                  _T("sub INTEGER NOT NULL,\n")
                  _T("k TEXT,\n")
                  _T("v TEXT,\n")
                  _T("CONSTRAINT pk_osm_waytags PRIMARY KEY (way_id, sub),\n")
                  _T("CONSTRAINT fk_osm_waytags FOREIGN KEY (way_id) ")
                  _T("REFERENCES osm_ways (way_id))\n");
            dbQuery(sql);
            
            // creating the OSM "raw" way-node refs
            sql = _T("CREATE TABLE osm_way_refs (\n")
                  _T("way_id INTEGER NOT NULL,\n")
                  _T("sub INTEGER NOT NULL,\n")
                  _T("node_id INTEGER NOT NULL,\n")
                  _T("CONSTRAINT pk_osm_waynoderefs PRIMARY KEY (way_id, sub),\n")
                  _T("CONSTRAINT fk_osm_waynoderefs FOREIGN KEY (way_id) \n")
                  _T("REFERENCES osm_ways (way_id))\n");
            dbQuery(sql);
            
            // creating an index supporting osm_way_refs.node_id
            dbQuery(_T("CREATE INDEX idx_osm_ref_way ON osm_way_refs (node_id)"));
            
            // creating the OSM "raw" relations
            sql = _T("CREATE TABLE osm_relations (\n")
                  _T("rel_id INTEGER NOT NULL PRIMARY KEY,\n")
                  _T("version INTEGER,\n")
                  _T("timestamp TEXT,\n")
                  _T("uid INTEGER,\n")
                  _T("user TEXT,\n")
                  _T("changeset INTEGER,\n")
                  _T("filtered INTEGER NOT NULL)\n");
            dbQuery(sql);
            
            // creating the OSM "raw" relation tags
            sql = _T("CREATE TABLE osm_relation_tags (\n")
                  _T("rel_id INTEGER NOT NULL,\n")
                  _T("sub INTEGER NOT NULL,\n")
                  _T("k TEXT,\n")
                  _T("v TEXT,\n")
                  _T("CONSTRAINT pk_osm_reltags PRIMARY KEY (rel_id, sub),\n")
                  _T("CONSTRAINT fk_osm_reltags FOREIGN KEY (rel_id) ")
                  _T("REFERENCES osm_relations (rel_id))\n");
            dbQuery(sql);
            
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
            dbQuery(sql);

            // creating an index supporting osm_relation_refs.ref
            dbQuery(_T("CREATE INDEX idx_osm_ref_relation ON osm_relation_refs (type, ref)"));

      }

      //Update DB structure and contents
      if (b_dbUsable)
      {
            char **results;
            int n_rows;
            int n_columns;
            sql = _T("SELECT value FROM settings WHERE key = 'DBVersion'");
            ret = sqlite3_get_table (m_database, sql.mb_str(), &results, &n_rows, &n_columns, &err_msg);
            if (ret != SQLITE_OK)
            {
                  sqlite3_free (err_msg);
                  sql = _T("CREATE TABLE settings (")
                  _T("key TEXT NOT NULL UNIQUE,")
                  _T("value TEXT)");
                  dbQuery(sql);
                  dbQuery(_T("INSERT INTO settings (key, value) VALUES ('DBVersion', '1')"));
                  db_ver = 1;
            } 
            else
            {
                  db_ver = atoi(results[1]);
            }
            sqlite3_free_table (results);
            wxLogMessage (_T("OSM_PI: Database version: %i\n"), db_ver);
      }

//      if (b_dbUsable && db_ver == 2)
//      {
//            dbQuery(_T("UPDATE settings SET value = 3 WHERE key = 'DBVersion'"));
//            db_ver = 3;
//      }

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

//            pConf->Read ( _T ( "Api Url" ),  &m_sApi_url, API_URL );

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
//          Database methods
//
//---------------------------------------------------------------------------------------------------------

bool osm_pi::dbQuery(wxString sql)
{
    if (!b_dbUsable)
        return false;
    ret = sqlite3_exec (m_database, sql.mb_str(), NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
    {
        sqlite3_free (err_msg);
        // some error occurred
        wxLogMessage (_T("Database error: %s in query: %s\n"), *err_msg, sql.c_str());
        b_dbUsable = false;
    }
    return b_dbUsable;
}

void osm_pi::dbGetTable(wxString sql, char ***results, int &n_rows, int &n_columns)
{
    ret = sqlite3_get_table (m_database, sql.mb_str(), results, &n_rows, &n_columns, &err_msg);
    if (ret != SQLITE_OK)
    {
        sqlite3_free (err_msg);
        wxLogMessage (_T("Database error: %s in query: %s\n"), *err_msg, sql.c_str());
        b_dbUsable = false;
    } 
}

wxString osm_pi::dbGetStringValue(wxString sql)
{
      char **result;
      int n_rows;
      int n_columns;
      dbGetTable(sql, &result, n_rows, n_columns);
      dbFreeResults(result);
      wxArrayString surveys;
      wxString ret = wxString::FromUTF8(result[1]);
      if(n_rows == 1)
            return ret;
      else
            return wxEmptyString;
}

int osm_pi::dbGetIntNotNullValue(wxString sql)
{
      char **result;
      int n_rows;
      int n_columns;
      dbGetTable(sql, &result, n_rows, n_columns);
      dbFreeResults(result);
      wxArrayString surveys;
      int ret = atoi(result[1]);
      if(n_rows == 1)
            return ret;
      else
            return 0;
}

void osm_pi::dbFreeResults(char **results)
{
      sqlite3_free_table (results);
}

int osm_pi::InsertNode(int id, double lat, double lon, TagList tags)
{
    //wxString time = _T("current_timestamp");
    //if (timestamp > 0)
    //      time = wxDateTime(timestamp).FormatISODate().Append(_T(" ")).Append(wxDateTime(timestamp).FormatISOTime());
    //wxString sql = wxString::Format(_T("INSERT INTO \"osm_nodes\" (\"version\", \"timestamp\", \"uid\", \"user\", \"changeset\", \"Geometry\",) VALUES (%f , %s, %i, %f, GeomFromText('POINT(%f %f)', %i))"), version, time.c_str(), uid, user, changeset, lon, lat, projection);
    //dbQuery (sql);
    //return sqlite3_last_insert_rowid(m_database);

	wxLogMessage (_T("OSM_PI: InsertNode [%i, %f, %f]"), id, (float)lat, (float)lon);
	// iterate over all the tags
	TagList::iterator it;
	for( it = tags.begin(); it != tags.end(); ++it )
	{
        //wxString sql = wxString::Format(_T("INSERT INTO \"osm_node_tags\" (\"node_id\", \"sub\", \"k\", \"v\") VALUES (%i , %s, %s, %s))"), node_id, sub, k, v);
        //dbQuery (sql);
        //return sqlite3_last_insert_rowid(m_database);
		wxString key = it->first, value = it->second;
		wxLogMessage (_T("OSM_PI: InsertNodeTags [%s, %s]"), key.c_str(), value.c_str());
	}
	// insert in the database
	return 0;
}

int osm_pi::InsertWay(int id, double lat, double lon, TagList tags)
{
	wxLogMessage (_T("OSM_PI: InsertWay [%i, %f, %f]"), id, (float)lat, (float)lon);
	// iterate over all the tags
	TagList::iterator it;
	for( it = tags.begin(); it != tags.end(); ++it )
	{
		wxString key = it->first, value = it->second;
		wxLogMessage (_T("OSM_PI: InsertWayTags [%s, %s]"), key.c_str(), value.c_str());
	}
	// insert in the database
	return 0;
}



//---------------------------------------------------------------------------------------------------------
//
//          Overpass/OSM methods
//
//---------------------------------------------------------------------------------------------------------

void osm_pi::DownloadUrl(wxString url)
{
    wxLogMessage (_T("OSM_PI: DownloadUrl [%s]"), url.c_str());
    CURL *curl;
    FILE *fp;
    CURLcode res;
    char outfilename[FILENAME_MAX] = "/tmp/features.xml";
    curl = curl_easy_init();
    if (curl) {
        fp = fopen(outfilename,"wb");
        curl_easy_setopt(curl, CURLOPT_URL, url.mb_str().data() );
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        fclose(fp);
    }

	if(res == CURLE_OK)
	{
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
	}
	else
	{
		//wxString m_errorString = wxT(curl_easy_strerror(res));
		//wxMessageBox(m_errorString);
		//wxLogMessage (_T("OSM_PI: Dang!! Couldnt download url because %s : [%s]"), m_errorString.c_str(), url.c_str());
	}
}

void osm_pi::ParseOsm(TiXmlElement *osm)
{
    TiXmlElement *el = osm->FirstChildElement();
    while (el)
    {
        wxString el_name = wxString::FromUTF8(el->Value());
        wxLogMessage (_T("OSM_PI: ParseOsm [%s]"), el_name.c_str());

        if (el_name == _T("bounds"))
        {
        }
        else if (el_name == _T("node"))
        {
            int id = atoi(el->Attribute("id"));
            int version = atoi(el->Attribute("version"));
            wxString timestamp = wxString::FromUTF8(el->Attribute("timestamp"));
            int uid = atoi(el->Attribute("uid"));
            wxString user = wxString::FromUTF8(el->Attribute("user"));
            int changeset = atoi(el->Attribute("changeset"));
            float lat = atof(el->Attribute("lat"));
            float lon = atof(el->Attribute("lon"));

            //InsertNode(id, lat, lon, tags);
            
	        wxString sql = _T("INSERT INTO osm_nodes (node_id, version, timestamp, uid, user, changeset, filtered, Geometry) ")
                           _T("VALUES (%i, %i, '%s', %i, '%s', %i, 0, GeomFromText('POINT(%f %f)',4326))");
            wxString sql_string = wxString::Format(sql, id, version, timestamp.c_str(), uid, user.c_str(), changeset, lat, lon);
            wxLogMessage (_T("OSM_PI: %s"), sql_string.c_str());
            //dbQuery (sql_string);

            err_msg = NULL;
            ret = sqlite3_exec (m_database, sql_string.mb_str(), NULL, NULL, &err_msg);
            if (ret != SQLITE_OK)
            {
                // some error occurred
                fprintf(stderr, "SQL error: %s\n", err_msg);
                sqlite3_free (err_msg);
            }
            int node_id = sqlite3_last_insert_rowid(m_database);

            //TagList tags = ParseTags(el);
            int tag_id = 0;
	        TiXmlElement *tag_el = el->FirstChildElement();
	        while (tag_el)
	        {
		        wxString tag_el_name = wxString::FromUTF8(tag_el->Value());
		        if (tag_el_name == _T("tag"))
		        {
			        wxString key = wxString::FromUTF8(tag_el->Attribute("k"));
			        wxString value = wxString::FromUTF8(tag_el->Attribute("v"));

	                wxString sql = _T("INSERT INTO osm_node_tags (node_id, sub, k, v) ")
                                   _T("VALUES (%i, %i, '%s', '%s')");
                    wxString sql_string = wxString::Format(sql, node_id, tag_id++, key.c_str(), value.c_str());
                    wxLogMessage (_T("OSM_PI: %s"), sql_string.c_str());

                    err_msg = NULL;
                    ret = sqlite3_exec (m_database, sql_string.mb_str(), NULL, NULL, &err_msg);
                    if (ret != SQLITE_OK)
                    {
                        // some error occurred
                        fprintf(stderr, "SQL error: %s\n", err_msg);
                        sqlite3_free (err_msg);
                    }
		        }
		        tag_el = tag_el->NextSiblingElement();
	        }


        }
        else if (el_name == _T("way"))
        {
            //int id = atoi(el->Attribute("id"));
            //float lat = atof(el->Attribute("lat"));
            //float lon = atof(el->Attribute("lon"));
            //TagList tags = ParseTags(el);

            //InsertWay(id, lat, lon, tags);
        }
        else if (el_name == _T("relation"))
        {
            //int id = atoi(el->Attribute("id"));
            //float lat = atof(el->Attribute("lat"));
            //float lon = atof(el->Attribute("lon"));
            //TagList tags = ParseTags(el);

            //InsertRelation(id, lat, lon, tags);
        }
        el = el->NextSiblingElement();
    }
}

TagList osm_pi::ParseTags(TiXmlElement *osm)
{
	TagList tags;
	TiXmlElement *el = osm->FirstChildElement();
	while (el)
	{
		wxString el_name = wxString::FromUTF8(el->Value());
		if (el_name == _T("tag"))
		{
			wxString key = wxString::FromUTF8(el->Attribute("k"));
			wxString value = wxString::FromUTF8(el->Attribute("v"));
			tags[key] = value;
		}
		el = el->NextSiblingElement();
	}
	return tags;
}
/*
NodeRefList osm_pi::ParseNodeRefs(TiXmlElement *osm)
{
	NodeRefList nodelist;
	TiXmlElement *el = osm->FirstChildElement();
	while (el)
	{
		wxString el_name = wxString::FromUTF8(el->Value());
		if (el_name == _T("nd"))
		{
			wxString ref = wxString::FromUTF8(el->Attribute("ref"));
			nodelist[nodelist->GetCount()] = value;
		}
		el = el->NextSiblingElement();
	}
	return nodelist;
}*/

/*
void osm_pi::ParseOsm(TiXmlElement *osm)
{
      TiXmlElement *parameter = osm->FirstChildElement();
      while (parameter)
      {
            wxString paramname = wxString::FromUTF8(parameter->Value());
            else if (paramname == _T("way"))
            {
//				  <way id="112284730">
//					<nd ref="1276680173"/>
//					<nd ref="1276680131"/>
//					<nd ref="1276680154"/>
//					<tag k="seamark:type" v="separation_line"/>
//				  </way>
            }
            else if (paramname == _T("relation"))
            {
                  //TODO: We do not need this, so let's ignore it for now
            }
            parameter = parameter->NextSiblingElement();
      }
}

int osm_pi::InsertWayTag(int way_id, wxString k, wxString v)
{
	wxLogMessage (_T("OSM_PI: InsertWayTag [%i, %d, %d]"), way_id, k, v);
	// add the tag to the database
	return 0
}

*/



