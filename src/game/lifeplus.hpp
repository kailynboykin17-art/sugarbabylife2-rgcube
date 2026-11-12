#pragma once
#include "common.hpp"
#include <string>
// Deeper life sim (lifeplus.cpp): genetics & inheritance, player-started fights, online shopping
// with package deliveries, sentiments (lasting feelings after big moments) and lot traits.
enum SentKind : uint8_t { SN_BESTMEM, SN_GRATEFUL, SN_HURT, SN_FURIOUS, SN_EMBARRASSED, SN_GUILTY, SN_COUNT };
enum LotTrait : uint8_t { LT_COZY, LT_PARTY, LT_CHEF, LT_ROMANTIC, LT_PEACEFUL, LT_BOOKNOOK, LT_GREEN, LT_HAUNTED, LT_GREMLINS, LT_PORCH, LT_COUNT };
// favorites & zodiac
extern const char* FAV_COLOR_NAMES[12];
extern const char* ZODIAC_NAMES[12];
int zodiacCompat(int a, int b);   // +2 same element, +1 complementary, 0 otherwise
// genetics
void geneticsOnBirth(int kid, int pa, int pb);
void geneticsOnAgeUp(int kid);
std::string geneticsBlurb(int kid);           // "Takes after Mom: eyes, smile..."
// sentiments
void addSentiment(int sim, int kind, int days);
int sentimentWith(int sim);                   // kind or -1
const char* sentimentName(int kind);
// lot traits
bool hasLot(int t);
float lotCookBonus();                         // extra time scale for the Chef's Kitchen
// fights
void startFight(int t);
// hooks
void lifeplusAfterSocial(int id, int t);
void lifeplusHourly(int hour);
// phone app: My Lot
void lotAppUpdate(const Input& in); void lotAppDrawTop(float t); void lotAppDrawBottom(float t);
