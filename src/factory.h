/***************************************************************************
 * $Id: factory.h, v0.1 2012-02-08 ktec Exp $
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

#ifndef _OSMOVERLAYFACTORY_H_
#define _OSMOVERLAYFACTORY_H_

#include <wx/wxprec.h>

#ifndef  WX_PRECOMP
  #include <wx/wx.h>
#endif //precompiled headers

#include "../../../include/ocpn_plugin.h"

class OsmOverlayFactory
{
public:
      OsmOverlayFactory();
      ~OsmOverlayFactory();

      bool RenderOverlay( wxDC &dc, PlugIn_ViewPort *vp );
      bool RenderGLOverlay( wxGLContext *pcontext, PlugIn_ViewPort *vp );

private:

};

#endif
