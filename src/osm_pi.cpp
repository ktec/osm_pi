/******************************************************************************
 * $Id: osm_pi.cpp,v 1.0 2011/02/26 01:54:37 ktec Exp $
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
    LoadConfig();


    m_pDownloader = new OsmDownloader();
    m_pOsmDb = new OsmDb();

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
    
    delete m_pOsmDb;
    delete m_pDownloader;
    
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
    OsmCfgDlg *dialog = new OsmCfgDlg( parent, wxID_ANY, _("OSM Preferences"), 
        wxPoint( m_osm_dialog_x, m_osm_dialog_y), wxDefaultSize, wxDEFAULT_DIALOG_STYLE );
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
}

void osm_pi::DoDrawBitmap( const wxBitmap &bitmap, wxCoord x, wxCoord y, bool usemask )
{
    wxLogMessage (_T("OSM_PI: DoDrawBitmap %i,%i"),x,y);

      if ( m_pdc ) {
            m_pdc->DrawBitmap( bitmap, x, y, usemask );
      } else {
            // GL doesn't draw anything if x<0 || y<0 so we must crop image first
            wxBitmap bmp;
            if ( x < 0 || y < 0 ) {
                  int dx = (x < 0 ? -x : 0);
                  int dy = (y < 0 ? -y : 0);
                  int w = bitmap.GetWidth()-dx;
                  int h = bitmap.GetHeight()-dy;
                  /* picture is out of viewport */
                  if ( w <= 0 || h <= 0 )
                        return;
                  wxBitmap newBitmap = bitmap.GetSubBitmap( wxRect( dx, dy, w, h ) );
                  x += dx;
                  y += dy;
                  bmp = newBitmap;
            } else {
                  bmp = bitmap;
            }
            wxImage image = bmp.ConvertToImage();
            int w = image.GetWidth(), h = image.GetHeight();

            if ( usemask ) {
                  unsigned char *d = image.GetData();
                  unsigned char *a = image.GetAlpha();

                  unsigned char mr, mg, mb;
                  if( !image.GetOrFindMaskColour( &mr, &mg, &mb ) && !a )
                        printf("trying to use mask to draw a bitmap without alpha or mask\n");

                  unsigned char *e = new unsigned char[4*w*h];
                  //               int w = image.GetWidth(), h = image.GetHeight();
                  int sb = w*h;
                  unsigned char r, g, b;
                  for ( int i=0 ; i<sb ; i++ ) {
                        r = d[i*3 + 0];
                        g = d[i*3 + 1];
                        b = d[i*3 + 2];

                        e[i*4 + 0] = r;
                        e[i*4 + 1] = g;
                        e[i*4 + 2] = b;

                        e[i*4 + 3] = a ? a[i] :
                        ((r==mr)&&(g==mg)&&(b==mb) ? 0 : 255);
                  }

                  glColor4f( 1, 1, 1, 1 );
                  glEnable( GL_BLEND );
                  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                  glRasterPos2i( x, y );
                  glPixelZoom( 1, -1 );
                  glDrawPixels( w, h, GL_RGBA, GL_UNSIGNED_BYTE, e );
                  glPixelZoom( 1, 1 );
                  glDisable( GL_BLEND );
                  free( e );
            } else {
                  glRasterPos2i( x, y );
                  glPixelZoom( 1, -1 ); /* draw data from top to bottom */
                  glDrawPixels( w, h, GL_RGB, GL_UNSIGNED_BYTE, image.GetData() );
                  glPixelZoom( 1, 1 );
            }
      }
}
bool osm_pi::RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp)
{
    m_pdc = &dc;
//    if (!b_dbUsable || !m_bRenderOverlay)
//       return false;

    double x1 = m_pastVp.lon_min;
    double y1 = m_pastVp.lat_min;
    double x2 = m_pastVp.lon_max;
    double y2 = m_pastVp.lat_max;
    // TODO: Query local database for seamarks
    std::vector<Poi> seamarks;
    m_pOsmDb->SelectNodes(x1,y1,x2,y2,seamarks);

    for(std::vector<Poi>::iterator it = seamarks.begin(); it != seamarks.end(); ++it) {
        wxPoint pl;
        double lat = (*it).latitude;
        double lon = (*it).longitude;
        GetCanvasPixLL(vp, &pl, lat, lon);
        DoDrawBitmap( *_img_osm, pl.x, pl.y, false );
        wxLogMessage (_T("OSM_PI: Vector %i @ latlon[%f,%f] xy[%i,%i]"),(*it).id,lat,lon,pl.x,pl.y);
    }

    return true;
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
    //DownloadUrl(m_api_url);

    double x1 = m_pastVp.lon_min;
    double y1 = m_pastVp.lat_min;
    double x2 = m_pastVp.lon_max;
    double y2 = m_pastVp.lat_max;
    
    bool success = m_pDownloader->Download(x1,y1,x2,y2);
    if (success)
    {
        wxLogMessage (_T("OSM_PI: We have a file to play with...."));
        m_pOsmDb->ConsumeOsm(OsmDownloader::m_osm_path);
    }
}

