/******************************************************************************
 * $Id: osm.h,v 1.0 2011/02/26 01:54:37 ktec Exp $
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

#ifndef _OSM_HPP_
#define _OSM_HPP_

#include <wx/wxprec.h>
#include <wx/hashmap.h>

#ifndef  WX_PRECOMP
  #include <wx/wx.h>
#endif //precompiled headers

#include <map>




struct Node
{
    long long id;
    int seamark_type;
	double latitude;
	double longitude;
};

/*
enum SeamarkType
{
    ANCHORAGE, ANCHOR_BERTH, BERTH, BUILDING, BEACON_CARDINAL, BEACON_ISOLATED_DANGER, BEACON_LATERAL, BEACON_SAFE_WATER, BEACON_SPECIAL_PURPOSE, BUOY_CARDINAL, BUOY_INSTALLATION, BUOY_ISOLATED_DANGER, BUOY_LATERAL, BUOY_SAFE_WATER, BUOY_SPECIAL_PURPOSE, CABLE_AREA, CABLE_SUBMARINE, CAUSWAY, COASTGUARD_STATION, DAYMARK, FOG_SIGNAL, GATE, HARBOUR, LANDMARK, LIGHT, LIGHT_MAJOR, LIGHT_MINOR, LIGHT_FLOAT, LIGHT_VESSEL, LOCK_BASIN, MOORING, NAVIGATION_LINE, NOTICE, PILE, PILOT_BOARDING, PLATFORM, PRODUCTION_AREA, RADAR_REFLECTOR, RADAR_TRANSPONDER, RADAR_STATION, RADIO_STATION, RECOMMENDED_TRACK, RESCUE_STATION, RESTRICTED_AREA, SANDWAVES, SEABED_AREA, SEPARATION_BOUNDARY, SEPARATION_CROSSING, SEPARATION_LANE, SEPARATION_LINE, SEPARATION_ROUNDABOUT, SEPARATION_ZONE, SHORELINE_CONSTRUCTION, SIGNAL_STATION_TRAFFIC, SIGNAL_STATION_WARNING, SMALL_CRAFT_FACILITY, TOPMARK, WRECK
};
*/

WX_DECLARE_STRING_HASH_MAP( wxString, SeamarkTypeHashMap );
SeamarkTypeHashMap SeamarkTypes;

SeamarkTypes["anchorage"]="hello world";

/*
SeamarkTypes[_T("anchorage")]=ANCHORAGE;
SeamarkTypes["anchor_berth"]=ANCHOR_BERTH;
SeamarkTypes["berth"]=BERTH;
SeamarkTypes["building"]=BUILDING;
SeamarkTypes["beacon_cardinal"]=BEACON_CARDINAL;
SeamarkTypes["beacon_isolated_danger"]=BEACON_ISOLATED_DANGER;
SeamarkTypes["beacon_lateral"]=BEACON_LATERAL;
SeamarkTypes["beacon_safe_water"]=BEACON_SAFE_WATER;
SeamarkTypes["beacon_special_purpose"]=BEACON_SPECIAL_PURPOSE;
SeamarkTypes["buoy_cardinal"]=BUOY_CARDINAL;
SeamarkTypes["buoy_installation"]=BUOY_INSTALLATION;
SeamarkTypes["buoy_isolated_danger"]=BUOY_ISOLATED_DANGER;
SeamarkTypes["buoy_lateral"]=BUOY_LATERAL;
SeamarkTypes["buoy_safe_water"]=BUOY_SAFE_WATER;
SeamarkTypes["buoy_special_purpose"]=BUOY_SPECIAL_PURPOSE;
SeamarkTypes["cable_area"]=CABLE_AREA;
SeamarkTypes["cable_submarine"]=CABLE_SUBMARINE;
SeamarkTypes["causway"]=CAUSWAY;
SeamarkTypes["coastguard_station"]=COASTGUARD_STATION;
SeamarkTypes["daymark"]=DAYMARK;
SeamarkTypes["fog_signal"]=FOG_SIGNAL;
SeamarkTypes["gate"]=GATE;
SeamarkTypes["harbour"]=HARBOUR;
SeamarkTypes["landmark"]=LANDMARK;
SeamarkTypes["light"]=LIGHT;
SeamarkTypes["light_major"]=LIGHT_MAJOR;
SeamarkTypes["light_minor"]=LIGHT_MINOR;
SeamarkTypes["light_float"]=LIGHT_FLOAT;
SeamarkTypes["light_vessel"]=LIGHT_VESSEL;
SeamarkTypes["lock_basin"]=LOCK_BASIN;
SeamarkTypes["mooring"]=MOORING;
SeamarkTypes["navigation_line"]=NAVIGATION_LINE;
SeamarkTypes["notice"]=NOTICE;
SeamarkTypes["pile"]=PILE;
SeamarkTypes["pilot_boarding"]=PILOT_BOARDING;
SeamarkTypes["platform"]=PLATFORM;
SeamarkTypes["production_area"]=PRODUCTION_AREA;
SeamarkTypes["radar_reflector"]=RADAR_REFLECTOR;
SeamarkTypes["radar_transponder"]=RADAR_TRANSPONDER;
SeamarkTypes["radar_station"]=RADAR_STATION;
SeamarkTypes["radio_station"]=RADIO_STATION;
SeamarkTypes["recommended_track"]=RECOMMENDED_TRACK;
SeamarkTypes["rescue_station"]=RESCUE_STATION;
SeamarkTypes["restricted_area"]=RESTRICTED_AREA;
SeamarkTypes["sandwaves"]=SANDWAVES;
SeamarkTypes["seabed_area"]=SEABED_AREA;
SeamarkTypes["separation_boundary"]=SEPARATION_BOUNDARY;
SeamarkTypes["separation_crossing"]=SEPARATION_CROSSING;
SeamarkTypes["separation_lane"]=SEPARATION_LANE;
SeamarkTypes["separation_line"]=SEPARATION_LINE;
SeamarkTypes["separation_roundabout"]=SEPARATION_ROUNDABOUT;
SeamarkTypes["separation_zone"]=SEPARATION_ZONE;
SeamarkTypes["shoreline_construction"]=SHORELINE_CONSTRUCTION;
SeamarkTypes["signal_station_traffic"]=SIGNAL_STATION_TRAFFIC;
SeamarkTypes["signal_station_warning"]=SIGNAL_STATION_WARNING;
SeamarkTypes["small_craft_facility"]=SMALL_CRAFT_FACILITY;
SeamarkTypes["topmark"]=TOPMARK;
SeamarkTypes["wreck"]=WRECK;
*/

#endif
