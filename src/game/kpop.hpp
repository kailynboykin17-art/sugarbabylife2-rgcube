#pragma once
#include "common.hpp"
#include <string>
// K-Pop Trainee mode (kpop.cpp): Starlight Entertainment's trainee program.
// Monthly evaluations, positions with their own training minigames, a survival show,
// dorm roommates, a no-dating contract with scandals, and hidden lineup chemistry.
void kpopOpen();
bool kpopUpdate(const Input& in);   // false = back to the menu
void kpopDrawTop(float t);
void kpopDrawBottom(float t);
void kpopSaveTo(const std::string& path);
void kpopLoadFrom(const std::string& path);
void kpopReset();
