#pragma once
#include "common.hpp"
#include <string>
#include <vector>
// Life Extras (lifeext.cpp): hosted parties & sleepovers, NPC invitations, reputation by circle,
// gossip that spreads, family traditions, active Ambitions careers, mid-life crises / teen rebellion /
// prom, menstrual cycles, roommate rent splitting, and the 52 extra Maple Valley residents.
// Menus use option ids 10000-10999; lxOption returns the next menu (empty = close).
typedef std::vector<std::pair<std::string, int>> LxMenu;
LxMenu lxApp(int which);   // 12 Teen Life, 13 Group; 0 Host, 1 Invites, 2 Rep, 3 Traditions, 4 Ambitions, 5 Body, 6 Bedrooms, 7 Skills+, 8 Live, 9 TikTak, 10 Contests, 11 News
LxMenu lxOption(int id);
bool lxWantsParty();       // main starts its party once a plan is confirmed
void lxAfterParty();       // rate the party once guests are in the room
void lxRepAdd(int circle, int d, const char* why);
int lxRep(int circle);
int lxSquashRumors();   // debunk false rumors; returns how many
void lxGossip(const std::string& text, bool truth, int circle, int sign);
enum { CI_FASHION, CI_ARTISTS, CI_JOCKS, CI_SCHOLARS, CI_PARTY, CI_FAMILY, CI_COUNT };
int lxRentShare(int price);
enum { XS_DANCE, XS_SING, XS_PAINT, XS_GAMING, XS_PHOTO, XS_HOME, XS_WRITE, XS_GARDEN, XS_COMEDY, XS_MIXOLOGY, XS_KNIT, XS_MAKEUP, XS_COUNT };
void lxGroupFrame();
int lxTattoos(int sim);         // tattoo bitmask for the doll painter
bool lxBirthControl(int sim);   // family planning: true = on birth control
void lxBirth(int mom, int baby); // birth story (hospital / home / water birth)
LxMenu lxWardrobe();   // outfit categories app (ids 15000+)
// Seasons calendar (replaces the phone Calendar app): 7/14/28-day grid, deterministic forecast,
// holidays with traditions, daily schedule, custom holidays, social events and planned getaways.
int lxForecast(int day);           // WX_* for any day (the real weather uses the same forecast)
bool lxCalUpdate(const Input& in); // returns true to open the planning menu (phone jump 29)
void lxCalDrawTop(float t);
void lxCalDrawBottom(float t);   // group members follow you around
void lxSkill(int k, int xp);          // extra skills (player character)
void lxClean(int amt);                // chores clean the house
void lxDrawPests(float t);            // roaches / rats overlay at home when it's filthy   // what the player's household owes after roommates chip in
int lxTrend(); const char* lxTrendName();
bool lxTrendy(int outfit);
void lxNews(const std::string& headline);
bool lxBlocks(int room, std::string* why);   // venue closed for renovation
void lxDrawWork(float t);                     // Ambitions shift scene on the top screen
void lxHourly(int hour);
void lxDaily();
void lxSaveTo(const std::string& path);
void lxLoadFrom(const std::string& path);
