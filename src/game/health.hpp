#pragma once
#include "common.hpp"
#include <string>
#include <vector>
// Health & rabbit holes (health.cpp): fictional illnesses with symptoms, diagnosis, treatment,
// recovery, sick days and family reactions; plus the third map page of "rabbit hole" places
// (clinic, hospital, school, work, police, city hall, bank, post office) you visit without going in.
enum Illness : uint8_t { IL_NONE, IL_COLD, IL_FLU, IL_FEVER, IL_ALLERGY, IL_STOMACH, IL_FATIGUE, IL_INJURY, IL_HEADACHE, IL_COUNT };
const char* illnessName(int il);
int illnessOf(int sim);
bool isSick(int sim);
bool diagnosed(int sim);
std::string healthStatus(int sim);            // "Healthy", "Feeling sick", "Flu (2 days left)"
void catchIllness(int sim, int il);           // il < 0: random
void healthHourly(int hour);
bool healthDaily(int sim);                    // per sim, from the daily tick; true if they passed away
void healthTreat(int sim, int level);         // 0 meds, 1 full cure (ER)

// rabbit holes
constexpr int RH_COUNT = 10;
const char* rabbitHoleName(int k);
std::vector<std::pair<std::string, int>> rabbitHoleOptions(int k);
bool rabbitHoleDo(int k, int opt);            // true = you head off (close the phone)
