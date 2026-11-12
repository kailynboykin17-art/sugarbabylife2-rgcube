#pragma once
#include "common.hpp"
// Dinner Rush: the waitressing minigame (Sunny Side Diner)
void serveStart();
bool serveUpdate(const Input& in);   // false when the shift is over (pay already given)
void serveDrawTop(float t);
void serveDrawBottom(float t);
