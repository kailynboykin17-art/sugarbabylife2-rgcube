#pragma once
#include "lifeext.hpp"
// Kid Life (kidlife.cpp): toy box & toy shop, first crush (same-age kids only, sweet & shy),
// Elementary / Middle / High school days with grade levels and school-life events, purse toggle.
// Menu ids 16000-16999.
LxMenu kidApp();
LxMenu kidOption(int id);
const char* schoolName(int stage);   // "Elementary School" / "Middle School" / "High School"
bool kidOutfitOk(int outfit, int stage);
bool outfitFits(int outfit, int stage, bool guy);   // kids-only range + guys-only range  // kids' clothes only for child-preteen; kids can't wear grown-up looks from the kid wardrobe filter
void kidHourly(int hour);
void kidDaily();
