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
#include "icons.h"
#include "ui.h"

/*    KMLOverlay user interface implementation
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

      Fit();
      // GetSize() doesn't seems to count aui borders so we add it now.
      wxSize sz = GetSize(); sz.IncBy(10,24);
      SetMinSize(sz);

      UpdateButtonsState();
      m_pFactory = new OsmOverlayFactory();
      m_pDatabase = new OsmDatabase();
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
      return m_pFactory->RenderOverlay( dc, vp );
}

bool OsmOverlayUI::RenderGLOverlay( wxGLContext *pcontext, PlugIn_ViewPort *vp )
{
      return m_pFactory->RenderGLOverlay( pcontext, vp );
}

void OsmOverlayUI::SetCurrentViewPort( PlugIn_ViewPort &vp )
{
    if (vp.clat == m_pViewPort.clat && vp.clon == m_pViewPort.clon 
        && vp.pix_height == m_pViewPort.pix_height && vp.pix_width == m_pViewPort.pix_width 
        && vp.rotation == m_pViewPort.rotation && vp.chart_scale == m_pViewPort.chart_scale 
        && vp.lat_max == m_pViewPort.lat_max && vp.lat_min == m_pViewPort.lat_min 
        && vp.lon_max == m_pViewPort.lon_max && vp.lon_min == m_pViewPort.lon_min 
        && vp.view_scale_ppm == m_pViewPort.view_scale_ppm)
    {
        //Prevents event storm killing the responsiveness. At least in course-up it looks needed.
        return;
    }
    m_pViewPort = vp;
    m_pFactory->SetCurrentViewPort( vp );
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
    //m_pDatabase
    //m_pDownloader
}

