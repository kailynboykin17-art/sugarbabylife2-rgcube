#pragma once
#include "common.hpp"
#include <string>
// World settings, extra story progression, rival romances (worldsim.cpp) and the hotel business (hotel in the same file).
float worldSpeedMul();               // game-time multiplier from World Settings
bool simGone(int s);                 // moved out of town
int lifespanDays();                  // elder days before passing on (by the lifespan setting)
void worldSimDaily();
void worldOpen();
bool worldUpdate(const Input& in);   // false = back to the menu
void worldDrawTop(float t);
void worldDrawBottom(float t);
// hotel (phone app "Hotel")
bool hotelStaying();
void hotelDaily();
void hotelAppOpen();
void hotelAppUpdate(const Input& in);
void hotelAppDrawTop(float t);
void hotelAppDrawBottom(float t);
