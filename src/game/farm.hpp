#pragma once
#include "lifeext.hpp"
// Apple Valley (farm.cpp): a farm-country town on the Metro (train ticket the first time, the Mayor
// welcomes you and hands you Sunny Acres). Village square (General Store, Errand Board, Saturday Fair,
// Cider Pub), Sunny Acres Farm (8 garden plots with giant crops, chicken coop, nectar maker & rack,
// canning stove, wild rabbits / birds / foxes) and the Big Red Barn (cow, llama, mini goats, mini sheep,
// pigs, ducks, cross-stitch). The farm kitchen has the nectar maker & canning stove. Home garden pots work anywhere. Menu ids 27000-27999, sidecar ".farm".
std::vector<std::pair<std::string, int>> farmSpotOptions(int spotKind);
LxMenu farmOption(int id);
LxMenu farmHomePots();               // Daily Life > Home Garden Pots
void farmArrive(int room);           // first visit: train ticket + Mayor's welcome
void farmDaily();
void farmDrawRoom(int room, float t, bool night);
bool farmRoom(int room);
void farmOnMainMove(int from, int to, bool furnished);   // Realty move: the farmhouse keeps the other home slot
LxMenu farmHomeRows();               // Daily Life rows: go to the farmhouse / back home
void farmSaveTo(const std::string& path);
void farmLoadFrom(const std::string& path);
