#pragma once
#include <vector>
#include <string>
#include "social.hpp"
// Realistic parenting & family drama (famdrama.cpp): family-only interactions (parents, kids, siblings),
// teens sneaking out at night, sibling fights, and dinner-table family drama events.
void famOptions(int target, std::vector<SocialOpt>& v);
bool famDoSocial(int id, int target);   // true = handled
void famHourly(int hour);
void famDaily();
// Family dynamics (Sims 4 Growing Together style): per pair of household members.
enum { DY_NONE, DY_CLOSE, DY_DISTANT, DY_SUPPORTIVE, DY_PERMISSIVE, DY_DIFFICULT, DY_STRICT, DY_JOKESTERS, DY_COUNT };
extern const char* DYN_NAMES[DY_COUNT];
int famDynamic(int a, int b);
void famSetDynamic(int a, int b, int d);
std::vector<std::pair<std::string, int>> famDynMenu(int id);   // ids 22000-22999 (Grow Up > Family Dynamics)
void famSaveTo(const std::string& path);
void famLoadFrom(const std::string& path);
