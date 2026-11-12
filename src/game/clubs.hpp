#pragma once
#include "common.hpp"
#include <string>
// Clubs (clubs.cpp): make your own club - name, icon, members, requirements, encouraged / discouraged
// activities, a group chat, scheduled gatherings with a dress code, leader / co-leader / member / rival
// ranks, club points & perks, and town clubs the NPCs run on their own (with drama).
// Daily Goals: three small goals every day with cash + club-point rewards.
void clubsOpen(int tab);   // 0 my clubs, 1 group chat, 2 daily goals
bool clubsUpdate(const Input& in);   // false = close
void clubsDrawTop(float t);
void clubsDrawBottom(float t);
void clubsHourly(int hour);
void clubsDaily();
void clubsNotify(int ev, int arg);   // goalNotify hook: travel, social, cook, etc.
void clubsSaveTo(const std::string& path);
void clubsLoadFrom(const std::string& path);
