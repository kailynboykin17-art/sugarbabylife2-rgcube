#pragma once
#include "common.hpp"
#include <string>
// Love Villa (villa.cpp): a 20-day dating-villa reality challenge (original, adults only) - islanders,
// bombshells, recouplings, couple challenges & dates, Casa Amor, public votes, dumpings and a finale.
// Also: the 12 cross-system story events (storyEventsDaily) from the content pack.
void villaOpen();
bool villaUpdate(const Input& in);
void villaDrawTop(float t);
void villaDrawBottom(float t);
void villaSaveTo(const std::string& path);
void villaLoadFrom(const std::string& path);
void storyEventsDaily();
