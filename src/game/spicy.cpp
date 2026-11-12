#include <cmath>
#include "lifeext.hpp"
#include "spicy.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include "lifestory.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include "vfs.hpp"
#define fopen vfopen   // sidecar saves go into the one-file save pack

using namespace col;

const char* SPICY_LEVEL_NAMES[SPL_COUNT] = {"OFF", "Fade to Black", "Censored", "Uncensored"};
static const char* LEVEL_DESC[SPL_COUNT] = {
    "No private scenes or spicy invites. Romance, dating & family still work.",
    "Doors close, the screen fades, time passes. Same relationship results.",
    "A short stylized scene of silhouettes under a solid pixel mosaic.",
    "Same scene without the mosaic overlay. Adult characters only."};

struct SState {
    uint32_t magic = 0x53504331;
    uint8_t level = SPL_OFF, pendingLevel = 0xFF;
    uint8_t dancer = 0, pad = 0;
    uint16_t danceXP = 0, fvPosts = 0;
    uint32_t fvSubs = 0;
    int32_t fvEarned = 0, danceEarned = 0;
    uint16_t fvPrice = 5;
};
static SState S;

// live scene (RAM only - never saved)
static bool active = false, pending = false;
static int sa = -1, sb = -1, stage = 0;   // 0 consent choice, 1 transition, 2 private scene, 3 return
static float st = 0;
static bool rollBaby = false;   // Mess Around already rolls pregnancy itself; only Hook Up rolls here

int spicyLevel() { return S.level; }

bool spicyEligible(int a, int b, std::string* why) {
    auto no = [&](const char* m) { if (why) *why = m; return false; };
    if (a < 0 || b < 0 || a >= (int)G.sims.size() || b >= (int)G.sims.size() || a == b) return no("Nobody to be with.");
    const Sim &A = G.sims[a], &B = G.sims[b];
    if (A.stage < ST_YA || B.stage < ST_YA) return no("Adults only.");
    if (isFamily(a, b)) return no("Not with family.");
    if (simDead(a) || simDead(b)) return no("...");
    if (G.room == R_SCHOOL || G.room == R_BOARDING || G.room == R_PLAYGROUND || G.room == R_COLLEGE) return no("Not here.");
    for (const Actor& x : G.rooms[G.house][G.room].actors)
        if (x.kind == 0 && x.id >= 0 && x.id < (int)G.sims.size() && x.id != a && x.id != b && G.sims[x.id].stage <= ST_TEEN) return no("There are kids or teens around - not now.");
    return true;
}

bool spicyBegin(int a, int b) {
    if (S.level == SPL_OFF) return false;
    std::string why;
    if (!spicyEligible(a, b, &why)) { toast(why); return false; }
    sa = a; sb = b; stage = 0; st = 0; active = pending = true;
    return true;
}
bool spicyPending() { bool p = pending; pending = false; return p; }

static Btn bA{20, 150, 130, 40}, bB{170, 150, 130, 40}, bHide{200, 196, 112, 30}, bFade{8, 196, 184, 30};
static void finish(bool happened) {
    active = false;
    struct Reset { ~Reset() { rollBaby = false; } } reset_;
    if (!happened) { toast("You decided to just cuddle instead. No hard feelings."); return; }
    if (sb >= 0 && sb < (int)G.sims.size()) {
        Sim& o = G.sims[sb];
        addMoodlet(o, "Glowing", EM_FLIRTY, 2, 720);
        addMemoryOnce(MEM_LOVE, sa, sb, "A private night together.");
        // pregnancy (separate from the presentation level; uses the game's Pregnancy setting)
        Sim& me = G.sims[sa];
        Sim* mom = !me.look.guy && o.look.guy ? &me : (me.look.guy && !o.look.guy ? &o : nullptr);
        if (mom && rollBaby && !lxBirthControl((int)(mom - &G.sims[0])) && G.pregnancyOn && !mom->pregnant && mom->stage <= ST_MIDDLE && irand(100) < 30) {
            mom->pregnant = true; mom->pregDays = 3;
            toast(mom == &me ? "Surprise! You're expecting a baby! (3 days)" : o.name + " is expecting your baby!");
            addMemoryOnce(MEM_LOVE, sa, sb, "We're having a baby!");
        }
    }
}

bool spicyUpdate(const Input& in) {
    if (!active) return false;
    st += in.dt;
    if (S.level == SPL_OFF) { finish(false); return false; }   // setting turned off mid-scene: end immediately
    const touchPosition& tp = in.tp;
    if (stage == 0) {
        if ((in.down & KEY_A) || (in.touch && bA.in(tp))) { sfxTap(); stage = 1; st = 0; }
        else if ((in.down & KEY_B) || (in.touch && bB.in(tp))) { sfxTap(); finish(false); return false; }
        return true;
    }
    if ((in.down & KEY_B) || (in.touch && bHide.in(tp))) { finish(true); return false; }   // instant hide/skip
    if (in.touch && bFade.in(tp) && S.level > SPL_FADE) { S.level = SPL_FADE; toast("Switched to Fade to Black."); }
    if (stage == 1 && st > 1.0f) { stage = S.level == SPL_FADE ? 3 : 2; st = 0; }
    if (stage == 2 && st > 4.0f) { stage = 3; st = 0; }
    if (stage == 3 && st > 1.5f) { finish(true); return false; }
    return true;
}

static void silhouettes(float t) {
    // two original, fully covered doll silhouettes on a bed - placeholder art, no anatomy
    rect(90, 150, 220, 40, C(240, 200, 220)); rect(90, 140, 220, 12, C(255, 255, 255)); rect(84, 120, 10, 70, C(150, 90, 110));
    const float sway = std::sin(t * 2.f) * 2.f;
    for (int k = 0; k < 2; k++) {
        const float x = 160 + k * 60 + (k ? -sway : sway);
        circle(x, 108, 14, C(60, 30, 50)); rect(x - 10, 120, 20, 30, C(60, 30, 50));
    }
    rect(100, 138, 200, 22, C(255, 150, 190));   // blanket over everything
    circle(200, 90 + std::sin(t * 3.f) * 4, 6, C(255, 80, 140)); circle(214, 84 + std::sin(t * 3.f + 1) * 4, 4, C(255, 120, 170));
}

void spicyDrawTop(float t) {
    if (!active) return;
    rect(0, 0, TOP_W, TOP_H, C(20, 10, 24));
    const std::string na = G.sims[sa].name.substr(0, G.sims[sa].name.find(' ')), nb = G.sims[sb].name.substr(0, G.sims[sb].name.find(' '));
    if (stage == 0) {
        textOutline("A Private Moment", 200, 60, 0.8f, C(255, 200, 230), C(UI_INK), 1);
        text((na + " & " + nb).c_str(), 200, 100, 0.55f, C(255, 255, 255), 1);
        text(("Spicy Mode: " + std::string(SPICY_LEVEL_NAMES[S.level])).c_str(), 200, 130, 0.45f, C(255, 180, 210), 1);
        text("Both of you can still say no.", 200, 160, 0.4f, C(200, 180, 200), 1);
        return;
    }
    if (stage == 1 || stage == 3) {
        const float a = stage == 1 ? std::min(1.f, st) : 1.f - std::min(1.f, st / 1.5f);
        rect(130, 40, 140, 150, C(110, 60, 50)); circle(250, 120, 4, C(230, 200, 90));   // the closed door
        text("Do Not Disturb", 200, 30, 0.4f, C(255, 255, 255), 1);
        rect(0, 0, TOP_W, TOP_H, C(0, 0, 0, (int)(a * 220)));
        if (stage == 3) textOutline("Later that night...", 200, 200, 0.6f, C(255, 220, 240), C(UI_INK), 1);
        return;
    }
    silhouettes(t);
    if (S.level == SPL_CENSORED)   // solid mosaic, drawn every frame in the same place: never flickers off
        for (int y = 0; y < 8; y++) for (int x = 0; x < 16; x++) rect(120 + x * 10.f, 76 + y * 10.f, 10, 10, C((x + y) % 2 ? RGB{200, 120, 160} : RGB{230, 150, 190}));
    for (int k = 0; k < 6; k++) circle(40 + k * 64.f, 30 + std::sin(t * 2 + k) * 8, 5, C(255, 90, 150, 180));
}

void spicyDrawBottom(float t) {
    (void)t;
    rect(0, 0, BOT_W, BOT_H, C(40, 20, 40));
    if (!active) return;
    if (stage == 0) {
        text("Continue the evening?", 160, 60, 0.6f, C(255, 255, 255), 1);
        text("You can skip or hide at any time (B).", 160, 100, 0.4f, C(220, 200, 220), 1);
        button(bA, "Continue (A)", false, 2, 0.5f); button(bB, "Not tonight (B)", false, 0, 0.5f);
        return;
    }
    text(stage == 2 ? "Private scene" : "...", 160, 80, 0.6f, C(255, 220, 240), 1);
    button(bHide, "Hide / Skip (B)", false, 0, 0.42f);
    if (S.level > SPL_FADE) button(bFade, "Switch to Fade to Black", false, 0, 0.4f);
}

// --------------------------------------------------------------- settings / FanVault / Velvet Room
std::vector<std::pair<std::string, int>> spicySettingsOptions() {
    std::vector<std::pair<std::string, int>> v; char b[96];
    snprintf(b, sizeof b, "Current: %s", SPICY_LEVEL_NAMES[S.level]); v.push_back({b, 9599});
    if (S.pendingLevel < SPL_COUNT) {
        snprintf(b, sizeof b, "CONFIRM: set to %s", SPICY_LEVEL_NAMES[S.pendingLevel]); v.push_back({b, 9590});
        v.push_back({LEVEL_DESC[S.pendingLevel], 9599});
        v.push_back({"Cancel", 9591});
        return v;
    }
    if (S.level != SPL_OFF) v.push_back({"Hook Up (find a match tonight)", 9592});
    for (int k = 0; k < SPL_COUNT; k++) { snprintf(b, sizeof b, "%s%s", k == S.level ? "> " : "", SPICY_LEVEL_NAMES[k]); v.push_back({b, 9500 + k}); }
    return v;
}
std::vector<std::pair<std::string, int>> fanVaultOptions() {
    std::vector<std::pair<std::string, int>> v; char b[96];
    if (S.level == SPL_OFF || MC().stage < ST_YA) { v.push_back({"FanVault is 18+. Turn on Spicy Mode first.", 9699}); return v; }
    snprintf(b, sizeof b, "%u subscribers @ $%d/mo  (earned $%ld)", (unsigned)S.fvSubs, S.fvPrice, (long)S.fvEarned); v.push_back({b, 9699});
    v.push_back({"Post a Spicy Photoshoot", 9600});
    v.push_back({"Go Live (tips)", 9601});
    v.push_back({"Raise Price (+$2)", 9602});
    v.push_back({"Lower Price (-$2)", 9603});
    return v;
}
std::vector<std::pair<std::string, int>> velvetSpotOptions(int spot) {
    std::vector<std::pair<std::string, int>> v; char b[64];
    switch (spot) {
    case 0: v.push_back({"Watch the Show ($10)", 9700}); v.push_back({"Make It Rain ($50 tips)", 9701});
        if (S.dancer) { snprintf(b, sizeof b, "Dance a Set (Lv %d)", std::min(10, S.danceXP / 100)); v.push_back({b, 9702}); } break;
    case 1: v.push_back({"Book the VIP Booth w/ Your Date ($60)", 9703}); break;
    case 2: v.push_back({"Order a Fancy Mocktail ($12)", 9704}); break;
    case 3: v.push_back({S.dancer ? "Check Your Earnings" : "Audition to Dance Here", 9705}); break;
    }
    return v;
}

void spicyOption(int id) {
    Sim& me = MC(); Activity a; char b[128];
    if (id >= 9500 && id < 9500 + SPL_COUNT) {
        if (me.stage < ST_YA) { toast("Spicy Mode is for adult characters only."); return; }
        S.pendingLevel = (uint8_t)(id - 9500); toast("Tap CONFIRM to apply."); return;
    }
    switch (id) {
    case 9590: if (S.pendingLevel < SPL_COUNT) { S.level = S.pendingLevel; toast(std::string("Spicy Mode: ") + SPICY_LEVEL_NAMES[S.level]); } S.pendingLevel = 0xFF; return;
    case 9591: S.pendingLevel = 0xFF; return;
    case 9592: {   // Hook Up: a consenting adult match comes over (they can say no)
        if (S.level == SPL_OFF || me.stage < ST_YA) return;
        if (!(roomIsHome(G.room) || G.room == R_HOTEL)) { toast("Hook Up works at home or a hotel room."); return; }
        std::vector<int> c;
        for (int i = 0; i < (int)G.sims.size(); i++) if (i != G.mc && !G.sims[i].household && canRomance(G.mc, i) && spicyEligible(G.mc, i, nullptr)) c.push_back(i);
        if (c.empty()) { toast("No matches nearby right now."); return; }
        const int t = c[irand((int)c.size())];
        const std::string n = G.sims[t].name.substr(0, G.sims[t].name.find(' '));
        if (irand(100) < 30) { toast(n + " left you on read. Their loss!"); return; }
        changeRel(G.mc, t, 5, 12);
        toast(n + " matched with you and came over!");
        rollBaby = true;
        if (spicyBegin(G.mc, t)) { me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 25); addMoodlet(me, "Hooked Up", EM_FLIRTY, 2, 600); } else rollBaby = false;
        return;
    }
    case 9600: {
        if (S.level == SPL_OFF || me.stage < ST_YA) return;
        a = makeAct("Spicy Photoshoot", 60, 8); a.skill = SK_STYLE; a.skillRate = 6; startActivity(a);
        const int gain = 3 + irand(6) + (int)me.skills[SK_STYLE] * 2 + (int)(G.followers / 500);
        S.fvSubs += gain; S.fvPosts++; lxSkill(XS_PHOTO, 12); lxRepAdd(CI_PARTY, 2, "FanVault fame"); if (irand(100) < 12) lxGossip(me.name.substr(0, me.name.find(' ')) + " has a FanVault page", irand(3) != 0, CI_FAMILY, -1);
        snprintf(b, sizeof b, "Posted to FanVault! +%d subscribers (%u total).", gain, (unsigned)S.fvSubs); toast(b);
        if (irand(100) < 8) { addMoodlet(me, "Family Found My FanVault", EM_EMBARRASSED, 2, 1440); toast("Uh oh... someone in the family found your FanVault."); }
        return;
    }
    case 9601: {
        if (S.level == SPL_OFF || me.stage < ST_YA) return;
        a = makeAct("FanVault Live", 45, 8); a.rate[N_SOCIAL] = 10; a.skill = SK_CHARISMA; a.skillRate = 6; startActivity(a);
        const int tips = (int)(S.fvSubs / 4) + irand(20) + (int)me.skills[SK_CHARISMA] * 5;
        G.money += tips; S.fvEarned += tips; snprintf(b, sizeof b, "Your live made $%d in tips!", tips); toast(b); return;
    }
    case 9602: S.fvPrice = (uint16_t)std::min(50, S.fvPrice + 2); if (S.fvSubs > 5) S.fvSubs -= S.fvSubs / 10; toast("Price raised. A few fans unsubscribed."); return;
    case 9603: S.fvPrice = (uint16_t)std::max(1, S.fvPrice - 2); S.fvSubs += 2; toast("Price lowered. New fans rolling in."); return;
    case 9700: if (G.money < 10) { toast("Not enough money!"); return; } G.money -= 10; a = makeAct("Watching the Show", 60, 8); a.rate[N_FUN] = 18; startActivity(a); return;
    case 9701: if (G.money < 50) { toast("Not enough money!"); return; } G.money -= 50; addMoodlet(me, "Made It Rain", EM_CONFIDENT, 2, 240); toast("Bills everywhere. The dancers love you."); return;
    case 9702: {
        a = makeAct("Dancing a Set", 60, 8); a.rate[N_ENERGY] = -12; a.skill = SK_FITNESS; a.skillRate = 8; startActivity(a);
        const int lv = std::min(10, S.danceXP / 100), pay = 60 + lv * 25 + (int)me.skills[SK_CHARISMA] * 10 + irand(40);
        S.danceXP = (uint16_t)std::min(1100, S.danceXP + 25); G.money += pay; S.danceEarned += pay;
        snprintf(b, sizeof b, "Great set! $%d in tips.", pay); toast(b); return;
    }
    case 9703:
        if (!G.date.active || G.date.partner < 0) { toast("Bring a date for the VIP booth."); return; }
        if (G.money < 60) { toast("Not enough money!"); return; } G.money -= 60;
        changeRel(G.mc, G.date.partner, 4, 10); G.date.score += 8; addMoodlet(me, "VIP Night", EM_FLIRTY, 2, 360); toast("Velvet curtains, private booth, just you two."); return;
    case 9704: if (G.money < 12) { toast("Not enough money!"); return; } G.money -= 12; addMoodlet(me, "Fancy Mocktail", EM_HAPPY, 1, 120); return;
    case 9705:
        if (S.dancer) { snprintf(b, sizeof b, "Dancer Lv %d. Lifetime tips: $%ld", std::min(10, S.danceXP / 100), (long)S.danceEarned); toast(b); return; }
        if (me.skills[SK_FITNESS] >= 2 || me.skills[SK_CHARISMA] >= 2) { S.dancer = 1; toast("Manager: \"You're hired! Dance a set at the stage anytime.\""); }
        else toast("Manager: \"Work on your Fitness or Charisma and come back.\"");
        return;
    }
}

bool spicyBlocks(int room, std::string* why) {
    if (room != R_VELVET || G.sims.empty()) return false;
    if (MC().stage < ST_YA) { if (why) *why = "The Velvet Room is 21+ only."; return true; }
    if (S.level == SPL_OFF) { if (why) *why = "The Velvet Room is 18+ - turn on Spicy Mode in the Spicy app."; return true; }
    return false;
}

void spicyDaily() {
    if (S.level == SPL_OFF || S.fvSubs == 0) return;
    const int pay = (int)(S.fvSubs * S.fvPrice / 30);
    if (pay > 0) { G.money += pay; S.fvEarned += pay; }
    if (S.fvPosts == 0 && S.fvSubs > 0) S.fvSubs -= std::max<uint32_t>(1, S.fvSubs / 20);   // no new posts: churn
    S.fvPosts = 0;
}

void spicyDrawRoom(int room, float t, bool night) {
    (void)room; (void)night;
    grad(0, 0, TOP_W, 158, C(40, 8, 30), C(80, 16, 50));
    for (int k = 0; k < 8; k++) circle(25 + k * 50.f, 14, 5, C(255, 60 + (int)(60 * (1 + std::sin(t * 3 + k))), 160));
    rect(110, 110, 180, 46, C(30, 10, 20)); rect(110, 106, 180, 6, C(255, 80, 170)); rect(198, 30, 4, 80, C(220, 220, 230));   // stage + pole
    rect(10, 90, 80, 66, C(120, 20, 50)); text("VIP", 50, 80, 0.35f, C(255, 200, 220), 1);
    rect(310, 110, 80, 46, C(70, 40, 30)); for (int k = 0; k < 5; k++) rect(318 + k * 14.f, 96, 6, 14, C(140, 200, 220, 200));
    textOutline("THE VELVET ROOM", 200, 50, 0.6f, C(255, 180, 220), C(UI_INK), 1); text("21+", 200, 70, 0.4f, C(255, 255, 255), 1);
    rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(50, 10, 30));
}

void spicySaveTo(const std::string& path) { FILE* f = fopen(path.c_str(), "wb"); if (!f) return; fwrite(&S, sizeof S, 1, f); fclose(f); }
void spicyLoadFrom(const std::string& path) {
    S = SState{}; active = false;
    FILE* f = fopen(path.c_str(), "rb"); if (!f) return;
    SState s; size_t n = fread(&s, sizeof s, 1, f); fclose(f);
    if (n != 1 || s.magic != 0x53504331) return;
    if (s.level >= SPL_COUNT) s.level = SPL_OFF; s.pendingLevel = 0xFF; s.dancer &= 1; s.fvPrice = std::max<uint16_t>(1, std::min<uint16_t>(50, s.fvPrice));
    S = s;
}
