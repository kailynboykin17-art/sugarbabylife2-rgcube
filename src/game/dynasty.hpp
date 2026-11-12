#pragma once
#include "lifeext.hpp"
// Dynasty (dynasty.cpp): 10-generation legacy scoring (8 categories, pass the torch), random life events
// (lottery, house fire, a relative moves in, inheritance, car trouble...), and rotational play that switches
// households for you every N days. Menu ids 20000-20999.
LxMenu dyApp();
LxMenu dyOption(int id);
void dyDaily();
void dySaveTo(const std::string& path);
void dyLoadFrom(const std::string& path);
int dyGeneration();   // 0 if no dynasty running
