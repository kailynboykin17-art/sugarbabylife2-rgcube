#include "deeplife.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include "lifestory.hpp"
#include "lifeext.hpp"
#include "gallery.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include "vfs.hpp"
#define fopen vfopen   // sidecar saves go into the one-file save pack

static void removeFromHome(int s) { for (int r = 0; r < R_COUNT; r++) { auto& v = G.rooms[G.house][r].actors; v.erase(std::remove_if(v.begin(), v.end(), [&](const Actor& a) { return a.kind == 0 && a.id == s; }), v.end()); } }
static void putHome(int s, int room) {
    for (int r = 0; r < R_COUNT; r++) for (const Actor& a : G.rooms[G.house][r].actors) if (a.kind == 0 && a.id == s) return;
    Actor a; a.kind = 0; a.id = (uint16_t)s; a.x = 300; a.tx = 240; a.y = 218; G.rooms[G.house][room].actors.push_back(a);
}

// ---------------------------------------------------------------- data
static const char* PREFS[10] = {"Stylish", "Funny & Charming", "Athletic", "Creative", "Smart", "Good Cook", "Has Money", "Famous Online", "Family-Oriented", "Party Animal"};
static const char* WANTS[10] = {"Save $5,000", "Reach Style Lv 5", "Make a Best Friend", "Go on a Date", "Get Promoted", "Throw a Party", "Reach 1,000 Followers", "Learn to Cook (Lv 4)", "Visit the Aquarium", "Get Fit (Fitness Lv 5)"};
static const char* FEARS[6] = {"Rejection", "Being Broke", "Public Embarrassment", "Being Alone", "Failure", "Confrontation"};
static const char* HABITS[6] = {"Exercise", "Reading", "Late-Night Scrolling", "Cooking at Home", "Journaling", "Impulse Shopping"};
static const char* PTYPES[5] = {"Come to my party", "Pay you back $100", "Keep your secret", "Show up for our date", "Help with homework"};
static const char* POLICY[4] = {"Independent", "Guided Goals", "Fully Manual", "Frozen"};

struct Promise { int16_t who = -1; uint8_t type = 0, status = 0; uint16_t due = 0; };   // status 0 active 1 kept 2 broken 3 forgiven
struct Away { int16_t mem[8] = {-1, -1, -1, -1, -1, -1, -1, -1}; int16_t mc = -1; int32_t money = 0; uint16_t leftDay = 0; uint8_t policy = 0, used = 0, homeId = 0; };
struct DL {
    uint32_t magic = 0x444C4632;
    uint8_t want[2] = {0xFF, 0xFF}, fear = 0xFF, wantDone = 0;
    uint8_t habit[6] = {};
    uint8_t kept[128] = {}, broken[128] = {}, lied[128] = {};
    Promise pr[6];
    Away away[4];
    uint8_t curPolicy = 0, fearCool = 0;
    char recap[6][48] = {};
    uint8_t recapN = 0;
};
static DL D;
static std::vector<Placed> awayItems[4][R_COUNT];   // each parked household keeps its own furniture
static int pickWho = -1;

static bool okSim(int s) { return s >= 0 && s < (int)G.sims.size() && !simDead(s); }
static std::string first(int s) { const std::string& n = G.sims[s].name; return n.substr(0, n.find(' ')); }
static uint32_t h32(uint32_t x) { x ^= x >> 16; x *= 0x7feb352d; x ^= x >> 15; x *= 0x846ca68b; x ^= x >> 16; return x; }
static int prefOf(int s, int k) { return (int)(h32((uint32_t)s * 3 + (uint32_t)k + 77) % 10); }   // k: 0,1 turn-ons, 2 turn-off
static bool hasPref(int p) {
    const Sim& me = MC();
    switch (p) {
    case 0: return me.skills[SK_STYLE] >= 4; case 1: return me.skills[SK_CHARISMA] >= 4; case 2: return me.skills[SK_FITNESS] >= 4;
    case 3: return me.skills[SK_CREATIVITY] >= 4; case 4: return me.skills[SK_LOGIC] >= 4; case 5: return me.skills[SK_COOKING] >= 4;
    case 6: return G.money >= 5000; case 7: return G.followers >= 1000; case 8: return me.stage >= ST_ADULT; case 9: return me.hasTrait(TR_DRAMATIC) || me.skills[SK_CHARISMA] >= 6;
    }
    return false;
}
int dlCompat(int a, int b) {
    (void)a; if (!okSim(b)) return 0;
    int c = 45 + (int)(h32((uint32_t)b * 31u + (uint32_t)G.mc) % 11) - 5;   // small fixed per-pair variance
    const Sim &A = MC(), &B = G.sims[b];
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) if (A.traits[i] && A.traits[i] == B.traits[j]) c += 8;
    if (hasPref(prefOf(b, 0))) c += 15; if (hasPref(prefOf(b, 1))) c += 12; if (hasPref(prefOf(b, 2))) c -= 20;
    if (b < 128) c -= D.broken[b] * 6 + D.lied[b] * 4, c += std::min(10, (int)D.kept[b] * 2);
    return std::max(0, std::min(100, c));
}
static std::vector<int> topFriends(int n) {
    std::vector<std::pair<int, int>> v;
    for (const Rel& r : G.rels) { int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1; if (o >= 0 && okSim(o) && !G.sims[o].household) v.push_back({r.fr + r.ro, o}); }
    std::sort(v.rbegin(), v.rend()); std::vector<int> o; for (size_t i = 0; i < v.size() && (int)i < n; i++) o.push_back(v[i].second); return o;
}
static int friendCount(int fr) { int n = 0; for (const Rel& r : G.rels) if ((r.a == G.mc || r.b == G.mc) && r.fr >= fr) n++; return n; }
static int wantProgress(int w) {   // 0..100
    const Sim& me = MC();
    switch (w) {
    case 0: return std::min(100, (int)(G.money * 100 / 5000)); case 1: return std::min(100, (int)(me.skills[SK_STYLE] * 20));
    case 2: return friendCount(80) ? 100 : std::min(99, friendCount(50) * 40); case 3: return G.date.active ? 100 : 0;
    case 4: return 0; case 5: return 0; case 6: return std::min(100, (int)(G.followers / 10));
    case 7: return std::min(100, (int)(me.skills[SK_COOKING] * 25)); case 8: return G.room == R_AQUARIUM ? 100 : 0;
    case 9: return std::min(100, (int)(me.skills[SK_FITNESS] * 20));
    }
    return 0;
}
static void rollWants() { for (int k = 0; k < 2; k++) if (D.want[k] >= 10) { int w; do w = irand(10); while (w == D.want[1 - k]); D.want[k] = (uint8_t)w; } if (D.fear >= 6) D.fear = (uint8_t)irand(6); }

// ---------------------------------------------------------------- menus
DlMenu dlApp(int which) {
    DlMenu v; char b[110];
    switch (which) {
    case 0: {
        v.push_back({"Chemistry - tap someone:", 11999});
        for (int o : topFriends(10)) {
            if (!canRomance(G.mc, o)) continue;
            snprintf(b, sizeof b, "%s - %d%% match", first(o).c_str(), dlCompat(G.mc, o)); v.push_back({b, 11000 + o});
        }
        if (v.size() == 1) v.push_back({"Meet more adults to see your matches.", 11999});
        break;
    }
    case 1:
        rollWants();
        for (int k = 0; k < 2; k++) { snprintf(b, sizeof b, "WANT: %s (%d%%)", WANTS[D.want[k]], wantProgress(D.want[k])); v.push_back({b, 11999}); }
        snprintf(b, sizeof b, "FEAR: %s", FEARS[D.fear]); v.push_back({b, 11999});
        snprintf(b, sizeof b, "Wants fulfilled: %d", D.wantDone); v.push_back({b, 11999});
        v.push_back({"Swap Wants (new ones)", 11300}); v.push_back({"Face Your Fear (therapy session $60)", 11301});
        break;
    case 2:
        for (int k = 0; k < 6; k++) { snprintf(b, sizeof b, "%s: %d%%%s", HABITS[k], D.habit[k], D.habit[k] >= 60 ? " (strong habit)" : ""); v.push_back({b, 11999}); }
        v.push_back({"Write in Your Journal", 11400}); v.push_back({"Read a Book", 11401}); v.push_back({"Morning Workout", 11402}); v.push_back({"Put the Phone Away (break scrolling)", 11403});
        break;
    case 3: {
        bool any = false;
        for (int i = 0; i < 6; i++) { const Promise& p = D.pr[i]; if (p.who < 0 || !okSim(p.who)) continue; any = true;
            static const char* ST[4] = {"due", "KEPT", "BROKEN", "forgiven"};
            snprintf(b, sizeof b, "%s: \"%s\" - %s (day %d)", first(p.who).c_str(), PTYPES[p.type], ST[p.status % 4], p.due + 1); v.push_back({b, 11999});
            if (p.status == 0) v.push_back({"  Keep this promise now", 11500 + i});
            if (p.status == 2) { v.push_back({"  Apologize & own it", 11510 + i}); v.push_back({"  Tell a lie to cover it", 11520 + i}); v.push_back({"  Show receipts (explain)", 11530 + i}); }
        }
        if (!any) v.push_back({"No promises yet.", 11999});
        v.push_back({"Make a New Promise...", 11540});
        break;
    }
    case 4: {
        snprintf(b, sizeof b, "This household: %s autonomy", POLICY[D.curPolicy % 4]); v.push_back({b, 11999});
        v.push_back({"Change Autonomy Policy", 11600});
        for (int i = 0; i < 4; i++) if (D.away[i].used && okSim(D.away[i].mc)) { snprintf(b, sizeof b, "Return to %s's household (away %d days)", first(D.away[i].mc).c_str(), G.day - D.away[i].leftDay); v.push_back({b, 11610 + i}); }
        v.push_back({"-- Switch to another family --", 11999});
        int shown = 0;
        for (int i = 0; i < (int)G.sims.size() && shown < 8; i++) {
            if (!okSim(i) || G.sims[i].household || G.sims[i].stage < ST_YA) continue;
            bool head = true; for (int j = 0; j < i; j++) if (okSim(j) && !G.sims[j].household && isFamily(i, j) && G.sims[j].stage >= ST_YA) { head = false; break; }
            if (!head) continue;
            int n = 1; for (int j = 0; j < (int)G.sims.size(); j++) if (j != i && okSim(j) && !G.sims[j].household && isFamily(i, j)) n++;
            const std::string& nm = G.sims[i].name; snprintf(b, sizeof b, "The %s family (%d)", nm.substr(nm.rfind(' ') + 1).c_str(), n);
            v.push_back({b, 12200 + i}); shown++;
        }
        if (D.recapN) v.push_back({"Last 'While You Were Away' recap", 11690});
        break;
    }
    }
    return v;
}

static void addPromise(int who, int type) {
    int slot = -1; for (int i = 0; i < 6; i++) if (D.pr[i].who < 0 || D.pr[i].status == 1 || D.pr[i].status == 3) { slot = i; break; }
    if (slot < 0) slot = 0;
    D.pr[slot] = Promise{}; D.pr[slot].who = (int16_t)who; D.pr[slot].type = (uint8_t)type; D.pr[slot].due = (uint16_t)(G.day + 1);
    if (type == 1) G.money += 100;
    changeRel(G.mc, who, 3, 0);
    toast("You promised " + first(who) + ": \"" + PTYPES[type] + "\" (due tomorrow).");
}

static DlMenu recapMenu() { DlMenu v; v.push_back({"While You Were Away...", 11999}); for (int i = 0; i < D.recapN; i++) v.push_back({D.recap[i], 11999}); if (!D.recapN) v.push_back({"Nothing major happened.", 11999}); return v; }
static void recap(const std::string& s) { if (D.recapN < 6) snprintf(D.recap[D.recapN++], 48, "%s", s.c_str()); }

static void switchTo(const std::vector<int>& members, int newMc, int32_t money, int home, int restoreSlot = -1) {
    // park the current household
    int slot = -1; for (int i = 0; i < 4; i++) if (!D.away[i].used) { slot = i; break; }
    if (slot < 0) slot = 0;
    Away& A = D.away[slot]; A = Away{}; A.used = 1; A.mc = (int16_t)G.mc; A.money = G.money; A.leftDay = G.day; A.policy = D.curPolicy; A.homeId = G.homeId;
    for (int r = 0; r < R_COUNT; r++) awayItems[slot][r] = roomIsHome(r) ? G.rooms[G.house][r].items : std::vector<Placed>();
    int k = 0;
    for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household) { if (k < 8) A.mem[k++] = (int16_t)i; G.sims[i].household = false; removeFromHome(i); }
    for (int m : members) { G.sims[m].household = true; putHome(m, R_LIVING); }
    G.mc = newMc; G.money = money; D.curPolicy = 0;
    if (home < 0) {   // a family of their own needs a home of their own: pick one nobody else lives in
        for (int t = 0; t < 40; t++) { int h = 2 + irand(std::max(1, galleryHouseCount() - 2)); bool used = h == A.homeId; for (const Away& o : D.away) if (o.used && o.homeId == h) used = true; if (!used) { home = h; break; } }
        if (home < 0) home = (A.homeId + 1) % galleryHouseCount();
    }
    std::vector<Placed> keep[R_COUNT];
    if (restoreSlot >= 0) for (int r = 0; r < R_COUNT; r++) keep[r].swap(awayItems[restoreSlot][r]);
    galleryMoveIn(home);
    bool any = false; for (int r = 0; r < R_COUNT; r++) any |= !keep[r].empty();
    if (any) for (int r = 0; r < R_COUNT; r++) if (roomIsHome(r)) G.rooms[G.house][r].items = keep[r];
}

bool dlMoveOut(int mover, int partner, int32_t take) {
    if (mover < 0 || mover >= (int)G.sims.size()) return false;
    int stay = -1; for (int i = 0; i < (int)G.sims.size(); i++) if (i != mover && i != partner && G.sims[i].household && G.sims[i].stage >= ST_YA) { stay = i; break; }
    if (stay < 0) return false;   // nobody grown-up left to keep the old house
    const int32_t total = G.money; take = std::min(take, total);
    G.mc = stay;
    std::vector<int> mem{mover}; if (partner >= 0 && partner != mover) mem.push_back(partner);
    switchTo(mem, mover, take, -1);
    for (Away& A : D.away) if (A.used && A.mc == stay && A.leftDay == G.day) {   // the household we just parked: take the movers out of it
        A.money = total - take;
        int k = 0; int16_t keep[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
        for (int j = 0; j < 8; j++) if (A.mem[j] >= 0 && A.mem[j] != mover && A.mem[j] != partner) keep[k++] = A.mem[j];
        memcpy(A.mem, keep, sizeof keep);
    }
    return true;
}

DlMenu dlOption(int id) {
    Sim& me = MC(); char b[120]; Activity a;
    if (id >= 11000 && id < 11200 && id - 11000 < (int)G.sims.size()) {
        const int o = id - 11000; if (!okSim(o)) return {};
        Rel& r = rel(G.mc, o); DlMenu v;
        snprintf(b, sizeof b, "%s - %d%% compatible", G.sims[o].name.c_str(), dlCompat(G.mc, o)); v.push_back({b, 11999});
        snprintf(b, sizeof b, "Turn-on: %s", r.fr >= 30 ? PREFS[prefOf(o, 0)] : "??? (get closer)"); v.push_back({b, 11999});
        snprintf(b, sizeof b, "Turn-on: %s", r.fr >= 50 ? PREFS[prefOf(o, 1)] : "??? (get closer)"); v.push_back({b, 11999});
        snprintf(b, sizeof b, "Turn-off: %s", r.fr >= 50 ? PREFS[prefOf(o, 2)] : "???"); v.push_back({b, 11999});
        if (o < 128 && (D.broken[o] || D.kept[o])) { snprintf(b, sizeof b, "History: %d promises kept, %d broken, %d lies", D.kept[o], D.broken[o], D.lied[o]); v.push_back({b, 11999}); }
        v.push_back({"Deep Talk (reveal more)", 11200 + o}); v.push_back({"Ask Them Out", 12000 + o});
        return v;
    }
    if (id >= 11200 && id < 11300) { const int o = id - 11200; a = makeAct("Deep Talk", 45, 8); a.rate[N_SOCIAL] = 15; startActivity(a); changeRel(G.mc, o, 8, 2);
        toast(first(o) + (dlCompat(G.mc, o) >= 60 ? ": \"I like that you actually listen.\"" : ": \"Huh. We're pretty different, huh?\"")); return dlOption(11000 + o); }
    if (id >= 12000 && id < 12200) {
        const int o = id - 12000; if (!okSim(o) || !canRomance(G.mc, o)) return {};
        const int c = dlCompat(G.mc, o), ro = rel(G.mc, o).ro;
        if (irand(100) < c + ro / 3 - 10) { changeRel(G.mc, o, 4, 12); if (rel(G.mc, o).status == RS_NONE && ro >= 40) rel(G.mc, o).status = RS_DATING; addMoodlet(me, "They Said Yes!", EM_FLIRTY, 2, 720); toast(first(o) + ": \"Yes! I was hoping you'd ask.\""); addMemoryOnce(MEM_LOVE, G.mc, o, "Asked " + first(o) + " out - they said yes!"); }
        else { addMoodlet(me, "Rejected", EM_SAD, 1, 360); toast(first(o) + (c < 40 ? ": \"You're cute, but I don't think we want the same things.\"" : ": \"Not right now... maybe someday?\"")); if (D.fear == 0) addMoodlet(me, "My Worst Fear", EM_TENSE, 2, 480); }
        return {};
    }
    if (id == 11600) { D.curPolicy = (uint8_t)((D.curPolicy + 1) % 4); return dlApp(4); }
    if (id >= 11610 && id < 11614) {
        Away A = D.away[id - 11610]; if (!A.used || !okSim(A.mc)) return {};
        const int slot = id - 11610; D.away[slot].used = 0;
        std::vector<int> mem; for (int m : A.mem) if (okSim(m)) mem.push_back(m);
        // off-screen progress for the household we're returning to
        D.recapN = 0; const int days = std::max(0, (int)G.day - (int)A.leftDay);
        int32_t money = A.money;
        if (A.policy != 3 && days > 0) {
            for (int m : mem) {
                Sim& s = G.sims[m]; if (s.stage < ST_TEEN) continue;
                const int sk = irand(SK_COUNT); s.skills[sk] = std::min(10.f, s.skills[sk] + 0.3f * days);
                if (irand(3) == 0) recap(first(m) + " got better at " + SKILL_NAMES[sk] + ".");
                if (A.policy == 0 && irand(4) == 0) { for (int o = 0; o < (int)G.sims.size(); o++) if (okSim(o) && !G.sims[o].household && o != m && irand(20) == 0) { changeRel(m, o, 25, 0); recap(first(m) + " made a friend: " + first(o) + "."); break; } }
            }
            const int income = 60 * days * (int)mem.size(); money += income;
            snprintf(b, sizeof b, "Earned $%d while you were away.", income); recap(b);
            if (A.policy == 0 && days >= 3 && irand(3) == 0) recap("They threw a little get-together.");
        } else if (A.policy == 3) recap("Frozen: nothing changed.");
        switchTo(mem, A.mc, money, A.homeId, slot);
        return recapMenu();
    }
    if (id == 11690) return recapMenu();
    if (id >= 12200 && id < 12400) {
        const int h = id - 12200; if (!okSim(h)) return {};
        std::vector<int> mem{h}; for (int j = 0; j < (int)G.sims.size() && mem.size() < 8; j++) if (j != h && okSim(j) && !G.sims[j].household && isFamily(h, j)) mem.push_back(j);
        switchTo(mem, h, 1500 + irand(3000), -1);
        toast("You're now playing the " + G.sims[h].name.substr(G.sims[h].name.rfind(' ') + 1) + " family! (Your old household keeps living.)"); sfxYay();
        return {};
    }
    switch (id) {
    case 11300: D.want[0] = D.want[1] = 0xFF; rollWants(); return dlApp(1);
    case 11301: if (G.money < 60) { toast("Not enough money!"); return {}; } G.money -= 60; a = makeAct("Therapy Session", 60, 8); startActivity(a); addMoodlet(me, "Braver Than Yesterday", EM_CONFIDENT, 2, 1440); D.fear = (uint8_t)((D.fear + 1 + irand(5)) % 6); toast("You worked through it. A new, smaller fear remains."); return {};
    case 11400: a = makeAct("Journaling", 30, 8); a.rate[N_FUN] = 4; startActivity(a); D.habit[4] = (uint8_t)std::min(100, D.habit[4] + 8); addMoodlet(me, "Clear Head", EM_FOCUSED, 1, 240); return {};
    case 11401: a = makeAct("Reading", 60, 8); a.skill = SK_LOGIC; a.skillRate = 8; startActivity(a); D.habit[1] = (uint8_t)std::min(100, D.habit[1] + 8); return {};
    case 11402: a = makeAct("Morning Workout", 45, 8); a.rate[N_ENERGY] = -8; a.skill = SK_FITNESS; a.skillRate = 10; startActivity(a); D.habit[0] = (uint8_t)std::min(100, D.habit[0] + 8); return {};
    case 11403: D.habit[2] = (uint8_t)std::max(0, D.habit[2] - 15); addMoodlet(me, "Unplugged", EM_FINE, 1, 300); toast("Phone down. You feel a little calmer."); return {};
    case 11540: { DlMenu v; v.push_back({"Promise who?", 11999}); for (int o : topFriends(6)) v.push_back({G.sims[o].name, 12400 + o}); return v; }
    }
    if (id >= 11500 && id < 11506) {   // keep promise
        Promise& p = D.pr[id - 11500]; if (p.who < 0 || p.status) return {};
        if (p.type == 1 && G.money < 100) { toast("You don't have the $100 to pay back yet."); return {}; }
        if (p.type == 1) G.money -= 100;
        p.status = 1; if (p.who < 128) D.kept[p.who] = (uint8_t)std::min(250, D.kept[p.who] + 1);
        changeRel(G.mc, p.who, 8, 0); toast(first(p.who) + ": \"You actually came through. That means a lot.\"");
        a = makeAct("Keeping a Promise", 60, 8); a.rate[N_SOCIAL] = 10; startActivity(a); return dlApp(3);
    }
    if (id >= 11510 && id < 11536) {
        Promise& p = D.pr[(id - 11510) % 10]; const int kind = (id - 11510) / 10; if (p.who < 0 || p.status != 2) return {};
        const std::string n = first(p.who);
        if (kind == 0) { p.status = 3; changeRel(G.mc, p.who, 5, 0); toast(n + ": \"Okay. Thanks for owning it.\""); }
        else if (kind == 1) {
            if (p.who < 128) D.lied[p.who]++;
            if (irand(100) < 45) { changeRel(G.mc, p.who, -15, -10); toast(n + " found the receipts. Caught in a lie!"); lxGossip(first(G.mc) + " lied to " + n, true, CI_FAMILY, -1); }
            else { p.status = 3; changeRel(G.mc, p.who, 2, 0); toast(n + " bought your story... for now."); }
        } else {
            if (irand(10) < 4 + (int)me.skills[SK_CHARISMA] / 2) { p.status = 3; changeRel(G.mc, p.who, 4, 0); toast("You showed the texts. " + n + ": \"Oh - I didn't know. My bad.\""); }
            else toast(n + ": \"Receipts or not, you still didn't show.\"");
        }
        return dlApp(3);
    }
    if (id >= 12400 && id < 12600) { pickWho = id - 12400; DlMenu v; v.push_back({"Promise what?", 11999}); for (int k = 0; k < 5; k++) v.push_back({PTYPES[k], 11800 + k}); return v; }
    if (id >= 11800 && id < 11805 && okSim(pickWho)) { addPromise(pickWho, id - 11800); return dlApp(3); }
    return {};
}


void dlHabit(int k, int amt) { if (k >= 0 && k < 6) D.habit[k] = (uint8_t)std::min(100, D.habit[k] + amt); }
void dlHourly(int hour) {
    if (G.sims.empty()) return;
    if (ACT.left > 0) {   // habits form from what you actually do
        const std::string& n = ACT.name;
        auto has = [&](const char* k) { return n.find(k) != std::string::npos; };
        if (has("Gym") || has("Workout") || has("Treadmill") || has("Dance") || has("Yoga") || has("Push")) D.habit[0] = (uint8_t)std::min(100, D.habit[0] + 3);
        if (has("Read") || has("Study")) D.habit[1] = (uint8_t)std::min(100, D.habit[1] + 3);
        if ((hour >= 23 || hour < 3) && (has("Live") || has("Filming") || has("Scroll") || has("Phone"))) D.habit[2] = (uint8_t)std::min(100, D.habit[2] + 5);
        if (has("Cook") || has("Bak")) D.habit[3] = (uint8_t)std::min(100, D.habit[3] + 3);
    }
    if (hour == 21) for (int k = 0; k < 2; k++) if (D.want[k] < 10 && wantProgress(D.want[k]) >= 100) {
        D.wantDone++; addMoodlet(MC(), "Want Fulfilled!", EM_CONFIDENT, 2, 1440); toast(std::string("Want fulfilled: ") + WANTS[D.want[k]] + "! (+$150)"); G.money += 150; sfxYay();
        addMemoryOnce(MEM_GOAL, G.mc, -1, WANTS[D.want[k]]); D.want[k] = 0xFF; rollWants();
    }
}

void dlDaily() {
    if (G.sims.empty()) return;
    rollWants();
    Sim& me = MC();
    // fears (with a cooldown so they don't spam)
    if (D.fearCool) D.fearCool--;
    else {
        bool hit = (D.fear == 1 && G.money < 200) || (D.fear == 3 && friendCount(40) == 0) || (D.fear == 0 && irand(100) < 8) || (D.fear == 2 && irand(100) < 6) || (D.fear == 4 && irand(100) < 6) || (D.fear == 5 && irand(100) < 6);
        if (hit) { addMoodlet(me, (std::string("Fear: ") + FEARS[D.fear]).c_str(), EM_TENSE, 1, 720); D.fearCool = 3; }
    }
    // habits pay off (or cost) and slowly fade
    if (D.habit[0] >= 50) { me.skills[SK_FITNESS] = std::min(10.f, me.skills[SK_FITNESS] + 0.1f); addMoodlet(me, "Workout Habit", EM_ENERGIZED, 1, 600); }
    if (D.habit[1] >= 50) me.skills[SK_LOGIC] = std::min(10.f, me.skills[SK_LOGIC] + 0.1f);
    if (D.habit[2] >= 50) { me.needs[N_ENERGY] = std::max(0.f, me.needs[N_ENERGY] - 15); addMoodlet(me, "Up Too Late Scrolling", EM_BORED, 1, 480); }
    if (D.habit[3] >= 50) G.money += 10;
    if (D.habit[4] >= 50) addMoodlet(me, "Journaling Clarity", EM_FOCUSED, 1, 480);
    if (D.habit[5] >= 50 && G.money > 60 && irand(100) < D.habit[5] / 2) { G.money -= 40; addMoodlet(me, "Impulse Buy!", EM_HAPPY, 1, 240); toast("Oops... you impulse-bought something cute online. -$40"); }
    for (int k = 0; k < 6; k++) D.habit[k] = (uint8_t)std::max(0, D.habit[k] - 2);
    // promises come due
    for (Promise& p : D.pr) if (p.who >= 0 && p.status == 0 && G.day > p.due) {
        p.status = 2; if (p.who < 128) D.broken[p.who] = (uint8_t)std::min(250, D.broken[p.who] + 1);
        if (okSim(p.who)) { changeRel(G.mc, p.who, -12, -5); addMoodlet(G.sims[p.who], "Let Down", EM_SAD, 2, 1440); toast(first(p.who) + " is hurt - you broke your promise: \"" + PTYPES[p.type] + "\"."); addMemoryOnce(MEM_FRIEND, p.who, G.mc, "They broke a promise."); }
    }
    // people remember
    for (int i = 0; i < (int)G.sims.size() && i < 128; i++) if (D.broken[i] >= 2 && okSim(i) && irand(100) < 4) { toast(first(i) + ": \"You say a lot of things. I'll believe it when I see it.\""); break; }
}

void dlSaveTo(const std::string& path) {
    FILE* f = fopen(path.c_str(), "wb"); if (!f) return; fwrite(&D, sizeof D, 1, f); fclose(f);
    f = fopen((path + "i").c_str(), "wb"); if (!f) return;
    const uint32_t hdr[2] = {0x32495741u, (uint32_t)R_COUNT}; fwrite(hdr, sizeof hdr, 1, f);   // "AWI2" + room count, so adding rooms never scrambles old saves
    for (int s = 0; s < 4; s++) for (int r = 0; r < R_COUNT; r++) { uint16_t n = (uint16_t)std::min<size_t>(awayItems[s][r].size(), 400); fwrite(&n, 2, 1, f); if (n) fwrite(awayItems[s][r].data(), sizeof(Placed), n, f); }
    fclose(f);
}
void dlLoadFrom(const std::string& path) {
    D = DL{}; for (auto& s : awayItems) for (auto& v : s) v.clear();
    if (FILE* fi = fopen((path + "i").c_str(), "rb")) {
        uint32_t hdr[2] = {0, 0}; int rc = R_COUNT_V50;   // files from before the header: written with the v50 room list
        if (fread(hdr, sizeof hdr, 1, fi) == 1 && hdr[0] == 0x32495741u && hdr[1] <= 512) rc = (int)hdr[1]; else fseek(fi, 0, SEEK_SET);
        for (int s = 0; s < 4; s++) for (int r = 0; r < rc; r++) { uint16_t n = 0; if (fread(&n, 2, 1, fi) != 1 || n > 400) { s = 4; break; } if (r >= R_COUNT) { fseek(fi, (long)n * (long)sizeof(Placed), SEEK_CUR); continue; }
            awayItems[s][r].resize(n); if (n && fread(awayItems[s][r].data(), sizeof(Placed), n, fi) != n) { awayItems[s][r].clear(); s = 4; break; } }
        fclose(fi);
        for (auto& s : awayItems) for (auto& v : s) for (Placed& p : v) if (p.item >= 4096) { v.clear(); break; }
    }
    FILE* f = fopen(path.c_str(), "rb"); if (!f) return;
    DL d; size_t n = fread(&d, 1, sizeof d, f); fclose(f);
    if (n < 16 || d.magic != 0x444C4632) return;
    for (Promise& p : d.pr) if (p.who >= (int)G.sims.size()) p.who = -1;
    for (Away& a : d.away) { if (a.mc >= (int)G.sims.size()) a.used = 0; for (auto& m : a.mem) if (m >= (int)G.sims.size()) m = -1; }
    for (auto& r : d.recap) r[47] = 0; if (d.recapN > 6) d.recapN = 0;
    D = d;
}
