#pragma once
#include "common.hpp"
#include <string>
// Family life (family.cpp): the Family Planner (curfew, chore chart, weekly family traditions,
// allowance), teens sneaking out after curfew (your kids... or you), and getting grounded.
enum Chore : uint8_t { CHR_NONE, CHR_DISHES, CHR_LAUNDRY, CHR_TIDY, CHR_TRASH, CHR_PET, CHR_GARDEN, CHR_COUNT };
enum FamEvent : uint8_t { FEV_PANCAKES, FEV_BEACH, FEV_DINNER, FEV_GAMENIGHT, FEV_MOVIE, FEV_COUNT };
extern const char* const CHORE_NAMES[CHR_COUNT];
extern const char* const FAMEV_NAMES[FEV_COUNT];
extern const char* const FAMEV_WHEN[FEV_COUNT];
void familyHourly(int hour);
void familyDaily();
bool familyTravelGate(int room);   // false = blocked or waiting on a choice (sneaking out, grounded)
void familyOnArrive(int room);      // coming home while sneaking...
std::string familyStatusLine();     // "Curfew 10 PM - Lily is out past curfew!" etc.
