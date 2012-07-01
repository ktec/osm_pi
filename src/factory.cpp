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
#include "db.h"
#include "icons.h"

//WX_DECLARE_STRING_HASH_MAP( wxString, SeamarkTypesHashMap );
//const SeamarkTypesHashMap* SeamarkTypes;

const wxColor OsmOverlayDefaultColor( 144, 144, 144 );

OsmOverlayFactory::Container::Container( wxString seamark_type, bool visible )
     : m_ready( false ), m_seamark_type( seamark_type ), m_visible( visible )

OsmOverlayFactory::OsmOverlayFactory( OsmDatabase *database )
    : m_pOsmDatabase(database)
{
}

OsmOverlayFactory::~OsmOverlayFactory()
{
    for ( size_t i = m_Objects.GetCount(); i > 0; i-- )
    {
        Container *cont = m_Objects.Item( i-1 );
        m_Objects.Remove( cont );
        delete cont;
    }
}

bool OsmOverlayFactory::RenderOverlay( wxDC &dc, PlugIn_ViewPort *vp )
{
    wxLogMessage (_T("OSM_PI: OsmOverlayFactory::RenderOverlay"));
    // hit the db and get the marks
    
    // m_seamark_type - GetMarkType()
    // std::vector<Poi> &features
    // int OsmDatabase::SelectNodes (double lat, double lon, double lat_max, double lon_max, std::vector<Poi> &features)

/*
    // TODO: Query local database for seamarks
    std::vector<Poi> seamarks;
    m_pOsmDatabase->SelectNodes(x1,y1,x2,y2,seamarks);

    for(std::vector<Poi>::iterator it = seamarks.begin(); it != seamarks.end(); ++it) {
        wxPoint pl;
        GetCanvasPixLL(vp, &pl, (*it).longitude, (*it).latitude);
        DoDrawBitmap( *_img_osm, pl.x, pl.y, false );
    }
*/

    for ( size_t i = 0; i < m_Objects.GetCount(); i++ )
    {
        m_Objects.Item( i )->Render( dc, vp );
    }
    return true;
}

bool OsmOverlayFactory::RenderGLOverlay( wxGLContext *pcontext, PlugIn_ViewPort *vp )
{
    wxLogMessage (_T("OSM_PI: OsmOverlayFactory::RenderGLOverlay"));
    for ( size_t i = 0; i < m_Objects.GetCount(); i++ )
    {
        m_Objects.Item( i )->RenderGL( pcontext, vp );
    }
    return true;
}

bool OsmOverlayFactory::Add( wxString seamark_type, bool visible )
{
    wxLogMessage (_T("OSM_PI: OsmOverlayFactory::Add %s"), seamark_type.c_str());
    Container *cont = new Container( seamark_type, visible );
    if ( cont->Setup() )
    {
        m_Objects.Add( cont );
        RequestRefresh( GetOCPNCanvasWindow() );
        return true;
    }
    else
    {
        delete cont;
        return false;
    }
}

void OsmOverlayFactory::SetVisibility( int idx, bool visible )
{
    m_Objects.Item( idx )->SetVisibility( visible );
    RequestRefresh( GetOCPNCanvasWindow() );
}

bool OsmOverlayFactory::GetVisibility( int idx )
{
      return m_Objects.Item( idx )->GetVisibility();
}

int OsmOverlayFactory::GetCount()
{
      return m_Objects.GetCount();
}




OsmOverlayFactory::Container::Container( wxString seamark_type, bool visible )
     : m_ready( false ), m_seamark_type( seamark_type ), m_visible( visible )
{
}

bool OsmOverlayFactory::Container::Setup()
{
      m_ready = true;
      return true;
}

void OsmOverlayFactory::Container::DoDrawBitmap( const wxBitmap &bitmap, wxCoord x, wxCoord y, bool usemask )
{
      if ( m_pdc ) {
            m_pdc->DrawBitmap( bitmap, x, y, usemask );
      } else {
            // GL doesn't draw anything if x<0 || y<0 so we must crop image first
            wxBitmap bmp;
            if ( x < 0 || y < 0 ) {
                  int dx = (x < 0 ? -x : 0);
                  int dy = (y < 0 ? -y : 0);
                  int w = bitmap.GetWidth()-dx;
                  int h = bitmap.GetHeight()-dy;
                  /* picture is out of viewport */
                  if ( w <= 0 || h <= 0 )
                        return;
                  wxBitmap newBitmap = bitmap.GetSubBitmap( wxRect( dx, dy, w, h ) );
                  x += dx;
                  y += dy;
                  bmp = newBitmap;
            } else {
                  bmp = bitmap;
            }
            wxImage image = bmp.ConvertToImage();
            int w = image.GetWidth(), h = image.GetHeight();

            if ( usemask ) {
                  unsigned char *d = image.GetData();
                  unsigned char *a = image.GetAlpha();

                  unsigned char mr, mg, mb;
                  if( !image.GetOrFindMaskColour( &mr, &mg, &mb ) && !a )
                        printf("trying to use mask to draw a bitmap without alpha or mask\n");

                  unsigned char *e = new unsigned char[4*w*h];
                  //               int w = image.GetWidth(), h = image.GetHeight();
                  int sb = w*h;
                  unsigned char r, g, b;
                  for ( int i=0 ; i<sb ; i++ ) {
                        r = d[i*3 + 0];
                        g = d[i*3 + 1];
                        b = d[i*3 + 2];

                        e[i*4 + 0] = r;
                        e[i*4 + 1] = g;
                        e[i*4 + 2] = b;

                        e[i*4 + 3] = a ? a[i] :
                        ((r==mr)&&(g==mg)&&(b==mb) ? 0 : 255);
                  }

                  glColor4f( 1, 1, 1, 1 );
                  glEnable( GL_BLEND );
                  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                  glRasterPos2i( x, y );
                  glPixelZoom( 1, -1 );
                  glDrawPixels( w, h, GL_RGBA, GL_UNSIGNED_BYTE, e );
                  glPixelZoom( 1, 1 );
                  glDisable( GL_BLEND );
                  free( e );
            } else {
                  glRasterPos2i( x, y );
                  glPixelZoom( 1, -1 ); /* draw data from top to bottom */
                  glDrawPixels( w, h, GL_RGB, GL_UNSIGNED_BYTE, image.GetData() );
                  glPixelZoom( 1, 1 );
            }
      }
}

bool OsmOverlayFactory::Container::DoRender()
{
    if ( !m_ready )
        return false;

    if ( !m_visible )
        return true;
        
    
        
    //GetCanvasPixLL( m_pvp,  &pt, lat, lon );
    //DoDrawBitmap( *_img_point, pt.x-16, pt.y-32, true );
    return true;
}

bool OsmOverlayFactory::Container::Render( wxDC &dc, PlugIn_ViewPort *vp )
{
      m_pdc = &dc;
      m_pcontext = NULL;
      m_pvp = vp;
      return DoRender();
}

bool OsmOverlayFactory::Container::RenderGL( wxGLContext *pcontext, PlugIn_ViewPort *vp )
{
      m_pdc = NULL;
      m_pcontext = pcontext;
      m_pvp = vp;
      return DoRender();
}

void OsmOverlayFactory::Container::SetVisibility( bool visible )
{
      m_visible = visible;
}

wxString OsmOverlayFactory::Container::GetMarkType()
{
      return m_seamark_type;
}

bool OsmOverlayFactory::Container::GetVisibility()
{
      return m_visible;
}


