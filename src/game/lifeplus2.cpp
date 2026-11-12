#include "lifeplus2.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "people.hpp"
#include "catalog.hpp"
#include "extras.hpp"
#include "emote.hpp"
#include "growup.hpp"
#include "tween.hpp"
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include "vfs.hpp"
#define fopen vfopen
using namespace col;

namespace {
// ------------------------------------------------------------------ saved state
struct LP {
    uint32_t magic = 0x4C505031;
    uint8_t asp = 0, aspDone = 0, clique = 0, cliqueRival = 0, nemesis = 0xFF, petPers[8] = {}, vacations = 0, pad = 0;
    uint16_t cnt[16] = {};              // counters for goals
    uint8_t bucket[6] = {}, bucketDone = 0, week[3] = {}, weekDone = 0;
    uint16_t weekNo = 0xFFFF;
    int16_t secretOf[6] = {-1, -1, -1, -1, -1, -1}; uint8_t secretKind[6] = {}, nSecrets = 0;
    uint8_t overdueDays = 0, evicted = 0, bestRunway = 0, eventSeason = 0xFF;
};
LP P;
enum { C_RUNWAY, C_DANCE, C_COOK, C_SALON, C_SHOP, C_PARTY, C_DATE, C_WORKOUT, C_SOCIAL, C_SECRET, C_RUMOR, C_EVENT, C_VACAY, C_TRICK, C_CLIQUE, C_NEMWIN };
std::string first(int s) { const std::string& n = G.sims[s].name; return n.substr(0, n.find(' ')); }
Sim& me() { return MC(); }
bool pay(int c) { if (G.money < c) { toast("Not enough money ($" + std::to_string(c) + ")."); return false; } G.money -= c; return true; }

// ------------------------------------------------------------------ the shared tap-timing minigame
enum { MG_COOK, MG_SALON, MG_RUNWAY, MG_SHOP, MG_DANCE, MG_EVENT };
struct Mini { bool on = false; int kind = 0, round = 0, rounds = 5, score = 0, rivalScore = -1; float pos = 0, dir = 1, speed = 1, zone = 0.5f, zoneW = 0.16f, flash = 0, wait = 0; std::string last; int tag = 0; };
Mini M;
const char* MG_TITLE[6] = {"COOK-OFF", "SALON STYLIST", "RUNWAY SHOW", "SHOPPING SPREE", "DANCE BATTLE", "TOWN EVENT"};
const char* MG_VERB[6][5] = {
    {"Chop!", "Stir!", "Season!", "Flip!", "Plate it!"}, {"Wash!", "Section!", "Curl!", "Spray!", "Reveal!"},
    {"Walk!", "Turn!", "Pose!", "Hair flip!", "Exit!"}, {"Grab!", "Grab!", "Grab!", "Grab!", "Checkout!"},
    {"Step!", "Spin!", "Drop!", "Freeze!", "Finale!"}, {"Go!", "Go!", "Go!", "Go!", "Go!"}};
const RGB MG_COL[6] = {{255, 170, 90}, {255, 120, 200}, {200, 120, 255}, {120, 200, 255}, {255, 90, 140}, {120, 220, 160}};
void newZone() { M.zone = 0.15f + irand(70) / 100.f; M.zoneW = std::max(0.07f, 0.17f - M.round * 0.015f); M.speed = 0.9f + M.round * 0.18f + irand(20) / 100.f; }
void startMini(int kind, int tag, int rival) { M = Mini{}; M.on = true; M.kind = kind; M.tag = tag; M.rivalScore = rival; newZone(); }
void finishMini();
}

bool lpMiniActive() { return M.on; }

bool lpMiniUpdate(const Input& in) {
    if (!M.on) return false;
    const float dt = std::min(0.05f, in.dt);
    M.flash = std::max(0.f, M.flash - dt * 2);
    if (M.wait > 0) { M.wait -= dt; if (M.wait <= 0 && M.round >= M.rounds) { finishMini(); return false; } return true; }
    M.pos += M.dir * M.speed * dt; if (M.pos > 1) { M.pos = 1; M.dir = -1; } if (M.pos < 0) { M.pos = 0; M.dir = 1; }
    if ((in.down & (KEY_A | KEY_B | KEY_X | KEY_Y)) || in.touch) {
        const float d = std::fabs(M.pos - M.zone);
        const int pts = d < M.zoneW * 0.35f ? 3 : d < M.zoneW ? 2 : d < M.zoneW * 2 ? 1 : 0;
        M.score += pts; M.last = pts == 3 ? "PERFECT!" : pts == 2 ? "Great!" : pts == 1 ? "Okay..." : "Miss!"; M.flash = 1;
        if (pts == 3) sfxYay(); else if (pts == 0) sfxNo(); else sfxTap();
        M.round++; M.wait = 0.5f; if (M.round < M.rounds) newZone();
    }
    return true;
}

void lpMiniDrawTop(float t) {
    const RGB c = MG_COL[M.kind % 6];
    grad(0, 0, TOP_W, TOP_H, C(c.r / 3 + 40, c.g / 3 + 30, c.b / 3 + 60), C(30, 20, 50));
    for (int i = 0; i < 16; i++) { const float x = std::fmod(i * 37.f + t * 20, 400.f); circle(x, 30 + (i % 4) * 8, 1.5f, C(255, 255, 255, 120)); }
    textOutline(MG_TITLE[M.kind % 6], 200, 14, 0.9f, C(c), C(255, 255, 255), 1);
    drawSimPortrait(me(), 200, 120, 46, t);
    if (M.flash > 0) textOutline(M.last.c_str(), 200, 176, 0.8f + M.flash * 0.2f, C(255, 255, 255, (uint8_t)(255 * std::min(1.f, M.flash * 2))), C(c), 1);
    char b[48]; snprintf(b, sizeof b, "Round %d/%d   Score %d", std::min(M.round + 1, M.rounds), M.rounds, M.score); text(b, 200, 210, 0.45f, C(WHITE), 1);
    if (M.rivalScore >= 0) { snprintf(b, sizeof b, "Rival to beat: %d", M.rivalScore); text(b, 200, 226, 0.36f, C(255, 180, 200), 1); }
}
void lpMiniDrawBottom(float t) {
    (void)t; const RGB c = MG_COL[M.kind % 6];
    rect(0, 0, 320, 240, C(40, 30, 56));
    text(M.round < M.rounds ? MG_VERB[M.kind % 6][M.round % 5] : "Done!", 160, 30, 0.8f, C(c), 1);
    const float x0 = 20, w = 280, y = 110;
    rrect(x0, y, w, 26, 10, C(70, 60, 90));
    rrect(x0 + (M.zone - M.zoneW) * w, y + 2, M.zoneW * 2 * w, 22, 8, C(c.r, c.g, c.b, 140));
    rrect(x0 + (M.zone - M.zoneW * 0.35f) * w, y + 2, M.zoneW * 0.7f * w, 22, 6, C(255, 255, 255, 170));
    rect(x0 + M.pos * w - 2, y - 8, 4, 42, C(255, 255, 255)); tri(x0 + M.pos * w - 6, y - 12, x0 + M.pos * w + 6, y - 12, x0 + M.pos * w, y - 4, C(255, 255, 255));
    text("Tap or press A when the marker hits the glow!", 160, 160, 0.38f, C(220, 200, 230), 1);
}

namespace {
void finishMini() {
    M.on = false; Sim& s = me(); const int sc = M.score, max = M.rounds * 3; const bool great = sc >= max * 0.7f;
    switch (M.kind) {
    case MG_COOK: { s.skills[SK_COOKING] = std::min(10.f, s.skills[SK_COOKING] + 0.2f + sc * 0.03f); for (auto& x : G.sims) if (x.household) x.needs[N_HUNGER] = std::min(100.f, x.needs[N_HUNGER] + 20 + sc * 3);
        toast(great ? "Chef's kiss! The family is OBSESSED with dinner." : sc >= max / 2 ? "Dinner was solid. Nobody complained." : "Dinner was... charred. Pizza delivery it is."); P.cnt[C_COOK]++; break; }
    case MG_SALON: { const int pay2 = 20 + sc * 6; G.money += pay2; s.skills[SK_STYLE] = std::min(10.f, s.skills[SK_STYLE] + 0.15f); toast((great ? "Your client cried happy tears! " : "Client left... fine. ") + std::string("+$") + std::to_string(pay2)); P.cnt[C_SALON]++; break; }
    case MG_RUNWAY: { const bool trendy = lxTrendy(s.outfit); const int total = sc + (trendy ? 3 : 0); P.bestRunway = (uint8_t)std::max<int>(P.bestRunway, total);
        if (M.rivalScore >= 0) { if (total > M.rivalScore) { toast("You OUTSHINE your nemesis on the runway! (+" + std::to_string(total - M.rivalScore) + ")"); P.cnt[C_NEMWIN]++; addMoodlet(s, "Beat My Nemesis", EM_CONFIDENT, 3, 1440); } else toast("Your nemesis won the runway. This isn't over."); }
        else { const int prize = total >= 14 ? 300 : total >= 10 ? 120 : 40; G.money += prize; toast((total >= 14 ? "1st place, runway queen! " : total >= 10 ? "2nd place! " : "You walked. That's what matters. ") + std::string("+$") + std::to_string(prize) + (trendy ? " (on-trend bonus!)" : "")); }
        lxRepAdd(CI_FASHION, sc / 3, "runway"); guPhoto("Runway show"); P.cnt[C_RUNWAY]++; break; }
    case MG_SHOP: { int got = 0; for (int k = 0; k < sc / 4 && got < 4; k++) { const int o = 1 + irand(FIRST_KID_OUTFIT - 1); if (!ownsOutfit(o)) { giveOutfit(o); got++; } }
        toast(got ? "Shopping spree haul: " + std::to_string(got) + " new outfits! (check your wardrobe)" : "You grabbed... a scrunchie. Better luck next time."); P.cnt[C_SHOP]++; break; }
    case MG_DANCE: { const int opp = M.rivalScore >= 0 ? M.rivalScore : 6 + irand(6); if (sc > opp) { G.money += 100; toast("You WON the dance battle! " + std::to_string(sc) + " to " + std::to_string(opp) + ". +$100"); if (M.rivalScore >= 0) P.cnt[C_NEMWIN]++; }
        else toast("Lost the dance battle " + std::to_string(sc) + " to " + std::to_string(opp) + ". Rematch?"); s.skills[SK_FITNESS] = std::min(10.f, s.skills[SK_FITNESS] + 0.15f); P.cnt[C_DANCE]++; break; }
    case MG_EVENT: { static const char* EV[4] = {"Spring Formal", "Summer Carnival", "Fall Harvest Fest", "Winter Ball"}; const char* n = EV[M.tag % 4];
        if (great) { addMoodlet(s, "Belle of the Ball", EM_CONFIDENT, 3, 1440); toast(std::string(n) + ": you were the STAR of the night!"); lxRepAdd(CI_PARTY, 3, n); }
        else toast(std::string(n) + ": fun night out!");
        guPhoto(n); P.cnt[C_EVENT]++; break; }
    }
    emotePlay(G.mc, great ? EMO_HAPPY : sc >= max / 2 ? EMO_IDEA : EMO_SWEAT);
}

// ------------------------------------------------------------------ aspirations / bucket list / weekly challenges
struct Asp { const char* name; const char* steps[4]; };
const Asp ASP[6] = {
    {"Fashion Icon", {"Walk a runway", "Fashion rep 20+", "Own 40 outfits", "Win 1st place on the runway"}},
    {"Big Happy Family", {"Live with 3+ family", "Cook 3 family dinners", "Family photo x5", "Everyone at home 60+ friendship"}},
    {"Mean Girl Queen", {"Get a nemesis", "Start 3 rumors", "Join the Populars", "Beat your nemesis twice"}},
    {"Mogul", {"Have $5,000", "Earn from the salon x3", "Have $20,000", "Have $75,000"}},
    {"Party Animal", {"Win a dance battle", "Party rep 20+", "Go to 2 town events", "Win 3 dance battles"}},
    {"Jet Setter", {"Take a vacation", "Take 3 vacations", "Have $3,000 saved", "Visit all 4 destinations"}},
};
int ownedCount() { int n = 0; for (int i = 0; i < NUM_OUTFITS; i++) n += ownsOutfit(i); return n; }
int famCount() { int n = 0; for (int i = 0; i < (int)G.sims.size(); i++) if (i != G.mc && G.sims[i].household && isFamily(G.mc, i)) n++; return n; }
bool aspStep(int a, int k) {
    switch (a * 4 + k) {
    case 0: return P.cnt[C_RUNWAY] >= 1; case 1: return lxRep(CI_FASHION) >= 20; case 2: return ownedCount() >= 40; case 3: return P.bestRunway >= 14;
    case 4: return famCount() >= 3; case 5: return P.cnt[C_COOK] >= 3; case 6: return guPhotoCount() >= 5;
    case 7: { int lo = 100; for (int i = 0; i < (int)G.sims.size(); i++) if (i != G.mc && G.sims[i].household) lo = std::min(lo, (int)rel(G.mc, i).fr); return lo >= 60; }
    case 8: return P.nemesis != 0xFF; case 9: return P.cnt[C_RUMOR] >= 3; case 10: return P.clique == 1; case 11: return P.cnt[C_NEMWIN] >= 2;
    case 12: return G.money >= 5000; case 13: return P.cnt[C_SALON] >= 3; case 14: return G.money >= 20000; case 15: return G.money >= 75000;
    case 16: return P.cnt[C_DANCE] >= 1 && P.cnt[C_NEMWIN] + P.cnt[C_DANCE] >= 1; case 17: return lxRep(CI_PARTY) >= 20; case 18: return P.cnt[C_EVENT] >= 2; case 19: return P.cnt[C_DANCE] >= 3;
    case 20: return P.cnt[C_VACAY] >= 1; case 21: return P.cnt[C_VACAY] >= 3; case 22: return G.money >= 3000; case 23: return (P.vacations & 15) == 15;
    }
    return false;
}
struct Goal { const char* name; int c; int need; };
const Goal POOL[] = {
    {"Win a dance battle", C_DANCE, 1}, {"Walk 2 runways", C_RUNWAY, 2}, {"Cook 2 meals (minigame)", C_COOK, 2}, {"Style 2 salon clients", C_SALON, 2},
    {"Go on a shopping spree", C_SHOP, 1}, {"Discover a secret", C_SECRET, 1}, {"Start a rumor", C_RUMOR, 1}, {"Attend a town event", C_EVENT, 1},
    {"Take a vacation", C_VACAY, 1}, {"Teach your pet a trick", C_TRICK, 1}, {"Join a clique", C_CLIQUE, 1}, {"Beat your nemesis", C_NEMWIN, 1},
    {"Walk 5 runways", C_RUNWAY, 5}, {"Win 3 dance battles", C_DANCE, 3}, {"Cook 5 meals (minigame)", C_COOK, 5}, {"Discover 3 secrets", C_SECRET, 3},
};
constexpr int NPOOL = sizeof POOL / sizeof POOL[0];
uint16_t base[16] = {};   // counters at the time a goal was set (weekly)
const char* CLIQUES[5] = {"", "Populars", "Nerds", "Jocks", "Emos"};
const char* SECRETS[8] = {"secretly failing a class", "dating someone's ex", "in huge credit card debt", "the one who started the rumor",
                          "cheated on a test", "has a secret second phone", "a famous TikTak account under a fake name", "can't actually swim"};
const char* DEST[4] = {"Sunset Beach Resort", "Big City Weekend", "Snowy Mountain Lodge", "Tropical Island"};
const char* PETP[5] = {"", "Playful", "Lazy", "Diva", "Chaotic"};
}

void lpCount(int what) { if (what == LP_COOK) P.cnt[C_COOK]++; else if (what == LP_SOCIAL) P.cnt[C_SOCIAL]++; }

LxMenu lpApp() {
    LxMenu m; const Sim& s = me();
    m.push_back({"-- Minigames --", 24999});
    m.push_back({"Cook-Off (cooking minigame)", 24001}); m.push_back({"Salon Stylist Shift", 24002}); m.push_back({"Runway Show ($20 entry)", 24003});
    m.push_back({"Mall Shopping Spree ($150)", 24004}); m.push_back({"Dance Battle", 24005});
    m.push_back({P.asp ? std::string("Aspiration: ") + ASP[P.asp - 1].name : "Pick a Life Aspiration", 24010});
    m.push_back({"Bucket List", 24020}); m.push_back({"Weekly Challenges", 24030});
    if (s.stage >= ST_PRETEEN && s.stage <= ST_YA) m.push_back({P.clique ? std::string("Clique: ") + CLIQUES[P.clique] : "Join a Clique", 24040});
    m.push_back({P.nemesis != 0xFF && P.nemesis < G.sims.size() ? "Nemesis: " + G.sims[P.nemesis].name : "Nemesis: (none yet)", 24050});
    m.push_back({"Secrets (" + std::to_string(P.nSecrets) + ")", 24060});
    m.push_back({"Start a Rumor", 24070}); m.push_back({"Squash Rumors About You", 24075});
    if (!G.pets.empty()) m.push_back({"Pet: " + std::string(PETP[P.petPers[0]]) + " personality", 24080});
    m.push_back({"Seasonal Town Event", 24090});
    m.push_back({"Take a Vacation", 24100});
    m.push_back({"Pawn Shop (sell furniture fast)", 24110});
    return m;
}

LxMenu lpOption(int id) {
    LxMenu m; Sim& s = me();
    switch (id) {
    case 24001: startMini(MG_COOK, 0, -1); return {};
    case 24002: startMini(MG_SALON, 0, -1); return {};
    case 24003: if (!pay(20)) return {}; startMini(MG_RUNWAY, 0, -1); return {};
    case 24004: if (!pay(150)) return {}; startMini(MG_SHOP, 0, -1); return {};
    case 24005: startMini(MG_DANCE, 0, -1); return {};
    // aspirations
    case 24010: if (P.asp) { const Asp& a = ASP[P.asp - 1]; for (int k = 0; k < 4; k++) m.push_back({std::string(aspStep(P.asp - 1, k) ? "[X] " : "[ ] ") + a.steps[k], 24999}); m.push_back({"Claim Reward", 24017}); m.push_back({"Change Aspiration", 24018}); return m; }
        for (int k = 0; k < 6; k++) m.push_back({ASP[k].name, 24011 + k}); return m;
    case 24011: case 24012: case 24013: case 24014: case 24015: case 24016: P.asp = (uint8_t)(id - 24010); toast(std::string("New aspiration: ") + ASP[P.asp - 1].name + "!"); return lpOption(24010);
    case 24017: { int n = 0; for (int k = 0; k < 4; k++) n += aspStep(P.asp - 1, k); if (n < 4) { toast(std::to_string(n) + "/4 steps done. Keep going!"); return {}; }
        if (P.aspDone >> (P.asp - 1) & 1) { toast("Already claimed this one. Pick a new aspiration!"); return {}; }
        P.aspDone |= 1 << (P.asp - 1); G.money += 2500; addMoodlet(s, "Aspiration Complete!", EM_CONFIDENT, 3, 2880); guPhoto(std::string("Became a ") + ASP[P.asp - 1].name); toast(std::string("ASPIRATION COMPLETE: ") + ASP[P.asp - 1].name + "! +$2,500"); return {}; }
    case 24018: P.asp = 0; return lpOption(24010);
    // bucket list
    case 24020: { if (!P.bucket[0]) for (int k = 0; k < 6; k++) { int g; bool dup; do { g = 1 + irand(NPOOL); dup = false; for (int j = 0; j < k; j++) dup |= P.bucket[j] == g; } while (dup); P.bucket[k] = (uint8_t)g; }
        int done = 0; for (int k = 0; k < 6; k++) { const Goal& g = POOL[P.bucket[k] - 1]; const bool ok = P.cnt[g.c] >= g.need; done += ok; m.push_back({std::string(ok ? "[X] " : "[ ] ") + g.name, 24999}); }
        if (done == 6 && !P.bucketDone) { P.bucketDone = 1; G.money += 3000; toast("BUCKET LIST COMPLETE! +$3,000 and bragging rights."); guPhoto("Finished my bucket list"); }
        return m; }
    case 24030: { const uint16_t wk = (uint16_t)(G.day / 7);
        if (P.weekNo != wk) { P.weekNo = wk; P.weekDone = 0; for (int k = 0; k < 3; k++) P.week[k] = (uint8_t)(1 + irand(NPOOL)); for (int i = 0; i < 16; i++) base[i] = P.cnt[i]; }
        for (int k = 0; k < 3; k++) { const Goal& g = POOL[P.week[k] - 1]; const bool ok = P.cnt[g.c] - base[g.c] >= g.need || (P.weekDone >> k & 1);
            if (ok && !(P.weekDone >> k & 1)) { P.weekDone |= 1 << k; G.money += 250; toast(std::string("Weekly challenge done: ") + g.name + " (+$250)"); }
            m.push_back({std::string(ok ? "[X] " : "[ ] ") + g.name + " (+$250)", 24999}); }
        m.push_back({"New challenges every 7 days", 24999}); return m; }
    // cliques
    case 24040: if (P.clique) { m.push_back({"Hang With the " + std::string(CLIQUES[P.clique]), 24046}); m.push_back({"Leave the Clique", 24047}); return m; }
        for (int k = 1; k < 5; k++) m.push_back({std::string("Try to join the ") + CLIQUES[k], 24040 + k}); return m;
    case 24041: case 24042: case 24043: case 24044: { const int k = id - 24040;
        const int need = k == 1 ? (int)(lxRep(CI_FASHION) + lxRep(CI_PARTY)) / 2 : k == 2 ? (int)s.skills[SK_LOGIC] * 10 : k == 3 ? (int)s.skills[SK_FITNESS] * 10 : (int)s.skills[SK_CREATIVITY] * 10;
        if (need + irand(40) >= 35) { P.clique = (uint8_t)k; P.cliqueRival = (uint8_t)(1 + (k % 4)); P.cnt[C_CLIQUE]++; toast(std::string("You're officially one of the ") + CLIQUES[k] + "! The " + CLIQUES[P.cliqueRival] + " are NOT happy."); emotePlay(G.mc, EMO_HAPPY); }
        else { toast(std::string("The ") + CLIQUES[k] + " said \"maybe next semester.\" Ouch."); emotePlay(G.mc, EMO_SAD); } return {}; }
    case 24046: s.needs[N_SOCIAL] = std::min(100.f, s.needs[N_SOCIAL] + 30); s.needs[N_FUN] = std::min(100.f, s.needs[N_FUN] + 15); toast(std::string("Hanging with the ") + CLIQUES[P.clique] + ". Inside jokes all day."); return {};
    case 24047: toast(std::string("You left the ") + CLIQUES[P.clique] + ". Free agent era."); P.clique = 0; return {};
    // nemesis
    case 24050: if (P.nemesis == 0xFF || P.nemesis >= G.sims.size()) { for (int i = 0; i < (int)G.sims.size() && (int)m.size() < 40; i++) if (i != G.mc && !isFamily(G.mc, i) && std::abs((int)G.sims[i].stage - (int)s.stage) <= 1 && rel(G.mc, i).met) m.push_back({"Declare " + G.sims[i].name + " your nemesis", 24500 + i});
            if (m.empty()) m.push_back({"(Meet more people first)", 24999}); return m; }
        m.push_back({"Runway Face-Off vs " + first(P.nemesis), 24051}); m.push_back({"Dance Battle vs " + first(P.nemesis), 24052}); m.push_back({"Make Peace", 24053}); return m;
    case 24051: startMini(MG_RUNWAY, 0, 6 + irand(8)); return {};
    case 24052: startMini(MG_DANCE, 0, 6 + irand(7)); return {};
    case 24053: changeRel(G.mc, P.nemesis, 20, 0); toast("You and " + first(P.nemesis) + " called a truce. For now."); P.nemesis = 0xFF; return {};
    // secrets
    case 24060: for (int k = 0; k < P.nSecrets; k++) if (P.secretOf[k] >= 0 && P.secretOf[k] < (int)G.sims.size()) m.push_back({first(P.secretOf[k]) + " is " + SECRETS[P.secretKind[k] % 8], 24600 + k});
        m.push_back({"Snoop for a Secret", 24061}); return m;
    case 24061: { int t = -1; for (int tries = 0; tries < 20 && t < 0; tries++) { const int i = irand((int)G.sims.size()); if (i != G.mc && rel(G.mc, i).met) t = i; }
        if (t < 0 || irand(100) < 35) { toast("You snooped around... nothing juicy today."); return {}; }
        if (P.nSecrets >= 6) { for (int k = 0; k < 5; k++) { P.secretOf[k] = P.secretOf[k + 1]; P.secretKind[k] = P.secretKind[k + 1]; } P.nSecrets = 5; }
        P.secretOf[P.nSecrets] = (int16_t)t; P.secretKind[P.nSecrets] = (uint8_t)irand(8); P.nSecrets++; P.cnt[C_SECRET]++;
        toast("You found out " + first(t) + " is " + SECRETS[P.secretKind[P.nSecrets - 1]] + "!!"); emotePlay(G.mc, EMO_SHOCK); return {}; }
    // rumors
    case 24070: for (int i = 0; i < (int)G.sims.size() && (int)m.size() < 40; i++) if (i != G.mc && rel(G.mc, i).met) m.push_back({"About " + G.sims[i].name, 24700 + i}); if (m.empty()) m.push_back({"(Meet people first)", 24999}); return m;
    case 24075: { const int got = lxSquashRumors(); if (got) { toast("You set the record straight. " + std::to_string(got) + " rumor(s) debunked."); lxRepAdd(CI_PARTY, 2, "set the record straight"); } else toast("No rumors to squash right now."); return {}; }
    // pets
    case 24080: m.push_back({"Teach a Trick", 24081}); m.push_back({"Play Together", 24082}); m.push_back({"Clean Up Its Mess", 24083}); return m;
    case 24081: if (irand(100) < 45 + (P.petPers[0] == 1 ? 25 : P.petPers[0] == 2 ? -20 : 0)) { static const char* T[4] = {"sit", "roll over", "high five", "play dead"}; toast(std::string("Your pet learned to ") + T[irand(4)] + "! Genius baby."); P.cnt[C_TRICK]++; emotePlay(G.mc, EMO_HAPPY); }
                else toast(P.petPers[0] == 3 ? "Your diva pet refused. It doesn't do tricks for free." : "Your pet stared at you blankly. Try again later."); return {};
    case 24082: s.needs[N_FUN] = std::min(100.f, s.needs[N_FUN] + 25); toast(P.petPers[0] == 4 ? "Your chaotic pet knocked over a lamp mid-zoomies." : "Playtime! Your pet is SO happy."); return {};
    case 24083: lxClean(10); toast("Mess cleaned. Your pet looks zero percent sorry."); return {};
    // seasonal events
    case 24090: { static const char* EV[4] = {"Spring Formal", "Summer Carnival", "Fall Harvest Fest", "Winter Ball"}; const int se = seasonOf(G.day);
        m.push_back({std::string("Go to the ") + EV[se] + " ($25)", 24091}); m.push_back({"(Each season has its own event)", 24999}); return m; }
    case 24091: if (!pay(25)) return {}; startMini(MG_EVENT, seasonOf(G.day), -1); return {};
    // vacations
    case 24100: { static const int PR[4] = {600, 400, 700, 1200}; for (int k = 0; k < 4; k++) m.push_back({std::string(DEST[k]) + " $" + std::to_string(PR[k]), 24101 + k}); return m; }
    case 24101: case 24102: case 24103: case 24104: { static const int PR[4] = {600, 400, 700, 1200}; const int k = id - 24101; if (!pay(PR[k])) return {};
        static const char* ACT[4][3] = {{"Jet skiing", "Sunset dinner", "Spa day"}, {"Rooftop bar", "Broadway show", "Thrift haul"}, {"Snowboarding", "Hot cocoa by the fire", "Snowball fight"}, {"Snorkeling", "Luau", "Hammock nap"}};
        for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household) { addMoodlet(G.sims[i], "Vacation Glow", EM_HAPPY, 3, 2880); for (float& n : G.sims[i].needs) n = std::min(100.f, n + 40); }
        toast(std::string(DEST[k]) + ": " + ACT[k][0] + ", " + ACT[k][1] + " & " + ACT[k][2] + ". Best trip EVER.");
        guPhoto(std::string("Vacation: ") + DEST[k]); P.cnt[C_VACAY]++; P.vacations |= 1 << k; G.day += 0; return {}; }
    // pawn shop
    case 24110: { auto& items = G.rooms[G.house][G.room].items; if (!roomIsHome(G.room) || items.empty()) { toast("Go home - pawn something from the room you're in."); return {}; }
        for (size_t k = 0; k < items.size() && k < 30; k++) m.push_back({std::string(SHAPES[itemShape(items[k].item)].name) + " -> $" + std::to_string(itemPrice(items[k].item) / 3), 24800 + (int)k}); return m; }
    }
    if (id >= 24500 && id < 24750 && id - 24500 < (int)G.sims.size()) { P.nemesis = (uint8_t)(id - 24500); changeRel(G.mc, P.nemesis, -20, 0); toast(first(P.nemesis) + " is now your NEMESIS. It's giving rivalry."); emotePlay(G.mc, EMO_ANGRY); return {}; }
    if (id >= 24600 && id < 24606) { const int k = id - 24600; m.push_back({"Keep It Secret (they'll trust you)", 24610 + k}); m.push_back({"Spill It", 24620 + k}); m.push_back({"Blackmail ($200)", 24630 + k}); return m; }
    if (id >= 24610 && id < 24636) {
        const int k = (id - 24600) % 10, act = (id - 24600) / 10; if (k >= P.nSecrets) return {}; const int t = P.secretOf[k];
        if (act == 1) { changeRel(G.mc, t, 8, 0); toast("You'll take it to the grave. " + first(t) + " trusts you more."); }
        if (act == 2) { lxGossip(G.sims[t].name + " is " + SECRETS[P.secretKind[k] % 8], true, CI_PARTY, -1); changeRel(G.mc, t, -20, 0); toast("You spilled it. The whole town knows by dinner."); P.cnt[C_RUMOR]++; }
        if (act == 3) { if (irand(100) < 65) { G.money += 200; changeRel(G.mc, t, -25, 0); toast(first(t) + " paid $200 to keep you quiet. Cold."); } else { changeRel(G.mc, t, -30, 0); lxRepAdd(CI_PARTY, -4, "blackmail"); toast(first(t) + " called your bluff and told everyone you tried to blackmail them!"); } }
        for (int j = k; j < 5; j++) { P.secretOf[j] = P.secretOf[j + 1]; P.secretKind[j] = P.secretKind[j + 1]; } P.nSecrets--; return {};
    }
    if (id >= 24700 && id < 24800 && id - 24700 < (int)G.sims.size()) { const int t = id - 24700; static const char* R[5] = {"has a secret boyfriend in another city", "got kicked out of the mall", "wears a wig", "copied someone's whole personality", "is moving away"};
        lxGossip(G.sims[t].name + " " + R[irand(5)], false, CI_PARTY, -1); P.cnt[C_RUMOR]++; toast("Rumor started. Now we wait..."); if (irand(100) < 25) { changeRel(G.mc, t, -15, 0); toast(first(t) + " found out you started it!!"); } return {}; }
    if (id >= 24800 && id < 24830) { auto& items = G.rooms[G.house][G.room].items; const int k = id - 24800; if (k >= (int)items.size()) return {}; const int cash = itemPrice(items[k].item) / 3; G.money += cash; toast("Pawned for $" + std::to_string(cash) + ". Quick cash, no questions."); items.erase(items.begin() + k); return {}; }
    return {};
}

bool lpBlocks(int room, std::string* why) {
    if (room != R_CLUB && room != R_VELVET) return false;
    if (lxRep(CI_PARTY) <= -25) { if (why) *why = "Bouncer: \"You're not on the list. Not tonight.\" (Party reputation too low)"; return true; }
    return false;
}

void lpHourly(int hour) {
    if (hour == 20 && (G.room == R_CLUB || G.room == R_VELVET) && lxRep(CI_PARTY) >= 40 && irand(3) == 0) { addMoodlet(me(), "VIP Treatment", EM_CONFIDENT, 2, 600); toast("VIP! The bouncer waves you past the line. Free drinks in the lounge."); }
}

void lpDaily() {
    Sim& s = me();
    // pets get personalities and make messes
    for (size_t i = 0; i < G.pets.size() && i < 8; i++) if (!P.petPers[i]) P.petPers[i] = (uint8_t)(1 + irand(4));
    if (!G.pets.empty() && irand(5) == 0) { static const char* MS[5] = {"", "Your playful pet brought you a 'gift' from the yard.", "Your lazy pet slept on your clean laundry.", "Your diva pet knocked your makeup off the counter.", "Your chaotic pet shredded a pillow. Feathers EVERYWHERE."};
        toast(MS[P.petPers[0] % 5]); lxClean(-6); }
    // nemesis sabotage
    if (P.nemesis != 0xFF && P.nemesis < G.sims.size() && irand(100) < 18) { static const char* NS[4] = {"posted a shady subtweet about you", "wore your exact outfit - but better", "flirted with your crush in front of you", "told your teacher/boss you were late"};
        toast("Your nemesis " + first(P.nemesis) + " " + NS[irand(4)] + "!"); addMoodlet(s, "Nemesis Drama", EM_ANGRY, 1, 600); emotePlay(G.mc, EMO_ANGRY); }
    // clique rivalry
    if (P.clique && irand(100) < 12) { toast(std::string("The ") + CLIQUES[P.cliqueRival] + " threw shade at the " + CLIQUES[P.clique] + " in the hallway. Tensions!"); lxRepAdd(CI_SCHOLARS, -1, "clique drama"); }
    if (P.clique && (lxRep(CI_FASHION) + lxRep(CI_PARTY)) / 2 <= -30 && irand(3) == 0) { toast(std::string("You got KICKED OUT of the ") + CLIQUES[P.clique] + ". Brutal."); P.clique = 0; emotePlay(G.mc, EMO_SAD); }
    // money trouble: unpaid bills -> late, repo, eviction warning
    const Bill& b = G.eco.bill;
    if (!b.paid && G.day > b.due) {
        P.overdueDays++;
        if (P.overdueDays == 3) toast("FINAL NOTICE: your bills are 3 days late. Pay in the Wallet app!");
        if (P.overdueDays == 5) { auto& items = G.rooms[G.house][R_LIVING].items; if (!items.empty()) { const int k = irand((int)items.size()); toast(std::string("REPO! They took your ") + SHAPES[itemShape(items[k].item)].name + "."); items.erase(items.begin() + k); } }
        if (P.overdueDays == 8) { toast("EVICTION WARNING: pay your bills or you could lose the house! (Pawn Shop in Life+ for quick cash)"); addMoodlet(s, "Money Stress", EM_TENSE, 3, 2880); }
    } else P.overdueDays = 0;
}

void lpSaveTo(const std::string& p) { FILE* f = fopen(p.c_str(), "wb"); if (!f) return; fwrite(&P, sizeof P, 1, f); fwrite(base, sizeof base, 1, f); fclose(f); }
void lpLoadFrom(const std::string& p) { P = LP{}; for (auto& x : base) x = 0; FILE* f = fopen(p.c_str(), "rb"); if (!f) return; LP l; if (fread(&l, 1, sizeof l, f) == sizeof l && l.magic == P.magic) { P = l; if (fread(base, sizeof base, 1, f) != 1) for (auto& x : base) x = 0; } fclose(f); }
