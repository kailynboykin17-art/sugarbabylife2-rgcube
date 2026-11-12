#pragma once
#include "common.hpp"
#include <string>
#include <vector>
// Crime, heat & the police, scandals (The Tea tabloid app) and blackmail - Teen 13+, nothing graphic.
enum CrimeId { CRIME_SHOPLIFT, CRIME_GRAFFITI, CRIME_SNEAKIN, CRIME_DINEDASH, CRIME_PICKPOCKET, CRIME_HACK, CRIME_HEIST, CRIME_EAVESDROP,
               CRIME_TP, CRIME_DITCH, CRIME_CHEAT, CRIME_HAMSTERS, CRIME_SKIPCLASS, CRIME_SPRAYCAR,
               CRIME_TAGSWAP, CRIME_FAKEID, CRIME_SCALP, CRIME_JOYRIDE, CRIME_FAKESICK, CRIME_SAMPLES, CRIME_COPYHW, CRIME_CATFISH, CRIME_COUNT };
enum ScandalKind { SC_SHOPLIFT, SC_ARREST, SC_TWOTIMING, SC_BLACKMAILER, SC_SECRET, SC_VANDAL, SC_DINEDASH, SC_PICKPOCKET, SC_MELTDOWN, SC_FAKEBAG, SC_KIND_COUNT };
constexpr int NUM_SECRETS = 18;
extern const char* SECRETS[NUM_SECRETS];
int secretOf(int sim);
int mischiefLevel();
int heatStars();
int dirtIndex(int sim);                   // -1 if you know nothing on them
int copSim();                             // Officer Reyes (created on demand)
bool copHere();

struct CrimeOpt { std::string label; int crime; };
std::vector<CrimeOpt> crimeVenueOptions(int room);
void crimeAttempt(int crime, int target = -1);
void crimeHeistResult(bool caught, int loot);
void scandalBreak(int kind);              // a scandal about YOU
void npcScandal(int sim, int kind, int extra = 0);
std::string scandalHeadline(const struct Scandal& s);
void crimeHourly(int hour);
void crimeDaily();
void crimeOnTravel(int room);

// socials: Pickpocket, Snoop Phone, Blackmail, Expose, Keep Secret
void crimeSocialOptions(int target, std::vector<struct SocialOpt>& v);
void crimeDoSocial(int id, int target);

// The Tea: the tabloid phone app
void teaAppDrawTop(float t);
void teaAppDrawBottom(float t);
void teaAppUpdate(const Input& in);
