#pragma once
#include "common.hpp"
#include <string>
#include <vector>
// Jail, boarding school and the fist-fight dust cloud (lockup.cpp).
// Jail: guilty repeat offenders, botched heists and public brawls the cops see get locked up at
//   Westbrook County Jail (a real place you live in until release day, or until bail is posted).
// Boarding school: kids & teens can enroll at Rosewood Academy for a 14-day term (or get sent by
//   their parents); on school days you live in the dorm, weekends you can go home.
bool jailed();
bool boarding();
bool lockupBlocks(int room, std::string* why);   // travel gate while locked up / on campus
void jailSentence(int days, const char* why);
void boardingEnroll(bool sentByParents);
std::vector<std::pair<std::string, int>> boardingSchoolOptions();   // extra School rabbit-hole options (ids 50+)
bool boardingSchoolDo(int opt);
bool lockupAction(int act);     // jail & dorm spot actions (district DA ids)
void lockupHourly(int hour);
void lockupDaily();

void murderAttempt(int target);   // adults only; Permanent Death OFF = they survive (hospital)
// fist fight: a cartoon dust cloud with fists, feet & POW!s over the two fighters
void brawlStart(int a, int b, bool publicFight);
bool brawlHides(int sim);
void brawlDraw(float t);
void brawlFrame(float dt);
