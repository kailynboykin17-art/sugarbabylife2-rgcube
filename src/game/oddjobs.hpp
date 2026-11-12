#pragma once
#include "common.hpp"
// Odd Jobs app (oddjobs.cpp): 31 playable shift minigames (7 hand-made + 24 on shared match/juggle/timing/quiz engines) - Nanny, Grocery Scanner, Barista (remember the
// order, then make it), Restaurant Server, Florist, Pet Groomer, Substitute Teacher. ~60s shifts, paid by score.
void oddJobsOpen();
bool oddJobsUpdate(const Input& in);   // false = back to play
void oddJobsDrawTop(float t);
void oddJobsDrawBottom(float t);
