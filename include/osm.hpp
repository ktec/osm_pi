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

enum SeamarkType
{
    ANCHORAGE, ANCHOR_BERTH, BERTH, BUILDING, BEACON_CARDINAL, BEACON_ISOLATED_DANGER, BEACON_LATERAL, BEACON_SAFE_WATER, BEACON_SPECIAL_PURPOSE, BUOY_CARDINAL, BUOY_INSTALLATION, BUOY_ISOLATED_DANGER, BUOY_LATERAL, BUOY_SAFE_WATER, BUOY_SPECIAL_PURPOSE, CABLE_AREA, CABLE_SUBMARINE, CAUSWAY, COASTGUARD_STATION, DAYMARK, FOG_SIGNAL, GATE, HARBOUR, LANDMARK, LIGHT, LIGHT_MAJOR, LIGHT_MINOR, LIGHT_FLOAT, LIGHT_VESSEL, LOCK_BASIN, MOORING, NAVIGATION_LINE, NOTICE, PILE, PILOT_BOARDING, PLATFORM, PRODUCTION_AREA, RADAR_REFLECTOR, RADAR_TRANSPONDER, RADAR_STATION, RADIO_STATION, RECOMMENDED_TRACK, RESCUE_STATION, RESTRICTED_AREA, SANDWAVES, SEABED_AREA, SEPARATION_BOUNDARY, SEPARATION_CROSSING, SEPARATION_LANE, SEPARATION_LINE, SEPARATION_ROUNDABOUT, SEPARATION_ZONE, SHORELINE_CONSTRUCTION, SIGNAL_STATION_TRAFFIC, SIGNAL_STATION_WARNING, SMALL_CRAFT_FACILITY, TOPMARK, WRECK
};

//WX_DECLARE_STRING_HASH_MAP( int, SeamarkTypeHashMap );

typedef std::map<const char*, int> SeamarkTypes;

const SeamarkTypes::value_type x[] = {
    std::make_pair("anchorage", ANCHORAGE),
    std::make_pair("anchor_berth", ANCHOR_BERTH),
    std::make_pair("berth", BERTH),
    std::make_pair("building", BUILDING),
    std::make_pair("beacon_cardinal", BEACON_CARDINAL),
    std::make_pair("beacon_isolated_danger", BEACON_ISOLATED_DANGER),
    std::make_pair("beacon_lateral", BEACON_LATERAL),
    std::make_pair("beacon_safe_water", BEACON_SAFE_WATER),
    std::make_pair("beacon_special_purpose", BEACON_SPECIAL_PURPOSE),
    std::make_pair("buoy_cardinal", BUOY_CARDINAL),
    std::make_pair("buoy_installation", BUOY_INSTALLATION),
    std::make_pair("buoy_isolated_danger", BUOY_ISOLATED_DANGER),
    std::make_pair("buoy_lateral", BUOY_LATERAL),
    std::make_pair("buoy_safe_water", BUOY_SAFE_WATER),
    std::make_pair("buoy_special_purpose", BUOY_SPECIAL_PURPOSE),
    std::make_pair("cable_area", CABLE_AREA),
    std::make_pair("cable_submarine", CABLE_SUBMARINE),
    std::make_pair("causway", CAUSWAY),
    std::make_pair("coastguard_station", COASTGUARD_STATION),
    std::make_pair("daymark", DAYMARK),
    std::make_pair("fog_signal", FOG_SIGNAL),
    std::make_pair("gate", GATE),
    std::make_pair("harbour", HARBOUR),
    std::make_pair("landmark", LANDMARK),
    std::make_pair("light", LIGHT),
    std::make_pair("light_major", LIGHT_MAJOR),
    std::make_pair("light_minor", LIGHT_MINOR),
    std::make_pair("light_float", LIGHT_FLOAT),
    std::make_pair("light_vessel", LIGHT_VESSEL),
    std::make_pair("lock_basin", LOCK_BASIN),
    std::make_pair("mooring", MOORING),
    std::make_pair("navigation_line", NAVIGATION_LINE),
    std::make_pair("notice", NOTICE),
    std::make_pair("pile", PILE),
    std::make_pair("pilot_boarding", PILOT_BOARDING),
    std::make_pair("platform", PLATFORM),
    std::make_pair("production_area", PRODUCTION_AREA),
    std::make_pair("radar_reflector", RADAR_REFLECTOR),
    std::make_pair("radar_transponder", RADAR_TRANSPONDER),
    std::make_pair("radar_station", RADAR_STATION),
    std::make_pair("radio_station", RADIO_STATION),
    std::make_pair("recommended_track", RECOMMENDED_TRACK),
    std::make_pair("rescue_station", RESCUE_STATION),
    std::make_pair("restricted_area", RESTRICTED_AREA),
    std::make_pair("sandwaves", SANDWAVES),
    std::make_pair("seabed_area", SEABED_AREA),
    std::make_pair("separation_boundary", SEPARATION_BOUNDARY),
    std::make_pair("separation_crossing", SEPARATION_CROSSING),
    std::make_pair("separation_lane", SEPARATION_LANE),
    std::make_pair("separation_line", SEPARATION_LINE),
    std::make_pair("separation_roundabout", SEPARATION_ROUNDABOUT),
    std::make_pair("separation_zone", SEPARATION_ZONE),
    std::make_pair("shoreline_construction", SHORELINE_CONSTRUCTION),
    std::make_pair("signal_station_traffic", SIGNAL_STATION_TRAFFIC),
    std::make_pair("signal_station_warning", SIGNAL_STATION_WARNING),
    std::make_pair("small_craft_facility", SMALL_CRAFT_FACILITY),
    std::make_pair("topmark", TOPMARK),
    std::make_pair("wreck", WRECK)
};


/*
    std::make_pair("anchorage", ANCHORAGE;
    std::make_pair("anchor_berth"]=ANCHOR_BERTH;
    std::make_pair("berth"]=BERTH;
    std::make_pair("building"]=BUILDING;
    std::make_pair("beacon_cardinal"]=BEACON_CARDINAL;
    std::make_pair("beacon_isolated_danger"]=BEACON_ISOLATED_DANGER;
    std::make_pair("beacon_lateral"]=BEACON_LATERAL;
    std::make_pair("beacon_safe_water"]=BEACON_SAFE_WATER;
    std::make_pair("beacon_special_purpose"]=BEACON_SPECIAL_PURPOSE;
    std::make_pair("buoy_cardinal"]=BUOY_CARDINAL;
    std::make_pair("buoy_installation"]=BUOY_INSTALLATION;
    std::make_pair("buoy_isolated_danger"]=BUOY_ISOLATED_DANGER;
    std::make_pair("buoy_lateral"]=BUOY_LATERAL;
    std::make_pair("buoy_safe_water"]=BUOY_SAFE_WATER;
    std::make_pair("buoy_special_purpose"]=BUOY_SPECIAL_PURPOSE;
    std::make_pair("cable_area"]=CABLE_AREA;
    std::make_pair("cable_submarine"]=CABLE_SUBMARINE;
    std::make_pair("causway"]=CAUSWAY;
    std::make_pair("coastguard_station"]=COASTGUARD_STATION;
    std::make_pair("daymark"]=DAYMARK;
    std::make_pair("fog_signal"]=FOG_SIGNAL;
    std::make_pair("gate"]=GATE;
    std::make_pair("harbour"]=HARBOUR;
    std::make_pair("landmark"]=LANDMARK;
    std::make_pair("light"]=LIGHT;
    std::make_pair("light_major"]=LIGHT_MAJOR;
    std::make_pair("light_minor"]=LIGHT_MINOR;
    std::make_pair("light_float"]=LIGHT_FLOAT;
    std::make_pair("light_vessel"]=LIGHT_VESSEL;
    std::make_pair("lock_basin"]=LOCK_BASIN;
    std::make_pair("mooring"]=MOORING;
    std::make_pair("navigation_line"]=NAVIGATION_LINE;
    std::make_pair("notice"]=NOTICE;
    std::make_pair("pile"]=PILE;
    std::make_pair("pilot_boarding"]=PILOT_BOARDING;
    std::make_pair("platform"]=PLATFORM;
    std::make_pair("production_area"]=PRODUCTION_AREA;
    std::make_pair("radar_reflector"]=RADAR_REFLECTOR;
    std::make_pair("radar_transponder"]=RADAR_TRANSPONDER;
    std::make_pair("radar_station"]=RADAR_STATION;
    std::make_pair("radio_station"]=RADIO_STATION;
    std::make_pair("recommended_track"]=RECOMMENDED_TRACK;
    std::make_pair("rescue_station"]=RESCUE_STATION;
    std::make_pair("restricted_area"]=RESTRICTED_AREA;
    std::make_pair("sandwaves"]=SANDWAVES;
    std::make_pair("seabed_area"]=SEABED_AREA;
    std::make_pair("separation_boundary"]=SEPARATION_BOUNDARY;
    std::make_pair("separation_crossing"]=SEPARATION_CROSSING;
    std::make_pair("separation_lane"]=SEPARATION_LANE;
    std::make_pair("separation_line"]=SEPARATION_LINE;
    std::make_pair("separation_roundabout"]=SEPARATION_ROUNDABOUT;
    std::make_pair("separation_zone"]=SEPARATION_ZONE;
    std::make_pair("shoreline_construction"]=SHORELINE_CONSTRUCTION;
    std::make_pair("signal_station_traffic"]=SIGNAL_STATION_TRAFFIC;
    std::make_pair("signal_station_warning"]=SIGNAL_STATION_WARNING;
    std::make_pair("small_craft_facility"]=SMALL_CRAFT_FACILITY;
    std::make_pair("topmark"]=TOPMARK;
    std::make_pair("wreck"]=WRECK;
*/

#endif
