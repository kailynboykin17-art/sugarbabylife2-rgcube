#pragma once
#include "common.hpp"
// Painted backgrounds (bgs.cpp): 400x240 PNGs replace the drawn backdrops of town/vacation places.
// Files: <LifeSim or romfs>/Backgrounds/<Room_Name>.png (room name with spaces/symbols as _),
// extra scenes for Movie Maker in Backgrounds/Extra/. Maple Valley only (Sugar Bunny Life 2 keeps its look).
void bgScan();
bool bgDrawRoom(int room, bool night);   // false = no painted background, draw the usual one
int bgExtraCount();
const char* bgExtraName(int k);
void bgDrawExtra(int k);
extern bool g_spotMarkers;               // painted background: show small markers instead of drawn props
