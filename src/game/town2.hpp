#pragma once
#include "lifeext.hpp"
// Town 2 (town2.cpp): walkable Food Court, school (Hallway & Lockers, Classroom, Cafeteria, Gym), Daycare,
// Church, Community Center, Community Pool, Block Party street, College Dorm. Option ids 18000-18899.
std::vector<std::pair<std::string, int>> town2SpotOptions(int spot);
LxMenu town2Option(int id);
void town2DrawRoom(int room, float t, bool night);
bool town2Room(int room);
