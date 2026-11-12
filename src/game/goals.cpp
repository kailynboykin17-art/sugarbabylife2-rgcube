#include "lifeext.hpp"
#include "clubs.hpp"
// Aspirations, Wants, Rewards, Collections and Weather (see goals.hpp)
#include "goals.hpp"
#include "legacy.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "social.hpp"
#include "extras.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>

using namespace col;
using namespace gm;

static int popcount32(uint64_t v) { int n = 0; while (v) { n += v & 1; v >>= 1; } return n; }

// ================================================================ wants
struct WantDef { const char* text; int ev; int arg; Stage minSt, maxSt; int pts; };
static const WantDef WANTS[] = {
    {"Chat with someone", GE_SOCIAL, -1, ST_TODDLER, ST_ELDER, 50},
    {"Make a new friend", GE_NEWFRIEND, -1, ST_CHILD, ST_ELDER, 120},
    {"Cook a meal", GE_COOK, -1, ST_TEEN, ST_ELDER, 90},
    {"Go to Sunset Beach", GE_TRAVEL, R_BEACH, ST_CHILD, ST_ELDER, 70},
    {"Hit the Glam Mall", GE_TRAVEL, R_MALL, ST_CHILD, ST_ELDER, 70},
    {"Stroll through Rosebud Park", GE_TRAVEL, R_PARK, ST_CHILD, ST_ELDER, 60},
    {"Grab a latte at Cafe Latte", GE_TRAVEL, R_CAFE, ST_TEEN, ST_ELDER, 60},
    {"Post on InstaGlam", GE_POST, -1, ST_TEEN, ST_ELDER, 70},
    {"Dance!", GE_DANCE, -1, ST_CHILD, ST_ELDER, 60},
    {"Play a mini-game", GE_MINIGAME, -1, ST_CHILD, ST_ELDER, 60},
    {"Work a great shift (3+ stars)", GE_SHIFT, 3, ST_TEEN, ST_ELDER, 140},
    {"Buy a new outfit", GE_BUYOUTFIT, -1, ST_CHILD, ST_ELDER, 100},
    {"Take a bubble bath or shower", GE_BATH, -1, ST_TODDLER, ST_ELDER, 50},
    {"Level up a skill", GE_SKILLUP, -1, ST_CHILD, ST_ELDER, 110},
    {"Flirt with someone cute", GE_SOCIAL, SO_FLIRT, ST_YA, ST_ELDER, 80},
    {"Hug someone", GE_SOCIAL, SO_HUG, ST_TODDLER, ST_ELDER, 60},
    {"Make someone laugh", GE_SOCIAL, SO_JOKE, ST_CHILD, ST_ELDER, 70},
    {"Compliment someone's look", GE_SOCIAL, SO_COMPLIMENT, ST_CHILD, ST_ELDER, 60},
    {"Find a collectible", GE_COLLECT, -1, ST_CHILD, ST_ELDER, 90},
    {"Get pampered at the spa", GE_TRAVEL, R_SPA, ST_TEEN, ST_ELDER, 80},
    {"Catch a movie", GE_TRAVEL, R_CINEMA, ST_CHILD, ST_ELDER, 70},
    {"Go ice skating", GE_TRAVEL, R_RINK, ST_CHILD, ST_ELDER, 70},
    {"Get a good night's sleep", GE_SLEEP, -1, ST_TODDLER, ST_ELDER, 50},
    {"Watch some TV", GE_TV, -1, ST_TODDLER, ST_ELDER, 40},
    {"Read a book", GE_READ, -1, ST_CHILD, ST_ELDER, 60},
    {"Do your makeup", GE_MAKEUP, -1, ST_TEEN, ST_ELDER, 60},
    {"Spill some tea", GE_SOCIAL, SO_SPILLTEA, ST_PRETEEN, ST_ELDER, 70},
    {"Take a selfie with someone", GE_SOCIAL, SO_SELFIE, ST_PRETEEN, ST_ELDER, 70},
    {"Go on a date", GE_DATE, -1, ST_YA, ST_ELDER, 150},
    {"Play pretend or with toys", GE_TOYS, -1, ST_TODDLER, ST_PRETEEN, 60},
    {"Finish your homework", GE_HOMEWORK, -1, ST_CHILD, ST_TEEN, 90},
    {"Go to a club meeting", GE_CLUB, -1, ST_CHILD, ST_TEEN, 90},
};
static const int NUM_WANTS = sizeof(WANTS) / sizeof(WANTS[0]);

static bool wantOk(int w) {
    const WantDef& d = WANTS[w];
    const Sim& me = MC();
    if (me.stage < d.minSt || me.stage > d.maxSt) return false;
    if (d.ev == GE_SHIFT && me.career == CR_NONE) return false;
    if (d.ev == GE_HOMEWORK && me.homework) return false;
    return true;
}
static void rollWant(int slot) {
    for (int tries = 0; tries < 60; tries++) {
        int w = irand(NUM_WANTS);
        if (!wantOk(w)) continue;
        bool dup = false;
        for (int k = 0; k < 3; k++) if (k != slot && G.wantType[k] == w) dup = true;
        if (dup) continue;
        G.wantType[slot] = (uint8_t)w;
        return;
    }
    G.wantType[slot] = 0;
}
static void refreshWants() { for (int k = 0; k < 3; k++) rollWant(k); G.wantsDay = G.day; }

// ================================================================ aspirations
const char* ASP_NAMES[NUM_ASPS] = {"Fashion Icon", "Social Butterfly", "Master Chef", "Career Queen",
                                   "Big Happy Family", "InstaGlam Royalty", "Collector Queen", "Brainiac"};
const char* ASP_DESC[NUM_ASPS] = {"Build your style, your closet and your fame.", "Be loved by everyone in town.",
                                  "Cook your way to culinary legend.", "Climb to the very top of your career.",
                                  "Find love and raise a happy family.", "Become the biggest star online.",
                                  "Find every shell, butterfly, charm and crystal.", "Become the smartest sim in town."};
struct Milestone { int metric; int target; const char* label; };
static const Milestone ASP_STEPS[NUM_ASPS][4][2] = {
    {{{M_STYLE, 2, "Style skill 2"}, {M_OUTFITS, 4, "Own 4 outfits"}}, {{M_STYLE, 4, "Style skill 4"}, {M_FOLLOWERS, 500, "500 followers"}},
     {{M_STYLE, 6, "Style skill 6"}, {M_OUTFITS, 8, "Own 8 outfits"}}, {{M_STYLE, 8, "Style skill 8"}, {M_FAME, 3, "3 fame stars"}}},
    {{{M_FRIENDS, 3, "3 friends"}, {M_REP, 10, "Reputation 10+"}}, {{M_FRIENDS, 6, "6 friends"}, {M_SQUAD, 1, "Join a squad"}},
     {{M_BFFS, 2, "2 best friends"}, {M_REP, 40, "Reputation 40+"}}, {{M_FRIENDS, 10, "10 friends"}, {M_REP, 70, "Reputation 70+"}}},
    {{{M_COOKING, 2, "Cooking skill 2"}, {M_DISHES, 5, "Cook 5 dishes"}}, {{M_COOKING, 4, "Cooking skill 4"}, {M_DISHES, 15, "Cook 15 dishes"}},
     {{M_COOKING, 6, "Cooking skill 6"}, {M_DISHES, 30, "Cook 30 dishes"}}, {{M_COOKING, 8, "Cooking skill 8"}, {M_DISHES, 60, "Cook 60 dishes"}}},
    {{{M_JOBLVL, 2, "Reach job level 2"}, {M_SHIFTS, 5, "Work 5 shifts"}}, {{M_JOBLVL, 3, "Reach job level 3"}, {M_MONEY, 5000, "Have $5,000"}},
     {{M_JOBLVL, 4, "Reach job level 4"}, {M_SHIFTS, 25, "Work 25 shifts"}}, {{M_JOBLVL, 5, "Top of your career"}, {M_MONEY, 25000, "Have $25,000"}}},
    {{{M_FRIENDS, 2, "2 friends"}, {M_DATES, 1, "Go on a date"}}, {{M_PARTNER, 1, "Be in a relationship"}, {M_DATES, 5, "Go on 5 dates"}},
     {{M_MARRIED, 1, "Get married"}, {M_KIDS, 1, "Have or adopt a child"}}, {{M_KIDS, 2, "Raise 2 kids"}, {M_FRIENDS, 6, "6 friends"}}},
    {{{M_POSTS, 5, "Make 5 posts"}, {M_FOLLOWERS, 300, "300 followers"}}, {{M_POSTS, 20, "Make 20 posts"}, {M_FOLLOWERS, 2000, "2,000 followers"}},
     {{M_FAME, 2, "2 fame stars"}, {M_STYLE, 4, "Style skill 4"}}, {{M_FAME, 4, "4 fame stars"}, {M_TROPHIES, 2, "Win 2 contests"}}},
    {{{M_COLLECT, 4, "Find 4 collectibles"}, {M_COLLECT, 4, ""}}, {{M_COLLECT, 10, "Find 10 collectibles"}, {M_SETS, 1, "Complete a set"}},
     {{M_COLLECT, 16, "Find 16 collectibles"}, {M_SETS, 2, "Complete 2 sets"}}, {{M_COLLECT, 24, "Find all 24"}, {M_SETS, 4, "Complete every set"}}},
    {{{M_LOGIC, 2, "Logic skill 2"}, {M_CREATIVE, 2, "Creativity skill 2"}}, {{M_LOGIC, 4, "Logic skill 4"}, {M_TROPHIES, 1, "Win a contest"}},
     {{M_LOGIC, 6, "Logic skill 6"}, {M_DEGREE, 1, "College degree (or an A at school)"}}, {{M_LOGIC, 8, "Logic skill 8"}, {M_CREATIVE, 6, "Creativity skill 6"}}},
};
static const int ASP_PTS[4] = {400, 700, 1000, 1600}, ASP_CASH[4] = {150, 400, 800, 2000};

static int metric(int m);
int goalMetric(int m) { return metric(m); }
static int metric(int m) {
    const Sim& me = MC();
    switch (m) {
    case M_STYLE: return skillLevel(me, SK_STYLE);
    case M_COOKING: return skillLevel(me, SK_COOKING);
    case M_CHARISMA: return skillLevel(me, SK_CHARISMA);
    case M_FITNESS: return skillLevel(me, SK_FITNESS);
    case M_CREATIVE: return skillLevel(me, SK_CREATIVITY);
    case M_LOGIC: return skillLevel(me, SK_LOGIC);
    case M_OUTFITS: return popcount32(G.ownedOutfits) + popcount32(G.ownedOutfits2);
    case M_FOLLOWERS: return (int)std::min<uint32_t>(G.followers, 100000000u);
    case M_FAME: return fameStars();
    case M_FRIENDS: case M_BFFS: {
        int n = 0, need = m == M_FRIENDS ? 40 : 80;
        for (const Rel& r : G.rels) if ((r.a == G.mc || r.b == G.mc) && r.met && r.fr >= need) n++;
        return n;
    }
    case M_DISHES: return (int)G.dishes;
    case M_JOBLVL: return me.career == CR_NONE ? 0 : me.careerLevel + 1;
    case M_MONEY: return G.money;
    case M_SHIFTS: return G.shifts;
    case M_PARTNER: return partnerOf(G.mc) >= 0 ? 1 : 0;
    case M_MARRIED: { int p = partnerOf(G.mc); const Rel* r = p >= 0 ? findRel(G.mc, p) : nullptr; return r && r->status == RS_MARRIED ? 1 : 0; }
    case M_KIDS: { int n = 0; for (size_t i = 0; i < G.sims.size(); i++) if ((int)i != G.mc && G.sims[i].household && G.sims[i].stage <= ST_TEEN) n++; return n; }
    case M_TROPHIES: return G.trophies;
    case M_REP: return G.rep;
    case M_SQUAD: return G.clique >= 0 ? 1 : 0;
    case M_COLLECT: return popcount32(G.collected);
    case M_SETS: { int n = 0; for (int s = 0; s < NUM_SETS; s++) if (((G.collected >> (s * SET_SIZE)) & 63u) == 63u) n++; return n; }
    case M_POSTS: return G.postsMade;
    case M_DATES: return G.datesHad;
    case M_DEGREE: return (me.degree || (me.stage <= ST_TEEN && me.gradePts >= 90)) ? 1 : 0;
    }
    return 0;
}

bool aspAllowed(int a) {
    const Sim& me = MC();
    if (a == 4) return me.isAdult();          // Big Happy Family
    if (a == 3) return me.stage >= ST_TEEN;   // Career Queen
    if (a == 5) return me.stage >= ST_TEEN;   // InstaGlam Royalty
    return me.stage >= ST_CHILD;
}
void aspChoose(int a) {
    if (a < 0 || a >= NUM_ASPS || !aspAllowed(a)) return;
    G.asp = (int8_t)a;
    toast(std::string("New life goal: ") + ASP_NAMES[a] + "!");
}
static void aspCheck() {
    if (G.asp < 0) return;
    int a = G.asp;
    while (G.aspLevel[a] < 4) {
        const Milestone* st = ASP_STEPS[a][G.aspLevel[a]];
        if (metric(st[0].metric) < st[0].target || metric(st[1].metric) < st[1].target) return;
        int lv = G.aspLevel[a]++;
        G.satisfaction += ASP_PTS[lv];
        G.money += ASP_CASH[lv];
        addMoodlet(MC(), "Goal Achieved!", EM_CONFIDENT, 2, 600);
        char b[160];
        if (G.aspLevel[a] >= 4) snprintf(b, sizeof b, "You completed %s! You're officially a legend. (+%d points, +$%d)", ASP_NAMES[a], ASP_PTS[lv], ASP_CASH[lv]);
        else snprintf(b, sizeof b, "%s - level %d complete! (+%d points, +$%d)", ASP_NAMES[a], lv + 1, ASP_PTS[lv], ASP_CASH[lv]);
        dlgSay(-1, b);
        fxBurst(200, 100, 30); sfxYay();
        snprintf(b, sizeof b, "%s Lv.%d! +%d pts, +$%d", ASP_NAMES[a], lv + 1, ASP_PTS[lv], ASP_CASH[lv]);
        rewardPopup("LIFE GOAL REWARD!", b);
        snprintf(b, sizeof b, "%s - level %d complete!", ASP_NAMES[a], lv + 1);
        addMemoryOnce(MEM_GOAL, G.mc, -1, b);
    }
}

// ================================================================ notifications
void goalNotify(int ev, int arg) {
    if (G.sims.empty()) return;
    legacyNotify(ev, arg);
    clubsNotify(ev, arg);
    if (G.wantsDay != G.day) refreshWants();
    for (int k = 0; k < 3; k++) {
        int w = G.wantType[k];
        if (w >= NUM_WANTS) continue;
        const WantDef& d = WANTS[w];
        if (d.ev != ev) continue;
        if (d.arg >= 0 && (ev == GE_SHIFT ? arg < d.arg : arg != d.arg)) continue;
        G.satisfaction += d.pts;
        addMoodlet(MC(), "Want Fulfilled", EM_HAPPY, 1, 120);
        char b[96]; snprintf(b, sizeof b, "%s! +%d points", d.text, d.pts);
        rewardPopup("WANT FULFILLED!", b);
        sfx(880, 1320, 120);
        G.wantType[k] = 255;   // a new want appears in a little while
    }
    aspCheck();
}

void goalsHourly(int hour) {
    legacyHourly();
    if (G.sims.empty()) return;
    if (G.wantsDay != G.day) refreshWants();
    if (G.weatherDay != G.day) weatherRoll();
    for (int k = 0; k < 3; k++) if (G.wantType[k] == 255 || (G.wantType[k] < NUM_WANTS && !wantOk(G.wantType[k]))) rollWant(k);
    // snow day: no school!
    if (hour == 6 && G.weather == WX_SNOW && isWeekday(G.day)) {
        bool any = false;
        for (Sim& s : G.sims) if (s.household && s.stage >= ST_CHILD && s.stage <= ST_TEEN && !s.schoolToday) { s.schoolToday = true; any = true; }
        if (any) toast("SNOW DAY! School is cancelled - go play in the snow!");
    }
    aspCheck();
}
void goalsDaily() { refreshWants(); weatherRoll(); }

// ================================================================ rewards
bool hasPerk(uint32_t p) { return (G.perks & p) != 0; }
float perkNeedMult(int n) {
    if (n == N_ENERGY && hasPerk(PK_NEVERWEARY)) return 0.7f;
    if (n == N_BLADDER && hasPerk(PK_IRONBLADDER)) return 0.6f;
    if (n == N_HUNGER && hasPerk(PK_SLOWHUNGER)) return 0.7f;
    if (n == N_HYGIENE && hasPerk(PK_FRESH)) return 0.6f;
    return 1.f;
}
float perkSkillMult() { return hasPerk(PK_QUICKLEARN) ? 1.3f : 1.f; }
float perkSocialBonus() { return hasPerk(PK_SOCIALSTAR) ? 0.12f : 0.f; }
float perkFollowerMult() { return hasPerk(PK_FANMAGNET) ? 1.3f : 1.f; }
float perkPerfMult() { return hasPerk(PK_FASTTRACK) ? 1.25f : 1.f; }

struct Reward { const char* name; const char* desc; int cost; uint32_t perk; };
static const Reward REWARDS[] = {
    {"Never Weary", "Energy drains 30% slower.", 1200, PK_NEVERWEARY},
    {"Iron Bladder", "Bladder drains 40% slower.", 700, PK_IRONBLADDER},
    {"Snack Queen", "Hunger drains 30% slower.", 800, PK_SLOWHUNGER},
    {"Always Fresh", "Hygiene drains 40% slower.", 700, PK_FRESH},
    {"Social Star", "Social moves succeed more often.", 1000, PK_SOCIALSTAR},
    {"Quick Learner", "All skills level 30% faster.", 1500, PK_QUICKLEARN},
    {"Fan Magnet", "30% more new followers.", 900, PK_FANMAGNET},
    {"Fast Track", "Work performance +25%.", 1300, PK_FASTTRACK},
    {"Glow-Up Elixir", "Instantly refills every need.", 250, 0},
    {"Lucky Charm", "A crisp $800 appears in your purse.", 600, 0},
    {"Happy Vibes", "Feel happy all day long.", 200, 0},
    {"Fame Boost", "+1,000 followers overnight.", 500, 0},
};
static const int NUM_REWARDS = sizeof(REWARDS) / sizeof(REWARDS[0]);

static bool buyReward(int i) {
    const Reward& r = REWARDS[i];
    if (r.perk && hasPerk(r.perk)) { toast("You already have that perk!"); return false; }
    if ((int)G.satisfaction < r.cost) { toast("Not enough Satisfaction points!"); sfxNo(); return false; }
    G.satisfaction -= r.cost;
    Sim& me = MC();
    if (r.perk) G.perks |= r.perk;
    else if (i == 8) { for (float& n : me.needs) n = 100; }
    else if (i == 9) G.money += 800;
    else if (i == 10) addMoodlet(me, "Happy Vibes", EM_HAPPY, 2, 1440);
    else if (i == 11) G.followers += 1000;
    toast(std::string("Redeemed: ") + r.name + "!");
    sfxYay(); fxBurst(160, 120, 16);
    return true;
}

// ================================================================ collections
const char* SET_NAMES[NUM_SETS] = {"Seashells", "Butterflies", "Glam Charms", "Ice Crystals"};
const char* COLLECT_NAMES[NUM_SETS][SET_SIZE] = {
    {"Pink Conch", "Sand Dollar", "Pearl Oyster", "Starfish", "Spiral Shell", "Rainbow Scallop"},
    {"Cherry Blossom Wing", "Lavender Lacewing", "Golden Monarch", "Blue Morpho", "Mint Glasswing", "Midnight Swallowtail"},
    {"Mini Heart Lock", "Glitter Flip Phone", "Tiny Lip Gloss", "Rhinestone Star", "Platform Shoe", "Diamond Ring"},
    {"Frost Flower", "Snow Diamond", "Icicle Prism", "Aurora Crystal", "Starlight Shard", "Glacier Heart"}};
static const RGB SET_COLS[NUM_SETS] = {{255, 196, 210}, {196, 170, 250}, {255, 120, 180}, {150, 220, 250}};
static uint8_t searchesToday[NUM_SETS] = {0, 0, 0, 0};
static uint16_t searchDay = 0xFFFF;

int collectSetFor(int room) { return room == R_BEACH ? 0 : room == R_PARK ? 1 : room == R_MALL ? 2 : room == R_RINK ? 3 : -1; }
int collectCount() { return popcount32(G.collected); }

void collectSearch(int set) {
    if (set < 0 || set >= NUM_SETS) return;
    if (searchDay != G.day) { for (uint8_t& s : searchesToday) s = 0; searchDay = G.day; }
    if (searchesToday[set] >= 4) { toast("You've searched here enough today. Come back tomorrow!"); sfxNo(); return; }
    searchesToday[set]++;
    Sim& me = MC();
    me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 8);
    float luck = 0.55f + (G.weather == WX_SUNNY ? 0.1f : 0) + (set == 3 && G.weather == WX_SNOW ? 0.2f : 0) + (set == 1 && G.weather == WX_RAIN ? -0.25f : 0);
    if (frand() > luck) {
        static const char* miss[NUM_SETS] = {"Just sand... and a soggy flip-flop.", "It fluttered away!", "The gacha gave you a plain sticker.", "Only slush this time."};
        toast(miss[set]); return;
    }
    // rarer items have a lower chance
    static const int W[SET_SIZE] = {30, 25, 18, 13, 9, 5};
    int roll = irand(100), k = 0, acc = 0;
    for (; k < SET_SIZE; k++) { acc += W[k]; if (roll < acc) break; }
    if (k >= SET_SIZE) k = SET_SIZE - 1;
    uint32_t bit = 1u << (set * SET_SIZE + k);
    char b[128];
    if (G.collected & bit) { G.money += 10 + k * 8; snprintf(b, sizeof b, "Another %s - sold the duplicate for $%d.", COLLECT_NAMES[set][k], 10 + k * 8); toast(b); return; }
    G.collected |= bit;
    int have = popcount32((G.collected >> (set * SET_SIZE)) & 63u);
    snprintf(b, sizeof b, "Found: %s! (%s %d/6)", COLLECT_NAMES[set][k], SET_NAMES[set], have);
    toast(b); sfxYay();
    addMoodlet(me, "Lucky Find", EM_HAPPY, 1, 120);
    goalNotify(GE_COLLECT, set);
    if (have == SET_SIZE) {
        G.satisfaction += 500; G.money += 300;
        snprintf(b, sizeof b, "You completed the %s collection! (+500 points, +$300)", SET_NAMES[set]);
        dlgSay(-1, b);
        rewardPopup("COLLECTION COMPLETE!", std::string(SET_NAMES[set]) + "! +500 pts, +$300");
        addMemoryOnce(MEM_GOAL, G.mc, -1, std::string("Completed the ") + SET_NAMES[set] + " collection!");
        addMoodlet(me, "Collector's Pride", EM_CONFIDENT, 2, 720);
        fxBurst(200, 100, 30);
    }
}

// ================================================================ weather
const char* WEATHER_NAMES[WX_COUNT] = {"Sunny", "Cloudy", "Rainy", "Stormy", "Snowy", "Heatwave"};
void weatherRoll() {
    int s = seasonOf(G.day), r = irand(100);
    uint8_t w = (uint8_t)lxForecast(G.day); (void)s; (void)r;
    if (false)
    switch (s) {
    case 0: w = r < 45 ? WX_SUNNY : r < 70 ? WX_CLOUDY : r < 94 ? WX_RAIN : WX_STORM; break;            // spring
    case 1: w = r < 50 ? WX_SUNNY : r < 75 ? WX_HEAT : r < 87 ? WX_CLOUDY : WX_STORM; break;             // summer
    case 2: w = r < 30 ? WX_SUNNY : r < 65 ? WX_CLOUDY : r < 94 ? WX_RAIN : WX_STORM; break;            // fall
    default: w = r < 45 ? WX_SNOW : r < 75 ? WX_CLOUDY : WX_SUNNY; break;                               // winter
    }
    G.weather = w; G.weatherDay = G.day;
}
bool roomOutdoors(int room) { return room == R_BEACH || room == R_PARK || room == R_YARD || room == R_PLAYGROUND || room == R_STREET; }
void weatherArrive(int room) {
    if (!roomOutdoors(room)) return;
    Sim& me = MC();
    switch (G.weather) {
    case WX_RAIN: case WX_STORM: addMoodlet(me, "Caught in the Rain", EM_SAD, 1, 120); me.needs[N_HYGIENE] = std::fmax(0.f, me.needs[N_HYGIENE] - 10); break;
    case WX_SNOW: addMoodlet(me, "Snow Day Magic", EM_PLAYFUL, 1, 180); break;
    case WX_HEAT:
        if (room == R_BEACH) addMoodlet(me, "Beach Day Bliss", EM_HAPPY, 2, 240);
        else { addMoodlet(me, "So Sweaty", EM_TENSE, 1, 120); me.needs[N_HYGIENE] = std::fmax(0.f, me.needs[N_HYGIENE] - 8); }
        break;
    case WX_SUNNY: addMoodlet(me, "Sunshine", EM_HAPPY, 1, 120); break;
    default: break;
    }
}
static void rainStreaks(float x0, float y0, float w, float h, float t, int n, u32 c, float len) {
    for (int i = 0; i < n; i++) {
        float fx = std::fmod(i * 37.3f + t * 40.f, w), fy = std::fmod(i * 53.7f + t * (260.f + (i % 5) * 20), h);
        line(x0 + fx, y0 + fy, x0 + fx - len * 0.35f, y0 + fy + len, 1.2f, c);
    }
}
static void snowFlakes(float x0, float y0, float w, float h, float t, int n, float r) {
    for (int i = 0; i < n; i++) {
        float fx = std::fmod(i * 41.1f + std::sin(t * 1.3f + i) * 8 + 400, w), fy = std::fmod(i * 29.7f + t * (24.f + (i % 4) * 6), h);
        circle(x0 + fx, y0 + fy, r * (0.7f + (i % 3) * 0.3f), C(255, 255, 255, 220));
    }
}
static void driftBits(float x0, float y0, float w, float h, float t, int n, bool petals) {   // fall leaves / spring petals
    static const uint32_t LEAF[4] = {C(214, 102, 36), C(232, 160, 40), C(178, 58, 40), C(150, 96, 40)};
    for (int i = 0; i < n; i++) {
        const float sp = 14 + (i * 37 % 17), ph = i * 1.7f;
        const float fy = std::fmod(t * sp + i * 53.f, h - 4) + 2, fx = std::fmod(i * 71.f + std::sin(t * 0.9f + ph) * 18 + t * 9 + 400.f, w - 6) + 3;
        const float r = petals ? 1.6f : 2.4f;
        ellipse(x0 + fx, y0 + fy, r * 2 * (0.6f + 0.4f * std::fabs(std::sin(t * 2 + ph))), r * 1.3f, petals ? C(255, 190, 215, 230) : LEAF[i % 4]);
    }
}
void drawWeather(float t, bool outdoors) {
    if (outdoors && (G.weather == WX_SUNNY || G.weather == WX_CLOUDY)) { const int se = seasonOf(G.day); if (se == 2) driftBits(0, 0, TOP_W, TOP_H, t, 18, false); else if (se == 0) driftBits(0, 0, TOP_W, TOP_H, t, 14, true); }
    switch (G.weather) {
    case WX_CLOUDY:
        rect(0, 0, TOP_W, TOP_H, C(120, 130, 150, outdoors ? 40 : 18));
        if (outdoors) for (int i = 0; i < 4; i++) { float x = std::fmod(i * 120.f + t * 6, 480.f) - 40; ellipse(x, 22 + (i % 2) * 10, 90, 26, C(240, 240, 248, 200)); ellipse(x + 26, 14 + (i % 2) * 10, 56, 24, C(250, 250, 255, 210)); }
        break;
    case WX_RAIN: case WX_STORM: {
        bool storm = G.weather == WX_STORM;
        rect(0, 0, TOP_W, TOP_H, C(60, 70, 100, outdoors ? (storm ? 90 : 60) : 25));
        if (outdoors) rainStreaks(0, 0, TOP_W, TOP_H, t, storm ? 70 : 45, C(220, 230, 255, 150), storm ? 16 : 12);
        if (storm && std::fmod(t, 7.f) < 0.12f) rect(0, 0, TOP_W, TOP_H, C(255, 255, 255, outdoors ? 160 : 70));
        break;
    }
    case WX_SNOW:
        rect(0, 0, TOP_W, TOP_H, C(220, 230, 255, outdoors ? 40 : 15));
        if (outdoors) { snowFlakes(0, 0, TOP_W, TOP_H, t, 50, 2.f); rect(0, 226, TOP_W, 14, C(255, 255, 255, 200)); }
        break;
    case WX_HEAT:
        rect(0, 0, TOP_W, TOP_H, C(255, 170, 80, outdoors ? 40 : 18));
        if (outdoors) for (int i = 0; i < 6; i++) { float y = 150 + i * 14 + std::sin(t * 3 + i) * 2; line(0, y, TOP_W, y + std::sin(t * 2 + i) * 3, 1, C(255, 255, 255, 25)); }
        break;
    default: break;
    }
}
void drawWeatherInWindow(float x, float y, float w, float h, float t) {
    if (G.weather == WX_SUNNY || G.weather == WX_CLOUDY) { const int se = seasonOf(G.day); if (se == 2) driftBits(x, y, w, h, t, 4, false); else if (se == 0) driftBits(x, y, w, h, t, 3, true); }
    switch (G.weather) {
    case WX_CLOUDY: rect(x, y, w, h, C(150, 160, 180, 90)); ellipse(x + w * 0.35f, y + 12, 40, 14, C(245, 245, 250, 230)); break;
    case WX_RAIN: case WX_STORM:
        rect(x, y, w, h, C(70, 80, 110, 120));
        rainStreaks(x, y, w, h, t, 14, C(220, 230, 255, 170), 8);
        if (G.weather == WX_STORM && std::fmod(t, 7.f) < 0.12f) rect(x, y, w, h, C(255, 255, 255, 180));
        break;
    case WX_SNOW: rect(x, y, w, h, C(225, 235, 255, 90)); snowFlakes(x, y, w, h, t, 12, 1.4f); rect(x, y + h - 6, w, 6, C(255, 255, 255, 230)); break;
    case WX_HEAT: rect(x, y, w, h, C(255, 190, 110, 70)); break;
    default: break;
    }
}

// ================================================================ the Goals app
static int gTab = 0, rewardSel = -1, rewardPage = 0;
static Btn tabB(int i) { return Btn{4 + i * 79.f, 26, 76, 24}; }
static Btn wantB(int i) { return Btn{6, 56 + i * 46.f, 308, 42}; }
static Btn aspB(int i) { return Btn{6 + (i % 2) * 156.f, 56 + (i / 2) * 36.f, 152, 32}; }
static Btn rewB(int i) { return Btn{6 + (i % 2) * 156.f, 56 + (i / 2) * 36.f, 152, 32}; }
static const Btn bBuy{236, 208, 80, 28}, bMore{150, 208, 80, 28};

static void drawStar(float cx, float cy, float r, u32 c) { sparkle(cx, cy, r, c); circle(cx, cy, r * 0.3f, c); }

void goalsAppUpdate(const Input& in) {
    if (G.wantsDay != G.day) refreshWants();
    if (!in.touch) {
        if (in.down & KEY_L) gTab = (gTab + 3) % 4;
        if (in.down & KEY_R) gTab = (gTab + 1) % 4;
        return;
    }
    const touchPosition& tp = in.tp;
    for (int i = 0; i < 4; i++) if (tabB(i).in(tp)) { gTab = i; rewardSel = -1; sfxTap(); return; }
    if (gTab == 1) {
        for (int i = 0; i < NUM_ASPS; i++) if (aspB(i).in(tp)) {
            if (!aspAllowed(i)) { toast("Not available at your life stage yet."); sfxNo(); return; }
            aspChoose(i); sfxYay(); aspCheck(); return;
        }
    } else if (gTab == 2) {
        for (int i = 0; i < 6; i++) {
            int k = rewardPage * 6 + i;
            if (k < NUM_REWARDS && rewB(i).in(tp)) { rewardSel = k; sfxTap(); return; }
        }
        if (bMore.in(tp)) { rewardPage = (rewardPage + 1) % ((NUM_REWARDS + 5) / 6); rewardSel = -1; sfxTap(); return; }
        if (bBuy.in(tp) && rewardSel >= 0) buyReward(rewardSel);
    }
}

void goalsAppDrawTop(float t) {
    char b[160];
    grad(0, 0, TOP_W, TOP_H, C(255, 236, 246), C(214, 246, 250));
    rect(0, 0, TOP_W, 20, C(UI1));
    text("Goals & Rewards", 6, 2, 0.48f, C(WHITE));
    snprintf(b, sizeof b, "%u points", (unsigned)G.satisfaction);
    drawStar(300, 10, 6, C(GOLD)); text(b, 394, 2, 0.48f, C(GOLD), 2);
    const Sim& me = MC();
    switch (gTab) {
    case 0: {
        text("Your Wants", 200, 28, 0.6f, C(UI2), 1);
        auto lines = wrapText("Little wishes pop up all day. Make them happen to earn Satisfaction points, then spend them on perks in Rewards!", 0.42f, 360);
        for (size_t i = 0; i < lines.size(); i++) text(lines[i].c_str(), 200, 54 + i * 16.f, 0.42f, C(UI1), 1);
        snprintf(b, sizeof b, "Weather today: %s   (%s)", WEATHER_NAMES[G.weather % WX_COUNT], SEASON_NAMES[seasonOf(G.day)]);
        text(b, 200, 110, 0.45f, C(BERRY), 1);
        if (G.asp >= 0) {
            snprintf(b, sizeof b, "Life goal: %s  (level %d/4)", ASP_NAMES[G.asp], G.aspLevel[G.asp]);
            text(b, 200, 140, 0.48f, C(UI2), 1);
        } else text("Pick a Life Goal in the Goal tab!", 200, 140, 0.48f, C(HOT), 1);
        drawSim(me, 200, 236, 0.6f, t, 0, PO_WAVE, false);
        break;
    }
    case 1: {
        if (G.asp < 0) { text("Choose a life goal below!", 200, 40, 0.6f, C(HOT), 1); text("Complete each level for points, cash and bragging rights.", 200, 70, 0.42f, C(UI1), 1); break; }
        int a = G.asp, lv = G.aspLevel[a];
        text(ASP_NAMES[a], 200, 26, 0.7f, C(BERRY), 1);
        text(ASP_DESC[a], 200, 52, 0.42f, C(UI1), 1);
        for (int k = 0; k < 4; k++) {
            float x = 128 + k * 48.f;
            if (k < lv) { drawStar(x, 80, 12, C(GOLD)); }
            else drawStar(x, 80, 9, C(210, 210, 220));
        }
        if (lv >= 4) { text("COMPLETE! You're a legend.", 200, 104, 0.55f, C(HOT), 1); break; }
        snprintf(b, sizeof b, "Level %d goals:", lv + 1);
        text(b, 200, 102, 0.5f, C(UI2), 1);
        for (int k = 0; k < 2; k++) {
            const Milestone& ms = ASP_STEPS[a][lv][k];
            if (!ms.label[0]) continue;
            int cur = metric(ms.metric);
            float y = 128 + k * 40.f;
            rrect(40, y, 320, 34, 8, C(255, 255, 255, 220));
            text(ms.label, 52, y + 3, 0.45f, C(UI1));
            float f = clampv((float)cur / std::max(1, ms.target), 0.f, 1.f);
            rrect(52, y + 21, 240, 8, 4, C(220, 220, 228)); rrect(52, y + 21, 240 * f, 8, 4, C(f >= 1 ? RGB{110, 210, 130} : RGB{255, 140, 190}));
            snprintf(b, sizeof b, "%d / %d", std::min(cur, ms.target), ms.target);
            text(b, 352, y + 16, 0.4f, C(UI2), 2);
        }
        snprintf(b, sizeof b, "Reward: +%d points & $%d", ASP_PTS[lv], ASP_CASH[lv]);
        text(b, 200, 214, 0.45f, C(BERRY), 1);
        break;
    }
    case 2: {
        text("Rewards Shop", 200, 26, 0.65f, C(BERRY), 1);
        if (rewardSel >= 0) {
            const Reward& r = REWARDS[rewardSel];
            rrect(50, 60, 300, 120, 14, C(255, 255, 255, 230));
            text(r.name, 200, 70, 0.65f, C(HOT), 1);
            text(r.desc, 200, 104, 0.48f, C(UI1), 1);
            snprintf(b, sizeof b, "%d points%s", r.cost, r.perk ? "  (permanent perk)" : "  (use it now)");
            text(b, 200, 134, 0.45f, C(UI2), 1);
            if (r.perk && hasPerk(r.perk)) text("OWNED", 200, 154, 0.5f, C(GOLD), 1);
            else text("Tap Redeem to buy it!", 200, 154, 0.42f, C(BERRY), 1);
        } else {
            text("Tap a reward to see what it does.", 200, 70, 0.48f, C(UI1), 1);
            int owned = popcount32(G.perks);
            snprintf(b, sizeof b, "Perks owned: %d / 8", owned);
            text(b, 200, 100, 0.48f, C(UI2), 1);
        }
        break;
    }
    case 3: {
        text("Collections", 200, 24, 0.6f, C(BERRY), 1);
        for (int s = 0; s < NUM_SETS; s++) {
            float y = 46 + s * 48.f;
            rrect(8, y, 384, 44, 8, C(255, 255, 255, 210));
            int have = popcount32((G.collected >> (s * SET_SIZE)) & 63u);
            snprintf(b, sizeof b, "%s %d/6", SET_NAMES[s], have);
            text(b, 14, y + 2, 0.42f, C(have == 6 ? GOLD : UI2));
            for (int k = 0; k < SET_SIZE; k++) {
                bool got = G.collected & (1u << (s * SET_SIZE + k));
                float cx = 130 + k * 44.f, cy = y + 22;
                circle(cx, cy, 15, C(got ? SET_COLS[s] : RGB{225, 225, 232}));
                if (got) {
                    if (s == 0) { ellipse(cx, cy + 2, 18, 12, C(255, 250, 245)); for (int l = 0; l < 3; l++) line(cx - 6 + l * 6, cy + 7, cx, cy - 4, 1, C(SET_COLS[s])); }
                    else if (s == 1) { ellipse(cx - 6, cy - 2, 12, 14, C(shade(SET_COLS[s], 0.8f))); ellipse(cx + 6, cy - 2, 12, 14, C(shade(SET_COLS[s], 0.8f))); rect(cx - 1, cy - 8, 2, 16, C(UI1)); }
                    else if (s == 2) heart(cx, cy, 7, C(255, 255, 255));
                    else sparkle(cx, cy, 11, C(255, 255, 255));
                } else text("?", cx, cy - 8, 0.5f, C(170, 170, 180), 1);
            }
        }
        break;
    }
    }
    (void)t;
}

void goalsAppDrawBottom(float t) {
    char b[96];
    static const char* TN[4] = {"Wants", "Life Goal", "Rewards", "Collect"};
    for (int i = 0; i < 4; i++) button(tabB(i), TN[i], gTab == i, 1, 0.42f);
    switch (gTab) {
    case 0:
        for (int k = 0; k < 3; k++) {
            Btn r = wantB(k);
            rrect(r.x, r.y, r.w, r.h, 10, C(255, 255, 255, 235));
            int w = G.wantType[k];
            if (w >= NUM_WANTS) { text("A new want is coming soon...", 160, r.y + 12, 0.45f, C(170, 170, 180), 1); continue; }
            heart(r.x + 18, r.y + 21, 7, C(HOT));
            text(WANTS[w].text, r.x + 34, r.y + 6, 0.48f, C(UI1));
            snprintf(b, sizeof b, "+%d points", WANTS[w].pts);
            text(b, r.x + 34, r.y + 24, 0.38f, C(BERRY));
        }
        break;
    case 1:
        for (int i = 0; i < NUM_ASPS; i++) {
            bool ok = aspAllowed(i);
            char lb[48]; snprintf(lb, sizeof lb, "%s%s", ASP_NAMES[i], G.aspLevel[i] >= 4 ? " *" : "");
            button(aspB(i), lb, G.asp == i, ok ? 0 : 1, 0.4f);
        }
        break;
    case 2:
        for (int i = 0; i < 6; i++) {
            int k = rewardPage * 6 + i;
            if (k >= NUM_REWARDS) break;
            bool owned = REWARDS[k].perk && hasPerk(REWARDS[k].perk);
            char lb[48]; snprintf(lb, sizeof lb, owned ? "%s (owned)" : "%s  %d", REWARDS[k].name, REWARDS[k].cost);
            button(rewB(i), lb, rewardSel == k, owned ? 1 : 0, 0.38f);
        }
        button(bMore, "More", false, 1, 0.42f);
        button(bBuy, "Redeem", false, 2, 0.45f);
        break;
    case 3: {
        static const char* where[NUM_SETS] = {"Seashells: Sunset Beach", "Butterflies: Rosebud Park", "Glam Charms: Glam Mall gacha", "Ice Crystals: Ice Palace"};
        text("Where to look:", 160, 58, 0.5f, C(WHITE), 1);
        for (int s = 0; s < NUM_SETS; s++) text(where[s], 160, 82 + s * 22.f, 0.44f, C(CREAM), 1);
        text("Rare finds turn up less often. Sunny days help;", 160, 178, 0.38f, C(UIL), 1);
        text("butterflies hide in the rain, crystals love snow.", 160, 192, 0.38f, C(UIL), 1);
        break;
    }
    }
    (void)t;
}

// the little weather icon for HUDs
void drawWeatherIcon(float x, float y) {
    switch (G.weather) {
    case WX_SUNNY: circle(x, y, 5, C(255, 214, 80)); for (int i = 0; i < 8; i++) { float a = i * 0.785f; line(x + std::cos(a) * 7, y + std::sin(a) * 7, x + std::cos(a) * 9, y + std::sin(a) * 9, 1.2f, C(255, 214, 80)); } break;
    case WX_CLOUDY: ellipse(x, y + 1, 16, 9, C(235, 238, 245)); ellipse(x + 3, y - 2, 10, 8, C(245, 247, 252)); break;
    case WX_RAIN: ellipse(x, y - 2, 16, 9, C(200, 210, 225)); for (int i = 0; i < 3; i++) line(x - 4 + i * 4, y + 3, x - 5 + i * 4, y + 7, 1.2f, C(140, 190, 255)); break;
    case WX_STORM: ellipse(x, y - 2, 16, 9, C(150, 155, 175)); tri(x, y + 1, x - 3, y + 7, x + 1, y + 5, C(255, 220, 80)); break;
    case WX_SNOW: for (int i = 0; i < 3; i++) { float a = i * 1.047f; line(x - std::cos(a) * 6, y - std::sin(a) * 6, x + std::cos(a) * 6, y + std::sin(a) * 6, 1.2f, C(255, 255, 255)); } break;
    case WX_HEAT: circle(x, y, 6, C(255, 140, 60)); circle(x, y, 3.5f, C(255, 214, 80)); break;
    }
}
