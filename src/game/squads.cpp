#include "squads.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "social.hpp"
#include "extras.hpp"
#include <cmath>
#include <cstdio>

const char* CLIQUE_NAMES[CQ_COUNT] = {"The It Girls", "Varsity Crew", "Art Kids", "Brainiacs", "Alt Kids"};
const char* CLIQUE_VIBE[CQ_COUNT] = {"glam, gossip & designer bags", "sports, cheer & pep rallies", "painting, drama & music",
                                     "grades, robotics & debate", "eyeliner, skate & garage bands"};
const char* CLUB_NAMES[CQ_COUNT] = {"Fashion Club", "Cheer Squad", "Drama Club", "Robotics Club", "Garage Band"};
static const int RIVAL[CQ_COUNT] = {CQ_ALT, CQ_BRAIN, -1, CQ_VARSITY, CQ_IT};

int cliqueOf(int i) {
    if (i < 0 || i >= (int)G.sims.size() || i == G.mc) return -1;
    const Sim& s = G.sims[i];
    if (s.household || s.stage < ST_CHILD) return -1;
    if (s.outfit == 15 || s.outfit == 14) return CQ_ALT;          // Scene Goth, Grunge
    if (s.hasTrait(TR_FASHIONISTA) || s.hasTrait(TR_GLAMOROUS)) return CQ_IT;
    if (s.hasTrait(TR_ACTIVE)) return CQ_VARSITY;
    if (s.hasTrait(TR_CREATIVE)) return CQ_ART;
    if (s.hasTrait(TR_BOOKWORM)) return CQ_BRAIN;
    if (s.hasTrait(TR_HOTHEAD) || s.hasTrait(TR_LONER)) return CQ_ALT;
    return (int)((s.name.size() * 7 + i) % CQ_COUNT);
}

const char* repLabel(int r) {
    return r <= -60 ? "Mean Girl Legend" : r <= -20 ? "Messy" : r < 20 ? "Low-Key" : r < 60 ? "Sweetheart" : "Town Darling";
}
float repBonus() { return G.rep / 500.f; }

static bool isMean(int id) {
    return id == SO_INSULT || id == SO_EYEROLL || id == SO_BACKHANDED || id == SO_MOCKOUTFIT || id == SO_RUMOR ||
           id == SO_COLDSHOULDER || id == SO_SPLASH || id == SO_CATFIGHT || id == SO_CALLOUT;
}
static bool isNice(int id) {
    return id == SO_COMPLIMENT || id == SO_HUG || id == SO_HIGHFIVE || id == SO_SELFIE || id == SO_APOLOGIZE ||
           id == SO_HOMEWORK || id == SO_SPILLTEA || id == SO_VENT || id == SO_SECRET;
}

static void standing(int c, int d) {
    if (c < 0) return;
    G.cliqueStand[c] = (uint8_t)clampv((int)G.cliqueStand[c] + d, 0, 100);
}

void socialRipple(int id, int t) {
    const bool mean = isMean(id), nice = isNice(id);
    // witnesses in the room
    int wit = 0;
    for (const Actor& a : curRoom().actors) {
        if (a.kind != 0 || a.id == G.mc || a.id == t) continue;
        if (!mean) continue;
        wit++;
        const Rel* wr = findRel(a.id, t);
        if (wr && wr->fr <= -10) { changeRel(G.mc, a.id, 2, 0); if (irand(3) == 0) say(a.id, irand(2) ? "LOL she deserved that" : "Oop- the DRAMA"); }
        else { changeRel(G.mc, a.id, -2, 0); if (irand(3) == 0) say(a.id, irand(2) ? "Yikes..." : "That was so not okay."); }
    }
    int before = G.rep;
    if (mean) G.rep -= 3 + std::min(wit, 4);
    else if (id == SO_APOLOGIZE) G.rep += 3;
    else if (nice) G.rep += 1;
    G.rep = (int16_t)clampv((int)G.rep, -100, 100);
    if (repLabel(before) != repLabel(G.rep)) toast(std::string("Your reputation is now: ") + repLabel(G.rep));

    // squads
    int c = cliqueOf(t);
    if (c < 0) return;
    if (mean) {
        standing(c, -6);
        if (G.clique >= 0 && RIVAL[G.clique] == c) standing(G.clique, 2);   // your squad lives for the drama
    } else standing(c, nice ? 3 : 1);
    if (G.clique == c && !mean) changeRel(G.mc, t, 2, 0);                  // squad-mates bond faster
    // an invitation to join
    const Rel* r = findRel(G.mc, t);
    if (G.clique != c && G.cliqueStand[c] >= 60 && r && r->fr >= 30 && G.squadAskDay != G.day && MC().stage >= ST_CHILD) {
        G.squadAskDay = G.day;
        dlgSay(t, std::string("Okay, it's official. We want you in ") + CLIQUE_NAMES[c] + ". Welcome to the squad!");
        if (G.clique >= 0) { standing(G.clique, -20); dlgSay(-1, std::string("You left ") + CLIQUE_NAMES[G.clique] + ". They are NOT happy about it."); }
        G.clique = (int8_t)c;
        addMemoryOnce(MEM_FRIEND, t, G.mc, std::string("Joined ") + CLIQUE_NAMES[c] + "!");
        if (RIVAL[c] >= 0) standing(RIVAL[c], -15);
        addMoodlet(MC(), "Squad Goals", EM_CONFIDENT, 2, 720);
        toast(std::string("You joined ") + CLIQUE_NAMES[c] + "!");
    }
}

void squadGreet(int sim) {
    int c = cliqueOf(sim);
    if (c < 0 || irand(3)) return;
    if (G.clique == c) say(sim, irand(2) ? "Heyyy bestie!!" : "Squad's here!");
    else if (G.clique >= 0 && RIVAL[G.clique] == c) say(sim, irand(2) ? "Ugh, look who it is." : "Oh. It's YOU.");
    else if (G.rep <= -40) say(sim, "*whispers* that's the mean one...");
    else if (G.rep >= 50) say(sim, "Omg hi! You're so sweet!");
}

void npcRelTick() {
    return;   // superseded by the life engine (life.cpp): townies now date, feud and break up based on who they actually spend time with
    if (irand(100) >= 7) return;
    std::vector<int> pool;
    for (size_t i = 0; i < G.sims.size(); i++) if (!G.sims[i].household && G.sims[i].stage >= ST_TEEN && (int)i != G.mc) pool.push_back((int)i);
    if (pool.size() < 3) return;
    int a = pool[irand((int)pool.size())], b = a;
    for (int tries = 0; tries < 12 && b == a; tries++) b = pool[irand((int)pool.size())];
    if (b == a) return;
    Sim& A = G.sims[a];
    Sim& B = G.sims[b];
    bool sameAge = (A.stage == ST_TEEN) == (B.stage == ST_TEEN);
    int poster = pool[irand((int)pool.size())];
    char txt[160];
    Rel& r = rel(a, b);
    if (r.status == RS_DATING && irand(100) < 40) {
        r.status = RS_NONE; r.fr = (int8_t)clampv(r.fr - 30, -100, 100); r.ro = 0;
        snprintf(txt, sizeof txt, "omg %s and %s BROKE UP?!?! not me crying", A.name.c_str(), B.name.c_str());
    } else if (sameAge && r.status == RS_NONE && canRomance(a, b) && partnerOf(a) < 0 && partnerOf(b) < 0 && irand(2)) {
        r.status = RS_DATING; r.fr = 40; r.ro = 60;
        snprintf(txt, sizeof txt, "%s and %s are officially a thing!! <3 cutest couple alert", A.name.c_str(), B.name.c_str());
    } else {
        r.fr = (int8_t)clampv(r.fr - 35, -100, 100);
        snprintf(txt, sizeof txt, "%s just unfollowed %s... the DRAMA", A.name.c_str(), B.name.c_str());
    }
    Post p; p.sim = (int16_t)poster; p.text = txt; p.likes = (uint32_t)(80 + irand(500));
    G.posts.insert(G.posts.begin(), p);
    if (G.posts.size() > 30) G.posts.pop_back();
    const Rel* ma = findRel(G.mc, a);
    const Rel* mb = findRel(G.mc, b);
    if ((ma && ma->met) || (mb && mb->met)) toast("Town tea: check InstaGlam!");
}
