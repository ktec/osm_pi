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
            _("Osm"), _T(""), NULL,
             OSM_TOOL_POSITION, 0, this);

      m_pOsmDialog = NULL;

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

void osm_pi::OnToolbarToolCallback(int id)
{
      if(NULL == m_pOsmDialog)
      {
            m_pOsmDialog = new OsmDlg(m_parent_window);
            m_pOsmDialog->plugin = this;
            m_pOsmDialog->Move(wxPoint(m_osm_dialog_x, m_osm_dialog_y));
      }

      m_pOsmDialog->Show(!m_pOsmDialog->IsShown());
}

