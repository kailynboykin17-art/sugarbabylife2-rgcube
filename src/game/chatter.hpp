#pragma once
#include <string>
// Town dialogue: ambient one-liners, townie conversations, social line variety
std::string ambientLine(int sim);
void chatterTick(float dt);                 // runs little two-person conversations in the current room
const char* varyLine(const char* line, int sim);
