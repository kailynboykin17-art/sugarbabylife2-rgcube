#include "life3.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "goals.hpp"
#include "extras.hpp"
#include "growup.hpp"
#include "tween.hpp"
#include "travelcourt.hpp"
#include "dynasty.hpp"
#include "levels.hpp"
#include "vfs.hpp"
#include "people.hpp"
#include "kidlife.hpp"
#include "lifestory.hpp"
#include "goals.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#define fopen vfopen
void toast(const std::string& s);
int lxHolidayToday(int day, const char** name);

namespace {
// ===================================================================== state (saved as ".lf3")
struct Recipe { char name[28]; uint8_t gen, pad[3]; };
struct L3 {
    uint32_t magic = 0x3346334Cu;   // "L3F3"
    // vacation homes
    uint8_t vhOwn[3] = {}, vhRent[3] = {}, vhDecor[3] = {}, vhBroken[3] = {};
    // attributes (Strength, Brains, Charm) + today's activity bits
    uint8_t attr[3] = {}, actBits[3] = {}; uint16_t actDay = 0xFFFF;
    // dreams
    uint8_t dream = 0, dreamsDone = 0, chapters = 0, pad0 = 0;
    // TV
    uint16_t tvWatched = 0, premiumUntil = 0;
    // family
    uint8_t craving = 0, cravingDone = 0, showerDone = 0, practice = 0, curfew = 22, curfewDay = 0, crashes = 0, nRecipes = 0;
    Recipe recipes[10] = {};
    uint16_t adopted = 0, handMeDowns = 0;
    // town
    uint8_t mayor = 0, campaign = 0, campDone = 0, feud = 0; uint16_t campEnd = 0, support = 0; int16_t feudWith = -1; uint16_t feudsSolved = 0;
    uint16_t paradeDay = 0xFFFF, showsWon = 0;
    // style
    uint16_t grwmDay = 0xFFFF, grwms = 0, rareFinds = 0, dropsBought = 0;
    // badges already announced (bitmask)
    uint32_t badges = 0;
    // hidden buffs (minutes left)
    uint16_t hbRested = 0, hbCaffeine = 0, hbMuse = 0, mcAuto = 0;
    // Apple Valley extras
    uint8_t greenhouse = 0, ghDay = 0, trees = 0, treeAge[6] = {}, pickedDay = 0, danceDay = 0, wed = 0, pad2 = 0;
    uint16_t apples = 0, cider = 0, ghVeg = 0;
    // household control panel: per-Sim autonomy (level + 1; 0 = default)
    uint8_t simAuto[128] = {};
    // settings
    uint8_t toastSp = 1, textSp = 1, popFreq = 2, wxWear = 1;
    int16_t wxPrev = -1; uint8_t wxOn = 0, pad3 = 0;
    // holiday fun (day the thing was done)
    uint16_t hGift = 0xFFFF, hDinner = 0xFFFF, hCostume = 0xFFFF, hNY = 0xFFFF;
    uint8_t resolution = 0, resDays = 0, pad4[2] = {};
} S;

Sim& me() { return MC(); }
bool pay(int c) { if (G.money < c) { toast("Not enough money ($" + std::to_string(c) + ")."); return false; } G.money -= c; return true; }
void need(int n, float d) { Sim& s = me(); s.needs[n] = std::max(0.f, std::min(100.f, s.needs[n] + d)); }
std::string cash(int v) { return "$" + std::to_string(v); }
std::string first(int i) { const std::string& n = G.sims[i].name; return n.substr(0, n.find(' ')); }
std::string last() { const std::string& n = me().name; const size_t p = n.find(' '); return p == std::string::npos ? "" : n.substr(p + 1); }
int household() { int n = 0; for (const Sim& s : G.sims) if (s.household) n++; return n; }
int ownedOutfits() { int n = 0; for (int k = 0; k < NUM_OUTFITS; k++) n += ownsOutfit(k); return n; }

// ===================================================================== vacation homes
struct VH { const char* name; int dest; int price; int rent; const char* event; const char* eventText; };
const VH VHS[3] = {{"Beach House", 0, 45000, 60, "Beach Bonfire", "S'mores, a guitar and the whole cove singing under the stars."},
                   {"Ski Cabin", 1, 40000, 50, "Ski Lodge Party", "Cocoa, a crackling fire and way too much apres-ski dancing."},
                   {"Belle Ville Flat", 2, 55000, 80, "Rooftop Wine & Cheese Night", "Fairy lights, tiny cheeses and the whole city glittering."}};
const char* SEASON_DECOR[4] = {"spring florals", "summer seashells", "fall pumpkins", "winter twinkle lights"};

// ===================================================================== attributes & dreams
const char* ATTR_N[3] = {"Strength", "Brains", "Charm"};
const char* L3ACT[3][5] = {{"Push-Ups", "Squats", "Morning Jog", "Lift Weights", "Yoga Flow"},
                         {"Read a Book", "Crossword", "Watch the News", "Study Session", "Play Chess"},
                         {"Practice Smiling in the Mirror", "Give a Compliment", "Tell Jokes", "Small Talk", "Dance Practice"}};
const int ACT_SKILL[3] = {SK_FITNESS, SK_LOGIC, SK_CHARISMA};
struct Dream { const char* name; const char* how; };
const Dream DREAMS[6] = {{"Bestselling Author", "Brains 6 + write 10 chapters"}, {"Mayor of Maple Valley", "Win a mayoral election"},
                         {"Fashion Mogul", "Charm 5, own 50 outfits, $20,000"}, {"Fitness Icon", "Strength 8 + Fitness skill 8"},
                         {"Influencer Royalty", "25,000 followers + 10 GRWM videos"}, {"Family Matriarch/Patriarch", "Household of 5 + 3 family recipes"}};
bool dreamMet(int d) {
    switch (d) {
    case 0: return S.attr[1] >= 6 && S.chapters >= 10;
    case 1: return S.mayor;
    case 2: return S.attr[2] >= 5 && ownedOutfits() >= 50 && G.money >= 20000;
    case 3: return S.attr[0] >= 8 && skillLevel(me(), SK_FITNESS) >= 8;
    case 4: return G.followers >= 25000 && S.grwms >= 10;
    case 5: return household() >= 5 && S.nRecipes >= 3;
    }
    return false;
}

// ===================================================================== TV
struct Channel { const char* name; int skill; bool premium; };
const Channel TV[8] = {{"Cook With Mama Cass", SK_COOKING, false}, {"Sweat Squad Workout", SK_FITNESS, false}, {"Runway Report", SK_STYLE, false},
                       {"Brain Buster Quiz Show", SK_LOGIC, false}, {"Late Night Laughs", SK_CHARISMA, false}, {"Paint Along", SK_CREATIVITY, false},
                       {"Premium: Celebrity Chef Masterclass", SK_COOKING, true}, {"Premium: Glow-Up Academy", SK_STYLE, true}};

// ===================================================================== badges
struct Badge { const char* name; const char* how; };
const Badge BADGES[16] = {{"Globetrotter", "Take 3 trips"}, {"Second Home", "Own a vacation home"}, {"Madam/Mister Mayor", "Win an election"},
                          {"Iron Body", "Strength 10"}, {"Big Brain", "Brains 10"}, {"Charmer", "Charm 10"}, {"Dream Achieved", "Complete a dream"},
                          {"Influencer", "10,000 followers"}, {"Fashion Collector", "Own 60 outfits"}, {"Family Chef", "3 family recipes"},
                          {"Peacemaker", "Settle 3 neighbor feuds"}, {"TV Junkie", "Watch 20 shows"}, {"Rich!", "$100,000"},
                          {"Big Family", "Household of 6"}, {"Thrift Queen", "3 rare thrift finds"}, {"Final Rose", "Win the dating show"}};
bool badgeMet(int b) {
    switch (b) {
    case 0: return G.trips >= 3; case 1: return S.vhOwn[0] || S.vhOwn[1] || S.vhOwn[2]; case 2: return S.mayor;
    case 3: return S.attr[0] >= 10; case 4: return S.attr[1] >= 10; case 5: return S.attr[2] >= 10; case 6: return S.dreamsDone > 0;
    case 7: return G.followers >= 10000; case 8: return ownedOutfits() >= 60; case 9: return S.nRecipes >= 3;
    case 10: return S.feudsSolved >= 3; case 11: return S.tvWatched >= 20; case 12: return G.money >= 100000;
    case 13: return household() >= 6; case 14: return S.rareFinds >= 3; case 15: return S.showsWon > 0;
    }
    return false;
}
void checkBadges() {
    for (int b = 0; b < 16; b++) if (!(S.badges >> b & 1) && badgeMet(b)) {
        S.badges |= 1u << b; toast(std::string("BADGE EARNED: ") + BADGES[b].name + "!"); lvlPrestige(3, "badge");
        addMemory(MEM_GOAL, G.mc, -1, std::string("Earned the ") + BADGES[b].name + " badge.");
    }
}

// ===================================================================== family helpers
const char* CRAVE[8] = {"pickles & ice cream", "hot Cheetos", "a whole lemon", "chocolate milk", "spicy noodles", "a giant burrito", "watermelon", "fries dipped in a milkshake"};
const char* RECIPE_A[8] = {"Grandma's", "Sunday", "Secret", "Midnight", "Holiday", "Lucky", "Famous", "Cozy"};
const char* RECIPE_B[8] = {"Mac & Cheese", "Peach Cobbler", "Gumbo", "Sweet Potato Pie", "Banana Pudding", "Lasagna", "Tamales", "Cinnamon Rolls"};
int nthKid(int k) { int n = 0; for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household && i != G.mc && G.sims[i].stage >= ST_TODDLER && G.sims[i].stage <= ST_TEEN) { if (n == k) return i; n++; } return -1; }
bool isParent() { return me().stage >= ST_YA && nthKid(0) >= 0; }
int adopt(Stage st) {
    Sim k = randomSim(st, irand(2)); k.household = true;
    const std::string fn = k.name.substr(0, k.name.find(' ')); k.name = fn + (last().empty() ? "" : " " + last());
    k.bio = "adopted - and SO loved";
    G.sims.push_back(k); const int idx = (int)G.sims.size() - 1;
    for (int i = 0; i < idx; i++) if (G.sims[i].household) { Rel& r = rel(i, idx); r.met = true; r.fr = i == G.mc ? 40 : 20; r.status = RS_FAMILY; }
    Actor a; a.kind = 0; a.id = (uint16_t)idx; a.x = 120.f + irand(160); a.tx = a.x; a.y = 212;
    G.rooms[G.house][st == ST_INFANT ? R_NURSERY : R_LIVING].actors.push_back(a);
    return idx;
}

// ===================================================================== town helpers
int randomTownie(bool romance) {
    for (int t = 0; t < 60; t++) { const int i = irand((int)G.sims.size()); if (i == G.mc || G.sims[i].household || G.sims[i].stage < ST_YA) continue; if (romance && !canRomance(G.mc, i)) continue; return i; }
    return -1;
}
const char* FEUD[4] = {"Your neighbor blasts music at 2 AM.", "Someone keeps 'borrowing' your packages.", "The HOA says your flamingos violate the bylaws.", "Your neighbor's dog keeps digging up your yard."};
int dsCont[3] = {-1, -1, -1}, dsScore[3] = {}, dsRound = 0, dsPref[3][3] = {};
const char* DSQ[3][4] = {{"What's your ideal date?", "Picnic in the park", "Fancy dinner", "Theme park day"},
                         {"What do you do on a Sunday?", "Sleep in", "Brunch with friends", "Hit the gym"},
                         {"Pick a vacation:", "Beach", "Ski trip", "City break"}};

// ===================================================================== style helpers
int dropOutfit(int k) {   // 3 outfits per season, rotating through the grown-up wardrobe
    const int s = seasonOf(G.day), yr = G.day / 28;
    int found = 0;
    for (int o = 9 + ((s * 7 + yr * 3) % 40); o < NUM_OUTFITS + 60; o++) {
        const int i = o % NUM_OUTFITS;
        if (i < 9 || !outfitFits(i, ST_YA, me().look.guy)) continue;
        if (found == k) return i;
        found++;
    }
    return 9 + k;
}
int hhAt(int k) { int n = 0; for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household) { if (n == k) return i; n++; } return -1; }
}  // namespace

// ===================================================================== hub
LxMenu l3App() {
    LxMenu m;
    { static const char* AL[3] = {"Off", "Medium", "Full"}; m.push_back({std::string("Selected Sim Autonomy: ") + AL[l3AutoOf(G.mc)] + " (tap to change)", 29002}); }
    m.push_back({"Household Control Panel", 29600});
    { const char* hn = nullptr; const bool ny = G.day % 28 == 27; m.push_back({lxHolidayToday(G.day, &hn) >= 0 ? std::string("Holiday Fun: ") + hn + " is TODAY!" : ny ? "Holiday Fun: New Year's Eve TONIGHT!" : "Holiday Fun", 29400}); }
    m.push_back({"Buffs & Moodlets", 29900});
    m.push_back({"Vacation Homes", 29010});
    m.push_back({"Attributes (Strength / Brains / Charm)", 29100});
    m.push_back({S.dream ? std::string("Dream: ") + DREAMS[S.dream - 1].name : "Pick a Dream", 29150});
    m.push_back({"Watch TV (skill channels)", 29200});
    m.push_back({"Badges (" + std::to_string(__builtin_popcount(S.badges)) + "/16)", 29250});
    m.push_back({"Family Extras", 29300});
    m.push_back({"Town Life", 29500});
    if (lvlAllows(LF_FARM)) m.push_back({"Apple Valley Extras (orchard, greenhouse, market...)", 29800});
    m.push_back({"Style & Content", 29700});
    m.push_back({"Settings & Backups", 29950});
    return m;
}

LxMenu l3Option(int id) {
    LxMenu m; char b[120]; Sim& s = me();
    if (id == 29000) return l3App();
    if (id == 29002) { if (G.mc >= 0 && G.mc < 128) S.simAuto[G.mc] = (uint8_t)((l3AutoOf(G.mc) + 1) % 3 + 1); S.mcAuto = (uint16_t)l3AutoOf(G.mc);
        static const char* AT[3] = {"Autonomy OFF: your Sim waits for you (they'll only act alone if a need gets critical).",
                                    "Autonomy MEDIUM: your Sim handles needs that get low. Tap anything to take over.",
                                    "Autonomy FULL: your Sim eats, sleeps, showers, chats, has fun and wanders on their own. Tap anything to take over."};
        toast(AT[S.mcAuto]); return l3App(); }
    // ------------------------------------------------------------- vacation homes
    if (id == 29010) {
        for (int k = 0; k < 3; k++) {
            const VH& v = VHS[k];
            if (!S.vhOwn[k]) { snprintf(b, sizeof b, "Buy a %s in %s ($%d)", v.name, k == 0 ? "Coral Cove" : k == 1 ? "Frostpeak" : "Belle Ville", v.price); m.push_back({b, 29020 + k}); continue; }
            snprintf(b, sizeof b, "%s: Family Weekend (2 days, free)", v.name); m.push_back({b, 29030 + k});
            snprintf(b, sizeof b, "%s: Rent It Out [%s] ($%d/day)", v.name, S.vhRent[k] ? "ON" : "OFF", v.rent); m.push_back({b, 29040 + k});
            snprintf(b, sizeof b, "%s: Decorate (%s, $50)", v.name, SEASON_DECOR[seasonOf(G.day)]); m.push_back({b, 29050 + k});
            if (vacationDest() == v.dest + 1) { snprintf(b, sizeof b, "Local Event: %s", v.event); m.push_back({b, 29060 + k}); }
        }
        return m;
    }
    if (id >= 29020 && id < 29023) { const int k = id - 29020; if (s.stage < ST_YA) { toast("You need to be a young adult to buy a home."); return {}; }
        if (!pay(VHS[k].price)) return {}; S.vhOwn[k] = 1; guPhoto(std::string("Keys to our ") + VHS[k].name); addMoodlet(s, "Vacation Home Owner", EM_HAPPY, 3, 1440);
        toast(std::string("You own a ") + VHS[k].name + "! Family weekends there are free now."); checkBadges(); return {}; }
    if (id >= 29030 && id < 29033) { const int k = id - 29030; if (G.vacDest) { toast("You're already on a trip!"); return {}; }
        if (S.vhRent[k]) { S.vhRent[k] = 0; toast("You cancelled the renters for the weekend."); }
        for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household) addMoodlet(G.sims[i], "Family Getaway", EM_HAPPY, 2, 2880);
        travelStartOwned(VHS[k].dest, 2, VHS[k].name); return {}; }
    if (id >= 29040 && id < 29043) { const int k = id - 29040; if (S.vhBroken[k]) { toast("Fix it up first - the last renters wrecked it."); return {}; }
        S.vhRent[k] = !S.vhRent[k]; toast(S.vhRent[k] ? "Listed! Rent comes in every day you're not staying there." : "Rental listing taken down."); return l3Option(29010); }
    if (id >= 29050 && id < 29053) { const int k = id - 29050; if (!pay(50)) return {}; S.vhDecor[k] = (uint8_t)(seasonOf(G.day) + 1);
        if (S.vhBroken[k]) { S.vhBroken[k] = 0; toast("Repaired AND decorated - good as new."); } else toast(std::string("Decorated with ") + SEASON_DECOR[seasonOf(G.day)] + " - renters pay more for in-season decor!"); return {}; }
    if (id >= 29060 && id < 29063) { const int k = id - 29060; need(N_FUN, 30); need(N_SOCIAL, 25); addMoodlet(s, VHS[k].event, EM_HAPPY, 2, 360); guPhoto(VHS[k].event);
        toast(std::string(VHS[k].event) + "! " + VHS[k].eventText); return {}; }
    // ------------------------------------------------------------- attributes
    if (id == 29100) {
        if (S.actDay != G.day) { S.actDay = (uint16_t)G.day; for (auto& a : S.actBits) a = 0; }
        for (int a = 0; a < 3; a++) { snprintf(b, sizeof b, "%s: level %d/10  (today %d/5 activities)", ATTR_N[a], S.attr[a], __builtin_popcount(S.actBits[a])); m.push_back({b, 29110 + a}); }
        m.push_back({"Do all 5 of an attribute's activities in one day to level it up.", 29999});
        return m;
    }
    if (id >= 29110 && id < 29113) { const int a = id - 29110;
        for (int k = 0; k < 5; k++) m.push_back({std::string(S.actBits[a] >> k & 1 ? "[X] " : "[ ] ") + L3ACT[a][k], 29120 + a * 5 + k}); return m; }
    if (id >= 29120 && id < 29135) { const int a = (id - 29120) / 5, k = (id - 29120) % 5;
        if (S.actDay != G.day) { S.actDay = (uint16_t)G.day; for (auto& x : S.actBits) x = 0; }
        if (S.actBits[a] >> k & 1) { toast("Already did that one today."); return {}; }
        if (s.needs[N_ENERGY] < 10) { toast("Too tired - rest first."); return {}; }
        S.actBits[a] |= (uint8_t)(1 << k); need(N_ENERGY, -6); skillGain(s, ACT_SKILL[a], 1.5f); if (a == 2) need(N_SOCIAL, 6); if (a == 0) need(N_HYGIENE, -8);
        if (S.actBits[a] == 31) { if (S.attr[a] < 10) { S.attr[a]++; toast(std::string(ATTR_N[a]) + " is now level " + std::to_string(S.attr[a]) + "!"); lvlPrestige(3, ATTR_N[a]); addMoodlet(s, "Stronger Every Day", EM_CONFIDENT, 2, 480); } else toast(std::string(ATTR_N[a]) + " is maxed! Legend."); checkBadges(); }
        else toast(std::string(L3ACT[a][k]) + " done (" + std::to_string(__builtin_popcount(S.actBits[a])) + "/5 today).");
        return l3Option(29110 + a); }
    // ------------------------------------------------------------- dreams
    if (id == 29150) {
        if (S.dream) { const int d = S.dream - 1; snprintf(b, sizeof b, "%s - %s", DREAMS[d].name, DREAMS[d].how); m.push_back({b, 29999});
            m.push_back({dreamMet(d) ? "Claim Your Dream! ($5,000)" : "(Not there yet - keep going!)", dreamMet(d) ? 29160 : 29999});
            if (d == 0) { snprintf(b, sizeof b, "Write a Chapter (%d/10, needs Brains 3)", S.chapters); m.push_back({b, 29161}); }
            m.push_back({"Change Dream", 29162}); return m; }
        for (int d = 0; d < 6; d++) { snprintf(b, sizeof b, "%s (%s)", DREAMS[d].name, DREAMS[d].how); m.push_back({b, 29170 + d}); } return m; }
    if (id >= 29170 && id < 29176) { S.dream = (uint8_t)(id - 29170 + 1); toast(std::string("New dream: ") + DREAMS[S.dream - 1].name + "! Track it in Life Extras."); return {}; }
    if (id == 29162) { S.dream = 0; return l3Option(29150); }
    if (id == 29161) { if (S.attr[1] < 3) { toast("You need Brains 3 to write a chapter."); return {}; } if (S.chapters < 255) S.chapters++; need(N_ENERGY, -10); skillGain(s, SK_CREATIVITY, 2.f); toast("Chapter " + std::to_string(S.chapters) + " written. The plot thickens..."); return {}; }
    if (id == 29160) { if (!S.dream || !dreamMet(S.dream - 1)) return {}; G.money += 5000; S.dreamsDone++; lvlPrestige(10, "dream achieved");
        addMemory(MEM_GOAL, G.mc, -1, std::string("Achieved the dream: ") + DREAMS[S.dream - 1].name + "!"); guPhoto(DREAMS[S.dream - 1].name);
        addMoodlet(s, "Dream Come True", EM_CONFIDENT, 4, 2880); toast(std::string("DREAM ACHIEVED: ") + DREAMS[S.dream - 1].name + "! +$5,000"); S.dream = 0; checkBadges(); return {}; }
    // ------------------------------------------------------------- TV
    if (id == 29200) {
        const bool prem = S.premiumUntil > G.day;
        for (int c = 0; c < 8; c++) { if (TV[c].premium && !prem) continue; m.push_back({std::string(TV[c].name) + " (" + SKILL_NAMES[TV[c].skill] + ")", 29210 + c}); }
        m.push_back({prem ? "Premium package: active" : "Buy Premium Channels ($15/week - double skill gain)", prem ? 29999 : 29220}); return m; }
    if (id >= 29210 && id < 29218) { const Channel& c = TV[id - 29210]; need(N_FUN, 12); need(N_ENERGY, -3); skillGain(s, c.skill, c.premium ? 3.f : 1.5f);
        if (S.tvWatched < 65535) S.tvWatched++; toast(std::string("You watched ") + c.name + "."); checkBadges(); return {}; }
    if (id == 29220) { if (!pay(15)) return {}; S.premiumUntil = (uint16_t)(G.day + 7); toast("Premium channels unlocked for a week!"); return l3Option(29200); }
    // ------------------------------------------------------------- badges
    if (id == 29250) { checkBadges(); for (int k = 0; k < 16; k++) { snprintf(b, sizeof b, "%s %s - %s", S.badges >> k & 1 ? "[BADGE]" : "[  ]", BADGES[k].name, BADGES[k].how); m.push_back({b, 29999}); } return m; }
    // ------------------------------------------------------------- family
    if (id == 29300) {
        if (s.pregnant) { snprintf(b, sizeof b, "Satisfy Craving: %s ($5)%s", CRAVE[S.craving % 8], S.cravingDone ? " - done!" : ""); m.push_back({b, 29310});
                          if (!S.showerDone) m.push_back({"Throw a Baby Shower ($80)", 29311}); }
        if (s.stage >= ST_YA) m.push_back({"Adoption Agency", 29320});
        if (s.stage == ST_TEEN) { snprintf(b, sizeof b, "Learner's Permit: Practice Drive (%d done)", S.practice); m.push_back({b, 29330}); m.push_back({"Take the Car Out", 29331}); m.push_back({"Beg for a Later Curfew", 29332}); }
        if (isParent()) { snprintf(b, sizeof b, "Set the Teen Curfew (now %d:00)", S.curfew > 12 ? S.curfew - 12 : S.curfew); m.push_back({b, 29333}); }
        if (nthKid(1) >= 0) m.push_back({"Hand-Me-Downs (pass clothes to a younger kid)", 29340});
        m.push_back({"Family Cookbook (" + std::to_string(S.nRecipes) + " recipes)", 29350});
        return m; }
    if (id == 29310) { if (S.cravingDone) { toast("Craving already satisfied today."); return {}; } if (!pay(5)) return {}; S.cravingDone = 1; need(N_HUNGER, 25);
        addMoodlet(s, "Craving Satisfied", EM_HAPPY, 2, 360); toast(std::string("You devoured ") + CRAVE[S.craving % 8] + ". The baby approves."); return {}; }
    if (id == 29311) { if (S.showerDone || !pay(80)) return {}; S.showerDone = 1; const int gifts = 150 + irand(200); G.money += gifts;
        for (int k = 0; k < 4; k++) { const int t = randomTownie(false); if (t >= 0) changeRel(G.mc, t, 6, 0); }
        guPhoto("Baby shower"); addMoodlet(s, "Showered with Love", EM_HAPPY, 3, 1440); toast("Baby shower! Onesies, a diaper cake and " + cash(gifts) + " in gift cards."); return {}; }
    if (id == 29320) { m.push_back({"Adopt a Baby ($500)", 29321}); m.push_back({"Adopt a Toddler ($500)", 29322}); m.push_back({"Adopt a Child ($500)", 29323}); m.push_back({"Adopt a Teen ($500)", 29324}); return m; }
    if (id >= 29321 && id < 29325) { if (household() >= 8) { toast("Your house is full (8 max)."); return {}; } if (!pay(500)) return {};
        static const Stage ST[4] = {ST_INFANT, ST_TODDLER, ST_CHILD, ST_TEEN};
        const int k = adopt(ST[id - 29321]); S.adopted++; guPhoto("Welcome home, " + first(k));
        addMemory(MEM_FAMILY, G.mc, k, "Adopted " + G.sims[k].name + " - our family grew today."); addMoodlet(s, "New Family Member", EM_HAPPY, 3, 2880);
        toast("Welcome home, " + first(k) + "! The paperwork is done - you're family now."); checkBadges(); return {}; }
    if (id == 29330) { if (s.needs[N_ENERGY] < 10) { toast("Too tired to drive."); return {}; } S.practice++; need(N_FUN, 8); skillGain(s, SK_LOGIC, 0.5f);
        static const char* PD[4] = {"Mom grabbed the dashboard the WHOLE time.", "You parallel parked! (On the third try.)", "Dad's foot kept hitting an invisible brake.", "Smooth ride. Your parent is proud (and alive)."};
        toast(std::string("Practice drive: ") + PD[irand(4)]); return {}; }
    if (id == 29331) { need(N_FUN, 15);
        if (irand(100) < (S.practice >= 5 ? 6 : 18)) { S.crashes++; const int c = 300; G.money = G.money > c ? G.money - c : 0; addMoodlet(s, "Fender Bender", EM_TENSE, 2, 720);
            for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household && G.sims[i].stage >= ST_YA) changeRel(G.mc, i, -4, 0);
            toast("CRUNCH. You backed into the mailbox. $300 in repairs and a VERY long talk."); }
        else toast(S.practice >= 5 ? "A smooth drive with the windows down. Freedom!" : "You drove to the mall and back. Barely legal, totally iconic."); return {}; }
    if (id == 29332) { if (irand(2)) { S.curfew = (uint8_t)std::min(24, S.curfew + 1); toast("They said yes! Curfew moved an hour later."); } else toast("\"Nice try.\" Curfew stays put."); return {}; }
    if (id == 29333) { S.curfew = (uint8_t)(S.curfew >= 24 ? 20 : S.curfew + 1); toast("Teen curfew set to " + std::to_string(S.curfew > 12 ? S.curfew - 12 : S.curfew) + (S.curfew >= 24 ? " AM." : " PM.")); return l3Option(29300); }
    if (id == 29340) { int older = -1, younger = -1;
        for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household && i != G.mc && G.sims[i].stage >= ST_CHILD && G.sims[i].stage <= ST_TEEN) {
            if (older < 0 || G.sims[i].stage > G.sims[older].stage) older = i; }
        for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household && i != G.mc && i != older && G.sims[i].stage >= ST_TODDLER && older >= 0 && G.sims[i].stage < G.sims[older].stage) { younger = i; break; }
        if (older < 0 || younger < 0) { toast("You need an older and a younger kid for hand-me-downs."); return {}; }
        if (outfitFits(G.sims[older].outfit, G.sims[younger].stage, G.sims[younger].look.guy)) G.sims[younger].outfit = G.sims[older].outfit;
        S.handMeDowns++; changeRel(older, younger, irand(2) ? 3 : -2, 0);
        toast(irand(2) ? first(younger) + " LOVES " + first(older) + "'s old clothes. Instant style upgrade!" : first(younger) + ": \"Why do I always get the old stuff?!\" (" + first(older) + " is smug.)"); return {}; }
    if (id == 29350) {
        for (int r = 0; r < S.nRecipes; r++) { snprintf(b, sizeof b, "Cook %s (gen %d)", S.recipes[r].name, S.recipes[r].gen); m.push_back({b, 29360 + r}); }
        if (S.nRecipes < 10) m.push_back({"Create a New Family Recipe (Cooking 3)", 29351});
        if (S.nRecipes && nthKid(0) >= 0) m.push_back({"Teach the Kids a Recipe", 29352});
        if (!S.nRecipes) m.push_back({"(No recipes yet - start the family cookbook!)", 29999});
        return m; }
    if (id == 29351) { if (skillLevel(s, SK_COOKING) < 3) { toast("You need Cooking level 3 to invent a family recipe."); return {}; } if (S.nRecipes >= 10) return {};
        Recipe& r = S.recipes[S.nRecipes++]; snprintf(r.name, sizeof r.name, "%s %s", RECIPE_A[irand(8)], RECIPE_B[irand(8)]); r.gen = (uint8_t)std::max(1, dyGeneration());
        skillGain(s, SK_COOKING, 3.f); toast(std::string("New family recipe: ") + r.name + "! It'll be passed down for generations."); checkBadges(); return {}; }
    if (id == 29352) { const int k = nthKid(irand(3)) >= 0 ? nthKid(irand(3)) : nthKid(0); if (k < 0) return {}; skillGain(G.sims[k], SK_COOKING, 4.f); changeRel(G.mc, k, 5, 0);
        toast("You taught " + first(k) + " the " + S.recipes[irand(S.nRecipes)].name + ". Flour everywhere, hearts full."); return {}; }
    if (id >= 29360 && id < 29370) { const int r = id - 29360; if (r >= S.nRecipes) return {};
        for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household) { G.sims[i].needs[N_HUNGER] = 100; addMoodlet(G.sims[i], "Taste of Home", EM_HAPPY, 2, 480); if (i != G.mc) changeRel(G.mc, i, 3, 0); }
        S.hbMuse = 240; toast(std::string("Everyone gathers for ") + S.recipes[r].name + ". It tastes like home."); return {}; }
    // ------------------------------------------------------------- town life
    if (id == 29500) {
        if (s.stage >= ST_ADULT || s.stage == ST_YA) {
            if (S.mayor) m.push_back({"You're the Mayor! Cut a Ribbon / Give a Speech", 29510});
            else if (S.campaign) { snprintf(b, sizeof b, "Campaign: support %d%% - election in %d day(s)", S.support, std::max(0, (int)S.campEnd - (int)G.day)); m.push_back({b, 29999});
                static const char* CA[5] = {"Kiss Babies & Shake Hands", "Put Up Posters ($50)", "Hold a Rally", "Mayoral Debate", "Bake Sale Fundraiser"};
                for (int k = 0; k < 5; k++) m.push_back({std::string(S.campDone >> k & 1 ? "[done today] " : "") + CA[k], 29520 + k}); }
            else m.push_back({"Run for Mayor of Maple Valley", 29511});
        }
        if (S.feud) { snprintf(b, sizeof b, "Neighbor Feud: %s", FEUD[(S.feud - 1) % 4]); m.push_back({b, 29999});
            m.push_back({"Talk It Out", 29530}); m.push_back({"Report to the HOA ($20)", 29531}); m.push_back({"Prank Them Back", 29532}); m.push_back({"Bring a Peace Pie", 29533}); }
        const int se = seasonOf(G.day); static const char* PAR[4] = {"Spring Flower Parade", "Summer Fireworks Show", "Harvest Parade", "Holiday Lights Parade"};
        if (G.day % 7 >= 5) m.push_back({std::string(PAR[se]) + (S.paradeDay == G.day ? " (seen today)" : ""), 29540});
        else m.push_back({std::string("Next: ") + PAR[se] + " (weekends)", 29999});
        if (s.stage >= ST_YA) m.push_back({"Glam Live: \"Rose Garden\" Dating Show", 29550});
        return m; }
    if (id == 29511) { S.campaign = 1; S.campEnd = (uint16_t)(G.day + 7); S.support = (uint16_t)(30 + S.attr[2] * 3); S.campDone = 0; toast("You're running for Mayor! 7 days to win the town over."); return l3Option(29500); }
    if (id >= 29520 && id < 29525) { const int k = id - 29520; if (!S.campaign) return {}; if (S.campDone >> k & 1) { toast("Already did that today."); return {}; }
        int g = 0; switch (k) {
        case 0: g = 3 + irand(3); need(N_SOCIAL, 10); toast("You kissed 11 babies and shook 40 hands."); break;
        case 1: if (!pay(50)) return {}; g = 6; toast("Your face is on every lamppost. Iconic."); break;
        case 2: g = 5 + irand(5); need(N_ENERGY, -15); toast("RALLY! The crowd chants your name."); break;
        case 3: { const int r = irand(10) + S.attr[2] + S.attr[1]; g = r >= 10 ? 10 : r >= 6 ? 3 : -5; toast(g >= 10 ? "You DESTROYED the debate. Clip goes viral." : g > 0 ? "Solid debate. A few good zingers." : "You froze on live TV. Oof."); break; }
        case 4: g = 4; G.money += 60; toast("Bake sale raised $60 and your approval rating."); break; }
        S.campDone |= (uint8_t)(1 << k); S.support = (uint16_t)std::max(0, std::min(100, (int)S.support + g)); return l3Option(29500); }
    if (id == 29510) { need(N_FUN, 10); G.money += 25; static const char* MY[4] = {"You cut the ribbon on a new playground.", "You gave a speech. Nobody listened. Still iconic.", "You declared it National Bunny Day.", "You opened the farmers market with a giant pair of scissors."};
        toast(std::string("Mayor duties: ") + MY[irand(4)]); return {}; }
    if (id >= 29530 && id < 29534) { if (!S.feud) return {}; const int w = S.feudWith;
        switch (id) {
        case 29530: if (irand(3)) { toast("You talked it out like adults. Truce!"); if (w >= 0) changeRel(G.mc, w, 6, 0); S.feud = 0; S.feudsSolved++; } else toast("They slammed the door. Round two later."); break;
        case 29531: if (!pay(20)) return {}; toast("The HOA sided with you. Your neighbor is FUMING."); if (w >= 0) changeRel(G.mc, w, -8, 0); S.feud = 0; S.feudsSolved++; break;
        case 29532: need(N_FUN, 20); if (w >= 0) changeRel(G.mc, w, -6, 0); toast(irand(2) ? "You saran-wrapped their car. They're plotting revenge." : "Glitter bomb in their mailbox. It's war now."); break;
        case 29533: if (w >= 0) changeRel(G.mc, w, 12, 0); toast("A warm pie on their porch. Tears. Hugs. Feud OVER."); S.feud = 0; S.feudsSolved++; break; }
        checkBadges(); return {}; }
    if (id == 29540) { if (S.paradeDay == G.day) { toast("You already caught today's show."); return {}; } S.paradeDay = (uint16_t)G.day;
        need(N_FUN, 25); need(N_SOCIAL, 10); for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household) addMoodlet(G.sims[i], "Parade Day", EM_HAPPY, 2, 360);
        static const char* PT[4] = {"Flower floats and a marching band!", "BOOM - the sky lit up pink and gold!", "Hay wagons, pie floats and the Apple Valley llama!", "Twinkling floats, carolers and fake snow!"};
        guPhoto("Watching the parade"); toast(PT[seasonOf(G.day)]); return {}; }
    if (id == 29550) { int n = 0; for (int k = 0; k < 3; k++) { dsCont[k] = randomTownie(true); dsScore[k] = 0; for (int q = 0; q < 3; q++) dsPref[k][q] = irand(3); if (dsCont[k] >= 0) n++; }
        if (n < 3) { toast("Not enough singles in town for the show right now."); return {}; }
        dsRound = 0; toast("Welcome to ROSE GARDEN! Three singles, three questions, one rose."); return l3Option(29560); }
    if (id >= 29560 && id < 29573 && (dsCont[0] < 0 || dsCont[1] < 0 || dsCont[2] < 0)) return {};
    if (id >= 29561 && id < 29564 && dsRound >= 3) return l3Option(29560);
    if (id >= 29570 && id < 29573 && dsRound < 3) return l3Option(29560);
    if (id == 29560) { if (dsRound >= 3) { for (int k = 0; k < 3; k++) { snprintf(b, sizeof b, "Give the Final Rose to %s", G.sims[dsCont[k]].name.c_str()); m.push_back({b, 29570 + k}); } return m; }
        snprintf(b, sizeof b, "Round %d: \"%s\"", dsRound + 1, DSQ[dsRound][0]); m.push_back({b, 29999});
        for (int a = 0; a < 3; a++) m.push_back({DSQ[dsRound][a + 1], 29561 + a}); return m; }
    if (id >= 29561 && id < 29564) { const int a = id - 29561; std::string fav;
        for (int k = 0; k < 3; k++) if (dsCont[k] >= 0 && dsPref[k][dsRound] == a) { dsScore[k]++; fav += (fav.empty() ? "" : " & ") + first(dsCont[k]); }
        toast(fav.empty() ? "Awkward silence. Nobody vibed with that answer." : fav + " lit up at that answer!"); dsRound++; return l3Option(29560); }
    if (id >= 29570 && id < 29573) { const int k = id - 29570, who = dsCont[k]; if (who < 0) return {};
        int best = 0; for (int j = 1; j < 3; j++) if (dsScore[j] > dsScore[best]) best = j;
        if (dsScore[k] >= dsScore[best]) { Rel& r = rel(G.mc, who); r.met = true; changeRel(G.mc, who, 15, 25); if (r.status == RS_NONE) r.status = RS_DATING;
            G.followers += 2000; S.showsWon++; guPhoto("Final rose with " + first(who)); addMoodlet(me(), "Final Rose", EM_FLIRTY, 3, 1440);
            toast(first(who) + " accepts the rose! The audience SCREAMS. +2,000 followers. You're dating now!"); }
        else { changeRel(G.mc, who, -5, 0); G.followers += 500; toast(first(who) + " hesitated... and said \"I think we're better as friends.\" Brutal. (+500 followers for the drama.)"); }
        for (auto& c : dsCont) c = -1; checkBadges(); return {}; }
    // ------------------------------------------------------------- style & content
    if (id == 29700) {
        static const char* SN[4] = {"Spring", "Summer", "Fall", "Winter"};
        snprintf(b, sizeof b, "-- %s Fashion Drop (limited) --", SN[seasonOf(G.day)]); m.push_back({b, 29999});
        for (int k = 0; k < 3; k++) { const int o = dropOutfit(k); snprintf(b, sizeof b, "%s%s%s", OUTFITS[o].name, ownsOutfit(o) ? " (owned)" : " ($90)", ""); m.push_back({b, ownsOutfit(o) ? 29999 : 29710 + k}); }
        m.push_back({S.grwmDay == G.day ? "Get Ready With Me video (posted today)" : "Film a Get-Ready-With-Me (TikTak)", 29720});
        m.push_back({"Thrift Store Dig ($10)", 29730});
        return m; }
    if (id >= 29710 && id < 29713) { const int o = dropOutfit(id - 29710); if (ownsOutfit(o)) return {}; if (!pay(90)) return {}; giveOutfit(o); S.dropsBought++;
        toast(std::string("Copped the drop: ") + OUTFITS[o].name + "! It's in your closet."); checkBadges(); return {}; }
    if (id == 29720) { if (S.grwmDay == G.day) { toast("One GRWM a day - the algorithm gets suspicious."); return {}; } S.grwmDay = (uint16_t)G.day; S.grwms++;
        const int f = 150 + skillLevel(s, SK_STYLE) * 120 + S.attr[2] * 60 + irand(300); G.followers += (uint32_t)f; skillGain(s, SK_STYLE, 2.f); need(N_FUN, 10);
        static const char* GR[4] = {"\"get ready with me for a date I'm not excited about\"", "\"GRWM: first day of school edition\"", "\"5-minute baddie makeup\"", "\"GRWM while I spill the tea\""};
        toast(std::string("Posted ") + GR[irand(4)] + " - +" + std::to_string(f) + " followers!"); checkBadges(); return {}; }
    if (id == 29730) { if (!pay(10)) return {}; const int r = irand(100);
        if (r < 25) { int pick = -1; for (int t = 0; t < 40 && pick < 0; t++) { const int o = 9 + irand(NUM_OUTFITS - 9); if (!ownsOutfit(o) && outfitFits(o, s.stage, s.look.guy)) pick = o; }
            if (pick >= 0) { giveOutfit(pick); S.rareFinds++; toast(std::string("RARE FIND! A vintage ") + OUTFITS[pick].name + " for $10!!"); checkBadges(); return {}; } }
        if (r < 75) { G.money += 5; toast("Found a cute scarf, resold it for $15. Small wins."); return {}; }
        toast("Just a pile of 2003 cargo shorts. Not today."); return {}; }
    // ------------------------------------------------------------- Apple Valley extras
    if (id == 29800) {
        static const char* IN[4] = {"peas, strawberries, lettuce", "corn, tomatoes, watermelon", "pumpkins, apples, squash", "nothing outdoors - greenhouse only!"};
        snprintf(b, sizeof b, "In season now: %s", IN[seasonOf(G.day)]); m.push_back({b, 29999});
        if (!S.greenhouse) m.push_back({"Build a Greenhouse ($2,000 - grows all year)", 29801});
        else m.push_back({S.ghDay == (uint8_t)(G.day & 0xFF) ? "Greenhouse (tended today)" : "Tend the Greenhouse", 29802});
        snprintf(b, sizeof b, "Orchard: %d/6 apple trees - Plant One ($300)", S.trees); m.push_back({b, S.trees < 6 ? 29803 : 29999});
        if (S.trees) m.push_back({S.pickedDay == (uint8_t)(G.day & 0xFF) ? "Apples picked today" : "Pick Apples", 29804});
        snprintf(b, sizeof b, "Cider Press (5 apples -> 1 cider) [%d apples, %d cider]", S.apples, S.cider); m.push_back({b, 29805});
        snprintf(b, sizeof b, "Farmers Market Stall (%d veggies, %d apples, %d cider)", S.ghVeg, S.apples, S.cider); m.push_back({b, 29810});
        if (G.day % 7 == 5) m.push_back({S.danceDay == (uint8_t)(G.day & 0xFF) ? "Barn Dance (danced today)" : "Saturday Barn Dance!", 29820});
        else m.push_back({"Barn Dance: Saturdays at the Big Red Barn", 29999});
        if (!S.wed) for (int i = 0; i < (int)G.sims.size(); i++) if (i != G.mc && rel(G.mc, i).status == RS_ENGAGED) { m.push_back({"Farm Wedding with " + first(i) + " ($1,500)", 29830}); break; }
        return m; }
    if (id == 29801) { if (s.stage < ST_YA) { toast("Young adults and up can build."); return {}; } if (!pay(2000)) return {}; S.greenhouse = 1; toast("Greenhouse built! Grow veggies in every season."); return {}; }
    if (id == 29802) { if (!S.greenhouse || S.ghDay == (uint8_t)(G.day & 0xFF)) return {}; S.ghDay = (uint8_t)(G.day & 0xFF);
        const int n = 3 + irand(3) + (seasonOf(G.day) == 3 ? 2 : 0); S.ghVeg = (uint16_t)std::min(999, S.ghVeg + n); skillGain(s, SK_FITNESS, 0.5f); need(N_ENERGY, -8);
        toast("Harvested " + std::to_string(n) + " greenhouse veggies."); return {}; }
    if (id == 29803) { if (S.trees >= 6 || !pay(300)) return {}; S.treeAge[S.trees++] = 0; toast("Apple tree planted! It bears fruit after 7 days."); return {}; }
    if (id == 29804) { if (S.pickedDay == (uint8_t)(G.day & 0xFF)) return {}; int n = 0; for (int t = 0; t < S.trees && t < 6; t++) if (S.treeAge[t] >= 7) n += 2 + irand(3);
        if (!n) { toast("Your trees are still too young to fruit."); return {}; } S.pickedDay = (uint8_t)(G.day & 0xFF); S.apples = (uint16_t)std::min(999, S.apples + n); need(N_FUN, 6); toast("Picked " + std::to_string(n) + " apples!"); return {}; }
    if (id == 29805) { if (S.apples < 5) { toast("You need 5 apples."); return {}; } S.apples -= 5; S.cider++; toast("Fresh-pressed cider! Sells for about $40."); return l3Option(29800); }
    if (id == 29810) { if (!S.ghVeg && !S.apples && !S.cider) { toast("Your stall is empty - grow or press something first."); return {}; }
        const int val = S.ghVeg * 6 + S.apples * 4 + S.cider * 40; snprintf(b, sizeof b, "A shopper eyes your stall (worth ~$%d).", val); m.push_back({b, 29999});
        m.push_back({"Fair price (sure sale)", 29811}); m.push_back({"Haggle up +30% (risky)", 29812}); m.push_back({"Haggle up +70% (very risky)", 29813}); return m; }
    if (id >= 29811 && id < 29814) { const int val = S.ghVeg * 6 + S.apples * 4 + S.cider * 40; if (!val) return {};
        const int k = id - 29811; const int pct[3] = {100, 130, 170}, odds[3] = {100, 60 + S.attr[2] * 3, 25 + S.attr[2] * 3};
        if (irand(100) < odds[k]) { const int got = val * pct[k] / 100; G.money += got; S.ghVeg = S.apples = S.cider = 0; skillGain(s, SK_CHARISMA, 1.f); toast("Sold out! +" + cash(got) + (k ? " - you drive a hard bargain." : "")); }
        else toast("\"That's highway robbery!\" The shopper walked off. Try again."); return {}; }
    if (id == 29820) { if (S.danceDay == (uint8_t)(G.day & 0xFF)) return {}; S.danceDay = (uint8_t)(G.day & 0xFF); need(N_FUN, 30); need(N_SOCIAL, 30); need(N_ENERGY, -10);
        addMoodlet(s, "Boot-Scootin'", EM_PLAYFUL, 2, 360); const int t = s.stage >= ST_YA ? randomTownie(true) : -1;
        if (t >= 0) { Rel& r = rel(G.mc, t); r.met = true; changeRel(G.mc, t, 5, 6); toast("Square dancing in the hay! You spun right into " + first(t) + "'s arms."); }
        else toast("Fiddles, hay bales and a line dance that went on for an hour. Yeehaw!"); return {}; }
    if (id == 29830) { int p = -1; for (int i = 0; i < (int)G.sims.size(); i++) if (i != G.mc && rel(G.mc, i).status == RS_ENGAGED) { p = i; break; }
        if (p < 0 || S.wed || !pay(1500)) return {}; rel(G.mc, p).status = RS_MARRIED; milestone(G.mc, MI_MARRIED, p); milestone(p, MI_MARRIED, G.mc); S.wed = 1; changeRel(G.mc, p, 10, 15);
        addMemory(MEM_FAMILY, G.mc, p, "Married " + G.sims[p].name + " in a barn wedding under string lights."); guPhoto("Barn wedding with " + first(p));
        addMoodlet(s, "Just Married!", EM_HAPPY, 4, 2880); toast("A barn wedding! String lights, sunflowers and a pie cake. You married " + first(p) + "!"); return {}; }
    // ------------------------------------------------------------- household control panel
    if (id == 29600) {
        static const char* AL[3] = {"Off", "Medium", "Full"};
        int k = 0;
        for (int i = 0; i < (int)G.sims.size() && k < 10; i++) if (G.sims[i].household) {
            snprintf(b, sizeof b, "%s%s (%s) - Autonomy: %s", i == G.mc ? "[PLAYING] " : "", G.sims[i].name.c_str(), STAGE_NAMES[G.sims[i].stage], AL[l3AutoOf(i)]);
            m.push_back({b, 29610 + k}); k++; }
        m.push_back({"Everyone: Full Autonomy", 29640}); m.push_back({"Everyone: Medium", 29641}); m.push_back({"Everyone: Off", 29642});
        return m; }
    if (id >= 29610 && id < 29620) { const int who = hhAt(id - 29610); if (who < 0) return {};
        static const char* AL[3] = {"Off", "Medium", "Full"};
        snprintf(b, sizeof b, "%s - %s, mood: %s", G.sims[who].name.c_str(), STAGE_NAMES[G.sims[who].stage], who == G.mc ? "you" : "family"); m.push_back({b, 29999});
        if (who != G.mc) m.push_back({"Play as " + first(who), 29620 + (id - 29610)});
        m.push_back({std::string("Autonomy: ") + AL[l3AutoOf(who)] + " (tap to change)", 29630 + (id - 29610)});
        return m; }
    if (id >= 29620 && id < 29630) { const int who = hhAt(id - 29620); if (who < 0 || who == G.mc) return {};
        cancelActivity(); S.wxOn = 0; S.wxPrev = -1; G.mc = who;
        for (int r = 0; r < R_COUNT; r++) for (const Actor& a : G.rooms[G.house][r].actors) if (a.kind == 0 && a.id == who) G.room = r;
        toast("Now playing as " + G.sims[who].name + "!"); return {}; }
    if (id >= 29630 && id < 29640) { const int who = hhAt(id - 29630); if (who < 0 || who >= 128) return {};
        S.simAuto[who] = (uint8_t)((l3AutoOf(who) + 1) % 3 + 1); return l3Option(29610 + (id - 29630)); }
    if (id >= 29640 && id < 29643) { const int lv = 2 - (id - 29640); for (int i = 0; i < (int)G.sims.size() && i < 128; i++) if (G.sims[i].household) S.simAuto[i] = (uint8_t)(lv + 1);
        toast("Everyone's autonomy set."); return l3Option(29600); }
    // ------------------------------------------------------------- holiday fun
    if (id == 29400) {
        const char* hn = nullptr; const int hs = lxHolidayToday(G.day, &hn); const bool ny = G.day % 28 == 27;
        if (hs < 0 && !ny) { m.push_back({"No holiday today. Spooky Day is day 19, Winterfest day 26, New Year's Eve day 28 of each year.", 29999});
            m.push_back({"(Add your own holidays in the Calendar app.)", 29999}); m.push_back({"Throw a Costume Party anyway ($60)", 29403}); return m; }
        if (hn) { snprintf(b, sizeof b, "Happy %s!", hn); m.push_back({b, 29999}); }
        if (hs >= 0) { m.push_back({S.hGift == G.day ? "Gifts exchanged!" : "Exchange Gifts with the Family ($40)", 29401});
                       m.push_back({S.hDinner == G.day ? "Holiday dinner done!" : (S.nRecipes ? "Holiday Dinner (with a family recipe!)" : "Holiday Dinner"), 29402});
                       m.push_back({S.hCostume == G.day ? "Costume party done!" : "Costume Party ($60)", 29403}); }
        if (ny) { m.push_back({S.hNY == G.day ? "Happy New Year!" : "New Year's Countdown (10 PM - midnight)", 29404}); }
        if (S.resDays) { static const char* RN[4] = {"", "Get Fit", "Get Smart", "Be Social"}; snprintf(b, sizeof b, "Resolution: %s (%d days left)", RN[S.resolution % 4], S.resDays); m.push_back({b, 29999}); }
        return m; }
    if (id == 29401) { if (S.hGift == G.day || !pay(40)) return {}; S.hGift = (uint16_t)G.day;
        for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household) { addMoodlet(G.sims[i], "Got a Gift!", EM_HAPPY, 2, 720); if (i != G.mc) changeRel(G.mc, i, 5, 0); }
        int pick = -1; for (int t = 0; t < 40 && pick < 0; t++) { const int o = 9 + irand(NUM_OUTFITS - 9); if (!ownsOutfit(o) && outfitFits(o, s.stage, s.look.guy)) pick = o; }
        if (pick >= 0) { giveOutfit(pick); toast(std::string("Gift time! Paper everywhere. You unwrapped: ") + OUTFITS[pick].name + "!"); }
        else toast("Gift time! Wrapping paper everywhere and hugs all around."); guPhoto("Opening gifts"); return {}; }
    if (id == 29402) { if (S.hDinner == G.day) return {}; S.hDinner = (uint16_t)G.day;
        for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household) { G.sims[i].needs[N_HUNGER] = 100; G.sims[i].needs[N_SOCIAL] = std::min(100.f, G.sims[i].needs[N_SOCIAL] + 40); addMoodlet(G.sims[i], "Holiday Feast", EM_HAPPY, S.nRecipes ? 3 : 2, 720); if (i != G.mc) changeRel(G.mc, i, 4, 0); }
        if (S.nRecipes) { S.hbMuse = 240; toast(std::string("Holiday dinner with ") + S.recipes[irand(S.nRecipes)].name + " - just like it's always been made."); }
        else toast("Holiday dinner! Everyone's stuffed. (Start a family cookbook to make it extra special.)"); guPhoto("Holiday dinner"); return {}; }
    if (id == 29403) { if (S.hCostume == G.day || !pay(60)) return {}; S.hCostume = (uint16_t)G.day;
        static const char* CO[6] = {"a vampire", "a fairy", "a pop star", "a pizza slice", "a cowgirl/cowboy", "a ghost in a bedsheet"};
        int n = 0; for (int k = 0; k < 4; k++) { const int t = randomTownie(false); if (t >= 0) { changeRel(G.mc, t, 5, 0); n++; } }
        need(N_FUN, 35); need(N_SOCIAL, 30); addMoodlet(s, "Costume Party!", EM_PLAYFUL, 3, 480); G.followers += 50 + n * 25;
        toast(std::string("Costume party! You went as ") + CO[irand(6)] + ". " + std::to_string(n) + " friends came and the best-costume contest got heated."); guPhoto("Costume party"); return {}; }
    if (id == 29404) { if (S.hNY == G.day) return {}; const int h = (int)(G.minute / 60); if (h < 22) { toast("The countdown starts at 10 PM."); return {}; }
        S.hNY = (uint16_t)G.day; for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household) addMoodlet(G.sims[i], "Happy New Year!", EM_HAPPY, 3, 720);
        need(N_FUN, 30); guPhoto("New Year's countdown");
        m.push_back({"10... 9... 8... FIREWORKS! Pick a resolution:", 29999});
        m.push_back({"Get Fit (faster Fitness for a week)", 29405}); m.push_back({"Get Smart (faster Logic for a week)", 29406}); m.push_back({"Be Social (faster Charisma for a week)", 29407}); return m; }
    if (id >= 29405 && id < 29408) { S.resolution = (uint8_t)(id - 29404); S.resDays = 7; toast("Resolution set! Stick with it for 7 days for a bonus."); return {}; }
    // ------------------------------------------------------------- settings & backups
    if (id == 29950) {
        static const char* SP3[3] = {"Short", "Normal", "Long"}; static const char* TX[3] = {"Slow", "Normal", "Fast"}; static const char* PF[4] = {"Off", "Rare", "Normal", "Often"};
        m.push_back({std::string("Pop-up message time: ") + SP3[S.toastSp % 3], 29951});
        m.push_back({std::string("Dialogue text speed: ") + TX[S.textSp % 3], 29952});
        m.push_back({std::string("Drama pop-ups: ") + PF[S.popFreq % 4], 29953});
        m.push_back({std::string("Weather wardrobe (auto raincoat/snow gear): ") + (S.wxWear ? "ON" : "OFF"), 29954});
        m.push_back({"-- Backups (made automatically once a day) --", 29999});
        bool any = false;
        for (int k = 0; k < 3; k++) { const int d = backupDay(k); if (d < 0) continue; any = true; snprintf(b, sizeof b, "Restore backup from Day %d%s", d + 1, k == 0 ? " (newest)" : ""); m.push_back({b, 29960 + k}); }
        if (!any) m.push_back({"(No backups yet - one is made the first time you save each day.)", 29999});
        return m; }
    if (id == 29951) { S.toastSp = (uint8_t)((S.toastSp + 1) % 3); return l3Option(29950); }
    if (id == 29952) { S.textSp = (uint8_t)((S.textSp + 1) % 3); return l3Option(29950); }
    if (id == 29953) { S.popFreq = (uint8_t)((S.popFreq + 1) % 4); return l3Option(29950); }
    if (id == 29954) { S.wxWear = !S.wxWear; return l3Option(29950); }
    // ------------------------------------------------------------- buffs
    if (id == 29900) {
        static const char* EN[EM_COUNT] = {"Fine", "Happy", "Flirty", "Confident", "Inspired", "Playful", "Energized", "Sad", "Angry", "Tense", "Bored", "Focused", "Embarrassed", "Uncomfortable"};
        int str = 0; const Emotion e = emotionOf(s, str);
        snprintf(b, sizeof b, "Mood: %s%s", str >= 4 ? "Very " : "", EN[e]); m.push_back({b, 29999});
        m.push_back({"-- Moodlets --", 29999});
        for (const Moodlet& md : s.moodlets) { snprintf(b, sizeof b, "%+d %s: %s (%dh%02dm)", md.power, EN[md.emotion % EM_COUNT], md.name.c_str(), md.minutes / 60, md.minutes % 60); m.push_back({b, 29999}); }
        if (s.moodlets.empty()) m.push_back({"(none right now)", 29999});
        m.push_back({"-- Hidden buffs --", 29999});
        if (S.hbRested) { snprintf(b, sizeof b, "Well Rested: +15%% skill speed (%dh)", S.hbRested / 60); m.push_back({b, 29999}); }
        if (S.hbCaffeine) { snprintf(b, sizeof b, "Caffeinated: +20%% Logic & Fitness (%dh)", S.hbCaffeine / 60); m.push_back({b, 29999}); }
        if (S.hbMuse) { snprintf(b, sizeof b, "Home Cooking Muse: +20%% Cooking & Creativity (%dh)", S.hbMuse / 60); m.push_back({b, 29999}); }
        if (!S.hbRested && !S.hbCaffeine && !S.hbMuse) m.push_back({"(none)", 29999});
        m.push_back({"-- Permanent buffs --", 29999});
        int perm[EM_COUNT] = {}; l3PermanentBuffs(perm, EM_COUNT); bool any = false;
        for (int k = 0; k < EM_COUNT; k++) if (perm[k]) { snprintf(b, sizeof b, "%+d %s (traits & status)", perm[k], EN[k]); m.push_back({b, 29999}); any = true; }
        if (!any) m.push_back({"(none)", 29999});
        m.push_back({"Drink a Coffee ($3) - Caffeinated", 29901});
        return m; }
    if (id == 29901) { if (!pay(3)) return {}; S.hbCaffeine = 180; addMoodlet(s, "Caffeinated", EM_ENERGIZED, 1, 180); need(N_ENERGY, 15); toast("Coffee! Logic & Fitness skills grow faster for 3 hours."); return {}; }
    return {};
}

// ===================================================================== hooks
int l3AutoOf(int i) {
    if (i < 0) return 2;
    const int v = i < 128 ? S.simAuto[i] : 0;
    if (v) return (v - 1) % 3;
    return i == G.mc ? S.mcAuto % 3 : 2;   // you start Off; the rest of the household starts Full (as before)
}
int l3MCAuto() { return l3AutoOf(G.mc); }
float l3ToastMult() { static const float M[3] = {0.6f, 1.f, 1.7f}; return M[S.toastSp % 3]; }
float l3TextMult() { static const float M[3] = {0.5f, 1.f, 2.2f}; return M[S.textSp % 3]; }
int l3PopPct(int p) { static const int M[4] = {0, 40, 100, 180}; return p * M[S.popFreq % 4] / 100; }
float l3SkillMult(int skill) {
    const Sim& s = MC(); float m = 1.f;
    if (S.hbRested) m += 0.15f;
    if (S.resDays && ((S.resolution == 1 && skill == SK_FITNESS) || (S.resolution == 2 && skill == SK_LOGIC) || (S.resolution == 3 && skill == SK_CHARISMA))) m += 0.3f;
    if (S.hbCaffeine && (skill == SK_LOGIC || skill == SK_FITNESS)) m += 0.2f;
    if (S.hbMuse && (skill == SK_COOKING || skill == SK_CREATIVITY)) m += 0.2f;
    switch (emotionOf(s)) {   // emotion-driven skill speed (Sims 4 style)
    case EM_FOCUSED: if (skill == SK_LOGIC) m += 0.4f; break;
    case EM_PLAYFUL: if (skill == SK_CHARISMA) m += 0.3f; break;
    case EM_FLIRTY: if (skill == SK_CHARISMA) m += 0.2f; break;
    case EM_ANGRY: if (skill == SK_FITNESS) m += 0.2f; break;   // rage workout
    case EM_HAPPY: m += 0.1f; break;
    case EM_SAD: m -= 0.2f; break;
    case EM_TENSE: m -= 0.1f; break;
    default: break;
    }
    return std::max(0.3f, m);
}
void l3PermanentBuffs(int score[], int n) {
    if (G.sims.empty() || G.mc < 0 || G.mc >= (int)G.sims.size() || n < EM_COUNT) return;
    const Sim& s = G.sims[G.mc];
    if (s.hasTrait(TR_GLAMOROUS)) score[EM_CONFIDENT] += 1;
    if (s.hasTrait(TR_LAZY) && s.needs[N_FUN] < 50) score[EM_BORED] += 1;
    if (s.hasTrait(TR_HOTHEAD) && (s.needs[N_HUNGER] < 30 || s.needs[N_ENERGY] < 30)) score[EM_ANGRY] += 1;
    if (S.mayor) score[EM_CONFIDENT] += 1;
    if (S.vhOwn[0] || S.vhOwn[1] || S.vhOwn[2]) score[EM_HAPPY] += 1;
    if (S.attr[0] >= 8) score[EM_ENERGIZED] += 1;
    if (S.attr[1] >= 8) score[EM_FOCUSED] += 1;
}
void l3Hourly(int hour) {
    if (G.sims.empty()) return;
    Sim& s = me();
    for (uint16_t* t : {&S.hbRested, &S.hbCaffeine, &S.hbMuse}) *t = (uint16_t)std::max(0, (int)*t - 60);
    if (hour >= 6 && hour <= 9 && s.needs[N_ENERGY] >= 95 && !S.hbRested) { S.hbRested = 360; addMoodlet(s, "Well Rested", EM_ENERGIZED, 1, 240); }
    if ((G.weather == WX_RAIN || G.weather == WX_STORM) && roomOutdoors(G.room)) addMoodlet(s, "Soaked", EM_UNCOMFORTABLE, 1, 120);
    if (G.weather == WX_SUNNY && G.room == R_BEACH) addMoodlet(s, "Sun-Kissed", EM_HAPPY, 1, 180);
    if (G.weather == WX_SNOW && roomOutdoors(G.room)) addMoodlet(s, "Snow Day!", EM_PLAYFUL, 1, 180);
    // weather wardrobe: slip into owned rain/snow gear, change back when it clears
    if (S.wxWear) {
        const bool bad = G.weather == WX_RAIN || G.weather == WX_STORM || G.weather == WX_SNOW;
        if (bad && !S.wxOn) {
            int pick = -1;
            for (int o = 0; o < NUM_OUTFITS && pick < 0; o++) if ((OUTFITS[o].topStyle == TS_RAINCOAT || (G.weather == WX_SNOW && OUTFITS[o].topStyle == TS_JACKET)) && ownsOutfit(o) && outfitFits(o, s.stage, s.look.guy)) pick = o;
            if (pick >= 0 && pick != s.outfit) { S.wxPrev = s.outfit; S.wxOn = 1; s.outfit = (uint8_t)pick; toast(std::string(G.weather == WX_SNOW ? "Bundled up in " : "Threw on ") + OUTFITS[pick].name + " for the weather."); }
        } else if (!bad && S.wxOn) { if (S.wxPrev >= 0 && S.wxPrev < NUM_OUTFITS) s.outfit = (uint8_t)S.wxPrev; S.wxOn = 0; S.wxPrev = -1; }
    }
    // teen curfew
    if (s.stage == ST_TEEN && !roomIsHome(G.room) && S.curfew < 24 && hour == S.curfew && S.curfewDay != (uint8_t)(G.day & 0xFF)) {
        S.curfewDay = (uint8_t)(G.day & 0xFF); addMoodlet(s, "Busted Past Curfew", EM_TENSE, 2, 720);
        for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household && G.sims[i].stage >= ST_YA) changeRel(G.mc, i, -3, 0);
        toast("Your phone buzzes: \"It's past curfew. Get. Home. NOW.\" - Mom");
    }
}
void l3Daily() {
    if (G.sims.empty()) return;
    Sim& s = me();
    for (int t = 0; t < S.trees && t < 6; t++) if (S.treeAge[t] < 255) S.treeAge[t]++;
    // vacation rentals
    for (int k = 0; k < 3; k++) if (S.vhOwn[k] && S.vhRent[k] && !S.vhBroken[k] && vacationDest() != VHS[k].dest + 1) {
        const int inc = VHS[k].rent + (S.vhDecor[k] == seasonOf(G.day) + 1 ? 20 : 0); G.money += inc;
        if (irand(100) < 7) { S.vhBroken[k] = 1; S.vhRent[k] = 0; toast(std::string("Ugh! Renters trashed your ") + VHS[k].name + ". Redecorate it to fix it up."); }
    }
    // pregnancy cravings
    if (s.pregnant) { if (!S.cravingDone && S.craving) addMoodlet(s, "Unsatisfied Craving", EM_TENSE, 1, 600); S.craving = (uint8_t)(1 + irand(8)); S.cravingDone = 0;
        toast(std::string("Craving alert: you NEED ") + CRAVE[S.craving % 8] + ". (Life Extras > Family Extras)"); }
    else { S.showerDone = 0; S.craving = 0; }
    // election day
    if (S.campaign && G.day >= S.campEnd) { S.campaign = 0; const int rival = 45 + irand(15);
        if (S.support >= rival) { S.mayor = 1; G.money += 500; addMemory(MEM_CAREER, G.mc, -1, "Elected Mayor of Maple Valley!"); guPhoto("Election night victory"); addMoodlet(s, "Elected Mayor!", EM_CONFIDENT, 4, 2880); lvlPrestige(10, "won the election");
            toast("ELECTION NIGHT: You won with " + std::to_string(S.support) + "%! Mayor " + s.name + " - +$500 and $100/day salary."); }
        else toast("ELECTION NIGHT: You lost " + std::to_string(rival) + "% to " + std::to_string(S.support) + "%. There's always next term."); }
    if (S.resDays && --S.resDays == 0) { addMoodlet(s, "Kept My Resolution!", EM_CONFIDENT, 3, 1440); toast("You kept your New Year's resolution all week!"); }
    S.campDone = 0;
    if (S.mayor) G.money += 100;
    // neighbor feuds
    if (!S.feud && irand(100) < 6) { const int w = randomTownie(false); if (w >= 0) { S.feud = (uint8_t)(1 + irand(4)); S.feudWith = (int16_t)w; toast(std::string("Neighbor drama: ") + FEUD[(S.feud - 1) % 4] + " (Life Extras > Town Life)"); } }
    checkBadges();
}

void l3SaveTo(const std::string& path) { FILE* f = fopen(path.c_str(), "wb"); if (!f) return; fwrite(&S, sizeof S, 1, f); fclose(f); }
void l3LoadFrom(const std::string& path) {
    S = L3{}; FILE* f = fopen(path.c_str(), "rb"); if (!f) return;
    L3 t; const size_t n = fread(&t, 1, sizeof t, f); fclose(f);
    if (n == sizeof t && t.magic == 0x3346334Cu) S = t;
    if (S.nRecipes > 10) S.nRecipes = 0; for (auto& r : S.recipes) r.name[sizeof r.name - 1] = 0;
    for (auto& a : S.attr) if (a > 10) a = 10; if (S.dream > 6) S.dream = 0; if (S.feudWith >= (int)G.sims.size()) { S.feud = 0; S.feudWith = -1; }
    if (S.trees > 6) S.trees = 6;
    if (S.mcAuto > 2) S.mcAuto = 2;
    if (S.wxPrev >= NUM_OUTFITS) { S.wxPrev = -1; S.wxOn = 0; } if (S.resolution > 3) S.resDays = 0;
    if (S.curfew < 18 || S.curfew > 24) S.curfew = 22;
}
