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
#endif //precompiled headers

#define     PLUGIN_VERSION_MAJOR    0
#define     PLUGIN_VERSION_MINOR    2

#define     MY_API_VERSION_MAJOR    1
#define     MY_API_VERSION_MINOR    8

#include <wx/aui/aui.h>
#include <wx/fileconf.h>
#include "../../../include/ocpn_plugin.h"
#include "ui.h"

//World Mercator
#define PROJECTION 3395
#define OSM_TOOL_POSITION -1  // Request default positioning of toolbar tool
//WX_DECLARE_STRING_HASH_MAP( wxString, TagList );
//WX_DEFINE_ARRAY(double, NodeRefList);
//WX_DECLARE_STRING_HASH_MAP( wxString, SeamarkType );

//----------------------------------------------------------------------------------------------------------
//    The PlugIn Class Definition
//----------------------------------------------------------------------------------------------------------

class osm_pi : public opencpn_plugin_18
{
public:
    osm_pi( void *ppimgr );

    //    The required PlugIn Methods
    int Init( void );
    bool DeInit( void );

    int GetAPIVersionMajor();
    int GetAPIVersionMinor();
    int GetPlugInVersionMajor();
    int GetPlugInVersionMinor();
    wxBitmap *GetPlugInBitmap();
    wxString GetCommonName();
    wxString GetShortDescription();
    wxString GetLongDescription();

    //    The required override PlugIn Methods
    int GetToolbarToolCount( void );
    void OnToolbarToolCallback( int id );

    //    Optional plugin overrides
    void SetColorScheme( PI_ColorScheme cs );
    void ShowPreferencesDialog( wxWindow* parent );

    //    The override PlugIn Methods
    bool RenderOverlay( wxDC &dc, PlugIn_ViewPort *vp );
    bool RenderGLOverlay( wxGLContext *pcontext, PlugIn_ViewPort *vp );

private:
    bool LoadConfig( void );
    bool SaveConfig( void );

    wxFileConfig    *m_pconfig;
    wxAuiManager    *m_pauimgr;
    int              m_toolbar_item_id;
    OsmOverlayUI    *m_puserinput;
    static const char* SeamarkTypes[];

};

#endif
