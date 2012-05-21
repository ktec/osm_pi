/******************************************************************************
 * $Id: osmgui_impl.h,v 1.0 2011/02/26 01:54:37 ktec Exp $
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

#ifndef _OSMGUI_IMPL_H_
#define _OSMGUI_IMPL_H_

#include "osmgui.h"
#include "osm_pi.h"

#include <wx/filedlg.h>

class osm_pi;

class OsmCfgDlg : public OsmCfgDlgDef
{
public:
      OsmCfgDlg( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("OSM preferences"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 496,587 ), long style = wxDEFAULT_DIALOG_STYLE );
};

class OsmDlg : public OsmDlgDef
{
public:
	OsmDlg( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("OSM"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 700,550 ), long style = wxDEFAULT_DIALOG_STYLE );
	void OnOsmProperties( wxCommandEvent& event );
	void OnOsmCancelClick( wxCommandEvent& event );
	void OnOsmOkClick( wxCommandEvent& event );
	osm_pi *plugin;
};

#endif
