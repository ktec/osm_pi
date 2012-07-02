/***************************************************************************
 * $Id: ui.cpp, v0.1 2012-01-20 ktec Exp $
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

#include <wx/wxprec.h>

#ifndef  WX_PRECOMP
  #include <wx/wx.h>
#endif //precompiled headers

#include <wx/filename.h>
#include "../include/icons.h"
#include "../include/ui.h"
#include "../include/osm.hpp"

/*    OsmOverlay user interface implementation
 *
 *************************************************************************/

OsmOverlayUI::OsmOverlayUI( wxWindow *pparent, wxWindowID id, wxString filename )
      :wxPanel( pparent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE, _("OSM overlay") )
{
      wxColour cl;
      GetGlobalColor(_T("DILG1"), &cl);
      SetBackgroundColour(cl);

      wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
      SetSizer( topsizer );

      m_pCheckListBox = new wxCheckListBox( this, wxID_ANY, wxDefaultPosition, wxSize(150, 180), 0,
                                         NULL, wxLB_SINGLE );
      m_pCheckListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED,
            wxCommandEventHandler( OsmOverlayUI::OnListItemSelected ), NULL, this );
      m_pCheckListBox->Connect( wxEVT_COMMAND_CHECKLISTBOX_TOGGLED,
            wxCommandEventHandler( OsmOverlayUI::OnCheckToggle ), NULL, this );
      topsizer->Add( m_pCheckListBox, 0, wxEXPAND|wxALL );

      wxBoxSizer *itemBoxSizer01 = new wxBoxSizer( wxHORIZONTAL );
      topsizer->Add( itemBoxSizer01, 0, wxALL );

      m_pButtonDownload = new wxBitmapButton( this, wxID_ANY, *_img_download, wxDefaultPosition, wxDefaultSize );
      itemBoxSizer01->Add( m_pButtonDownload, 0, wxALIGN_CENTER, 2 );
      m_pButtonDownload->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler( OsmOverlayUI::OnDownload ), NULL, this );

      m_pButtonStart = new wxBitmapButton( this, wxID_ANY, *_img_start, wxDefaultPosition, wxDefaultSize );
      itemBoxSizer01->Add( m_pButtonStart, 0, wxALIGN_CENTER, 2 );
      m_pButtonStart->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler( OsmOverlayUI::OnStart ), NULL, this );



      Fit();
      // GetSize() doesn't seems to count aui borders so we add it now.
      wxSize sz = GetSize(); sz.IncBy(10,24);
      SetMinSize(sz);

      UpdateButtonsState();
      m_pDatabase = new OsmDatabase();
      m_pFactory = new OsmOverlayFactory();
      m_pDownloader = new OsmDownloader();
}

OsmOverlayUI::~OsmOverlayUI()
{
      delete m_pFactory;
}

void OsmOverlayUI::SetColorScheme( PI_ColorScheme cs )
{
      wxColour cl;
      GetGlobalColor( _T("DILG1"), &cl );
      SetBackgroundColour( cl );

      Refresh(false);
}

bool OsmOverlayUI::RenderOverlay( wxDC &dc, PlugIn_ViewPort *vp )
{
/*
    if (vp.clat == m_pViewPort.clat && vp.clon == m_pViewPort.clon && vp.pix_height == m_pViewPort.pix_height && vp.pix_width == m_pViewPort.pix_width && vp.rotation == m_pViewPort.rotation && vp.chart_scale == m_pViewPort.chart_scale && 
        vp.lat_max == m_pViewPort.lat_max && vp.lat_min == m_pViewPort.lat_min && vp.lon_max == m_pViewPort.lon_max && vp.lon_min == m_pViewPort.lon_min && vp.view_scale_ppm == m_pViewPort.view_scale_ppm)
    {
        return; //Prevents event storm killing the responsiveness. At least in course-up it looks needed.
    }
    m_pViewPort = vp;
//    if (!b_dbUsable || !m_bRenderOverlay)
//       return false;
*/

    m_pViewPort = *vp;
    double x1 = m_pViewPort.lon_min;
    double y1 = m_pViewPort.lat_min;
    double x2 = m_pViewPort.lon_max;
    double y2 = m_pViewPort.lat_max;

    std::vector<Node> nodes;
    m_pDatabase->SelectNodes(x1,y1,x2,y2,nodes);

    for(std::vector<Node>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        Node node = *it;
        AddNode(node, true);
    }

    return m_pFactory->RenderOverlay( dc, vp );
}

bool OsmOverlayUI::RenderGLOverlay( wxGLContext *pcontext, PlugIn_ViewPort *vp )
{
    m_pViewPort = *vp;
    return m_pFactory->RenderGLOverlay( pcontext, vp );
}

void OsmOverlayUI::AddGroup( wxString group_name, bool visible )
{
    if (! m_pFactory->AddGroup( group_name, visible )) {
        return;
    }
    m_pCheckListBox->Append( group_name );
    m_pCheckListBox->Check( m_pCheckListBox->GetCount()-1, visible );
}

void OsmOverlayUI::AddNode( Node node, bool visible )
{
    if (! m_pFactory->AddNode( node, visible )) {
        return;
    }
}

bool OsmOverlayUI::GetVisibility( int idx )
{
      return m_pFactory->GetVisibility( idx ); // m_pCheckListBox->IsChecked( idx );
}

void OsmOverlayUI::OnListItemSelected( wxCommandEvent& event )
{
      UpdateButtonsState();
}

void OsmOverlayUI::OnCheckToggle( wxCommandEvent& event )
{
      m_pFactory->SetVisibility( event.GetInt(), m_pCheckListBox->IsChecked( event.GetInt() ) );
}

void OsmOverlayUI::UpdateButtonsState()
{
//      m_pButtonDelete->Enable( m_pCheckListBox->GetSelection() != wxNOT_FOUND );
}

void OsmOverlayUI::OnDownload( wxCommandEvent &event )
{
    // start download
    double x1 = m_pViewPort.lon_min;
    double y1 = m_pViewPort.lat_min;
    double x2 = m_pViewPort.lon_max;
    double y2 = m_pViewPort.lat_max;
    
    bool success = m_pDownloader->Download(x1,y1,x2,y2);
    if (success)
    {
        wxLogMessage (_T("OSM_PI: We have a file to play with...."));
        m_pDatabase->ConsumeOsm(OsmDownloader::m_osm_path);
        RequestRefresh( GetOCPNCanvasWindow() );
    }
}

void OsmOverlayUI::OnStart( wxCommandEvent &event )
{
    wxLogMessage (_T("OSM_PI: OnStart"));
}



