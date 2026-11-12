// K-Pop Trainee mode: Starlight Entertainment's trainee program.
#include "kpop.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <algorithm>
#include "vfs.hpp"
#define fopen vfopen   // sidecar saves go into the one-file save pack
using namespace col;

namespace {
enum Pos : uint8_t { PS_NONE, PS_VOCAL, PS_DANCE, PS_RAP, PS_VISUAL, PS_LEADER, PS_COUNT };
const char* POS_N[PS_COUNT] = {"No Position", "Main Vocal", "Lead Dancer", "Main Rapper", "Visual", "Leader"};
enum Trait : uint8_t { TT_HARD, TT_LAZY, TT_DIVA, TT_SHY, TT_CHARM, TT_HOTHEAD, TT_SWEET, TT_COUNT };
const char* TRAIT_N[TT_COUNT] = {"Hardworking", "Lazy", "Diva", "Shy", "Charismatic", "Hothead", "Sweetheart"};
// roommate chemistry: +1 good, 0 fine, -1 friction
const int8_t COMPAT[TT_COUNT][TT_COUNT] = {
    // HARD LAZY DIVA SHY CHARM HOT SWEET
    {1, -1, 0, 1, 1, -1, 1},    // hardworking
    {-1, 1, -1, 0, 0, -1, 1},   // lazy
    {0, -1, -1, 1, 0, -1, 1},   // diva
    {1, 0, 1, 0, 1, -1, 1},     // shy
    {1, 0, 0, 1, 0, 0, 1},      // charismatic
    {-1, -1, -1, -1, 0, -1, 0}, // hothead
    {1, 1, 1, 1, 1, 0, 1},      // sweetheart
};
enum St : uint8_t { TS_ACTIVE, TS_OUT, TS_DEBUT };
struct Trainee { char name[16]; uint8_t sk[5]; uint8_t pos, trait, strikes, status, agency; uint16_t fans; };   // sk: vocal dance rap visual leadership
enum Phase : uint8_t { PH_TRAINEE, PH_SHOW, PH_DEBUT, PH_FIRED };
const int NT = 16;   // 0 = you, 1..7 your agency, 8..15 rival agencies (survival show)
struct KState {
    uint32_t magic = 0x4B504F31;   // "KPO1"
    uint8_t started = 0, phase = PH_TRAINEE, evals = 0, noDating = 1, scandal = 0, footprint = 10, cohesion = 20, roommate = 1, swapUsed = 0, showEp = 0, lastStars = 0, lives = 0;
    uint16_t day = 1; uint32_t fans = 0;
    uint8_t lineup[5] = {0, 1, 2, 3, 4};
    char group[20] = "";
    Trainee t[NT];
};
KState K;
const char* AGENCY_N[4] = {"Starlight Ent.", "Moonbeam Music", "Velvet Records", "Neon Pop Lab"};
const char* NAMES[] = {"Hana", "Jiwoo", "Mina", "Seol", "Yuna", "Riri", "Dahye", "Eunbi", "Kai", "Minho", "Taeyang", "Jun", "Soobin", "Haneul", "Nari", "Bora",
                       "Chae", "Lia", "Somi", "Woojin", "Areum", "Yeji", "Dami", "Sunny", "Rina", "Jisoo", "Hyun", "Leo", "Bella", "Coco", "Mimi", "Jade"};
const char* SK_N[5] = {"Vocal", "Dance", "Rap", "Visual", "Leader"};

enum Scr { SC_HUB, SC_ROSTER, SC_MINI, SC_RESULT, SC_SOCIAL, SC_LEADQ };
int scr = SC_HUB;
std::string resultTitle, resultText;
int rosterSel = 0;
// minigame
int mgType = 0; float mgT = 0, mgMeter = 0, mgScore = 0; int mgStep = 0, mgHits = 0, mgSeq[8], mgPhase = 0; bool mgDone = false; int mgPurpose = 0;   // purpose 0 training, 1 show, 2 live
int leadQ = 0;

int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
Trainee& me() { return K.t[0]; }
int stat(const Trainee& t) { int p = t.pos; return p >= PS_VOCAL && p <= PS_LEADER ? t.sk[p - 1] : (t.sk[0] + t.sk[1] + t.sk[2] + t.sk[3] + t.sk[4]) / 5; }
int avgSk(const Trainee& t) { return (t.sk[0] + t.sk[1] + t.sk[2] + t.sk[3] + t.sk[4]) / 5; }
bool posAllowed(const Trainee& t, int p, std::string* why) {
    switch (p) {
    case PS_VOCAL: if (t.sk[0] < 30) { if (why) *why = "Main Vocal needs Vocal 30+"; return false; } break;
    case PS_DANCE: if (t.sk[1] < 30) { if (why) *why = "Lead Dancer needs Dance 30+"; return false; } break;
    case PS_RAP: if (t.sk[2] < 25) { if (why) *why = "Main Rapper needs Rap 25+"; return false; } break;
    case PS_VISUAL: if (t.trait == TT_SHY && t.sk[3] < 50) { if (why) *why = "Too shy for Visual (needs Visual 50+)"; return false; } break;
    case PS_LEADER: if (!(t.trait == TT_CHARM || t.trait == TT_HARD || t.trait == TT_SWEET) || t.sk[4] < 25) { if (why) *why = "Leader needs a Charismatic/Hardworking/Sweet trait + Leader 25+"; return false; } break;
    }
    return true;
}
void makeTrainee(Trainee& t, const char* name, int agency, int seed) {
    memset(&t, 0, sizeof t); snprintf(t.name, sizeof t.name, "%s", name);
    for (int k = 0; k < 5; k++) t.sk[k] = (uint8_t)(15 + irand(30));
    t.sk[seed % 5] = (uint8_t)std::min(100, t.sk[seed % 5] + 20);
    t.trait = (uint8_t)irand(TT_COUNT); t.agency = (uint8_t)agency; t.status = TS_ACTIVE; t.fans = (uint16_t)(irand(200));
    t.pos = PS_NONE;
    for (int p = 1; p < PS_COUNT; p++) if (posAllowed(t, ((seed + p) % 5) + 1, nullptr)) { t.pos = (uint8_t)(((seed + p) % 5) + 1); break; }
}
int roomVibe() { return COMPAT[me().trait % TT_COUNT][K.t[K.roommate].trait % TT_COUNT]; }
const char* vibeName() { int v = roomVibe(); return v > 0 ? "Great" : v < 0 ? "Tense" : "OK"; }
void result(const std::string& t, const std::string& s) { resultTitle = t; resultText = s; scr = SC_RESULT; }
std::string fmt(const char* f, ...) { char b[400]; va_list a; va_start(a, f); vsnprintf(b, sizeof b, f, a); va_end(a); return b; }
int activeCount(bool agencyOnly) { int n = 0; for (int i = 0; i < NT; i++) if (K.t[i].status == TS_ACTIVE && (!agencyOnly || K.t[i].agency == 0)) n++; return n; }

void start() {
    K = KState{}; K.started = 1;
    std::string nm = G.sims.empty() ? "You" : MC().name.substr(0, MC().name.find(' '));
    makeTrainee(K.t[0], nm.c_str(), 0, 0);
    me().trait = (uint8_t)(G.sims.empty() ? TT_HARD : MC().hasTrait(TR_HOTHEAD) ? TT_HOTHEAD : MC().hasTrait(TR_DRAMATIC) ? TT_DIVA : MC().hasTrait(TR_INSECURE) ? TT_SHY : TT_HARD);
    me().pos = PS_NONE;
    int used[NT]; int nu = 0;
    for (int i = 1; i < NT; i++) {
        int n; bool dup; do { n = irand(32); dup = false; for (int k = 0; k < nu; k++) if (used[k] == n) dup = true; } while (dup);
        used[nu++] = n; makeTrainee(K.t[i], NAMES[n], i < 8 ? 0 : 1 + (i - 8) / 3, i);
    }
    K.roommate = 1 + irand(7);
}
void advanceDay();

// ---------------------------------------------------------------- evaluation, show, debut
void monthlyEval() {
    K.evals++;
    struct R { int i, s; } r[8]; int n = 0;
    for (int i = 0; i < 8; i++) if (K.t[i].status == TS_ACTIVE) r[n++] = {i, stat(K.t[i]) + avgSk(K.t[i]) / 2 + irand(20) + (i == 0 ? K.cohesion / 8 - K.scandal / 10 : 0)};
    std::sort(r, r + n, [](const R& a, const R& b) { return a.s > b.s; });
    std::string s = "Judges' ranking:\n";
    for (int k = 0; k < n; k++) s += fmt("%d. %s (%s) %d\n", k + 1, K.t[r[k].i].name, POS_N[K.t[r[k].i].pos], r[k].s);
    int myRank = 0; for (int k = 0; k < n; k++) if (r[k].i == 0) myRank = k + 1;
    for (int k = 0; k < std::min(2, n); k++) { Trainee& t = K.t[r[k].i]; for (int q = 0; q < 5; q++) t.sk[q] = (uint8_t)std::min(100, t.sk[q] + 4); t.fans += 150; }
    for (int k = n - 2; k < n && k >= 2; k++) {
        Trainee& t = K.t[r[k].i]; t.strikes++;
        if (t.strikes >= 3) { t.status = TS_OUT; if (r[k].i != 0) s += fmt("%s's contract was terminated.\n", t.name); }
    }
    if (myRank <= 2) s += "\nYOU placed top 2! Spot locked in, big stat boost.";
    else if (myRank >= n - 1) s += fmt("\nBottom of the ranking. Strike %d of 3...", me().strikes);
    else s += "\nSafe this month. Keep grinding.";
    if (me().status == TS_OUT) { K.phase = PH_FIRED; s += "\n\nCONTRACT TERMINATED. You've been released from Starlight Ent."; }
    if (K.evals == 2 && K.phase == PH_TRAINEE) s += "\n\nNews: you've been invited to the survival show PRODUCE STAR 101!";
    if (K.evals >= 4 && K.phase == PH_TRAINEE && myRank <= 5) {
        int k = 0; K.lineup[k++] = 0; for (int q = 0; q < n && k < 5; q++) if (r[q].i != 0) K.lineup[k++] = (uint8_t)r[q].i;
        K.phase = PH_DEBUT; snprintf(K.group, sizeof K.group, "STARLIGHT 5");
        for (int q = 0; q < 5; q++) K.t[K.lineup[q]].status = TS_DEBUT;
        s += "\n\nDEBUT! Starlight Ent. announces you in their new group STARLIGHT 5!";
    }
    sfxYay(); result("MONTHLY EVALUATION", s);
}
int showScore(int i) { const Trainee& t = K.t[i]; return stat(t) + avgSk(t) / 2 + t.fans / 40 + irand(30); }
void showEpisode(int perf) {
    K.showEp++;
    int keep = K.showEp == 1 ? 12 : K.showEp == 2 ? 9 : 7;
    struct R { int i, s; } r[NT]; int n = 0;
    for (int i = 0; i < NT; i++) if (K.t[i].status == TS_ACTIVE) r[n++] = {i, i == 0 ? perf + avgSk(me()) / 2 + (int)std::min<uint32_t>(60, K.fans / 60) - K.scandal / 5 + irand(15) : showScore(i)};
    std::sort(r, r + n, [](const R& a, const R& b) { return a.s > b.s; });
    int myRank = 0; for (int k = 0; k < n; k++) if (r[k].i == 0) myRank = k + 1;
    uint32_t votes = (uint32_t)(n - myRank + 1) * 9000 + (uint32_t)perf * 300;
    K.fans += votes / 20;
    std::string s = fmt("Episode %d. Your stage scored %d. Fan votes: %u.\nYou ranked #%d of %d.\n", K.showEp, perf, votes, myRank, n);
    for (int k = keep; k < n; k++) { if (r[k].i == 0) continue; K.t[r[k].i].status = TS_OUT; }
    if (myRank > keep) {
        s += "\nYou were eliminated... but your fans are LOUD. Back to your agency as a fan favorite.";
        K.phase = PH_TRAINEE; K.showEp = 0;
        for (int i = 8; i < NT; i++) K.t[i].status = TS_OUT;
        for (int i = 1; i < 8; i++) if (K.t[i].status == TS_OUT && K.t[i].strikes < 3) K.t[i].status = TS_ACTIVE;
    } else if (K.showEp >= 3) {
        int k = 0; K.lineup[k++] = 0; for (int q = 0; q < n && k < 5; q++) if (r[q].i != 0 && q < keep) K.lineup[k++] = (uint8_t)r[q].i;
        snprintf(K.group, sizeof K.group, "STARLIGHT X"); K.phase = PH_DEBUT;
        for (int q = 0; q < 5; q++) K.t[K.lineup[q]].status = TS_DEBUT;
        s += "\nFINAL! You made the top 7 and DEBUT in the project group STARLIGHT X!";
    } else s += fmt("\nYou survived! Top %d move on.", keep);
    sfxYay(); result("PRODUCE STAR 101", s);
}
void liveShow(int perf) {
    K.lives++;
    int sum = 0; for (int q = 0; q < 5; q++) sum += avgSk(K.t[K.lineup[q]]);
    int fb = sum / 5 / 2 + K.cohesion / 2 + perf / 3 - K.scandal / 4;
    int stars = clampi(fb / 18, 1, 5); K.lastStars = (uint8_t)stars;
    uint32_t gain = (uint32_t)stars * stars * 800; K.fans += gain;
    static const char* FB[6] = {"", "Off-sync and shaky. The comments are brutal.", "Some cute moments, but the chemistry isn't there yet.", "Solid stage! Fans are happy.", "Amazing stage - fancams are going viral!", "LEGENDARY. Trending #1 worldwide."};
    result("LIVE PERFORMANCE FEEDBACK", fmt("%s - comeback stage #%d\nFeedback: %d/5 stars\n%s\n+%u fans", K.group, K.lives, stars, FB[stars], gain));
}

// ---------------------------------------------------------------- daily actions
void scandalCheck(int risk, const char* what) {
    if (irand(100) < risk) {
        K.scandal = (uint8_t)std::min(100, K.scandal + 40); K.fans = K.fans * 85 / 100; me().strikes++;
        if (me().strikes >= 3 && K.phase != PH_DEBUT) K.phase = PH_FIRED;
        result("SCANDAL!", fmt("Dispatch caught you %s. The agency is FURIOUS: a strike on your contract and fans are leaving.%s", what, K.phase == PH_FIRED ? "\n\nCONTRACT TERMINATED." : ""));
        sfxNo();
    }
}
void train(int sk, int score) {
    const bool mine = me().pos == sk + 1;
    int g = score / 12 + (mine ? score / 12 : 0) + (me().trait == TT_HARD ? 1 : me().trait == TT_LAZY ? -1 : 0);
    me().sk[sk] = (uint8_t)std::min(100, me().sk[sk] + std::max(1, g));
    std::string s = fmt("%s +%d%s. Score %d/100.", SK_N[sk], std::max(1, g), mine ? " (your position: double gains!)" : "", score);
    advanceDay();
    if (scr != SC_RESULT) result("TRAINING DONE", s);
}
void advanceDay() {
    K.day++;
    if (K.scandal) K.scandal = (uint8_t)std::max(0, K.scandal - 3);
    for (int i = 1; i < NT; i++) if (K.t[i].status != TS_OUT) { int k = irand(5); K.t[i].sk[k] = (uint8_t)std::min(100, K.t[i].sk[k] + irand(3)); }
    if (roomVibe() < 0 && irand(4) == 0) { K.cohesion = (uint8_t)std::max(0, K.cohesion - 3); result("DORM DRAMA", fmt("%s (%s) and you fought over the bathroom schedule again. Team synergy took a hit.", K.t[K.roommate].name, TRAIT_N[K.t[K.roommate].trait])); }
    if (K.phase == PH_TRAINEE && K.day % 10 == 0) monthlyEval();
}
void startMini(int type, int purpose) {
    mgType = type; mgPurpose = purpose; mgT = 0; mgMeter = 0.3f; mgScore = 0; mgStep = 0; mgHits = 0; mgPhase = 0; mgDone = false;
    for (int i = 0; i < 8; i++) mgSeq[i] = irand(4);
    if (type == 4) { leadQ = irand(4); scr = SC_LEADQ; return; }
    scr = SC_MINI;
}
void finishMini(int score) {
    score = clampi(score, 0, 100);
    if (mgPurpose == 1) { showEpisode(score); return; }
    if (mgPurpose == 2) { liveShow(score); return; }
    train(mgType, score);
}

// leader scenarios: the best answer depends on who's involved
struct LeadQ { const char* q; const char* a[3]; int best; };
const LeadQ LQ[4] = {
    {"Two members are fighting over who gets the killing part. What do you do?", {"Let them battle it out in practice", "Split the part between them", "Give it to whoever's better, period"}, 1},
    {"The maknae is crying after the vocal coach yelled at them.", {"Take them for late-night tteokbokki", "Tell them to toughen up", "Report the coach"}, 0},
    {"The group is exhausted three days before evaluation.", {"Extra midnight practice for everyone", "A half day off, then focused practice", "Let everyone skip tomorrow"}, 1},
    {"A member was spotted on a date. Rumors are spreading.", {"Cover for them with the managers", "Tell the CEO immediately", "Talk to them privately first"}, 2},
};

// ---------------------------------------------------------------- layout
Btn hubB(int i) { return Btn{6 + (i % 2) * 156.f, 24 + (i / 2) * 36.f, 152, 32}; }
const Btn bBack{4, 208, 72, 28}, bEvent{82, 208, 234, 28};
Btn rowB(int i) { return Btn{6, 22 + i * 23.f, 308, 21}; }
Btn choiceB(int i) { return Btn{10, 96 + i * 38.f, 300, 34}; }
const Btn bOk{110, 204, 100, 30}, bTap{60, 120, 200, 70};
const char* HUB_N[10] = {"Vocal Lesson", "Dance Practice", "Rap Session", "Visual Shoot", "Leader Meeting", "Group Practice", "Rest Day", "Bubble (Social)", "Sneak Out", "Roster & Dorm"};
}

void kpopReset() { K = KState{}; scr = SC_HUB; }
void kpopOpen() { if (!K.started) start(); scr = SC_HUB; }

static const char* eventLabel() {
    if (K.phase == PH_FIRED) return "Re-audition (new contract)";
    if (K.phase == PH_DEBUT) return "Live Comeback Stage!";
    if (K.phase == PH_SHOW) { static char b[40]; snprintf(b, sizeof b, "PRODUCE STAR 101: Ep. %d", K.showEp + 1); return b; }
    if (K.evals >= 2 && K.showEp == 0) return "Join PRODUCE STAR 101";
    return nullptr;
}

bool kpopUpdate(const Input& in) {
    const float dt = in.dt;
    if (scr == SC_MINI) {
        mgT += dt;
        const bool A = (in.held & KEY_A) || ((in.held & KEY_TOUCH) && bTap.in(in.tp));
        const bool Ad = (in.down & KEY_A) || (in.touch && bTap.in(in.tp));
        int dir = (in.down & KEY_DUP) ? 0 : (in.down & KEY_DRIGHT) ? 1 : (in.down & KEY_DDOWN) ? 2 : (in.down & KEY_DLEFT) ? 3 : -1;
        switch (mgType) {
        case 0: {   // vocal: hold the note inside the moving green band
            mgMeter += (A ? 0.9f : -0.7f) * dt; mgMeter = std::max(0.f, std::min(1.f, mgMeter));
            float band = 0.5f + 0.3f * std::sin(mgT * 1.6f);
            if (std::fabs(mgMeter - band) < 0.12f) mgScore += dt;
            if (mgT >= 6.f) finishMini((int)(mgScore / 6.f * 115));
            break; }
        case 1:   // dance: memorize 6 arrows, then repeat them
            if (mgPhase == 0) { if (mgT > 2.4f) { mgPhase = 1; mgT = 0; } }
            else if (dir >= 0) { if (dir == mgSeq[mgStep]) { mgHits++; sfxTap(); } else sfxNo(); if (++mgStep >= 6) finishMini(mgHits * 100 / 6); }
            else if (mgT > 8.f) finishMini(mgHits * 100 / 6);
            break;
        case 2: {   // rap: hit A on each beat as it crosses the line
            int beat = (int)(mgT / 0.6f); float ph = std::fmod(mgT, 0.6f);
            if (Ad) { if ((ph < 0.12f || ph > 0.48f) && beat != mgStep) { mgHits++; mgStep = beat; sfxTap(); } else mgHits = std::max(0, mgHits - 1); }
            if (mgT >= 6.f) finishMini(mgHits * 10);
            break; }
        case 3:   // visual: hit the pose the photographer calls
            if (dir >= 0 && mgPhase == 0) { if (dir == mgSeq[mgStep]) { mgHits++; sfxTap(); } else sfxNo(); mgPhase = 1; }
            if (mgT > 1.3f) { mgT = 0; mgPhase = 0; if (++mgStep >= 6) finishMini(mgHits * 100 / 6); }
            break;
        }
        if (in.down & KEY_B) { scr = SC_HUB; }
        return true;
    }
    if (in.down & KEY_B) { if (scr == SC_HUB) return false; scr = SC_HUB; return true; }
    if (!in.touch) return true;
    if (scr == SC_RESULT) { if (bOk.in(in.tp)) { sfxTap(); scr = SC_HUB; } return true; }
    if (scr == SC_LEADQ) {
        for (int i = 0; i < 3; i++) if (choiceB(i).in(in.tp)) {
            int score = i == LQ[leadQ].best ? 100 : 40;
            if (i == LQ[leadQ].best) K.cohesion = (uint8_t)std::min(100, K.cohesion + 4);
            sfxTap(); finishMini(score); return true;
        }
        return true;
    }
    if (scr == SC_SOCIAL) {
        for (int i = 0; i < 4; i++) if (rowB(i + 1).in(in.tp)) {
            sfxTap();
            switch (i) {
            case 0: K.fans += 120 + irand(80); K.footprint = (uint8_t)std::min(100, K.footprint + 5); result("BUBBLE", "Cute selfie posted. Fans are screaming in the comments!"); break;
            case 1: K.fans += 300 + irand(200); K.footprint = (uint8_t)std::min(100, K.footprint + 15); result("BUBBLE", "A vague 2 AM post... fans are theorizing. So are the managers."); scandalCheck(K.footprint / 5, "posting cryptic messages at 2 AM"); break;
            case 2: if (irand(2)) { K.fans += 600; result("BUBBLE", "Your clap-back went VIRAL. Iconic."); } else scandalCheck(100, "fighting with fans online"); break;
            case 3: K.footprint = (uint8_t)std::max(0, K.footprint - 25); result("BUBBLE", "Old posts scrubbed. Your digital footprint is cleaner."); break;
            }
            return true;
        }
        if (bBack.in(in.tp)) scr = SC_HUB;
        return true;
    }
    if (scr == SC_ROSTER) {
        for (int i = 0; i < 8; i++) if (rowB(i).in(in.tp)) {
            Trainee& t = K.t[i];
            if (t.status != TS_ACTIVE && t.status != TS_DEBUT) { sfxNo(); return true; }
            int p = t.pos; std::string why;
            for (int k = 0; k < PS_COUNT; k++) { p = (p + 1) % PS_COUNT; if (p == PS_NONE || posAllowed(t, p, &why)) break; }
            t.pos = (uint8_t)p; sfxTap(); return true;
        }
        if (Btn{4, 208, 72, 28}.in(in.tp)) { scr = SC_HUB; return true; }
        if (Btn{82, 208, 112, 28}.in(in.tp)) { K.noDating ^= 1; sfxTap(); return true; }
        if (Btn{200, 208, 116, 28}.in(in.tp)) {
            if (K.swapUsed == K.evals + 1) { result("DORM", "You already swapped rooms this month."); return true; }
            int best = K.roommate; for (int i = 1; i < 8; i++) if (K.t[i].status == TS_ACTIVE && i != K.roommate && irand(2)) { best = i; break; }
            K.roommate = (uint8_t)best; K.swapUsed = (uint8_t)(K.evals + 1); sfxTap();
            result("ROOM SWAP", fmt("New roommate: %s (%s). Room vibe: %s.", K.t[best].name, TRAIT_N[K.t[best].trait], vibeName()));
            return true;
        }
        return true;
    }
    // hub
    if (bBack.in(in.tp)) return false;
    if (eventLabel() && bEvent.in(in.tp)) {
        sfxTap();
        if (K.phase == PH_FIRED) { start(); result("NEW CONTRACT", "You passed a new audition. Fresh start at Starlight Ent.!"); return true; }
        if (K.phase == PH_TRAINEE) { K.phase = PH_SHOW; K.showEp = 0; for (int i = 0; i < NT; i++) if (K.t[i].status != TS_OUT || i >= 8) K.t[i].status = TS_ACTIVE; }
        int p = me().pos; startMini(p >= PS_VOCAL && p <= PS_VISUAL ? p - 1 : 0, K.phase == PH_DEBUT ? 2 : 1);
        return true;
    }
    if (K.phase == PH_FIRED) { result("RELEASED", "Your contract was terminated. Re-audition to start over."); return true; }
    for (int i = 0; i < 10; i++) {
        if (!hubB(i).in(in.tp)) continue;
        sfxTap();
        switch (i) {
        case 0: case 1: case 2: case 3: case 4: startMini(i, 0); break;
        case 5: { int g = 3 + roomVibe() * 2 + (me().pos == PS_LEADER ? 2 : 0); K.cohesion = (uint8_t)clampi(K.cohesion + g, 0, 100); for (int k = 0; k < 5; k++) me().sk[k] = (uint8_t)std::min(100, me().sk[k] + 1);
                  advanceDay(); if (scr != SC_RESULT) result("GROUP PRACTICE", fmt("Everyone ran the choreo 40 times. The lineup feels a little more in sync. (Room vibe: %s)", vibeName())); break; }
        case 6: K.scandal = (uint8_t)std::max(0, K.scandal - 10); advanceDay(); if (scr != SC_RESULT) result("REST DAY", "Face mask, sheet pan dakgalbi and 14 hours of sleep. You feel human again."); break;
        case 7: scr = SC_SOCIAL; break;
        case 8:
            advanceDay(); if (scr == SC_RESULT) break;
            if (K.noDating) { result("SNEAK OUT", "You sneak out for a secret date by the Han river..."); scandalCheck(20 + K.footprint / 2, "on a secret date"); if (scr == SC_RESULT && resultTitle == "SNEAK OUT") resultText += " You made it back before curfew. Heart racing."; }
            else { K.fans = K.fans > 50 ? K.fans - 50 : 0; result("DATE NIGHT", "No-dating clause is off. A sweet date - a few fans grumble, most are happy for you."); }
            break;
        case 9: scr = SC_ROSTER; break;
        }
        return true;
    }
    return true;
}

// ---------------------------------------------------------------- drawing
static void bar(float x, float y, float w, int v, RGB c) { rrect(x, y, w, 7, 3, C(80, 60, 90, 120)); rrect(x, y, w * v / 100.f, 7, 3, C(c)); }
void kpopDrawTop(float t) {
    grad(0, 0, TOP_W, TOP_H, C(40, 20, 70), C(200, 80, 160));
    for (int i = 0; i < 6; i++) { float x = 30 + i * 70.f + std::sin(t + i) * 10; quad(x, 0, x + 16, 0, x + 60, 240, x - 30, 240, C(255, 230, 255, 18)); }
    for (int i = 0; i < 14; i++) sparkle(std::fmod(i * 53.f + t * 8, 400.f), 20 + (i * 37) % 120, 3, C(255, 255, 255, 120));
    if (scr == SC_MINI) {
        static const char* MG[4] = {"VOCAL: hold A in the green band!", "DANCE: memorize, then repeat with the D-Pad", "RAP: tap A on every beat", "VISUAL: hit the pose! (D-Pad)"};
        textOutline(MG[mgType], 200, 10, 0.5f, C(255, 255, 255), C(200, 40, 120), 1);
        if (mgType == 0) {
            float band = 0.5f + 0.3f * std::sin(mgT * 1.6f);
            rrect(180, 40, 40, 170, 8, C(30, 20, 40)); rrect(182, 40 + (1 - band - 0.12f) * 170, 36, 0.24f * 170, 6, C(90, 220, 140, 200));
            rrect(176, 40 + (1 - mgMeter) * 170 - 3, 48, 6, 3, C(255, 230, 80));
            text(fmt("%d%%", (int)(mgScore / 6 * 115)).c_str(), 260, 110, 0.6f, C(255, 255, 255), 1);
        } else if (mgType == 1) {
            static const char* AR[4] = {"UP", "RIGHT", "DOWN", "LEFT"};
            if (mgPhase == 0) for (int i = 0; i < 6; i++) { textOutline(AR[mgSeq[i]], 40 + i * 64.f, 100, 0.55f, C(255, 255, 255), C(120, 40, 200), 1); }
            else { text("Your turn!", 200, 90, 0.7f, C(255, 255, 255), 1); for (int i = 0; i < 6; i++) circle(80 + i * 48.f, 140, 10, C(i < mgStep ? RGB{255, 220, 80} : RGB{90, 60, 110})); }
        } else if (mgType == 2) {
            float ph = std::fmod(mgT, 0.6f) / 0.6f;
            line(200, 60, 200, 200, 3, C(255, 255, 255));
            for (int k = -1; k <= 4; k++) { float x = 200 + (k - ph) * 80.f; circle(x, 130, 12, C(255, 80, 160)); }
            text(fmt("Hits: %d", mgHits).c_str(), 200, 200, 0.55f, C(255, 255, 255), 1);
        } else {
            static const char* PO[4] = {"HEART (Up)", "WINK (Right)", "PEACE (Down)", "SMOLDER (Left)"};
            textOutline(PO[mgSeq[std::min(mgStep, 7)]], 200, 100, 0.8f, C(255, 240, 120), C(200, 40, 120), 1);
            text(fmt("Shot %d/6   Hits %d", std::min(mgStep + 1, 6), mgHits).c_str(), 200, 160, 0.5f, C(255, 255, 255), 1);
            if (mgPhase) rect(0, 0, TOP_W, TOP_H, C(255, 255, 255, 60));
        }
        return;
    }
    textOutline(K.phase == PH_DEBUT ? K.group : "STARLIGHT ENT. TRAINEE", 200, 4, 0.6f, C(255, 255, 255), C(255, 70, 150), 1);
    if (!G.sims.empty()) { circle(48, 62, 34, C(255, 255, 255)); drawSimPortrait(MC(), 48, 62, 32, t); }
    const Trainee& m = me();
    text(fmt("%s  -  %s", m.name, POS_N[m.pos]).c_str(), 92, 30, 0.5f, C(255, 255, 255));
    text(fmt("Trait: %s   Strikes: %d/3", TRAIT_N[m.trait], m.strikes).c_str(), 92, 48, 0.4f, C(255, 220, 240));
    static const RGB SC[5] = {{255, 120, 180}, {120, 200, 255}, {255, 200, 80}, {200, 140, 255}, {120, 230, 160}};
    for (int k = 0; k < 5; k++) { text(SK_N[k], 92, 66 + k * 14.f, 0.36f, C(255, 255, 255)); bar(140, 69 + k * 14.f, 120, m.sk[k], SC[k]); text(fmt("%d", m.sk[k]).c_str(), 266, 66 + k * 14.f, 0.36f, C(255, 255, 255)); }
    rrect(8, 140, 384, 94, 10, C(255, 255, 255, 40));
    const char* ph = K.phase == PH_FIRED ? "Contract terminated" : K.phase == PH_DEBUT ? "Debuted idol" : K.phase == PH_SHOW ? "On PRODUCE STAR 101" : "Trainee";
    text(fmt("Day %d   Status: %s", K.day, ph).c_str(), 18, 146, 0.42f, C(255, 255, 255));
    if (K.phase == PH_TRAINEE) text(fmt("Monthly Evaluation in %d day%s", 10 - K.day % 10, 10 - K.day % 10 == 1 ? "" : "s").c_str(), 18, 164, 0.42f, C(255, 240, 140));
    else if (K.phase == PH_DEBUT && K.lives) text(fmt("Last stage: %d/5 stars", K.lastStars).c_str(), 18, 164, 0.42f, C(255, 240, 140));
    text(fmt("Fans: %u", K.fans).c_str(), 18, 182, 0.42f, C(255, 255, 255));
    text(fmt("Roommate: %s (%s)  Vibe: %s", K.t[K.roommate].name, TRAIT_N[K.t[K.roommate].trait], vibeName()).c_str(), 18, 200, 0.38f, C(255, 255, 255));
    text(fmt("No-Dating Clause: %s   Scandal: %d%%", K.noDating ? "ON" : "OFF", K.scandal).c_str(), 18, 216, 0.38f, C(K.scandal > 40 ? RGB{255, 120, 120} : RGB{255, 255, 255}));
}
void kpopDrawBottom(float t) {
    grad(0, 0, BOT_W, BOT_H, C(255, 236, 248), C(240, 220, 255));
    (void)t;
    if (scr == SC_MINI) {
        text("Press B to quit", 160, 6, 0.4f, C(UI_INK), 1);
        if (mgType == 0 || mgType == 2) button(bTap, mgType == 0 ? "HOLD (or A)" : "TAP (or A)", false, 0, 0.6f);
        else text("Use the D-Pad!", 160, 140, 0.6f, C(UI_INK), 1);
        return;
    }
    if (scr == SC_RESULT || scr == SC_LEADQ) {
        const std::string& title = scr == SC_LEADQ ? std::string("LEADER MEETING") : resultTitle;
        textOutline(title.c_str(), 160, 4, 0.5f, C(255, 255, 255), C(200, 40, 120), 1);
        std::string body = scr == SC_LEADQ ? std::string(LQ[leadQ].q) : resultText;
        float y = 26; const float maxY = scr == SC_LEADQ ? 92 : 198;
        for (auto& para : wrapText(body, 0.38f, 300)) { if (y > maxY - 12) break; text(para.c_str(), 10, y, 0.38f, C(UI_INK)); y += 13; }
        if (scr == SC_LEADQ) for (int i = 0; i < 3; i++) button(choiceB(i), LQ[leadQ].a[i], false, i, 0.38f);
        else button(bOk, "OK", false, 2, 0.45f);
        return;
    }
    if (scr == SC_SOCIAL) {
        text(fmt("Bubble - digital footprint: %d%%", K.footprint).c_str(), 160, 4, 0.42f, C(UI_INK), 1);
        static const char* SO[4] = {"Post a cute selfie", "Vague 2 AM post", "Reply to a hater", "Scrub old posts"};
        for (int i = 0; i < 4; i++) button(rowB(i + 1), SO[i], false, i == 3 ? 2 : 0, 0.42f);
        button(bBack, "Back", false, 1, 0.42f);
        return;
    }
    if (scr == SC_ROSTER) {
        text("Tap a trainee to change their position", 160, 4, 0.38f, C(UI_INK), 1);
        for (int i = 0; i < 8; i++) {
            const Trainee& x = K.t[i];
            std::string l = x.status == TS_OUT ? fmt("%s - (left the agency)", x.name) : fmt("%s%s: %s  [%s]%s", x.name, i == 0 ? " (you)" : "", POS_N[x.pos], TRAIT_N[x.trait], i == K.roommate ? " roomie" : "");
            button(rowB(i), l.c_str(), i == 0, x.status == TS_OUT ? 1 : 0, 0.36f);
        }
        button(Btn{4, 208, 72, 28}, "Back", false, 1, 0.4f);
        button(Btn{82, 208, 112, 28}, K.noDating ? "No-Dating: ON" : "No-Dating: OFF", K.noDating, 3, 0.36f);
        button(Btn{200, 208, 116, 28}, "Room Swap", false, 2, 0.38f);
        return;
    }
    text(K.phase == PH_FIRED ? "Released from the agency" : "Pick today's schedule", 160, 4, 0.42f, C(UI_INK), 1);
    for (int i = 0; i < 10; i++) {
        bool mine = i < 5 && me().pos == i + 1;
        button(hubB(i), mine ? (std::string(HUB_N[i]) + " *").c_str() : HUB_N[i], mine, i < 5 ? 0 : i < 7 ? 2 : 3, 0.4f);
    }
    button(bBack, "Back", false, 1, 0.42f);
    if (const char* e = eventLabel()) button(bEvent, e, true, 2, 0.4f);
}

void kpopSaveTo(const std::string& path) {
    if (!K.started) { remove(path.c_str()); return; }
    FILE* f = fopen(path.c_str(), "wb"); if (!f) return;
    fwrite(&K, sizeof K, 1, f); fclose(f);
}
void kpopLoadFrom(const std::string& path) {
    K = KState{};
    FILE* f = fopen(path.c_str(), "rb"); if (!f) return;
    KState k; size_t n = fread(&k, sizeof k, 1, f); fclose(f);
    if (n != 1 || k.magic != 0x4B504F31) return;
    k.roommate = (uint8_t)clampi(k.roommate, 1, 7); k.phase %= 4;
    for (auto& x : k.t) { x.name[15] = 0; x.pos %= PS_COUNT; x.trait %= TT_COUNT; x.status %= 3; }
    for (auto& l : k.lineup) l %= NT;
    k.group[19] = 0;
    K = k;
}
