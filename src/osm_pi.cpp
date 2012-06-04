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
//      LoadConfig();

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

      //    This PlugIn needs a toolbar icon, so request its insertion
      m_leftclick_tool_id  = InsertPlugInTool(_T(""), _img_osm, _img_osm, wxITEM_NORMAL,
            _("OpenSeaMap"), _T(""), NULL,
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

//      SaveConfig();
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

void osm_pi::ShowPreferencesDialog( wxWindow* parent )
{
      OsmCfgDlg *dialog = new OsmCfgDlg( parent, wxID_ANY, _("OSM Preferences"), wxPoint( m_osm_dialog_x, m_osm_dialog_y), wxDefaultSize, wxDEFAULT_DIALOG_STYLE );
      dialog->Fit();
      wxColour cl;
      DimeWindow(dialog);
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
      m_overpass_url = wxString::Format(wxT("http://overpass.osm.rambler.ru/cgi/xapi?*[bbox=[%f,%f,%f,%f][seamark:type=*]"), 
            (float)m_pastVp.lon_min, (float)m_pastVp.lat_min, (float)m_pastVp.lon_max, (float)m_pastVp.lat_max);
      wxLogMessage (_T("OSM_PI: SetCurrentViewPort overpass_url = [%s]"), m_overpass_url.c_str());

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
      DownloadUrl(m_overpass_url);
}

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
			ParseOsm(doc.FirstChildElement());
		}
		else
		{
			wxLogMessage (_T("OSM_PI: Failed to load file: %s"), "/tmp/features.xml");
		}
		// cannot enter here in debug mode
		wxLogMessage (_T("OSM_PI: Sweet!! Just downloaded this file: [%s]"), url.c_str());
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
        //wxLogMessage (_T("OSM_PI: ParseOsm [%s]"), el_name.c_str());
        if (el_name == _T("node"))
        {
            int node_id = atoi(el->Attribute("id"));
            float lat = atof(el->Attribute("lat"));
            float lon = atof(el->Attribute("lon"));
            TagList tags = ParseTags(el);
            InsertNode(node_id, lat, lon, tags);
        }
        else if (el_name == _T("way"))
        {
            int way_id = atoi(el->Attribute("id"));
            float lat = atof(el->Attribute("lat"));
            float lon = atof(el->Attribute("lon"));
            TagList tags = ParseTags(el);
            InsertWay(way_id, lat, lon, tags);
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

int osm_pi::InsertNode(int id, double lat, double lon, TagList tags)
{
	wxLogMessage (_T("OSM_PI: InsertNode [%i, %f, %f]"), id, (float)lat, (float)lon);
	// iterate over all the tags
	TagList::iterator it;
	for( it = tags.begin(); it != tags.end(); ++it )
	{
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



