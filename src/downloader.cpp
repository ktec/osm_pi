/******************************************************************************
 * $Id: osmdownloader.cpp,v 1.0 2011/02/26 01:54:37 ktec Exp $
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

#include "../include/downloader.h"

const char *OsmDownloader::m_osm_path = "/tmp/features.osm";
const char *OsmDownloader::m_api_url = "http://open.mapquestapi.com/xapi/api/0.6/";
//const char *OsmDownloader::m_api_url = "http://www.overpass-api.de/api/xapi?";
    //url = _("http://www.overpass-api.de/api/xapi?*[bbox=[%f,%f,%f,%f][seamark:type=*]]");
//const char *OsmDownloader::m_api_url = "http://overpass.osm.rambler.ru/cgi/xapi?";
    //url = _("http://overpass.osm.rambler.ru/cgi/xapi?*[bbox=[%f,%f,%f,%f][seamark:type=*]]");


OsmDownloader::OsmDownloader()
{
    // constructor
}

OsmDownloader::~OsmDownloader()
{
    // destructor
}

size_t OsmDownloader::write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}

bool OsmDownloader::Download(double x1, double y1, double x2, double y2)
{
    wxString url = GetApiUrl(x1,y1,x2,y2);
    wxLogMessage (_T("OSM_PI: Downloading %s"),url.c_str());

    CURL *curl;
    FILE *fp;
    CURLcode res;
    
    curl = curl_easy_init();
    if (curl) {
        fp = fopen(m_osm_path,"wb");
        curl_easy_setopt(curl, CURLOPT_URL, url.mb_str().data() );
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        fclose(fp);
    }

		//wxString m_errorString = wxT(curl_easy_strerror(res));
		//wxMessageBox(m_errorString);
		//wxLogMessage (_T("OSM_PI: Dang!! Couldnt download url because %s : [%s]"), m_errorString.c_str(), url.c_str());
    
	return (res == CURLE_OK);
}

wxString OsmDownloader::GetApiUrl(double x1, double y1, double x2, double y2)
{
    return wxString::Format(_("%s*[seamark:type=*][bbox=%f,%f,%f,%f]"), wxString::FromUTF8(m_api_url).c_str(), (float)x1, (float)y1, (float)x2, (float)y2);
}

