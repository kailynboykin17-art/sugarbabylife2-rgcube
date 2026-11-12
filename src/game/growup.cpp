#include "levels.hpp"
#include "famdrama.hpp"
#include "growup.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "people.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include "tween.hpp"
#include "kidlife.hpp"
#include "deeplife.hpp"
#include <string>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include "vfs.hpp"
#define fopen vfopen   // sidecar saves go into the one-file save pack

extern bool g_tapSound;
namespace {
struct GU {
    uint32_t magic = 0x47553031;
    uint8_t allowance = 5, autonomy = 2, license = 0, car = 0, job = 0, clubs = 0, role = 0, pet = 0, petFood = 100, petClean = 100, petLow = 0;
    uint8_t imag = 0, bully = 0, nPhotos = 0, pierced = 0, choresToday = 0, pad[2] = {};
    int32_t savings = 0; uint32_t ach = 0;
    uint8_t prevStage[256] = {};
    char photos[24][44] = {};
    uint8_t ms[256] = {};   // per-sim life milestones (bits)
    uint8_t major = 0, chapter = 0, diff = 1, startSet = 0, perks = 0, willHeir = 0, hasWill = 0, believe = 0;
    uint64_t tips[2] = {};
    uint32_t storyDone = 0;
    uint8_t tapSound = 0;
};
GU U;
int rtStep = -1, rtPatience = 0;   // road trip (RAM)
int dqStep = -1, dqScore = 0;      // driver's test (RAM)
int pickMode = 0;                  // 1 playdate/sleepover, 2 prom-posal, 3 kid party
int friendSel = -1;

std::string first(int s) { const std::string& n = G.sims[s].name; return n.substr(0, n.find(' ')); }
bool kidSt(int st) { return st == ST_CHILD || st == ST_PRETEEN; }
bool hasKid() { for (auto& s : G.sims) if (s.household && s.stage >= ST_TODDLER && s.stage <= ST_TEEN) return true; return false; }
bool parent() { return MC().stage >= ST_YA; }

const char* ACH[] = {"First Allowance", "Honor Roll", "Party Host (Kid Bday)", "Sleepover Survivor", "Grandma's Favorite", "Road Trip Champ", "Imaginary Bestie",
                     "Licensed Driver", "First Car", "First Paycheck", "Prom-posal Pro", "Homecoming Royalty", "Club Kid", "Talent Show Star", "Stood Up to a Bully",
                     "5th Grade Grad", "8th Grade Grad", "High School Grad", "Pet Parent", "Back-to-School Ready", "Mall Rat", "Family Photographer"};
constexpr int NACH = sizeof ACH / sizeof ACH[0];
void ach(int k) { if (U.ach >> k & 1) return; U.ach |= 1u << k; toast(std::string("Life Checklist: ") + ACH[k] + "!"); sfxTap(); }

const char* CLUBS[5] = {"Cheer", "Band", "Art Club", "Drama Club", "Robotics"};
const uint8_t CLUBSK[5] = {SK_FITNESS, SK_CREATIVITY, SK_CREATIVITY, SK_CHARISMA, SK_LOGIC};
const char* PETS[4] = {"", "Hamster", "Goldfish", "Bunny"};
const char* JOBS[3] = {"", "Burger Barn crew", "Babysitter"};
const char* CARS[3] = {"Used Pink Hatchback", "Hand-Me-Down Minivan", "Little White Convertible"};
const char* IMAG[5] = {"", "Mr. Sparkles (a unicorn)", "Captain Bubbles (a pirate)", "Princess Pickle", "Gerald (a polite dinosaur)"};

std::vector<int> peers(int stage, bool teenOnly = false) {
    std::vector<int> v;
    for (int i = 0; i < (int)G.sims.size() && (int)v.size() < 150; i++)
        if (i != G.mc && !isFamily(G.mc, i) && (teenOnly ? G.sims[i].stage == ST_TEEN : G.sims[i].stage == stage)) v.push_back(i);
    return v;
}
LxMenu pickList(int mode, int stage, bool teen) { pickMode = mode; LxMenu m; for (int i : peers(stage, teen)) m.push_back({G.sims[i].name, 17600 + i});
    if (m.empty()) m.push_back({"(No one the right age around)", 17999}); return m; }

LxMenu roadTrip() {
    static const char* Q[5] = {"\"Are we there yet?\" (mile 12)", "\"I'm BORED.\" (mile 40)", "\"He's touching my side!\" (mile 75)", "\"I have to pee.\" (mile 110)", "\"ARE WE THERE YET?!\" (mile 150)"};
    LxMenu m; m.push_back({std::string(Q[rtStep]) + "  Patience " + std::to_string(rtPatience) + "%", 17999});
    m.push_back({"Play I Spy", 17061}); m.push_back({"Sing-Along", 17062}); m.push_back({"Hand Out Snacks", 17063}); m.push_back({"\"Don't make me turn this car around!\"", 17064});
    return m;
}
LxMenu drivers() {
    static const char* Q[5] = {"A red octagon sign means...", "Light turns yellow. You...", "Parallel parking! You...", "School bus with stop arm out. You...", "Merging onto the highway. You..."};
    static const char* A[5][3] = {{"Stop", "Slow down", "Honk"}, {"Speed up", "Stop if safe", "Close your eyes"}, {"Check mirrors, back in slow", "Just floor it", "Leave it in the street"},
                                  {"Pass carefully", "Stop and wait", "Honk to say hi"}, {"Match speed & signal", "Stop on the ramp", "Merge without looking"}};
    LxMenu m; m.push_back({std::string("Q") + std::to_string(dqStep + 1) + "/5: " + Q[dqStep], 17999});
    for (int i = 0; i < 3; i++) m.push_back({A[dqStep][i], 17101 + i});
    return m;
}
}

float guAutonomy() { static const float F[4] = {0, 0.5f, 1, 1.6f}; return F[U.autonomy & 3]; }
int guPhotoCount() { return U.nPhotos; }
void guPhoto(const std::string& cap) {
    if (U.nPhotos >= 24) { memmove(U.photos[0], U.photos[1], sizeof U.photos[0] * 23); U.nPhotos = 23; }
    char d[12]; snprintf(d, sizeof d, "Day %d: ", (int)G.day);
    snprintf(U.photos[U.nPhotos++], 44, "%s%s", d, cap.c_str());
    if (U.nPhotos >= 5) ach(21);
}

// ---------------------------------------------------------------- life moments, story mode, settings
enum { MS_WORD = 1, MS_STEPS = 2, MS_POTTY = 4, MS_KINDER = 8, MS_TOOTH = 16, MS_SANTA = 32 };
const char* MAJORS[7] = {"", "Fashion Design", "Nursing", "Business", "Psychology", "Film", "Undecided"};
struct Chapter { const char* name; const char* intro; int money; const char* goals[3]; };
const Chapter CH[5] = {
    {"", "", 0, {"", "", ""}},
    {"Single Mom of 3", "Three kids, one paycheck, zero sleep. You've got this, mama.", 1200, {"Throw a kid's birthday party", "Get a kid on the honor roll", "Save up $5,000"}},
    {"New Kid at School", "New town, new school, and nobody knows your name... yet.", 300, {"Make 3 friends (rel 40+)", "Join a club", "Win the talent show"}},
    {"Broke College Kid", "Ramen budget, big dreams. Pick a major and survive finals.", 150, {"Pick a college major", "Get a first car", "Make $2,000"}},
    {"Big Family Drama", "Everyone's under one roof. What could possibly go wrong?", 2500, {"Host a family reunion", "Take 5 family photos", "Have the whole family over 60 rel"}},
};
int friends40() { int n = 0; for (const Rel& r : G.rels) if ((r.a == G.mc || r.b == G.mc) && r.fr >= 40) n++; return n; }
bool goalMet(int c, int g) {
    switch (c * 3 + g) {
    case 3: return U.ach >> 2 & 1; case 4: return U.ach >> 1 & 1; case 5: return G.money >= 5000;
    case 6: return friends40() >= 3; case 7: return U.clubs != 0; case 8: return U.ach >> 13 & 1;
    case 9: return U.major != 0; case 10: return U.car != 0; case 11: return G.money >= 2000;
    case 12: return U.storyDone >> 31 & 1; case 13: return U.nPhotos >= 5;
    case 14: { int lo = 100; for (int i = 0; i < (int)G.sims.size(); i++) if (i != G.mc && G.sims[i].household) lo = std::min(lo, (int)rel(G.mc, i).fr); return lo >= 60; }
    }
    return false;
}
int achCount() { int n = 0; for (int i = 0; i < NACH; i++) n += U.ach >> i & 1; return n; }

LxMenu guApp() {
    LxMenu m; const Sim& me = MC(); const int st = me.stage;
    static const char* AU[4] = {"Off", "Low", "Normal", "High"};
    m.push_back({std::string("Autonomy: ") + AU[U.autonomy & 3] + " (tap to change)", 17400});
    m.push_back({"Photo Album (" + std::to_string(U.nPhotos) + ")", 17410});
    m.push_back({"Take a Family Photo", 17411});
    m.push_back({"Life Checklist (" + std::to_string(achCount()) + "/" + std::to_string(NACH) + ") & Perks", 17420});
    m.push_back({U.chapter ? std::string("Story: ") + CH[U.chapter].name : "Story Mode", 17430});
    m.push_back({"Difficulty & Money", 17440});
    m.push_back({std::string("Tap Sounds: ") + (U.tapSound ? "ON" : "OFF") + " (change)", 17450});
    m.push_back({"Family Dynamics", 17800});
    m.push_back({"Family Outings & Hangouts", 26000});
    m.push_back({"Family Game Night", 17700}); m.push_back({"Family Movie Night", 17701}); m.push_back({"Host a Family Reunion ($80)", 17702});
    if (st >= ST_YA) m.push_back({"Move Out: First Apartment", 17710});
    if (st == ST_YA) m.push_back({U.major ? std::string("College Major: ") + MAJORS[U.major] : "Pick a College Major", 17720});
    if (st >= ST_MIDDLE) m.push_back({U.hasWill ? "Will: written (update)" : "Write a Will", 17730});
    if (parent() && hasKid()) { m.push_back({"Allowance: $" + std::to_string(U.allowance) + "/chore (set)", 17010}); m.push_back({"Report Cards", 17020});
                                m.push_back({"Throw a Kid's Birthday Party ($60)", 17030}); m.push_back({"Back-to-School Shopping", 17320}); }
    if (st >= ST_CHILD && st <= ST_TEEN) m.push_back({"Do a Chore (earn allowance)", 17015});
    if (st >= ST_CHILD && st <= ST_TEEN) { m.push_back({"Playdate / Sleepover", 17040}); m.push_back({"Clubs", 17200}); m.push_back({"Talent Show", 17210}); m.push_back({"School Play Auditions", 17215}); }
    if (U.bully && st >= ST_CHILD && st <= ST_TEEN) m.push_back({"!! A kid is picking on your friend", 17220});
    m.push_back({"Grandparents Visit", 17050});
    m.push_back({"Family Road Trip", 17060});
    if (st == ST_TODDLER || st == ST_CHILD) m.push_back({U.imag ? std::string("Imaginary Friend: ") + IMAG[U.imag] : "Make an Imaginary Friend", 17070});
    if (st >= ST_CHILD && st <= ST_PRETEEN || hasKid()) m.push_back({U.pet ? std::string("Kid Pet: ") + PETS[U.pet] : "Get a Kid Pet", 17300});
    if (st == ST_PRETEEN || st == ST_TEEN) m.push_back({"Hang Out at the Mall", 17340});
    if (st == ST_TEEN) {
        m.push_back({U.license ? "Driver's License: YES" : "Take the Driver's Test", 17100});
        if (U.license) m.push_back({U.car ? std::string("My Car: ") + CARS[U.car - 1] : "Buy a First Car ($800)", 17110});
        m.push_back({U.job ? std::string("Job: ") + JOBS[U.job] + " - Work a Shift" : "Get a First Job", 17120});
        m.push_back({"Savings: $" + std::to_string(U.savings) + " (withdraw)", 17125});
        m.push_back({"Do a Prom-posal", 17130}); m.push_back({"Run for Homecoming Court", 17131});
        m.push_back({"Sneak Out Tonight", 17140});
    }
    return m;
}

LxMenu guOption(int id) {
    Sim& me = MC(); LxMenu m;
    switch (id) {
    case 17400: U.autonomy = (U.autonomy + 1) & 3; return guApp();
    case 17410: for (int i = U.nPhotos - 1; i >= 0; i--) m.push_back({U.photos[i], 17999}); if (m.empty()) m.push_back({"(No photos yet)", 17999}); return m;
    case 17411: { std::string who; int n = 0; for (int i = 0; i < (int)G.sims.size() && n < 4; i++) if (G.sims[i].household) { who += (n ? ", " : "") + first(i); n++; }
                  guPhoto("Family photo - " + who); for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household) tweenPlay(i, TW_CHEER); toast("Say cheese! Saved to the Photo Album."); return {}; }
    case 17420: {
        static const char* PK[4] = {"5 done: Money Bag (+$15/day)", "10 done: Social Butterfly (social never drops below 30)", "15 done: Teacher's Pet (+grades daily)", "22 done: LEGEND ($5,000 bonus)"};
        const int need[4] = {5, 10, 15, 22}; const int n = achCount();
        for (int i = 0; i < 4; i++) m.push_back({std::string(n >= need[i] ? "PERK ON: " : "Locked - ") + PK[i], 17999});
        for (int i = 0; i < NACH; i++) m.push_back({std::string(U.ach >> i & 1 ? "[X] " : "[ ] ") + ACH[i], 17999}); return m; }
    case 17430:
        if (U.chapter) { m.push_back({std::string(CH[U.chapter].name) + ":", 17999}); for (int g = 0; g < 3; g++) m.push_back({std::string(goalMet(U.chapter, g) ? "[X] " : "[ ] ") + CH[U.chapter].goals[g], 17999});
                         m.push_back({"Claim Chapter Reward", 17435}); m.push_back({"Quit This Story", 17436}); return m; }
        for (int c = 1; c < 5; c++) m.push_back({std::string(CH[c].name) + (U.storyDone >> c & 1 ? " (done)" : ""), 17430 + c}); return m;
    case 17431: case 17432: case 17433: case 17434: U.chapter = (uint8_t)(id - 17430); G.money = CH[U.chapter].money; toast(std::string(CH[U.chapter].name) + ": " + CH[U.chapter].intro); return {};
    case 17435: { bool all = true; for (int g = 0; g < 3; g++) all = all && goalMet(U.chapter, g); if (!all) { toast("Finish all 3 goals first!"); return {}; }
        G.money += 1500; U.storyDone |= 1u << U.chapter; toast(std::string("Chapter complete: ") + CH[U.chapter].name + "! +$1,500"); guPhoto(std::string("Finished: ") + CH[U.chapter].name); U.chapter = 0; return {}; }
    case 17450: U.tapSound ^= 1; g_tapSound = U.tapSound; toast(U.tapSound ? "Tap clicks ON." : "Tap clicks OFF."); return guApp();
    case 17436: U.chapter = 0; toast("Story ended. Back to free play."); return {};
    case 17440: { static const char* D[3] = {"Easy", "Normal", "Hard"};
        m.push_back({std::string("Difficulty: ") + D[U.diff % 3] + " (tap to change)", 17441});
        if (!U.startSet) { m.push_back({"Start Broke ($100)", 17442}); m.push_back({"Start Comfy ($5,000)", 17443}); m.push_back({"Start Rich ($50,000)", 17444}); }
        else m.push_back({"(Starting money already chosen for this save)", 17999});
        return m; }
    case 17441: U.diff = (U.diff + 1) % 3; toast(U.diff == 0 ? "Easy: +$20 a day." : U.diff == 2 ? "Hard: $25/day in extra bills, needs drain faster." : "Normal difficulty."); return guOption(17440);
    case 17442: case 17443: case 17444: { static const int M[3] = {100, 5000, 50000}; G.money = M[id - 17442]; U.startSet = 1; toast("Money set to $" + std::to_string(G.money) + "."); return {}; }
    case 17800: return famDynMenu(22000);
    case 17700: { static const char* GN[4] = {"Monopoly gets VICIOUS. Someone flips the board.", "Uno: Mom stacks four +4s. Betrayal.", "Charades: Dad acts out a toaster for 5 minutes.", "Pictionary - nobody can draw. Everyone wins."};
        for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household) { G.sims[i].needs[N_FUN] = std::min(100.f, G.sims[i].needs[N_FUN] + 30); if (i != G.mc) changeRel(G.mc, i, 4, 0); tweenPlay(i, TW_CLAP); }
        toast(std::string("Family Game Night! ") + GN[irand(4)]); return {}; }
    case 17701: { static const char* MV[4] = {"a scary movie (the little ones sleep in your bed)", "a princess movie - EVERYONE cries", "an action movie with way too much popcorn", "a home video marathon - baby pictures!"};
        for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household) { G.sims[i].needs[N_FUN] = std::min(100.f, G.sims[i].needs[N_FUN] + 25); if (i != G.mc) changeRel(G.mc, i, 3, 0); }
        toast(std::string("Movie night: ") + MV[irand(4)] + "."); return {}; }
    case 17702: { if (G.money < 80) { toast("Reunions cost $80."); return {}; } G.money -= 80;
        static const char* DR[4] = {"Aunt Denise brings up the 2009 wedding incident. AGAIN.", "Your cousin announces an engagement - and steals the spotlight.", "Uncle Ray's potato salad sits in the sun too long.", "Grandma finally reveals who broke the antique vase."};
        int n = 0; for (int i = 0; i < (int)G.sims.size(); i++) if (i != G.mc && isFamily(G.mc, i)) { changeRel(G.mc, i, 5, 0); n++; }
        toast("Family reunion! " + std::to_string(n + 6) + " relatives, 3 casseroles and 1 cousin you forgot existed."); toast(DR[irand(4)]);
        guPhoto("Family reunion group photo"); U.storyDone |= 1u << 31; return {}; }
    case 17710: m.push_back({"Move Out Alone", 17711}); for (const Rel& r : G.rels) if ((r.a == G.mc || r.b == G.mc) && r.status >= RS_DATING && r.status <= RS_MARRIED) { const int p = r.a == G.mc ? r.b : r.a; if (p < 250) m.push_back({"Move In Together with " + G.sims[p].name, 17712 + 0 * p}); friendSel = p; break; } return m;
    case 17711: case 17712: { if (G.money < 500) { toast("Deposit + first month: $500."); return {}; }
        const int mover = G.mc; const int partner = id == 17712 ? friendSel : -1; G.money -= 500;
        const int32_t take = std::min<int32_t>(G.money, 1500 + G.money / 4);
        for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household && i != mover && i != partner && G.sims[i].stage >= ST_ADULT) addMoodlet(G.sims[i], "Empty Nest Feels", EM_SAD, 1, 1440);
        if (!dlMoveOut(mover, partner, take)) { G.money += 500; toast("You're the only grown-up here - someone has to stay! (Use Realty to move the whole family instead.)"); return {}; }
        addMoodlet(MC(), "My Own Place!", EM_HAPPY, 3, 2880); guPhoto(partner >= 0 ? "Moving in together" : "Moving into my first apartment");
        toast("Keys in hand! You took $" + std::to_string(take) + " to your new place. Your family still lives at the old house (Households app to visit them)."); return {}; }
    case 17720: for (int i = 1; i < 7; i++) m.push_back({MAJORS[i], 17720 + i}); return m;
    case 17721: case 17722: case 17723: case 17724: case 17725: case 17726: U.major = (uint8_t)(id - 17720); toast(std::string("Declared major: ") + MAJORS[U.major] + "! (Finals week every 2 weeks - study at the Dorm)"); return {};
    case 17730: for (int i = 0; i < (int)G.sims.size(); i++) if (i != G.mc && isFamily(G.mc, i) && i < 250) m.push_back({"Leave everything to " + G.sims[i].name, 17740 + i});
                if (m.empty()) m.push_back({"(No family to leave it to)", 17999}); return m;
    // allowance & chores
    case 17010: for (int a : {0, 2, 5, 10, 20}) m.push_back({"$" + std::to_string(a) + " per chore", 17011 + std::min(4, a / 5 + (a == 2))}); return m;
    case 17011: case 17012: case 17013: case 17014: case 17016: break;
    case 17015:
        if (U.choresToday >= 3) { toast("Chore chart's done for today!"); return {}; }
        U.choresToday++; lxClean(20); me.needs[N_FUN] = std::max(0.f, me.needs[N_FUN] - 8); G.money += U.allowance; U.savings += 0;
        toast("Chore chart: +1 gold star! Earned $" + std::to_string(U.allowance) + "."); if (U.allowance) ach(0); return {};
    // report cards
    case 17020: for (int i = 0; i < (int)G.sims.size(); i++) { const Sim& s = G.sims[i]; if (s.household && s.stage >= ST_CHILD && s.stage <= ST_TEEN) m.push_back({first(i) + " - " + schoolName(s.stage) + ": " + letterGrade(s.gradePts), 17999}); }
                m.push_back({"Parent-Teacher Conference", 17021}); return m;
    case 17021: {
        int worst = -1; for (int i = 0; i < (int)G.sims.size(); i++) { const Sim& s = G.sims[i]; if (s.household && s.stage >= ST_CHILD && s.stage <= ST_TEEN && (worst < 0 || s.gradePts < G.sims[worst].gradePts)) worst = i; }
        if (worst < 0) return {};
        Sim& k = G.sims[worst];
        if (k.gradePts >= 75) { toast("Teacher: \"" + first(worst) + " is a joy to have in class!\""); addMoodlet(k, "Teacher Bragged on Me", EM_CONFIDENT, 1, 720); changeRel(G.mc, worst, 4, 0); if (k.gradePts >= 90) ach(1); }
        else { toast("Teacher: \"" + first(worst) + " has been... chatty. And missing homework.\" The car ride home is SILENT."); k.gradePts = std::min(100.f, k.gradePts + 8);
               changeRel(G.mc, worst, -5, 0); addMoodlet(k, "Busted at Conference", EM_ANGRY, 1, 720); }
        return {}; }
    // kid birthday party
    case 17030: { if (G.money < 60) { toast("Parties cost $60."); return {}; }
        int kid = -1; for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household && G.sims[i].stage >= ST_TODDLER && G.sims[i].stage <= ST_PRETEEN) { kid = i; break; }
        if (kid < 0) { toast("No little kid to throw a party for!"); return {}; }
        G.money -= 60; Sim& k = G.sims[kid]; int guests = 0;
        for (int i : peers(k.stage)) { if (guests >= 5) break; changeRel(kid, i, 5, 0); guests++; }
        G.toys |= 1u << irand(12); addMoodlet(k, "Best Birthday Party Ever", EM_HAPPY, 3, 1440); tweenPlay(kid, TW_CHEER);
        toast("Cake, balloons, " + std::to_string(guests) + " classmates, a pinata & goodie bags! " + first(kid) + " got a new toy!");
        guPhoto(first(kid) + "'s birthday party"); ach(2); return {}; }
    // playdates & sleepovers
    case 17040: return pickList(1, me.stage, false);
    case 17045: case 17046: { if (friendSel < 0) return {}; const int f = friendSel; changeRel(G.mc, f, id == 17046 ? 10 : 6, 0); me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 30);
        me.needs[N_SOCIAL] = std::min(100.f, me.needs[N_SOCIAL] + 35);
        if (id == 17045) toast("Playdate with " + first(f) + ": tag, snacks and a blanket fort!");
        else { static const char* S[4] = {"pillow fight", "scary stories with a flashlight", "prank calls on the toy phone", "face masks and gossip"};
               toast("Sleepover with " + first(f) + ": " + S[irand(4)] + "! Nobody slept."); me.needs[N_ENERGY] = std::max(0.f, me.needs[N_ENERGY] - 25); guPhoto("Sleepover with " + first(f)); ach(3); }
        return {}; }
    // grandparents
    case 17050: { static const char* ST[4] = {"\"Back in MY day, we walked to school uphill both ways!\"", "\"Your mom was the WILDEST teenager, let me tell you...\"",
                                              "\"This was your great-grandma's recipe. Now it's yours.\"", "\"I met your grandpa at a roller rink in 1979.\""};
        const int gift = 10 + irand(20); G.money += gift; me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 15);
        toast(std::string("Grandma & Grandpa visit! ") + ST[irand(4)]); toast("They sneak you $" + std::to_string(gift) + " and cookies. Don't tell your parents.");
        addMoodlet(me, "Spoiled by Grandparents", EM_HAPPY, 2, 720); guPhoto("Grandparents' visit"); ach(4); return {}; }
    // road trip minigame
    case 17060: if (G.money < 40) { toast("Gas money: $40."); return {}; } G.money -= 40; rtStep = 0; rtPatience = 70; return roadTrip();
    case 17061: case 17062: case 17063: case 17064: {
        if (rtStep < 0) return {};
        static const int BEST[5] = {17061, 17062, 17063, 17063, 17064};   // what works best each leg
        rtPatience += id == BEST[rtStep] ? 15 : id == 17064 ? -10 : -3; rtPatience = std::max(0, std::min(100, rtPatience));
        toast(id == BEST[rtStep] ? "That worked! Peace in the back seat." : "...the whining continues.");
        if (++rtStep < 5 && rtPatience > 0) return roadTrip();
        rtStep = -1;
        if (rtPatience > 0) { toast("WE'RE HERE! Beach, boardwalk & ice cream. Patience left: " + std::to_string(rtPatience) + "%");
            for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household) { addMoodlet(G.sims[i], "Family Road Trip", EM_HAPPY, 2, 1440); G.sims[i].needs[N_FUN] = 100; }
            guPhoto("Family road trip"); if (rtPatience >= 60) ach(5); }
        else toast("Mom/Dad turned the car around. Everyone's grounded from fun.");
        return {}; }
    // imaginary friend
    case 17070: if (!U.imag) { U.imag = 1 + irand(4); toast(std::string("Meet your imaginary friend: ") + IMAG[U.imag] + "!"); ach(6); return guApp(); }
        m.push_back({"Have a Tea Party Together", 17071}); m.push_back({"Blame Them for the Mess", 17072}); m.push_back({"Say Goodbye (growing up)", 17073}); return m;
    case 17071: me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 25); me.needs[N_SOCIAL] = std::min(100.f, me.needs[N_SOCIAL] + 15); toast(std::string(IMAG[U.imag]) + " pours invisible tea. Delicious."); return {};
    case 17072: toast(irand(2) ? "\"It wasn't me, it was " + std::string(IMAG[U.imag]) + "!\" ...Somehow it works." : "Nobody believes you. Time-out."); return {};
    case 17073: toast(std::string(IMAG[U.imag]) + " waves goodbye. You'll always remember them."); U.imag = 0; return {};
    // driver's test
    case 17100: if (U.license) return {}; dqStep = 0; dqScore = 0; return drivers();
    case 17101: case 17102: case 17103: {
        static const int OK[5] = {0, 1, 0, 1, 0};
        if (dqStep < 0) return {};
        if (id - 17101 == OK[dqStep]) dqScore++;
        if (++dqStep < 5) return drivers();
        dqStep = -1;
        if (dqScore >= 4) { U.license = 1; toast("PASSED! " + std::to_string(dqScore) + "/5. The DMV photo is terrible but you're LICENSED."); guPhoto("Got my driver's license!"); ach(7); }
        else toast("Failed (" + std::to_string(dqScore) + "/5). The instructor needs a minute.");
        return {}; }
    case 17110: if (U.car) { toast("Cruising in your " + std::string(CARS[U.car - 1]) + "!"); me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 15); return {}; }
        for (int i = 0; i < 3; i++) m.push_back({std::string(CARS[i]) + " $800", 17111 + i}); return m;
    case 17111: case 17112: case 17113: { int have = G.money + U.savings; if (have < 800) { toast("Need $800 (wallet + savings)."); return {}; }
        int fromSav = std::min((int)U.savings, 800); U.savings -= fromSav; G.money -= 800 - fromSav; U.car = (uint8_t)(id - 17110);
        toast(std::string("Your first car: ") + CARS[U.car - 1] + "!"); guPhoto("My first car"); ach(8); return {}; }
    // first job
    case 17120: if (!U.job) { m.push_back({"Apply: Burger Barn", 17121}); m.push_back({"Apply: Babysitting", 17122}); return m; }
        { const int pay = U.job == 1 ? 32 : 40; U.savings += pay; me.needs[N_ENERGY] = std::max(0.f, me.needs[N_ENERGY] - 20);
          toast(U.job == 1 ? "Shift at Burger Barn: you smell like fries. +$" + std::to_string(pay) + " to savings." : "Babysat the Millers' twins. Survived. +$" + std::to_string(pay) + " to savings.");
          ach(9); return {}; }
    case 17121: case 17122: U.job = (uint8_t)(id - 17120); toast(std::string("Hired as ") + JOBS[U.job] + "!"); return guApp();
    case 17125: if (U.savings <= 0) return {}; G.money += U.savings; toast("Withdrew $" + std::to_string(U.savings) + " from savings."); U.savings = 0; return {};
    // prom-posal & homecoming
    case 17130: return pickList(2, ST_TEEN, true);
    case 17131: { int pop = 0; for (const Rel& r : G.rels) if ((r.a == G.mc || r.b == G.mc) && r.fr > 40) pop++;
        if (pop >= 6 || irand(100) < 10 + pop * 8) { toast("You're on HOMECOMING COURT! Crown, sash, the whole thing."); addMoodlet(me, "Homecoming Royalty", EM_CONFIDENT, 3, 2880); guPhoto("Homecoming court"); ach(11); }
        else toast("Votes are in... you didn't make court. There's always next year."); return {}; }
    case 17140: { if (G.mcGrounded) toast("You're grounded... you sneak out ANYWAY.");
        me.needs[N_FUN] = 100; me.needs[N_ENERGY] = std::max(0.f, me.needs[N_ENERGY] - 30); addMoodlet(me, "Snuck Out", EM_PLAYFUL, 2, 720);
        if (irand(100) < 40) { toast("Mrs. Kowalski next door saw you climb out the window... and she TALKS."); lxGossip(me.name + " was seen sneaking out at midnight", true, CI_FAMILY, -1);
            for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household && G.sims[i].stage >= ST_ADULT && i != G.mc) changeRel(G.mc, i, -8, 0);
            G.mcGrounded = 5; toast("Your parents found out. GROUNDED for 5 days."); }
        else toast("Out all night with friends - and nobody saw. This time.");
        return {}; }
    // clubs / talent / play / bully
    case 17200: for (int i = 0; i < 5; i++) m.push_back({std::string(U.clubs >> i & 1 ? "[In] " : "[Join] ") + CLUBS[i], 17201 + i}); return m;
    case 17201: case 17202: case 17203: case 17204: case 17205: U.clubs ^= 1 << (id - 17201); if (U.clubs) ach(12); return guOption(17200);
    case 17210: m.push_back({"Sing", 17211}); m.push_back({"Dance", 17212}); m.push_back({"Magic Tricks", 17213}); m.push_back({"Stand-Up Comedy", 17214}); return m;
    case 17211: case 17212: case 17213: case 17214: {
        static const uint8_t SK[4] = {SK_CHARISMA, SK_FITNESS, SK_LOGIC, SK_CHARISMA};
        const int score = (int)me.skills[SK[id - 17211]] * 8 + irand(40);
        if (score > 55) { toast("Standing ovation! You WON the talent show!"); addMoodlet(me, "Talent Show Winner", EM_CONFIDENT, 2, 1440); guPhoto("Won the talent show"); ach(13); }
        else if (score > 30) toast("Solid applause. 2nd place!"); else toast("Your mic cut out. Someone's little brother clapped."); return {}; }
    case 17215: { static const char* R[4] = {"Tree #3", "the Narrator", "the Lead!", "Understudy"}; const int r = std::min(3, (int)(me.skills[SK_CHARISMA] / 3) + irand(2));
        U.role = r; toast(std::string("Cast list is up... you got ") + R[r]); if (r == 2) guPhoto("Got the lead in the school play"); return {}; }
    case 17220: m.push_back({"Stand Up for Your Friend", 17221}); m.push_back({"Tell a Teacher", 17222}); m.push_back({"Walk Away", 17223}); return m;
    case 17221: U.bully = 0; addMoodlet(me, "Stood Up for a Friend", EM_CONFIDENT, 2, 1440); toast("You said \"Leave them alone!\" - and the whole lunch table backed you up."); ach(14); return {};
    case 17222: U.bully = 0; toast("The teacher handled it. Your friend says thanks."); me.gradePts = std::min(100.f, me.gradePts + 2); return {};
    case 17223: U.bully = 0; addMoodlet(me, "Should've Said Something", EM_SAD, 1, 720); toast("You walked away. It doesn't feel great."); return {};
    // kid pets
    case 17300: if (!lvlAllows(LF_PETS)) { toast("Pets unlock at Level 4 (Furpals!)."); return {}; } if (!U.pet) { m.push_back({"Hamster $20", 17301}); m.push_back({"Goldfish $10", 17302}); m.push_back({"Bunny $35", 17303}); return m; }
        m.push_back({std::string("Food ") + std::to_string(U.petFood) + "%  Clean " + std::to_string(U.petClean) + "%", 17999});
        m.push_back({"Feed", 17305}); m.push_back({"Clean the Habitat", 17306}); m.push_back({"Play / Cuddle", 17307}); return m;
    case 17301: case 17302: case 17303: { static const int P[3] = {20, 10, 35}; const int k = id - 17300; if (G.money < P[k - 1]) { toast("Not enough money."); return {}; }
        G.money -= P[k - 1]; U.pet = (uint8_t)k; U.petFood = U.petClean = 100; U.petLow = 0; toast(std::string("Welcome home, little ") + PETS[k] + "! Keep it fed and clean."); guPhoto(std::string("New ") + PETS[k]); ach(18); return {}; }
    case 17305: U.petFood = 100; toast("Nom nom."); return {};
    case 17306: U.petClean = 100; lxClean(5); toast("Fresh bedding! Your pet does a happy wiggle."); return {};
    case 17307: me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 15); addMoodlet(me, "Pet Cuddles", EM_HAPPY, 1, 240); toast(std::string("Your ") + PETS[U.pet] + " is the cutest."); return {};
    // back to school
    case 17320: { static const char* B[5] = {"Glitter Backpack", "Rolling Backpack", "Mini Leather Backpack", "Cat-Ear Backpack", "Sporty Drawstring"}; for (int i = 0; i < 5; i++) m.push_back({std::string(B[i]) + " + Supplies $45", 17321 + i}); return m; }
    case 17321: case 17322: case 17323: case 17324: case 17325: if (G.money < 45) { toast("Need $45."); return {}; } G.money -= 45;
        for (int i = 0; i < (int)G.sims.size(); i++) { Sim& s = G.sims[i]; if (s.household && s.stage >= ST_CHILD && s.stage <= ST_TEEN) { s.gradePts = std::min(100.f, s.gradePts + 4); addMoodlet(s, "Ready for School", EM_CONFIDENT, 1, 2880); } }
        toast("New backpack, gel pens, binders & a lunchbox. Back-to-school READY!"); ach(19); return {};
    // mall
    case 17340: m.push_back({"Food Court ($8)", 17341}); m.push_back({"Photo Booth ($5)", 17342}); m.push_back({"Ear Piercing Kiosk ($25)", 17343}); m.push_back({"Window Shop & People-Watch", 17344}); return m;
    case 17341: if (G.money < 8) return {}; G.money -= 8; me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + 45); me.needs[N_SOCIAL] = std::min(100.f, me.needs[N_SOCIAL] + 15); toast("Pretzel bites, smoothie & mall gossip."); ach(20); return {};
    case 17342: if (G.money < 5) return {}; G.money -= 5; guPhoto("Mall photo booth strip"); tweenPlay(G.mc, TW_HOP); toast("Four poses: silly, cute, peace sign, blurry. Saved to the album."); ach(20); return {};
    case 17343: if (G.money < 25) return {}; if (me.stage == ST_PRETEEN && irand(2)) { toast("The kiosk lady: \"I need a parent's signature, sweetie.\""); return {}; }
        G.money -= 25; U.pierced = 1; if (!me.look.f.pierce) me.look.f.pierce = 1; addMoodlet(me, "New Piercings!", EM_CONFIDENT, 1, 1440); toast("OW. ...okay, they look SO cute."); ach(20); return {};
    case 17344: me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 10); toast("You try on sunglasses at every kiosk."); ach(20); return {};
    }
    if (id >= 17011 && id <= 17016) { static const int A[6] = {0, 2, 5, 10, 20, 0}; U.allowance = (uint8_t)A[std::min(4, id - 17011)]; toast("Allowance set to $" + std::to_string(U.allowance) + " per chore."); return {}; }
    if (id >= 17740 && id < 17990 && id - 17740 < (int)G.sims.size()) { U.willHeir = (uint8_t)(id - 17740); U.hasWill = 1; toast("Will signed and notarized. " + first(U.willHeir) + " is your heir."); return {}; }
    if (id >= 17600 && id < 17600 + (int)G.sims.size()) {
        const int t = id - 17600;
        if (pickMode == 1) { friendSel = t; m.push_back({"Playdate with " + first(t), 17045}); m.push_back({"Sleepover with " + first(t), 17046}); return m; }
        if (pickMode == 2) {
            static const char* P[3] = {"a giant poster in the hallway", "a pizza box that says \"PROM? I'M SAUCY ABOUT YOU\"", "a flash mob in the cafeteria"};
            if (irand(100) < 40 + rel(G.mc, t).fr / 2) { toast("You asked " + first(t) + " with " + P[irand(3)] + "... they said YES!"); changeRel(G.mc, t, 8, 4); guPhoto("Prom-posal with " + first(t)); ach(10); }
            else { toast(first(t) + " already has a date. Brutal."); addMoodlet(me, "Prom-posal Rejected", EM_SAD, 1, 1440); }
            return {};
        }
    }
    return {};
}

void guHourly(int hour) {
    Sim& me = MC();
    if (hour == 16 && U.clubs && isWeekday(G.day) && me.stage >= ST_CHILD && me.stage <= ST_TEEN)
        for (int i = 0; i < 5; i++) if (U.clubs >> i & 1) { me.skills[CLUBSK[i]] = std::min(10.f, me.skills[CLUBSK[i]] + 0.2f); me.needs[N_SOCIAL] = std::min(100.f, me.needs[N_SOCIAL] + 5); toast(std::string(CLUBS[i]) + " after school!"); break; }
}

void guDaily() {
    U.choresToday = 0;
    if (U.pet) { U.petFood = (uint8_t)std::max(0, U.petFood - 30); U.petClean = (uint8_t)std::max(0, U.petClean - 20);
        if (U.petFood == 0 || U.petClean == 0) { if (++U.petLow >= 3) { toast(std::string("Your ") + PETS[U.pet] + " went to live at Grandma's where it'll get more attention."); U.pet = 0; } else toast(std::string("Your ") + PETS[U.pet] + " needs care! (Grow Up app > Kid Pet)"); }
        else U.petLow = 0; }
    const Sim& me = MC();
    if (!U.bully && me.stage >= ST_CHILD && me.stage <= ST_TEEN && irand(100) < 6 * guAutonomy() + 2) { U.bully = 1; toast("Someone at school keeps picking on your friend... (Grow Up app)"); }
    // graduations
    for (int i = 0; i < (int)G.sims.size() && i < 256; i++) {
        const Sim& s = G.sims[i]; const uint8_t was = U.prevStage[i]; U.prevStage[i] = (uint8_t)(s.stage + 1);
        if (!was || !s.household || was - 1 == s.stage) continue;
        const char* what = was - 1 == ST_CHILD && s.stage == ST_PRETEEN ? "5th Grade Graduation" : was - 1 == ST_PRETEEN && s.stage == ST_TEEN ? "8th Grade Promotion" : was - 1 == ST_TEEN && s.stage == ST_YA ? "HIGH SCHOOL GRADUATION" : nullptr;
        if (!what) continue;
        toast(std::string(what) + "! " + first(i) + " walks the stage while the family cheers.");
        guPhoto(first(i) + "'s " + what);
        if (i == G.mc) ach(s.stage == ST_PRETEEN ? 15 : s.stage == ST_TEEN ? 16 : 17);
    }
    // difficulty & perks
    if (U.diff == 0) G.money += 20;
    if (U.diff == 2) { G.money = G.money > 25 ? G.money - 25 : 0; for (auto& s : G.sims) if (s.household) s.needs[N_ENERGY] = std::max(0.f, s.needs[N_ENERGY] - 5); }
    { const int n = achCount(); Sim& m2 = MC();
      if (n >= 5) G.money += 15;
      if (n >= 10) m2.needs[N_SOCIAL] = std::max(30.f, m2.needs[N_SOCIAL]);
      if (n >= 15 && m2.stage >= ST_CHILD && m2.stage <= ST_TEEN) m2.gradePts = std::min(100.f, m2.gradePts + 1);
      if (n >= 22 && !(U.perks & 1)) { U.perks |= 1; G.money += 5000; toast("LEGEND perk unlocked: +$5,000!"); } }
    // college finals
    if (U.major && MC().stage == ST_YA && G.day % 14 == 13) toast(std::string("FINALS WEEK for ") + MAJORS[U.major] + " majors! Study at the College Dorm.");
    // baby & kid milestones
    for (int i = 0; i < (int)G.sims.size() && i < 256; i++) {
        Sim& s = G.sims[i]; if (!s.household) continue; uint8_t& f = U.ms[i]; const std::string n = first(i);
        auto hit = [&](int bit, int chance, const std::string& msg, const std::string& photo) { if (f & bit || irand(100) >= chance) return false; f |= bit; toast(msg); if (!photo.empty()) guPhoto(photo); addMoodlet(s, "Milestone!", EM_HAPPY, 1, 720); return true; };
        if (s.stage == ST_INFANT) { static const char* W[4] = {"\"Mama!\"", "\"Dada!\"", "\"Uh-oh!\"", "\"NO!\""}; if (hit(MS_WORD, 12, n + "'s first word: " + W[irand(4)], n + "'s first word")) break; }
        if (s.stage == ST_TODDLER) {
            if (hit(MS_STEPS, 15, n + " took their FIRST STEPS! Everyone screams.", n + "'s first steps")) break;
            if (f & MS_STEPS && hit(MS_POTTY, 10, n + " is officially potty trained! Sticker chart complete.", "")) break;
        }
        if (s.stage == ST_CHILD) {
            if (s.stageDays > 1) f |= MS_KINDER;
            if (hit(MS_KINDER, 100, "First day of kindergarten! " + n + " has a brand-new backpack and a brave face.", n + "'s first day of kindergarten")) break;
            if (!(f & MS_TOOTH) && irand(100) < 4) { f |= MS_TOOTH; toast(n + " lost a tooth! The Tooth Fairy left $2 under the pillow."); if (i == G.mc) G.money += 2; }
            else if (f & MS_TOOTH && irand(100) < 3) { toast(n + " lost another tooth. The Tooth Fairy is going broke. +$2"); if (i == G.mc) G.money += 2; }
            if (!(f & MS_SANTA) && s.stageDays > 3 && irand(100) < 2) { f |= MS_SANTA; toast(n + " found the Santa presents in the closet... the magic is OVER."); addMoodlet(s, "Santa Isn't Real?!", EM_SAD, 2, 1440); }
        }
        if (s.stage == ST_CHILD && seasonOf(G.day) == 3 && G.day % 7 == 6 && !(f & MS_SANTA)) toast(n + " left cookies out for Santa. Still believes!");
    }
    if (MC().gradePts >= 90 && MC().stage >= ST_CHILD && MC().stage <= ST_TEEN) ach(1);
    if (U.job && irand(4) == 0) {}
}

void guOnDeath(int s) {
    // every family elder leaves a will: they name one heir (the relative they're closest to), who gets the estate
    if (s < 0 || s >= (int)G.sims.size() || !isFamily(G.mc, s) || G.sims[s].stage < ST_MIDDLE) return;
    const int estate = 500 + irand(4500);
    int heir = -1, best = -999;
    for (int i = 0; i < (int)G.sims.size(); i++) if (i != s && isFamily(s, i) && G.sims[i].stage >= ST_TEEN) { const int r = rel(s, i).fr + (G.sims[i].household ? 10 : 0) + irand(15); if (r > best) { best = r; heir = i; } }
    if (heir < 0) heir = G.mc;
    guPhoto("Remembering " + first(s));
    if (heir == G.mc) { G.money += estate; toast("The will is read: " + first(s) + " left YOU everything - $" + std::to_string(estate) + " and a box of old photos."); }
    else {
        toast("The will is read: " + first(s) + " left the estate ($" + std::to_string(estate) + ") to " + first(heir) + ". You got their old jewelry box.");
        if (rel(G.mc, heir).fr < 40) { changeRel(G.mc, heir, -10, 0); toast("Family drama: you think " + first(heir) + " sweet-talked their way into that will..."); }
        else if (G.sims[heir].household) G.money += estate;   // shared household money
    }
}
int guWillHeir() { return U.hasWill ? U.willHeir : -1; }
bool guTipSeen(int i) { return i < 0 || i >= 128 ? true : (U.tips[i / 64] >> (i % 64)) & 1; }
void guTipMark(int i) { if (i >= 0 && i < 128) U.tips[i / 64] |= 1ull << (i % 64); }
void guSaveTo(const std::string& p) { FILE* f = fopen(p.c_str(), "wb"); if (!f) return; fwrite(&U, sizeof U, 1, f); fclose(f); }
extern bool g_tapSound;
void guLoadFrom(const std::string& p) { U = GU{}; g_tapSound = false; FILE* f = fopen(p.c_str(), "rb"); if (!f) return; GU u; if (fread(&u, 1, sizeof u, f) >= 8 && u.magic == U.magic) U = u; else U = GU{}; fclose(f); g_tapSound = U.tapSound; }
