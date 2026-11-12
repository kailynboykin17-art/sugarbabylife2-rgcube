#pragma once
#include "common.hpp"
// Work shifts you actually play: each career has its own mini-game.
// The result (0-5 stars) feeds job performance, promotions and tips.
bool shiftHasGame(int career);
void shiftStart(int career);
bool shiftUpdate(const Input& in);   // false = shift over (pay already handed out)
void shiftDrawTop(float t);
void shiftDrawBottom(float t);
extern float g_shiftHours;           // in-game hours the shift took (main fast-forwards the clock)
