#include "dailylife.hpp"
#include "farm.hpp"
#include "growup.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "people.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include "tween.hpp"
#include "goals.hpp"
#include "health.hpp"
#include <string>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include "vfs.hpp"
#define fopen vfopen   // sidecar saves go into the one-file save pack

extern const char* HAIRSTYLE_NAMES[];
std::vector<std::string> toastHistory();
void lifeMarry(int a, int b);

namespace {
struct DL {
    uint32_t magic = 0x444C5931;
    int16_t fridge = 12; uint8_t sick[256] = {}; uint8_t hairDays = 0, nails = 100, bff = 0xFF, fame = 0;
    uint8_t ex[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t biz = 0, bizPrice = 1, bizStaff = 0, bizRep = 10; int32_t bizTotal = 0;
    int16_t credit = 650; int32_t loan = 0; int16_t loanPay = 0; uint8_t missed = 0;
    uint8_t reviewDays = 0, costume = 0, sweater = 0, hoaWarn = 0, chatRead[3] = {};
    uint16_t lastBday = 0xFFFF;
    uint8_t myLike[3] = {0xFF, 0xFF, 0xFF}, myDislike = 0xFF;
};
DL D;
int rq = -1, rqScore = 0, rqRecipe = 0;   // recipe steps (RAM)
int pv = -1, pvScore = 0;                 // performance review (RAM)
int pick = 0;                             // 1 bff, 2 breakup, 3 get back together

std::string first(int s) { const std::string& n = G.sims[s].name; return n.substr(0, n.find(' ')); }
Sim& me() { return MC(); }
void need(int n, float d) { Sim& s = me(); s.needs[n] = std::max(0.f, std::min(100.f, s.needs[n] + d)); }
bool pay(int c) { if (G.money < c) { toast("Not enough money ($" + std::to_string(c) + ")."); return false; } G.money -= c; return true; }
bool adult() { return me().stage >= ST_YA; }
int hairIdx(const char* n) { for (int i = 0; i < NUM_HAIRSTYLES; i++) if (!strcmp(HAIRSTYLE_NAMES[i], n)) return i; return -1; }
void setHair(const char* n) { const int k = hairIdx(n); if (k >= 0) me().look.hairStyle = (uint8_t)k; }
int season() { return seasonOf(G.day); }
int dayInSeason() { return G.day % 7; }

struct Recipe { const char* name; int servings; const char* steps[3][3]; int ok[3]; };
const Recipe RC[5] = {
    {"Mac & Cheese", 4, {{"Boil the pasta", "Bake the pasta raw", "Microwave the box"}, {"Stir in cheese sauce", "Add ketchup", "Skip the cheese"}, {"Bake till bubbly", "Freeze it", "Serve cold"}}, {0, 0, 0}},
    {"Pancakes", 3, {{"Mix flour, egg & milk", "Mix flour & water only", "Use pancake... soup?"}, {"Hot buttered pan", "Cold pan", "No pan"}, {"Flip when bubbly", "Flip right away", "Never flip"}}, {0, 0, 0}},
    {"Tacos", 4, {{"Brown the meat", "Boil the meat", "Skip the meat"}, {"Add taco seasoning", "Add sugar", "Add nothing"}, {"Warm the shells", "Wet the shells", "Crush the shells"}}, {0, 0, 0}},
    {"Spaghetti", 5, {{"Salt the water & boil", "Cold water soak", "Fry dry pasta"}, {"Simmer the sauce", "Burn the sauce", "Skip the sauce"}, {"Toss & add parmesan", "Add mayo", "Serve plain"}}, {0, 0, 0}},
    {"Birthday Cake", 8, {{"Cream butter & sugar", "Melt the sugar black", "Skip the butter"}, {"Bake 30 minutes", "Bake 3 minutes", "Bake 3 hours"}, {"Cool then frost", "Frost it hot", "Forget to frost"}}, {0, 0, 0}},
};
LxMenu recipeStep() {
    LxMenu m; const Recipe& r = RC[rqRecipe]; m.push_back({std::string(r.name) + " - step " + std::to_string(rq + 1) + "/3:", 19999});
    int order[3] = {0, 1, 2}; for (int i = 2; i > 0; i--) std::swap(order[i], order[irand(i + 1)]);
    for (int i : order) m.push_back({r.steps[rq][i], 19101 + i});
    return m;
}
LxMenu reviewStep() {
    static const char* Q[3] = {"Boss: \"How'd you handle the big deadline?\"", "Boss: \"A customer was rude. What did you do?\"", "Boss: \"Where do you see yourself in a year?\""};
    static const char* A[3][3] = {{"Planned ahead & delivered early", "Pulled an all-nighter", "Missed it"}, {"Stayed calm & fixed it", "Rolled my eyes", "Walked out"}, {"Leading the team", "Here, I guess", "Not at this job lol"}};
    LxMenu m; m.push_back({Q[pv], 19999}); for (int i = 0; i < 3; i++) m.push_back({A[pv][i], 19301 + i}); return m;
}
const char* BIZ[5] = {"", "Boutique", "Hair Salon", "Bakery", "Nail Shop"};
const char* GC[3] = {"Besties Only", "Family Group Chat", "Work Gossip"};
}

const char* LIKE_NAMES[LK_COUNT] = {"Fashion", "Music", "Sports", "Cooking", "Gaming", "Books", "Dogs", "Cats", "Parties", "Nature", "The Beach", "Shopping", "Dancing", "Art", "Horror Movies", "Cars"};
static uint32_t hsh(int s, int k) { uint32_t h = 2166136261u; for (char c : G.sims[s].name) h = (h ^ (uint8_t)c) * 16777619u; return (h ^ (uint32_t)(k * 0x9E3779B9u)) * 2654435761u; }
int dlyLikes(int s, int t) {
    if (s < 0 || s >= (int)G.sims.size()) return 0;
    if (s == G.mc && D.myLike[0] != 0xFF) { for (int i = 0; i < 3; i++) if (D.myLike[i] == t) return 1; return D.myDislike == t ? -1 : 0; }
    const int a = hsh(s, 1) % LK_COUNT, b = hsh(s, 2) % LK_COUNT, c = hsh(s, 3) % LK_COUNT;
    if (t == a || t == b) return 1;
    if (t == c && c != a && c != b) return -1;
    return 0;
}
void dlyActivity(int t) {
    const int l = dlyLikes(G.mc, t);
    if (l > 0) { addMoodlet(MC(), (std::string("Loves ") + LIKE_NAMES[t]).c_str(), EM_HAPPY, 1, 240); MC().needs[N_FUN] = std::min(100.f, MC().needs[N_FUN] + 10); }
    else if (l < 0) { addMoodlet(MC(), (std::string("Ugh, ") + LIKE_NAMES[t]).c_str(), EM_UNCOMFORTABLE, 1, 240); MC().needs[N_FUN] = std::max(0.f, MC().needs[N_FUN] - 10); }
}
static int talkTopic = 0;
void dlyLikeOptions(int t, std::vector<SocialOpt>& v) {
    if (t < 0 || t >= (int)G.sims.size() || G.sims[t].stage < ST_CHILD || MC().stage < ST_CHILD) return;
    v.push_back({SO_ASKLIKES, "Ask What They're Into", SC_FRIENDLY});
    talkTopic = (int)((G.day + t) % LK_COUNT);
    for (int k = 0; k < LK_COUNT; k++) if (dlyLikes(G.mc, (talkTopic + k) % LK_COUNT) > 0) { talkTopic = (talkTopic + k) % LK_COUNT; break; }
    static std::string label; label = std::string("Talk About ") + LIKE_NAMES[talkTopic];
    v.push_back({SO_TALKLIKE, label.c_str(), SC_FRIENDLY});
}
bool dlyLikeSocial(int id, int t) {
    if (id != SO_ASKLIKES && id != SO_TALKLIKE) return false;
    const std::string n = first(t);
    if (id == SO_ASKLIKES) {
        std::string likes, dis; for (int k = 0; k < LK_COUNT; k++) { const int l = dlyLikes(t, k); if (l > 0) likes += (likes.empty() ? "" : " & ") + std::string(LIKE_NAMES[k]); else if (l < 0) dis = LIKE_NAMES[k]; }
        toast(n + " loves " + (likes.empty() ? "a little of everything" : likes) + (dis.empty() ? "." : " - but can't stand " + dis + "."));
        changeRel(G.mc, t, 2, 0); return true;
    }
    const int l = dlyLikes(t, talkTopic);
    if (l > 0) { changeRel(G.mc, t, 7, 0); tweenPlay(t, TW_HOP); toast(n + " LIGHTS UP. \"Oh my gosh, I love " + LIKE_NAMES[talkTopic] + "!\""); }
    else if (l < 0) { changeRel(G.mc, t, -4, 0); tweenPlay(t, TW_SHAKE); toast(n + " makes a face. \"Ew, I hate " + std::string(LIKE_NAMES[talkTopic]) + ".\""); }
    else { changeRel(G.mc, t, 1, 0); toast(n + " nods politely. Not really their thing."); }
    return true;
}
void dlyCure(int s) { if (s >= 0 && s < (int)G.sims.size()) healthTreat(s, 1); }
void dlyNailsDone() { D.nails = 100; }
bool dlySick(int s) { return s >= 0 && s < (int)G.sims.size() && isSick(s); }   // one sickness system: health.cpp

LxMenu dlyApp() {
    LxMenu m; const Sim& s = me();
    m.push_back({"Notifications (recent pop-ups)", 19700});
    m.push_back({"Fridge: " + std::to_string(D.fridge) + " servings", 19999});
    for (auto& r : farmHomeRows()) m.push_back(r);
    m.push_back({"Life Extras (buffs, vacation homes, dreams, town...)", 29000});
    m.push_back({"Home Garden Pots", 27500});
    m.push_back({"Grocery Delivery ($30, +10 servings)", 19001}); m.push_back({"Grab Leftovers", 19002}); m.push_back({"Cook a Recipe", 19100});
    if (dlySick(G.mc)) { m.push_back({"You're sick! Stay Home & Rest", 19010}); m.push_back({"Doctor Visit ($40)", 19011}); }
    m.push_back({"Hair: " + std::string(HAIRSTYLE_NAMES[s.look.hairStyle % NUM_HAIRSTYLES]) + " (" + std::to_string(D.hairDays) + " days growth)", 19999});
    m.push_back({"Nails: " + std::to_string(D.nails) + "% fresh", 19999});
    { std::string l, d; for (int k = 0; k < LK_COUNT; k++) { const int x = dlyLikes(G.mc, k); if (x > 0) l += (l.empty() ? "" : ", ") + std::string(LIKE_NAMES[k]); else if (x < 0) d = LIKE_NAMES[k]; }
      m.push_back({"Likes: " + (l.empty() ? std::string("?") : l), 19600}); m.push_back({"Dislikes: " + (d.empty() ? std::string("nothing") : d) + " (tap to edit)", 19600}); }
    m.push_back({"Group Chats", 19200});
    m.push_back({D.bff != 0xFF && D.bff < G.sims.size() ? "BFF: " + first(D.bff) : "Pick a Best Friend", 19210});
    m.push_back({"Exes & Breakups", 19220});
    if (adult()) {
        m.push_back({"Celebrity Path (fame " + std::to_string(D.fame) + ")", 19230});
        m.push_back({D.biz ? std::string("My ") + BIZ[D.biz] + " (open for the day)" : "Start a Business ($2,000)", 19240});
        m.push_back({"Side Hustles", 19260});
        m.push_back({"Credit Score: " + std::to_string(D.credit) + (D.loan ? "  Loan: $" + std::to_string(D.loan) : ""), 19270});
        if (s.career != CR_NONE) m.push_back({"Performance Review", 19300});
        if (D.hoaWarn) m.push_back({"!! HOA Complaint Letter", 19280});
    }
    m.push_back({"Holidays & Birthdays", 19400});
    return m;
}

LxMenu dlyOption(int id) {
    Sim& s = me(); LxMenu m;
    switch (id) {
    case 19001: if (pay(30)) { D.fridge = (int16_t)std::min(60, D.fridge + 10); toast("Groceries delivered! The fridge is stocked."); } return {};
    case 19002: if (D.fridge <= 0) { toast("The fridge is EMPTY. Order groceries!"); return {}; } D.fridge--; need(N_HUNGER, 35); toast("Leftovers. Still good. Probably."); return {};
    case 19100: for (int i = 0; i < 5; i++) m.push_back({std::string(RC[i].name) + " (uses 2 servings of groceries)", 19110 + i}); return m;
    case 19110: case 19111: case 19112: case 19113: case 19114: if (D.fridge < 2) { toast("Not enough ingredients - order groceries!"); return {}; }
        D.fridge -= 2; rqRecipe = id - 19110; rq = 0; rqScore = 0; return recipeStep();
    case 19101: case 19102: case 19103: {
        if (rq < 0) return {};
        if (id - 19101 == RC[rqRecipe].ok[rq]) rqScore++;
        if (++rq < 3) return recipeStep();
        rq = -1; const int q = rqScore * 25 + (int)s.skills[SK_COOKING] * 3 + irand(10);
        const char* tier = q >= 85 ? "PERFECT" : q >= 60 ? "Tasty" : q >= 35 ? "Edible" : "Burnt";
        s.skills[SK_COOKING] = std::min(10.f, s.skills[SK_COOKING] + 0.3f); D.fridge = (int16_t)(D.fridge + RC[rqRecipe].servings);
        for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household) G.sims[i].needs[N_HUNGER] = std::min(100.f, G.sims[i].needs[N_HUNGER] + (q >= 35 ? 50 : 15));
        toast(std::string(tier) + " " + RC[rqRecipe].name + "! The family eats" + (q >= 85 ? " and asks for seconds." : q >= 35 ? "." : "... politely."));
        if (q >= 85) addMoodlet(s, "Kitchen Queen", EM_CONFIDENT, 1, 720); dlyActivity(LK_COOKING); return {}; }
    case 19010: need(N_ENERGY, 40); MC().schoolToday = true; MC().workedToday = true; toast("Soup, blankets, and trashy TV. You called in sick - excused from school/work today."); return {};
    case 19011: if (pay(40)) { healthTreat(G.mc, 0); toast("Doctor: \"Rest and fluids.\" You feel better already."); } return {};
    // group chats
    case 19200: for (int i = 0; i < 3; i++) m.push_back({std::string(GC[i]) + (D.chatRead[i] ? " (new!)" : ""), 19201 + i}); return m;
    case 19201: case 19202: case 19203: {
        const int g = id - 19201; D.chatRead[g] = 0;
        static const char* L[3][4] = {{"\"WHO posted that pic of me??\"", "\"girls night friday??\" (12 people typing)", "\"ok but why did she leave me on READ\"", "Someone sent a screenshot of the OTHER chat..."},
                                      {"Mom: \"Who ate my yogurt\"", "Grandma sent 14 good-morning flowers", "Dad: \"lol\" (to a sad message)", "Auntie: \"we need to talk about Thanksgiving\""},
                                      {"\"did u see what Brenda wore\"", "\"boss is in a MOOD today\"", "\"who's stealing the office snacks\"", "\"don't screenshot this but...\""}};
        const int k = irand(4); toast(std::string(GC[g]) + ": " + L[g][k]);
        if (k == 3 && irand(2)) { lxGossip("Someone leaked the " + std::string(GC[g]) + " screenshots", true, CI_PARTY, -1); toast("The screenshot is spreading around town..."); }
        if (k == 2) addMoodlet(s, "Left on Read", EM_SAD, 1, 240);
        need(N_SOCIAL, 10); return {}; }
    // BFF
    case 19210: if (D.bff != 0xFF && D.bff < G.sims.size()) { m.push_back({"Give a BFF Necklace", 19211}); m.push_back({"Twin! Matching Outfits", 19212}); m.push_back({"Friendship Breakup", 19213}); return m; }
        pick = 1; for (int i = 0; i < (int)G.sims.size() && i < 250; i++) if (i != G.mc && rel(G.mc, i).fr >= 50) m.push_back({G.sims[i].name, 19500 + i});
        if (m.empty()) m.push_back({"(Need a friend at 50+ first)", 19999}); return m;
    case 19211: changeRel(G.mc, D.bff, 8, 0); addMoodlet(s, "Best Friends Forever", EM_HAPPY, 2, 2880); toast("Half a heart for you, half for " + first(D.bff) + ". BFFs FOREVER."); return {};
    case 19212: G.sims[D.bff].outfit = s.outfit; tweenPlay(G.mc, TW_CHEER); tweenPlay(D.bff, TW_CHEER); toast("Twinning with " + first(D.bff) + " today!"); return {};
    case 19213: changeRel(G.mc, D.bff, -30, 0); toast("You and " + first(D.bff) + " aren't friends anymore. You unfollowed each other everywhere."); addMoodlet(s, "Friendship Breakup", EM_SAD, 2, 2880); D.bff = 0xFF; return {};
    // exes
    case 19220: for (int i = 0; i < 8; i++) if (D.ex[i] < G.sims.size()) m.push_back({"Ex: " + G.sims[D.ex[i]].name + " (get back together?)", 19530 + i});
        m.push_back({"Break Up with Someone", 19221}); return m;
    case 19221: pick = 2; for (int i = 0; i < (int)G.sims.size() && i < 250; i++) if (i != G.mc && rel(G.mc, i).status >= RS_DATING && rel(G.mc, i).status <= RS_MARRIED) m.push_back({"Break up with " + G.sims[i].name, 19500 + i});
        if (m.empty()) m.push_back({"(You're single!)", 19999}); return m;
    // celebrity
    case 19230: m.push_back({"Go Live & Grow Your Fame", 19231}); if (D.fame >= 20) m.push_back({"Paparazzi Walk", 19232}); if (D.fame >= 40) m.push_back({"Fan Meet & Greet", 19233}); if (D.fame >= 60) m.push_back({"Red Carpet Premiere", 19234});
        if (D.fame < 20) m.push_back({"(Reach fame 20 to unlock paparazzi)", 19999}); return m;
    case 19231: { const int g = 2 + (int)s.skills[SK_CHARISMA] / 2 + (int)std::min<uint32_t>(10, G.followers / 500); D.fame = (uint8_t)std::min(100, D.fame + g); toast("Your live hit a new viewer record! Fame +" + std::to_string(g)); return {}; }
    case 19232: D.fame = (uint8_t)std::min(100, D.fame + 3); toast(irand(2) ? "Flash flash flash! Your coffee-run outfit is trending." : "They caught you mid-sneeze. Tabloid cover."); return {};
    case 19233: G.money += 200; D.fame = (uint8_t)std::min(100, D.fame + 4); need(N_SOCIAL, 30); toast("200 fans! Selfies, signed photos, someone cried. +$200"); return {};
    case 19234: G.money += 1000; D.fame = (uint8_t)std::min(100, D.fame + 6); addMoodlet(s, "Red Carpet Ready", EM_CONFIDENT, 3, 1440); guPhoto("Red carpet premiere"); toast("Red carpet! \"WHO ARE YOU WEARING?\" +$1,000"); return {};
    // business
    case 19240: if (!D.biz) { for (int i = 1; i < 5; i++) m.push_back({std::string("Open a ") + BIZ[i], 19240 + i}); return m; }
        m.push_back({"Open for the Day", 19250}); m.push_back({std::string("Prices: ") + (D.bizPrice == 0 ? "Cheap" : D.bizPrice == 1 ? "Fair" : "Luxury") + " (change)", 19251});
        m.push_back({"Staff: " + std::to_string(D.bizStaff) + " (hire $100/day each)", 19252}); m.push_back({"Fire a Worker", 19253}); m.push_back({"Earnings so far: $" + std::to_string(D.bizTotal), 19999}); return m;
    case 19241: case 19242: case 19243: case 19244: if (!pay(2000)) return {}; D.biz = (uint8_t)(id - 19240); toast(std::string("Grand opening of your ") + BIZ[D.biz] + "! Ribbon cut."); guPhoto(std::string("Grand opening: my ") + BIZ[D.biz]); return {};
    case 19250: { const int cust = std::max(1, 4 + D.bizStaff * 4 + D.bizRep / 8 - D.bizPrice * 3 + irand(5)); const int each = 15 + D.bizPrice * 20;
        const int cost = D.bizStaff * 100 + 40; const int profit = cust * each - cost; G.money += profit; D.bizTotal += profit;
        D.bizRep = (uint8_t)std::max(0, std::min(100, D.bizRep + (D.bizPrice == 2 && D.bizStaff == 0 ? -2 : 2))); need(N_ENERGY, -20);
        toast(std::to_string(cust) + " customers today! " + (profit >= 0 ? "Profit $" : "Loss $") + std::to_string(std::abs((long)profit))); return {}; }
    case 19251: D.bizPrice = (uint8_t)((D.bizPrice + 1) % 3); return dlyOption(19240);
    case 19252: if (D.bizStaff < 4) { D.bizStaff++; toast("New hire! They're... enthusiastic."); } return dlyOption(19240);
    case 19253: if (D.bizStaff) { D.bizStaff--; toast("You let someone go. Awkward."); } return dlyOption(19240);
    // side hustles
    case 19260: m.push_back({"Resell Thrifted Clothes", 19261}); m.push_back({"Braid Hair for Clients", 19262}); m.push_back({"Bake for Orders", 19263}); return m;
    case 19261: { dlyActivity(LK_FASHION); const int p = 20 + irand(60); G.money += p; toast("Your Y2K haul sold out online! +$" + std::to_string(p)); return {}; }
    case 19262: { const int p = 60 + (int)s.skills[SK_STYLE] * 12; G.money += p; need(N_ENERGY, -25); toast("Knotless braids, 6 hours, hands sore. +$" + std::to_string(p)); return {}; }
    case 19263: { if (D.fridge < 2) { toast("Need groceries to bake."); return {}; } D.fridge -= 2; const int p = 30 + (int)s.skills[SK_COOKING] * 10; G.money += p; toast("3 dozen cupcakes delivered! +$" + std::to_string(p)); return {}; }
    // credit & loans
    case 19270: m.push_back({"Credit score: " + std::to_string(D.credit) + (D.credit >= 740 ? " (Excellent)" : D.credit >= 670 ? " (Good)" : D.credit >= 580 ? " (Fair)" : " (Poor)"), 19999});
        if (D.loan) { m.push_back({"Loan: $" + std::to_string(D.loan) + " ($" + std::to_string(D.loanPay) + "/day auto-pay)", 19999}); m.push_back({"Pay Off Loan Now", 19273}); }
        else { m.push_back({"Car Loan: $3,000", 19271}); m.push_back({"Home Loan: $20,000", 19272}); } return m;
    case 19271: case 19272: { if (D.credit < 580) { toast("Loan DENIED. Your credit score is too low."); return {}; }
        const int amt = id == 19271 ? 3000 : 20000; const int rate = D.credit >= 740 ? 3 : D.credit >= 670 ? 6 : 10;
        D.loan = amt + amt * rate / 100; D.loanPay = (int16_t)std::max<int32_t>(20, D.loan / 60); G.money += amt;
        toast("Approved at " + std::to_string(rate) + "% - $" + std::to_string(amt) + " deposited! Payments are $" + std::to_string(D.loanPay) + "/day."); return {}; }
    case 19273: if (pay(D.loan)) { D.loan = 0; D.loanPay = 0; D.credit = (int16_t)std::min(850, D.credit + 30); toast("Loan paid off! Credit score up."); } return {};
    case 19280: m.push_back({"Pay the $100 Fine", 19281}); m.push_back({"Fight It at the HOA Meeting", 19282}); return m;
    case 19281: if (pay(100)) { D.hoaWarn = 0; toast("Fine paid. Karen from the HOA smiles smugly."); } return {};
    case 19282: D.hoaWarn = 0; if (irand(100) < 30 + (int)s.skills[SK_CHARISMA] * 6) toast("You WON the HOA meeting. Standing ovation from the neighbors."); else { G.money = G.money > 150 ? G.money - 150 : 0; toast("You lost the HOA vote. Fine doubled: -$150."); } return {};
    // performance review
    case 19300: pv = 0; pvScore = 0; return reviewStep();
    case 19301: case 19302: case 19303: if (pv < 0) return {}; pvScore += id == 19301 ? 2 : id == 19302 ? 1 : 0;
        if (++pv < 3) return reviewStep();
        pv = -1; D.reviewDays = 0;
        if (pvScore >= 5) { s.careerLevel = (uint8_t)std::min(9, s.careerLevel + 1); G.money += 300; addMoodlet(s, "PROMOTED!", EM_CONFIDENT, 3, 2880); toast("PROMOTION! New title, bigger check, +$300 bonus."); guPhoto("Got promoted!"); }
        else if (pvScore >= 3) toast("\"Meets expectations.\" A 2% raise and a pat on the back.");
        else { addMoodlet(s, "Bad Review", EM_SAD, 2, 1440); toast("\"We need to see improvement.\" Ouch."); }
        return {};
    // holidays
    case 19400: {
        const int se = season();
        if (se == 2) { m.push_back({"HALLOWEEN: Pick a Costume", 19410}); m.push_back({"Trick-or-Treat Door to Door", 19411}); m.push_back({"Costume Contest", 19412}); m.push_back({"Haunted House", 19413}); }
        if (se == 3) { m.push_back({"CHRISTMAS: Decorate the Tree", 19420}); m.push_back({"Family Gift Exchange", 19421}); m.push_back({"Ugly Sweater Party", 19422}); }
        if (se == 0 && dayInSeason() <= 2) { m.push_back({"VALENTINE'S: School/Work Dance", 19430}); m.push_back({"Send Candy Grams", 19431}); }
        m.push_back({"Throw a Surprise Birthday (adult, $120)", 19440});
        if (m.size() == 1) m.push_back({"(Halloween = fall, Christmas = winter, Valentine's = early spring)", 19999});
        return m; }
    case 19410: { static const char* C[6] = {"a witch", "a vampire", "a Y2K pop star", "a bumblebee", "a zombie prom queen", "a fairy"}; D.costume = (uint8_t)(1 + irand(6)); toast(std::string("Costume picked: ") + C[D.costume - 1] + "!"); return {}; }
    case 19411: { int candy = 0; for (int i = 0; i < 6; i++) candy += irand(4) ? 3 + irand(5) : 0;
        need(N_FUN, 30); need(N_ENERGY, -15); toast("You hit 6 houses: " + std::to_string(candy) + " pieces of candy! One house gave out toothbrushes. Boo."); guPhoto("Trick-or-treating"); return {}; }
    case 19412: if (!D.costume) { toast("Pick a costume first!"); return {}; } if (irand(100) < 40 + (int)s.skills[SK_STYLE] * 5) { G.money += 100; toast("1st place, costume contest! +$100"); } else toast("Honorable mention. The judges were wrong."); return {};
    case 19413: dlyActivity(LK_HORROR); need(N_FUN, 25); addMoodlet(s, irand(2) ? "TERRIFIED" : "Fearless", irand(2) ? EM_UNCOMFORTABLE : EM_CONFIDENT, 1, 360); toast(irand(2) ? "A chainsaw guy chased you OUT of the haunted house." : "You laughed at every jump scare."); return {};
    case 19420: for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household) { G.sims[i].needs[N_FUN] = std::min(100.f, G.sims[i].needs[N_FUN] + 20); tweenPlay(i, TW_ARMSUP); }
        guPhoto("Decorating the Christmas tree"); toast("Lights, tinsel, and the star on top! The tree is GORGEOUS."); return {};
    case 19421: { static const char* G2[4] = {"fuzzy socks", "a Y2K flip phone case", "a gift card (again)", "a matching PJ set for everyone"};
        for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household && i != G.mc) changeRel(G.mc, i, 5, 0); toast(std::string("Gift exchange! You got ") + G2[irand(4)] + "."); return {}; }
    case 19422: dlyActivity(LK_PARTIES); D.sweater = 1; need(N_FUN, 25); need(N_SOCIAL, 25); toast(irand(2) ? "Your sweater has LIGHTS. You win ugliest sweater!" : "Someone's sweater plays Jingle Bells on loop."); return {};
    case 19430: { need(N_FUN, 20); bool partner = false; for (const Rel& r : G.rels) if ((r.a == G.mc || r.b == G.mc) && r.status >= RS_DATING && r.status <= RS_MARRIED) partner = true;
        if (partner) { addMoodlet(s, "Valentine's Date", EM_FLIRTY, 2, 1440); toast("Slow dancing with your Valentine. Swoon."); }
        else { addMoodlet(s, "Single on Valentine's", EM_SAD, 1, 1440); toast("You danced with your friends. And ate all the chocolate. Alone."); } return {}; }
    case 19431: { int got = irand(5); need(N_SOCIAL, 10); toast("You sent 5 candy grams. You got " + std::to_string(got) + " back" + (got ? "!" : "... it's fine.")); return {}; }
    case 19440: { if (!pay(120)) return {}; int who = -1; for (int i = 0; i < (int)G.sims.size(); i++) if (i != G.mc && G.sims[i].household && G.sims[i].stage >= ST_YA) { who = i; break; }
        if (who < 0) for (int i = 0; i < (int)G.sims.size(); i++) if (i != G.mc && rel(G.mc, i).fr >= 40 && G.sims[i].stage >= ST_YA) { who = i; break; }
        if (who < 0) { toast("No grown-up to surprise."); G.money += 120; return {}; }
        static const char* T[3] = {"\"SURPRISE!\" They actually screamed.", "Black balloons and \"OVER THE HILL\" banners. They're horrified.", "They knew. They pretended not to. Iconic."};
        const char* age = G.sims[who].stage >= ST_ELDER ? "turning 70" : G.sims[who].stage >= ST_MIDDLE ? "turning 50" : G.sims[who].stage >= ST_ADULT ? "turning 30" : "turning 21";
        changeRel(G.mc, who, 10, 0); addMoodlet(G.sims[who], "Surprise Party!", EM_HAPPY, 2, 1440); toast(first(who) + " is " + age + "! " + T[irand(3)]); guPhoto(first(who) + "'s surprise party"); return {}; }
    }
    if (id == 19700) { for (const std::string& s : toastHistory()) m.push_back({s, 19999}); if (m.empty()) m.push_back({"(No notifications yet)", 19999}); return m; }
    if (id == 19600) { m.push_back({"Pick 3 Likes, then 1 Dislike:", 19999}); for (int k = 0; k < LK_COUNT; k++) m.push_back({LIKE_NAMES[k], 19610 + k}); D.myLike[0] = D.myLike[1] = D.myLike[2] = 0xFE; D.myDislike = 0xFF; return m; }
    if (id >= 19610 && id < 19610 + LK_COUNT) {
        const int k = id - 19610;
        for (int i = 0; i < 3; i++) if (D.myLike[i] == 0xFE) { D.myLike[i] = (uint8_t)k; if (i < 2) { m.push_back({std::string("Liked ") + LIKE_NAMES[k] + "! Pick another:", 19999}); for (int j = 0; j < LK_COUNT; j++) m.push_back({LIKE_NAMES[j], 19610 + j}); return m; }
                                                     m.push_back({"Now pick 1 DISLIKE:", 19999}); for (int j = 0; j < LK_COUNT; j++) m.push_back({LIKE_NAMES[j], 19630 + j}); return m; }
        return {};
    }
    if (id >= 19630 && id < 19630 + LK_COUNT) { D.myDislike = (uint8_t)(id - 19630); toast("Saved your likes & dislikes!"); return {}; }
    if (id >= 19500 && id < 19750 && id - 19500 < (int)G.sims.size()) {
        const int t = id - 19500;
        if (pick == 1) { D.bff = (uint8_t)t; changeRel(G.mc, t, 10, 0); toast(first(t) + " is officially your BEST FRIEND!"); return {}; }
        if (pick == 2) { Rel& r = rel(G.mc, t); r.status = RS_NONE; r.ro = (int8_t)std::max(-50, r.ro - 40); changeRel(G.mc, t, -15, 0);
            for (int i = 0; i < 8; i++) if (D.ex[i] == 0xFF || i == 7) { D.ex[i] = (uint8_t)t; break; }
            addMoodlet(s, "Heartbroken", EM_SAD, 2, 2880); toast("You broke up with " + first(t) + ". Crying in the car is valid."); return {}; }
    }
    if (id >= 19530 && id < 19538) { const int t = D.ex[id - 19530]; if (t >= (int)G.sims.size()) return {};
        if (irand(100) < 30 + rel(G.mc, t).fr / 2) { rel(G.mc, t).status = RS_DATING; rel(G.mc, t).ro = 40; D.ex[id - 19530] = 0xFF; toast("You and " + first(t) + " are back together! (Everyone saw this coming.)"); }
        else toast(first(t) + ": \"I've moved on.\" Oof.");
        return {}; }
    return {};
}

void dlyHourly(int hour) {
    if (hour == 12) for (int g = 0; g < 3; g++) if (irand(100) < 25) D.chatRead[g] = 1;
    if (hour == 19 && irand(100) < 6) {   // ex run-in
        for (int i = 0; i < 8; i++) if (D.ex[i] < G.sims.size()) {
            const int t = D.ex[i]; bool dating = false; for (const Rel& r : G.rels) if ((r.a == t || r.b == t) && r.a != G.mc && r.b != G.mc && r.status == RS_DATING) dating = true;
            toast(dating ? "You ran into your ex " + first(t) + " - with their NEW partner. Smile and wave." : "You ran into your ex " + first(t) + ". They look... good. Ugh.");
            addMoodlet(me(), dating ? "Jealous" : "Ex Run-In", dating ? EM_ANGRY : EM_UNCOMFORTABLE, 1, 360); break;
        }
    }
}

void dlyDaily() {
    // fridge
    int eaters = 0; for (const auto& s : G.sims) if (s.household) eaters++;
    D.fridge = (int16_t)std::max(0, D.fridge - std::max(1, eaters / 2));
    if (D.fridge == 0) toast("The fridge is empty! (Daily Life app > Grocery Delivery)");
    // sickness: winter colds feed the main health system (symptoms, sick days, doctor & ER all in health.cpp)
    for (int i = 0; i < (int)G.sims.size(); i++) { Sim& s = G.sims[i]; if (!s.household || isSick(i)) continue;
        if (irand(100) < (season() == 3 ? 3 : 1)) { catchIllness(i, IL_COLD); if (i == G.mc || irand(2)) toast(first(i) + " caught a cold! " + (s.stage >= ST_CHILD && s.stage <= ST_TEEN ? "Home from school." : "Calling in sick.")); } }
    // hair growth
    if (++D.hairDays >= 10) {
        D.hairDays = 0; const char* cur = HAIRSTYLE_NAMES[me().look.hairStyle % NUM_HAIRSTYLES];
        static const char* GROW[][2] = {{"Pixie", "Bob"}, {"Pixie Cut", "Bob"}, {"Short Crop", "Bob"}, {"Bob", "Sleek Long"}, {"Curly Bob", "Curls"}, {"Short Twists", "Twists"}, {"Buzz Cut", "Short Crop"}};
        for (auto& g : GROW) if (!strcmp(cur, g[0])) { setHair(g[1]); toast(std::string("Your hair grew out! Now it's a ") + g[1] + "."); break; }
    }
    // nails
    D.nails = (uint8_t)std::max(0, D.nails - 8); if (D.nails == 24) toast("Your nails are chipping. Time for a fill!");
    // weather outfits
    { const std::string on = OUTFITS[me().outfit % NUM_OUTFITS].name;
      auto has = [&](const char* k) { return on.find(k) != std::string::npos; };
      if ((G.weather == WX_SNOW || season() == 3) && (has("Swim") || has("Bikini") || has("Tank") || has("Shorts") || has("Summer"))) { addMoodlet(me(), "Freezing!", EM_UNCOMFORTABLE, 1, 720); toast("BRRR. You're dressed for summer in the snow."); }
      else if (G.weather == WX_HEAT && (has("Sweater") || has("Coat") || has("Puffer") || has("Fur") || has("Winter"))) { addMoodlet(me(), "Melting!", EM_UNCOMFORTABLE, 1, 720); toast("It's 100 degrees and you're in a sweater. Why."); } }
    // loans & credit
    if (D.loan > 0) { if (G.money >= D.loanPay) { G.money -= D.loanPay; D.loan = std::max<int32_t>(0, D.loan - D.loanPay); if (irand(5) == 0) D.credit = (int16_t)std::min(850, D.credit + 2); if (!D.loan) toast("Your loan is PAID OFF!"); }
                      else { D.credit = (int16_t)std::max(300, D.credit - 15); toast("Missed loan payment! Credit score dropped."); } }
    // business upkeep & reviews & HOA
    if (D.biz) D.bizRep = (uint8_t)std::max(0, D.bizRep - 1);
    if (me().career != CR_NONE && ++D.reviewDays == 7) toast("Your performance review is this week! (Daily Life app)");
    if (!D.hoaWarn && adult() && irand(100) < 4) { D.hoaWarn = 1; toast(irand(2) ? "HOA letter: \"Your party was TOO LOUD.\"" : "HOA letter: \"Your yard is a DISGRACE.\""); }
    // celebrity fades without effort; paparazzi pop up when famous
    if (D.fame > 0 && irand(3) == 0) D.fame--;
    if (D.fame >= 50 && irand(100) < 15) toast("Paparazzi outside your house AGAIN.");
}

void dlySaveTo(const std::string& p) { FILE* f = fopen(p.c_str(), "wb"); if (!f) return; fwrite(&D, sizeof D, 1, f); fclose(f); }
void dlyLoadFrom(const std::string& p) { D = DL{}; FILE* f = fopen(p.c_str(), "rb"); if (!f) return; DL d; if (fread(&d, 1, sizeof d, f) >= 8 && d.magic == D.magic) D = d; fclose(f); }
