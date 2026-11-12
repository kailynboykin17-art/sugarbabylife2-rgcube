#pragma once
#include "common.hpp"
#include <string>
#include <vector>
// Deep Life (deeplife.cpp): romance chemistry (turn-ons / turn-offs / compatibility), wants & fears,
// habits, promises & lies with remembered receipts, and household switching with off-screen progress
// and a "While You Were Away" recap. Option ids 11000-11999; dlOption returns the next menu.
typedef std::vector<std::pair<std::string, int>> DlMenu;
DlMenu dlApp(int which);   // 0 Chemistry, 1 Wants & Fears, 2 Habits, 3 Promises, 4 Households
DlMenu dlOption(int id);
bool dlMoveOut(int mover, int partner, int32_t take);   // real move-out: new household + new home; the old family keeps living
int dlCompat(int a, int b);   // 0..100 romantic compatibility (player vs NPC)
void dlHabit(int k, int amt);   // 5 = impulse shopping
void dlHourly(int hour);
void dlDaily();
void dlSaveTo(const std::string& path);
void dlLoadFrom(const std::string& path);
