/***************************************************************************
 * $Id: ui.h, v0.1 2012-01-20 ktec Exp $
 *
 * Project:  OpenCPN
 * Purpose:  OSM overlay plugin
 * Author:   Keith Salisbury
 *
 ***************************************************************************
 *   Copyright (C) 2012 by Keith Salisbury                                 *
 *   keithsalisbury@gmail.com                                              *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 */

#ifndef _OsmOverlayUI_H_
#define _OsmOverlayUI_H_

#include <wx/wxprec.h>

#ifndef  WX_PRECOMP
  #include <wx/wx.h>
#endif //precompiled headers

#include <wx/checklst.h>
#include "../../../include/ocpn_plugin.h"
#include "db.h"
#include "downloader.h"
#include "factory.h"

class OsmOverlayUI : public wxPanel // must be a wxPanel, not wxWindow so AutoLayout works
{
public:
    OsmOverlayUI( wxWindow *pparent, wxWindowID id, wxString filename );
    ~OsmOverlayUI();

    void SetColorScheme( PI_ColorScheme cs );
    bool RenderOverlay( wxDC &dc, PlugIn_ViewPort *vp );
    bool RenderGLOverlay( wxGLContext *pcontext, PlugIn_ViewPort *vp );

    void AddGroup( wxString group_name, bool visible );
    void AddNode( Node node, bool visible );
    wxString GetSeamarkType( int idx );
    bool GetVisibility( int idx );
    int GetCount();

private:
    void OnListItemSelected( wxCommandEvent& event );
    void OnCheckToggle( wxCommandEvent& event );
    void OnDownload( wxCommandEvent& event );
    void OnStart( wxCommandEvent& event );
    void UpdateButtonsState();

    wxCheckListBox       *m_pCheckListBox;
    wxBitmapButton       *m_pButtonDownload;

    OsmDatabase          *m_pDatabase;
    OsmOverlayFactory    *m_pFactory;
    OsmDownloader        *m_pDownloader;
    PlugIn_ViewPort      m_pViewPort;
    
};

#endif
