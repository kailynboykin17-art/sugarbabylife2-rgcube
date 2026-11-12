#pragma once
#include "common.hpp"
// Dollhouse view (houseview.cpp): your home as a cutaway dollhouse (peaked roof for houses, flat skyline roof
// for apartments). Every room is a live mini version of itself; tap one on the touch screen to walk in.
void houseViewOpen();
bool houseViewUpdate(const Input& in);   // false = back to play
void houseViewDrawTop(float t);
void houseViewDrawBottom(float t);
