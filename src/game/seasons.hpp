#pragma once
#include "common.hpp"
#include <string>
// Seasons & festivals (seasons.cpp). Every season has its own look outdoors (blossom petals,
// summer shimmer, falling leaves, snow) and festivals with things to do: Sweetheart Day,
// Blossom Festival, Summer Splash, the Summer Street Fair, the Harvest Festival, Spooky Night,
// Winterfest and the New Year's Eve countdown. Townies show up for them (life.cpp), and when
// you sleep at night you skip to a fresh morning with a "Good Morning!" recap.
int festToday();                         // -1 none
int festOn(int day);
const char* festName(int f);
int festRoom(int f);
int festStart(int f);
int festEnd(int f);
bool festOpenNow();                      // today's festival is running right now
int festActs(std::string out[3], int ids[3]);   // festival activities here & now (not done yet)
void festDo(int i);
void seasonsHourly(int hour);
void seasonsDaily();
void seasonsDrawOverlay(float t);        // seasonal outdoors + festival decorations (top screen)
void seasonsDrawSleep(float t);          // the night fast-forward fade
bool seasonsNightSleep();                // you're asleep at night: skip to morning
void seasonsGoodMorning();               // the new-day recap card
