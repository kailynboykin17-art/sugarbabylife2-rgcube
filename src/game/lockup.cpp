#include "lifeplus2.hpp"
#include "lifeext.hpp"
#include "spicy.hpp"
#include "hotdate.hpp"
#include "challenge.hpp"
#include "travelcourt.hpp"
// Jail, boarding school and the fist-fight dust cloud.
#include "lockup.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include "goals.hpp"
#include "crime.hpp"
#include "district.hpp"
#include "lifestory.hpp"
#include "travelcourt.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>
using namespace col;
void toast(const std::string& m);
void travelTo(int room);
void say(int simIdx, const std::string& line);
void fxBurst(float x, float y, int n);
bool momentActive();
typedef std::string (*MomentFn)(int, int);
void momentCustom(const std::string& title, const std::string& text, const char* o0, const char* o1, const char* o2, MomentFn fn, int ctx, int portraitSim);

namespace {
bool g_moving = false;   // our own transfers skip the travel gates
void moveTo(int room) { g_moving = true; travelTo(room); g_moving = false; }
int hourNowL() { return (int)(G.minute / 60); }
bool juvie() { return MC().stage <= ST_TEEN; }
int bailCost() { int days = jailed() ? std::max(1, (int)G.jailUntil - (int)G.day) : 1; return 150 * days + 50 * std::min<int>(G.rapSheet, 10); }
bool hasGrownUp() { for (size_t i = 0; i < G.sims.size(); i++) if (G.sims[i].household && (int)i != G.mc && G.sims[i].isAdult()) return true; return false; }
bool student() { const Sim& me = MC(); return me.stage >= ST_CHILD && me.stage <= ST_TEEN; }
}

bool jailed() { return G.jailUntil != 0xFFFF; }
bool boarding() { return G.boardEnd != 0xFFFF; }

bool lockupBlocks(int room, std::string* why) {
    if (g_moving || G.sims.empty()) return false;
    if (hotdateBlocks(room, why)) return true;
    if (spicyBlocks(room, why)) return true;
    if (lxBlocks(room, why)) return true;
    if (lpBlocks(room, why)) return true;
    if (jailed() && room != R_JAIL) {
        if (why) *why = juvie() ? "You're in juvie! Wait it out or get someone to post bail." : "You're locked up! Serve your time or post bail at the guard desk.";
        return true;
    }
    if (!jailed() && room == R_JAIL) { if (why) *why = "You don't want to go back in there."; return true; }
    if (G.vacDest && !vacationTripRoom(room) && room != R_JAIL) {
        if (why) *why = std::string("You're on vacation in ") + vacationName() + "! Stay and enjoy it - or fly home from your hotel suite to end the trip.";
        return true;
    }
    if (boarding() && isWeekday(G.day) && room != R_BOARDING && room != R_JAIL) {
        if (why) *why = "Rosewood rules: no leaving campus on school days. Weekends only!";
        return true;
    }
    if (room == R_BOARDING && !boarding() && !MC().isAdult() && !student()) { if (why) *why = "Rosewood is for students (and their parents)."; return true; }
    return false;
}

// ---------------------------------------------------------------- jail
void jailSentence(int days, const char* why) {
    Sim& me = MC();
    if (me.stage < ST_TEEN) { G.mcGrounded = 7; toast("You're too young for jail... so you're grounded for a WEEK instead."); return; }
    days = std::max(1, std::min(days, 7));
    G.jailUntil = (uint16_t)(G.day + days);
    lxNews(me.name.substr(0, me.name.find(' ')) + " arrested for " + why);
    if (G.vacDest) G.vacDest = 0;
    cancelActivity();
    moveTo(R_JAIL);
    addMoodlet(me, juvie() ? "Stuck in Juvie" : "Locked Up", EM_SAD, 3, days * 1440);
    char b[200];
    snprintf(b, sizeof b, "%s for %s: %d day%s. The door clangs shut behind you.", juvie() ? "Juvenile detention" : "Westbrook County Jail", why, days, days == 1 ? "" : "s");
    toast(b); sfxNo();
    addMemoryOnce(MEM_FUN, G.mc, -1, juvie() ? "Spent time in juvie. Not my proudest moment." : "Did time in the county jail. Never again.");
}
static void release(const char* how) {
    G.jailUntil = 0xFFFF;
    Sim& me = MC();
    me.moodlets.erase(std::remove_if(me.moodlets.begin(), me.moodlets.end(), [](const Moodlet& m) { return m.name == "Locked Up" || m.name == "Stuck in Juvie"; }), me.moodlets.end());
    addMoodlet(me, "Free at Last!", EM_HAPPY, 2, 720);
    toast(how); sfxYay();
    if (G.room == R_JAIL) moveTo(R_LIVING);
}

// ---------------------------------------------------------------- boarding school
void boardingEnroll(bool sentByParents) {
    G.boardEnd = (uint16_t)(G.day + 14);
    Sim& me = MC();
    addMoodlet(me, sentByParents ? "Shipped Off to Rosewood" : "New School, New Me", sentByParents ? EM_SAD : EM_CONFIDENT, 2, 1440);
    addMemoryOnce(MEM_FUN, G.mc, -1, sentByParents ? "My parents sent me to Rosewood Academy boarding school." : "Started at Rosewood Academy boarding school.");
    toast("Welcome to Rosewood Academy! On school days you live in the dorm; weekends you can go home.");
    if (isWeekday(G.day)) moveTo(R_BOARDING);
}
static std::string parentsSend(int ch, int) {
    if (ch == 0) { boardingEnroll(true); return "You pack a suitcase. Your mom cries at the gate. Your dad pretends he isn't."; }
    if (ch == 1) {
        if (irand(100) < 35 + skillLevel(MC(), SK_CHARISMA) * 4) { MC().gradePts = std::min(100.f, MC().gradePts + 5); return "\"...ONE more chance. Grades up, no more trouble.\" You promise. You mean it. Probably."; }
        boardingEnroll(true); return "\"We've already paid the deposit.\" Begging didn't work. Rosewood, here you come.";
    }
    addMoodlet(MC(), "Door Slammed", EM_ANGRY, 2, 240);
    boardingEnroll(true); return "You slam your door so hard a poster falls. It doesn't matter - you're going.";
}
std::vector<std::pair<std::string, int>> boardingSchoolOptions() {
    std::vector<std::pair<std::string, int>> v;
    if (jailed()) return v;
    if (student()) {
        if (!boarding()) v.push_back({"Enroll at Rosewood Academy ($600)", 50});
        else { v.push_back({"Head Back to Rosewood", 51}); v.push_back({"Drop Out of Rosewood", 52}); }
    } else if (MC().isAdult()) v.push_back({"Tour Rosewood Academy", 53});
    return v;
}
bool boardingSchoolDo(int opt) {
    Sim& me = MC();
    switch (opt) {
    case 50:
        if (G.money < 600) { toast("Rosewood costs $600 a term. Not enough money!"); sfxNo(); return false; }
        G.money -= 600; boardingEnroll(false); return false;
    case 51: moveTo(R_BOARDING); return false;
    case 52: G.boardEnd = 0xFFFF; addMoodlet(me, "Dropped Out", EM_TENSE, 1, 720); toast("You left Rosewood. Back to regular school."); if (G.room == R_BOARDING) moveTo(R_LIVING); return false;
    case 53: moveTo(R_BOARDING); toast("A prefect in a blazer gives you the tour. Very fancy. Very strict."); return false;
    }
    return false;
}

// ---------------------------------------------------------------- spot actions
bool lockupAction(int act) {
    Sim& me = MC(); Activity a; char b[160];
    switch (act) {
    case DA_JAILSLEEP: a = makeAct("Sleeping on the Bunk", 480, 40); a.kind = AK_SLEEP; a.pose = PO_SLEEP; a.rate[N_ENERGY] = 10; a.rate[N_FUN] = -2; startActivity(a); return true;
    case DA_PUSHUPS: a = makeAct("Cell Push-ups", 45, 8); a.rate[N_ENERGY] = -8; a.skill = SK_FITNESS; a.skillRate = 12; startActivity(a); addMoodlet(me, "Jailhouse Strong", EM_CONFIDENT, 1, 240); return true;
    case DA_CALLHOME: {
        int n = 0; for (size_t i = 0; i < G.sims.size(); i++) if (G.sims[i].household && (int)i != G.mc) { changeRel(G.mc, (int)i, 3, 0); n++; }
        me.needs[N_SOCIAL] = std::min(100.f, me.needs[N_SOCIAL] + 25);
        toast(n ? "You call home. Everyone's worried... but they still love you." : "You call... and leave a voicemail for yourself. Sad.");
        a = makeAct("On the Payphone", 15, 0); startActivity(a); return true; }
    case DA_LAWYER:
        if (G.money < 150) { toast("A lawyer costs $150. You can't afford one."); sfxNo(); return true; }
        G.money -= 150;
        if (irand(100) < 50 + skillLevel(me, SK_LOGIC) * 3 && G.jailUntil > G.day) { G.jailUntil--; if (G.jailUntil <= G.day) { release("Your lawyer found a technicality! You're free to go."); return true; } toast("Your lawyer got a day knocked off your sentence!"); sfxYay(); }
        else toast("Your lawyer \"looked into it.\" Nothing changed. Still billed you.");
        return true;
    case DA_JAILHOOPS: a = makeAct("Shooting Hoops in the Yard", 45, 8); a.rate[N_FUN] = 14; a.rate[N_ENERGY] = -8; a.skill = SK_FITNESS; a.skillRate = 6; startActivity(a); return true;
    case DA_JAILFRIEND: me.needs[N_SOCIAL] = std::min(100.f, me.needs[N_SOCIAL] + 20); { static const char* F[] = {"Your cellmate teaches you a card trick. Respect.", "An inmate named Big Tasha says you're \"alright.\" High praise.", "You trade a granola bar for gossip. Worth it."}; toast(F[irand(3)]); } addMoodlet(me, "Made a Jail Friend", EM_HAPPY, 1, 240); return true;
    case DA_JAILFIGHT: {
        int foe = -1;
        for (const Actor& x : G.rooms[G.house][G.room].actors) if (x.kind == 0 && x.id != G.mc && !G.sims[x.id].household) { foe = x.id; break; }
        brawlStart(G.mc, foe, false);
        int add = irand(2) + 1; G.jailUntil = (uint16_t)std::min(G.day + 7, G.jailUntil + add);
        if (irand(2)) { addMoodlet(me, "Won the Yard Fight", EM_CONFIDENT, 2, 480); snprintf(b, sizeof b, "You won the yard fight... and the guards added %d day%s to your time.", add, add == 1 ? "" : "s"); }
        else { addMoodlet(me, "Lost the Yard Fight", EM_SAD, 2, 480); me.needs[N_ENERGY] = std::max(0.f, me.needs[N_ENERGY] - 20); snprintf(b, sizeof b, "You lost the yard fight. Ouch. And +%d day%s for fighting.", add, add == 1 ? "" : "s"); }
        toast(b); return true; }
    case DA_BAIL: { int c = bailCost(); if (G.money < c) { snprintf(b, sizeof b, "Bail is $%d. You don't have it.", c); toast(b); sfxNo(); return true; } G.money -= c; snprintf(b, sizeof b, "Bail posted ($%d). Grab your stuff and go!", c); release(b); return true; }
    case DA_TIMELEFT: { int d = std::max(0, (int)G.jailUntil - (int)G.day); snprintf(b, sizeof b, "Guard: \"You're out %s at 8 AM. Bail's $%d if anybody loves you.\"", d <= 0 ? "tomorrow" : d == 1 ? "tomorrow" : (std::string("in ") + std::to_string(d) + " days").c_str(), bailCost()); toast(b); return true; }
    case DA_DORMSLEEP: a = makeAct("Sleeping in the Dorm", 480, 40); a.kind = AK_SLEEP; a.pose = PO_SLEEP; a.rate[N_ENERGY] = 13; a.rate[N_FUN] = 2; startActivity(a); return true;
    case DA_HOMESICK: me.needs[N_SOCIAL] = std::min(100.f, me.needs[N_SOCIAL] + 15); toast("You video-call home. Your little sibling shows you a drawing. You miss them."); addMoodlet(me, "Homesick", EM_SAD, 1, 240); return true;
    case DA_PREP: a = makeAct("Studying for Exams", 90, 10); a.kind = AK_HOMEWORK; a.rate[N_FUN] = -4; a.skill = SK_LOGIC; a.skillRate = 10; startActivity(a); me.gradePts = std::min(100.f, me.gradePts + 5); me.homework = false; return true;
    case DA_ROOMIES: a = makeAct("Hanging with Roommates", 60, 0); a.rate[N_SOCIAL] = 20; a.rate[N_FUN] = 12; startActivity(a); return true;
    case DA_PILLOW: a = makeAct("Pillow Fight!", 20, 0); a.rate[N_FUN] = 30; a.rate[N_SOCIAL] = 10; startActivity(a); fxBurst(200, 150, 16); toast("Feathers EVERYWHERE. The house mother is not amused."); return true;
    case DA_SNEAKOUT:
        if (irand(100) < 40) { addMoodlet(me, "Detention", EM_TENSE, 2, 720); me.gradePts = std::max(0.f, me.gradePts - 4); toast("Caught by the prefect with a flashlight. Saturday detention."); }
        else { me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 30); addMoodlet(me, "Midnight Adventure", EM_PLAYFUL, 2, 360); toast("You and your roommates raid the kitchen at midnight. Legendary."); }
        return true;
    case DA_HOTELSLEEP: a = makeAct("Sleeping in the Suite", 480, 40); a.kind = AK_SLEEP; a.pose = PO_SLEEP; a.rate[N_ENERGY] = 15; a.rate[N_HYGIENE] = 2; startActivity(a); return true;
    case DA_SUITESVC:
        if (G.money < 25) { toast("Room service is $25. Not enough money!"); sfxNo(); return true; }
        G.money -= 25; me.needs[N_HUNGER] = 100; addMoodlet(me, "Breakfast in Bed", EM_HAPPY, 1, 240);
        toast("Knock knock - room service! Pancakes, fruit and a tiny jar of jam."); return true;
    case DA_MINIBAR:
        if (G.money < 12) { toast("The minibar is $12 a snack. Highway robbery."); sfxNo(); return true; }
        G.money -= 12; me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + 20); me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 8);
        toast("$12 for a mini bag of pretzels. Worth it? Absolutely not. Did you do it? Yes."); return true;
    case DA_BALCONY: a = makeAct("Enjoying the Balcony View", 45, 0); a.rate[N_FUN] = 10; a.rate[N_SOCIAL] = 2; startActivity(a); addMoodlet(me, "Breathtaking View", EM_HAPPY, 1, 240); return true;
    case DA_SUITEFLY: travelDo(11); return true;
    case DA_LANDMARK: a = makeAct("Taking Tourist Photos", 30, 0); a.rate[N_FUN] = 18; startActivity(a);
        addMemoryOnce(MEM_FUN, G.mc, -1, std::string("Took a million photos at the famous landmark in ") + vacationName() + ".");
        toast("Click click click. At least 40 of these are the same picture."); return true;
    case DA_TOUR:
        if (G.money < 30) { toast("The guided tour is $30."); sfxNo(); return true; }
        G.money -= 30; a = makeAct("On a Guided Tour", 90, 0); a.rate[N_FUN] = 10; a.rate[N_SOCIAL] = 8; a.rate[N_ENERGY] = -3; a.skill = SK_LOGIC; a.skillRate = 6; startActivity(a);
        toast("Your tour guide knows EVERY fun fact. You now know them too."); return true;
    case DA_STREETFOOD:
        if (G.money < 12) { toast("Street food is $12."); sfxNo(); return true; }
        G.money -= 12; me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + 50); addMoodlet(me, "Delicious Local Food", EM_HAPPY, 1, 240);
        { static const char* F[3] = {"Grilled pineapple & coconut shrimp. Island perfection.", "Hot pretzels and maple taffy rolled in the snow!", "A warm crepe with Nutella. You might move here."}; toast(F[(G.vacDest + 2) % 3]); }
        return true;
    case DA_CONCIERGE: { static const char* T[3] = {"\"The Waterfall Lagoon at sunset - trust me.\"", "\"The village market has the best maple taffy in the world.\"", "\"Go to the museum on a weekday morning. No crowds.\""}; toast(std::string("Concierge: ") + T[(G.vacDest + 2) % 3]); me.needs[N_SOCIAL] = std::min(100.f, me.needs[N_SOCIAL] + 8); return true; }
    case DA_LOUNGECOFFEE: if (G.money < 6) { toast("Coffee is $6."); sfxNo(); return true; } G.money -= 6; me.needs[N_ENERGY] = std::min(100.f, me.needs[N_ENERGY] + 15); toast("A perfect little espresso in a fancy cup."); return true;
    case DA_LUGGAGE: { int R[7]; int n = vacationMapRooms(R); if (n >= 7) moveTo(R[6]); toast("Your car to the sights has arrived!"); return true; }
    case DA_RAINSHOWER: a = makeAct("Rain Shower", 20, 0); a.rate[N_HYGIENE] = 60; a.rate[N_FUN] = 4; startActivity(a); return true;
    case DA_HSOAK: a = makeAct("Bubble Soak", 45, 0); a.rate[N_HYGIENE] = 30; a.rate[N_FUN] = 14; startActivity(a); addMoodlet(me, "Five-Star Soak", EM_HAPPY, 1, 240); return true;
    case DA_HGLAM: a = makeAct("Glamming Up", 20, 0); a.rate[N_FUN] = 6; startActivity(a); addMoodlet(me, "Vacation Glam", EM_CONFIDENT, 1, 240); return true;
    case DA_INFINITY: a = makeAct("Swimming in the Infinity Pool", 45, 0); a.rate[N_FUN] = 18; a.rate[N_HYGIENE] = 6; a.rate[N_ENERGY] = -4; a.skill = SK_FITNESS; a.skillRate = 4; startActivity(a); return true;
    case DA_MASSAGE: if (G.money < 40) { toast("A spa massage is $40."); sfxNo(); return true; } G.money -= 40; a = makeAct("Spa Massage", 60, 0); a.rate[N_ENERGY] = 10; a.rate[N_FUN] = 10; startActivity(a); addMoodlet(me, "Totally Relaxed", EM_HAPPY, 2, 480); return true;
    case DA_POOLDRINK: if (G.money < 8) { toast("Mocktails are $8."); sfxNo(); return true; } G.money -= 8; me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 10); toast("A tropical mocktail with a tiny umbrella. Iconic."); return true;
    case DA_GATE:
        if (boarding() && isWeekday(G.day)) { toast("Gate's locked on school days. Weekends only!"); sfxNo(); return true; }
        moveTo(R_LIVING); return true;
    }
    return false;
}

// ---------------------------------------------------------------- clock
void lockupHourly(int hour) {
    if (G.sims.empty()) return;
    Sim& me = MC();
    if (jailed()) {
        if (G.day >= G.jailUntil && hour >= 8) { release(juvie() ? "Released from juvie. The sun has never looked so good." : "Released! You walk out of County with your stuff in a plastic bag."); return; }
        if (G.room != R_JAIL) moveTo(R_JAIL);
        if (hour == 12) { me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + 40); toast("Lunch: mystery meat and a carton of milk. You eat it anyway."); }
        if (hour == 22) toast("\"LIGHTS OUT!\" The whole block goes dark.");
        if (me.career != CR_NONE && hour == 9 && isWeekday(G.day)) me.performance = std::max(-100.f, me.performance - 10);
    }
    if (boarding() && !jailed()) {
        if (isWeekday(G.day) && hour == 7 && G.room != R_BOARDING) { moveTo(R_BOARDING); toast("School day! Back to Rosewood."); }
        if (isWeekday(G.day) && hour == 8) { me.schoolToday = true; me.gradePts = std::min(100.f, me.gradePts + 2); skillGain(me, SK_LOGIC, 2); }
        if (hour == 22 && G.room == R_BOARDING) toast("The house mother calls lights-out. Whispering continues.");
    }
}
void lockupDaily() {
    if (G.sims.empty()) return;
    Sim& me = MC();
    if (boarding() && G.day >= G.boardEnd) {
        G.boardEnd = 0xFFFF; me.gradePts = std::min(100.f, me.gradePts + 10);
        addMoodlet(me, "Finished the Term", EM_CONFIDENT, 2, 1440);
        addMemoryOnce(MEM_FUN, G.mc, -1, "Finished a term at Rosewood Academy.");
        toast("Rosewood term complete! Grades are way up. (Re-enroll at the School if you want another term.)");
        if (G.room == R_BOARDING) moveTo(R_LIVING);
    }
    if (!boarding() && !jailed() && student() && hasGrownUp() && !momentActive() && me.gradePts < 25 && (G.rapSheet > 0 || G.mcGrounded) && irand(100) < 15)
        momentCustom("Rosewood Academy", "Your parents slide a glossy brochure across the table. \"Rosewood Academy. Boarding school. Your grades... the trouble... we think it's time.\"", "Fine. I'll go.", "Beg to stay", "Storm off", parentsSend, 0, G.mc);
}

// ---------------------------------------------------------------- murder (stylized, adults only)
namespace { int g_bloodKind = 0; float g_bloodT = 0, g_bloodX = 200, g_bloodY = 214; }
void murderAttempt(int t) {
    if (t < 0 || t >= (int)G.sims.size() || t == G.mc) return;
    Sim& me = MC(); Sim& s = G.sims[t];
    if (me.stage < ST_YA || s.stage < ST_YA) { toast("Not happening."); return; }   // never kids or teens
    const int tragedy = challengeDanger(0), violence = challengeDanger(1), perma = challengeDanger(2);
    brawlStart(G.mc, t, false);
    for (const Actor& a : G.rooms[G.house][G.room].actors) if (a.kind == 0 && a.id == t) { g_bloodX = a.x; g_bloodY = a.y; }
    g_bloodKind = violence; g_bloodT = 0;
    const bool lethal = perma && tragedy > 0;
    const std::string n = s.name.substr(0, s.name.find(' '));
    changeRel(G.mc, t, -100, -100);
    int witnesses = 0; for (const Actor& a : G.rooms[G.house][G.room].actors) if (a.kind == 0 && a.id != G.mc && a.id != t) witnesses++;
    addMoodlet(me, "Guilty Conscience", EM_TENSE, 3, 2880);
    if (lethal) {
        lifeKill(t, DK_MURDER);
        toast(n + " is dead. Nobody saw... right?");
    } else {
        s.needs[N_ENERGY] = 0; addMoodlet(s, "Attacked", EM_TENSE, 3, 2880);
        toast(n + " survived and was rushed to the hospital. (Permanent Death is off in Game Modes > Challenge Modes > Options.)");
    }
    G.rapSheet++;
    if (witnesses > 0 || copHere() || irand(100) < 30) { courtSchedule(4); jailSentence(lethal ? 7 : 3, lethal ? "murder" : "assault"); }
    else toast("You got away with it... for now.");
}
// ---------------------------------------------------------------- fist fight dust cloud
namespace {
struct Brawl { bool on = false, pub = false; int a = -1, b = -1; float x = 200, y = 214, age = 0; };
Brawl g_br;
const float DUR = 2.8f;
bool g_arrestPending = false;
}
void brawlStart(int a, int b, bool publicFight) {
    g_br = Brawl{}; g_br.on = true; g_br.a = a; g_br.b = b; g_br.pub = publicFight;
    float ax = 200, bx = -1, y = 214;
    for (const Actor& x : G.rooms[G.house][G.room].actors) if (x.kind == 0) {
        if (x.id == a) { ax = x.x; y = x.y; }
        else if (b >= 0 && x.id == b) bx = x.x;
    }
    g_br.x = bx >= 0 ? (ax + bx) / 2 : ax + 14; g_br.y = y;
    g_br.x = std::max(50.f, std::min(350.f, g_br.x));
    sfxNo();
}
static void bloodDraw() {
    if (!g_bloodKind || g_bloodT > 30) return;   // stylized pixel decals; Clean shows nothing
    const int n = g_bloodKind == 1 ? 2 : 7;
    for (int k = 0; k < n; k++) circle(g_bloodX - 14 + (k * 37 % 30), g_bloodY - 2 + (k * 13 % 7), g_bloodKind == 1 ? 1.5f : 2.f + k % 2, C(120, 0, 20, 200));
    if (g_bloodKind == 2) ellipse(g_bloodX, g_bloodY + 2, 14, 3, C(110, 0, 20, 150));
}
bool brawlHides(int sim) { return g_br.on && (sim == g_br.a || sim == g_br.b); }
void brawlFrame(float dt) {
    g_bloodT += dt;
    if (g_br.on) {
        g_br.age += dt;
        if (g_br.age >= DUR) {
            g_br.on = false;
            fxBurst(g_br.x, g_br.y - 40, 18);
            if (g_br.pub && !jailed() && MC().stage >= ST_TEEN && (copHere() || irand(100) < 25)) g_arrestPending = true;
        }
    }
    if (g_arrestPending && !momentActive() && !g_br.on) { g_arrestPending = false; G.rapSheet++; jailSentence(1, "fighting in public"); }
}
void brawlDraw(float t) {
    (void)t;
    bloodDraw();
    if (!g_br.on) return;
    const float age = g_br.age, fade = std::min(1.f, std::min(age * 6.f, (DUR - age) * 4.f));
    const uint8_t A = (uint8_t)(255 * std::max(0.f, fade));
    const float cx = g_br.x + std::sin(age * 31.f) * 3.f, cy = g_br.y - 42 + std::sin(age * 23.f) * 2.f;
    auto skin = [&](int s) { return s >= 0 && s < (int)G.sims.size() ? skinRGB(G.sims[s].look.skin) : RGB{200, 150, 110}; };
    // limbs flying out of the cloud (drawn first so the cloud overlaps their roots)
    const int beat = (int)(age / 0.12f);
    for (int k = 0; k < 4; k++) {
        uint32_t sd = (uint32_t)(beat * 4 + k) * 2654435761u;
        int kind = (sd >> 5) % 3;
        float ang = ((sd >> 9) % 360) * 3.14159f / 180.f, out = 34 + ((sd >> 3) % 10);
        float ex = cx + std::cos(ang) * out * 1.25f, ey = cy + std::sin(ang) * out * 0.9f;
        float rx = cx + std::cos(ang) * 18, ry = cy + std::sin(ang) * 14;
        int who = (sd >> 13) & 1 ? g_br.a : g_br.b;
        if (kind == 0) {   // fist
            line(rx, ry, ex, ey, 6.5f, C(UI_INK, A)); line(rx, ry, ex, ey, 4.5f, C(skin(who), A));
            circle(ex, ey, 6.5f, C(UI_INK, A)); circle(ex, ey, 5.f, C(skin(who), A));
            line(ex - 2, ey - 2, ex + 2, ey - 2, 1, C(UI_INK, A));
        } else if (kind == 1) {   // leg & sneaker
            line(rx, ry, ex, ey, 7.f, C(UI_INK, A)); line(rx, ry, ex, ey, 5.f, C(70, 90, 150, A));
            ellipse(ex, ey, 14, 7, C(UI_INK, A)); ellipse(ex, ey, 12, 5, C(255, 255, 255, A));
        } else {   // hair flying / a stray earring
            circle(ex, ey, 3.5f, C(GOLD, A)); sparkle(ex + 4, ey - 4, 4, C(255, 255, 255, A));
        }
    }
    // the dust cloud
    static const float OX[11] = {-30, -18, -4, 12, 26, 34, -34, -20, 0, 20, 4}, OY[11] = {-6, -20, -26, -22, -12, 4, 10, 18, 22, 16, -4};
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < 11; i++) {
            float r = 15 + 4 * std::sin(age * 17 + i * 1.7f);
            float px = cx + OX[i] + std::sin(age * 13 + i) * 3, py = cy + OY[i] + std::cos(age * 11 + i * 2) * 3;
            if (pass == 0) circle(px, py, r + 2, C(UI_INK, A));
            else { circle(px, py, r, C(236, 228, 214, A)); circle(px - r * 0.25f, py - r * 0.3f, r * 0.45f, C(250, 246, 238, A)); }
        }
    // swirl lines inside the cloud
    for (int i = 0; i < 3; i++) { float a0 = age * 9 + i * 2.1f; line(cx + std::cos(a0) * 10, cy + std::sin(a0) * 8, cx + std::cos(a0 + 1.2f) * 22, cy + std::sin(a0 + 1.2f) * 14, 1.5f, C(170, 160, 150, A)); }
    // stars & comic words
    for (int i = 0; i < 3; i++) { float a0 = age * 4 + i * 2.09f; sparkle(cx + std::cos(a0) * 46, cy - 26 + std::sin(a0 * 1.3f) * 10, 6, C(GOLD, A)); }
    static const char* W[] = {"POW!", "BAM!", "WHAM!", "SMACK!", "OOF!", "THWACK!"};
    const int wi = (int)(age / 0.45f);
    const float wp = std::fmod(age, 0.45f) / 0.45f, ws = 0.45f + 0.2f * std::min(1.f, wp * 4);
    const float wx = cx + ((wi % 2) ? 30 : -30), wy = cy - 46 - (wi % 3) * 4;
    textOutline(W[(wi * 7) % 6], wx, wy, ws, C(255, 230, 60, A), C(220, 30, 60, A), 1, 1.2f);
}
