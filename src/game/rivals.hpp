#pragma once
#include "common.hpp"
// Sugar Rivals (rivals.cpp): an original 3v3 hero-shooter arena (Marvel Rivals-style gameplay, all-original heroes).
// Run around the top screen, auto-aim at the nearest enemy, use 2 abilities + an ultimate. First team to 12 KOs wins.
void rivalsOpen();
bool rivalsUpdate(const Input& in);   // false = back to the game
void rivalsDrawTop(float t);
void rivalsDrawBottom(float t);
