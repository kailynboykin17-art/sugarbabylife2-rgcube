#include "settings2.hpp"
#include "townpulse.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "extras.hpp"
#include "emote.hpp"
#include <string>
#include <deque>
#include <vector>
#include <algorithm>
#include <cstdio>
#include "vfs.hpp"
#define fopen vfopen
void lifeMarry(int a, int b);

namespace {
std::deque<std::string> LOG;
std::string first(int s) { const std::string& n = G.sims[s].name; return n.substr(0, n.find(' ')); }
std::string last(int s) { const std::string& n = G.sims[s].name; const size_t p = n.rfind(' '); return p == std::string::npos ? "" : n.substr(p + 1); }
bool townie(int i) { return i >= 0 && i < (int)G.sims.size() && !G.sims[i].household && i != G.mc; }
bool known(int i) { return rel(G.mc, i).met; }
int partnerOf(int i) { for (const Rel& r : G.rels) if ((r.a == i || r.b == i) && r.status >= RS_DATING && r.status <= RS_MARRIED) return r.a == i ? r.b : r.a; return -1; }
int randTownie(bool adult) { for (int t = 0; t < 20; t++) { const int i = irand((int)G.sims.size()); if (townie(i) && (!adult || G.sims[i].stage >= ST_YA)) return i; } return -1; }
void report(const std::string& s, bool newsworthy, int a, int b) {
    LOG.push_front("Day " + std::to_string(G.day) + ": " + s); if (LOG.size() > 30) LOG.pop_back();
    const bool care = (a >= 0 && known(a)) || (b >= 0 && known(b));
    if (newsworthy) lxNews(s);
    else if (care) lxGossip(s, true, CI_PARTY, 0);
    if (care && irand(3) == 0) toast("Town news: " + s);
}

void event() {
    const int roll = irand(100);
    const int a = randTownie(roll >= 35 && roll < 75); if (a < 0) return;
    if (roll < 20) {   // new friends
        const int b = randTownie(false); if (b < 0 || b == a || std::abs((int)G.sims[a].stage - (int)G.sims[b].stage) > 2) return;
        changeRel(a, b, 25, 0); report(first(a) + " and " + first(b) + " are suddenly best friends.", false, a, b);
    } else if (roll < 35) {   // feud
        const int b = randTownie(false); if (b < 0 || b == a || std::abs((int)G.sims[a].stage - (int)G.sims[b].stage) > 2) return;
        changeRel(a, b, -30, 0); static const char* W[4] = {"over a parking spot", "after a group chat leak", "because of a stolen recipe", "over who wore it better"};
        report(first(a) + " and " + first(b) + " are feuding " + W[irand(4)] + ".", false, a, b);
    } else if (roll < 50) {   // new couple
        if (partnerOf(a) >= 0) return; const int b = randTownie(true);
        if (b < 0 || b == a || partnerOf(b) >= 0 || !canRomance(a, b) || isFamily(a, b)) return;
        Rel& r = rel(a, b); r.met = true; r.fr = std::max<int8_t>(r.fr, 40); r.ro = 50; r.status = RS_DATING;
        report(first(a) + " and " + first(b) + " are officially dating!", false, a, b);
    } else if (roll < 60) {   // breakup
        const int b = partnerOf(a); if (b < 0 || !townie(b)) return; Rel& r = rel(a, b); if (r.status == RS_MARRIED) return;
        r.status = RS_NONE; r.ro = -20; r.fr = (int8_t)std::max(-50, r.fr - 30);
        report(first(a) + " and " + first(b) + " broke up. It was MESSY.", false, a, b);
    } else if (roll < 67) {   // engaged -> married
        const int b = partnerOf(a); if (b < 0 || !townie(b)) return; Rel& r = rel(a, b);
        if (r.status == RS_DATING) { r.status = RS_ENGAGED; report(first(a) + " proposed to " + first(b) + " - they said YES!", true, a, b); }
        else if (r.status == RS_ENGAGED) { lifeMarry(a, b); report(first(a) + " and " + first(b) + " got married!", true, a, b); }
    } else if (roll < 72) {   // baby
        const int b = partnerOf(a); if (b < 0 || !townie(b) || rel(a, b).status != RS_MARRIED || G.sims.size() >= 116) return;
        if (G.sims[a].stage > ST_ADULT || G.sims[b].stage > ST_ADULT) return;
        Sim baby = randomSim(ST_INFANT, irand(2)); baby.household = false;
        baby.name = baby.name.substr(0, baby.name.find(' ')) + " " + last(a); baby.bio = first(a) + " and " + first(b) + "'s baby";
        G.sims.push_back(baby); const int k = (int)G.sims.size() - 1;
        rel(a, k).status = RS_FAMILY; rel(a, k).fr = 60; rel(b, k).status = RS_FAMILY; rel(b, k).fr = 60;
        report(first(a) + " and " + first(b) + " welcomed a baby, " + first(k) + "!", true, a, b);
    } else if (roll < 82) {   // career moves
        Sim& s = G.sims[a]; if (s.stage < ST_YA || s.stage >= ST_ELDER) return;
        if (s.career != CR_NONE && irand(3)) { s.careerLevel = (uint8_t)std::min(9, s.careerLevel + 1); report(first(a) + " got promoted!", false, a, -1); }
        else if (s.career != CR_NONE) { s.career = CR_NONE; s.careerLevel = 0; report(first(a) + " quit their job to \"find themselves.\"", false, a, -1); }
        else { joinCareer(s, 1 + irand(CR_COUNT - 1)); report(first(a) + " landed a new job.", false, a, -1); }
    } else if (roll < 92) {   // party
        static const char* P[4] = {"a rooftop party", "a backyard BBQ", "a Y2K-themed birthday", "a game night that got LOUD"};
        const char* p = P[irand(4)];
        if (known(a) && rel(G.mc, a).fr >= 20 && irand(2)) { toast(first(a) + " is throwing " + p + " - and you're invited! (Invites app)"); changeRel(G.mc, a, 2, 0); }
        report(first(a) + " threw " + p + ".", false, a, -1);
    } else {   // makeover / glow-up
        report(first(a) + " got a whole glow-up. New hair, new attitude.", false, a, -1);
    }
}
}

void tpDaily() {
    if (!gsOn(GS_PULSE)) return; const int n = 2 + irand(4); for (int i = 0; i < n; i++) event(); }
LxMenu tpLog() { LxMenu m; for (const auto& s : LOG) m.push_back({s, 20999}); if (m.empty()) m.push_back({"(Quiet day in town... for now)", 20999}); return m; }

void tpSaveTo(const std::string& p) { FILE* f = fopen(p.c_str(), "wb"); if (!f) return; for (const auto& s : LOG) { fputs(s.c_str(), f); fputc('\n', f); } fclose(f); }
void tpLoadFrom(const std::string& p) { LOG.clear(); FILE* f = fopen(p.c_str(), "rb"); if (!f) return; char b[256]; while (fgets(b, sizeof b, f) && LOG.size() < 30) { std::string s(b); while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back(); if (!s.empty()) LOG.push_back(s); } fclose(f); }
