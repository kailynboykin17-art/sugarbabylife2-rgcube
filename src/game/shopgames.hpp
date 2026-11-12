#pragma once
#include "common.hpp"
// Store minigames: 0 Budget Grocery Run, 1 Library Book Match, 2 Thrift Grab, 3 Corner Store Cashier
void shopStart(int kind);
bool shopUpdate(const Input& in);
void shopDrawTop(float t);
void shopDrawBottom(float t);
