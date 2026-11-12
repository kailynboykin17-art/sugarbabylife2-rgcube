#pragma once
#include "common.hpp"
// Midnight Heist robber minigame
void heistStart();
bool heistUpdate(const Input& in);   // false when finished (result already applied)
void heistDrawTop(float t);
void heistDrawBottom(float t);
