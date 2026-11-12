#pragma once
#include "common.hpp"
enum { MG_MEMORY, MG_PINATA, MG_CATCH, MG_SIMON, MG_TTT, MG_RUNWAY, MG_NAILS, MG_KARAOKE, MG_VOLLEY, MG_SMOOTHIE };
extern const char* MG_NAMES[10];
extern const void* g_mgContest;   // the TownEvent being competed in (nullptr = just for fun)
extern int g_mgLastScore;
void mgStart(int game);
bool mgUpdate(const Input& in);   // false = back to the house
void mgDrawTop(float t);
void mgDrawBottom(float t);
