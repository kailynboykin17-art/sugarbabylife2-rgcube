#pragma once
#include "common.hpp"
// Legacy Challenges: story cards with an aspiration, traits, a career and a goal checklist.
// Pick a card, play it out in town, tick off the goals, then pass the torch to the next generation.
void legacyOpen();
bool legacyUpdate(const Input& in);   // false = back to town
void legacyDrawTop(float t);
void legacyDrawBottom(float t);
void legacyHourly();                  // auto-checks goals
void legacyNotify(int ev, int arg);   // goalNotify hook (trips to the spa, the club...)
const char* legacyMenuLabel();
