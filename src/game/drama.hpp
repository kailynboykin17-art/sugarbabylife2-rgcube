#pragma once
#include <string>
// Drama & social mechanics (drama.cpp)
void dramaAfterSocial(int id, int target, int dFr, int dRo);   // thoughts, first impressions, grudges, BFFs
float dramaSocialBonus(int target);                             // luck & grudges nudge social success
bool dramaHasGrudge(int sim);
int dramaBff();                                                 // your best friend (-1 if none yet)
int dramaFavoriteGift(int sim);
const char* dramaGiftReaction(int sim, int kind);               // non-null if it was their favourite
void dramaSleepover(int target);
void dramaHourly(int hour);
void dramaDaily();
bool dramaMomentMaybe();                                         // BFF's-boyfriend drama + 10 story events
void dramaAddGrudge(int sim, int days);                        // they hold it against you for a few days
