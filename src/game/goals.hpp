#pragma once
#include "common.hpp"
#include <string>
// Life goals & fun extras:
//  * Aspirations - pick a life goal, complete 4 levels of milestones
//  * Wants       - 3 small rotating wishes; fulfilling them earns Satisfaction points
//  * Rewards     - spend points on perks (slower needs, faster skills...) and treats
//  * Collections - shells, butterflies, charms & crystals found around town
//  * Weather     - sunny / cloudy / rain / storm / snow / heatwave, rolled daily by season

// things that happen in the game that wants & goals listen for
enum GoalEv {
    GE_SOCIAL, GE_NEWFRIEND, GE_TRAVEL, GE_COOK, GE_POST, GE_DANCE, GE_MINIGAME, GE_SHIFT, GE_BUYOUTFIT, GE_BATH,
    GE_SKILLUP, GE_COLLECT, GE_SLEEP, GE_TV, GE_READ, GE_MAKEUP, GE_DATE, GE_TOYS, GE_HOMEWORK, GE_CLUB, GE_COUNT
};
void goalNotify(int ev, int arg = 0);
namespace gm {
enum Metric { M_STYLE, M_COOKING, M_CHARISMA, M_FITNESS, M_CREATIVE, M_LOGIC, M_OUTFITS, M_FOLLOWERS, M_FAME, M_FRIENDS, M_BFFS,
              M_DISHES, M_JOBLVL, M_MONEY, M_SHIFTS, M_PARTNER, M_MARRIED, M_KIDS, M_TROPHIES, M_REP, M_SQUAD, M_COLLECT, M_SETS,
              M_POSTS, M_DATES, M_DEGREE };
}
int goalMetric(int m);   // the numbers aspirations (and legacy challenges) track

// aspirations
constexpr int NUM_ASPS = 8;
extern const char* ASP_NAMES[NUM_ASPS];
extern const char* ASP_DESC[NUM_ASPS];
bool aspAllowed(int a);   // age-appropriate?
void aspChoose(int a);
void goalsHourly(int hour);
void goalsDaily();

// rewards & perks
enum Perk : uint32_t {
    PK_NEVERWEARY = 1, PK_IRONBLADDER = 2, PK_SLOWHUNGER = 4, PK_SOCIALSTAR = 8, PK_QUICKLEARN = 16,
    PK_FANMAGNET = 32, PK_FASTTRACK = 64, PK_FRESH = 128
};
bool hasPerk(uint32_t p);
float perkNeedMult(int need);      // decay multiplier from perks (MC only)
float perkSkillMult();
float perkSocialBonus();
float perkFollowerMult();
float perkPerfMult();

// collections
constexpr int NUM_SETS = 4, SET_SIZE = 6;
extern const char* SET_NAMES[NUM_SETS];
extern const char* COLLECT_NAMES[NUM_SETS][SET_SIZE];
int collectSetFor(int room);        // -1 = nothing to find here
void collectSearch(int set);        // roll a find
int collectCount();

// weather
enum Weather : uint8_t { WX_SUNNY, WX_CLOUDY, WX_RAIN, WX_STORM, WX_SNOW, WX_HEAT, WX_COUNT };
extern const char* WEATHER_NAMES[WX_COUNT];
void weatherRoll();
bool roomOutdoors(int room);
void drawWeather(float t, bool outdoors);                     // full-screen overlay
void drawWeatherInWindow(float x, float y, float w, float h, float t);
void weatherArrive(int room);                                   // moodlets when you step outside

// the Goals phone app (Wants / Life Goal / Rewards / Collections)
void goalsAppDrawTop(float t);
void goalsAppDrawBottom(float t);
void goalsAppUpdate(const Input& in);

// life moments: little story events with choices
bool momentActive();
bool momentUpdate(const Input& in);   // true = input consumed
void momentDrawTop(float t);
void momentDrawBottom(float t);
void momentMaybeStart();              // called hourly
typedef std::string (*MomentFn)(int choice, int ctx);   // returns the result text ("" = just close)
void momentCustom(const std::string& title, const std::string& text, const char* o0, const char* o1, const char* o2, MomentFn fn, int ctx, int portraitSim = -1);
bool crimeMomentMaybe();              // crime.cpp: blackmail texts, burglars...
void drawWeatherIcon(float x, float y);
