/***************************************************************************
 * $Id: factory.cpp, v0.1 2012-02-08 SethDart Exp $
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
#include <wx/mstream.h>

#ifndef  WX_PRECOMP
  #include <wx/wx.h>
#endif //precompiled headers

#include "factory.h"
#include "icons.h"

const wxColor OsmOverlayDefaultColor( 144, 144, 144 );

OsmOverlayFactory::OsmOverlayFactory()
{
}

OsmOverlayFactory::~OsmOverlayFactory()
{
}

bool OsmOverlayFactory::RenderOverlay( wxDC &dc, PlugIn_ViewPort *vp )
{
    wxLogMessage (_T("OSM_PI: OsmOverlayFactory::RenderOverlay"));
    return true;
}

bool OsmOverlayFactory::RenderGLOverlay( wxGLContext *pcontext, PlugIn_ViewPort *vp )
{
    wxLogMessage (_T("OSM_PI: OsmOverlayFactory::RenderGLOverlay"));
    return true;
}

