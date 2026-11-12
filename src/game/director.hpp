#pragma once
#include "common.hpp"
// Starlight Studios: be a movie director. Pick a script (2000s/2010s/2020s teen movies & made-for-TV
// thrillers), cast stars whose genre fits each role, style them on theme, pick the location, make the
// big script choices, snap the poster, and get reviewed by the critics (up to 3 stars).
void directorOpen();
bool directorUpdate(const Input& in);   // false = back to the menu
void directorDrawTop(float t);
void directorDrawBottom(float t);
