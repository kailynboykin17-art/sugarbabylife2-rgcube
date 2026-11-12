#pragma once
#include "common.hpp"
#include <string>
// Life story (lifestory.cpp): milestones journal, family roles & dynamics, five kinds of
// (non-graphic) passing with grief & memorials, adoption, and the Life Book screen.

enum MilestoneId : uint8_t {
    MI_SMILE, MI_LAUGH, MI_WORDS, MI_STEPS, MI_FOODS, MI_POTTY, MI_BIRTHDAY,
    MI_FRIEND, MI_SLEEPOVER, MI_SCHOOLDAY, MI_AWARD, MI_HOBBY, MI_BESTIE, MI_CRUSH, MI_PARTY,
    MI_DANCE, MI_RELATIONSHIP, MI_DATE, MI_KISS, MI_GRAD, MI_JOB, MI_PROMO, MI_MOVEOUT, MI_MARRIED, MI_PARENT, MI_CAREERPEAK,
    MI_COUNT
};
enum DeathKind : uint8_t { DK_NONE, DK_OLDAGE, DK_ILLNESS, DK_ACCIDENT, DK_ENVIRON, DK_SPIRIT,
                           DK_HUNGER, DK_FIRE, DK_DROWN, DK_ELECTRIC, DK_LAUGH, DK_EMBARRASS, DK_PUFFER, DK_METEOR, DK_FROZEN, DK_HEAT, DK_EXHAUST, DK_HEARTBREAK, DK_MURDER, DK_COUNT };

void milestone(int sim, int id, int other = -1);      // records once per sim (moodlet, memory, family warmth)
bool hasMilestone(int sim, int id);
void lifeStoryAgeUp(int sim);                          // from ageUp()
void lifeStoryHourly(int hour);
void lifeStoryDaily();
void lifeStoryAfterSocial(int t);                      // relatives greet & bond differently
void lifeStoryAdopted(int kid);

bool simDead(int s);
int deathKind(int s);
const char* deathName(int kind);
std::string familyRole(int a, int b);                  // what b is to a ("Mother", "Half-brother", "" if none)
std::string familyDynamic(int a, int b);               // "Close Family", "Rival Siblings"...
int pendingMemorial();                                 // sim needing a memorial, -1 none
void holdMemorial();
std::string seanceContact();                           // talk to a spirit at the haunted house ("" = nobody)

void lifeBookOpen();
bool lifeBookUpdate(const Input& in);                  // false = back to the menu
void lifeBookDrawTop(float t);
void lifeBookDrawBottom(float t);

void lifeKill(int sim, int kind);   // story/crime deaths (respects nothing: caller checks settings)
