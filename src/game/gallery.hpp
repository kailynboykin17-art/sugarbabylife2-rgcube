#pragma once
#include "common.hpp"
// Home Gallery (gallery.cpp): an OFFLINE house catalog. Browse, filter, favorite, preview a floor plan,
// then buy or rent and move the household in (furnished or bringing your own furniture).
// Everything is built into the game - no internet, accounts or downloads.
void galleryOpen();
bool galleryUpdate(const Input& in);   // false = back to the menu
void galleryDrawTop(float t);
void galleryDrawBottom(float t);
void galleryDaily();                   // weekly rent
const char* galleryHomeName();
int galleryHouseCount();
void galleryMoveIn(int i);
