// Clubs & Daily Goals.
#include "clubs.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "goals.hpp"
#include "extras.hpp"
#include "cas.hpp"
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
void travelTo(int room);
void say(int simIdx, const std::string& line);

namespace {
enum Act { A_DANCE, A_SHOP, A_FLIRT, A_STUDY, A_COOK, A_GOSSIP, A_WORKOUT, A_ART, A_TV, A_PARTY, A_COUNT };
const char* ACT_N[A_COUNT] = {"Dancing", "Shopping", "Flirting", "Studying", "Cooking", "Gossiping", "Working Out", "Making Art", "Watching TV", "Partying"};
const char* ICONS[8] = {"heart", "star", "crown", "music", "book", "flower", "cherry", "moon"};
const char* DRESS[6] = {"Anything goes", "All pink", "Y2K", "All black", "Formal", "Pajamas"};
const char* RANK_N[4] = {"Member", "Co-Leader", "Leader", "Rival"};
const int GATHER_ROOMS[8] = {R_CAFE, R_CLUB, R_MALL, R_PARK, R_LIBRARY, R_GYM, R_BEACH, R_LIVING};
const char* PERK_N[4] = {"Club Discount (10% off shops)", "Bestie Bond (+friendship gains)", "Hype Squad (+fun at gatherings)", "Legendary Club (+$50 a day)"};
const int PERK_COST[4] = {60, 120, 200, 400};

struct Club {
    char name[24]; uint8_t icon, req, dress, used;   // req: 0 anyone, 1 teens+, 2 adults, 3 friends only
    uint16_t enc, dis;   // bitmasks of Act
    int16_t mem[10]; uint8_t rank[10]; uint8_t nmem;
    uint16_t points; uint8_t perks; bool mine;
    uint8_t gRoom, gHour, gDress; uint16_t gDay;   // next gathering (gDay 0xFFFF = none)
};
struct ClubMsg { int16_t club, from; uint16_t day; char text[96]; };
struct Daily { uint8_t kind[3], prog[3], need[3], done[3]; uint16_t day; };
struct State { uint32_t magic = 0x434C4231; Club c[6]; uint8_t nc = 0; Daily d{}; uint16_t streak = 0; };
State S;
std::vector<ClubMsg> chat;   // capped, saved

std::string fmt(const char* f, ...) { char b[400]; va_list a; va_start(a, f); vsnprintf(b, sizeof b, f, a); va_end(a); return b; }
std::string nm(int s) { if (s < 0 || s >= (int)G.sims.size()) return "?"; const std::string& n = G.sims[s].name; return n.substr(0, n.find(' ')); }
bool ok(int s) { return s >= 0 && s < (int)G.sims.size(); }
int memIdx(const Club& c, int s) { for (int k = 0; k < c.nmem; k++) if (c.mem[k] == s) return k; return -1; }
bool eligible(const Club& c, int s) {
    if (!ok(s)) return false; const Sim& x = G.sims[s];
    if (x.stage < ST_CHILD) return false;
    if (c.req == 1 && x.stage < ST_TEEN) return false;
    if (c.req == 2 && x.stage < ST_YA) return false;
    if (c.req == 3) { const Rel* r = findRel(G.mc, s); if (!r || r->fr < 30) return false; }
    return true;
}
void post(int club, int from, const std::string& t) {
    ClubMsg m; m.club = (int16_t)club; m.from = (int16_t)from; m.day = G.day; snprintf(m.text, sizeof m.text, "%s", t.c_str());
    chat.push_back(m); if (chat.size() > 60) chat.erase(chat.begin());
}
void addMember(Club& c, int s, uint8_t rank) { if (c.nmem < 10 && memIdx(c, s) < 0 && ok(s)) { c.mem[c.nmem] = (int16_t)s; c.rank[c.nmem] = rank; c.nmem++; } }
void removeMember(Club& c, int k) { for (int j = k; j + 1 < c.nmem; j++) { c.mem[j] = c.mem[j + 1]; c.rank[j] = c.rank[j + 1]; } c.nmem--; }
int clubOfPlayer() { for (int i = 0; i < S.nc; i++) if (S.c[i].mine) return i; return -1; }

// ---- daily goals
enum DK { DK_TALK, DK_GIFT, DK_COOK, DK_TRAVEL, DK_POST, DK_DANCE, DK_WORK, DK_TV, DK_CLUB, DK_COUNT };
const char* DK_N[DK_COUNT] = {"Chat with %d people", "Give %d gift", "Cook %d meal", "Visit %d places in town", "Post %d time on InstaGlam", "Dance %d time", "Work %d shift", "Watch TV %d time", "Do %d club activity"};
const int DK_NEED[DK_COUNT] = {3, 1, 1, 2, 1, 1, 1, 1, 1};
void rollDaily() {
    S.d.day = G.day;
    int used = 0;
    for (int k = 0; k < 3; k++) {
        int pick = 0, tries = 0; do { pick = irand(DK_COUNT); } while (++tries < 64 && (used >> pick & 1 || (pick == DK_WORK && MC().career == CR_NONE) || (pick == DK_CLUB && clubOfPlayer() < 0)));
        used |= 1 << pick; S.d.kind[k] = (uint8_t)pick; S.d.need[k] = (uint8_t)DK_NEED[pick]; S.d.prog[k] = 0; S.d.done[k] = 0;
    }
}
void dailyHit(int kind) {
    if (S.d.day != G.day) rollDaily();
    for (int k = 0; k < 3; k++) if (S.d.kind[k] == kind && !S.d.done[k]) {
        if (++S.d.prog[k] >= S.d.need[k]) {
            S.d.done[k] = 1; G.money += 75; int c = clubOfPlayer(); if (c >= 0) S.c[c].points += 10;
            toast(std::string("Daily goal done: ") + fmt(DK_N[kind], S.d.need[k]) + "! +$75");
            sfxYay();
            if (S.d.done[0] && S.d.done[1] && S.d.done[2]) { S.streak++; G.money += 100 + 25 * std::min<int>(S.streak, 10); toast(fmt("All 3 daily goals! Streak: %d day%s. Bonus cash!", S.streak, S.streak == 1 ? "" : "s")); }
        }
    }
}
// which activity a club action / event counts as
int actOf(int ev, int arg) {
    switch (ev) {
    case GE_DANCE: return A_DANCE; case GE_COOK: return A_COOK; case GE_TV: return A_TV; case GE_READ: case GE_HOMEWORK: return A_STUDY;
    case GE_BUYOUTFIT: return A_SHOP; case GE_DATE: return A_FLIRT;
    case GE_SKILLUP: return arg == SK_FITNESS ? A_WORKOUT : arg == SK_CREATIVITY ? A_ART : -1;
    case GE_TRAVEL: return arg == R_MALL || arg == R_BOUTIQUE || arg == R_THRIFT ? A_SHOP : arg == R_CLUB || arg == R_FRAT ? A_PARTY : arg == R_GYM ? A_WORKOUT : arg == R_LIBRARY ? A_STUDY : -1;
    case GE_SOCIAL: return irand(3) == 0 ? A_GOSSIP : -1;
    }
    return -1;
}

// ---- autonomous town clubs
const char* NPC_CLUBS[5] = {"Glitter Girls", "Varsity Crew", "Book Babes", "Night Owls", "Garden Gang"};
void seedTownClubs() {
    if (S.nc) return;
    for (int i = 0; i < 3 && S.nc < 6; i++) {
        Club& c = S.c[S.nc]; c = Club{}; snprintf(c.name, sizeof c.name, "%s", NPC_CLUBS[i]);
        c.icon = (uint8_t)(i * 3 % 8); c.req = 0; c.used = 1; c.mine = false; c.gDay = 0xFFFF;
        c.enc = (uint16_t)((i == 0 ? (1 << A_SHOP | 1 << A_GOSSIP | 1 << A_DANCE) : i == 1 ? (1 << A_WORKOUT | 1 << A_PARTY) : (1 << A_STUDY | 1 << A_TV)));
        c.dis = (uint16_t)(i == 2 ? 1 << A_PARTY : 1 << A_STUDY);
        std::vector<int> pool; for (size_t s = 0; s < G.sims.size(); s++) if ((int)s != G.mc && !G.sims[s].household && G.sims[s].stage >= ST_TEEN) pool.push_back((int)s);
        for (int k = 0; k < 5 && !pool.empty(); k++) { int j = irand((int)pool.size()); addMember(c, pool[j], k == 0 ? 2 : k == 1 ? 1 : 0); pool.erase(pool.begin() + j); }
        S.nc++;
    }
}
const char* CHAT_PLAN[] = {"Who's free Saturday?? %s at noon, be there", "Ok but we NEED a %s trip this week", "Voting on the next meetup: %s?"};
const char* CHAT_GOSSIP[] = {"Did you guys see what %s wore today", "Not to start drama but %s left the group chat AGAIN", "%s said they're 'too busy' for us lol", "Shoutout to %s for the snacks!!"};
const char* CHAT_FIGHT[] = {"%s you literally said you'd come", "I'm not doing this with you %s", "Ok %s that was rude and you know it"};

// ---- UI
int tab = 0, sel = 0, edit = -1, page = 0;
enum Sub { SB_LIST, SB_CLUB, SB_EDIT, SB_MEMBERS, SB_GATHER, SB_PERKS };
int sub = SB_LIST;
Btn rowB(int k) { return Btn{6, 30 + k * 24.f, 308, 22}; }
Btn tabB(int k) { return Btn{2 + k * 106.f, 2, 104, 22}; }
const Btn bBack{4, 208, 64, 28}, bA{72, 208, 80, 28}, bB{156, 208, 80, 28}, bC{240, 208, 76, 28};
void createClub() {
    if (S.nc >= 6) { toast("Too many clubs in town already!"); sfxNo(); return; }
    if (clubOfPlayer() >= 0) { toast("You already lead a club."); sfxNo(); return; }
    Club& c = S.c[S.nc]; c = Club{}; c.used = 1; c.mine = true; c.gDay = 0xFFFF;
    std::string n = "My Club"; askText("Name your club", n, 22); if (n.empty()) n = "My Club";
    snprintf(c.name, sizeof c.name, "%s", n.c_str());
    addMember(c, G.mc, 2); c.enc = 1 << A_GOSSIP; c.dis = 0;
    edit = S.nc; S.nc++; sub = SB_EDIT; sfxYay();
    post(edit, G.mc, std::string("Welcome to ") + c.name + "!!");
}
}

void clubsOpen(int t) { tab = t; sub = SB_LIST; page = 0; seedTownClubs(); if (S.d.day != G.day) rollDaily(); }

bool clubsUpdate(const Input& in) {
    if (in.down & KEY_B) { if (sub != SB_LIST) { sub = sub == SB_CLUB ? SB_LIST : SB_CLUB; return true; } return false; }
    if (!in.touch) return true;
    const auto tp = in.tp;
    if (sub == SB_LIST) for (int k = 0; k < 3; k++) if (tabB(k).in(tp)) { tab = k; page = 0; sfxTap(); return true; }
    if (bBack.in(tp)) { if (sub == SB_LIST) return false; sub = sub == SB_CLUB ? SB_LIST : SB_CLUB; sfxTap(); return true; }
    if (tab == 2) return true;   // daily goals: read-only
    if (tab == 1) { if (bA.in(tp)) page = std::max(0, page - 1); if (bB.in(tp)) page++; return true; }
    Club* c = edit >= 0 && edit < S.nc ? &S.c[edit] : nullptr;
    switch (sub) {
    case SB_LIST:
        for (int k = 0; k < S.nc && k < 6; k++) if (rowB(k + 1).in(tp)) { edit = k; sub = SB_CLUB; sfxTap(); return true; }
        if (bC.in(tp)) createClub();
        return true;
    case SB_CLUB: {
        if (!c) return true;
        const int me = memIdx(*c, G.mc); const bool lead = me >= 0 && c->rank[me] >= 1 && c->rank[me] <= 2;
        if (rowB(5).in(tp)) {   // join / leave
            if (me >= 0) { if (c->rank[me] == 2 && c->mine) { toast("Leaders can't leave their own club."); return true; } removeMember(*c, me); toast("You left the club."); }
            else if (!eligible(*c, G.mc)) { toast("You don't meet this club's requirements."); sfxNo(); }
            else if (c->nmem >= 10) toast("The club is full.");
            else { addMember(*c, G.mc, 0); post(edit, G.mc, "Hiii I just joined!"); toast(std::string("You joined ") + c->name + "!"); sfxYay(); }
            return true;
        }
        if (!lead) return true;
        if (rowB(6).in(tp)) { sub = SB_EDIT; sfxTap(); }
        if (bA.in(tp)) { sub = SB_MEMBERS; page = 0; sfxTap(); }
        if (bB.in(tp)) { sub = SB_GATHER; sfxTap(); }
        if (bC.in(tp)) { sub = SB_PERKS; sfxTap(); }
        return true; }
    case SB_EDIT:
        if (!c) return true;
        for (int a = 0; a < A_COUNT; a++) {
            Btn b{6 + (a % 2) * 156.f, 30 + (a / 2) * 24.f, 152, 22};
            if (b.in(tp)) { uint16_t bit = (uint16_t)(1 << a); if (c->enc & bit) { c->enc &= ~bit; c->dis |= bit; } else if (c->dis & bit) c->dis &= ~bit; else c->enc |= bit; sfxTap(); return true; }
        }
        if (Btn{6, 152, 100, 22}.in(tp)) { c->icon = (uint8_t)((c->icon + 1) % 8); sfxTap(); }
        if (Btn{110, 152, 100, 22}.in(tp)) { c->req = (uint8_t)((c->req + 1) % 4); sfxTap(); }
        if (Btn{214, 152, 100, 22}.in(tp)) { std::string n = c->name; if (askText("Rename club", n, 22) && !n.empty()) snprintf(c->name, sizeof c->name, "%s", n.c_str()); }
        if (Btn{6, 178, 150, 22}.in(tp)) { c->dress = (uint8_t)((c->dress + 1) % 6); sfxTap(); }
        return true;
    case SB_MEMBERS: {
        if (!c) return true;
        // tap a member to cycle rank (member -> co-leader -> rival -> kick); tap "Invite" to add nearby townies
        for (int k = 0; k < 7; k++) { int i = page * 7 + k; if (i < c->nmem && rowB(k).in(tp)) {
            if (c->mem[i] == G.mc) return true;
            uint8_t& r = c->rank[i];
            if (r == 0) { r = 1; post(edit, c->mem[i], "Co-leader?? Omg ok I won't let you down"); }
            else if (r == 1) { r = 3; post(edit, c->mem[i], "Wow. Demoted to RIVAL? Fine. Game on."); changeRel(G.mc, c->mem[i], -5, 0); }
            else { post(edit, c->mem[i], "I can't believe you kicked me out."); changeRel(G.mc, c->mem[i], -10, 0); removeMember(*c, i); }
            sfxTap(); return true; } }
        if (bA.in(tp)) {   // invite: best friends who qualify
            std::vector<std::pair<int, int>> cand;
            for (const Rel& r : G.rels) { int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1; if (o >= 0 && memIdx(*c, o) < 0 && eligible(*c, o)) cand.push_back({-r.fr, o}); }
            std::sort(cand.begin(), cand.end());
            if (cand.empty()) { toast("Nobody you know qualifies yet. Make more friends!"); return true; }
            int o = cand[0].second; const Rel* r = findRel(G.mc, o);
            if ((r ? r->fr : 0) + irand(40) > 30) { addMember(*c, o, 0); post(edit, o, "Thanks for the invite, I'm in!"); toast(nm(o) + " joined your club!"); sfxYay(); }
            else { toast(nm(o) + " said no thanks... for now."); sfxNo(); }
        }
        if (bB.in(tp)) page++;
        if (bC.in(tp)) page = std::max(0, page - 1);
        return true; }
    case SB_GATHER:
        if (!c) return true;
        if (rowB(0).in(tp)) c->gRoom = (uint8_t)((c->gRoom + 1) % 8);
        if (rowB(1).in(tp)) c->gHour = (uint8_t)(c->gHour >= 22 ? 8 : std::max(8, c->gHour + 2));
        if (rowB(2).in(tp)) c->gDress = (uint8_t)((c->gDress + 1) % 6);
        if (rowB(3).in(tp)) c->gDay = c->gDay == 0xFFFF || c->gDay < G.day ? (uint16_t)(G.day + 1) : (uint16_t)(c->gDay + 1);
        if (bC.in(tp)) {
            if (c->gDay == 0xFFFF) c->gDay = (uint16_t)(G.day + 1);
            if (c->gHour < 8) c->gHour = 18;
            post(edit, G.mc, fmt("Gathering %s at %d:00 - %s! Dress code: %s", c->gDay == G.day ? "TODAY" : "tomorrow", c->gHour, ROOM_NAMES[GATHER_ROOMS[c->gRoom]], DRESS[c->gDress]));
            toast("Gathering scheduled! Check your phone's Clubs app."); sfxYay(); sub = SB_CLUB;
        }
        sfxTap(); return true;
    case SB_PERKS:
        if (!c) return true;
        for (int p = 0; p < 4; p++) if (rowB(p + 1).in(tp)) {
            if (c->perks >> p & 1) { toast("Already unlocked!"); return true; }
            if (c->points < PERK_COST[p]) { toast(fmt("Needs %d club points.", PERK_COST[p])); sfxNo(); return true; }
            c->points -= PERK_COST[p]; c->perks |= 1 << p; toast(std::string("Perk unlocked: ") + PERK_N[p]); sfxYay(); return true;
        }
        return true;
    }
    return true;
}

void clubsNotify(int ev, int arg) {
    if (G.sims.empty()) return;
    if (ev == GE_SOCIAL) dailyHit(DK_TALK);
    if (ev == GE_COOK) dailyHit(DK_COOK);
    if (ev == GE_TRAVEL) dailyHit(DK_TRAVEL);
    if (ev == GE_POST) dailyHit(DK_POST);
    if (ev == GE_DANCE) dailyHit(DK_DANCE);
    if (ev == GE_SHIFT) dailyHit(DK_WORK);
    if (ev == GE_TV) dailyHit(DK_TV);
    const int a = actOf(ev, arg); if (a < 0) return;
    for (int i = 0; i < S.nc; i++) {
        Club& c = S.c[i]; if (memIdx(c, G.mc) < 0) continue;
        if (c.enc >> a & 1) { c.points += 5; dailyHit(DK_CLUB); if (c.perks & 2) for (int k = 0; k < c.nmem; k++) if (c.mem[k] != G.mc) changeRel(G.mc, c.mem[k], 1, 0); }
        if (c.dis >> a & 1) { c.points = (uint16_t)std::max(0, c.points - 3); if (irand(4) == 0 && c.nmem > 1) post(i, c.mem[irand(c.nmem)], std::string("Um, ") + ACT_N[a] + "?? That's literally against club rules.");  }
    }
}
void clubsHourly(int hour) {
    if (G.sims.empty()) return;
    for (int i = 0; i < S.nc; i++) {
        Club& c = S.c[i];
        if (c.gDay == G.day && c.gHour == hour) {   // the gathering starts
            const int room = GATHER_ROOMS[c.gRoom % 8];
            const bool in = memIdx(c, G.mc) >= 0;
            if (in) {
                toast(fmt("%s gathering is starting at %s! (Dress code: %s)", c.name, ROOM_NAMES[room], DRESS[c.gDress]));
                if (room != R_LIVING) travelTo(room);
                int came = 0;
                for (int k = 0; k < c.nmem; k++) { int s = c.mem[k]; if (s == G.mc || c.rank[k] == 3) continue; if (irand(100) < 70) { changeRel(G.mc, s, 3, 0); came++; } }
                c.points += 10 + came * 3; addMoodlet(MC(), "Club Gathering", EM_HAPPY, (c.perks & 4) ? 2 : 1, 240);
                post(i, G.mc, fmt("%d of us showed up - best meetup yet!", came + 1));
            } else post(i, c.mem[0], "Great meetup today everyone!!");
            c.gDay = 0xFFFF;
        }
    }
    // group chat chatter, roughly every few hours
    if (hour % 4 == 2 && S.nc) {
        int i = irand(S.nc); Club& c = S.c[i]; if (c.nmem < 2) return;
        int a = c.mem[irand(c.nmem)], b = c.mem[irand(c.nmem)]; if (a == G.mc) return;
        int roll = irand(10);
        if (roll < 3) post(i, a, fmt(CHAT_PLAN[irand(3)], ROOM_NAMES[GATHER_ROOMS[irand(8)]]));
        else if (roll < 7) post(i, a, fmt(CHAT_GOSSIP[irand(4)], nm(b).c_str()));
        else if (a != b) { post(i, a, fmt(CHAT_FIGHT[irand(3)], nm(b).c_str())); changeRel(a, b, -4, 0); }
    }
}
void clubsDaily() {
    if (G.sims.empty()) return;
    seedTownClubs();
    if (S.d.day + 1 < G.day && !(S.d.done[0] && S.d.done[1] && S.d.done[2])) S.streak = 0;
    rollDaily();
    for (int i = 0; i < S.nc; i++) {
        Club& c = S.c[i];
        if (c.perks & 8) { if (memIdx(c, G.mc) >= 0) G.money += 50; }
        if (!c.mine && c.gDay == 0xFFFF && irand(3) == 0) { c.gDay = (uint16_t)(G.day + 1); c.gHour = (uint8_t)(12 + 2 * irand(4)); c.gRoom = (uint8_t)irand(7); post(i, c.mem[0], fmt("Meetup tomorrow at %d:00, %s!", c.gHour, ROOM_NAMES[GATHER_ROOMS[c.gRoom]])); }
        // drama: someone quits, switches clubs, or a rival is born
        if (c.nmem > 2 && irand(6) == 0) {
            int k = 1 + irand(c.nmem - 1); int s = c.mem[k]; if (s == G.mc) continue;
            int r = irand(3);
            if (r == 0) { post(i, s, "I'm done with this club. Bye."); removeMember(c, k); }
            else if (r == 1 && S.nc > 1) { int j = (i + 1 + irand(S.nc - 1)) % S.nc; if (S.c[j].nmem < 10) { post(i, s, std::string("Sorry guys, I'm joining ") + S.c[j].name + "."); removeMember(c, k); addMember(S.c[j], s, 0); } }
            else if (c.rank[k] == 0) { c.rank[k] = 3; post(i, s, "Some of you are fake and I'm saying it."); }
        }
        // autonomous recruiting: townies with friends in the club join
        if (!c.mine && c.nmem < 7 && irand(3) == 0) { int s = irand((int)G.sims.size()); if (s != G.mc && eligible(c, s) && !G.sims[s].household) { addMember(c, s, 0); post(i, s, "Just joined! Hi everyone :)"); } }
    }
}

// ---------------------------------------------------------------- drawing
static void icon(int k, float x, float y, float r, u32 c) {
    switch (k % 8) {
    case 0: heart(x, y, r, c); break; case 1: sparkle(x, y, r * 1.3f, c); break;
    case 2: tri(x - r, y + r * 0.6f, x - r, y - r * 0.4f, x + r, y + r * 0.6f, c); tri(x + r, y + r * 0.6f, x + r, y - r * 0.4f, x - r, y + r * 0.6f, c); tri(x - r * 0.4f, y + r * 0.6f, x, y - r, x + r * 0.4f, y + r * 0.6f, c); break;
    case 3: circle(x - r * 0.4f, y + r * 0.5f, r * 0.45f, c); rect(x - r * 0.05f, y - r, r * 0.25f, r * 1.5f, c); break;
    case 4: rect(x - r, y - r * 0.7f, r * 2, r * 1.4f, c); break;
    case 5: for (int p = 0; p < 5; p++) circle(x + std::cos(p * 1.256f) * r * 0.6f, y + std::sin(p * 1.256f) * r * 0.6f, r * 0.45f, c); break;
    case 6: circle(x - r * 0.4f, y + r * 0.4f, r * 0.5f, c); circle(x + r * 0.4f, y + r * 0.4f, r * 0.5f, c); break;
    default: circle(x, y, r, c); break;
    }
}
void clubsDrawTop(float t) {
    grad(0, 0, TOP_W, TOP_H, C(255, 236, 248), C(232, 226, 255));
    for (int i = 0; i < 10; i++) sparkle(std::fmod(i * 47.f + t * 5, 400.f), 20 + (i * 37) % 200, 3, C(255, 255, 255, 140));
    static const char* TT[3] = {"CLUBS", "GROUP CHATS", "DAILY GOALS"};
    textOutline(TT[tab], 200, 4, 0.6f, C(255, 255, 255), C(BERRY), 1);
    float y = 34;
    auto line = [&](const std::string& s, float sc, RGB c) { for (auto& l : wrapText(s, sc, 372)) { if (y > 230) break; text(l.c_str(), 14, y, sc, C(c)); y += sc * 32; } };
    if (tab == 2) {
        if (S.d.day != G.day) rollDaily();
        line(fmt("Today's goals (day %d)   Streak: %d", G.day + 1, S.streak), 0.45f, BERRY);
        for (int k = 0; k < 3; k++) {
            const int kd = S.d.kind[k] % DK_COUNT;
            rrect(14, y, 372, 40, 8, C(S.d.done[k] ? RGB{200, 240, 210} : RGB{255, 255, 255}));
            text(fmt(DK_N[kd], S.d.need[k]).c_str(), 24, y + 4, 0.45f, C(UI_INK));
            text(S.d.done[k] ? "DONE! +$75" : fmt("%d / %d", S.d.prog[k], S.d.need[k]).c_str(), 24, y + 22, 0.38f, C(S.d.done[k] ? RGB{40, 140, 70} : RGB{110, 90, 120}));
            y += 46;
        }
        line("Finish all three for a streak bonus. New goals every morning!", 0.38f, RGB{110, 90, 120});
        return;
    }
    if (tab == 1) {
        int shown = 0;
        for (int i = (int)chat.size() - 1 - page * 9; i >= 0 && shown < 9; i--, shown++) {
            const ClubMsg& m = chat[i];
            text(fmt("[%s] %s: %s", m.club >= 0 && m.club < S.nc ? S.c[m.club].name : "?", nm(m.from).c_str(), m.text).c_str(), 10, y + shown * 22.f, 0.36f, C(m.from == G.mc ? BERRY : UI_INK));
        }
        if (chat.empty()) line("No messages yet. Join a club to get the group chats going!", 0.42f, UI_INK);
        return;
    }
    const Club* c = edit >= 0 && edit < S.nc && sub != SB_LIST ? &S.c[edit] : nullptr;
    if (!c) {
        line("Make your own club or join a town club. Clubs earn points from approved activities and unlock perks.", 0.42f, UI_INK);
        line("Town clubs run themselves - they plan meetups, recruit, gossip and sometimes fall apart.", 0.4f, RGB{110, 90, 120});
        return;
    }
    icon(c->icon, 34, 50, 14, C(BERRY));
    text(c->name, 60, 36, 0.6f, C(BERRY));
    static const char* REQ[4] = {"Anyone", "Teens & up", "Adults only", "Friends only"};
    text(fmt("%d members   %d points   Join: %s", c->nmem, c->points, REQ[c->req % 4]).c_str(), 60, 58, 0.4f, C(UI_INK));
    y = 80;
    std::string enc, dis; for (int a = 0; a < A_COUNT; a++) { if (c->enc >> a & 1) enc += std::string(enc.empty() ? "" : ", ") + ACT_N[a]; if (c->dis >> a & 1) dis += std::string(dis.empty() ? "" : ", ") + ACT_N[a]; }
    line("Encouraged: " + (enc.empty() ? std::string("-") : enc), 0.4f, RGB{40, 140, 70});
    line("Discouraged: " + (dis.empty() ? std::string("-") : dis), 0.4f, RGB{190, 60, 80});
    line(std::string("Dress code: ") + DRESS[c->dress % 6], 0.4f, UI_INK);
    if (c->gDay != 0xFFFF) line(fmt("Next gathering: day %d at %d:00 - %s (%s)", c->gDay + 1, c->gHour, ROOM_NAMES[GATHER_ROOMS[c->gRoom % 8]], DRESS[c->gDress % 6]), 0.4f, BERRY);
    float x = 14; y += 4;
    for (int k = 0; k < c->nmem && k < 10; k++) {
        if (!ok(c->mem[k])) continue;
        drawSimPortrait(G.sims[c->mem[k]], x + 16, y + 18, 15, t);
        text(nm(c->mem[k]).c_str(), x + 16, y + 36, 0.3f, C(UI_INK), 1);
        text(RANK_N[c->rank[k] % 4], x + 16, y + 46, 0.28f, C(c->rank[k] == 3 ? RGB{190, 60, 80} : RGB{110, 90, 120}), 1);
        x += 37;
    }
}
void clubsDrawBottom(float t) {
    (void)t;
    grad(0, 0, BOT_W, BOT_H, C(255, 246, 250), C(244, 236, 255));
    if (sub == SB_LIST) { static const char* TN[3] = {"Clubs", "Group Chat", "Daily Goals"}; for (int k = 0; k < 3; k++) button(tabB(k), TN[k], tab == k, k == 2 ? 2 : 0, 0.4f); }
    button(bBack, sub == SB_LIST ? "Close" : "Back", false, 1, 0.4f);
    if (tab == 2) return;
    if (tab == 1) { button(bA, "Newer", false, 0, 0.38f); button(bB, "Older", false, 0, 0.38f); return; }
    Club* c = edit >= 0 && edit < S.nc ? &S.c[edit] : nullptr;
    switch (sub) {
    case SB_LIST:
        for (int k = 0; k < S.nc && k < 6; k++) { const Club& x = S.c[k]; button(rowB(k + 1), fmt("%s%s  (%d)", x.name, x.mine ? " - yours" : memIdx(x, G.mc) >= 0 ? " - joined" : "", x.nmem).c_str(), x.mine, x.mine ? 2 : 0, 0.38f); }
        if (S.nc < 6 && clubOfPlayer() < 0) button(bC, "+ Create", true, 2, 0.38f);
        break;
    case SB_CLUB: {
        if (!c) break;
        const int me = memIdx(*c, G.mc); const bool lead = me >= 0 && c->rank[me] >= 1 && c->rank[me] <= 2;
        text(c->name, 160, 34, 0.5f, C(BERRY), 1);
        text(me >= 0 ? (std::string("You are: ") + RANK_N[c->rank[me] % 4]).c_str() : "You're not a member", 160, 60, 0.4f, C(UI_INK), 1);
        button(rowB(5), me >= 0 ? "Leave Club" : "Join Club", me < 0, me >= 0 ? 1 : 2, 0.4f);
        if (lead) { button(rowB(6), "Edit Name, Icon & Rules", false, 0, 0.38f); button(bA, "Members", false, 0, 0.36f); button(bB, "Gathering", false, 3, 0.36f); button(bC, "Perks", false, 2, 0.36f); }
        break; }
    case SB_EDIT:
        if (!c) break;
        for (int a = 0; a < A_COUNT; a++) {
            const bool e = c->enc >> a & 1, d = c->dis >> a & 1;
            button(Btn{6 + (a % 2) * 156.f, 30 + (a / 2) * 24.f, 152, 22}, fmt("%s %s", e ? "+" : d ? "-" : " ", ACT_N[a]).c_str(), e, d ? 1 : e ? 2 : 0, 0.36f);
        }
        button(Btn{6, 152, 100, 22}, (std::string("Icon: ") + ICONS[c->icon % 8]).c_str(), false, 3, 0.34f);
        { static const char* REQ[4] = {"Join: anyone", "Join: teens+", "Join: adults", "Join: friends"}; button(Btn{110, 152, 100, 22}, REQ[c->req % 4], false, 3, 0.34f); }
        button(Btn{214, 152, 100, 22}, "Rename", false, 3, 0.34f);
        button(Btn{6, 178, 150, 22}, (std::string("Dress: ") + DRESS[c->dress % 6]).c_str(), false, 0, 0.34f);
        text("Tap: encourage (+) / discourage (-) / off", 240, 182, 0.3f, C(110, 90, 120), 1);
        break;
    case SB_MEMBERS:
        if (!c) break;
        for (int k = 0; k < 7; k++) { int i = page * 7 + k; if (i >= c->nmem) break; button(rowB(k), fmt("%s - %s%s", ok(c->mem[i]) ? G.sims[c->mem[i]].name.c_str() : "?", RANK_N[c->rank[i] % 4], c->mem[i] == G.mc ? " (you)" : "").c_str(), false, c->rank[i] == 3 ? 1 : 0, 0.36f); }
        text("Tap: promote > rival > kick out", 160, 198, 0.3f, C(110, 90, 120), 1);
        button(bA, "Invite", true, 2, 0.38f); button(bB, "More", false, 0, 0.36f); button(bC, "Back pg", false, 0, 0.34f);
        break;
    case SB_GATHER:
        if (!c) break;
        button(rowB(0), (std::string("Where: ") + ROOM_NAMES[GATHER_ROOMS[c->gRoom % 8]]).c_str(), false, 0, 0.38f);
        button(rowB(1), fmt("Time: %d:00", c->gHour < 8 ? 18 : c->gHour).c_str(), false, 0, 0.38f);
        button(rowB(2), (std::string("Dress code: ") + DRESS[c->gDress % 6]).c_str(), false, 0, 0.38f);
        button(rowB(3), c->gDay == 0xFFFF || c->gDay < G.day ? "Day: tomorrow" : fmt("Day: %d", c->gDay + 1).c_str(), false, 0, 0.38f);
        button(bC, "Schedule!", true, 2, 0.38f);
        break;
    case SB_PERKS:
        if (!c) break;
        text(fmt("Club points: %d", c->points).c_str(), 160, 34, 0.42f, C(BERRY), 1);
        for (int p = 0; p < 4; p++) button(rowB(p + 1), fmt("%s - %s", PERK_N[p], c->perks >> p & 1 ? "UNLOCKED" : fmt("%d pts", PERK_COST[p]).c_str()).c_str(), c->perks >> p & 1, c->perks >> p & 1 ? 2 : 0, 0.34f);
        text("Earn points: encouraged activities, gatherings, daily goals", 160, 160, 0.32f, C(110, 90, 120), 1);
        break;
    }
}

void clubsSaveTo(const std::string& path) {
    FILE* f = fopen(path.c_str(), "wb"); if (!f) return;
    fwrite(&S, sizeof S, 1, f); uint32_t n = (uint32_t)chat.size(); fwrite(&n, 4, 1, f); if (n) fwrite(chat.data(), sizeof(ClubMsg), n, f); fclose(f);
}
void clubsLoadFrom(const std::string& path) {
    S = State{}; chat.clear();
    FILE* f = fopen(path.c_str(), "rb"); if (!f) return;
    State s; uint32_t n = 0;
    if (fread(&s, sizeof s, 1, f) == 1 && s.magic == 0x434C4231) {
        if (s.nc > 6) s.nc = 0;
        for (auto& c : s.c) { c.name[23] = 0; if (c.nmem > 10) c.nmem = 0; for (int k = 0; k < c.nmem; k++) if (c.mem[k] >= (int)G.sims.size()) c.mem[k] = 0; }
        S = s;
        if (fread(&n, 4, 1, f) == 1 && n <= 60) { chat.resize(n); if (n && fread(chat.data(), sizeof(ClubMsg), n, f) != n) chat.clear(); for (auto& m : chat) m.text[95] = 0; }
    }
    fclose(f);
}
