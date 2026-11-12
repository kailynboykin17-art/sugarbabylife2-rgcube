#pragma once
#include "common.hpp"
#include <string>
#include <vector>
// Hot Date-style Downtown: taxi in, street carts, a boutique with display racks + dressing booth + register
// (cart must be rung up before leaving), and a sit-down bistro (host seats you, waiter takes a cheap /
// average / expensive order, chef cooks, you eat, then the check must be paid before leaving).
std::vector<std::pair<std::string, int>> hotdateSpotOptions(int spotKind);   // ids are offsets from DA_HD0
void hotdateAction(int a);
bool hotdateBlocks(int room, std::string* why);
bool hotdateTaxi(int target);   // charges the fare when riding downtown; false = can't afford
void hotdateHourly();
void hotdateDrawRoom(int room, float t, bool night);
void hotdateSaveTo(const std::string& path);
void hotdateLoadFrom(const std::string& path);
