/******************************************************************************
 * $Id: osm_pi.h,v 1.0 2011/02/26 01:54:37 ktec Exp $
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

#ifndef _OSMPI_H_
#define _OSMPI_H_

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
  #include <wx/glcanvas.h>
#endif //precompiled headers

#include <wx/fileconf.h>
#include <wx/hashmap.h>

#define     PLUGIN_VERSION_MAJOR    0
#define     PLUGIN_VERSION_MINOR    2

#define     MY_API_VERSION_MAJOR    1
#define     MY_API_VERSION_MINOR    8

//World Mercator
#define PROJECTION 3395

#include "../../../include/ocpn_plugin.h"
#include "osmgui_impl.h"

class OsmDlg;

//----------------------------------------------------------------------------------------------------------
//    The PlugIn Class Definition
//----------------------------------------------------------------------------------------------------------

#define OSM_TOOL_POSITION    -1          // Request default positioning of toolbar tool

class osm_pi : public opencpn_plugin_18
{
public:
      osm_pi(void *ppimgr);

//    The required PlugIn Methods
      int Init(void);
      bool DeInit(void);
      
      int GetAPIVersionMajor();
      int GetAPIVersionMinor();
      int GetPlugInVersionMajor();
      int GetPlugInVersionMinor();
      wxBitmap *GetPlugInBitmap();
      wxString GetCommonName();
      wxString GetShortDescription();
      wxString GetLongDescription();

//    The required override PlugIn Methods
      int GetToolbarToolCount(void);
      void ShowPreferencesDialog( wxWindow* parent );

      void OnToolbarToolCallback(int id);

//    Optional plugin overrides
      void SetColorScheme(PI_ColorScheme cs);

//    The override PlugIn Methods
      bool RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp);
      //bool RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp);

//    Other public methods
      void SetOsmDialogX    (int x){ m_osm_dialog_x = x;};
      void SetOsmDialogY    (int x){ m_osm_dialog_y = x;}

      void OnOsmDialogClose();

private:

      wxFileConfig     *m_pconfig;
      wxWindow         *m_parent_window;
      bool              LoadConfig(void);
      bool              SaveConfig(void);

      double            m_lat, m_lon;
      wxDateTime        m_lastPosReport;

      bool PointInLLBox(PlugIn_ViewPort *vp, double x, double y);

      OsmDlg            *m_pOsmDialog;

      int               m_osm_dialog_x, m_osm_dialog_y;
      int               m_display_width, m_display_height;
      bool              m_bRenderOverlay;
      int               m_iOpacity;
      int               m_iUnits;

      int               m_leftclick_tool_id;

      int               dbGetIntNotNullValue(wxString sql);
      wxString          dbGetStringValue(wxString sql);
};

#endif
