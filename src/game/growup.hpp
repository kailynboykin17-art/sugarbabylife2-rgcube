#pragma once
#include "lifeext.hpp"
// Growing Up (growup.cpp): allowance & chores chart, report cards & parent-teacher conferences, kid birthday
// parties, playdates & sleepovers, grandparents, family road trip ("are we there yet?"), imaginary friends,
// driver's test + first car, teen first jobs & savings, prom-posals & homecoming court, sneaking out (and
// nosy neighbors), clubs, talent show & school play, kid-safe bullies, graduations, kid pets, back-to-school
// shopping, the Mall hangout, autonomy settings, photo album, and a life checklist. Menu ids 17000-17999.
LxMenu guApp();
LxMenu guOption(int id);
float guAutonomy();   // 0 / 0.5 / 1 / 1.6 multiplier for NPC chatter & autonomous drama
void guHourly(int hour);
void guDaily();
void guPhoto(const std::string& caption);
int guPhotoCount();
void guOnDeath(int sim);
int guWillHeir();   // heir named in the player will, or -1
bool guTipSeen(int app); void guTipMark(int app);
void guSaveTo(const std::string& path);
void guLoadFrom(const std::string& path);
