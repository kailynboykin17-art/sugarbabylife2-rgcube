#include "settings2.hpp"
float guAutonomy();
#include "famdrama.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include "lifeext.hpp"
#include "tween.hpp"
#include <string>
#include <algorithm>
#include <cstdio>
#include "vfs.hpp"
#define fopen vfopen

static bool kid(const Sim& s) { return s.stage >= ST_CHILD && s.stage <= ST_TEEN; }
static bool grown(const Sim& s) { return s.stage >= ST_ADULT; }
static std::string first(int s) { const std::string& n = G.sims[s].name; return n.substr(0, n.find(' ')); }
static bool sibling(int a, int b) { const Sim &A = G.sims[a], &B = G.sims[b]; return isFamily(a, b) && A.stage >= ST_CHILD && B.stage >= ST_CHILD && A.stage <= ST_YA && B.stage <= ST_YA; }
static uint8_t snuck[256];   // teens who snuck out last night (index < 256); RAM only
static int dramaCool = 0;

void famOptions(int t, std::vector<SocialOpt>& v) {
    if (t < 0 || t >= (int)G.sims.size() || !isFamily(G.mc, t)) return;
    const Sim &me = MC(), &s = G.sims[t];
    v.push_back({SO_FAMHUG, "Family Hug", SC_FAMILY});
    v.push_back({SO_FAMTALK, "Catch Up", SC_FAMILY});
    v.push_back({SO_FAMTEA, "Spill Family Tea", SC_FAMILY});
    { const int d = famDynamic(G.mc, t); if (d == DY_CLOSE) v.push_back({SO_FAMTALK, "Deep Talk (Close)", SC_FAMILY}); if (d == DY_JOKESTERS) v.push_back({SO_SIBGAME, "Joke Around (Jokesters)", SC_FUNNY}); }
    if (grown(me) && kid(s)) {   // parent -> kid
        v.push_back({SO_PROUD, "Tell Them You're Proud", SC_FAMILY});
        v.push_back({SO_LECTURE, "Give a Lecture", SC_FAMILY});
        if (s.stage == ST_TEEN) { v.push_back({SO_SETCURFEW, "Set a Curfew", SC_FAMILY}); v.push_back({SO_CHECKPHONE, "Check Their Phone", SC_FAMILY}); v.push_back({SO_TAKEPHONE, "Take Their Phone", SC_FAMILY});
            if (t < 256 && snuck[t]) v.push_back({SO_CONFRONTSNEAK, "Confront: You Snuck Out!", SC_FAMILY}); }
    }
    if (kid(me) && grown(s)) {   // kid -> parent
        v.push_back({SO_ASKMONEY, "Ask for Money", SC_FAMILY});
        v.push_back({SO_ASKPERMISSION, "Ask to Go Out", SC_FAMILY});
        v.push_back({SO_SAYSORRY, "Apologize", SC_FAMILY});
        if (me.stage == ST_TEEN) v.push_back({SO_TALKBACK, "Talk Back", SC_MEAN});
    }
    if (sibling(G.mc, t)) {
        v.push_back({SO_SIBGAME, "Play a Sibling Game", SC_FAMILY});
        v.push_back({SO_SIBSECRET, "Share a Sibling Secret", SC_FAMILY});
        v.push_back({SO_SIBCOVER, "Cover for Them", SC_FAMILY});
        v.push_back({SO_SIBTEASE, "Tease", SC_MEAN});
        v.push_back({SO_SIBFIGHT, "Fight Over the Remote", SC_MEAN});
        v.push_back({SO_TATTLE, "Tattle on Them", SC_MEAN});
    }
}

bool famDoSocial(int id, int t) {
    if (id < SO_FAMHUG || id > SO_FAMTEA || t < 0 || t >= (int)G.sims.size()) return false;
    Sim &me = MC(), &s = G.sims[t]; const std::string n = first(t);
    auto say = [&](const std::string& line) { toast(line); };
    switch (id) {
    case SO_FAMHUG: changeRel(G.mc, t, 6, 0); addMoodlet(me, "Family Hug", EM_HAPPY, 1, 240); addMoodlet(s, "Family Hug", EM_HAPPY, 1, 240); tweenPlay(G.mc, TW_LEANR); tweenPlay(t, TW_LEANL); say("A big warm family hug."); break;
    case SO_FAMTALK: changeRel(G.mc, t, 4, 0); say(n + " fills you in on everything going on."); break;
    case SO_FAMTEA: changeRel(G.mc, t, 3, 0); addMoodlet(me, "Family Tea", EM_PLAYFUL, 1, 240); say(irand(2) ? n + ": \"Don't tell Mom, but Auntie's dating again...\"" : n + ": \"Grandma said WHAT at Thanksgiving?!\""); break;
    case SO_PROUD: changeRel(G.mc, t, 8, 0); addMoodlet(s, "My Parent Is Proud of Me", EM_CONFIDENT, 2, 1440); tweenPlay(t, TW_HOP); say(n + " lights up."); break;
    case SO_LECTURE:
        if (s.stage == ST_TEEN && irand(2)) { changeRel(G.mc, t, -4, 0); addMoodlet(s, "Lectured", EM_ANGRY, 1, 360); tweenPlay(t, TW_SHAKE); say(n + " rolls their eyes so hard you can hear it."); }
        else { changeRel(G.mc, t, -1, 0); s.gradePts = std::min(100.f, s.gradePts + 3); say(n + ": \"...okay, okay. I hear you.\""); }
        break;
    case SO_SETCURFEW: G.curfew = (uint8_t)(G.curfew > 20 ? 21 : 22); changeRel(G.mc, t, -2, 0); say("Curfew is " + std::to_string(G.curfew) + ":00. " + n + ": \"That's SO unfair.\""); break;
    case SO_CHECKPHONE:
        if (irand(100) < 40) { say("You found texts planning to sneak out tonight!"); if (t < 256) snuck[t] = 1; changeRel(G.mc, t, -6, 0); addMoodlet(s, "Privacy Invaded", EM_ANGRY, 2, 720); }
        else { say("Just memes and group chats. " + n + " is furious you looked."); changeRel(G.mc, t, -8, 0); addMoodlet(s, "Privacy Invaded", EM_ANGRY, 2, 720); }
        break;
    case SO_TAKEPHONE: changeRel(G.mc, t, -6, 0); addMoodlet(s, "Phone Taken Away", EM_ANGRY, 2, 1440); s.needs[N_SOCIAL] = std::max(0.f, s.needs[N_SOCIAL] - 30); say(n + ": \"I'm literally going to die.\""); break;
    case SO_CONFRONTSNEAK:
        if (t < 256) snuck[t] = 0;
        if (irand(100) < 50) { say(n + " confesses and apologizes. Grounded for 3 days."); changeRel(G.mc, t, -2, 0); }
        else { say(n + " lies... badly. Grounded for a week!"); changeRel(G.mc, t, -6, 0); addMoodlet(s, "Grounded", EM_ANGRY, 2, 2880); }
        if (t == G.mc) G.mcGrounded = 3;
        break;
    case SO_ASKMONEY:
        if (irand(100) < 45 + rel(G.mc, t).fr / 3) { const int amt = 10 + irand(30); G.money += amt; say(n + " hands you $" + std::to_string(amt) + ". \"Don't spend it all at once.\""); }
        else say(n + ": \"Money doesn't grow on trees!\"");
        break;
    case SO_ASKPERMISSION:
        if (G.mcGrounded) say(n + ": \"You're grounded. Nice try.\"");
        else if (irand(100) < 55 + rel(G.mc, t).fr / 4) { say(n + ": \"Fine, but be home by curfew.\""); addMoodlet(me, "Got Permission", EM_HAPPY, 1, 360); }
        else { say(n + ": \"Not tonight.\""); addMoodlet(me, "So Unfair", EM_ANGRY, 1, 360); }
        break;
    case SO_SAYSORRY: changeRel(G.mc, t, 7, 0); say(n + " softens. \"Thank you for saying that.\""); break;
    case SO_TALKBACK: changeRel(G.mc, t, -7, 0); addMoodlet(me, "Mouthed Off", EM_ANGRY, 1, 360); tweenPlay(t, TW_SHAKE); if (irand(3) == 0) { G.mcGrounded = 2; say(n + ": \"That's it - you're grounded!\""); } else say(n + ": \"Watch your tone.\""); break;
    case SO_SIBGAME: changeRel(G.mc, t, 6, 0); me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 15); tweenPlay(G.mc, TW_CHEER); tweenPlay(t, TW_CHEER); say("Sibling game night - you're both cracking up."); break;
    case SO_SIBSECRET: changeRel(G.mc, t, 7, 0); say(n + " promises to take it to the grave."); break;
    case SO_SIBCOVER: changeRel(G.mc, t, 9, 0); addMoodlet(s, "My Sibling Has My Back", EM_HAPPY, 2, 720); if (t < 256) snuck[t] = 0; say("You covered for " + n + ". They owe you big."); break;
    case SO_SIBTEASE: changeRel(G.mc, t, irand(2) ? -3 : 1, 0); tweenPlay(t, TW_SHAKE); say(irand(2) ? n + ": \"MOM! They're doing it again!\"" : n + " laughs it off."); break;
    case SO_SIBFIGHT: {
        changeRel(G.mc, t, -6, 0); tweenPlay(G.mc, TW_SHAKE); tweenPlay(t, TW_SHAKE);
        int parent = -1; for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household && grown(G.sims[i]) && isFamily(i, t) && i != G.mc) { parent = i; break; }
        if (parent >= 0 && irand(2)) say(first(parent) + ": \"BOTH of you, to your rooms!\""); else say("You and " + n + " scream over the remote. Nobody wins.");
        addMoodlet(me, "Sibling Fight", EM_ANGRY, 1, 360); addMoodlet(s, "Sibling Fight", EM_ANGRY, 1, 360); break; }
    case SO_TATTLE: changeRel(G.mc, t, -10, 0); say("You told on " + n + ". They will NOT forget this."); addMoodlet(s, "Betrayed by Sibling", EM_ANGRY, 2, 1440); break;
    default: return false;
    }
    return true;
}

void famHourly(int hour) {
    if (G.sims.empty() || guAutonomy() == 0 || !gsOn(GS_DRAMA)) return;
    // teens sneaking out after curfew
    if (hour == 1) for (int i = 0; i < (int)G.sims.size() && i < 256; i++) {
        Sim& s = G.sims[i]; if (!s.household || s.stage != ST_TEEN || i == G.mc) continue;
        int bad = 0; for (const Rel& r : G.rels) if ((r.a == i || r.b == i)) { const int o = r.a == i ? r.b : r.a; if (o >= 0 && o < (int)G.sims.size() && G.sims[o].household && grown(G.sims[o])) bad += r.fr < 30; }
        if (irand(100) < 6 + bad * 6 + (s.hasTrait(TR_DRAMATIC) ? 6 : 0)) { snuck[i] = 1; addMoodlet(s, "Snuck Out Last Night", EM_PLAYFUL, 1, 720); s.needs[N_ENERGY] = std::max(0.f, s.needs[N_ENERGY] - 30);
            if (irand(3) == 0) toast("You hear the window creak... " + first(i) + " just snuck out!"); }
    }
    // dinner-table drama
    if (hour == 18 && roomIsHome(G.room) && dramaCool <= 0 && irand(100) < 12) {
        std::vector<int> hh; for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household && G.sims[i].stage >= ST_CHILD) hh.push_back(i);
        if (hh.size() >= 2) {
            const int a = hh[irand((int)hh.size())]; int b = a; while (b == a) b = hh[irand((int)hh.size())];
            static const char* D[6] = {"argue about who ate the last leftovers", "fight over chores - again", "bring up something from YEARS ago", "get into it about grades",
                                       "clash over curfew", "make up and laugh about it by dessert"};
            const int k = irand(6); toast("Family drama at dinner: " + first(a) + " and " + first(b) + " " + D[k] + "!");
            changeRel(a, b, k == 5 ? 4 : -4, 0); tweenPlay(a, k == 5 ? TW_CHEER : TW_SHAKE); tweenPlay(b, k == 5 ? TW_CHEER : TW_SHAKE);
            dramaCool = 3;
        }
    }
}

static void dynDaily();
void famDaily() {
    dynDaily();
    if (guAutonomy() == 0 || !gsOn(GS_DRAMA)) return;
    if (dramaCool > 0) dramaCool--;
    // siblings bicker (and make up) on their own
    for (int i = 0; i < (int)G.sims.size(); i++) for (int j = i + 1; j < (int)G.sims.size(); j++) {
        if (!G.sims[i].household || !G.sims[j].household || !sibling(i, j) || irand(100) >= 8) continue;
        if (irand(3)) { changeRel(i, j, -3, 0); if (i == G.mc || j == G.mc) toast(first(i) + " and " + first(j) + " are fighting over the bathroom again."); }
        else changeRel(i, j, 4, 0);
    }
}

// ======================================================================= family dynamics
const char* DYN_NAMES[DY_COUNT] = {"None", "Close", "Distant", "Supportive", "Permissive", "Difficult", "Strict", "Jokesters"};
namespace {
struct DynPair { uint16_t a, b; uint8_t d, pad; };
std::vector<DynPair> DYN;
int dynPick = -1;   // the pair being edited
bool ageGap(int a, int b) { const Sim &A = G.sims[a], &B = G.sims[b]; return (grown(A) && kid(B)) || (grown(B) && kid(A)) || (A.stage >= ST_YA && B.stage <= ST_TODDLER) || (B.stage >= ST_YA && A.stage <= ST_TODDLER); }
int older(int a, int b) { return G.sims[a].stage >= G.sims[b].stage ? a : b; }
bool pairOk(int a, int b) { if (a == b || a < 0 || b < 0 || a >= (int)G.sims.size() || b >= (int)G.sims.size()) return false; const Rel& r = rel(a, b); return isFamily(a, b) || (r.status >= RS_DATING && r.status <= RS_MARRIED); }
std::vector<std::pair<int, int>> pairs() {
    std::vector<std::pair<int, int>> v;
    for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household) for (int j = i + 1; j < (int)G.sims.size(); j++) if (G.sims[j].household && pairOk(i, j) && G.sims[i].stage >= ST_TODDLER && G.sims[j].stage >= ST_TODDLER) v.push_back({i, j});
    return v;
}
}
int famDynamic(int a, int b) { if (a > b) std::swap(a, b); for (const auto& p : DYN) if (p.a == a && p.b == b) return p.d; return DY_NONE; }
void famSetDynamic(int a, int b, int d) {
    if (a > b) std::swap(a, b);
    for (auto& p : DYN) if (p.a == a && p.b == b) { p.d = (uint8_t)d; return; }
    if (d != DY_NONE) DYN.push_back({(uint16_t)a, (uint16_t)b, (uint8_t)d, 0});
}
std::vector<std::pair<std::string, int>> famDynMenu(int id) {
    std::vector<std::pair<std::string, int>> m; auto P = pairs();
    if (id == 22000) {
        for (size_t k = 0; k < P.size() && k < 400; k++) { const int a = P[k].first, b = P[k].second;
            m.push_back({first(a) + " & " + first(b) + ": " + DYN_NAMES[famDynamic(a, b)], 22100 + (int)k}); }
        if (m.empty()) m.push_back({"(No family in your household yet)", 22999});
        return m;
    }
    if (id >= 22100 && id < 22500) {
        const int k = id - 22100; if (k >= (int)P.size()) return m; dynPick = k; const int a = P[k].first, b = P[k].second;
        m.push_back({first(a) + " & " + first(b) + (ageGap(a, b) ? " (caregiver & child)" : ""), 22999});
        static const int SIB[] = {DY_NONE, DY_CLOSE, DY_DISTANT, DY_DIFFICULT, DY_JOKESTERS};
        static const int GAP[] = {DY_NONE, DY_SUPPORTIVE, DY_PERMISSIVE, DY_STRICT, DY_CLOSE, DY_DISTANT, DY_DIFFICULT};
        if (ageGap(a, b)) { for (int d : GAP) m.push_back({DYN_NAMES[d], 22600 + d}); } else for (int d : SIB) m.push_back({DYN_NAMES[d], 22600 + d});
        return m;
    }
    if (id >= 22600 && id < 22600 + DY_COUNT && dynPick >= 0 && dynPick < (int)P.size()) {
        const int a = P[dynPick].first, b = P[dynPick].second; famSetDynamic(a, b, id - 22600);
        toast(first(a) + " & " + first(b) + " are now " + DYN_NAMES[id - 22600] + "."); return famDynMenu(22000);
    }
    return m;
}

static void dynDaily() {
    for (const auto& pr : pairs()) {
        const int a = pr.first, b = pr.second, d = famDynamic(a, b); Rel& r = rel(a, b);
        const int o = older(a, b), y = o == a ? b : a; const bool mcIn = a == G.mc || b == G.mc;
        auto say = [&](const std::string& t) { if (mcIn || irand(3) == 0) toast(t); };
        switch (d) {
        case DY_CLOSE: changeRel(a, b, 2, 0); if (irand(5) == 0) { addMoodlet(G.sims[a], "Deep Talk", EM_HAPPY, 1, 600); addMoodlet(G.sims[b], "Deep Talk", EM_HAPPY, 1, 600); say(first(a) + " and " + first(b) + " stayed up having a deep talk."); } break;
        case DY_DISTANT: if (irand(3) == 0) changeRel(a, b, -1, 0); if (irand(8) == 0) say(first(a) + " and " + first(b) + " barely said a word to each other today. Just small talk."); break;
        case DY_SUPPORTIVE: addMoodlet(G.sims[y], "Feeling Supported", EM_CONFIDENT, 1, 720); if (irand(5) == 0) say(first(o) + " hyped " + first(y) + " up. \"I'm so proud of you.\""); changeRel(a, b, 1, 0); break;
        case DY_PERMISSIVE: if (irand(4) == 0) { if (G.money > 15) G.money -= 15; say(first(y) + " demanded snacks and $15. " + first(o) + " just... said yes."); } break;
        case DY_DIFFICULT: changeRel(a, b, -2, 0); if (irand(3) == 0) { addMoodlet(G.sims[a], "Family Tension", EM_ANGRY, 1, 480); addMoodlet(G.sims[b], "Family Tension", EM_ANGRY, 1, 480); static const char* W[4] = {"the dishes", "the thermostat", "who's annoying who", "literally nothing"}; say(first(a) + " and " + first(b) + " argued about " + W[irand(4)] + "."); } break;
        case DY_STRICT: addMoodlet(G.sims[y], "House Rules", EM_TENSE, 1, 480); G.sims[y].gradePts = std::min(100.f, G.sims[y].gradePts + 1); if (irand(5) == 0) say(first(o) + ": \"Homework first. Then phone.\" " + first(y) + " sighs."); break;
        case DY_JOKESTERS: addMoodlet(G.sims[a], "Family Jokes", EM_PLAYFUL, 1, 480); addMoodlet(G.sims[b], "Family Jokes", EM_PLAYFUL, 1, 480); if (irand(5) == 0) say(first(a) + " and " + first(b) + " can't stop pranking each other."); break;
        }
        // organic shifts: how they keep treating each other changes the dynamic
        if (irand(100) < 6) {
            int nd = -1;
            if (r.fr >= 75 && d != DY_CLOSE && d != DY_JOKESTERS) nd = irand(3) == 0 ? DY_JOKESTERS : DY_CLOSE;
            else if (r.fr <= -15 && d != DY_DIFFICULT) nd = DY_DIFFICULT;
            else if (r.fr > -5 && r.fr < 15 && d == DY_NONE) nd = DY_DISTANT;
            else if (d == DY_DIFFICULT && r.fr >= 40) nd = DY_NONE;
            if (nd >= 0) { famSetDynamic(a, b, nd); toast(nd == DY_NONE ? first(a) + " and " + first(b) + " made peace - things aren't Difficult anymore." : "Family dynamic: " + first(a) + " and " + first(b) + " are now " + DYN_NAMES[nd] + "!"); }
        }
    }
}
void famSaveTo(const std::string& p) { FILE* f = fopen(p.c_str(), "wb"); if (!f) return; uint32_t n = (uint32_t)DYN.size(); fwrite(&n, 4, 1, f); if (n) fwrite(DYN.data(), sizeof(DynPair), n, f); fclose(f); }
void famLoadFrom(const std::string& p) { DYN.clear(); FILE* f = fopen(p.c_str(), "rb"); if (!f) return; uint32_t n = 0; if (fread(&n, 4, 1, f) == 1 && n < 5000) { DYN.resize(n); if (n && fread(DYN.data(), sizeof(DynPair), n, f) != n) DYN.clear(); } fclose(f); }
