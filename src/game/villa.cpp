// Love Villa reality mode + cross-system story events.
#include "villa.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <algorithm>
#include <vector>
#include "vfs.hpp"
#define fopen vfopen   // sidecar saves go into the one-file save pack
using namespace col;
void toast(const std::string& m);
void say(int simIdx, const std::string& line);
bool momentActive();
typedef std::string (*MomentFn)(int, int);
void momentCustom(const std::string& title, const std::string& text, const char* o0, const char* o1, const char* o2, MomentFn fn, int ctx, int portraitSim);

namespace {
std::string fmt(const char* f, ...) { char b[400]; va_list a; va_start(a, f); vsnprintf(b, sizeof b, f, a); va_end(a); return b; }
std::string nm(int s) { if (s < 0 || s >= (int)G.sims.size()) return "?"; const std::string& n = G.sims[s].name; return n.substr(0, n.find(' ')); }

// ---------------------------------------------------------------- villa
// islander slot 0 is you. bond[i] = your chemistry with islander i; partner[i] = who they're coupled with (-1 single)
struct Villa {
    uint32_t magic = 0x564C4131;
    int8_t active = 0, day = 0, done = 0, ending = 0;
    int16_t sim[14]; int8_t partner[14]; int8_t bond[14]; uint8_t in[14]; uint8_t casa[14]; uint8_t n = 0;
    int16_t votes = 0; uint8_t winsChallenge = 0, cheated = 0, hideaway = 0;
};
Villa V;
const char* DAYS[20] = {"Get Grafting", "First Bombshell", "Sandcastle Challenge", "Double Trouble", "First Recoupling", "Pong Challenge", "Disco Party",
                        "Triple Threat", "Painting Challenge", "DUMP DUMP DUMP", "Hideaway Dinner", "Seduction Party", "CASA AMOR!", "Casa Challenge Dates",
                        "Woo-Who?!", "Back to the Villa", "Movie Night", "Let's Roll Challenge", "Final Recoupling", "Villa Finale"};
const char* DAYTXT[20] = {
    "Day 1. Six islanders, one villa. The girls pick who they want to couple up with.",
    "Day 2. A bombshell walks in and takes two islanders on dates.",
    "Day 3. Couples race to build the best sandcastle. Winners get a wine-tasting date.",
    "Day 4. Two more bombshells arrive to shake things up.",
    "Day 5. Recoupling night. Whoever's left single is dumped from the villa.",
    "Day 6. Ping-pong tournament! The winning couple goes horseback riding.",
    "Day 7. Disco party. Everyone reveals if they're committed or open to options.",
    "Day 8. Three new islanders pick who they're most into for a spa date.",
    "Day 9. Painting challenge - the best painter picks a date.",
    "Day 10. The public has voted. The least popular couple is in danger.",
    "Day 11. Two couples get a dinner date; one gets the Hideaway.",
    "Day 12. Seduction party - dress up and dance for your partner.",
    "Day 13. CASA AMOR: half the villa leaves, six new singles arrive.",
    "Day 14. Casa challenge dates. Who's turning heads?",
    "Day 15. The Woo-Who game. Do you stay loyal?",
    "Day 16. Stick or twist: who comes back to the villa with you?",
    "Day 17. Movie night: everyone sees what happened at Casa. Damage control!",
    "Day 18. Bowling challenge - the winner secures a spot in the finale.",
    "Day 19. Final recoupling. Anyone single goes home.",
    "Day 20. The finale. The public crowns the winning couple."};
bool simOk(int s) { return s >= 0 && s < (int)G.sims.size(); }
int me() { return 0; }
int addIslander(int s, bool casa) { if (V.n >= 14) return -1; int i = V.n++; V.sim[i] = (int16_t)s; V.partner[i] = -1; V.bond[i] = (int8_t)(irand(20)); V.in[i] = 1; V.casa[i] = casa; return i; }
std::vector<int> pool(bool guy) {
    std::vector<int> v;
    for (size_t s = 0; s < G.sims.size(); s++) {
        if ((int)s == G.mc || G.sims[s].household || G.sims[s].stage < ST_YA || G.sims[s].stage > ST_ADULT || (bool)G.sims[s].look.guy != guy) continue;
        bool used = false; for (int i = 0; i < V.n; i++) if (V.sim[i] == (int)s) used = true;
        if (!used) v.push_back((int)s);
    }
    return v;
}
int recruit(bool guy, bool casa) { auto v = pool(guy); if (v.empty()) return -1; return addIslander(v[irand((int)v.size())], casa); }
bool myGuy() { return !MC().look.guy; }   // the gender you're coupling with (dating-show default)
int myPartner() { return V.partner[0]; }
void couple(int a, int b) { if (a < 0 || b < 0) return; int pa = V.partner[a], pb = V.partner[b]; if (pa >= 0) V.partner[pa] = -1; if (pb >= 0) V.partner[pb] = -1; V.partner[a] = (int8_t)b; V.partner[b] = (int8_t)a; }
void npcCouple() {   // NPC islanders pair off by chance
    std::vector<int> g, b;
    for (int i = 1; i < V.n; i++) if (V.in[i] && V.partner[i] < 0) ((bool)G.sims[V.sim[i]].look.guy ? b : g).push_back(i);
    while (!g.empty() && !b.empty()) { int x = g.back(); g.pop_back(); int y = b[irand((int)b.size())]; b.erase(std::find(b.begin(), b.end(), y)); couple(x, y); }
}
void dumpSingles(std::string& out) {
    for (int i = 1; i < V.n; i++) if (V.in[i] && V.partner[i] < 0) { V.in[i] = 0; out += fmt("%s was dumped from the villa. ", nm(V.sim[i]).c_str()); }
}
std::vector<int> options(bool includePartner) {   // islanders you could pick
    std::vector<int> v;
    for (int i = 1; i < V.n; i++) if (V.in[i] && (bool)G.sims[V.sim[i]].look.guy == myGuy() && (includePartner || i != myPartner())) v.push_back(i);
    return v;
}
enum Scr { VS_INTRO, VS_DAY, VS_PICK, VS_RESULT, VS_END };
int scr = VS_INTRO; std::string res; std::vector<int> pickList; int pickKind = 0;   // 0 couple up, 1 date, 2 bring back from casa
Btn rowB(int k) { return Btn{10, 30 + k * 30.f, 300, 27}; }
const Btn bBack{4, 208, 72, 28}, bGo{164, 208, 152, 28};

void startVilla() {
    V = Villa{}; V.active = 1;
    addIslander(G.mc, false);
    for (int k = 0; k < 3; k++) recruit(myGuy(), false);
    for (int k = 0; k < 2; k++) recruit(!myGuy(), false);
    for (int i = 1; i < V.n; i++) V.bond[i] = (int8_t)(10 + irand(20));
    scr = VS_DAY; sfxYay();
}
void doDay(int choice) {
    res.clear();
    const int d = V.day; const int p = myPartner();
    auto bump = [&](int i, int amt) { if (i >= 0) V.bond[i] = (int8_t)std::clamp(V.bond[i] + amt, -50, 100); };
    switch (d) {
    case 0: case 4: case 18: {   // couplings
        pickList = options(true); pickKind = 0;
        if (pickList.empty()) { res = "There's nobody left to couple up with!"; break; }
        if (choice < 0) { scr = VS_PICK; return; }
        int i = pickList[choice % pickList.size()];
        bool yes = V.partner[i] < 0 || V.bond[i] + irand(30) > 35 || i == p;
        if (yes) { couple(0, i); bump(i, 10); res = fmt("You pick %s... and they pick you back! ", nm(V.sim[i]).c_str()); }
        else res = fmt("%s chose to stay with their partner. Ouch. ", nm(V.sim[i]).c_str());
        npcCouple();
        if (d > 0) { if (myPartner() < 0) { res += "You're single at the recoupling... you've been DUMPED from the villa."; V.done = 1; V.ending = 3; break; } dumpSingles(res); }
        break; }
    case 1: case 3: case 7: { int n = d == 3 ? 2 : d == 7 ? 3 : 1;
        for (int k = 0; k < n; k++) { int b = recruit(k % 2 ? !myGuy() : myGuy(), false); if (b >= 0) res += fmt("Bombshell %s walks in! ", nm(V.sim[b]).c_str()); }
        if (choice == 0) { int last = V.n - 1; bump(last, 15); res += "You flirt with the newest arrival. Your partner notices..."; bump(p, -8); }
        else if (choice == 1) { bump(p, 8); res += "You stay loyal and reassure your partner."; }
        else res += "You watch the chaos from the daybeds with popcorn.";
        npcCouple(); break; }
    case 2: case 5: case 8: case 17: {
        const int win = choice == 0 ? 45 : choice == 1 ? 60 : 35;
        if (irand(100) < win) { V.winsChallenge++; bump(p, 12); res = fmt("You WIN the %s! ", DAYS[d]); if (d == 17) res += "Your spot in the finale is secured!"; else res += "A romantic date for you two."; }
        else { bump(p, 2); res = fmt("You lose the %s, but you had fun.", DAYS[d]); }
        break; }
    case 6: case 11: case 14:
        if (choice == 0) { bump(p, 15); res = "You tell everyone you're fully committed. Your partner melts."; }
        else if (choice == 1) { auto o = options(false); if (!o.empty()) { int i = o[irand((int)o.size())]; bump(i, 15); bump(p, -12); V.cheated++; res = fmt("You and %s share a moment. The villa saw everything.", nm(V.sim[i]).c_str()); } }
        else { res = "You keep your options open. Mysterious."; bump(p, -3); }
        break;
    case 9: { int lowest = -1, lb = 999; for (int i = 1; i < V.n; i++) if (V.in[i] && V.partner[i] > 0 && V.bond[i] < lb && i != p) { lb = V.bond[i]; lowest = i; }
        if (p >= 0 && V.bond[p] < 15 && irand(2) == 0) { res = "The public voted... YOU and your partner are least popular. You're dumped."; V.done = 1; V.ending = 3; break; }
        if (lowest >= 0) { int q = V.partner[lowest]; V.in[lowest] = 0; if (q >= 0) V.in[q] = 0; res = fmt("%s and %s are dumped from the villa!", nm(V.sim[lowest]).c_str(), q >= 0 ? nm(V.sim[q]).c_str() : "their partner"); }
        break; }
    case 10: if (choice != 2) { bump(p, 10); if (irand(2)) { V.hideaway++; res = "You get the HIDEAWAY tonight. Fairy lights and a four-poster bed."; } else res = "A candlelit dinner for two. The villa cooked for you!"; } else res = "You cook dinner for the dating couples. Team player!"; break;
    case 12: { for (int k = 0; k < 6; k++) { int c = recruit(k % 2 ? !myGuy() : myGuy(), true); (void)c; }
        res = "Casa Amor! Six new singles move in. Your partner is in the other villa.";
        if (choice == 1) { auto o = options(false); for (int i : o) if (V.casa[i]) { bump(i, 20); V.cheated++; res += fmt(" You can't stop talking to %s...", nm(V.sim[i]).c_str()); break; } }
        else bump(p, 6);
        break; }
    case 13: res = choice == 0 ? "You win the Casa challenge and pick a date at the arcade." : "You sit this one out and write your partner a letter."; if (choice == 0) V.winsChallenge++; break;
    case 15: {
        pickList = options(true); pickKind = 2;
        if (choice < 0) { scr = VS_PICK; return; }
        int i = pickList[choice % pickList.size()];
        if (i == p) { bump(p, 20); res = fmt("You stick with %s. ", nm(V.sim[i]).c_str()); if (irand(100) < 25 + V.cheated * 10) res += "...but THEY come back with someone new. Heartbreak."; }
        else { couple(0, i); bump(i, 10); res = fmt("You twist - %s comes back to the villa with you! Your old partner is SHOCKED.", nm(V.sim[i]).c_str()); }
        npcCouple(); for (int k = 1; k < V.n; k++) if (V.casa[k] && V.partner[k] < 0) V.in[k] = 0;
        break; }
    case 16: if (choice == 0) { bump(p, 8 - V.cheated * 4); res = V.cheated ? "Movie night shows EVERYTHING. You apologize. It's... a start." : "Movie night: you have nothing to hide. Your partner beams."; }
             else { bump(p, -5); res = "You deny it all. The footage disagrees."; } break;
    case 19: {
        int score = (p >= 0 ? V.bond[p] : 0) + V.winsChallenge * 6 + V.hideaway * 4 - V.cheated * 8 + irand(25);
        if (p < 0) { V.ending = 3; res = "No partner at the finale..."; }
        else if (score >= 70) { V.ending = 0; G.money += 50000; res = fmt("YOU and %s WIN the villa! The public loved your story. +$50,000!", nm(V.sim[p]).c_str()); changeRel(G.mc, V.sim[p], 30, 30); }
        else if (score >= 45) { V.ending = 1; res = fmt("Runners-up! You and %s leave together, stronger than ever.", nm(V.sim[p]).c_str()); changeRel(G.mc, V.sim[p], 20, 20); }
        else { V.ending = 2; res = "You made the final, but the spark faded. You leave as friends."; changeRel(G.mc, V.sim[p], 10, 0); }
        V.done = 1; break; }
    }
    if (!V.done && V.day < 19) { V.day++; }
    else if (!V.done) V.done = 1;
    if (V.done) addMemoryOnce(MEM_LOVE, G.mc, -1, "Was an islander on Love Villa.");
    scr = VS_RESULT; sfxTap();
}
const char* CH[20][3] = {
    {"", "", ""}, {"Flirt with the bombshell", "Stay loyal", "Watch the drama"}, {"Build fast", "Build fancy", "Build silly"}, {"Flirt with a bombshell", "Stay loyal", "Watch the drama"}, {"", "", ""},
    {"Aggressive pong", "Team play", "Trick shots"}, {"Committed!", "Open to options", "Keep it mysterious"}, {"Flirt with a newcomer", "Stay loyal", "Watch the drama"},
    {"Paint your partner", "Paint the sunset", "Abstract chaos"}, {"Await the vote...", "Await the vote...", "Await the vote..."}, {"Hope for the Hideaway", "Ask for a dinner date", "Cook for the others"},
    {"Dance for your partner", "Dance for someone else", "Sit it out"}, {"Stay loyal", "Explore Casa", "Stay loyal"}, {"Win the challenge", "Sit it out", "Sit it out"}, {"Stay loyal", "Kiss someone new", "Skip the game"},
    {"", "", ""}, {"Own up to everything", "Deny it", "Deny it"}, {"Bowl for real", "Team strategy", "Trash talk"}, {"", "", ""}, {"Walk into the finale", "Walk into the finale", "Walk into the finale"}};
}

void villaOpen() { scr = V.active && !V.done ? VS_DAY : VS_INTRO; }
bool villaUpdate(const Input& in) {
    if (in.down & KEY_B) return false;
    if (!in.touch && !(in.down & KEY_A)) return true;
    const auto tp = in.tp;
    switch (scr) {
    case VS_INTRO:
        if (in.touch && bBack.in(tp)) return false;
        if ((in.touch && bGo.in(tp)) || (in.down & KEY_A)) { if (MC().stage < ST_YA) { toast("Love Villa is for adults only."); sfxNo(); return true; } startVilla(); }
        return true;
    case VS_DAY: {
        const bool pick = V.day == 0 || V.day == 4 || V.day == 18 || V.day == 15;
        if (in.touch && bBack.in(tp)) return false;
        if (pick) { if ((in.touch && bGo.in(tp)) || (in.down & KEY_A)) doDay(-1); return true; }
        for (int c = 0; c < 3; c++) if (in.touch && rowB(c + 3).in(tp)) { doDay(c); return true; }
        return true; }
    case VS_PICK:
        for (int k = 0; k < (int)pickList.size() && k < 6; k++) if (in.touch && rowB(k).in(tp)) { doDay(k); return true; }
        return true;
    case VS_RESULT:
        if ((in.touch && bGo.in(tp)) || (in.down & KEY_A)) scr = V.done ? VS_END : VS_DAY;
        return true;
    case VS_END:
        if ((in.touch && (bGo.in(tp) || bBack.in(tp))) || (in.down & KEY_A)) { V.active = 0; scr = VS_INTRO; return false; }
        return true;
    }
    return true;
}
void villaDrawTop(float t) {
    grad(0, 0, TOP_W, TOP_H, C(110, 200, 245), C(255, 190, 170));
    rect(0, 150, TOP_W, 90, C(60, 190, 220)); for (int k = 0; k < 8; k++) rect(20 + k * 48 + 8 * std::sin(t + k), 166 + (k % 3) * 18.f, 26, 2, C(255, 255, 255, 140));
    for (int k = 0; k < 2; k++) { float x = 30 + k * 340.f; line(x, 150, x + 8, 60, 5, C(140, 100, 60)); for (int f = 0; f < 5; f++) { float a = f * 1.25f; line(x + 8, 60, x + 8 + std::cos(a) * 30, 60 + std::sin(a) * 14 + 6, 4, C(60, 170, 80)); } }
    textOutline("LOVE VILLA", 200, 4, 0.7f, C(255, 255, 255), C(240, 60, 130), 1);
    float y = 30;
    auto para = [&](const std::string& s, float sc, RGB c) { for (auto& l : wrapText(s, sc, 360)) { if (y > 140) break; textOutline(l.c_str(), 200, y, sc, C(c), C(80, 30, 60), 1, 0.8f); y += sc * 32; } };
    if (scr == VS_INTRO) { para("A 20-day dating villa. Couple up, survive bombshells, Casa Amor and public votes, and win the $50,000 prize.", 0.45f, RGB{255, 255, 255}); para("(Adults only. Your town life waits for you.)", 0.38f, RGB{255, 230, 240}); return; }
    if (scr == VS_RESULT || scr == VS_END) para(res, 0.44f, RGB{255, 255, 255});
    else { para(fmt("Day %d: %s", V.day + 1, DAYS[V.day]), 0.5f, RGB{255, 240, 120}); para(DAYTXT[V.day], 0.42f, RGB{255, 255, 255}); }
    // islanders, coupled ones side by side
    float x = 22;
    for (int i = 0; i < V.n && x < 390; i++) {
        if (!V.in[i] || !simOk(V.sim[i])) continue;
        drawSimPortrait(G.sims[V.sim[i]], x, 200, 13, t);
        if (V.partner[i] >= 0) heart(x + 12, 188, 4, C(255, 80, 140));
        if (i == 0) text("you", x, 215, 0.28f, C(255, 255, 255), 1);
        x += 30;
    }
}
void villaDrawBottom(float t) {
    (void)t;
    grad(0, 0, BOT_W, BOT_H, C(255, 236, 244), C(255, 214, 230));
    switch (scr) {
    case VS_INTRO: button(bBack, "Back", false, 1, 0.4f); button(bGo, "Enter the Villa", true, 2, 0.4f); break;
    case VS_DAY: {
        const int p = myPartner();
        text(p >= 0 ? fmt("Coupled with %s  (chemistry %d)", nm(V.sim[p]).c_str(), V.bond[p]).c_str() : "You're single!", 160, 6, 0.4f, C(BERRY), 1);
        if (V.day == 0 || V.day == 4 || V.day == 18 || V.day == 15) button(bGo, V.day == 15 ? "Stick or Twist..." : "Choose who to couple with", true, 2, 0.36f);
        else for (int c = 0; c < 3; c++) button(rowB(c + 3), CH[V.day][c], false, c, 0.38f);
        button(bBack, "Pause", false, 1, 0.4f);
        break; }
    case VS_PICK:
        text("Pick an islander", 160, 6, 0.42f, C(BERRY), 1);
        for (int k = 0; k < (int)pickList.size() && k < 6; k++) { int i = pickList[k]; button(rowB(k), fmt("%s%s  (chemistry %d)", nm(V.sim[i]).c_str(), i == myPartner() ? " - your partner" : V.partner[i] >= 0 ? " - coupled" : "", V.bond[i]).c_str(), i == myPartner(), V.casa[i] ? 3 : 0, 0.38f); }
        break;
    case VS_RESULT: button(bGo, "Next Day", true, 2, 0.4f); break;
    case VS_END: { static const char* E[4] = {"WINNERS!", "Runners-up", "Finalists", "Dumped"}; text(E[V.ending % 4], 160, 90, 0.7f, C(BERRY), 1); button(bGo, "Leave the Villa", true, 2, 0.4f); break; }
    }
}
void villaSaveTo(const std::string& path) { FILE* f = fopen(path.c_str(), "wb"); if (!f) return; fwrite(&V, sizeof V, 1, f); fclose(f); }
void villaLoadFrom(const std::string& path) {
    V = Villa{}; FILE* f = fopen(path.c_str(), "rb"); if (!f) return;
    Villa v; if (fread(&v, sizeof v, 1, f) == 1 && v.magic == 0x564C4131 && v.n <= 14 && v.day >= 0 && v.day < 20) {
        bool ok = true; for (int i = 0; i < v.n; i++) { if (!simOk(v.sim[i])) ok = false; if (v.partner[i] >= v.n) v.partner[i] = -1; }
        if (ok) V = v;
    }
    fclose(f);
}

// ---------------------------------------------------------------- cross-system story events (E001-E012)
namespace {
int evSim = -1;
int findNamed(const char* first) { for (size_t i = 0; i < G.sims.size(); i++) if (G.sims[i].name.rfind(std::string(first) + " ", 0) == 0 && (int)i != G.mc) return (int)i; return -1; }
int anyFriend(int minFr) { for (const Rel& r : G.rels) { int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1; if (o >= 0 && r.fr >= minFr && !G.sims[o].household) return o; } return -1; }
std::string evGeneric(int ch, int k) {
    // payoff tables: [event][choice] -> friendship change, money, text
    struct P { int fr, money; const char* text; };
    static const P PAY[12][3] = {
        {{12, -40, "Your personalized gift is the hit of the party. They hug you twice."}, {4, -20, "A generic gift. They're polite about it."}, {-3, 0, "You show up empty-handed and apologize. They laugh it off... mostly."}},
        {{14, -60, "You gift the exact keycaps. They're speechless."}, {8, -20, "You help hunt down repair parts together."}, {3, 0, "You cheer them on while they fix it themselves."}},
        {{10, 0, "Watch party! You both trash-talk the contestants' outfits."}, {6, 0, "You join their club watch party. New friends!"}, {-2, 0, "You complain about the show. They stop inviting you."}},
        {{10, -15, "You cook side by side. The dish is incredible."}, {8, -15, "You let them lead. They're a natural!"}, {2, -30, "Takeout it is. Still a good night."}},
        {{12, -20, "Your dish is the star of the potluck."}, {6, -10, "Store-bought, but nobody cares - it's about the company."}, {10, 0, "You help the struggling cook save their dish. Hero."}},
        {{4, 0, "You toss the old leftovers. Fresh start."}, {-6, 0, "You pretend it's fine. Someone gets a stomachache."}, {10, -15, "You cook a new meal together instead."}},
        {{8, 0, "You reschedule for a sunny day. They appreciate it."}, {6, -10, "You swap to an indoor cafe date. Cozy!"}, {-8, 0, "You guilt-trip them. That did NOT land."}},
        {{4, 0, "\"No spoilers!\" They zip their lips."}, {2, 0, "You ask for everything. Now you know who wins."}, {-2, 0, "You tease them. They tease back harder."}},
        {{10, 0, "You switch to their favorite spot. The night is perfect."}, {12, 0, "You ask what they'd like. Great call."}, {-5, 0, "You insist on the reservation. They're quiet all dinner."}},
        {{14, 0, "You gift the record. They play it on repeat for a week."}, {3, 0, "You keep it for yourself. It sounds amazing."}, {0, 40, "You sell it for $40. No regrets. Maybe one."}},
        {{10, 0, "You practice the routine together. You're both sore and happy."}, {-3, 0, "You roast the dance coach. They loved that coach."}, {6, 0, "You try a solo audition. Bold!"}},
        {{12, -40, "Housewarming party! The whole street shows up."}, {8, -20, "You swap small gifts with the neighbors."}, {4, 0, "A quiet night in your new place. Perfect."}}};
    const P& p = PAY[k][ch % 3];
    if (evSim >= 0) changeRel(G.mc, evSim, p.fr, 0);
    G.money = std::max<long>(0L, (long)G.money + p.money);
    addMemoryOnce(MEM_FRIEND, G.mc, evSim, p.text);
    return p.text;
}
#define EVFN(k) std::string ev##k(int c, int) { return evGeneric(c, k); }
EVFN(0) EVFN(1) EVFN(2) EVFN(3) EVFN(4) EVFN(5) EVFN(6) EVFN(7) EVFN(8) EVFN(9) EVFN(10) EVFN(11)
MomentFn EVF[12] = {ev0, ev1, ev2, ev3, ev4, ev5, ev6, ev7, ev8, ev9, ev10, ev11};
}
void storyEventsDaily() {
    if (G.sims.empty() || momentActive() || irand(100) >= 22) return;
    const bool adult = MC().stage >= ST_YA;
    std::vector<int> ok;
    auto sim = [&](int k) -> int {
        switch (k) {
        case 0: return findNamed("Alana") >= 0 ? findNamed("Alana") : anyFriend(20);
        case 1: return findNamed("Rio") >= 0 ? findNamed("Rio") : anyFriend(20);
        case 2: return G.tvWatched ? (findNamed("Suki") >= 0 ? findNamed("Suki") : anyFriend(10)) : -1;
        case 3: case 4: return anyFriend(30);
        case 5: { bool old = false; for (auto& f : G.foodInv) if ((int)G.day - (int)f.day > 6) old = true; return old ? anyFriend(0) : -1; }
        case 6: return adult ? anyFriend(40) : -1;
        case 7: return G.tvWatched ? anyFriend(10) : -1;
        case 8: return adult ? anyFriend(40) : -1;
        case 9: return findNamed("Dora") >= 0 ? findNamed("Dora") : anyFriend(20);
        case 10: return G.tvWatched >> 7 & 1 ? anyFriend(10) : -1;
        case 11: return G.homeId >= 2 ? anyFriend(0) : -1;
        }
        return -1;
    };
    for (int k = 0; k < 12; k++) if (!(G.storyEvDone >> k & 1) && sim(k) >= 0) ok.push_back(k);
    if (ok.empty()) return;
    const int k = ok[irand((int)ok.size())]; evSim = sim(k);
    G.storyEvDone |= 1u << k;
    const std::string n = nm(evSim);
    static const char* T[12] = {"The Birthday Mix-Up", "The Broken Keyboard", "The Fashion Watch Party", "One Pan, Two Chefs", "Potluck Club Night", "The Spoiled Leftovers",
                                "Garden Pass Raincheck", "The Episode Spoiler", "First Date, Wrong Restaurant", "The Thrifted Record", "The Dance Show Challenge", "Moving Day Housewarming"};
    const std::string X[12] = {
        "It's " + n + "'s birthday week and you're invited to the party!", n + "'s keyboard just broke right before a big deadline.", n + " is hosting a watch party for The Maple Edit.",
        n + " is over and you're both starving. The stove is right there...", "Your club's potluck is tonight and " + n + " is bringing their famous dish.", "There's something green growing in the fridge. " + n + " is staring at it.",
        "You had garden plans with " + n + ", but it's pouring rain.", n + " just watched the new episode... and they look like they're about to spill.", "You booked a fancy place for your date with " + n + " - but they can't eat anything on the menu.",
        "You found a rare vintage record at the thrift store. " + n + " collects these.", "After that Studio Storm episode, " + n + " wants to try the routine.", "You just moved! " + n + " asks if you're throwing a housewarming."};
    static const char* O[12][3] = {{"Personalized gift", "Generic gift", "Apologize, no gift"}, {"Gift keycaps", "Help find parts", "Encourage them"}, {"Invite them over", "Join their party", "Complain about the show"},
                                   {"Cook together", "Let them lead", "Order takeout"}, {"Cook something amazing", "Bring store food", "Help someone struggling"}, {"Toss it", "Pretend it's fine", "Cook something new together"},
                                   {"Reschedule", "Swap to a cafe", "Guilt-trip them"}, {"Ask them to stop", "Ask for details", "Tease them"}, {"Change venue", "Ask what they want", "Insist on it"},
                                   {"Gift it to them", "Keep it", "Sell it"}, {"Practice together", "Roast the coach", "Try a solo"}, {"Invite the neighbors", "Swap small gifts", "Celebrate quietly"}};
    momentCustom(T[k], X[k], O[k][0], O[k][1], O[k][2], EVF[k], 0, evSim);
}
