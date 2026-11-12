#pragma once
#include "common.hpp"
#include <string>
#include <vector>
// Vacation worlds, courts and the celebrity life (travelcourt.cpp).
// Vacations: 3 destinations (Coral Cove, Frostpeak, Belle Ville), booked at the Travel Agency rabbit hole.
// Courts: serious busts get a court date - plead, hire a lawyer or defend yourself, then the verdict.
// Celebrity: a fame-gated career plus paparazzi, fans and red-carpet moments around town.
bool vacationRoom(int room);
bool vacationOpen(int room, std::string* why);
std::vector<std::pair<std::string, int>> travelOptions();
bool travelDo(int opt);
std::vector<std::pair<std::string, int>> courtOptions();
bool courtDo(int opt);
void courtSchedule(int crime);
void travelCourtHourly(int hour);
void celebShiftBonus();

bool vacationTripRoom(int r);
const char* vacationName();
int vacationSuite();
int vacationMapRooms(int* out);
int vacationDest();
const char* vacationShort(int r);
bool travelStartOwned(int dest, int days, const char* homeName);
