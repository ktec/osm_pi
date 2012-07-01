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

#include "osm_pi.h"
#include "icons.h"
#include "prefdlg.h"

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

osm_pi::osm_pi(void *ppimgr)
      :opencpn_plugin_18(ppimgr)
{
    // Create the PlugIn icons
    initialize_images();
}

//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

const char* osm_pi::SeamarkTypes[] = { "anchorage","anchor_berth","berth","building","beacon_cardinal","beacon_isolated_danger","beacon_lateral","beacon_safe_water","beacon_special_purpose","buoy_cardinal","buoy_installation","buoy_isolated_danger","buoy_lateral","buoy_safe_water","buoy_special_purpose","cable_area","cable_submarine","causway","coastguard_station","daymark","fog_signal","gate","harbour","landmark","light","light_major","light_minor","light_float","light_vessel","lock_basin","mooring","navigation_line","notice","pile","pilot_boarding","platform","production_area","radar_reflector","radar_transponder","radar_station","radio_station","recommended_track","rescue_station","restricted_area","sandwaves","seabed_area","separation_boundary","separation_crossing","separation_lane","separation_line","separation_roundabout","separation_zone","shoreline_construction","signal_station_traffic","signal_station_warning","small_craft_facility","topmark","wreck",NULL };

int osm_pi::Init(void)
{
    AddLocaleCatalog( _T("opencpn-osm_pi") );

    m_puserinput = NULL;
    //    This PlugIn needs a toolbar icon, so request its insertion
    m_toolbar_item_id  = InsertPlugInTool( _T(""), _img_osm, _img_osm, wxITEM_NORMAL,
        _("OpenSeaMap overlay"), _T(""), NULL, OSM_TOOL_POSITION, 0, this );

    m_pauimgr = GetFrameAuiManager();

    m_puserinput = new OsmOverlayUI( GetOCPNCanvasWindow(), wxID_ANY, _T("") );
    wxAuiPaneInfo pane = wxAuiPaneInfo().Name(_T("OsmOverlay")).Caption(_("OSM overlay")).CaptionVisible(true).Float().FloatingPosition(50,150).Dockable(false).Resizable().CloseButton(true).Show(false);
    m_pauimgr->AddPane( m_puserinput, pane );
    m_pauimgr->Update();

    //    Get a pointer to the opencpn configuration object
    m_pconfig = GetOCPNConfigObject();
    //    And load the configuration items
    LoadConfig();

    return (WANTS_TOOLBAR_CALLBACK    |
          INSTALLS_TOOLBAR_TOOL     |
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
    SaveConfig();
    if ( m_puserinput )
    {
        m_pauimgr->DetachPane( m_puserinput );
        m_puserinput->Close();
        m_puserinput->Destroy();
        m_puserinput = NULL;
    }
    
    return true;
}

//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

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

int osm_pi::GetToolbarToolCount(void)
{
    return 1;
}

void osm_pi::SetColorScheme(PI_ColorScheme cs)
{
    if ( m_puserinput )
    {
        m_puserinput->SetColorScheme( cs );
    }
}

bool osm_pi::LoadConfig(void)
{
    wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

    if(pConf)
    {
        pConf->SetPath ( _T( "/Settings/Osm" ) );
        //pConf->Read ( _T ( "Api Url" ),  &m_sApi_url, API_URL );

        wxString visible;
        const char **np = osm_pi::SeamarkTypes;
        while (*np) 
        {
            wxString seamark_type = wxString::FromAscii(*np++);
            pConf->Read( seamark_type, &visible, _T("Y") );
            m_puserinput->AddSeamarkType( seamark_type, (visible==_T("Y")) );
        }
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
        //pConf->Write ( _T ( "Api Url" ), m_sApi_url );
        const char **np = osm_pi::SeamarkTypes;
        int i = 0;
        while (*np) 
        {
            wxString seamark_type = wxString::FromAscii(*np++);
            pConf->Write( seamark_type, (m_puserinput->GetVisibility( i++ )?_T("Y"):_T("N")) );
        }
        return true;
    }
    else
        return false;
}

void osm_pi::ShowPreferencesDialog( wxWindow* parent )
{
    OsmOverlayPreferencesDialog *dialog = new OsmOverlayPreferencesDialog( parent, wxID_ANY );

    if ( dialog->ShowModal() == wxID_OK )
    {
        // OnClose should handle that for us normally but it doesn't seems to do so
        // We must save changes first
        dialog->SavePreferences();

        SaveConfig();
    }
    dialog->Destroy();
}

bool osm_pi::RenderOverlay( wxDC &dc, PlugIn_ViewPort *vp )
{
    if ( m_puserinput )
    {
        return m_puserinput->RenderOverlay( dc, vp );
    }
    return false;
}

bool osm_pi::RenderGLOverlay( wxGLContext *pcontext, PlugIn_ViewPort *vp )
{
    if ( m_puserinput )
    {
        return m_puserinput->RenderGLOverlay( pcontext, vp );
    }
    return false;
}

void osm_pi::OnToolbarToolCallback(int id)
{
    if ( m_puserinput )
    {
        wxAuiPaneInfo &pane = m_pauimgr->GetPane( m_puserinput );
        if ( pane.IsOk() && !pane.IsShown() )
        {
            pane.Show();
            m_pauimgr->Update();
        }
    }
}

