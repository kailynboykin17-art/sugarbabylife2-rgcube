#pragma once
#include "common.hpp"
// Story mode: "From Intern to Icon" - start as the coffee intern for the icy creative
// director of the House of Vane and work your way up to designing your own collections.
// Visual-novel scenes, choices with consequences, minigames, a very persuasive credit
// card, and full runway shows you sit and watch.
void storyStart();
bool storyUpdate(const Input& in);   // false when the player leaves story mode
void storyDrawTop(float t);
void storyDrawBottom(float t);
const char* storyRankName();
