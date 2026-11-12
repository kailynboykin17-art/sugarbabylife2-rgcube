// Weekly town events, seasonal specials and contests.
#include "game.hpp"
#include "extras.hpp"
#include "sim.hpp"
#include "minigames.hpp"
#include <cstdio>
#include <vector>
#include "oc.hpp"
#include <algorithm>
#include <cmath>

// name, room, start, end, contest minigame (-1 none), host line speaker tag, intro
static const TownEvent WEEKLY[] = {
    {"Spa Monday", R_SPA, 10, 19, MG_NAILS, "Spa Host", "Welcome to Spa Monday! Massages are half price, and our Nail Art Contest is open. Show us your best manicure skills!"},
    {"Smoothie Tuesday", R_MALL, 12, 20, MG_SMOOTHIE, "Mall Host", "It's Smoothie Tuesday at Glam Mall! Blend the most orders before time runs out to win the Smoothie Crown!"},
    {"Movie Night", R_CINEMA, 18, 23, -1, "Usher", "Movie Night! Tonight every ticket is free. Grab some popcorn and bring a date!"},
    {"Karaoke Night", R_CAFE, 18, 23, MG_KARAOKE, "Cafe Host", "Karaoke Night at Cafe Latte! Hit the notes, wow the crowd, and you could take home the Golden Mic!"},
    {"Friday Glow Party", R_CLUB, 19, 24, MG_SIMON, "DJ", "Welcome to the Friday Glow Party! Win the Dance-Off and you're the queen of the night!"},
    {"Beach Volley Bash", R_BEACH, 11, 18, MG_VOLLEY, "Lifeguard", "Beach Volley Bash is ON! Keep the ball in the air longest to win the Sunset Trophy!"},
    {"Mall Sale Day", R_MALL, 10, 20, -1, "Mall Host", "Sale Day! Every outfit at Glam Mall is half price today. Shop till you drop!"},
};
static const TownEvent FASHION_SHOW = {"Fashion Show", R_BOUTIQUE, 17, 22, MG_RUNWAY, "Designer", "Welcome to the Diva Fashion Show! Strut the runway to the beat. The judges are watching every step!"};
static const TownEvent SKATE = {"Skate Social", R_RINK, 15, 21, -1, "Rink Host", "Skate Social at the Ice Palace! Couples skate and hot cocoa all afternoon."};
static const TownEvent PROM = {"Prom Night", R_SCHOOL, 18, 23, -2, "Principal", "Welcome to Glitter High's Prom Night! Dance, have fun, and at the end of the night we crown Prom Royalty!"};
static const TownEvent PICNIC = {"Picnic Fest", R_PARK, 11, 18, -1, "Park Ranger", "Picnic Fest at Rosebud Park! Free snacks, sunshine and good vibes all day."};
static const TownEvent ICEBALL = {"Winter Ice Ball", R_RINK, 18, 23, -2, "Ice Queen", "Welcome to the Winter Ice Ball! Skate, sparkle, and at midnight we crown the Ice Royalty!"};

// eight newer events around town (Uptown & Maple Lane)
static const TownEvent NEWEV[8] = {
    {"Pet Parade", R_PETSHOP, 10, 17, -1, "Pet Shop Owner", "It's the Pet Parade! Dress-up costumes, free treats, and the cutest pets in Starlight Bay. Adoptions are half price today!"},
    {"Book Fair", R_LIBRARY, 9, 18, -1, "Librarian", "Welcome to the Book Fair! Read anything today and you'll learn twice as fast. Bring a friend!"},
    {"Charity Fun Run", R_GYM, 7, 13, MG_VOLLEY, "Coach", "The Charity Fun Run is on! Every workout today raises money for the hospital. Win the challenge for a medal!"},
    {"Masquerade Ball", R_HOTEL, 19, 24, -2, "Hotel Host", "Welcome to the Masquerade Ball! Masks on, lights low... and at midnight we crown the Belle of the Ball!"},
    {"Wedding Expo", R_CHAPEL, 11, 18, -1, "Wedding Planner", "The Wedding Expo is here! Cake tastings, flowers everywhere, and a discount on every dream wedding booked today."},
    {"Health Fair", R_HOSPITAL, 9, 16, -1, "Nurse Joy", "Health Fair day! Free checkups, free smoothies and good vibes. Treat yourself kindly!"},
    {"Block Party", R_STREET, 15, 22, -1, "Neighbor", "Block Party on Maple Lane! Music, lemonade and every neighbor on the street. Come say hi!"},
    {"Battle of the Bands", R_CLUB, 19, 24, MG_KARAOKE, "Emcee", "It's the Battle of the Bands! Grab the mic, rock the crowd, and the loudest cheers win the Golden Guitar!"},
};
static const TownEvent MOREEV[5] = {
    {"Talent Show", R_SCHOOL, 15, 20, MG_KARAOKE, "Principal", "Glitter High's Talent Show! Sing your heart out - the best act wins the Star Trophy!"},
    {"Spring Carnival", R_PARK, 12, 22, -1, "Carnival Barker", "Step right up! Rides, cotton candy, and a Ferris wheel that's PERFECT for a first kiss."},
    {"Food Truck Friday", R_STREET, 16, 22, -1, "Food Truck Chef", "Food trucks line Maple Lane tonight! Tacos, crepes, boba - everything's half price."},
    {"New Year's Eve Bash", R_CLUB, 20, 24, MG_SIMON, "DJ", "It's the New Year's Eve Bash! Dance till midnight and win the Dance-Off for the Golden Disco Ball!"},
    {"Starlight Cast Meet & Greet", R_MALL, 13, 19, -1, "Mall Host", "The whole Starlight Cast is at the mall today! Say hi - they're signing autographs and taking selfies."},
};
int eventsOn(int day, const TownEvent** out, int max) {
    int n = 0;
    auto add = [&](const TownEvent* e) { if (n < max) out[n++] = e; };
    int wd = weekday(day), d28 = day % 28;
    switch (wd) {
    case 0: add(&WEEKLY[0]); break;
    case 1: add(&WEEKLY[1]); break;
    case 2: add(&WEEKLY[2]); break;
    case 3: add(&WEEKLY[3]); break;
    case 4: add(&WEEKLY[4]); break;
    case 5: add(&WEEKLY[5]); if ((day / 7) % 2 == 1) add(&FASHION_SHOW); break;
    case 6: add(&WEEKLY[6]); add(&SKATE); break;
    }
    if (d28 == 5) add(&PROM);        // Spring 6
    if (d28 == 10) add(&PICNIC);     // Summer 4
    if (d28 == 23) add(&ICEBALL);    // Winter 3
    static const int NEWDAY[8] = {2, 8, 12, 16, 18, 20, 25, 27};
    for (int i = 0; i < 8; i++) if (d28 == NEWDAY[i]) add(&NEWEV[i]);
    static const int MOREDAY[5] = {4, 13, 19, 26, 1};
    for (int i = 0; i < 5; i++) if (d28 == MOREDAY[i]) add(&MOREEV[i]);
    return n;
}

const TownEvent* eventHere(int room) {
    const TownEvent* ev[6];
    int n = eventsOn(G.day, ev, 6), h = (int)(G.minute / 60);
    for (int i = 0; i < n; i++) if (ev[i]->room == room && h >= ev[i]->startH && h < ev[i]->endH) return ev[i];
    return nullptr;
}

void eventMorningNotice() {
    const TownEvent* ev[6];
    int n = eventsOn(G.day, ev, 6);
    for (int i = 0; i < n; i++) {
        char b[120];
        int s = ev[i]->startH, e = ev[i]->endH;
        snprintf(b, sizeof b, "Today: %s at %s, %d%s-%d%s", ev[i]->name, ROOM_NAMES[ev[i]->room], s > 12 ? s - 12 : s, s >= 12 ? "pm" : "am", e > 12 ? e - 12 : e, e >= 12 && e < 24 ? "pm" : "am");
        toast(b);
    }
}

static uint16_t s_trackDay = 0xffff;
static std::vector<const TownEvent*> s_greeted, s_entered;
static void rollDay() { if (s_trackDay != G.day) { s_trackDay = G.day; s_greeted.clear(); s_entered.clear(); } }
static bool has(const std::vector<const TownEvent*>& v, const TownEvent* e) { for (auto* x : v) if (x == e) return true; return false; }
bool eventEntered(const TownEvent* e) { rollDay(); return has(s_entered, e); }
void eventMarkEntered(const TownEvent* e) { rollDay(); if (!has(s_entered, e)) s_entered.push_back(e); }

void eventArrive(int room) {
    const TownEvent* e = eventHere(room);
    if (!e) return;
    rollDay();
    if (has(s_greeted, e)) return;
    s_greeted.push_back(e);
    dlgSay(-1, std::string(e->name) + "!");
    dlgSay(-1, e->intro);
    addMoodlet(MC(), "Party Vibes", EM_HAPPY, 1, 240);
    for (int i = 0; i < 8; i++) if (e == &NEWEV[i]) {   // a little perk for showing up
        Sim& me = MC();
        switch (i) {
        case 0: G.money += 50; toast("Pet Parade perk: $50 coupon toward your next pet!"); break;
        case 1: skillGain(me, SK_LOGIC, 6); toast("Book Fair perk: +Logic!"); break;
        case 2: skillGain(me, SK_FITNESS, 6); G.rep = (int16_t)std::min(100, G.rep + 3); toast("Fun Run perk: +Fitness, +Reputation!"); break;
        case 3: addMoodlet(me, "Mysterious", EM_FLIRTY, 2, 360); break;
        case 4: G.money += 100; toast("Wedding Expo perk: $100 wedding voucher!"); break;
        case 5: for (int n = 0; n < N_COUNT; n++) me.needs[n] = std::fmin(100.f, me.needs[n] + 20); toast("Health Fair perk: all needs +20!"); break;
        case 6: me.needs[N_SOCIAL] = 100; G.followers += 50; toast("Block Party perk: social maxed, +50 followers!"); break;
        case 7: G.followers += 100; toast("Battle of the Bands: +100 followers just for showing up!"); break;
        }
        G.satisfaction += 50;
    }
    for (int i = 0; i < 5; i++) if (e == &MOREEV[i]) {
        Sim& me = MC();
        switch (i) {
        case 1: me.needs[N_FUN] = 100; toast("Carnival perk: fun maxed! Ride the Ferris wheel with someone cute."); break;
        case 2: me.needs[N_HUNGER] = 100; toast("Food Truck perk: you ate EVERYTHING. Hunger maxed!"); break;
        case 4: { for (int k = 0; k < OC_COUNT; k++) { int sm = ocSim(k); if (sm < 0) continue; Actor a; a.kind = 0; a.id = (uint16_t)sm; a.x = 40 + k * 44.f; a.tx = a.x; a.y = 196 + (k % 3) * 14; curRoom().actors.push_back(a); }
            int n = 0; for (int k = 0; k < OC_COUNT; k++) { int sm = ocSim(k); if (sm >= 0) { rel(G.mc, sm).met = true; changeRel(G.mc, sm, 10, 0); n++; } }
            G.followers += 150; toast("You met the whole Starlight Cast! +150 followers"); (void)n; break; }
        default: break;
        }
        G.satisfaction += 50;
    }
}

static int prize(int place) { return place == 1 ? 250 : place == 2 ? 120 : 40; }

void eventContestResult(const TownEvent* e, int score) {
    if (!e) return;
    int gold = 10, silver = 5;
    switch (e->game) {
    case MG_NAILS: gold = 8; silver = 4; break;
    case MG_SMOOTHIE: gold = 7; silver = 4; break;
    case MG_KARAOKE: gold = 40; silver = 20; break;
    case MG_SIMON: gold = 8; silver = 5; break;
    case MG_VOLLEY: gold = 15; silver = 8; break;
    case MG_RUNWAY: gold = 30; silver = 15; break;
    }
    int place = score >= gold ? 1 : score >= silver ? 2 : 3;
    int money = prize(place);
    uint32_t fans = place == 1 ? 400 + G.followers / 10 : place == 2 ? 150 : 40;
    G.money += money;
    G.followers += fans;
    char b[160];
    if (place == 1) {
        G.trophies++;
        addMemoryOnce(MEM_FUN, G.mc, -1, std::string("Won the ") + e->name + "!");
        snprintf(b, sizeof b, "And the winner of the %s is... %s!! You take home $%d and the trophy!", e->name, MC().name.c_str(), money);
        addMoodlet(MC(), "Champion!", EM_CONFIDENT, 3, 720);
        fxBurst(200, 80, 50);
    } else if (place == 2) {
        snprintf(b, sizeof b, "%s takes 2nd place in the %s! So close! Here's $%d.", MC().name.c_str(), e->name, money);
        addMoodlet(MC(), "Runner-Up", EM_HAPPY, 1, 240);
    } else {
        snprintf(b, sizeof b, "Thanks for competing in the %s! Better luck next time. Here's $%d for trying.", e->name, money);
        addMoodlet(MC(), "Didn't Win", EM_SAD, 1, 180);
    }
    dlgSay(-1, b);
    snprintf(b, sizeof b, "+%u followers from the contest!", (unsigned)fans);
    toast(b);
}

// Prom / Ice Ball crowning
void eventCrowning(const TownEvent* e) {
    int friends = 0;
    for (const Rel& r : G.rels) if ((r.a == G.mc || r.b == G.mc) && r.fr >= 30) friends++;
    int pop = friends * 3 + fameStars() * 5 + skillLevel(MC(), SK_STYLE) + skillLevel(MC(), SK_CHARISMA);
    const char* title = MC().look.guy ? "King" : "Queen";
    char b[160];
    if (pop >= 14) {
        snprintf(b, sizeof b, "And this year's %s %s is... %s!!!", e->room == R_RINK ? "Ice" : e->room == R_HOTEL ? "Masquerade" : "Prom", title, MC().name.c_str());
        dlgSay(-1, b);
        dlgSay(-1, "The crowd goes wild! Everyone is chanting your name!");
        G.trophies++; G.followers += 800 + G.followers / 8;
        addMemoryOnce(MEM_FUN, G.mc, -1, std::string("Crowned ") + (e->room == R_RINK ? "Ice " : "Prom ") + title + "!");
        addMoodlet(MC(), "Royalty!", EM_CONFIDENT, 3, 1440);
        fxBurst(200, 60, 60);
    } else {
        snprintf(b, sizeof b, "The crown goes to someone else this year... Make more friends and get famous, and next time it could be you!");
        dlgSay(-1, b);
        addMoodlet(MC(), "Snubbed", EM_SAD, 1, 240);
    }
}
