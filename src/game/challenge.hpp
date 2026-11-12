#pragma once
#include "common.hpp"
#include <string>
// Challenge Modes (challenge.cpp): four story-challenge runs from the Ultimate Game Modes pack -
// Doll Wars: Last Look, After Dark: The Lockdown (adults only), The Whisper Network and Bellweather Academy.
// Each run is self-contained (your town save is never erased). Danger is controlled by three separate
// options: Tragedy Frequency, Visual Violence (Clean / Mild / Stylized pixel blood) and Permanent Death.
void challengeOpen();
bool challengeUpdate(const Input& in);   // false = back to the menu
void challengeDrawTop(float t);
void challengeDrawBottom(float t);
int challengeDanger(int what);   // 0 tragedy 0-3, 1 violence 0-2, 2 permadeath 0/1, 3 reduced flash
void challengeSaveTo(const std::string& path);
void challengeLoadFrom(const std::string& path);
