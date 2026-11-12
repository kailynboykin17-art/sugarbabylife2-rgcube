#pragma once
#include "common.hpp"
// Photo filters (filters.cpp): lightweight full-screen color grades drawn over the top screen, in the spirit
// of the Moonivy's Planet look (soft warm cream wash, peachy highlights, muted colors, dreamy sparkle).
// Pure 2D overlays - no shaders, nothing loaded from the PC .ini file.
constexpr int NUM_FILTERS = 8;
const char* filterName(int f);
void filterDraw(float t);   // call after the world is drawn, before the HUD
