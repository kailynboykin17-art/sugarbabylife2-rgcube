#include "growup.hpp"
#include "lifeext.hpp"
// Life story: milestones, family roles & dynamics, deaths (non-graphic), memorials, adoption, Life Book.
#include "worldsim.hpp"
#include "health.hpp"
#include "lifestory.hpp"
#include "people.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>
using namespace col;
void toast(const std::string& m);
bool askText(const char* hint, std::string& out, int maxLen, bool numeric);

namespace {
bool ok(int s) { return s >= 0 && s < (int)G.sims.size(); }
void fit() { size_t n = G.sims.size(); if (G.deathType.size() < n) G.deathType.resize(n, 0); if (G.deathDay.size() < n) G.deathDay.resize(n, 0); if (G.sickDays.size() < n) G.sickDays.resize(n, 0); }

struct MsInfo { const char* name; const char* desc; uint8_t mem; RGB col; };
const MsInfo MS[MI_COUNT] = {
    {"First Smile", "A gummy little smile that melted everyone.", MEM_FAMILY, {255, 214, 80}},
    {"First Laugh", "A real belly giggle!", MEM_FAMILY, {255, 214, 80}},
    {"First Words", "Said their very first word.", MEM_FAMILY, {120, 200, 255}},
    {"First Steps", "Wobbled across the room all alone.", MEM_FAMILY, {120, 230, 160}},
    {"First Foods", "Tried real food (and wore most of it).", MEM_FAMILY, {255, 150, 60}},
    {"Potty Trained", "Big kid potty, all by themselves!", MEM_FAMILY, {120, 230, 160}},
    {"First Birthday", "Cake smash for the history books.", MEM_FAMILY, {255, 110, 170}},
    {"First Friend", "Made their very first friend.", MEM_FRIEND, {120, 230, 160}},
    {"First Sleepover", "Pillow forts, snacks and secrets.", MEM_FRIEND, {190, 120, 220}},
    {"First Day of School", "Backpack on, brave face ready.", MEM_LIFE, {120, 200, 255}},
    {"First School Award", "Top marks on the report card!", MEM_LIFE, {255, 214, 80}},
    {"First Hobby Club", "Found an extracurricular they love.", MEM_FUN, {60, 190, 190}},
    {"First Childhood Bestie", "Best friends forever (pinky promise).", MEM_FRIEND, {255, 110, 170}},
    {"First Crush", "Butterflies for the first time.", MEM_LOVE, {255, 110, 170}},
    {"First Birthday Party", "A real party with friends!", MEM_FUN, {255, 110, 170}},
    {"First School Dance", "Slow songs and awkward swaying.", MEM_FUN, {190, 120, 220}},
    {"First Relationship", "Officially dating!", MEM_LOVE, {255, 110, 170}},
    {"First Date", "Went on their very first date.", MEM_LOVE, {255, 110, 170}},
    {"First Kiss", "The first kiss. Fireworks.", MEM_LOVE, {230, 50, 90}},
    {"Graduation", "Caps in the air!", MEM_LIFE, {255, 214, 80}},
    {"First Job", "Got hired for the first time.", MEM_CAREER, {120, 200, 255}},
    {"First Promotion", "Moved up at work!", MEM_CAREER, {255, 214, 80}},
    {"Moved Out", "A brand-new place to call home.", MEM_HOME, {150, 110, 80}},
    {"Married", "Said \"I do.\"", MEM_LOVE, {255, 255, 255}},
    {"Became a Parent", "A whole new little person to love.", MEM_FAMILY, {255, 150, 190}},
    {"Career Peak", "Reached the top of their career!", MEM_CAREER, {255, 200, 60}},
};

const Lineage* lineOf(int kid) { for (const Lineage& l : G.lineage) if (l.kid == kid) return &l; return nullptr; }
void parentsOf(int s, int& a, int& b) { a = b = -1; const Lineage* l = lineOf(s); if (l) { if (ok(l->pa)) a = l->pa; if (ok(l->pb)) b = l->pb; } }
bool isParent(int p, int kid) { int a, b; parentsOf(kid, a, b); return p >= 0 && (p == a || p == b); }
int sharedParents(int x, int y) { int a1, b1, a2, b2; parentsOf(x, a1, b1); parentsOf(y, a2, b2); int n = 0; for (int p : {a1, b1}) if (p >= 0 && (p == a2 || p == b2)) n++; return n; }
bool sibling(int x, int y) { return x != y && sharedParents(x, y) > 0; }
bool married(int x, int y) { const Rel* r = findRel(x, y); return r && r->status == RS_MARRIED; }
bool adoptedKid(int k) { return std::find(G.adopted.begin(), G.adopted.end(), (uint16_t)k) != G.adopted.end(); }
bool isGuy(int s) { return G.sims[s].look.guy; }
const char* g2(int s, const char* f, const char* m) { return isGuy(s) ? m : f; }

std::vector<std::pair<int, int>> estranged;   // pairs we've seen fall out (for reconciliation moments)
}

// ================================================================ milestones
bool hasMilestone(int sim, int id) { for (const MilestoneRec& m : G.milestones) if (m.sim == sim && m.id == id) return true; return false; }
void milestone(int sim, int id, int other) {
    if (!ok(sim) || id < 0 || id >= MI_COUNT || hasMilestone(sim, id) || simDead(sim)) return;
    MilestoneRec r; r.sim = (uint16_t)sim; r.day = G.day; r.id = (uint8_t)id; r.other = (int16_t)(ok(other) ? other : -1);
    G.milestones.push_back(r);
    if (G.milestones.size() > 2000) G.milestones.erase(G.milestones.begin());
    Sim& s = G.sims[sim];
    if (!s.household && sim != G.mc) return;   // townies keep the record quietly
    std::string txt = s.name + ": " + MS[id].name + "!";
    if (ok(other)) txt += " (with " + G.sims[other].name + ")";
    addMemoryOnce(MS[id].mem, sim, ok(other) ? other : -1, txt);
    addMoodlet(s, MS[id].name, id == MI_GRAD || id == MI_PROMO || id == MI_CAREERPEAK || id == MI_AWARD ? EM_CONFIDENT : id == MI_KISS || id == MI_CRUSH || id == MI_RELATIONSHIP ? EM_FLIRTY : EM_HAPPY, 2, 480);
    // the whole family feels it
    for (size_t j = 0; j < G.sims.size(); j++) if ((int)j != sim && G.sims[j].household && !simDead((int)j)) {
        changeRel(sim, (int)j, 2, 0);
        if (id <= MI_PARTY && G.sims[j].isAdult()) addMoodlet(G.sims[j], "Proud Family Moment", EM_HAPPY, 1, 240);
    }
    toast("Milestone! " + txt);
}

void lifeStoryAgeUp(int sim) {
    if (!ok(sim)) return;
    const Sim& s = G.sims[sim];
    if (s.stage == ST_TODDLER) milestone(sim, MI_BIRTHDAY);
    if (s.stage == ST_CHILD) milestone(sim, MI_PARTY);
    if (s.stage == ST_YA) milestone(sim, MI_GRAD);
}

void lifeStoryHourly(int hour) {
    fit();
    const int wd = weekday(G.day);
    for (size_t i = 0; i < G.sims.size(); i++) {
        const int s = (int)i; const Sim& o = G.sims[i];
        if (!(o.household || s == G.mc) || simDead(s)) continue;
        switch (o.stage) {
        case ST_INFANT: milestone(s, MI_SMILE); if (o.stageDays >= 1) milestone(s, MI_LAUGH); if (o.stageDays >= 1 && hour >= 12) milestone(s, MI_FOODS); break;
        case ST_TODDLER: milestone(s, MI_STEPS); if (o.stageDays >= 1) milestone(s, MI_WORDS); if (o.stageDays >= 2) milestone(s, MI_POTTY); break;
        default: break;
        }
        if (o.stage >= ST_CHILD && o.stage <= ST_TEEN) {
            if (isWeekday(G.day) && hour >= 8 && hour < 15) milestone(s, MI_SCHOOLDAY);
            if (o.gradePts >= 90) milestone(s, MI_AWARD);
            for (int k : {SK_CREATIVITY, SK_FITNESS, SK_LOGIC}) if (skillLevel(o, k) >= 2) { milestone(s, MI_HOBBY); break; }
        }
        if (o.stage == ST_TEEN && wd == 4 && hour >= 19) milestone(s, MI_DANCE);
        // relationships
        for (const Rel& r : G.rels) {
            if (r.a != s && r.b != s) continue;
            int t = r.a == s ? r.b : r.a; if (!ok(t) || simDead(t)) continue;
            if (r.fr >= 40 && r.status == RS_NONE) milestone(s, MI_FRIEND, t);
            if (r.fr >= 70 && r.status == RS_NONE && o.stage <= ST_TEEN) milestone(s, MI_BESTIE, t);
            if (r.ro >= 30 && o.stage >= ST_PRETEEN) milestone(s, MI_CRUSH, t);
            if (r.status >= RS_DATING && r.status <= RS_MARRIED) milestone(s, MI_RELATIONSHIP, t);
            if (r.status == RS_MARRIED) milestone(s, MI_MARRIED, t);
        }
        if (s == G.mc && G.datesHad > 0) milestone(s, MI_DATE, partnerOf(s));
        if (o.career != CR_NONE) milestone(s, MI_JOB);
        if (o.career != CR_NONE && o.careerLevel >= 1) milestone(s, MI_PROMO);
        if (o.career != CR_NONE && o.careerLevel >= 4) milestone(s, MI_CAREERPEAK);
        if (o.isAdult() && G.house == 1) milestone(s, MI_MOVEOUT);
        for (const Lineage& l : G.lineage) if ((l.pa == s || l.pb == s) && ok(l.kid)) { milestone(s, MI_PARENT, l.kid); break; }
    }
    // reconciliation: estranged relatives who patched things up
    for (size_t k = 0; k < estranged.size();) {
        const Rel* r = findRel(estranged[k].first, estranged[k].second);
        if (r && r->fr >= 0) {
            int a = estranged[k].first, b = estranged[k].second;
            if (G.sims[a].household || G.sims[b].household || a == G.mc || b == G.mc) {
                addMemoryOnce(MEM_FAMILY, a, b, G.sims[a].name + " and " + G.sims[b].name + " finally made up.");
                toast(G.sims[a].name + " and " + G.sims[b].name + " reconciled! Family is family.");
                addMoodlet(G.sims[a], "Reconciled", EM_HAPPY, 2, 600); addMoodlet(G.sims[b], "Reconciled", EM_HAPPY, 2, 600);
            }
            estranged.erase(estranged.begin() + k);
        } else k++;
    }
    if (hour == 0) for (const Rel& r : G.rels) if (r.fr <= -30 && ok(r.a) && ok(r.b) && !familyRole(r.a, r.b).empty()) {
        std::pair<int, int> p(r.a, r.b);
        if (std::find(estranged.begin(), estranged.end(), p) == estranged.end() && estranged.size() < 64) estranged.push_back(p);
    }
}

void lifeStoryAfterSocial(int t) {
    if (!ok(t) || simDead(t)) return;
    std::string role = familyRole(G.mc, t);
    if (role.empty()) return;
    changeRel(G.mc, t, 1, 0);   // family bonds a bit faster
    if (irand(5) == 0) {
        std::string my = familyRole(t, G.mc);
        static const char* LINE[4] = {"Love you, %s!", "Hey %s! Missed you.", "You're my favorite %s, don't tell anyone.", "Family dinner soon, %s?"};
        char b[96]; std::string low = my; for (auto& ch : low) ch = (char)tolower(ch);
        snprintf(b, sizeof b, LINE[irand(4)], low.c_str()); say(t, b);
        if (irand(3) == 0) addMemoryOnce(MEM_FAMILY, G.mc, t, "Quality time with my " + low + " " + G.sims[t].name + ".");
    }
}

void lifeStoryAdopted(int kid) {
    if (!ok(kid)) return;
    Lineage L; L.kid = (uint16_t)kid; L.pa = (int16_t)G.mc; int p = partnerOf(G.mc); L.pb = (int16_t)(ok(p) ? p : -1);
    G.lineage.push_back(L); G.adopted.push_back((uint16_t)kid);
    milestone(G.mc, MI_PARENT, kid); if (ok(p)) milestone(p, MI_PARENT, kid);
}

// ================================================================ family roles
std::string familyRole(int a, int b) {
    if (!ok(a) || !ok(b) || a == b) return "";
    // direct line
    if (isParent(b, a)) return adoptedKid(a) ? g2(b, "Adoptive Mother", "Adoptive Father") : g2(b, "Mother", "Father");
    if (isParent(a, b)) return adoptedKid(b) ? g2(b, "Adopted Daughter", "Adopted Son") : g2(b, "Daughter", "Son");
    if (married(a, b)) return g2(b, "Wife", "Husband");
    int pa, pb; parentsOf(a, pa, pb);
    if (sibling(a, b)) return sharedParents(a, b) >= 2 || (pb < 0) ? g2(b, "Sister", "Brother") : g2(b, "Half-sister", "Half-brother");
    for (int p : {pa, pb}) if (p >= 0 && isParent(b, p)) return g2(b, "Grandmother", "Grandfather");
    { int ka, kb; parentsOf(b, ka, kb); for (int p : {ka, kb}) if (p >= 0 && isParent(a, p)) return g2(b, "Granddaughter", "Grandson"); }
    for (int p : {pa, pb}) if (p >= 0 && sibling(p, b)) return g2(b, "Aunt", "Uncle");
    { int ka, kb; parentsOf(b, ka, kb); for (int p : {ka, kb}) if (p >= 0 && sibling(p, a)) return g2(b, "Niece", "Nephew"); }
    { int qa, qb; parentsOf(b, qa, qb); for (int p : {pa, pb}) for (int q : {qa, qb}) if (p >= 0 && q >= 0 && sibling(p, q)) return "Cousin"; }
    // step family
    for (int p : {pa, pb}) if (p >= 0 && married(p, b)) return g2(b, "Stepmother", "Stepfather");
    { int qa, qb; parentsOf(b, qa, qb); for (int p : {pa, pb}) for (int q : {qa, qb}) if (p >= 0 && q >= 0 && married(p, q) && !sibling(a, b)) return g2(b, "Stepsister", "Stepbrother"); }
    if (a == G.mc && std::find(G.love.exes.begin(), G.love.exes.end(), (uint16_t)b) != G.love.exes.end()) return G.sims[b].isAdult() ? "Ex" : "";
    // the starting household: family with no recorded lineage
    if (isFamily(a, b)) {
        const Sim &A = G.sims[a], &B = G.sims[b];
        if (B.stage == ST_ELDER && A.stage <= ST_TEEN) return g2(b, "Grandmother", "Grandfather");
        if (A.stage == ST_ELDER && B.stage <= ST_TEEN) return g2(b, "Granddaughter", "Grandson");
        if (B.isAdult() && A.stage <= ST_TEEN) return (pa >= 0 || pb >= 0) ? "Guardian" : g2(b, "Mother", "Father");
        if (A.isAdult() && B.stage <= ST_TEEN) return g2(b, "Daughter", "Son");
        return g2(b, "Sister", "Brother");
    }
    return "";
}

std::string familyDynamic(int a, int b) {
    std::string role = familyRole(a, b);
    if (role.empty()) return "";
    const Rel* r = findRel(a, b); int fr = r ? r->fr : 0;
    const bool sib = role.find("ister") != std::string::npos || role.find("rother") != std::string::npos;
    const bool par = role == "Mother" || role == "Father" || role.find("Adoptive") == 0 || role == "Guardian" || role.find("Step") == 0;
    if (fr <= -30) return "Estranged";
    if (sib && fr < 0) return "Rival Siblings";
    if (fr < 0) return "Family Tension";
    if (par) {   // favoritism: are you the kid they're closest to?
        int best = -1, bestFr = -999, kids = 0;
        for (const Lineage& l : G.lineage) if ((l.pa == b || l.pb == b) && ok(l.kid) && !simDead(l.kid)) { kids++; const Rel* q = findRel(b, l.kid); int f = q ? q->fr : 0; if (f > bestFr) { bestFr = f; best = l.kid; } }
        if (kids >= 2 && best == a && fr >= 40) return "Favorite Child";
        return fr >= 40 ? "Supportive Parent" : "Strict Parent";
    }
    if (sib && fr >= 40 && G.sims[b].stage > G.sims[a].stage) return "Protective Sibling";
    if (fr >= 60) return "Close Family";
    if (fr < 15) return "Distant Family";
    return "Family";
}

// ================================================================ deaths (non-graphic)
bool simDead(int s) { return s >= 0 && s < (int)G.deathType.size() && G.deathType[s] != 0; }
int deathKind(int s) { return simDead(s) ? G.deathType[s] : 0; }
const char* deathName(int k) {
    static const char* N[DK_COUNT] = {"", "Old Age", "Illness", "Freak Accident", "Wild Weather", "Spirit Realm", "Hunger", "Kitchen Fire", "Drowning", "Electrocution", "Laughter", "Embarrassment", "Bad Pufferfish", "Meteor", "Frozen", "Heatstroke", "Exhaustion", "Broken Heart", "Murdered"};
    return N[k % DK_COUNT];
}
static void passAway(int s, int kind) {
    if (s >= 0 && s < (int)G.sims.size()) lxNews("In memoriam: " + G.sims[s].name);
    fit();
    if (!ok(s) || simDead(s) || s == G.mc) return;
    guOnDeath(s);
    Sim& d = G.sims[s];
    const bool home = d.household;
    G.deathType[s] = (uint8_t)kind; G.deathDay[s] = G.day;
    G.lastDeath = (int16_t)s; G.lastDeathDay = G.day; G.memorialDone = false;
    d.household = false; d.pregnant = false;
    for (int h = 0; h < 2; h++) for (int r = 0; r < R_COUNT; r++) { auto& v = G.rooms[h][r].actors; for (size_t k = 0; k < v.size();) { if (v[k].kind == 0 && v[k].id == s) v.erase(v.begin() + k); else k++; } }
    static const char* HOW[DK_COUNT] = {"", "peacefully, after a long and happy life", "after a long illness", "in a totally freak llama-float parade mishap", "when a wild glitter storm swept through town", "and crossed into the spirit realm (people swear they still see them at Hollow Oak Manor)",
        "after forgetting to eat for far too long", "in a kitchen fire (the grease fire spread way too fast)", "after a swim that went terribly wrong",
        "after trying to fix the wiring themselves", "after laughing so hard they literally couldn't stop", "of pure, total embarrassment",
        "after a very questionable plate of pufferfish sushi", "when a meteor fell out of the sky. Right on them. What are the odds?!",
        "after staying out in the freezing cold too long", "after getting way too much sun in a brutal heatwave", "from working themselves to total exhaustion",
        "of a broken heart (they were never the same after the breakup)", "- and the whole town is asking who did it"};
    std::string msg = d.name + " passed away " + HOW[kind] + ".";
    // who grieves, and how hard
    bool anyone = home;
    for (size_t j = 0; j < G.sims.size(); j++) {
        int k = (int)j; if (k == s || simDead(k)) continue;
        Sim& o = G.sims[j];
        std::string role = familyRole(k, s);
        const Rel* r = findRel(k, s);
        int power = 0;
        if (!role.empty()) power = (role == "Mother" || role == "Father" || role == "Wife" || role == "Husband" || role == "Daughter" || role == "Son" || role.find("Adopt") == 0) ? 3 : 2;
        else if (r && r->fr >= 40) power = 1;
        if (r && r->fr <= -30) power = power ? 1 : 0;   // estranged: complicated feelings
        if (!power) continue;
        if (o.household || k == G.mc) anyone = true;
        const int mins = 1440 * power;
        if (kind == DK_ACCIDENT || kind == DK_ENVIRON) addMoodlet(o, "In Shock", EM_TENSE, power, (uint16_t)std::min(4000, mins / 2));
        if (kind == DK_SPIRIT) addMoodlet(o, "Spooked & Grieving", EM_UNCOMFORTABLE, 1, 1440);
        if (kind >= DK_HUNGER && kind != DK_HEARTBREAK) addMoodlet(o, "In Shock", EM_TENSE, power, (uint16_t)std::min(4000, mins / 2));
        addMoodlet(o, kind == DK_OLDAGE ? "Missing Them" : "Heartbroken", EM_SAD, power, (uint16_t)std::min(30000, mins * 2));
    }
    addMemory(MEM_FAMILY, s, -1, "In loving memory of " + d.name + " (" + deathName(kind) + ").");
    if (anyone) { toast(msg + " Hold a memorial at the chapel's Flower Altar."); sfxNo(); }
}

void lifeKill(int s, int kind) { passAway(s, kind); }
void lifeStoryDaily() {
    fit();
    const bool storm = irand(100) < 3;
    for (size_t i = 0; i < G.sims.size(); i++) {
        int s = (int)i; Sim& o = G.sims[i];
        if (simDead(s)) continue;
        if (healthDaily(s)) { passAway(s, DK_ILLNESS); continue; }
        if (s == G.mc || !G.aging) continue;
        if (o.stage == ST_ELDER) {
            o.stageDays++;
            if (o.stageDays >= lifespanDays() - 4 + (s * 7) % 9) { passAway(s, DK_OLDAGE); continue; }
        }
        if (o.stage < ST_YA) continue;   // freak exits are grown-ups only
        int r = irand(20000);
        if (r < 6) passAway(s, DK_ACCIDENT);
        else if (storm && r < 30) passAway(s, DK_ENVIRON);
        else if (r >= 19996) { int str = 0; Emotion e = emotionOf(o, str); if (str >= 5 && (e == EM_EMBARRASSED || e == EM_ANGRY || e == EM_PLAYFUL)) passAway(s, DK_SPIRIT); }
        else {   // the rest of the Sims-style ways to go (rare; some need the right situation)
            int str = 0; const Emotion e = emotionOf(o, str); const int season = seasonOf(G.day);
            if (o.needs[N_HUNGER] < 2 && r < 400) passAway(s, DK_HUNGER);
            else if (o.needs[N_ENERGY] < 2 && r < 300) passAway(s, DK_EXHAUST);
            else if (r >= 30 && r < 34 && o.household) passAway(s, DK_FIRE);
            else if (r >= 34 && r < 38) passAway(s, DK_DROWN);
            else if (r >= 38 && r < 41) passAway(s, DK_ELECTRIC);
            else if (r >= 41 && r < 140 && str >= 5 && e == EM_PLAYFUL) passAway(s, DK_LAUGH);
            else if (r >= 140 && r < 240 && str >= 5 && e == EM_EMBARRASSED) passAway(s, DK_EMBARRASS);
            else if (r >= 240 && r < 243) passAway(s, DK_PUFFER);
            else if (r == 243) passAway(s, DK_METEOR);
            else if (r >= 244 && r < 250 && season == 3) passAway(s, DK_FROZEN);
            else if (r >= 250 && r < 256 && season == 1) passAway(s, DK_HEAT);
            else if (r >= 256 && r < 330 && str >= 5 && e == EM_SAD && o.stage >= ST_ADULT) passAway(s, DK_HEARTBREAK);
        }
    }
}

int pendingMemorial() { return (!G.memorialDone && ok(G.lastDeath) && G.day - G.lastDeathDay <= 14) ? G.lastDeath : -1; }
void holdMemorial() {
    int d = pendingMemorial(); if (d < 0) return;
    G.memorialDone = true;
    for (size_t j = 0; j < G.sims.size(); j++) if ((G.sims[j].household || (int)j == G.mc) && !simDead((int)j)) {
        addMoodlet(G.sims[j], "Said Goodbye", EM_FINE, 2, 720);
        for (size_t k = j + 1; k < G.sims.size(); k++) if (G.sims[k].household && !simDead((int)k)) changeRel((int)j, (int)k, 4, 0);
    }
    addMemory(MEM_FAMILY, d, G.mc, "We held a memorial for " + G.sims[d].name + ". Flowers, stories, laughter and tears.");
    toast("A beautiful memorial for " + G.sims[d].name + ". The family feels closer.");
}
std::string seanceContact() {
    std::vector<int> pool; for (size_t i = 0; i < G.sims.size(); i++) if (simDead((int)i)) pool.push_back((int)i);
    if (pool.empty()) return "";
    int g = pool[irand((int)pool.size())];
    changeRel(G.mc, g, 4, 0);   // ghosts keep their relationships
    static const char* L[4] = {"says they're proud of you.", "says to stop leaving dishes in the sink.", "whispers \"I'm still watching your drama. Iconic.\"", "says the afterlife has great lighting."};
    return "The spirit of " + G.sims[g].name + " " + L[irand(4)];
}

// ================================================================ Life Book screen
namespace {
int page = 0, sel = 0, scroll = 0;
std::vector<int> book;   // household + MC + the departed
void rebuild() {
    book.clear(); book.push_back(G.mc);
    for (size_t i = 0; i < G.sims.size(); i++) if ((int)i != G.mc && (G.sims[i].household || simDead((int)i))) book.push_back((int)i);
    if (sel >= (int)book.size()) sel = 0;
}
int cur() { return book.empty() ? G.mc : book[sel % book.size()]; }
std::vector<int> relativesOf(int s) { std::vector<int> v; for (size_t i = 0; i < G.sims.size(); i++) if (!familyRole(s, (int)i).empty()) v.push_back((int)i); return v; }
const Btn B_BACK{4, 208, 70, 28}, B_TAB0{80, 208, 76, 28}, B_TAB1{160, 208, 76, 28}, B_ADOPT{240, 208, 76, 28};
int candIdx = 0;
Btn hhB(int i) { return Btn{4.f + (i % 2) * 158, 36.f + (i / 2) * 42, 154, 38}; }
std::string hhName() { if (!G.hhName.empty()) return G.hhName; const std::string& n = G.sims[G.mc].name; size_t sp = n.rfind(' '); return (sp == std::string::npos ? n : n.substr(sp + 1)) + " Family"; }
std::vector<int> inviteCands() { std::vector<std::pair<int, int>> v; for (size_t i = 0; i < G.sims.size(); i++) { int s = (int)i; if (G.sims[i].household || s == G.mc || simDead(s)) continue; const Rel* r = findRel(G.mc, s); if (r && r->met && r->fr >= 10) v.push_back({-r->fr, s}); } std::sort(v.begin(), v.end()); std::vector<int> o; for (auto& x : v) o.push_back(x.second); return o; }
void leaveHome(int s) { Sim& d = G.sims[s]; d.household = false; for (int h = 0; h < 2; h++) for (int r = 0; r < R_COUNT; r++) { auto& v = G.rooms[h][r].actors; for (size_t k = 0; k < v.size();) { if (v[k].kind == 0 && v[k].id == s) v.erase(v.begin() + k); else k++; } } }
const Btn B_PREV{4, 4, 36, 26}, B_NEXT{280, 4, 36, 26}, B_UP{280, 150, 36, 24}, B_DN{280, 178, 36, 24};
}

void adoptKid();
void lifeBookOpen() { fit(); if (G.hhName.empty()) G.hhName = hhName(); page = 0; sel = 0; scroll = 0; rebuild(); }
bool lifeBookUpdate(const Input& in) {
    if (in.down & KEY_B) return false;
    if (in.down & KEY_L) { sel = (sel + (int)book.size() - 1) % std::max(1, (int)book.size()); scroll = 0; }
    if (in.down & KEY_R) { sel = (sel + 1) % std::max(1, (int)book.size()); scroll = 0; }
    if (!in.touch) return true;
    const touchPosition& tp = in.tp;
    if (B_BACK.in(tp)) { sfxTap(); return false; }
    if (B_TAB0.in(tp)) { page = 0; scroll = 0; sfxTap(); }
    if (B_TAB1.in(tp)) { page = 1; scroll = 0; sfxTap(); }
    if (B_PREV.in(tp)) { sel = (sel + (int)book.size() - 1) % std::max(1, (int)book.size()); scroll = 0; sfxTap(); }
    if (B_NEXT.in(tp)) { sel = (sel + 1) % std::max(1, (int)book.size()); scroll = 0; sfxTap(); }
    if (B_UP.in(tp) && scroll > 0) { scroll--; sfxTap(); }
    if (B_DN.in(tp)) { scroll++; sfxTap(); }
    if (B_ADOPT.in(tp)) { page = 2; scroll = 0; sfxTap(); return true; }
    if (page == 2) {
        const int s = cur(); const bool home = G.sims[s].household || s == G.mc;
        std::vector<int> cands = inviteCands(); if (candIdx >= (int)cands.size()) candIdx = 0;
        if (hhB(0).in(tp)) { std::string n = hhName(); if (askText("Household name", n, 30, false) && !n.empty()) { G.hhName = n; toast("Household renamed: " + n); } sfxTap(); }
        if (hhB(1).in(tp)) {
            if (!home || simDead(s)) { toast("Only living household members can be played."); sfxNo(); }
            else if (s == G.mc) toast("You're already playing as " + G.sims[s].name + ".");
            else if (G.sims[s].stage < ST_CHILD) { toast("Babies and toddlers can't be played - pick someone older."); sfxNo(); }
            else { cancelActivity(); G.mc = s; for (int r = 0; r < R_COUNT; r++) for (const Actor& a : G.rooms[G.house][r].actors) if (a.kind == 0 && a.id == s) G.room = r; toast("Now playing as " + G.sims[s].name + "!"); sfxYay(); rebuild(); }
        }
        if (hhB(2).in(tp) || hhB(3).in(tp)) {
            const bool newHH = hhB(3).in(tp);
            if (s == G.mc || !G.sims[s].household) { toast(s == G.mc ? "Switch to someone else before moving yourself out." : "They don't live here."); sfxNo(); }
            else if (G.sims[s].stage < ST_TEEN) { toast("Kids can't move out on their own!"); sfxNo(); }
            else {
                std::string nm = G.sims[s].name;
                if (newHH) { std::string sur; if (askText("New household's last name", sur, 20, false) && !sur.empty()) { size_t sp = nm.find(' '); nm = (sp == std::string::npos ? nm : nm.substr(0, sp)) + " " + sur; G.sims[s].name = nm; } }
                leaveHome(s); milestone(s, MI_MOVEOUT);
                for (size_t j = 0; j < G.sims.size(); j++) if (G.sims[j].household) addMoodlet(G.sims[j], "Miss Them Already", EM_SAD, 1, 480);
                toast(nm + (newHH ? " started a brand-new household in town!" : " moved out. They'll still visit!")); sfxTap(); rebuild();
            }
        }
        if (hhB(4).in(tp)) { if (!cands.empty()) candIdx = (candIdx + 1) % (int)cands.size(); sfxTap(); }
        if (hhB(5).in(tp)) {
            if (cands.empty()) { toast("Make some friends first, then invite them to move in."); sfxNo(); }
            else if (householdCount() >= 8) { toast("Your house is full! (8 Sims max)"); sfxNo(); }
            else { int c = cands[candIdx]; moveIn(c); changeRel(G.mc, c, 5, 0); toast(G.sims[c].name + " moved in! Welcome to the " + hhName() + "."); sfxYay(); candIdx = 0; rebuild(); }
        }
        if (hhB(6).in(tp)) adoptKid();
        return true;
    }
    return true;
}
void adoptKid() {
    {
        Sim& me = G.sims[G.mc];
        if (!me.isAdult()) { toast("Only grown-ups can adopt."); sfxNo(); }
        else if (householdCount() >= 8) { toast("Your house is full! (8 Sims max)"); sfxNo(); }
        else if (G.money < 500) { toast("Adoption fees are $500."); sfxNo(); }
        else {
            G.money -= 500;
            Sim k = randomSim(irand(2) ? ST_TODDLER : ST_CHILD, -1);
            { const std::string& mn = G.sims[G.mc].name; size_t sp = mn.rfind(' '); std::string first = k.name.substr(0, k.name.find(' ')); if (sp != std::string::npos) k.name = first + mn.substr(sp); }
            G.sims.push_back(k); int idx = (int)G.sims.size() - 1; fit();
            moveIn(idx); rel(G.mc, idx).status = RS_FAMILY; rel(G.mc, idx).met = true; changeRel(G.mc, idx, 30, 0);
            int p = partnerOf(G.mc); if (ok(p)) { rel(p, idx).status = RS_FAMILY; rel(p, idx).met = true; changeRel(p, idx, 25, 0); }
            lifeStoryAdopted(idx);
            toast("Welcome home, " + G.sims[idx].name + "! Your family just grew."); sfxYay(); rebuild();
        }
    }
}

static void nameBox(int s, int viewer, float cx, float y, float t) {
    if (!ok(s)) return;
    bool dead = simDead(s);
    rrect(cx - 34, y, 68, 40, 6, C(UI_INK)); rrect(cx - 33, y + 1, 66, 38, 5, dead ? C(200, 196, 204) : C(255, 248, 252));
    drawSimPortrait(G.sims[s], cx, y + 13, 10, t);
    std::string nm = G.sims[s].name.substr(0, G.sims[s].name.find(' '));
    text(nm.c_str(), cx, y + 23, 0.32f, C(UI_INK), 1);
    std::string role = s == viewer ? "" : familyRole(viewer, s);
    if (!role.empty()) text(role.c_str(), cx, y + 31, 0.26f, C(255, 70, 150), 1);
    if (dead) text("(passed)", cx + 30, y + 1, 0.24f, C(90, 80, 100), 2);
}

void lifeBookDrawTop(float t) {
    if (page == 2) {
        grad(0, 0, TOP_W, TOP_H, C(240, 250, 255), C(238, 226, 255));
        std::string hn = hhName(); textOutline(hn.c_str(), 200, 4, 0.6f, C(255, 255, 255), C(255, 70, 150), 1);
        int items = 0; for (int r = 0; r < R_COUNT; r++) if (roomIsHome(r)) items += (int)G.rooms[G.house][r].items.size();
        int outfits = 0; for (int k = 0; k < NUM_OUTFITS; k++) if (ownsOutfit(k)) outfits++;
        static const char* HT[3] = {"Apartment", "House", "Two-Story Home"};
        char b[160];
        snprintf(b, sizeof b, "Funds $%d   -   %s   -   %d members", (int)G.money, HT[G.houseType[G.house % 2] % 3], householdCount()); text(b, 200, 28, 0.42f, C(UI_INK), 1);
        snprintf(b, sizeof b, "Inventory: %d furniture  -  %d outfits  -  groceries %d  -  %d pets%s%s%s", items, outfits, (int)G.groceries, (int)G.pets.size(), (G.gadgets & 1) ? "  -  e-reader" : "", (G.gadgets & 2) ? "  -  SugarPod" : "", (G.gadgets & 4) ? "  -  laptop" : "");
        { auto L = wrapText(b, 0.34f, 380); for (size_t k = 0; k < L.size() && k < 2; k++) text(L[k].c_str(), 200, 44 + k * 12.f, 0.34f, C(110, 90, 120), 1); }
        int n = 0;
        for (size_t i = 0; i < G.sims.size() && n < 8; i++) {
            int s = (int)i; if (!(G.sims[i].household || s == G.mc) || simDead(s)) continue;
            float x = 10 + (n % 2) * 194.f, y = 74 + (n / 2) * 40.f; n++;
            rrect(x, y, 186, 36, 6, C(s == cur() ? RGB{255, 220, 236} : RGB{255, 255, 255}));
            drawSimPortrait(G.sims[s], x + 18, y + 18, 12, t);
            std::string nm = G.sims[s].name + (s == G.mc ? " (you)" : ""); text(nm.c_str(), x + 36, y + 2, 0.34f, C(UI_INK));
            const Rel* r = s == G.mc ? nullptr : findRel(G.mc, s);
            std::string role = s == G.mc ? STAGE_NAMES[G.sims[s].stage] : familyRole(G.mc, s); if (role.empty()) role = "Roommate";
            snprintf(b, sizeof b, "%s  -  %s%s", role.c_str(), isSick(s) ? "sick" : "healthy", r ? (r->fr >= 60 ? "  -  close" : r->fr < 0 ? "  -  tense" : "") : "");
            text(b, x + 36, y + 18, 0.28f, C(255, 70, 150));
        }
        // other households in town, grouped by last name
        std::vector<std::pair<std::string, int>> fam;
        for (size_t i = 0; i < G.sims.size(); i++) { if (G.sims[i].household || simDead((int)i)) continue; const std::string& nm = G.sims[i].name; size_t sp = nm.rfind(' '); if (sp == std::string::npos) continue;
            std::string sur = nm.substr(sp + 1); bool f = false; for (auto& e : fam) if (e.first == sur) { e.second++; f = true; } if (!f) fam.push_back({sur, 1}); }
        std::sort(fam.begin(), fam.end(), [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) { return a.second > b.second; });
        std::string o = "Other households: "; for (size_t k = 0; k < fam.size() && k < 4; k++) { if (fam[k].second < 2) break; o += "The " + fam[k].first + "s (" + std::to_string(fam[k].second) + ")  "; }
        text(o.c_str(), 200, 226, 0.3f, C(130, 110, 140), 1);
        return;
    }
    grad(0, 0, TOP_W, TOP_H, C(255, 240, 248), C(238, 226, 255));
    int s = cur(); const Sim& o = G.sims[s];
    char b[128];
    snprintf(b, sizeof b, "%s's Life Book", o.name.c_str());
    textOutline(b, 200, 4, 0.6f, C(255, 255, 255), C(255, 70, 150), 1);
    if (simDead(s)) { snprintf(b, sizeof b, "In loving memory  -  %s, %s", deathName(deathKind(s)), dateStr(G.deathDay[s]).c_str()); text(b, 200, 26, 0.38f, C(110, 90, 120), 1); }
    else { snprintf(b, sizeof b, "%s  -  %s", STAGE_NAMES[o.stage], healthStatus(s).c_str()); text(b, 200, 26, 0.38f, C(110, 90, 120), 1); }
    if (page == 0) {
        std::vector<const MilestoneRec*> mine; for (const MilestoneRec& m : G.milestones) if (m.sim == s) mine.push_back(&m);
        snprintf(b, sizeof b, "Milestones: %d / %d", (int)mine.size(), (int)MI_COUNT); text(b, 200, 42, 0.42f, C(UI_INK), 1);
        if (mine.empty()) text("No milestones yet. Life is just getting started!", 200, 110, 0.42f, C(140, 120, 140), 1);
        int first = std::min(scroll * 2, std::max(0, (int)mine.size() - 1));
        for (int k = 0; k < 6 && first + k < (int)mine.size(); k++) {
            const MilestoneRec& m = *mine[first + k]; float y = 62 + k * 29.f;
            rrect(14, y, 372, 26, 6, C(255, 255, 255, 220));
            circle(30, y + 13, 9, C(MS[m.id].col)); heart(30, y + 13, 4, C(255, 255, 255));
            std::string line = std::string(MS[m.id].name) + (ok(m.other) ? "  with " + G.sims[m.other].name : "");
            text(line.c_str(), 46, y + 1, 0.4f, C(UI_INK));
            text(MS[m.id].desc, 46, y + 13, 0.3f, C(130, 110, 130));
            text(dateStr(m.day).c_str(), 382, y + 1, 0.3f, C(150, 120, 150), 2);
        }
    } else {   // family tree around this Sim
        int pa, pb; parentsOf(s, pa, pb);
        std::vector<int> gp; for (int p : {pa, pb}) if (p >= 0) { int a, c; parentsOf(p, a, c); if (a >= 0) gp.push_back(a); if (c >= 0) gp.push_back(c); }
        std::vector<int> sibs, kids; int spouse = -1;
        for (size_t i = 0; i < G.sims.size(); i++) { int k = (int)i; if (k == s) continue; std::string r = familyRole(s, k);
            if (r.find("ister") != std::string::npos || r.find("rother") != std::string::npos) sibs.push_back(k);
            else if (r == "Daughter" || r == "Son" || r.find("Adopted") == 0) kids.push_back(k);
            else if (r == "Wife" || r == "Husband") spouse = k;
            else if (pa < 0 && pb < 0 && (r == "Mother" || r == "Father" || r == "Guardian")) { if (pa < 0) pa = k; else if (pb < 0) pb = k; } }
        auto row = [&](const std::vector<int>& v, float y) { int n = std::min(5, (int)v.size()); for (int k = 0; k < n; k++) nameBox(v[k], s, 200 - (n - 1) * 38.f + k * 76.f, y, t); };
        text("Grandparents", 8, 42, 0.3f, C(150, 120, 150)); row(gp, 40);
        std::vector<int> par; if (pa >= 0) par.push_back(pa); if (pb >= 0) par.push_back(pb);
        text("Parents", 8, 90, 0.3f, C(150, 120, 150)); row(par, 86);
        std::vector<int> mid = sibs; mid.insert(mid.begin(), s); if (spouse >= 0) mid.insert(mid.begin() + 1, spouse);
        if (mid.size() > 5) mid.resize(5);
        row(mid, 134);
        text("Kids", 8, 186, 0.3f, C(150, 120, 150)); row(kids, 182);
        if (!par.empty()) line(200, 126, 200, 134, 1.5f, C(200, 150, 190));
        if (!kids.empty()) line(200, 174, 200, 182, 1.5f, C(200, 150, 190));
    }
}

void lifeBookDrawBottom(float t) {
    rect(0, 0, BOT_W, BOT_H, C(255, 246, 250));
    int s = cur();
    button(B_PREV, "<", false, 2, 0.5f); button(B_NEXT, ">", false, 2, 0.5f);
    text(G.sims[s].name.c_str(), 160, 8, 0.5f, C(UI_INK), 1);
    if (page == 2) {
        const bool home = G.sims[s].household || s == G.mc;
        std::vector<int> cands = inviteCands(); if (candIdx >= (int)cands.size()) candIdx = 0;
        std::string first = G.sims[s].name.substr(0, G.sims[s].name.find(' '));
        button(hhB(0), "Rename Household", false, 2, 0.36f);
        button(hhB(1), ("Play as " + first).c_str(), false, home ? 0 : 1, 0.36f);
        button(hhB(2), ("Move " + first + " Out").c_str(), false, home && s != G.mc ? 0 : 1, 0.36f);
        button(hhB(3), ("New Household: " + first).c_str(), false, home && s != G.mc ? 0 : 1, 0.32f);
        button(hhB(4), cands.empty() ? "Invite: (no friends)" : ("Invite: " + G.sims[cands[candIdx]].name + " >").c_str(), false, 0, 0.3f);
        button(hhB(5), "Move Them In", false, 2, 0.36f);
        button(hhB(6), "Adopt a Child ($500)", false, 3, 0.34f);
        text("Use < > to pick a member", 238, 168, 0.3f, C(140, 120, 150), 1);
    } else if (page == 0) {
        text("Every first, every big moment - saved forever.", 160, 40, 0.36f, C(130, 110, 130), 1);
        int n = 0; for (const MilestoneRec& m : G.milestones) if (m.sim == s) n++;
        char b[64]; snprintf(b, sizeof b, "%d milestones recorded", n); text(b, 160, 60, 0.45f, C(255, 70, 150), 1);
        int d = pendingMemorial(); if (d >= 0) { snprintf(b, sizeof b, "Memorial pending for %s", G.sims[d].name.c_str()); text(b, 160, 90, 0.38f, C(110, 90, 120), 1); text("(Chapel > Flower Altar)", 160, 106, 0.32f, C(140, 120, 140), 1); }
    } else {
        std::vector<int> rels = relativesOf(s);
        if (rels.empty()) text("No relatives yet.", 160, 80, 0.42f, C(140, 120, 140), 1);
        int first = std::min(scroll * 5, std::max(0, (int)rels.size() - 1));
        for (int k = 0; k < 5 && first + k < (int)rels.size(); k++) {
            int r = rels[first + k]; float y = 36 + k * 30.f;
            rrect(4, y, 270, 27, 5, C(simDead(r) ? RGB{220, 216, 224} : RGB{255, 255, 255}));
            std::string a = G.sims[r].name + (simDead(r) ? " (passed)" : "");
            text(a.c_str(), 10, y + 1, 0.36f, C(UI_INK));
            std::string d = familyRole(s, r) + "  -  " + familyDynamic(s, r);
            text(d.c_str(), 10, y + 14, 0.3f, C(255, 70, 150));
        }
    }
    if (page != 2) { button(B_UP, "^", false, 0, 0.45f); button(B_DN, "v", false, 0, 0.45f); }
    button(B_BACK, "Back", false, 1, 0.42f);
    button(B_TAB0, "Milestones", page == 0, 0, 0.34f);
    button(B_TAB1, "Family Tree", page == 1, 0, 0.34f);
    button(B_ADOPT, "Household", page == 2, 3, 0.34f);
    (void)t;
}
