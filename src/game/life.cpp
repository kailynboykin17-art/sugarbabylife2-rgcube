#include "traits2.hpp"
// The life engine: Starlight Bay keeps living without you.
//  - every townie has a hobby, a dream (goal with progress), a favourite hangout and a job
//  - a daily routine (school, work shifts, lunch, hobby hours, date nights, club nights,
//    Sunday chapel, rain moves everyone indoors) decides who you'll meet where
//  - townies who are in the same place hour by hour actually interact: friendships form,
//    feuds start, couples get together, cheat, break up, get engaged, marry, have babies
//  - they remember what happened (to each other and with you) and talk about it
//  - one town event a day (sales, open mic, bonfires, lost pets, public feuds, weddings,
//    celebrity sightings, new neighbours...) pulls people in, with choices when you show up
//  - family storylines at home (sibling squabbles, sneaking out, anniversaries, report cards)
// Everything feeds STATUS, texts, chatter and the people you see walking around.
#include "worldsim.hpp"
#include "lifestory.hpp"
#include "life.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "goals.hpp"
#include "status.hpp"
#include "squads.hpp"
#include "chat.hpp"
#include "extras.hpp"
#include "drama.hpp"
#include "seasons.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {
uint32_t hsh(uint32_t a) { a ^= a >> 16; a *= 0x7feb352du; a ^= a >> 15; a *= 0x846ca68bu; return a ^ (a >> 16); }
bool valid(int s) { return s >= 0 && s < (int)G.sims.size(); }
std::string nm(int s) { return valid(s) ? G.sims[s].name : std::string("someone"); }
bool townie(int s) { return valid(s) && !G.sims[s].household && s != G.mc && !simDead(s) && !simGone(s); }
bool isCastSim(int s) { return valid(s) && G.sims[s].bio.rfind("[Cast]", 0) == 0; }
NpcLife& L(int s) { if (G.life.size() < G.sims.size()) G.life.resize(G.sims.size()); return G.life[s]; }
int hourNow() { return (int)(G.minute / 60); }
bool badWeather() { return G.weather == WX_RAIN || G.weather == WX_STORM || G.weather == WX_SNOW; }
int frOf(int a, int b) { const Rel* r = findRel(a, b); return r ? r->fr : 0; }
int roOf(int a, int b) { const Rel* r = findRel(a, b); return r ? r->ro : 0; }
bool knowsMC(int s, int minFr) { const Rel* r = findRel(G.mc, s); return r && r->met && r->fr >= minFr; }
void insta(int sim, const std::string& text) {
    Post p; p.sim = (int16_t)sim; p.text = text; p.likes = (uint32_t)(30 + irand(400));
    G.posts.insert(G.posts.begin(), p);
    if (G.posts.size() > 30) G.posts.pop_back();
}

// ------------------------------------------------------------------ hobbies, dreams, hangouts, jobs
struct HobbyDef { const char* name; const char* doing; int room; int8_t skill; uint8_t trait; const char* brag[3]; };
const HobbyDef HOB[] = {
    {"painting", "painting", R_PARK, SK_CREATIVITY, TR_CREATIVE, {"Just finished a new painting!", "I got paint in my hair again.", "The light here is perfect for painting."}},
    {"skating", "skating", R_RINK, SK_FITNESS, TR_ACTIVE, {"I finally landed my spin!", "My knees are SO bruised lol.", "One more lap!"}},
    {"baking", "baking", R_BAKERY, SK_COOKING, TR_FOODIE, {"I made croissants from scratch!", "Don't ask about the burnt cookies.", "Butter is a love language."}},
    {"gaming", "gaming", R_ARCADE, SK_LOGIC, TR_GAMER, {"New high score. Bow down.", "One more round. Then I'll sleep.", "Who keeps beating my score?!"}},
    {"reading", "reading", R_LIBRARY, SK_LOGIC, TR_BOOKWORM, {"This book ruined me. In a good way.", "Three chapters from the end!", "Shh, it's the plot twist."}},
    {"thrifting", "thrifting", R_MALL, SK_STYLE, TR_FASHIONISTA, {"Found vintage boots for five bucks!", "My closet is full. Still shopping.", "This jacket is calling my name."}},
    {"surfing", "surfing", R_BEACH, SK_FITNESS, TR_ACTIVE, {"Caught the best wave this morning!", "Salt water hair, don't care.", "The swell is HUGE today."}},
    {"singing", "practicing my songs", R_CAFE, SK_CHARISMA, TR_MUSIC, {"I wrote a new song!", "My voice is warmed up. Watch out.", "La la laaa... sorry. Habit."}},
    {"pilates", "doing pilates", R_GYM, SK_FITNESS, TR_ACTIVE, {"Core of STEEL.", "Pilates is my therapy.", "I can't feel my legs. Worth it."}},
    {"photography", "taking photos", R_STREET, SK_CREATIVITY, TR_CREATIVE, {"The light right now is unreal.", "Hold still, you look cute!", "Got the shot!"}},
    {"movies", "watching movies", R_CINEMA, -1, TR_LAZY, {"I've seen this one four times.", "No spoilers. I mean it.", "Popcorn is a food group."}},
    {"sewing", "sewing", R_BOUTIQUE, SK_STYLE, TR_FASHIONISTA, {"I made this top myself!", "Pricked my finger AGAIN.", "Hand-stitched. Every bit."}},
    {"dancing", "dancing", R_CLUB, SK_FITNESS, TR_OUTGOING, {"I'm learning a new routine!", "Dance battle me. I dare you.", "This song is MY song."}},
    {"pet rescue", "volunteering with rescue pets", R_PETSHOP, -1, TR_PETLOVER, {"We found a home for a puppy today!", "I want to adopt ALL of them.", "Kitten cuddles = best job ever."}},
    {"journaling", "journaling", R_CAFE, SK_CREATIVITY, TR_LONER, {"Writing it down helps.", "Don't read over my shoulder!", "Dear diary... it's been a week."}},
    {"skincare", "doing skincare", R_SPA, SK_STYLE, TR_GLAMOROUS, {"Ten-step routine. Glowing.", "Face mask selfie incoming.", "Hydrate or die-drate."}},
    {"playing tag", "playing tag", R_PLAYGROUND, SK_FITNESS, TR_ACTIVE, {"I'm the fastest!", "Tag, you're it!", "Swings are the BEST."}},
};
constexpr int NUM_HOB = sizeof(HOB) / sizeof(HOB[0]);
constexpr int HOB_KID = NUM_HOB - 1;

enum GoalKind : uint8_t { GK_PRACTICE, GK_LOVE, GK_BFF, GK_VIRAL, GK_SCHOOL, GK_WORK };
struct GoalDef { const char* name; uint8_t kind; };
// the first NUM_HOB dreams line up with the hobbies
const GoalDef GOALS[] = {
    {"paint a mural downtown", GK_PRACTICE}, {"win the Skate Cup", GK_PRACTICE}, {"open a little bakery", GK_PRACTICE}, {"go pro at gaming", GK_PRACTICE},
    {"read 100 books", GK_PRACTICE}, {"open a vintage shop", GK_PRACTICE}, {"surf the big swell", GK_PRACTICE}, {"headline open mic night", GK_PRACTICE},
    {"run a marathon", GK_PRACTICE}, {"get a photo in a gallery", GK_PRACTICE}, {"write a movie script", GK_PRACTICE}, {"design my own collection", GK_PRACTICE},
    {"win a dance battle", GK_PRACTICE}, {"adopt a rescue puppy", GK_PRACTICE}, {"finish my novel", GK_PRACTICE}, {"launch a skincare line", GK_PRACTICE},
    {"be the fastest kid at recess", GK_PRACTICE},
    {"find true love", GK_LOVE}, {"find a real best friend", GK_BFF}, {"go viral on STATUS", GK_VIRAL}, {"make the honor roll", GK_SCHOOL}, {"get promoted", GK_WORK},
};
constexpr int G_LOVE = NUM_HOB, G_BFF = NUM_HOB + 1, G_VIRAL = NUM_HOB + 2, G_SCHOOL = NUM_HOB + 3, G_WORK = NUM_HOB + 4;

int hobbyOf(int s) {
    if (!valid(s)) return 0;
    const Sim& o = G.sims[s];
    uint32_t k = hsh((uint32_t)s * 977u + 13u);
    if (o.stage <= ST_PRETEEN && k % 3 == 0) return HOB_KID;
    int c[NUM_HOB]; int n = 0;
    for (int h = 0; h < HOB_KID; h++) if (o.hasTrait(HOB[h].trait) && !(HOB[h].room == R_CLUB && !o.isAdult())) c[n++] = h;
    if (n && (k >> 4) % 4) return c[(k >> 8) % n];
    int h = (int)((k >> 12) % HOB_KID);
    if (HOB[h].room == R_CLUB && !o.isAdult()) h = 7;
    return h;
}
int hobbyRoom(int s) { return HOB[hobbyOf(s)].room; }

struct Job { int room, start, hours; const char* title; };
const Job JOBS[] = {
    {R_CAFE, 7, 8, "barista"}, {R_BOUTIQUE, 10, 8, "sales associate"}, {R_RESTAURANT, 15, 8, "server"}, {R_DINER, 6, 8, "line cook"},
    {R_HOSPITAL, 8, 9, "nurse"}, {R_LIBRARY, 9, 8, "librarian"}, {R_GYM, 6, 8, "personal trainer"}, {R_HOTEL, 9, 8, "concierge"},
    {R_BAKERY, 5, 8, "baker"}, {R_MALL, 10, 8, "store manager"}, {R_SPA, 10, 8, "esthetician"}, {R_PETSHOP, 9, 8, "pet groomer"},
    {R_SUSHI, 12, 8, "sushi chef"}, {R_CINEMA, 14, 8, "usher"},
};
constexpr int NUM_JOBS = sizeof(JOBS) / sizeof(JOBS[0]);
Job jobOf(int s) {
    if (!valid(s)) return {-1, 0, 0, ""};
    const Sim& o = G.sims[s];
    if (o.stage < ST_YA || o.stage > ST_MIDDLE) return {-1, 0, 0, ""};
    if (s == G.cop) return {R_STREET, 8, 10, "police officer"};
    if (o.career != CR_NONE && o.career < CR_COUNT) { const CareerDef& c = CAREERS[o.career]; return {c.room, c.startHour, c.hours, c.titles[o.careerLevel % 5]}; }
    uint32_t k = hsh((uint32_t)s * 389u + 7u);
    if (k % 6 == 0) return {-1, 0, 0, "freelancer"};
    if (o.stage == ST_YA && k % 7 == 1) return {R_COLLEGE, 9, 5, "college student"};
    return JOBS[(k >> 4) % NUM_JOBS];
}

int favOf(int s) {
    static const int KID[] = {R_PARK, R_ARCADE, R_PLAYGROUND, R_BAKERY, R_MALL, R_BEACH, R_RINK, R_CINEMA, R_PETSHOP, R_LIBRARY};
    static const int ALL[] = {R_CAFE, R_MALL, R_PARK, R_BEACH, R_BOUTIQUE, R_ARCADE, R_DINER, R_BAKERY, R_SUSHI, R_STREET, R_LIBRARY, R_CINEMA, R_RINK, R_SPA, R_HOTEL, R_GYM};
    if (!valid(s)) return R_PARK;
    const Sim& o = G.sims[s];
    uint32_t k = hsh((uint32_t)s * 151u + 99u);
    if (o.stage <= ST_PRETEEN) return KID[k % (sizeof KID / sizeof KID[0])];
    if (k % 2) {   // their traits pick
        if (o.hasTrait(TR_FASHIONISTA)) return (k >> 3) % 2 ? R_BOUTIQUE : R_MALL;
        if (o.hasTrait(TR_FOODIE)) return (k >> 3) % 2 ? R_SUSHI : R_DINER;
        if (o.hasTrait(TR_BOOKWORM)) return (k >> 3) % 2 ? R_LIBRARY : R_CAFE;
        if (o.hasTrait(TR_ACTIVE)) return (k >> 3) % 2 ? R_BEACH : R_PARK;
        if (o.hasTrait(TR_GLAMOROUS)) return (k >> 3) % 2 ? R_SPA : R_HOTEL;
        if (o.hasTrait(TR_GOOFBALL)) return R_ARCADE;
    }
    return ALL[(k >> 6) % (sizeof ALL / sizeof ALL[0])];
}

int goalIndex(int s) {
    if (!valid(s)) return 0;
    const Sim& o = G.sims[s];
    NpcLife& l = L(s);
    if (l.goalsDone % 2 == 0) return hobbyOf(s);
    int opts[4]; int n = 0;
    opts[n++] = G_BFF;
    if (o.stage >= ST_TEEN) opts[n++] = G_VIRAL;
    if (o.stage >= ST_CHILD && o.stage <= ST_TEEN) opts[n++] = G_SCHOOL;
    if (o.isAdult() && partnerOf(s) < 0) opts[n++] = G_LOVE;
    else if (o.isAdult() && jobOf(s).room >= 0) opts[n++] = G_WORK;
    return opts[hsh((uint32_t)s * 31u + l.goalsDone * 7u) % n];
}

// ------------------------------------------------------------------ memories
void remember(int s, int kind, int other) {
    if (!valid(s)) return;
    NpcLife& l = L(s);
    for (int i = 3; i > 0; i--) l.mem[i] = l.mem[i - 1];
    l.mem[0].kind = (uint8_t)kind; l.mem[0].other = (int16_t)other; l.mem[0].day = G.day;
}
const LifeMemory* recentMem(int s, int kind, int maxDays, int other = -9) {
    if (!valid(s) || s >= (int)G.life.size()) return nullptr;
    const NpcLife& l = G.life[s];
    for (const LifeMemory& m : l.mem)
        if (m.other != -1 && m.kind == kind && G.day - m.day <= maxDays && (other == -9 || m.other == other)) return &m;
    return nullptr;
}

// ------------------------------------------------------------------ town events
enum WorldEv : uint8_t { WE_NONE, WE_SALE, WE_OPENMIC, WE_BONFIRE, WE_LOSTPET, WE_FEUD, WE_CELEB, WE_MARKET, WE_TOURNEY, WE_SAMPLES, WE_WEDDING,
                         WE_NEWBIE, WE_FITNESS, WE_PREMIERE, WE_FESTIVAL, WE_COUNT };
struct WEDef { const char* fmt; int room; int8_t h0, h1; uint8_t pull; int8_t hobby; bool outdoor; };
const WEDef WED[WE_COUNT] = {
    {"", -1, 0, 0, 0, -1, false},
    {"Flash sale at the Mall! Half the town is shopping.", R_MALL, 10, 20, 30, 5, false},
    {"Open mic night at the Cafe! Singers, poets, chaos.", R_CAFE, 18, 22, 18, 7, false},
    {"Beach bonfire tonight! S'mores and gossip.", R_BEACH, 19, 23, 25, 6, true},
    {"{A} lost their pet! Everyone's searching the Park.", R_PARK, 9, 18, 12, 13, true},
    {"{A} vs {B}: their feud is going public at {P}...", -1, 13, 18, 15, -1, false},
    {"Celebrity sighting! {A} was spotted at the Hotel.", R_HOTEL, 11, 17, 25, -1, false},
    {"Farmers market in the Park this morning!", R_PARK, 8, 13, 25, 2, true},
    {"Arcade tournament! Winner gets bragging rights.", R_ARCADE, 14, 20, 10, 3, false},
    {"Free samples at the Bakery all morning!", R_BAKERY, 8, 13, 25, 2, false},
    {"Wedding bells! {A} & {B} marry at the Chapel today.", R_CHAPEL, 14, 18, 8, -1, false},
    {"New in town: {A} just moved to Starlight Bay! Spotted at {P}.", -1, 10, 18, 0, -1, false},
    {"Fitness challenge at the Gym! Who's the strongest?", R_GYM, 8, 12, 15, 8, false},
    {"Midnight movie premiere at the Cinema!", R_CINEMA, 20, 23, 25, 10, false},
    {"", -1, 0, 0, 55, -1, false},   // a seasonal festival (seasons.cpp): most of the town goes
};
bool evToday() { return G.lifeEv > WE_NONE && G.lifeEv < WE_COUNT && G.lifeEvDay == G.day; }
bool evOnAt(int hour) {
    if (!evToday()) return false;
    if (G.lifeEv == WE_FESTIVAL) { int f = festToday(); return f >= 0 && hour >= festStart(f) && hour < festEnd(f); }
    return hour >= WED[G.lifeEv].h0 && hour < WED[G.lifeEv].h1;
}
std::string evText() {
    if (!evToday()) return "";
    if (G.lifeEv == WE_FESTIVAL) return std::string(festName(festToday())) + " at " + ROOM_NAMES[G.lifeEvRoom % R_COUNT] + " today!";
    std::string o = WED[G.lifeEv].fmt;
    auto rep = [&](const char* k, const std::string& v) { size_t p; while ((p = o.find(k)) != std::string::npos) o.replace(p, std::strlen(k), v); };
    rep("{A}", nm(G.lifeEvA)); rep("{B}", nm(G.lifeEvB));
    rep("{P}", G.lifeEvRoom < R_COUNT ? std::string(ROOM_NAMES[G.lifeEvRoom]) : std::string("town"));
    return o;
}
int eventPull(int s, int hour) {
    if (!evOnAt(hour)) return -1;
    const WEDef& d = WED[G.lifeEv];
    int room = G.lifeEvRoom;
    if (room >= R_COUNT) return -1;
    if (s == G.lifeEvA || s == G.lifeEvB) return room;
    if (G.lifeEv == WE_WEDDING) {
        if ((valid(G.lifeEvA) && frOf(s, G.lifeEvA) >= 20) || (valid(G.lifeEvB) && frOf(s, G.lifeEvB) >= 20) || isFamily(s, G.lifeEvA) || isFamily(s, G.lifeEvB)) return room;
    }
    if (d.hobby >= 0 && hobbyOf(s) == d.hobby) return room;
    if (hsh((uint32_t)s * 53u + G.day * 11u) % 100 < d.pull) return room;
    return -1;
}

// ------------------------------------------------------------------ the daily routine
int indoorAlt(int s) { static const int IN[5] = {R_CAFE, R_MALL, R_LIBRARY, R_CINEMA, R_ARCADE}; return IN[hsh((uint32_t)s + G.day) % 5]; }
int routine(int s, int hour) {
    const Sim& o = G.sims[s];
    uint32_t k = hsh((uint32_t)s * 131u + G.day * 7919u);
    bool wk = isWeekday(G.day);
    int wd = weekday(G.day);
    int hob = hobbyRoom(s), fav = favOf(s);
    if (s == G.cop) return (hour >= 8 && hour < 20) ? ((hour / 3) % 2 ? R_STREET : R_PARK) : -1;
    bool nightOwl = o.hasTrait(TR_OUTGOING) || o.hasTrait(TR_GLAMOROUS);
    if (hour < 7 || hour >= 23) {
        if (o.isAdult() && o.stage <= ST_MIDDLE && nightOwl && (wd == 4 || wd == 5) && (hour >= 23 || hour < 2) && k % 3 == 0) return R_CLUB;
        return -1;   // asleep at home
    }
    if (o.stage <= ST_TEEN) {
        if (wk && hour >= 8 && hour < 15) return R_SCHOOL;
        if (hour == 7) return k % 4 == 0 ? R_BAKERY : -1;
        if (hour >= 15 && hour < 18) return (k >> 3) % 3 ? hob : fav;
        if (hour >= 18 && hour < 20) return -1;   // family dinner
        if (hour >= 20 && hour < 22 && o.stage == ST_TEEN && wd >= 4) return (k >> 5) % 2 ? fav : R_CINEMA;
        if (!wk && hour >= 9 && hour < 18) {
            uint32_t q = hsh(k + (uint32_t)(hour - 9) / 3);
            return q % 3 == 0 ? hob : q % 3 == 1 ? fav : ((q >> 4) % 2 ? R_MALL : R_PARK);
        }
        return -1;
    }
    if (o.stage == ST_ELDER) {
        if (wd == 6 && hour >= 9 && hour < 11 && k % 2) return R_CHAPEL;
        if (hour < 10) return R_PARK;
        if (hour < 12) return R_CAFE;
        if (hour == 12) return R_DINER;
        if (hour >= 14 && hour < 17) return k % 2 ? hob : fav;
        return -1;
    }
    // grown-ups
    Job j = jobOf(s);
    if (wk && j.room >= 0 && hour >= j.start && hour < j.start + j.hours) return j.room;
    if (wk && j.room < 0 && j.title[0] == 'f' && hour >= 10 && hour < 13 && k % 2) return R_CAFE;   // freelancers work from the cafe
    if (wd == 6 && hour >= 9 && hour < 11 && (o.hasTrait(TR_FAMILY) || k % 5 == 0)) return R_CHAPEL;
    if (hour <= 8) return o.hasTrait(TR_ACTIVE) ? R_GYM : (k % 3 == 0 ? R_CAFE : -1);
    if (hour == 12 || hour == 13) { static const int LUNCH[4] = {R_CAFE, R_DINER, R_SUSHI, R_BAKERY}; return (k >> 2) % 2 ? LUNCH[(k >> 4) % 4] : fav; }
    int part = partnerOf(s);
    if (hour >= 19 && hour < 22) {
        if (part >= 0 && part != G.mc && (wd >= 4 || (k >> 9) % 3 == 0)) {   // date night: both partners land in the same place
            static const int DATE[4] = {R_RESTAURANT, R_CINEMA, R_SUSHI, R_BEACH};
            return DATE[hsh((uint32_t)std::min(s, part) * 31u + G.day) % 4];
        }
        if (o.hasTrait(TR_OUTGOING) && hour >= 21 && o.stage <= ST_MIDDLE) return R_CLUB;
        return (k >> 11) % 3 == 0 ? fav : -1;
    }
    if (hour >= 17 && hour < 19) return (k >> 6) % 3 ? hob : fav;
    if (hour == 22) return (nightOwl && o.stage <= ST_MIDDLE && wd >= 4) ? R_CLUB : -1;
    uint32_t q = hsh(k + (uint32_t)hour / 3 * 17u);
    switch (q % 5) {
    case 0: return hob;
    case 1: return fav;
    case 2: return R_MALL;
    case 3: return (G.weather == WX_SUNNY || G.weather == WX_HEAT) ? R_BEACH : R_CAFE;
    default: return R_PARK;
    }
}

// ------------------------------------------------------------------ how two townies get on
int compat(int a, int b) {
    const Sim &A = G.sims[a], &B = G.sims[b];
    int c = 0;
    for (int i = 0; i < 3; i++) if (A.traits[i] != TR_NONE && B.hasTrait(A.traits[i])) c += 2;
    if (hobbyOf(a) == hobbyOf(b)) c += 2;
    int ca = cliqueOf(a), cb = cliqueOf(b);
    if (ca >= 0 && ca == cb) c += 1;
    if (A.hasTrait(TR_HOTHEAD) || B.hasTrait(TR_HOTHEAD)) c -= 1;
    if (A.hasTrait(TR_CHEERFUL) || B.hasTrait(TR_CHEERFUL)) c += 1;
    if (A.hasTrait(TR_KIND) || B.hasTrait(TR_KIND)) c += 1;
    if (A.hasTrait(TR_MEAN) || B.hasTrait(TR_MEAN)) c -= 2;
    if ((A.hasTrait(TR_SNOB) && !B.hasTrait(TR_GLAMOROUS)) || (B.hasTrait(TR_SNOB) && !A.hasTrait(TR_GLAMOROUS))) c -= 1;
    if (A.hasTrait(TR_LONER) || B.hasTrait(TR_LONER)) c -= 1;
    if ((A.hasTrait(TR_NEAT) && B.hasTrait(TR_LAZY)) || (B.hasTrait(TR_NEAT) && A.hasTrait(TR_LAZY))) c -= 2;
    if ((A.hasTrait(TR_GLAMOROUS) && B.hasTrait(TR_FASHIONISTA)) || (B.hasTrait(TR_GLAMOROUS) && A.hasTrait(TR_FASHIONISTA))) c += 1;
    return c;
}
bool sameCrowd(int a, int b) {
    const Sim &A = G.sims[a], &B = G.sims[b];
    auto grp = [](const Sim& s) { return s.stage <= ST_PRETEEN ? 0 : s.stage == ST_TEEN ? 1 : s.stage == ST_ELDER ? 3 : 2; };
    return grp(A) == grp(B);
}
Actor* actorHere(int sim) { for (Actor& a : curRoom().actors) if (a.kind == 0 && a.id == sim) return &a; return nullptr; }
void sayHere(int sim, const std::string& line) {
    Actor* a = actorHere(sim);
    if (a) { a->say = line; a->sayT = 3.f; }
}
void newsToFriends(int a, int b, int kind) {   // the people they're close to text you about it
    if (knowsMC(a, 20) && irand(2)) chatNews(a, kind, b);
    else if (valid(b) && knowsMC(b, 20) && irand(2)) chatNews(b, kind, a);
}

void startDating(int a, int b, const char* how) {
    Rel& r = rel(a, b);
    r.status = RS_DATING; r.ro = (int8_t)std::max<int>(r.ro, 50);
    L(a).sinceDay = G.day; L(b).sinceDay = G.day;
    remember(a, LM_ROMANCE, b); remember(b, LM_ROMANCE, a);
    static const char* const SOFT[4] = {"date night with my favorite person <3", "not me smiling at my phone because of {B}", "{B} >>> everyone else", "ok we're official. be nice"};
    std::string t = SOFT[irand(4)];
    size_t p = t.find("{B}"); if (p != std::string::npos) t.replace(p, 3, nm(b));
    statusPostKind(a, SP_COUPLE, t, b);
    insta(a, nm(a) + " & " + nm(b) + " " + how);
    newsToFriends(a, b, LM_ROMANCE);
    sayHere(a, "Did you hear?! " + nm(b) + " and I are official!");
}
void breakUp(int a, int b, bool cheated) {
    Rel& r = rel(a, b);
    r.status = RS_NONE; r.ro = 0; r.fr = (int8_t)clampv(r.fr - 25, -100, 100);
    remember(a, LM_BREAKUP, b); remember(b, LM_BREAKUP, a);
    if (cheated) remember(b, LM_HURT, a);
    static const char* const SAD[4] = {"some chapters just end i guess", "single. don't ask.", "guess loyalty is dead lol", "deleting all our pics rn"};
    statusPostKind(b, SP_BREAKUP, SAD[cheated ? 2 : irand(4)], a);
    if (G.sims[a].hasTrait(TR_HOTHEAD) || irand(3) == 0) statusPostKind(a, SP_SUBTWEET, "some people show their true colors eventually", b);
    insta(b, std::string("omg ") + nm(a) + " and " + nm(b) + " BROKE UP?! not me crying");
    newsToFriends(b, a, LM_BREAKUP);
}

// ------------------------------------------------------------------ two townies in the same place
void interact(int a, int b, int room) {
    const Sim &A = G.sims[a], &B = G.sims[b];
    int c = compat(a, b);
    int frBefore = frOf(a, b);
    bool crowd = sameCrowd(a, b);
    bool fight = (c <= -1 && irand(3) == 0) || (A.hasTrait(TR_HOTHEAD) && B.hasTrait(TR_HOTHEAD) && irand(3) == 0) || (frBefore <= -30 && irand(2));
    int dfr = fight ? -(6 + irand(6)) : c + irand(4) - (crowd ? 0 : 1);
    int dro = 0;
    Rel& r0 = rel(a, b);
    bool romantic = crowd && canRomance(a, b) && r0.status != RS_FAMILY;
    if (romantic && !fight) {
        dro = c > 0 ? 1 + irand(4) : irand(2);
        if (A.hasTrait(TR_ROMANTIC) || B.hasTrait(TR_ROMANTIC)) dro += 2;
        int pa = partnerOf(a), pb = partnerOf(b);
        bool taken = (pa >= 0 && pa != b) || (pb >= 0 && pb != a);
        if (taken) dro = (A.hasTrait(TR_FAMILY) || B.hasTrait(TR_FAMILY)) ? 0 : dro / 2;
    }
    if (r0.status >= RS_DATING && r0.status <= RS_MARRIED && !fight) { dro += 2; dfr += 1; }
    changeRel(a, b, dfr, 0);
    if (dro) changeRel(a, b, 0, dro);
    int frNow = frOf(a, b);
    uint8_t stNow = rel(a, b).status;
    // on screen, if you're there
    if (room == G.room) {
        if (fight) { sayHere(a, irand(2) ? "Excuse me?! Say that again." : "Don't even start with me, " + nm(b) + "."); sayHere(b, "Whatever. You're SO dramatic."); }
        else if (dro >= 3) { sayHere(a, irand(2) ? "You look really nice today..." : "Hehe, stop it."); sayHere(b, "*blushes*"); }
        else if (dfr >= 3) sayHere(a, irand(2) ? "Okay you're actually hilarious." : "We should hang out more!");
    }
    if (fight) { remember(a, LM_FIGHT, b); remember(b, LM_FIGHT, a); }
    // friendship milestones
    if (frBefore < 60 && frNow >= 60 && stNow != RS_FAMILY) {
        remember(a, LM_FRIEND, b); remember(b, LM_FRIEND, a);
        statusPostKind(a, SP_HYPE, "my bestie " + nm(b) + " >>> literally everyone", b);
        if (goalIndex(a) == G_BFF) lifeBoostGoal(a, 100);
        if (goalIndex(b) == G_BFF) lifeBoostGoal(b, 100);
    }
    if (frBefore > -40 && frNow <= -40) {
        statusPostKind(a, SP_SUBTWEET, irand(2) ? "fake people are SO exhausting" : "not naming names but you know who you are", b);
        insta(a, nm(a) + " just unfollowed " + nm(b) + "... the DRAMA");
    }
    // romance
    if (romantic && rel(a, b).status == RS_NONE && roOf(a, b) >= 45 && frOf(a, b) >= 20) {
        int pa = partnerOf(a), pb = partnerOf(b);
        if (pa < 0 && pb < 0) startDating(a, b, "are officially a thing!! <3");
        else if (irand(3) == 0) {   // someone's cheating...
            int cheater = pa >= 0 ? a : b, ex = pa >= 0 ? pa : pb, other = cheater == a ? b : a;
            statusPostKind(ST_ACC_TEA, SP_RUMOR, "spotted: " + nm(cheater) + " getting VERY cozy with " + nm(other) + " at " + ROOM_NAMES[room % R_COUNT] + "... doesn't " + nm(cheater) + " have a partner??", cheater, other);
            if (ex == G.mc) {
                toast("STATUS: people say " + nm(cheater) + " was flirting with " + nm(other) + "...");
                changeRel(G.mc, cheater, -4, -8);
                remember(cheater, LM_ROMANCE, other);
            } else if (valid(ex) && !G.sims[ex].household && rel(cheater, ex).status == RS_DATING) {
                breakUp(cheater, ex, true);
                startDating(cheater, other, "are dating?! after THAT??");
            }
        }
    }
    Rel& r2 = rel(a, b);
    if (r2.status == RS_DATING && (r2.fr < 0 || r2.ro < 20)) breakUp(a, b, false);
    else if (r2.status == RS_DATING && r2.ro >= 75 && r2.fr >= 45 && G.day >= L(a).sinceDay + 3 && irand(5) == 0) {
        r2.status = RS_ENGAGED;   // (no more uses of r2 below: posting can add rels)
        L(a).sinceDay = G.day; L(b).sinceDay = G.day;
        statusAnnounce(nm(a) + " PROPOSED to " + nm(b) + " at " + ROOM_NAMES[room % R_COUNT] + "!!! and the answer was YES", a);
        newsToFriends(b, a, LM_ENGAGED);
        if (knowsMC(a, 25) || knowsMC(b, 25)) {
            Event e; e.day = (uint16_t)(G.day + 2); e.hour = 14; e.type = EV_PLAN; e.loc = R_CHAPEL;
            e.text = "Wedding: " + nm(a) + " & " + nm(b) + " (Chapel)";
            G.events.push_back(e);
            toast("You're invited to " + nm(a) + " & " + nm(b) + "'s wedding! (Planner)");
        }
    }
}

// ------------------------------------------------------------------ dreams
void goalStep(int s, int amt) {
    if (!valid(s) || amt <= 0) return;
    NpcLife& l = L(s);
    int before = l.goalProg;
    int gi = goalIndex(s);
    l.goalProg = (uint8_t)std::min(100, before + amt);
    if (before < 50 && l.goalProg >= 50 && !G.sims[s].household && G.sims[s].stage >= ST_TEEN && irand(2))
        statusPostKind(s, SP_MOOD, std::string("halfway to my dream: ") + GOALS[gi].name + "!! no stopping now", -1);
    if (l.goalProg >= 100) {
        l.goalProg = 0;
        l.goalsDone = (uint8_t)std::min(250, l.goalsDone + 1);
        if (gi < NUM_HOB) { l.hobbyLvl = (uint8_t)std::min(10, l.hobbyLvl + 1); if (HOB[gi].skill >= 0) skillGain(G.sims[s], HOB[gi].skill, 2.f); }
        remember(s, LM_GOAL, s);
        if (!G.sims[s].household) {
            if (G.sims[s].stage >= ST_TEEN) statusPostKind(s, SP_HYPE, std::string("I DID IT. I actually did it: ") + GOALS[gi].name + "!! thank u to everyone who believed in me", -1);
            if (knowsMC(s, 15)) chatNews(s, LM_GOAL, -1);
            if (G.room < R_COUNT) sayHere(s, std::string("I did it! I ") + (GOALS[gi].name) + "!");
        }
        if (gi == G_WORK && G.sims[s].career != CR_NONE && G.sims[s].careerLevel < 4) G.sims[s].careerLevel++;
    }
}

// ------------------------------------------------------------------ babies, weddings, new faces
int addTownie(Stage st, int gender) {
    if (G.sims.size() >= 120) return -1;
    Sim s = randomSim(st, gender);
    s.household = false;
    G.sims.push_back(s);
    int idx = (int)G.sims.size() - 1;
    L(idx);
    return idx;
}
void npcBaby(int a, int b) {
    if (!valid(a) || !valid(b)) return;
    int idx = addTownie(ST_INFANT, -1);
    if (idx < 0) return;
    Sim& bb = G.sims[idx];
    bb.look.skin = irand(2) ? G.sims[a].look.skin : G.sims[b].look.skin;
    bb.look.hairCol = irand(2) ? G.sims[a].look.hairCol : G.sims[b].look.hairCol;
    bb.stageDays = 0;
    for (int p : {a, b}) { Rel& r = rel(p, idx); r.status = RS_FAMILY; r.fr = 60; r.met = true; L(p).kids++; remember(p, LM_BABY, idx); }
    statusAnnounce(nm(a) + " & " + nm(b) + " just welcomed baby " + nm(idx) + "!! the cutest little face in Starlight Bay", a);
    insta(a, "Meet " + nm(idx) + " <3 our whole world");
    newsToFriends(a, b, LM_BABY);
}
void marry(int a, int b) {
    if (!valid(a) || !valid(b)) return;
    Rel& r = rel(a, b);
    if (r.status != RS_ENGAGED) return;
    r.status = RS_MARRIED;
    L(a).sinceDay = G.day; L(b).sinceDay = G.day;
    statusAnnounce("JUST MARRIED: " + nm(a) + " & " + nm(b) + "!! the vows had everyone in TEARS", a);
    insta(a, "Forever starts today <3 #JustMarried");
}
void quietAge(int s) {
    Sim& o = G.sims[s];
    if (o.stage >= ST_CHILD || STAGE_DAYS[o.stage] == 255) return;
    if (++o.stageDays < STAGE_DAYS[o.stage]) return;
    o.stage = (Stage)(o.stage + 1);
    o.stageDays = 0;
    if (o.stage == ST_CHILD) {
        if (o.traits[0] == TR_NONE) { uint8_t t; do t = (uint8_t)irand(TR_COUNT); while (!traitOk(t, o.stage)); o.traits[0] = t; }
        randomWardrobe(o.look, o.stage);
    }
}

// ------------------------------------------------------------------ choices when you run into town events
int g_evSeen = -1;
std::string evMoment(int choice, int ctx) {
    int A = G.lifeEvA, B = G.lifeEvB;
    switch (ctx) {
    case WE_LOSTPET:
        if (!valid(A)) return "";
        if (choice == 0) {
            if (irand(100) < 70) { changeRel(G.mc, A, 15, 0); remember(A, LM_HELPED, G.mc); G.rep = (int16_t)std::min(100, G.rep + 3); statusPostKind(A, SP_HYPE, "FOUND HIM!! thank u " + MC().name + " for searching with me for HOURS <3", G.mc); return "You found the little runaway hiding under a bench! " + nm(A) + " is crying happy tears."; }
            changeRel(G.mc, A, 6, 0); return "No luck yet... but " + nm(A) + " hugs you anyway. \"Thank you for trying.\"";
        }
        if (choice == 1) { statusPostKind(G.mc, SP_MOOD, "LOST PET near the park!! belongs to " + nm(A) + ", please share", A); changeRel(G.mc, A, 5, 0); return "Your post is getting shared everywhere. " + nm(A) + " sends you a grateful heart."; }
        changeRel(G.mc, A, -2, 0); return "";
    case WE_FEUD:
        if (!valid(A) || !valid(B)) return "";
        if (choice == 0) { changeRel(A, B, 12, 0); changeRel(G.mc, A, 3, 0); changeRel(G.mc, B, 3, 0); return "You step between them. Somehow it works: they're actually TALKING now."; }
        if (choice == 1) { statusPostKind(G.mc, SP_RUMOR, "caught " + nm(A) + " and " + nm(B) + " going OFF on each other lmao", A, B); G.followers += 25 + irand(30); changeRel(G.mc, A, -6, 0); changeRel(G.mc, B, -6, 0); return "Your video is going viral. They both saw it. Oops."; }
        changeRel(G.mc, A, 10, 0); changeRel(G.mc, B, -12, 0); changeRel(A, B, -6, 0); remember(B, LM_HURT, G.mc); return "You took " + nm(A) + "'s side. " + nm(B) + " will NOT forget that.";
    case WE_CELEB:
        if (!valid(A)) return "";
        if (choice == 0) { G.followers += 40 + irand(40); changeRel(G.mc, A, 3, 0); return "Selfie secured! Your followers are losing it."; }
        if (choice == 1) { if (irand(2)) { changeRel(G.mc, A, 10, 0); return nm(A) + " actually comes over to YOU. \"Love your energy.\""; } return "You play it SO cool that " + nm(A) + " doesn't notice you at all."; }
        changeRel(G.mc, A, -2, 0); MC().needs[N_FUN] = std::min(100.f, MC().needs[N_FUN] + 20); return "You screamed. In public. It was worth it.";
    case WE_WEDDING:
        if (!valid(A) || !valid(B)) return "";
        if (choice == 0) { bool good = irand(10) < 4 + skillLevel(MC(), SK_CHARISMA); changeRel(G.mc, A, good ? 8 : 2, 0); changeRel(G.mc, B, good ? 8 : 2, 0); return good ? "Your toast has the whole chapel laughing and crying. Iconic." : "You forgot half your toast, but everyone claps anyway."; }
        if (choice == 1) { addMoodlet(MC(), "Caught the Bouquet!", EM_FLIRTY, 2, 480); int p = partnerOf(G.mc); if (p >= 0) changeRel(G.mc, p, 0, 5); return "You caught the bouquet! Everyone's looking at you now..."; }
        if (roOf(G.mc, A) >= 40 || roOf(G.mc, B) >= 40) {
            int mine = roOf(G.mc, A) >= roOf(G.mc, B) ? A : B, other = mine == A ? B : A;
            Rel& r = rel(A, B); r.status = RS_NONE; r.ro = 0; r.fr = (int8_t)clampv(r.fr - 30, -100, 100);
            remember(other, LM_HURT, G.mc); remember(other, LM_BREAKUP, mine);
            statusAnnounce("WEDDING CANCELLED?! " + MC().name + " OBJECTED and " + nm(mine) + " ran out of the chapel with them", G.mc);
            changeRel(G.mc, mine, 5, 15); changeRel(G.mc, other, -40, 0);
            G.lifeEv = WE_NONE;
            return nm(mine) + " drops the flowers... and walks over to YOU. The chapel gasps.";
        }
        G.rep = (int16_t)std::max(-100, G.rep - 10); changeRel(G.mc, A, -25, 0); changeRel(G.mc, B, -25, 0);
        statusAnnounce(MC().name + " just OBJECTED at " + nm(A) + " & " + nm(B) + "'s wedding?? for NO reason??", G.mc);
        return "Silence. Then the whole chapel boos you out. That's going on STATUS.";
    }
    return "";
}

// ------------------------------------------------------------------ family storylines at home
int g_fa = -1, g_fb = -1, g_famDay = -1;
std::string famMoment(int choice, int ctx) {
    int A = g_fa, B = g_fb;
    Sim& me = MC();
    switch (ctx) {
    case 0:   // squabble
        if (!valid(A) || !valid(B)) return "";
        if (choice == 2) { changeRel(A, B, 6, 0); changeRel(G.mc, A, 2, 0); changeRel(G.mc, B, 2, 0); return "A mirror schedule, taped to the door. Peace returns. For now."; }
        { int win = choice == 0 ? A : B, lose = choice == 0 ? B : A; changeRel(G.mc, win, 6, 0); changeRel(G.mc, lose, -6, 0); changeRel(A, B, -4, 0); remember(lose, LM_HURT, G.mc); return nm(win) + " smirks. " + nm(lose) + " slams a door. \"You ALWAYS take their side!\""; }
    case 1:   // sneaking out
        if (!valid(A)) return "";
        if (choice == 0) { changeRel(G.mc, A, -8, 0); G.sims[A].gradePts = clampv(G.sims[A].gradePts + 4.f, 0.f, 100.f); return nm(A) + " climbs back in at midnight and finds you waiting. Grounded for a week."; }
        if (choice == 1) { changeRel(G.mc, A, 6, 0); remember(A, LM_KIND, G.mc); return "\"...ok. Coming home.\" Later, " + nm(A) + " actually tells you what's going on with them."; }
        changeRel(G.mc, A, 10, 0);
        if (irand(3) == 0) { G.rep = (int16_t)std::max(-100, G.rep - 4); return "You covered for them... and the neighbors saw everything. Awkward."; }
        return "Your secret's safe with me. " + nm(A) + " owes you BIG.";
    case 2:   // anniversary
        if (!valid(A)) return "";
        if (choice == 0) { G.money -= 60; changeRel(G.mc, A, 4, 12); addMemory(MEM_LOVE, G.mc, A, "Anniversary dinner with " + nm(A)); return "Candles, dessert, and " + nm(A) + " can't stop smiling. (-$60)"; }
        if (choice == 1) { changeRel(G.mc, A, 4, 8); me.skills[SK_COOKING] = std::min(10.f, me.skills[SK_COOKING] + 0.3f); return "You cook their favorite. It's a little burnt. They love it anyway."; }
        changeRel(G.mc, A, 0, 4); return "A sappy little note on the pillow. They keep it forever.";
    case 3:   // family dinner
        for (size_t i = 0; i < G.sims.size(); i++) {
            Sim& s = G.sims[i];
            if (!s.household || (int)i == G.mc) continue;
            if (choice == 0) { changeRel(G.mc, (int)i, 4, 0); s.needs[N_HUNGER] = std::min(100.f, s.needs[N_HUNGER] + 30); }
            else if (choice == 1) { changeRel(G.mc, (int)i, 2, 0); s.needs[N_FUN] = std::min(100.f, s.needs[N_FUN] + 15); }
            else changeRel(G.mc, (int)i, -2, 0);
        }
        if (choice == 0) { me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + 30); addMemory(MEM_FAMILY, G.mc, -1, "Family dinner night"); return "Everyone helps (sort of). Best spaghetti ever."; }
        if (choice == 1) { G.money -= 35; me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 15); return "Pizza on the floor and a movie. Perfect. (-$35)"; }
        return "Everyone scrolls in silence. Nobody's mad... but nobody's close either.";
    case 4:   // report card
        if (!valid(A)) return "";
        if (choice == 0) { changeRel(G.mc, A, 6, 0); addMoodlet(G.sims[A], "Ice Cream Celebration", EM_HAPPY, 2, 240); return nm(A) + " gets sprinkles. Extra sprinkles."; }
        if (choice == 1) { G.sims[A].gradePts = clampv(G.sims[A].gradePts + 8.f, 0.f, 100.f); changeRel(G.mc, A, -1, 0); return "Homework time, every night at 6. " + nm(A) + " groans... but the grades go up."; }
        changeRel(G.mc, A, -8, 0); G.sims[A].gradePts = clampv(G.sims[A].gradePts + 3.f, 0.f, 100.f); remember(A, LM_HURT, G.mc); return nm(A) + " stomps upstairs. \"You don't get it!\"";
    case 5:   // money
        if (choice == 0) { G.money += 150; me.needs[N_ENERGY] = std::max(0.f, me.needs[N_ENERGY] - 25); return "Extra shifts all week. Exhausting... but the fridge is full again. (+$150)"; }
        if (choice == 1) { G.money += 80; return "Your old stuff sells in an hour. Someone LOVED that lamp. (+$80)"; }
        if (valid(A)) { G.money += 200; changeRel(G.mc, A, -3, 0); return nm(A) + " sends $200 with a long lecture about budgeting."; }
        return "Nobody picks up. Ramen it is.";
    case 6:   // relative visit
        if (!valid(A)) return "";
        if (choice == 0) {
            Actor a; a.kind = 0; a.id = (uint16_t)A; a.x = 380; a.tx = 280; a.y = 210; a.say = "Look at you! You've grown!"; a.sayT = 3; a.stay = 180;
            G.rooms[G.house][R_LIVING].actors.push_back(a);
            changeRel(G.mc, A, 6, 0); return nm(A) + " is in the living room with homemade cookies!";
        }
        if (choice == 1) { Event e; e.day = (uint16_t)(G.day + 1); e.hour = 11; e.type = EV_PLAN; e.sim = (int16_t)A; e.loc = R_CAFE; e.text = "Brunch with " + nm(A); G.events.push_back(e); changeRel(G.mc, A, 2, 0); return "Brunch tomorrow at 11! (Planner)"; }
        changeRel(G.mc, A, -5, 0); return "\"Oh... okay. Maybe next time, sweetie.\"";
    case 7:   // their dream
        if (!valid(A)) return "";
        if (choice == 0) { lifeBoostGoal(A, 20); changeRel(G.mc, A, 5, 0); return nm(A) + " stands a little taller. \"Okay. I can do this.\""; }
        if (choice == 1) { lifeBoostGoal(A, 12); changeRel(G.mc, A, 4, 0); me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 10); return "You practice together all evening. Real progress!"; }
        changeRel(G.mc, A, -6, 0); remember(A, LM_HURT, G.mc); return nm(A) + " goes quiet. \"Thanks for the support.\"";
    case 9:   // you vs. your sibling
        if (!valid(A)) return "";
        if (choice == 0) { changeRel(G.mc, A, -5, 0); remember(A, LM_HURT, G.mc); return nm(A) + " grumbles and hands over their allowance. \"FINE. Happy now?\""; }
        if (choice == 1) { changeRel(G.mc, A, 6, 0); remember(A, LM_KIND, G.mc); return nm(A) + " looks guilty... then makes you a smoothie. A clean one."; }
        changeRel(G.mc, A, 3, 0); return "New rule: ask first. " + nm(A) + " pinky-promises.";
    case 8: {   // friend's baby
        if (!valid(A)) return "";
        if (choice == 0) { G.money -= 40; changeRel(G.mc, A, 12, 0); return "A tiny onesie with a bow. " + nm(A) + " LOVES it. (-$40)"; }
        if (choice == 1) { changeRel(G.mc, A, 8, 0); addMoodlet(me, "Baby Snuggles", EM_HAPPY, 2, 240); return "You hold the baby. Tiny fingers. Instant tears."; }
        changeRel(G.mc, A, 3, 0); return "\"aww thank u!!\"";
    }
    }
    return "";
}
std::string optA, optB;
bool famStoryline() {
    std::vector<int> kids, teens, grown;
    for (size_t i = 0; i < G.sims.size(); i++) {
        const Sim& s = G.sims[i];
        if (!s.household || (int)i == G.mc) continue;
        if (s.stage >= ST_CHILD && s.stage <= ST_TEEN) kids.push_back((int)i);
        if (s.stage == ST_TEEN) teens.push_back((int)i);
        if (s.stage >= ST_CHILD) grown.push_back((int)i);
    }
    std::vector<int> pick;
    int h = hourNow();
    if (kids.size() >= 2 || (kids.size() >= 1 && MC().stage <= ST_TEEN)) pick.push_back(0);
    if (!teens.empty() && h >= 20 && MC().isAdult()) pick.push_back(1);
    { int p = partnerOf(G.mc); const Rel* r = p >= 0 ? findRel(G.mc, p) : nullptr; if (r && r->status == RS_MARRIED && G.day % 7 == 3) pick.push_back(2); }
    if (!grown.empty()) pick.push_back(3);
    if (!kids.empty() && MC().isAdult() && isWeekday(G.day)) pick.push_back(4);
    if (G.money < 400) pick.push_back(5);
    int relative = -1;
    for (const Rel& r : G.rels) { int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1; if (r.status == RS_FAMILY && valid(o) && !G.sims[o].household && G.sims[o].stage >= ST_ADULT) relative = o; }
    if (relative >= 0) pick.push_back(6);
    if (!grown.empty()) pick.push_back(7);
    if (pick.empty()) return false;
    int kind = pick[irand((int)pick.size())];
    g_fa = g_fb = -1;
    switch (kind) {
    case 0:
        g_fa = kids[irand((int)kids.size())];
        if (kids.size() >= 2) { for (int tries = 0; tries < 12 && (g_fb < 0 || g_fb == g_fa); tries++) g_fb = kids[irand((int)kids.size())]; if (g_fb == g_fa) g_fb = G.mc; } else g_fb = G.mc;
        if (g_fb == G.mc) {   // you're one of the siblings
            momentCustom("Sibling Squabble", nm(g_fa) + " \"borrowed\" your favorite top and spilled smoothie on it. They're pretending it was always like that.",
                         "Demand a new one", "Let it go", "Make a sharing rule", famMoment, 9, g_fa);
            return true;
        }
        optA = "Side with " + nm(g_fa); optB = "Side with " + nm(g_fb);
        momentCustom("Sibling Squabble", nm(g_fa) + " and " + nm(g_fb) + " are fighting over the bathroom mirror. AGAIN. Both of them are yelling your name.", optA.c_str(), optB.c_str(), "Make a schedule", famMoment, 0, g_fa);
        return true;
    case 1:
        g_fa = teens[irand((int)teens.size())];
        momentCustom("Sneaking Out?", nm(g_fa) + "'s window is open and their bed is stuffed with pillows shaped like a person. Classic.",
                     "Wait up. Grounded.", "Text: come home, no questions", "Cover for them", famMoment, 1, g_fa);
        return true;
    case 2:
        g_fa = partnerOf(G.mc);
        momentCustom("Anniversary!", "It's your anniversary with " + nm(g_fa) + "... and you almost forgot. Almost.", "Surprise dinner date ($60)", "Cook their favorite", "Write a sappy note", famMoment, 2, g_fa);
        return true;
    case 3:
        momentCustom("Family Dinner", "Everyone's actually home at the same time for once. Miracles happen.", "Cook together", "Order pizza ($35)", "Everyone on their phones", famMoment, 3, grown[0]);
        return true;
    case 4: {
        g_fa = kids[irand((int)kids.size())];
        momentCustom("Report Card", nm(g_fa) + "'s report card came home: " + letterGrade(G.sims[g_fa].gradePts) + ". They're watching your face very closely.",
                     "Celebrate with ice cream", "Set up homework time", "Lecture & ground them", famMoment, 4, g_fa);
        return true;
    }
    case 5:
        g_fa = relative;
        momentCustom("Money's Tight", "Bills are piling up and the fridge is looking sad. Something has to give.", "Pick up extra shifts", "Sell old stuff online", relative >= 0 ? "Ask family for help" : "Hope for the best", famMoment, 5, -1);
        return true;
    case 6:
        g_fa = relative;
        momentCustom("Family Visit", nm(g_fa) + " is in town and wants to stop by! They're already asking if you're eating enough.", "Invite them over", "Brunch tomorrow", "Pretend you're busy", famMoment, 6, g_fa);
        return true;
    case 7:
        g_fa = grown[irand((int)grown.size())];
        momentCustom("A Big Dream", nm(g_fa) + " wants to " + lifeGoalName(g_fa) + ", but they're nervous to really go for it.", "Cheer them on", "Practice with them", "Tell them to be realistic", famMoment, 7, g_fa);
        return true;
    }
    return false;
}
}   // namespace

// ================================================================== API
int lifeAddTownie(int stage, int gender) { return addTownie((Stage)stage, gender); }
void lifeMarry(int a, int b) { marry(a, b); }
const char* lifeHobbyName(int sim) { return HOB[hobbyOf(sim)].name; }
const char* lifeGoalName(int sim) { return GOALS[goalIndex(sim)].name; }
const char* lifeFavPlaceName(int sim) { return ROOM_NAMES[favOf(sim) % R_COUNT]; }
bool lifeAway(int sim) { return valid(sim) && sim < (int)G.life.size() && G.life[sim].away > 0; }
int lifeGoalProgress(int sim) { return valid(sim) ? L(sim).goalProg : 0; }

int lifeWhere(int s, int hour) {
    if (!townie(s)) return -1;
    const Sim& o = G.sims[s];
    if (o.stage <= ST_TODDLER) return -1;
    if (L(s).away) return -2;
    int r = routine(s, hour);
    int e = eventPull(s, hour);
    if (e >= 0) r = e;
    if (r >= 0 && e < 0 && badWeather() && roomOutdoors(r)) r = indoorAlt(s);
    if (r >= 0 && !townieFits(r, o)) r = o.stage <= ST_TEEN ? R_ARCADE : -1;
    return r;
}

std::string lifeStatusLine(int s) {
    if (!valid(s)) return "";
    if (G.sims[s].household) return "Lives with you";
    if (lifeAway(s)) return "Out of town";
    int w = lifeWhere(s, hourNow());
    Job j = jobOf(s);
    std::string o = w < 0 ? std::string("At home") : std::string("At ") + ROOM_NAMES[w % R_COUNT];
    if (w >= 0 && w == j.room && isWeekday(G.day)) o += std::string(" (") + j.title + ")";
    else if (w >= 0 && w == hobbyRoom(s)) o += std::string(" (") + HOB[hobbyOf(s)].doing + ")";
    return o;
}

void lifeRemember(int sim, int kind, int other) { remember(sim, kind, other); }
void lifeBoostGoal(int sim, int amount) { goalStep(sim, amount); }

int lifePickTownie(int room) {
    if (G.sims.empty() || room < 0 || room >= R_COUNT) return -1;
    int hour = hourNow();
    const auto& here = G.rooms[G.house][room].actors;
    std::vector<int> v;
    for (size_t i = 0; i < G.sims.size(); i++) {
        int s = (int)i;
        if (!townie(s) || s == G.cop || G.sims[s].stage <= ST_TODDLER) continue;
        if (lifeWhere(s, hour) != room) continue;
        bool dup = false; for (const Actor& a : here) if (a.kind == 0 && a.id == s) dup = true;
        if (!dup) v.push_back(s);
    }
    return v.empty() ? -1 : v[irand((int)v.size())];
}

std::string lifeLine(int s) {
    if (!townie(s)) return "";
    const Sim& o = G.sims[s];
    if (o.stage < ST_CHILD) return "";
    int r = irand(100);
    int hour = hourNow();
    auto sub = [](std::string t, const char* k, const std::string& v) { size_t p; while ((p = t.find(k)) != std::string::npos) t.replace(p, std::strlen(k), v); return t; };
    // a town event here
    if (evOnAt(hour) && G.lifeEvRoom == G.room && r < 35) {
        static const char* const EV[WE_COUNT][2] = {
            {"", ""}, {"Everything's half off!!", "I can't stop buying things."}, {"Is it my turn at the mic yet?", "That last singer was SO good."},
            {"S'mores? S'mores.", "The stars are so pretty tonight."}, {"Here, kitty kitty!", "Has anyone seen a little runaway?"},
            {"Uh oh, it's getting heated over there...", "Did you SEE that?!"}, {"Is that really them?!", "Act natural. ACT NATURAL."},
            {"These tomatoes are huge!", "Fresh honey! I'm obsessed."}, {"I'm going to win this time.", "Rematch! RIGHT NOW."},
            {"Free samples = free lunch.", "I've had six cookies. No regrets."}, {"I'm going to cry, I know it.", "The flowers are beautiful!"},
            {"Have you met the new person yet?", "Welcome to the neighborhood!"}, {"One more rep!", "My arms are jelly."},
            {"No spoilers!!", "I've waited months for this."}, {"Best festival EVER!", "Come on, let's do everything!"}};
        return EV[G.lifeEv][irand(2)];
    }
    // their job
    Job j = jobOf(s);
    if (j.room == G.room && isWeekday(G.day) && hour >= j.start && hour < j.start + j.hours && r < 50) {
        static const char* const W[5] = {"Welcome in! Let me know if you need anything.", "Only {h} more hours of my shift...", "My manager is watching. Act busy.", "I love being a {t}. Mostly.", "Break time can't come soon enough."};
        std::string t = W[irand(5)];
        return sub(sub(t, "{h}", std::to_string(std::max(1, j.start + j.hours - hour))), "{t}", j.title);
    }
    // what they remember about you
    if (r < 60) {
        if (recentMem(s, LM_HELPED, 6, G.mc)) return "You're the one who helped me! I owe you, " + MC().name + "!";
        if (recentMem(s, LM_HURT, 3, G.mc)) return irand(2) ? "Oh. It's you. I haven't forgotten." : "Still thinking about what you said, " + MC().name + "...";
        if (recentMem(s, LM_KIND, 3, G.mc)) return "You were so sweet to me the other day, " + MC().name + "!";
    }
    // their own news
    if (r < 70) {
        const LifeMemory* m;
        if ((m = recentMem(s, LM_BABY, 5))) return "Our baby " + nm(m->other) + " kept us up ALL night. Worth it.";
        if ((m = recentMem(s, LM_BREAKUP, 3))) return irand(2) ? "I'm SO over " + nm(m->other) + ". Totally. Completely." : "Please don't say " + nm(m->other) + "'s name around me.";
        if ((m = recentMem(s, LM_ROMANCE, 3))) return "Did you hear? Me and " + nm(m->other) + " are a thing now!";
        if ((m = recentMem(s, LM_FIGHT, 2))) return "Ugh, " + nm(m->other) + " is SO annoying.";
        if ((m = recentMem(s, LM_FRIEND, 3))) return nm(m->other) + " is literally my best friend now.";
        if (recentMem(s, LM_GOAL, 3)) return "I did it! I actually reached my dream!";
        if (recentMem(s, LM_MOVEDIN, 4)) return "I just moved here! Where's the good coffee?";
        int p = partnerOf(s);
        if (p >= 0 && actorHere(p)) return irand(2) ? nm(p) + ", stop, you're embarrassing me <3" : "Me and " + nm(p) + " come here all the time.";
    }
    if (hobbyRoom(s) == G.room && r < 85) return HOB[hobbyOf(s)].brag[irand(3)];
    if (r < 12) { int p = L(s).goalProg; return p >= 40 ? "I'm " + std::to_string(p) + "% of the way to my dream: " + lifeGoalName(s) + "!" : std::string("Someday I'm going to ") + lifeGoalName(s) + "."; }
    return "";
}

int lifeConvo(int a, int b, std::string out[4]) {
    if (!townie(a) || !townie(b)) return 0;
    for (int i = 0; i < 4; i++) out[i].clear();
    const Rel* r = findRel(a, b);
    int f = r ? r->fr : 0;
    int n = 0;
    if (r && r->status >= RS_DATING && r->status <= RS_MARRIED) {
        static const char* const C[3][4] = {{"Hey you <3", "Hey cutie.", "Dinner later?", "It's a date!"},
                                            {"You look really good today.", "Stoppp.", "I mean it!", "*hand holding*"},
                                            {"Should we get a puppy?", "We can barely keep a plant alive.", "...a small puppy?", "Fine. A small one."}};
        int k = irand(r->status == RS_DATING ? 2 : 3);
        for (int i = 0; i < 4; i++) out[i] = C[k][i];
        n = 4;
        changeRel(a, b, 1, 1);
    } else if (f <= -30) {
        static const char* const C[2][4] = {{"Oh. It's you.", "Don't start, {a}.", "I'm not the one who started it!", "Whatever."},
                                            {"Nice outfit. Did you borrow it from {b2}?", "Wow. Original.", "Just saying!", "Bye."}};
        int k = irand(2);
        for (int i = 0; i < 4; i++) out[i] = C[k][i];
        n = 4;
        changeRel(a, b, -1, 0);
    } else if (hobbyOf(a) == hobbyOf(b) && irand(2)) {
        out[0] = std::string("Have you been ") + HOB[hobbyOf(a)].doing + " lately?";
        out[1] = "Every day! I'm getting so good.";
        out[2] = "We should go together sometime!";
        out[3] = "Deal!";
        n = 4;
        changeRel(a, b, 2, 0);
    } else {
        const LifeMemory* m = recentMem(b, LM_BREAKUP, 4);
        if (!m) m = recentMem(b, LM_FIGHT, 2);
        if (m && valid(m->other) && m->other != a) {
            out[0] = "Are you okay after the whole " + nm(m->other) + " thing?";
            out[1] = irand(2) ? "Honestly? Not really." : "I'm fine. Totally fine.";
            out[2] = "I'm here if you need me.";
            n = 3;
            changeRel(a, b, 3, 0);
        } else if (irand(2)) {
            out[0] = "How's the dream going? You know, " + std::string(lifeGoalName(b)) + "?";
            int p = L(b).goalProg;
            out[1] = p >= 50 ? "So close! " + std::to_string(p) + "% there!" : "Slowly... but I'm not giving up.";
            out[2] = "You've got this!";
            n = 3;
            changeRel(a, b, 1, 0);
        }
    }
    for (int i = 0; i < n; i++) {
        size_t p;
        while ((p = out[i].find("{a}")) != std::string::npos) out[i].replace(p, 3, nm(a));
        while ((p = out[i].find("{b2}")) != std::string::npos) { int x = irand((int)G.sims.size()); out[i].replace(p, 4, x != a && x != b ? nm(x) : std::string("your mom")); }
    }
    return n;
}

void lifeHourly(int hour) {
    if (G.sims.size() < 3) return;
    L(0);
    if (hour == 8 && evToday()) toast("Today in town: " + evText());
    if (G.lifeEv == WE_WEDDING && evToday() && hour == WED[WE_WEDDING].h1) marry(G.lifeEvA, G.lifeEvB);
    // where is everybody? (and dreams move forward while they're out living)
    static std::vector<int> at[R_COUNT];
    for (auto& v : at) v.clear();
    const int ns = (int)G.sims.size();
    for (int s = 0; s < ns; s++) {
        if (!townie(s) || G.sims[s].stage < ST_CHILD) continue;
        int w = lifeWhere(s, hour);
        if (w < 0 || w >= R_COUNT) continue;
        at[w].push_back(s);
        int gi = goalIndex(s);
        const Sim& o = G.sims[s];
        int amt = 0;
        if (GOALS[gi].kind == GK_PRACTICE && gi < NUM_HOB && w == HOB[gi].room) amt = irand(3) + (o.hasTrait(TR_AMBITIOUS) ? 1 : 0) - (o.hasTrait(TR_LAZY) && irand(2) ? 1 : 0);
        else if (GOALS[gi].kind == GK_SCHOOL && w == R_SCHOOL) amt = irand(4) == 0;
        else if (GOALS[gi].kind == GK_WORK && w == jobOf(s).room) amt = irand(4) == 0;
        else if (GOALS[gi].kind == GK_LOVE && (w == R_CLUB || w == R_CAFE) && irand(3) == 0) amt = 1;
        if (amt > 0) goalStep(s, amt);
    }
    // the people who share a place this hour interact
    int rooms[R_COUNT]; int nr = 0;
    for (int r = 0; r < R_COUNT; r++) if (at[r].size() >= 2) rooms[nr++] = r;
    for (int k = 0; k < 4 && nr > 0; k++) {
        int pick = irand(nr);
        int r = rooms[pick];
        auto& v = at[r];
        int a = v[irand((int)v.size())], b = -1;
        for (int tries = 0; tries < 12 && (b < 0 || b == a); tries++) b = v[irand((int)v.size())];   // never spin forever on duplicates
        if (b == a || b < 0) { rooms[pick] = rooms[--nr]; continue; }
        if (G.storyProg && G.sims[a].stage >= ST_CHILD && G.sims[b].stage >= ST_CHILD) interact(a, b, r);
        rooms[pick] = rooms[--nr];
    }
    // couples on the same date night grow closer
    for (int r = 0; r < R_COUNT; r++)
        for (size_t i = 0; i < at[r].size(); i++) {
            int a = at[r][i], p = partnerOf(a);
            if (p > a && std::find(at[r].begin(), at[r].end(), p) != at[r].end() && irand(2)) changeRel(a, p, 1, 2);
        }
    // in town with you: people whose routine is taking them elsewhere head off
    if (!roomIsHome(G.room) && G.room != R_NEIGHBOR) {
        for (Actor& a : curRoom().actors) {
            if (a.kind != 0 || !townie(a.id) || a.passer || a.stay <= 0 || (G.date.active && G.date.partner == a.id)) continue;
            int w = lifeWhere(a.id, hour);
            if (w == G.room || irand(2)) continue;
            a.stay = std::min(a.stay, 4.f + irand(20));
            Job j = jobOf(a.id);
            if (w >= 0 && w == j.room) { a.say = "Shift starts soon, gotta run!"; a.sayT = 2.5f; }
            else if (w == R_SCHOOL) { a.say = "I'm gonna be late for school!"; a.sayT = 2.5f; }
            else if (w >= 0 && w < R_COUNT && irand(2)) { a.say = std::string("Heading to ") + ROOM_NAMES[w] + "!"; a.sayT = 2.5f; }
            else if (w == -1 && hour >= 21) { a.say = "So sleepy. Night, everyone!"; a.sayT = 2.5f; }
        }
    }
    // family storylines when you're home in the evening
    if (hour >= 18 && hour <= 21 && roomIsHome(G.room) && !momentActive() && g_famDay != G.day && G.dramaDay != G.day && irand(100) < 14) {
        if (famStoryline()) g_famDay = G.day;
    }
}

void lifeDaily() {
    if (G.sims.size() < 3) return;
    L(0);
    // travellers
    int ns = (int)G.sims.size();
    for (int s = 0; s < ns; s++) {
        NpcLife& l = L(s);
        if (!townie(s)) { l.away = 0; continue; }
        if (l.away > 0 && --l.away == 0 && G.sims[s].stage >= ST_TEEN) statusPostKind(s, SP_MOOD, "back in Starlight Bay!! did i miss anything?? (i definitely missed something)", -1);
        quietAge(s);
    }
    if (irand(100) < 12) {
        int s = irand(ns);
        if (townie(s) && G.sims[s].stage >= ST_TEEN && partnerOf(s) != G.mc && !isCastSim(s) && s != G.cop) {
            L(s).away = (uint8_t)(1 + irand(3));
            static const char* const WHY[5] = {"visiting my grandma this week!! bye starlight bay", "vacation mode: ON", "road trip!! no service probably",
                                              "family stuff out of town, text me", "summer camp counselor era (send snacks)"};
            statusPostKind(s, SP_MOOD, WHY[irand(5)], -1);
        }
    }
    // dreams that move day by day; love lives on a schedule
    for (int s = 0; s < ns; s++) {
        if (!valid(s) || G.sims[s].stage < ST_CHILD) continue;
        int gi = goalIndex(s);
        if (GOALS[gi].kind == GK_LOVE) {
            int p = partnerOf(s);
            const Rel* r = p >= 0 ? findRel(s, p) : nullptr;
            int target = !r ? 0 : r->status == RS_DATING ? 60 : r->status == RS_ENGAGED ? 85 : 100;
            if (target > L(s).goalProg) goalStep(s, target - L(s).goalProg);
        } else if (GOALS[gi].kind == GK_VIRAL && townie(s) && hsh((uint32_t)s * 17u + G.day) % 3 == 0) goalStep(s, irand(6) + (G.sims[s].hasTrait(TR_OUTGOING) ? 2 : 0));
        else if (GOALS[gi].kind == GK_BFF) {
            int best = 0;
            for (const Rel& r : G.rels) if ((r.a == s || r.b == s) && r.status != RS_FAMILY) best = std::max<int>(best, r.fr);
            int target = std::min(100, best * 100 / 60);
            if (target > L(s).goalProg) goalStep(s, target - L(s).goalProg);
        }
    }
    // weddings & babies
    int wedA = -1, wedB = -1;
    for (const Rel& r : G.rels) {
        if (!townie(r.a) || !townie(r.b)) continue;
        if (r.status == RS_ENGAGED && G.day >= L(r.a).sinceDay + 2 && wedA < 0) { wedA = r.a; wedB = r.b; }
    }
    for (size_t i = 0; i < G.rels.size(); i++) {
        Rel r = G.rels[i];
        if (r.status != RS_MARRIED || !townie(r.a) || !townie(r.b)) continue;
        const Sim &A = G.sims[r.a], &B = G.sims[r.b];
        if (A.stage < ST_YA || A.stage > ST_ADULT || B.stage < ST_YA || B.stage > ST_ADULT) continue;
        if (G.day < L(r.a).sinceDay + 3 || L(r.a).kids >= 2) continue;
        int chance = (A.hasTrait(TR_FAMILY) || B.hasTrait(TR_FAMILY)) ? 20 : 6;
        if (irand(100) < chance) {
            npcBaby(r.a, r.b);
            if ((knowsMC(r.a, 40) || knowsMC(r.b, 40)) && !momentActive()) {
                g_fa = knowsMC(r.a, 40) ? r.a : r.b;
                momentCustom("New Baby!", nm(r.a) + " & " + nm(r.b) + " just had a baby! " + nm(g_fa) + " sent you the tiniest, cutest photo.", "Send a gift ($40)", "Go visit", "Text congrats", famMoment, 8, g_fa);
            }
            break;   // one baby a day is plenty
        }
    }
    // today's town event
    G.lifeEv = WE_NONE; G.lifeEvA = G.lifeEvB = -1; G.lifeEvRoom = 0; G.lifeEvDay = G.day;
    g_evSeen = -1;
    if (festToday() >= 0 && wedA < 0) {
        G.lifeEv = WE_FESTIVAL; G.lifeEvRoom = (uint8_t)festRoom(festToday());
    } else if (wedA >= 0) {
        G.lifeEv = WE_WEDDING; G.lifeEvA = (int16_t)wedA; G.lifeEvB = (int16_t)wedB; G.lifeEvRoom = R_CHAPEL;
    } else {
        std::vector<int> opts;
        for (int e = 1; e < WE_COUNT; e++) {
            if (e == WE_WEDDING) continue;
            if (WED[e].outdoor && badWeather()) continue;
            opts.push_back(e);
        }
        for (int tries = 0; tries < 6 && G.lifeEv == WE_NONE && !opts.empty(); tries++) {
            int e = opts[irand((int)opts.size())];
            int A = -1, B = -1, room = WED[e].room;
            if (e == WE_LOSTPET) { A = irand(ns); if (!townie(A) || G.sims[A].stage < ST_CHILD || lifeAway(A)) continue; }
            else if (e == WE_FEUD) {
                for (const Rel& r : G.rels) if (r.fr <= -30 && townie(r.a) && townie(r.b) && !lifeAway(r.a) && !lifeAway(r.b) && G.sims[r.a].stage >= ST_TEEN && G.sims[r.b].stage >= ST_TEEN && irand(2)) { A = r.a; B = r.b; break; }
                if (A < 0) continue;
                room = favOf(A);
            } else if (e == WE_CELEB) {
                std::vector<int> cast;
                for (int s = 0; s < ns; s++) if (isCastSim(s) && townie(s) && !lifeAway(s) && G.sims[s].isAdult()) cast.push_back(s);
                if (cast.empty()) continue;
                A = cast[irand((int)cast.size())];
            } else if (e == WE_NEWBIE) {
                if (G.sims.size() >= 120 || irand(2)) continue;
                A = addTownie(irand(4) == 0 ? ST_TEEN : irand(3) == 0 ? ST_ADULT : ST_YA, irand(2));
                if (A < 0) continue;
                ns = (int)G.sims.size();
                remember(A, LM_MOVEDIN, A);
                room = favOf(A);
                if (!townieFits(room, G.sims[A])) room = R_CAFE;
                statusPostKind(A, SP_MOOD, "just moved to Starlight Bay!! hi?? be nice to me pls", -1);
            }
            if (room < 0 || room >= R_COUNT) continue;
            G.lifeEv = (uint8_t)e; G.lifeEvA = (int16_t)A; G.lifeEvB = (int16_t)B; G.lifeEvRoom = (uint8_t)room;
        }
    }
    if (G.lifeEv != WE_NONE && irand(3)) statusPostKind(ST_ACC_TEA, SP_EVENT, evText(), G.lifeEvA >= 0 ? G.lifeEvA : -1);
}

void lifeOnTravel(int room) {
    if (!evOnAt(hourNow()) || room != G.lifeEvRoom || g_evSeen == G.day) return;
    g_evSeen = G.day;
    // make sure the people it's about are actually here
    for (int s : {(int)G.lifeEvA, (int)G.lifeEvB}) {
        if (!townie(s) || actorHere(s)) continue;
        Actor a; a.kind = 0; a.id = (uint16_t)s; a.x = 120.f + irand(160); a.tx = a.x; a.y = 196.f + irand(30); a.stay = 120;
        curRoom().actors.push_back(a);
    }
    toast(evText());
    addMoodlet(MC(), "Town Buzz", EM_PLAYFUL, 1, 180);
    if (momentActive()) return;
    int A = G.lifeEvA, B = G.lifeEvB;
    switch (G.lifeEv) {
    case WE_LOSTPET:
        momentCustom("Lost Pet!", nm(A) + " is calling for their lost pet, eyes all red. \"He's never run off before...\"", "Help search", "Post it on STATUS", "Keep walking", evMoment, WE_LOSTPET, A);
        break;
    case WE_FEUD:
        momentCustom("Public Feud", nm(A) + " and " + nm(B) + " are about to throw down right here. Everyone's pretending not to watch. Everyone is watching.",
                     "Break it up", "Film it for STATUS", ("Take " + nm(A) + "'s side").c_str(), evMoment, WE_FEUD, A);
        break;
    case WE_CELEB:
        momentCustom("Celebrity Sighting!", nm(A) + " is RIGHT THERE. Sunglasses indoors. Iconic.", "Ask for a selfie", "Play it cool", "Scream a little", evMoment, WE_CELEB, A);
        break;
    case WE_WEDDING:
        momentCustom("The Wedding", nm(A) + " & " + nm(B) + "'s wedding! Flowers, tears, a string quartet... \"If anyone objects, speak now.\"", "Give a toast", "Catch the bouquet", "OBJECT!", evMoment, WE_WEDDING, A);
        break;
    case WE_NEWBIE:
        if (townie(A)) { dlgSay(A, "Hi! I'm " + nm(A) + ", I just moved here. Everyone keeps saying I HAVE to meet you!"); Rel& r = rel(G.mc, A); r.met = true; r.fr = (int8_t)std::max<int>(r.fr, 8); }
        break;
    default: break;
    }
}

std::string lifeTodayHeadline() { return evText(); }

// exported for romance.cpp
int lifeCompat(int a, int b) { return compat(a, b); }
bool lifeSimHere(int s) { return actorHere(s) != nullptr; }
