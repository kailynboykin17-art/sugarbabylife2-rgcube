#pragma once
#include "lifeext.hpp"
// Life+ (lifeplus.cpp): tap-timing minigames (cooking, salon, runway, shopping spree, dance battle), life aspirations,
// bucket list, weekly challenges, school cliques, a nemesis, secrets, start/kill gossip, reputation gates (bouncers/VIP),
// getting caught cheating, money trouble (repo, eviction, pawn shop), seasonal town events, pet personalities, vacations.
// Option ids 24000-24999.
LxMenu lpApp();
LxMenu lpOption(int id);
void lpDaily();
void lpHourly(int hour);
bool lpMiniActive();
bool lpMiniUpdate(const Input& in);   // false = finished -> back to play
void lpMiniDrawTop(float t);
void lpMiniDrawBottom(float t);
bool lpBlocks(int room, std::string* why);   // bouncer: bad party rep keeps you out of clubs
void lpCount(int what);                       // other systems report achievements (LP_* below)
enum { LP_PARTY, LP_DATE, LP_WORKOUT, LP_COOK, LP_SHOP, LP_SOCIAL, LP_COUNT };
void lpSaveTo(const std::string& path);
void lpLoadFrom(const std::string& path);
