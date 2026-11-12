#pragma once
#include "common.hpp"
#include <string>
// The life engine (life.cpp): every townie has a hobby, a life goal, a favourite place,
// a daily routine and memories, and the town keeps living hour by hour without you -
// friendships, feuds, couples, breakups, engagements, weddings, babies, goals reached,
// new neighbours, people going away, daily town events and family storylines at home.
enum LifeMem : uint8_t { LM_KIND, LM_HURT, LM_GIFT, LM_DATE, LM_FIGHT, LM_FRIEND, LM_ROMANCE, LM_BREAKUP, LM_HELPED, LM_GOAL, LM_BABY, LM_MOVEDIN, LM_ENGAGED };
const char* lifeHobbyName(int sim);
const char* lifeGoalName(int sim);
const char* lifeFavPlaceName(int sim);
int lifeWhere(int sim, int hour);           // room they'd be in at that hour (-1 home, -2 away)
void lifeRemember(int sim, int kind, int other);
void lifeBoostGoal(int sim, int amount);
std::string lifeLine(int sim);              // something they'd say out loud right now (memories, goals, hobbies), "" if nothing
int lifePickTownie(int room);               // someone whose routine has them here now (-1 if nobody)
void lifeHourly(int hour);
void lifeDaily();
void lifeOnTravel(int room);
std::string lifeTodayHeadline();            // today's town event, for the HUD / phone
bool lifeAway(int sim);                     // out of town for a few days
int lifeGoalProgress(int sim);              // 0..100 toward their current dream
std::string lifeStatusLine(int sim);        // "At the Cafe (barista)", "At home", "Out of town"
int lifeConvo(int a, int b, std::string out[4]);   // a real conversation between two townies (0 = none)
