#pragma once
#include "common.hpp"
// Boutique stylist shifts: clients ask for a vibe, you put together the look.
void styleStart(int clients, bool forWork);
bool styleUpdate(const Input& in);   // false = finished
void styleDrawTop(float t);
void styleDrawBottom(float t);
