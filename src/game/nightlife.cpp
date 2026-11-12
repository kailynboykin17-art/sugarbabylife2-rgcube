#include "lifeext.hpp"
#include "nightlife.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "district.hpp"
#include "lockup.hpp"
#include "challenge.hpp"
#include "lifestory.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include "vfs.hpp"
#define fopen vfopen   // sidecar saves go into the one-file save pack

using namespace col;
static void ol(float x, float y, float w, float h, RGB c, float th) { rect(x - th, y - th, w + 2 * th, h + 2 * th, C(c)); }

// ------------------------------------------------------------------ the made-up substances
struct Sub { const char* name; const char* peak; Emotion peakEm; const char* down; Emotion downEm; int price, hook, kick; };
static const int NSUB = 6;
static const Sub SUBS[NSUB] = {
    {"Giggle Gummies", "Giggly & Glowing", EM_PLAYFUL, "Gummy Slump", EM_SAD, 20, 6, 1},
    {"Starfizz Drops", "Starfizz Sparkle", EM_CONFIDENT, "Fizzled Out", EM_TENSE, 30, 9, 2},
    {"Moonmilk", "Moonmilk Melt", EM_HAPPY, "Heavy Moon", EM_BORED, 25, 7, 1},
    {"Velvet Haze", "Velvet Vision", EM_INSPIRED, "Hazy & Hungry", EM_BORED, 25, 8, 1},
    {"Zoom Pops", "ZOOM!", EM_ENERGIZED, "Zoom Crash", EM_TENSE, 40, 14, 3},
    {"Cloud Nine Mints", "On Cloud Nine", EM_FLIRTY, "Cloudy Comedown", EM_SAD, 35, 10, 2},
};
static const char* CREW = "Neon Kings";

struct NState {
    uint32_t magic = 0x4E495431;
    uint8_t enabled = 0;               // After Hours content switch (adults only, off by default)
    int8_t active = -1, activeSim = -1, doses = 0;
    uint16_t mins = 0;                 // minutes since the dose
    uint8_t phase = 0;                 // 0 come-up, 1 peak, 2 comedown, 3 done
    uint8_t usedToday[NSUB] = {}, usedYesterday = 0;
    uint8_t addict[64][NSUB] = {};     // per sim, 0..100
    uint8_t stash[NSUB] = {};
    uint8_t permission = 0, clients = 0, heat = 0, crewRank = 0, rehabDays = 0, pad = 0;
    uint16_t dealXP = 0;
    int32_t dealt = 0;
};
static NState N;

static int lvl() { return std::min(10, N.dealXP / 100); }
static uint8_t* addictOf(int s, int k) { return &N.addict[std::max(0, std::min(63, s))][k]; }
static bool pay(int amt) { if (G.money < amt) { toast("Not enough money!"); sfxNo(); return false; } G.money -= amt; return true; }
static int topAddiction(int s, int* which = nullptr) { int b = 0, w = 0; for (int k = 0; k < NSUB; k++) if (*addictOf(s, k) > b) { b = *addictOf(s, k); w = k; } if (which) *which = w; return b; }

std::vector<std::pair<std::string, int>> nightSpotOptions(int kind) {
    std::vector<std::pair<std::string, int>> v;
    const Sim& me = MC(); const bool adult = me.stage >= ST_YA; char b[64];
    switch (kind) {
    case SP_LANES: break;
    case SP_BOWLSNACK: v.push_back({"Cosmic Nachos ($8)", DA_BOWLSNACK}); break;
    case SP_KMIC: break;
    case SP_KBOOTH: v.push_back({"Rent a Private Booth ($20)", DA_KBOOTH}); break;
    case SP_EASEL: v.push_back({"Paint a Canvas ($15)", DA_PAINT}); break;
    case SP_SIPBAR: v.push_back({"Sparkling Lemonade ($6)", DA_SIP}); break;
    case SP_DFLOOR: v.push_back({"Take a Dance Class ($15)", DA_DANCECLASS}); break;
    case SP_BARRE: v.push_back({"Stretch at the Barre", DA_STRETCH}); break;
    case SP_NFOOD: v.push_back({"Spicy Skewers ($7)", DA_SKEWERS}); break;
    case SP_REHABDESK:
        v.push_back({N.rehabDays ? "Rehab: Today's Program" : "Check Into Rehab ($300, 3 days)", DA_REHAB});
        v.push_back({"Detox Tea & Check-up ($20)", DA_DETOX}); break;
    case SP_CIRCLE: v.push_back({"Join the Support Circle", DA_SUPPORT}); break;
    case SP_VENDOR:
        if (!adult) { v.push_back({"Buy a Glow Bracelet ($3)", DA_SKEWERS}); break; }   // kids & teens never see the back table
        v.push_back({N.enabled ? "After Hours content: ON (turn off)" : "Turn On After Hours (18+, fictional)", DA_NTOGGLE});
        if (!N.enabled) break;
        for (int k = 0; k < NSUB; k++) { snprintf(b, sizeof b, "Buy %s ($%d)", SUBS[k].name, SUBS[k].price); v.push_back({b, DA_NBUY0 + k}); }
        for (int k = 0; k < NSUB; k++) if (N.stash[k]) { snprintf(b, sizeof b, "Take %s (have %d)", SUBS[k].name, N.stash[k]); v.push_back({b, DA_NUSE0 + k}); }
        if (!N.permission) v.push_back({"Ask Vex for Work", DA_ASKWORK});
        else {
            v.push_back({"Restock 5 to Sell (wholesale)", DA_RESTOCK});
            snprintf(b, sizeof b, "Deal to Clients (Lv %d)", lvl()); v.push_back({b, DA_DEAL});
            v.push_back({"Check Client List", DA_CLIENTS});
            v.push_back({"Lay Low (lower heat)", DA_LAYLOW});
            if (!N.crewRank) v.push_back({"Join the Neon Kings", DA_CREW}); else v.push_back({"Run a Crew Job", DA_CREWJOB});
        }
        break;
    }
    return v;
}

static void startHigh(int k) {
    Sim& me = MC();
    if (N.active >= 0 && N.activeSim == G.mc) {
        N.doses++;
        const int odChance = 20 * (N.doses - 1) + SUBS[k].kick * 5;
        if (irand(100) < odChance) {
            const bool lethal = challengeDanger(2) && challengeDanger(0) >= 2 && irand(100) < 35;
            N.active = -1; N.phase = 3;
            if (lethal) { toast("You mixed too much. The night went dark for good..."); lifeKill(G.mc, DK_ILLNESS); return; }
            G.money = G.money > 500 ? G.money - 500 : 0; me.needs[N_ENERGY] = 0; me.needs[N_FUN] = 0;
            addMoodlet(me, "Overdid It (ER Visit)", EM_UNCOMFORTABLE, 3, 1440);
            toast("You overdid it and woke up in the ER. -$500. (Deaths only happen with Tragedy + Permanent Death on.)"); sfxNo();
            return;
        }
    } else N.doses = 1;
    N.active = (int8_t)k; N.activeSim = (int8_t)G.mc; N.mins = 0; N.phase = 0;
    N.usedToday[k] = 1;
    uint8_t* a = addictOf(G.mc, k); *a = (uint8_t)std::min(100, *a + SUBS[k].hook + (*a > 50 ? 4 : 0));
    char b[64]; snprintf(b, sizeof b, "Coming Up: %s", SUBS[k].name);
    addMoodlet(me, b, EM_ENERGIZED, 1, 60);
    toast(std::string("You took ") + SUBS[k].name + ". Here it comes...");
}

static void dealOnce() {
    int k = -1; for (int i = 0; i < NSUB; i++) if (N.stash[i]) { k = i; break; }
    if (k < 0) { toast("Nothing to sell. Restock first."); sfxNo(); return; }
    if (N.clients == 0) N.clients = 2;
    N.stash[k]--;
    const int price = SUBS[k].price * (150 + lvl() * 25 + N.crewRank * 20) / 100;
    G.money += price; N.dealt += price;
    N.dealXP = (uint16_t)std::min(1100, N.dealXP + 18 + irand(10));
    if (irand(100) < 35 && N.clients < 40) N.clients++;
    N.heat = (uint8_t)std::min(100, N.heat + std::max(2, 9 - lvl() / 2));
    lxRepAdd(CI_PARTY, 1, "known supplier"); lxRepAdd(CI_FAMILY, -2, "dealing"); if (irand(100) < 10) lxGossip(G.sims[G.mc].name.substr(0, G.sims[G.mc].name.find(' ')) + " is selling stuff at the Night Market", irand(2), CI_FAMILY, -1);
    char b[96]; snprintf(b, sizeof b, "Sold %s for $%d. Clients: %d  Heat: %d%%", SUBS[k].name, price, N.clients, N.heat); toast(b);
    if (irand(100) < N.heat / 3) {
        memset(N.stash, 0, sizeof N.stash); N.heat = 20;
        G.rapSheet++; jailSentence(2 + N.heat / 40 + (N.crewRank ? 1 : 0), "dealing");
    }
}

bool nightAction(int act) {
    Sim& me = MC(); Activity a;
    if (act >= DA_NBUY0 && act <= DA_NBUY5) {
        const int k = act - DA_NBUY0;
        if (!N.enabled || me.stage < ST_YA) return true;
        if (!pay(SUBS[k].price)) return true;
        N.stash[k] = (uint8_t)std::min(99, N.stash[k] + 1); toast(std::string("Got ") + SUBS[k].name + ". Vex winks."); return true;
    }
    if (act >= DA_NUSE0 && act <= DA_NUSE5) {
        const int k = act - DA_NUSE0;
        if (!N.enabled || me.stage < ST_YA || !N.stash[k]) return true;
        if (N.rehabDays) { toast("You're in rehab. You put it back."); return true; }
        N.stash[k]--; startHigh(k); return true;
    }
    switch (act) {
    case DA_BOWLSNACK: if (!pay(8)) return true; me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + 30); addMoodlet(me, "Cosmic Nachos", EM_HAPPY, 1, 120); toast("Glow-in-the-dark nachos. Iconic."); return true;
    case DA_KBOOTH: if (!pay(20)) return true; a = makeAct("Private Karaoke Booth", 90, 8); a.rate[N_FUN] = 14; a.rate[N_SOCIAL] = 10; a.skill = SK_CHARISMA; a.skillRate = 6; startActivity(a); lxSkill(XS_SING, 15); return true;
    case DA_PAINT: if (!pay(15)) return true; a = makeAct("Painting a Canvas", 120, 8); a.rate[N_FUN] = 8; a.skill = SK_CREATIVITY; a.skillRate = 14; startActivity(a); lxRepAdd(CI_ARTISTS, 2, "paints a lot"); lxSkill(XS_PAINT, 15); return true;
    case DA_SIP: if (!pay(6)) return true; me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 10); addMoodlet(me, "Sip & Chill", EM_HAPPY, 1, 90); return true;
    case DA_DANCECLASS: if (!pay(15)) return true; a = makeAct("Dance Class", 60, 8); a.rate[N_ENERGY] = -10; a.rate[N_FUN] = 10; a.skill = SK_FITNESS; a.skillRate = 14; startActivity(a); lxRepAdd(CI_JOCKS, 2, "dance classes"); lxSkill(XS_DANCE, 15); return true;
    case DA_STRETCH: a = makeAct("Stretching at the Barre", 30, 8); a.rate[N_ENERGY] = -2; a.skill = SK_FITNESS; a.skillRate = 6; startActivity(a); return true;
    case DA_SKEWERS: if (!pay(7)) return true; me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + 35); toast("Hot off the grill!"); return true;
    case DA_REHAB: {
        if (!N.rehabDays) { if (!pay(300)) return true; N.rehabDays = 3; toast("You checked into Serenity. 3 days of rehab - come to the desk each day."); }
        a = makeAct("Rehab Program", 240, 8); a.rate[N_FUN] = -4; a.rate[N_SOCIAL] = 4; startActivity(a);
        for (int k = 0; k < NSUB; k++) { uint8_t* x = addictOf(G.mc, k); *x = (uint8_t)std::max(0, *x - 15); }
        addMoodlet(me, "Doing the Work", EM_FOCUSED, 2, 480); return true;
    }
    case DA_DETOX: if (!pay(20)) return true; for (int k = 0; k < NSUB; k++) { uint8_t* x = addictOf(G.mc, k); *x = (uint8_t)std::max(0, *x - 5); } addMoodlet(me, "Detox Tea", EM_FINE, 1, 240); toast("Clearer already."); return true;
    case DA_SUPPORT: a = makeAct("Support Circle", 60, 8); a.rate[N_SOCIAL] = 12; startActivity(a);
        for (int k = 0; k < NSUB; k++) { uint8_t* x = addictOf(G.mc, k); *x = (uint8_t)std::max(0, *x - 8); }
        addMoodlet(me, topAddiction(G.mc) ? "One Day at a Time" : "Being a Good Listener", EM_HAPPY, 1, 360); return true;
    case DA_NTOGGLE:
        if (me.stage < ST_YA) return true;
        N.enabled = !N.enabled; toast(N.enabled ? "After Hours ON: fictional party substances, addiction, rehab & dealing (adults only)." : "After Hours OFF."); return true;
    case DA_ASKWORK:
        if (me.skills[SK_CHARISMA] >= 2 || G.money >= 200) {
            if (me.skills[SK_CHARISMA] < 2) G.money -= 200;
            N.permission = 1; N.clients = 2; toast("Vex: \"Fine. Two regulars. Don't get sloppy.\" (Dealing unlocked)");
        } else toast("Vex: \"Come back with more charm - or $200.\"");
        return true;
    case DA_RESTOCK: { const int k = irand(NSUB); if (!pay(SUBS[k].price * 5 * 6 / 10)) return true; N.stash[k] = (uint8_t)std::min(99, N.stash[k] + 5); toast(std::string("Restocked 5 ") + SUBS[k].name + "."); return true; }
    case DA_DEAL: dealOnce(); return true;
    case DA_CLIENTS: { char b[128]; snprintf(b, sizeof b, "Dealing Lv %d (%d xp). %d clients. Heat %d%%. Earned $%ld. Crew: %s", lvl(), N.dealXP, N.clients, N.heat, (long)N.dealt, N.crewRank ? CREW : "none"); toast(b); return true; }
    case DA_LAYLOW: a = makeAct("Laying Low", 120, 8); a.rate[N_FUN] = -4; startActivity(a); N.heat = (uint8_t)std::max(0, N.heat - 15); toast("You keep your head down."); return true;
    case DA_CREW:
        if (lvl() < 3) { toast("The Neon Kings want Dealing Lv 3 first."); return true; }
        N.crewRank = 1; toast("You're in the Neon Kings now. Better prices - and more heat."); return true;
    case DA_CREWJOB:
        N.heat = (uint8_t)std::min(100, N.heat + 12); G.money += 120 * N.crewRank;
        if (irand(100) < 25 && N.crewRank < 5) { N.crewRank++; toast("Crew job done. You ranked up in the Neon Kings!"); }
        else toast("Crew job done. +$" + std::to_string(120 * N.crewRank));
        if (irand(100) < N.heat / 4) { N.heat = 30; G.rapSheet++; jailSentence(3, "crew job"); }
        return true;
    }
    return false;
}

void nightHourly(int hour) {
    (void)hour;
    if (N.active < 0 || N.activeSim < 0 || N.activeSim >= (int)G.sims.size()) return;
    Sim& s = G.sims[N.activeSim]; const Sub& d = SUBS[N.active];
    N.mins += 60;
    if (N.phase == 0 && N.mins >= 60) { N.phase = 1; addMoodlet(s, d.peak, d.peakEm, 2 + (N.doses > 1), 180); s.needs[N_FUN] = std::min(100.f, s.needs[N_FUN] + 30);
        if (N.active == 3) skillGain(s, SK_CREATIVITY, 0.4f); if (N.active == 5) s.needs[N_SOCIAL] = std::min(100.f, s.needs[N_SOCIAL] + 20); }
    else if (N.phase == 1 && N.mins >= 240) { N.phase = 2; addMoodlet(s, d.down, d.downEm, 1 + d.kick / 2, 180); s.needs[N_ENERGY] = std::max(0.f, s.needs[N_ENERGY] - 15 * d.kick); if (N.active == 3) s.needs[N_HUNGER] = std::max(0.f, s.needs[N_HUNGER] - 30); }
    else if (N.phase == 2 && N.mins >= 420) { N.phase = 3; N.active = -1; N.doses = 0; }
}

void nightDaily() {
    bool any = false; for (int k = 0; k < NSUB; k++) any |= N.usedToday[k] != 0;
    if (any && G.mc < (int)G.sims.size()) addMoodlet(MC(), "Hangover", EM_UNCOMFORTABLE, 1 + (N.doses > 1), 600);
    for (size_t i = 0; i < G.sims.size() && i < 64; i++) for (int k = 0; k < NSUB; k++) {
        uint8_t* x = addictOf((int)i, k);
        const bool used = (int)i == G.mc && N.usedToday[k];
        if (*x >= 40 && !used && !simDead((int)i)) {
            Sim& s = G.sims[i]; char b[64];
            snprintf(b, sizeof b, *x >= 70 ? "Hooked: Craving %s" : "Craving %s", SUBS[k].name);
            addMoodlet(s, b, EM_TENSE, *x >= 70 ? 3 : 2, 1440); s.needs[N_FUN] = std::max(0.f, s.needs[N_FUN] - (*x >= 70 ? 30 : 15));
            if ((int)i == G.mc && *x >= 70 && irand(100) < 30) toast("Your withdrawal is rough. The Serenity Center on the Neon Strip can help.");
        }
        if (!used) *x = (uint8_t)std::max(0, *x - (N.rehabDays ? 12 : 3));
    }
    memset(N.usedToday, 0, sizeof N.usedToday);
    if (N.rehabDays) { N.rehabDays--; if (!N.rehabDays) { addMoodlet(MC(), "Rehab Graduate", EM_CONFIDENT, 2, 2880); toast("You finished rehab. Proud of you."); } }
    N.heat = (uint8_t)std::max(0, N.heat - 5);
    if (N.permission && N.clients > 0 && irand(100) < 20) N.clients--;   // clients drift away if ignored
}

void nightSaveTo(const std::string& path) {
    FILE* f = fopen(path.c_str(), "wb"); if (!f) return;
    fwrite(&N, sizeof N, 1, f); fclose(f);
}
void nightLoadFrom(const std::string& path) {
    N = NState{};
    FILE* f = fopen(path.c_str(), "rb"); if (!f) return;
    NState n; size_t r = fread(&n, sizeof n, 1, f); fclose(f);
    if (r != 1 || n.magic != 0x4E495431) return;
    if (n.active >= NSUB) n.active = -1;
    n.phase %= 4; n.enabled &= 1; n.heat = std::min<uint8_t>(n.heat, 100); n.crewRank = std::min<uint8_t>(n.crewRank, 5);
    for (auto& row : n.addict) for (auto& x : row) x = std::min<uint8_t>(x, 100);
    N = n;
}

// ------------------------------------------------------------------ backdrops
void nightDrawRoom(int room, float t, bool night) {
    switch (room) {
    case R_GLOWBOWL:
        grad(0, 0, TOP_W, 158, C(30, 16, 60), C(60, 20, 90));
        for (int k = 0; k < 30; k++) circle((k * 53) % 400, (k * 31) % 140 + 6, 1.5f, C(120, 255, 240, 160));
        for (int k = 0; k < 4; k++) { const float x = 30 + k * 90.f; rect(x, 100, 70, 58, C(240, 200, 120)); rect(x + 4, 100, 2, 58, C(255, 80, 200)); rect(x + 64, 100, 2, 58, C(80, 220, 255)); for (int p = 0; p < 3; p++) circle(x + 22 + p * 13, 108, 4, C(255, 255, 255)); }
        textOutline("GLOW BOWL", 200, 20, 0.7f, C(120, 255, 240), C(255, 46, 140), 1);
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(70, 40, 110));
        break;
    case R_KARAOKE:
        grad(0, 0, TOP_W, 158, C(80, 10, 50), C(140, 30, 80));
        rect(70, 60, 130, 70, C(40, 10, 30)); for (int k = 0; k < 8; k++) circle(80 + k * 16.f, 56, 4, C(255, 220, 120, 200 + (int)(50 * ((k + (int)(t * 4)) % 2))));
        ol(260, 40, 110, 60, RGB{255, 200, 230}, 3); rect(263, 43, 104, 54, C(30, 20, 50)); text("~ la la la ~", 315, 62, 0.4f, C(255, 255, 255), 1);
        textOutline("KARAOKE LOUNGE", 200, 16, 0.6f, C(255, 220, 240), C(UI_INK), 1);
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(90, 20, 60));
        break;
    case R_PAINTSIP:
        grad(0, 0, TOP_W, 158, C(250, 240, 228), C(240, 226, 210));
        static const RGB PC[5] = {{255, 120, 150}, {120, 180, 240}, {250, 200, 90}, {140, 210, 150}, {190, 140, 230}};
        for (int k = 0; k < 5; k++) { const float x = 24 + k * 74.f; ol(x, 40, 50, 60, RGB{150, 110, 70}, 2); rect(x + 2, 42, 46, 56, C(255, 255, 255)); circle(x + 25, 70, 14, C(PC[k])); }
        textOutline("PAINT & SIP", 200, 14, 0.6f, C(255, 255, 255), C(200, 90, 120), 1);
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(200, 170, 140));
        break;
    case R_DANCESTUDIO:
        grad(0, 0, TOP_W, 158, C(230, 220, 240), C(210, 200, 230));
        rect(20, 20, 360, 110, C(200, 220, 240)); for (int k = 0; k < 6; k++) rect(20 + k * 60.f, 20, 2, 110, C(240, 240, 250));   // mirror wall
        rect(300, 92, 90, 4, C(160, 120, 80));
        textOutline("STEP UP STUDIO", 200, 6, 0.5f, C(255, 255, 255), C(150, 96, 230), 1);
        rect(0, 156, TOP_W, 2, C(UI_INK)); for (int k = 0; k < 10; k++) rect(k * 40.f, 158, 40, 82, C(k % 2 ? RGB{200, 160, 110} : RGB{214, 176, 126}));
        break;
    case R_NIGHTMKT:
        grad(0, 0, TOP_W, 158, C(16, 18, 40), C(40, 30, 70));
        for (int k = 0; k < 14; k++) circle(10 + k * 30.f, 30 + (k % 2) * 6, 5, C(k % 3 == 0 ? RGB{255, 120, 80} : k % 3 == 1 ? RGB{255, 220, 120} : RGB{255, 90, 160}));
        rect(0, 30, TOP_W, 1, C(80, 70, 90));
        rect(50, 90, 120, 66, C(90, 40, 60)); rect(46, 84, 128, 10, C(255, 46, 140));
        rect(240, 90, 120, 66, C(60, 70, 50)); rect(236, 84, 128, 10, C(255, 180, 60));
        textOutline("NIGHT MARKET", 200, 50, 0.6f, C(255, 220, 120), C(UI_INK), 1);
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(50, 50, 60));
        break;
    case R_SERENITY:
        grad(0, 0, TOP_W, 158, C(220, 238, 232), C(200, 226, 220));
        for (int k = 0; k < 3; k++) { circle(60 + k * 140.f, 120, 18, C(110, 170, 120)); rect(56 + k * 140.f, 130, 8, 26, C(150, 110, 80)); }
        ol(150, 40, 100, 50, RGB{120, 160, 150}, 2); text("one day", 200, 52, 0.4f, C(80, 110, 100), 1); text("at a time", 200, 68, 0.4f, C(80, 110, 100), 1);
        textOutline("SERENITY CENTER", 200, 14, 0.55f, C(255, 255, 255), C(80, 140, 120), 1);
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(190, 210, 200));
        break;
    }
    if (night) rect(0, 0, TOP_W, TOP_H, C(10, 10, 30, room == R_GLOWBOWL || room == R_NIGHTMKT || room == R_KARAOKE ? 40 : 90));
}
bool nlAfterHours() { return N.enabled != 0; }
void nlSetAfterHours(bool on) { N.enabled = on ? 1 : 0; }
