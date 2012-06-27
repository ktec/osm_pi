/******************************************************************************
 * $Id: OSMDOWNLOADER.h,v 1.0 2011/02/26 01:54:37 ktec Exp $
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

#ifndef _OSMDOWNLOADER_H_
#define _OSMDOWNLOADER_H_

#include <cstddef>
#define CURL_STATICLIB
#include <curl/curl.h>
#include <curl/easy.h>

class OsmDownloader
{
    public:
        OsmDownloader();
        ~OsmDownloader();

        bool Download(double x1, double y1, double x2, double y2);

    private:
        static const char *m_osm_path;
        static const char *m_api_url;
        static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);
};

#endif
