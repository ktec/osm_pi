/******************************************************************************
 * $Id: osmgui_impl.cpp,v 1.0 2011/02/26 01:54:37 ktec Exp $
 *
 * Project:  OpenCPN
 * Purpose:  OSM Plugin
 * Author:   Keith Salisbury
 *
 ***************************************************************************
 *   Copyright (C) 2012 by Keith Salisbury   *
 *   $EMAIL$   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.             *
 ***************************************************************************
 */

#include "osmgui_impl.h"

OsmCfgDlg::OsmCfgDlg( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : OsmCfgDlgDef( parent, id, title, pos, size, style )
{
}

OsmDlg::OsmDlg( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : OsmDlgDef( parent, id, title, pos, size, style )
{
}

void OsmDlg::OnOsmProperties( wxCommandEvent& event )
{
      wxMessageBox(_("Sorry, this function is not yet implemented..."));
      event.Skip(); 
}

void OsmDlg::OnOsmCancelClick( wxCommandEvent& event ) { event.Skip(); }

void OsmDlg::OnOsmOkClick( wxCommandEvent& event ) { event.Skip(); }