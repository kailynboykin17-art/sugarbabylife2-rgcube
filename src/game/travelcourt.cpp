#include "lockup.hpp"
// Vacation worlds, courts & the celebrity life.
#include "travelcourt.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include "goals.hpp"
#include "lifestory.hpp"
#include <cstdio>
#include <algorithm>
void toast(const std::string& m);
void travelTo(int room);

namespace {
struct Dest { const char* name; int cost; int roomA, roomB; const char* blurb; int suite, sights; };
const Dest DEST[3] = {
    {"Coral Cove", 400, R_ISLAND, R_ISLANDTOWN, "Turquoise water, tiki bars & sunsets", R_COVESUITE, R_COVESIGHTS},
    {"Frostpeak", 350, R_SLOPES, R_LODGE, "Ski slopes, hot cocoa & a cozy lodge", R_PEAKSUITE, R_PEAKSIGHTS},
    {"Belle Ville", 600, R_BOULEVARD, R_TOWER, "Crepes, couture & city lights", R_BELLESUITE, R_BELLESIGHTS},
};
}

bool vacationRoom(int r) { return (r >= R_ISLAND && r <= R_TOWER) || (r >= R_COVESUITE && r <= R_HOTELPOOL); }
bool vacationTripRoom(int r) {   // the rooms of the trip you're on
    if (!G.vacDest) return false;
    const Dest& d = DEST[G.vacDest - 1];
    return r == d.roomA || r == d.roomB || r == d.suite || r == d.sights || r == R_HOTELLOBBY || r == R_HOTELBATH || r == R_HOTELPOOL;
}
int vacationMapRooms(int* out) {   // the trip's own town map: hotel first, then the sights
    if (!G.vacDest) return 0;
    const Dest& d = DEST[G.vacDest - 1];
    const int R[7] = {R_HOTELLOBBY, d.suite, R_HOTELBATH, R_HOTELPOOL, d.roomA, d.roomB, d.sights};
    for (int k = 0; k < 7; k++) out[k] = R[k];
    return 7;
}
int vacationDest() { return G.vacDest; }
const char* vacationShort(int r) {
    switch (r) {
    case R_HOTELLOBBY: return "Hotel Lobby"; case R_HOTELBATH: return "Bathroom"; case R_HOTELPOOL: return "Pool & Spa";
    case R_COVESUITE: case R_PEAKSUITE: case R_BELLESUITE: return "My Suite";
    case R_ISLAND: return "Beach"; case R_ISLANDTOWN: return "Village"; case R_COVESIGHTS: return "Lagoon";
    case R_SLOPES: return "Slopes"; case R_LODGE: return "Lodge"; case R_PEAKSIGHTS: return "Market";
    case R_BOULEVARD: return "Boulevard"; case R_TOWER: return "Tower"; case R_BELLESIGHTS: return "Museum";
    }
    return ROOM_NAMES[r];
}
const char* vacationName() { return G.vacDest ? DEST[G.vacDest - 1].name : ""; }
int vacationSuite() { return G.vacDest ? DEST[G.vacDest - 1].suite : R_LIVING; }
bool vacationOpen(int r, std::string* why) {
    if (!vacationRoom(r)) return true;
    if (vacationTripRoom(r)) return true;
    if (why) *why = "Book a trip at the Travel Agency first!";
    return false;
}

std::vector<std::pair<std::string, int>> travelOptions() {
    std::vector<std::pair<std::string, int>> v;
    if (G.vacDest) { v.push_back({std::string("Back to ") + DEST[G.vacDest - 1].name, 10}); v.push_back({"End the Trip Early", 11}); return v; }
    for (int k = 0; k < 3; k++) { char b[64]; snprintf(b, sizeof b, "%s - 3 days ($%d)", DEST[k].name, DEST[k].cost); v.push_back({b, k}); }
    return v;
}
static void endTrip(const char* why) {
    if (!G.vacDest) return;
    const Dest& d = DEST[G.vacDest - 1];
    addMemoryOnce(MEM_FUN, G.mc, -1, std::string("Vacation in ") + d.name + "! Best trip ever.");
    G.vacDest = 0;
    if (vacationRoom(G.room)) travelTo(R_LIVING);
    addMoodlet(MC(), "Post-Vacation Glow", EM_HAPPY, 2, 1440);
    toast(why);
}
bool travelDo(int opt) {
    Sim& me = MC();
    if (opt == 10 && G.vacDest) { travelTo(DEST[G.vacDest - 1].suite); return true; }
    if (opt == 11) { endTrip("Home sweet home. You unpack... eventually."); return true; }
    if (opt < 0 || opt > 2) return false;
    if (me.isLittle() || me.stage < ST_TEEN) { toast("You need a grown-up (or at least a teen) to book a trip!"); sfxNo(); return false; }
    if (G.money < DEST[opt].cost) { toast("Not enough money for that trip!"); sfxNo(); return false; }
    G.money -= DEST[opt].cost; if (G.trips < 255) G.trips++; G.vacDest = (uint8_t)(opt + 1); G.vacEnd = (uint16_t)(G.day + 3);
    cancelActivity();
    addMoodlet(me, "On Vacation!", EM_HAPPY, 2, 4320);
    toast(std::string("Bon voyage! Welcome to ") + DEST[opt].name + ". " + DEST[opt].blurb + ".");
    sfxYay(); travelTo(R_HOTELLOBBY);
    toast("Check-in complete! Your suite is home base. You're here until the trip ends - or fly home from the suite.");
    return true;
}

bool travelStartOwned(int dest, int days, const char* homeName) {   // trips to your own vacation home: no hotel bill
    if (dest < 0 || dest > 2 || G.vacDest) return false;
    Sim& me = MC(); if (G.trips < 255) G.trips++; G.vacDest = (uint8_t)(dest + 1); G.vacEnd = (uint16_t)(G.day + days);
    cancelActivity(); addMoodlet(me, "Weekend Getaway!", EM_HAPPY, 2, days * 1440);
    toast(std::string("Off to your ") + homeName + " in " + DEST[dest].name + "! The suite is all yours - no check-in, no bill.");
    sfxYay(); travelTo(DEST[dest].suite); return true;
}
// ---------------------------------------------------------------- courts
void courtSchedule(int crime) {
    if (G.courtDay != 0xFFFF) return;
    G.courtDay = (uint16_t)(G.day + 1); G.courtCrime = (uint8_t)crime;
    toast("You got a court date for tomorrow at 10 AM. (Or go early: Courthouse on the map.)");
}
static std::string verdict(int choice, int) {
    Sim& me = MC();
    G.courtDay = 0xFFFF;
    float ch = choice == 0 ? 0.f : choice == 1 ? 0.75f : 0.25f + skillLevel(me, SK_CHARISMA) * 0.06f + skillLevel(me, SK_LOGIC) * 0.03f;
    ch -= std::min(0.3f, G.rapSheet * 0.05f);
    const int fine = 150 + G.rapSheet * 50;
    if (choice == 1) G.money = std::max<int32_t>(0, G.money - 200);
    if (choice == 0) {   // plea deal
        G.money = std::max<int32_t>(0, G.money - fine / 2);
        addMoodlet(me, "Owned Up To It", EM_FINE, 1, 360);
        char b[200]; snprintf(b, sizeof b, "You plead guilty. The judge appreciates the honesty: a $%d fine and ten hours of community service. Lesson learned.", fine / 2);
        me.needs[N_ENERGY] = std::max(0.f, me.needs[N_ENERGY] - 25);
        return b;
    }
    if (frand() < ch) {
        if (G.rapSheet > 0) G.rapSheet--;
        addMoodlet(me, "Not Guilty!", EM_CONFIDENT, 3, 720);
        if (fameStars() >= 2) G.followers += 2000;
        return choice == 1 ? "Your lawyer tears the case apart. \"NOT GUILTY!\" The courtroom gasps. You hug your lawyer (who bills you for the hug)." : "You give the speech of your LIFE. The jury is moved. \"Not guilty!\" Someone in the gallery starts a slow clap.";
    }
    G.money = std::max<int32_t>(0, G.money - fine);
    addMoodlet(me, "Guilty Verdict", EM_SAD, 3, 1440);
    me.needs[N_ENERGY] = std::max(0.f, me.needs[N_ENERGY] - 30); me.needs[N_FUN] = std::max(0.f, me.needs[N_FUN] - 30);
    if (fameStars() >= 1) { G.followers = G.followers > 1000 ? G.followers - G.followers / 10 : 0; }
    if (G.rapSheet >= 3) {   // repeat offenders do real time
        int days = std::min(5, (int)G.rapSheet - 1);
        char j[220]; snprintf(j, sizeof j, "\"Guilty.\" With your record, the judge isn't playing: a $%d fine and %d days in County. The bailiff walks you out in cuffs.", fine, days);
        jailSentence(days, "a guilty verdict");
        return j;
    }
    char b[220]; snprintf(b, sizeof b, "\"Guilty.\" The gavel bangs. A $%d fine, community service and six months of probation. Your mug shot is NOT your angle.", fine);
    return b;
}
static void courtroom() {
    const char* CR[] = {"shoplifting", "vandalism", "dine & dash", "pickpocketing", "the heist", "breaking curfew", "trespassing", "a prank gone wrong"};
    char b[240]; snprintf(b, sizeof b, "Westbrook County Court is now in session. The charge: %s. Judge Halloway peers over her glasses. \"How do you plead?\"", CR[G.courtCrime % 8]);
    momentCustom("COURT DATE", b, "Plead guilty (plea deal)", "Hire a lawyer ($200)", "Defend myself!", verdict, 0, -1);
}
std::vector<std::pair<std::string, int>> courtOptions() {
    std::vector<std::pair<std::string, int>> v;
    if (G.courtDay != 0xFFFF) v.push_back({"Attend Your Court Date", 0});
    if (MC().isAdult()) v.push_back({"Serve Jury Duty (+$40)", 1});
    if (G.rapSheet > 0) { char b[48]; snprintf(b, sizeof b, "Clear Your Record ($%d)", 100 * G.rapSheet); v.push_back({b, 2}); }
    v.push_back({"Watch a Trial from the Gallery", 3});
    return v;
}
bool courtDo(int opt) {
    Sim& me = MC(); Activity a;
    switch (opt) {
    case 0: courtroom(); return true;
    case 1: G.money += 40; skillGain(me, SK_LOGIC, 3); addMoodlet(me, "Civic Duty", EM_CONFIDENT, 1, 360); toast("Jury duty: you were the deciding vote. The defendant was innocent - you could just tell."); cancelActivity(); a = makeAct("Jury Duty", 240, 12); startActivity(a); return true;
    case 2: { int c = 100 * G.rapSheet; if (G.money < c) { toast("Not enough money!"); sfxNo(); return false; } G.money -= c; G.rapSheet = 0; toast("Record expunged. Fresh start!"); addMoodlet(me, "Clean Slate", EM_HAPPY, 2, 720); return false; }
    case 3: addMoodlet(me, "Courtroom Drama", EM_PLAYFUL, 1, 240); toast("A neighbor dispute over a stolen garden gnome. Riveting. You took notes."); cancelActivity(); a = makeAct("Watching a Trial", 90, 6); a.rate[N_FUN] = 10; startActivity(a); return true;
    }
    return false;
}

// ---------------------------------------------------------------- celebrity life
void celebShiftBonus() {
    Sim& me = MC();
    if (me.career != CR_CELEBRITY) return;
    uint32_t f = 300u * (me.careerLevel + 1) * (1 + fameStars());
    G.followers += f;
    static const char* GIG[] = {"a red-carpet premiere", "a magazine cover shoot", "a late-night talk show", "a perfume launch", "an awards show", "a surprise music video cameo"};
    toast(std::string("Celebrity gig: ") + GIG[irand(6)] + "! +" + std::to_string(f) + " followers.");
}

void travelCourtHourly(int hour) {
    Sim& me = MC();
    if (G.vacDest && G.day >= G.vacEnd && hour >= 8) endTrip("Vacation's over! You fly home with a tan and way too many photos.");
    if (G.courtDay != 0xFFFF && G.day >= G.courtDay && hour == 10) courtroom();
    // paparazzi & fans follow famous Sims around town
    if (fameStars() >= 3 && G.room > R_YARD && !vacationRoom(G.room) && irand(100) < 6) {
        int r = irand(3);
        if (r == 0) { G.followers += 500; toast("Paparazzi! Flashes everywhere. Your photo is trending."); addMoodlet(me, "Camera Ready", EM_CONFIDENT, 1, 120); }
        else if (r == 1) { toast("A fan asked for your autograph and nearly cried. Adorable."); addMoodlet(me, "Adored by Fans", EM_HAPPY, 1, 180); }
        else { toast("Paparazzi caught you mid-bite. Unflattering. The internet is having FUN."); addMoodlet(me, "Bad Angle", EM_EMBARRASSED, 1, 120); }
    }
    if (G.vacDest && vacationRoom(G.room) && irand(100) < 4) addMoodlet(me, "Living the Dream", EM_HAPPY, 1, 120);
}
