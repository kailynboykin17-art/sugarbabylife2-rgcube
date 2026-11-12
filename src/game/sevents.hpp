#pragma once
#include "common.hpp"
#include <string>
#include <vector>
// Social Events (sevents.cpp): plan 42 kinds of events on the calendar (weddings, parties, dates,
// proposals, grieving...). At start time the household travels to the venue, guests arrive, and the
// event has goals; finish them for Bronze / Silver / Gold. Gold gives a memory moodlet + memory + news.
typedef std::vector<std::pair<std::string, int>> SeMenu;
SeMenu seApp();               // the Events app: active event goals, or plan / start one
SeMenu sePlanMenu(int day);   // calendar "+ Add Event" -> social events for that day
SeMenu seOption(int id);      // ids 14000-14999
void seHourly(int hour);
void seSaveTo(const std::string& path);
void seLoadFrom(const std::string& path);
