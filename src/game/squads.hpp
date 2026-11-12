#pragma once
#include "common.hpp"
// Deeper social life: reputation, town squads (cliques), townie couples & feuds,
// and school clubs for kids and teens.
enum Clique { CQ_IT, CQ_VARSITY, CQ_ART, CQ_BRAIN, CQ_ALT, CQ_COUNT };
extern const char* CLIQUE_NAMES[CQ_COUNT];
extern const char* CLIQUE_VIBE[CQ_COUNT];
extern const char* CLUB_NAMES[CQ_COUNT];   // school club that feeds each squad
int cliqueOf(int sim);                      // which squad a townie runs with (-1 = none)
const char* repLabel(int rep);
float repBonus();                           // added to social success chance
void socialRipple(int socialId, int target);   // reputation, squad standing, witnesses
void npcRelTick();                          // townies date, break up and feud on their own
void squadGreet(int sim);                   // a squad-mate says hi when you arrive
