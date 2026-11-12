#pragma once
// Your love life (romance.cpp): first dates, situationships, cheating scandals, breakups & exes,
// engagements, weddings & marriage upkeep, romantic rivals - all shaped by compatibility.
void romanceAfterSocial(int id, int t, bool success);   // hook from doSocial
void romanceAfterDate(int partner, float score);         // hook from endDate
void romanceHourly(int hour);
int romanceCompat(int t);                                // -5..+8ish, MC vs t
const char* romanceVibe(int t);                          // "Soulmates?!" etc.
