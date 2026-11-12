#pragma once
#include "common.hpp"
#include <string>
#include <vector>
// Sparkle Land amusement park (2 screens) + player Chores app. Option ids 9800-9999.
std::vector<std::pair<std::string, int>> funparkSpotOptions(int spot);
std::vector<std::pair<std::string, int>> choresOptions();
void funparkOption(int id);
void funparkDrawRoom(int room, float t, bool night);
