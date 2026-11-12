#pragma once
#include "lifeext.hpp"
// Game Mode (levels.cpp): Ultimate Sandbox (everything open, the default) or Levels - earn Prestige from
// 3 daily tasks, odd jobs, farm errands, fair ribbons and family outings across 50 levels: 19 levels unlock
// part of the world (pets, odd jobs, pots, Metro, rink, salon, nightlife, modeling, Downtown, Sparkle Land,
// the arts, community, Club Glitz, 18+ gigs, vacations, Apple Valley, farm buildings, the farmhouse);
// the rest pay cash, titles (every 10) and daily-income perks. Menu ids 28000-28999, sidecar ".lvl".
// value = the level that unlocks it (50 levels; levels in between pay cash, titles & perks)
enum LvFeature { LF_CREDIT = 2, LF_PETS = 4, LF_ODDJOBS = 5, LF_POTS = 7, LF_METRO = 9, LF_RINK = 11, LF_SALON = 13, LF_NIGHTLIFE = 15, LF_MODELING = 17,
                 LF_DOWNTOWN = 19, LF_FUNPARK = 21, LF_ARTS = 23, LF_COMMUNITY = 25, LF_CLUB = 27, LF_ADULTGIGS = 29, LF_TRAVEL = 31, LF_FARM = 33,
                 LF_FARMBUILD = 35, LF_FARMHOUSE = 37, LF_MAX = 50 };
bool lvlLevels();                       // true = Levels mode
bool lvlAllows(int feature);            // always true in Ultimate Sandbox
bool lvlRoomOk(int room, std::string* why);
void lvlPrestige(int pts, const char* why);
LxMenu lvlMenu();
LxMenu lvlOption(int id);
void lvlDaily();
void lvlHourly(int hour);
enum { LEV_ODDJOB = 1, LEV_OUTING = 2, LEV_FARM = 4 };
void lvlEvent(int ev);   // marks a daily-task event (LEV_*)
void lvlSaveTo(const std::string& path);
void lvlLoadFrom(const std::string& path);
