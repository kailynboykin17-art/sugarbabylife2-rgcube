#pragma once
#include "lifeext.hpp"
// Life Extras (life3.cpp), Daily Life > Life Extras. Menu ids 29000-29999, sidecar ".lf3".
//  Vacation homes (Coral Cove / Frostpeak / Belle Ville): buy, free family weekends, rent out, local events.
//  Attributes (Strength / Brains / Charm: 5 daily activities each), Dream goals, TV skill channels, badges.
//  Family: pregnancy cravings & baby showers, adoption agency, learner's permit / curfew / fender benders,
//  hand-me-downs, the family cookbook (recipes carried through Dynasty generations).
//  Town: run for Mayor, neighbor feuds, seasonal parades & fireworks, "Rose Garden" dating show.
//  Style: seasonal fashion drops, Get-Ready-With-Me videos, thrift-store digs.
//  Buffs (Sims 4 style): visible buffs with +power & timers, hidden buffs (skill boosts & cooldowns),
//  permanent trait / status buffs that feed the mood, emotion-based skill speed, new triggers (soaked, well rested).
LxMenu l3App();
LxMenu l3Option(int id);
void l3Hourly(int hour);
void l3Daily();
float l3SkillMult(int skill);                 // hidden & emotion buffs on skill speed (main sim)
void l3PermanentBuffs(int score[], int n);    // permanent trait/status buffs added to the main sim's mood
void l3SaveTo(const std::string& path);
void l3LoadFrom(const std::string& path);
int l3MCAuto();                               // selected Sim autonomy: 0 off, 1 medium, 2 full
int l3AutoOf(int sim);                        // per-Sim autonomy: 0 off, 1 medium, 2 full
float l3ToastMult();                          // settings: toast duration
float l3TextMult();                           // settings: dialogue typing speed
int l3PopPct(int basePct);                    // settings: pop-up / drama frequency
// save.cpp: rotating daily backups (3 kept per slot)
int backupDay(int k);
struct Game; bool restoreBackup(int k, Game& out);
