// Movie Maker: a scene-script editor + cinematic playback using the game's own Sims and rooms.
#include "bgs.hpp"
#include "moviemaker.hpp"
#include "lifestory.hpp"
#include "cc.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "people.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
using namespace col;
void toast(const std::string& m);
bool askText(const char* hint, std::string& out, int maxLen, bool numeric);

namespace {
enum Scr { S_SERIES, S_EPS, S_SCENES, S_SCENE, S_LINES, S_LINE, S_PLAY };
enum Act { A_IDLE, A_TALK, A_LAUGH, A_CRY, A_ANGRY, A_SHOCK, A_WALK, A_SIT, A_HUG, A_ARGUE, A_CELEB, A_ROMANCE, A_COUNT };
const char* ACT_N[A_COUNT] = {"Idle", "Talking", "Laughing", "Crying", "Angry", "Shocked", "Walking", "Sitting", "Hugging", "Arguing", "Celebrating", "Romantic"};
enum Cam { CAM_WIDE, CAM_MED, CAM_CLOSE, CAM_TWO, CAM_FOCUS, CAM_EST, CAM_COUNT };
const char* CAM_N[CAM_COUNT] = {"Wide Shot", "Medium Shot", "Close-Up", "Two-Shot", "Character Focus", "Establishing"};
const char* TRANS_N[3] = {"Cut", "Fade", "Iris"};
const char* POS_N[3] = {"Left", "Center", "Right"};
const float POS_X[3] = {110, 200, 290};

Scr scr = S_SERIES;
int si = 0, ei = 0, sci = 0, li = 0, scroll = 0, delArm = -1;
std::vector<MovieScene> reel; int pScene = 0, pLine = -1; float pT = 0, pIntro = 0; bool paused = false; Scr playBack = S_SCENES;

MovieSeries* series() { return si >= 0 && si < (int)G.movies.size() ? &G.movies[si] : nullptr; }
MovieEp* ep() { MovieSeries* s = series(); return s && ei >= 0 && ei < (int)s->eps.size() ? &s->eps[ei] : nullptr; }
MovieScene* scene() { MovieEp* e = ep(); return e && sci >= 0 && sci < (int)e->scenes.size() ? &e->scenes[sci] : nullptr; }
MovieLine* lineP() { MovieScene* s = scene(); return s && li >= 0 && li < (int)s->lines.size() ? &s->lines[li] : nullptr; }
bool okSim(int s) { return s >= 0 && s < (int)G.sims.size(); }
// locations: every room, then custom backgrounds from the SD card (stored as 200 + k)
uint8_t stepLoc(uint8_t loc, int dir) {
    std::vector<int> all; for (int r = 0; r < R_COUNT; r++) if (r != R_NEIGHBOR) all.push_back(r);
    for (int k = 0; k < bgExtraCount() && k < 50; k++) all.push_back(150 + k);
    for (int k = 0; k < ccBackgroundCount() && k < 50; k++) all.push_back(200 + k);
    int i = 0; for (size_t q = 0; q < all.size(); q++) if (all[q] == loc) i = (int)q;
    return (uint8_t)all[(i + dir + (int)all.size()) % all.size()];
}
const char* locName(int loc) { return loc >= 200 ? ccBackgroundName(loc - 200) : loc >= 150 ? bgExtraName(loc - 150) : ROOM_NAMES[loc % R_COUNT]; }

std::vector<int> castPool() {
    std::vector<int> v{G.mc};
    for (size_t i = 0; i < G.sims.size(); i++) if ((int)i != G.mc && G.sims[i].household) v.push_back((int)i);
    for (size_t i = 0; i < G.sims.size(); i++) if ((int)i != G.mc && !G.sims[i].household && !simDead((int)i)) { const Rel* r = findRel(G.mc, (int)i); if (r && r->met) v.push_back((int)i); }
    for (size_t i = 0; i < G.sims.size(); i++) if (!simDead((int)i) && std::find(v.begin(), v.end(), (int)i) == v.end()) v.push_back((int)i);
    return v;
}
int cycleCast(int cur, int dir) {
    std::vector<int> p = castPool(); p.insert(p.begin(), -1);
    int k = 0; for (size_t i = 0; i < p.size(); i++) if (p[i] == cur) k = (int)i;
    k = (k + dir + (int)p.size()) % (int)p.size(); return p[k];
}
std::string ask(const char* hint, const std::string& cur, int maxLen) { std::string s = cur; if (askText(hint, s, maxLen, false) && !s.empty()) return s; return cur; }

Btn rowB(int k) { return Btn{6, 26.f + k * 30, 250, 27}; }
const Btn B_SIDE0{262, 26, 54, 27}, B_SIDE1{262, 58, 54, 27}, B_SIDE2{262, 90, 54, 27};
const Btn B_UP{262, 154, 26, 24}, B_DN{290, 154, 26, 24};
const Btn B_BACK{4, 208, 70, 28}, B_MID{80, 208, 116, 28}, B_RIGHT{200, 208, 116, 28};

void startPlay(const std::vector<MovieScene>& v, Scr back) {
    if (v.empty()) { toast("Add a scene first!"); sfxNo(); return; }
    reel = v; pScene = 0; pLine = -1; pT = 0; pIntro = 0; paused = false; playBack = back; scr = S_PLAY;
}
float lineLen(const MovieScene& s, int l) {
    if (l < 0 || l >= (int)s.lines.size()) return (float)std::max<int>(2, s.dur);
    return std::max(2.2f, std::min(8.f, 1.2f + s.lines[l].text.size() * 0.06f)) + (s.dur > 4 ? (s.dur - 4) * 0.3f : 0);
}
void nextBeat() {
    if (pScene >= (int)reel.size()) return;
    const MovieScene& s = reel[pScene];
    pT = 0;
    if (pLine + 1 < (int)s.lines.size()) { pLine++; return; }
    if (s.lines.empty() && pLine < 0) { pLine = 0; return; }
    pScene++; pLine = -1; pIntro = 0;
}
}

void movieOpen() { scr = S_SERIES; scroll = 0; delArm = -1; }

bool movieUpdate(const Input& in) {
    const touchPosition& tp = in.tp; const bool tap = in.touch;
    if (scr == S_PLAY) {
        if (in.down & KEY_B) { scr = playBack; return true; }
        if (in.down & KEY_Y) paused = !paused;
        if (tap && B_BACK.in(tp)) { scr = playBack; sfxTap(); return true; }
        if (tap && B_MID.in(tp)) { paused = !paused; sfxTap(); return true; }
        if ((in.down & KEY_A) || (tap && B_RIGHT.in(tp))) { nextBeat(); return true; }
        if (pScene >= (int)reel.size()) { if (tap || (in.down & KEY_A)) scr = playBack; return true; }
        if (!paused) {
            pIntro += in.dt;
            if (pIntro > 0.9f) { pT += in.dt; if (pT > (pLine < 0 ? 1.4f : lineLen(reel[pScene], pLine))) nextBeat(); }
        }
        return true;
    }
    if (in.down & KEY_B) {
        if (scr == S_SERIES) { saveGame(G); return false; }
        scr = scr == S_LINE ? S_LINES : scr == S_LINES ? S_SCENE : scr == S_SCENE ? S_SCENES : scr == S_SCENES ? S_EPS : S_SERIES; scroll = 0; return true;
    }
    if (!tap) return true;
    if (B_UP.in(tp) && scr != S_SCENE && scr != S_LINE) { if (scroll > 0) scroll--; sfxTap(); return true; }
    if (B_DN.in(tp) && scr != S_SCENE && scr != S_LINE) { scroll++; sfxTap(); return true; }
    if (B_BACK.in(tp)) {
        sfxTap();
        if (scr == S_SERIES) { toast(saveGame(G) ? "Movie projects saved!" : "Save failed! Check SD card."); return false; }
        scr = scr == S_LINE ? S_LINES : scr == S_LINES ? S_SCENE : scr == S_SCENE ? S_SCENES : scr == S_SCENES ? S_EPS : S_SERIES; scroll = 0; delArm = -1; return true;
    }
    switch (scr) {
    case S_SERIES: {
        for (int k = 0; k < 5; k++) { int i = scroll * 5 + k; if (i < (int)G.movies.size() && rowB(k).in(tp)) { si = i; scr = S_EPS; scroll = 0; sfxTap(); return true; } }
        if (B_SIDE0.in(tp) || B_MID.in(tp)) {
            if (G.movies.size() >= 8) { toast("8 series max - delete one first."); sfxNo(); break; }
            MovieSeries s; s.title = ask("Series title", "My Sims Series", 40);
            MovieEp e; e.title = "Episode 1"; s.eps.push_back(e);
            G.movies.push_back(s); si = (int)G.movies.size() - 1; scr = S_EPS; scroll = 0; sfxYay();
        }
        break; }
    case S_EPS: {
        MovieSeries* s = series(); if (!s) { scr = S_SERIES; break; }
        for (int k = 0; k < 5; k++) { int i = scroll * 5 + k; if (i < (int)s->eps.size() && rowB(k).in(tp)) { ei = i; scr = S_SCENES; scroll = 0; sfxTap(); return true; } }
        if (B_SIDE0.in(tp)) { if (s->eps.size() >= 12) { toast("12 episodes max."); break; } MovieEp e; char b[32]; snprintf(b, sizeof b, "Episode %d", (int)s->eps.size() + 1); e.title = ask("Episode title", b, 40); s->eps.push_back(e); sfxTap(); }
        if (B_SIDE1.in(tp)) { s->title = ask("Series title", s->title, 40); sfxTap(); }
        if (B_SIDE2.in(tp)) { if (delArm == 0) { G.movies.erase(G.movies.begin() + si); scr = S_SERIES; delArm = -1; toast("Series deleted."); } else { delArm = 0; toast("Tap Delete again to erase this whole series."); } }
        break; }
    case S_SCENES: {
        MovieEp* e = ep(); if (!e) { scr = S_EPS; break; }
        for (int k = 0; k < 5; k++) { int i = scroll * 5 + k; if (i < (int)e->scenes.size() && rowB(k).in(tp)) { sci = i; scr = S_SCENE; sfxTap(); return true; } }
        if (B_SIDE0.in(tp)) {
            if (e->scenes.size() >= 20) { toast("20 scenes max per episode."); break; }
            MovieScene sc; char b[32]; snprintf(b, sizeof b, "Scene %d", (int)e->scenes.size() + 1); sc.title = b;
            sc.loc = (uint8_t)(e->scenes.empty() ? G.room : e->scenes.back().loc);
            if (!e->scenes.empty()) for (int c = 0; c < 3; c++) sc.cast[c] = e->scenes.back().cast[c];
            else { sc.cast[0].sim = (int16_t)G.mc; sc.cast[0].pos = 0; int p = partnerOf(G.mc); if (p < 0) for (size_t i = 0; i < G.sims.size(); i++) if ((int)i != G.mc && G.sims[i].household) { p = (int)i; break; } if (p >= 0) { sc.cast[1].sim = (int16_t)p; sc.cast[1].pos = 2; } }
            e->scenes.push_back(sc); sci = (int)e->scenes.size() - 1; scr = S_SCENE; sfxYay();
        }
        if (B_SIDE1.in(tp)) { e->title = ask("Episode title", e->title, 40); sfxTap(); }
        if (B_SIDE2.in(tp)) { if (delArm == 1) { series()->eps.erase(series()->eps.begin() + ei); scr = S_EPS; delArm = -1; toast("Episode deleted."); } else { delArm = 1; toast("Tap Delete again to erase this episode."); } }
        if (B_MID.in(tp)) startPlay(e->scenes, S_SCENES);
        break; }
    case S_SCENE: {
        MovieScene* s = scene(); if (!s) { scr = S_SCENES; break; }
        if (Btn{4, 22, 204, 22}.in(tp)) s->title = ask("Scene title", s->title, 40);
        if (Btn{212, 22, 104, 22}.in(tp)) s->dur = (uint8_t)(s->dur >= 9 ? 2 : s->dur + 1);
        if (Btn{4, 48, 28, 22}.in(tp)) s->loc = stepLoc(s->loc, -1);
        if (Btn{180, 48, 28, 22}.in(tp)) s->loc = stepLoc(s->loc, 1);
        if (Btn{212, 48, 104, 22}.in(tp)) s->cam = (uint8_t)((s->cam + 1) % CAM_COUNT);
        if (Btn{4, 74, 104, 22}.in(tp)) s->trans = (uint8_t)((s->trans + 1) % 3);
        for (int c = 0; c < 3; c++) {
            float y = 100 + c * 26.f; MovieActor& a = s->cast[c];
            if (Btn{4, y, 22, 22}.in(tp)) a.sim = (int16_t)cycleCast(a.sim, -1);
            if (Btn{128, y, 22, 22}.in(tp)) a.sim = (int16_t)cycleCast(a.sim, 1);
            if (Btn{154, y, 50, 22}.in(tp)) a.pos = (uint8_t)((a.pos + 1) % 3);
            if (Btn{208, y, 54, 22}.in(tp)) a.act = (uint8_t)((a.act + 1) % A_COUNT);
            if (Btn{266, y, 50, 22}.in(tp)) a.outfit = (uint8_t)(a.outfit == 0xFF ? 0 : a.outfit + 1 >= NUM_OUTFITS ? 0xFF : a.outfit + 1);
        }
        if (Btn{4, 178, 104, 26}.in(tp)) { scr = S_LINES; scroll = 0; }
        if (Btn{112, 178, 100, 26}.in(tp)) { startPlay({*s}, S_SCENE); }
        if (Btn{216, 178, 100, 26}.in(tp)) { if (delArm == 2) { ep()->scenes.erase(ep()->scenes.begin() + sci); scr = S_SCENES; delArm = -1; toast("Scene deleted."); } else { delArm = 2; toast("Tap Delete again to erase this scene."); } }
        sfxTap();
        break; }
    case S_LINES: {
        MovieScene* s = scene(); if (!s) { scr = S_SCENES; break; }
        for (int k = 0; k < 5; k++) { int i = scroll * 5 + k; if (i < (int)s->lines.size() && rowB(k).in(tp)) { li = i; scr = S_LINE; sfxTap(); return true; } }
        if (B_SIDE0.in(tp) || B_MID.in(tp)) {
            if (s->lines.size() >= 30) { toast("30 lines max per scene."); break; }
            MovieLine l; l.who = s->lines.empty() ? (s->cast[0].sim >= 0 ? 0 : -1) : (int8_t)(s->lines.back().who == 0 && s->cast[1].sim >= 0 ? 1 : s->cast[0].sim >= 0 ? 0 : -1);
            l.act = A_TALK; l.text = ask(l.who < 0 ? "Narration" : "Dialogue", "", 80);
            if (l.text.empty()) l.text = l.who < 0 ? "Meanwhile..." : "...";
            s->lines.push_back(l); li = (int)s->lines.size() - 1; sfxTap();
        }
        if (B_RIGHT.in(tp)) startPlay({*s}, S_LINES);
        break; }
    case S_LINE: {
        MovieScene* s = scene(); MovieLine* l = lineP(); if (!s || !l) { scr = S_LINES; break; }
        if (Btn{4, 26, 312, 26}.in(tp)) { int w = l->who; do w = w >= 2 ? -1 : w + 1; while (w >= 0 && s->cast[w].sim < 0); l->who = (int8_t)w; }
        if (Btn{4, 56, 312, 40}.in(tp)) l->text = ask(l->who < 0 ? "Narration" : "Dialogue", l->text, 80);
        if (Btn{4, 100, 154, 26}.in(tp)) l->emo = (uint8_t)((l->emo + 1) % EM_COUNT);
        if (Btn{162, 100, 154, 26}.in(tp)) l->act = (uint8_t)((l->act + 1) % A_COUNT);
        if (Btn{4, 132, 100, 26}.in(tp) && li > 0) { std::swap(s->lines[li], s->lines[li - 1]); li--; }
        if (Btn{108, 132, 100, 26}.in(tp) && li + 1 < (int)s->lines.size()) { std::swap(s->lines[li], s->lines[li + 1]); li++; }
        if (Btn{212, 132, 104, 26}.in(tp)) { s->lines.erase(s->lines.begin() + li); scr = S_LINES; }
        sfxTap();
        break; }
    default: break;
    }
    return true;
}

namespace {
void listHeader(const char* title, const char* sub) {
    rect(0, 0, BOT_W, BOT_H, C(250, 244, 255));
    rect(0, 0, BOT_W, 22, C(60, 30, 80)); text(title, 160, 3, 0.45f, C(255, 220, 120), 1);
    if (sub) text(sub, 160, 190, 0.3f, C(140, 120, 150), 1);
}
std::string castName(int s) { return okSim(s) ? G.sims[s].name.substr(0, G.sims[s].name.find(' ')) : "(empty)"; }

void drawActor(const MovieActor& a, int act, int emo, float x, float foot, float sc, float t, float otherX) {
    if (!okSim(a.sim)) return;
    Sim s = G.sims[a.sim];
    s.moodlets.clear(); Moodlet m; m.name = "Acting"; m.emotion = (uint8_t)emo; m.power = 9; m.minutes = 60; s.moodlets.push_back(m);
    for (float& n : s.needs) n = 100;
    if (a.outfit != 0xFF && a.outfit < NUM_OUTFITS) s.outfit = a.outfit;
    if (act == A_ROMANCE && s.stage < ST_TEEN) act = A_HUG;
    Pose pose = PO_IDLE; float dx = 0, dy = 0;
    switch (act) {
    case A_TALK: dy = -std::fabs(std::sin(t * 8)) * 1.5f; break;
    case A_LAUGH: dy = -std::fabs(std::sin(t * 14)) * 3; break;
    case A_ANGRY: case A_ARGUE: dx = std::sin(t * 40) * 1.5f; break;
    case A_SHOCK: dy = -std::fabs(std::sin(t * 3)) * 8; break;
    case A_WALK: pose = PO_WALK; g_walkPhase = std::fmod(t, 1.f); dx = std::sin(t * 0.8f) * 30; break;
    case A_SIT: pose = PO_SIT; break;
    case A_CELEB: pose = PO_DANCE; break;
    case A_HUG: case A_ROMANCE: if (otherX > -900) dx = (otherX - x) * 0.32f; break;
    default: break;
    }
    drawSim(s, x + dx, foot + dy, sc, t, 0, pose, otherX > -900 && otherX < x);
    float hy = foot + dy - 88 * sc;
    if (act == A_CRY) for (int k = 0; k < 2; k++) { float ph = std::fmod(t * 1.5f + k * 0.5f, 1.f); circle(x + dx - 5 * sc + k * 10 * sc, hy + 26 * sc + ph * 16 * sc, 2 * sc, C(120, 190, 255, (uint8_t)(255 * (1 - ph)))); }
    if (act == A_ANGRY || act == A_ARGUE) textOutline(act == A_ARGUE ? "!?" : "#", x + dx + 18 * sc, hy, 0.5f * sc, C(255, 60, 60), C(UI_INK), 1);
    if (act == A_SHOCK) textOutline("!", x + dx + 16 * sc, hy - 4, 0.6f * sc, C(255, 230, 80), C(UI_INK), 1);
    if (act == A_LAUGH) textOutline("haha", x + dx + 22 * sc, hy, 0.35f * sc, C(255, 255, 255), C(UI_INK), 1);
    if (act == A_CELEB) for (int k = 0; k < 4; k++) sparkle(x + std::sin(t * 2 + k) * 30 * sc, hy + std::cos(t * 3 + k) * 20 * sc, 3 * sc, C(255, 230, 120));
    if (act == A_ROMANCE || act == A_HUG) heart(x + dx + 12 * sc, hy - 6 - std::fmod(t * 10, 12.f), 4 * sc, C(255, 90, 150));
}

void drawBackdrop(int loc, float t) {
    if (loc >= 200) { ccDrawBackground(loc - 200, t); return; }
    if (loc >= 150) { bgDrawExtra(loc - 150); return; }
    const int saveRoom = G.room; RoomState& rs = G.rooms[G.house][loc % R_COUNT];
    std::vector<Actor> keep; keep.swap(rs.actors);
    G.room = loc % R_COUNT;
    drawRoom(G, t, -1, -1, false);
    G.room = saveRoom; rs.actors.swap(keep);
}

void drawSceneFrame(const MovieScene& s, int line, float t, bool cinematic) {
    drawBackdrop(s.loc, t);
    int speaker = (line >= 0 && line < (int)s.lines.size()) ? s.lines[line].who : -1;
    int cam = s.cam;
    std::vector<int> on; for (int c = 0; c < 3; c++) if (okSim(s.cast[c].sim)) on.push_back(c);
    float sc = 1.f, foot = 224; bool estab = cam == CAM_EST && (line <= 0);
    if (cam == CAM_MED) { sc = 1.45f; foot = 262; }
    if (cam == CAM_TWO) { sc = 1.6f; foot = 270; }
    if (cam == CAM_FOCUS) { sc = 1.3f; foot = 250; }
    if (cam == CAM_CLOSE) { rect(0, 0, TOP_W, TOP_H, C(0, 0, 0, 90)); }
    if (!estab) {
        for (int c : on) {
            const MovieActor& a = s.cast[c];
            int act = a.act, emo = EM_FINE;
            if (c == speaker && line >= 0) { act = s.lines[line].act; emo = s.lines[line].emo; }
            if (cam == CAM_CLOSE && c != (speaker >= 0 && okSim(s.cast[speaker].sim) ? speaker : on[0])) continue;
            if (cam == CAM_TWO && on.size() > 2 && c == on[2] && c != speaker) continue;
            float x = POS_X[a.pos % 3];
            if (cam == CAM_TWO) x = 200 + (c == on[0] ? -80.f : 80.f);
            if (cam == CAM_CLOSE) x = 200;
            float scX = cam == CAM_CLOSE ? 2.7f : sc, ft = cam == CAM_CLOSE ? 360.f : foot;
            if (cam == CAM_MED) x = 200 + (x - 200) * 1.3f;
            float ox = -999; for (int d : on) if (d != c) { ox = (cam == CAM_TWO) ? 200 + (d == on[0] ? -80.f : 80.f) : POS_X[s.cast[d].pos % 3]; break; }
            drawActor(a, act, emo, x, ft, scX, t, ox);
            if (cam == CAM_FOCUS && c != speaker && speaker >= 0) rect(x - 40, ft - 120 * scX, 80, 120 * scX, C(0, 0, 0, 70));
        }
    }
    if (estab) {
        rect(0, 92, TOP_W, 52, C(0, 0, 0, 120));
        textOutline(s.title.c_str(), 200, 100, 0.8f, C(255, 255, 255), C(UI_INK), 1);
        text(locName(s.loc), 200, 126, 0.42f, C(255, 220, 150), 1);
    }
    if (cinematic) { rect(0, 0, TOP_W, 16, C(0, 0, 0)); rect(0, 224, TOP_W, 16, C(0, 0, 0)); }
    if (line >= 0 && line < (int)s.lines.size()) {
        const MovieLine& l = s.lines[line];
        bool narr = l.who < 0 || !okSim(s.cast[l.who].sim);
        auto L = wrapText(l.text, 0.46f, 340);
        float h = 22 + L.size() * 15.f, y = narr ? 22 : 218 - h;
        rrect(20, y, 360, h, 8, narr ? C(30, 20, 40, 210) : C(255, 255, 255, 235));
        std::string who = narr ? "Narrator" : G.sims[s.cast[l.who].sim].name;
        textOutline(who.c_str(), 30, y - 7, 0.38f, C(255, 220, 120), C(UI_INK), 0);
        for (size_t k = 0; k < L.size(); k++) text(L[k].c_str(), 200, y + 8 + k * 15.f, 0.46f, narr ? C(255, 245, 230) : C(UI_INK), 1);
    }
}
}

void movieDrawTop(float t) {
    if (scr == S_PLAY) {
        if (pScene >= (int)reel.size()) {
            rect(0, 0, TOP_W, TOP_H, C(0, 0, 0));
            textOutline("THE END", 200, 96, 1.0f, C(255, 255, 255), C(255, 70, 150), 1);
            text("Tap or press A to go back", 200, 140, 0.4f, C(200, 200, 200), 1);
            return;
        }
        const MovieScene& s = reel[pScene];
        drawSceneFrame(s, pLine, t, true);
        if (pIntro < 0.9f) {
            float k = 1 - pIntro / 0.9f;
            if (s.trans == 1) rect(0, 0, TOP_W, TOP_H, C(0, 0, 0, (uint8_t)(255 * k)));
            else if (s.trans == 2) { float r = (1 - k) * 260; rect(0, 0, TOP_W, std::max(0.f, 120 - r), C(0, 0, 0)); rect(0, std::min(240.f, 120 + r), TOP_W, 240, C(0, 0, 0)); rect(0, 0, std::max(0.f, 200 - r * 1.6f), TOP_H, C(0, 0, 0)); rect(std::min(400.f, 200 + r * 1.6f), 0, 400, TOP_H, C(0, 0, 0)); }
        }
        if (paused) textOutline("PAUSED", 200, 4, 0.5f, C(255, 255, 255), C(UI_INK), 1);
        return;
    }
    MovieScene* s = scene();
    if ((scr == S_SCENE || scr == S_LINES || scr == S_LINE) && s) {
        drawSceneFrame(*s, scr == S_LINE ? li : -1, t, false);
        rect(0, 0, TOP_W, 16, C(0, 0, 0, 150));
        char b[96]; snprintf(b, sizeof b, "%s  -  %s  -  %s", s->title.c_str(), locName(s->loc), CAM_N[s->cam % CAM_COUNT]);
        text(b, 200, 1, 0.36f, C(255, 255, 255), 1);
        return;
    }
    grad(0, 0, TOP_W, TOP_H, C(40, 20, 60), C(120, 50, 110));
    for (int i = 0; i < 9; i++) sparkle(30 + i * 44.f, 30 + 10 * std::sin(t + i), 3, C(255, 230, 160, 160));
    textOutline("MOVIE MAKER", 200, 50, 1.0f, C(255, 255, 255), C(255, 70, 150), 1);
    text("Write, cast, direct and replay your own series", 200, 92, 0.45f, C(255, 220, 240), 1);
    MovieSeries* ms = series(); MovieEp* e = ep();
    if (scr == S_EPS && ms) { textOutline(ms->title.c_str(), 200, 130, 0.6f, C(255, 220, 120), C(UI_INK), 1); }
    if (scr == S_SCENES && ms && e) { textOutline(ms->title.c_str(), 200, 124, 0.5f, C(255, 220, 120), C(UI_INK), 1); text(e->title.c_str(), 200, 150, 0.5f, C(255, 255, 255), 1); }
    text("Movies save with your game (Back saves too)", 200, 214, 0.34f, C(220, 200, 230), 1);
}

void movieDrawBottom(float t) {
    char b[128];
    switch (scr) {
    case S_SERIES:
        listHeader("Your Series", "Tap a series to open it");
        for (int k = 0; k < 5; k++) { int i = scroll * 5 + k; if (i >= (int)G.movies.size()) break; snprintf(b, sizeof b, "%s  (%d eps)", G.movies[i].title.c_str(), (int)G.movies[i].eps.size()); button(rowB(k), b, false, 0, 0.38f); }
        if (G.movies.empty()) text("No series yet - make your first one!", 130, 80, 0.38f, C(140, 120, 150), 1);
        button(B_SIDE0, "New", false, 2, 0.4f);
        button(B_MID, "New Series", false, 2, 0.4f);
        break;
    case S_EPS: { MovieSeries* s = series(); if (!s) break;
        listHeader(s->title.c_str(), "Tap an episode to edit its scenes");
        for (int k = 0; k < 5; k++) { int i = scroll * 5 + k; if (i >= (int)s->eps.size()) break; snprintf(b, sizeof b, "%s  (%d scenes)", s->eps[i].title.c_str(), (int)s->eps[i].scenes.size()); button(rowB(k), b, false, 0, 0.38f); }
        button(B_SIDE0, "New Ep", false, 2, 0.34f); button(B_SIDE1, "Rename", false, 0, 0.34f); button(B_SIDE2, "Delete", delArm == 0, 1, 0.34f);
        break; }
    case S_SCENES: { MovieEp* e = ep(); if (!e) break;
        listHeader(e->title.c_str(), "Tap a scene to edit it");
        for (int k = 0; k < 5; k++) { int i = scroll * 5 + k; if (i >= (int)e->scenes.size()) break; const MovieScene& sc = e->scenes[i];
            snprintf(b, sizeof b, "%d. %s @ %s", i + 1, sc.title.c_str(), locName(sc.loc)); button(rowB(k), b, false, 0, 0.34f); }
        button(B_SIDE0, "New", false, 2, 0.36f); button(B_SIDE1, "Rename", false, 0, 0.34f); button(B_SIDE2, "Delete", delArm == 1, 1, 0.34f);
        button(B_MID, "Play Episode", false, 3, 0.4f);
        break; }
    case S_SCENE: { MovieScene* s = scene(); if (!s) break;
        rect(0, 0, BOT_W, BOT_H, C(250, 244, 255));
        text("Scene Editor", 160, 4, 0.4f, C(60, 30, 80), 1);
        snprintf(b, sizeof b, "Title: %s", s->title.c_str()); button(Btn{4, 22, 204, 22}, b, false, 0, 0.34f);
        snprintf(b, sizeof b, "Length: %ds", s->dur); button(Btn{212, 22, 104, 22}, b, false, 0, 0.34f);
        button(Btn{4, 48, 28, 22}, "<", false, 2, 0.4f); button(Btn{180, 48, 28, 22}, ">", false, 2, 0.4f);
        rrect(34, 48, 144, 22, 5, C(255, 255, 255)); text(locName(s->loc), 106, 51, 0.32f, C(UI_INK), 1);
        button(Btn{212, 48, 104, 22}, CAM_N[s->cam % CAM_COUNT], false, 3, 0.3f);
        snprintf(b, sizeof b, "Transition: %s", TRANS_N[s->trans % 3]); button(Btn{4, 74, 104, 22}, b, false, 0, 0.3f);
        text("Cast  /  spot  /  acting  /  outfit", 212, 78, 0.3f, C(140, 120, 150), 1);
        for (int c = 0; c < 3; c++) { float y = 100 + c * 26.f; const MovieActor& a = s->cast[c];
            button(Btn{4, y, 22, 22}, "<", false, 2, 0.36f); rrect(28, y, 98, 22, 5, C(255, 255, 255)); text(castName(a.sim).c_str(), 77, y + 4, 0.34f, C(UI_INK), 1); button(Btn{128, y, 22, 22}, ">", false, 2, 0.36f);
            button(Btn{154, y, 50, 22}, POS_N[a.pos % 3], false, 0, 0.3f); button(Btn{208, y, 54, 22}, ACT_N[a.act % A_COUNT], false, 0, 0.28f);
            button(Btn{266, y, 50, 22}, a.outfit == 0xFF ? "Own" : OUTFITS[a.outfit % NUM_OUTFITS].name, false, 0, 0.24f); }
        snprintf(b, sizeof b, "Script (%d)", (int)s->lines.size()); button(Btn{4, 178, 104, 26}, b, false, 2, 0.36f);
        button(Btn{112, 178, 100, 26}, "Preview", false, 3, 0.38f); button(Btn{216, 178, 100, 26}, "Delete", delArm == 2, 1, 0.36f);
        break; }
    case S_LINES: { MovieScene* s = scene(); if (!s) break;
        listHeader("Script", "Tap a line to edit who says it, the mood and the action");
        for (int k = 0; k < 5; k++) { int i = scroll * 5 + k; if (i >= (int)s->lines.size()) break; const MovieLine& l = s->lines[i];
            std::string who = l.who < 0 || !okSim(s->cast[l.who].sim) ? "Narrator" : castName(s->cast[l.who].sim);
            std::string tx = who + ": " + l.text; if (tx.size() > 34) tx = tx.substr(0, 32) + "..";
            button(rowB(k), tx.c_str(), false, l.who < 0 ? 3 : 0, 0.32f); }
        button(B_SIDE0, "+ Line", false, 2, 0.36f); button(B_MID, "Add Line", false, 2, 0.4f); button(B_RIGHT, "Preview", false, 3, 0.4f);
        break; }
    case S_LINE: { MovieScene* s = scene(); MovieLine* l = lineP(); if (!s || !l) break;
        rect(0, 0, BOT_W, BOT_H, C(250, 244, 255));
        snprintf(b, sizeof b, "Line %d of %d", li + 1, (int)s->lines.size()); text(b, 160, 4, 0.4f, C(60, 30, 80), 1);
        std::string who = l->who < 0 ? "Narrator" : castName(s->cast[l->who].sim);
        snprintf(b, sizeof b, "Speaker: %s  (tap to change)", who.c_str()); button(Btn{4, 26, 312, 26}, b, false, 2, 0.36f);
        rrect(4, 56, 312, 40, 6, C(255, 255, 255)); { auto L = wrapText(l->text, 0.36f, 300); for (size_t k = 0; k < L.size() && k < 3; k++) text(L[k].c_str(), 160, 58 + k * 12.f, 0.36f, C(UI_INK), 1); }
        snprintf(b, sizeof b, "Mood: %s", EMOTION_NAMES[l->emo % EM_COUNT]); button(Btn{4, 100, 154, 26}, b, false, 0, 0.34f);
        rrect(6, 102, 8, 22, 3, C(EMOTION_COLORS[l->emo % EM_COUNT]));
        snprintf(b, sizeof b, "Action: %s", ACT_N[l->act % A_COUNT]); button(Btn{162, 100, 154, 26}, b, false, 0, 0.34f);
        button(Btn{4, 132, 100, 26}, "Move Up", false, 0, 0.34f); button(Btn{108, 132, 100, 26}, "Move Down", false, 0, 0.34f); button(Btn{212, 132, 104, 26}, "Delete", false, 1, 0.34f);
        text("Tap the text box to rewrite the line", 160, 166, 0.3f, C(140, 120, 150), 1);
        break; }
    case S_PLAY:
        rect(0, 0, BOT_W, BOT_H, C(20, 14, 26));
        if (pScene < (int)reel.size()) { snprintf(b, sizeof b, "Scene %d / %d: %s", pScene + 1, (int)reel.size(), reel[pScene].title.c_str()); text(b, 160, 80, 0.42f, C(255, 220, 240), 1); }
        text("A / Next: skip ahead     Y: pause     B: stop", 160, 110, 0.32f, C(160, 140, 170), 1);
        button(B_BACK, "Stop", false, 1, 0.4f); button(B_MID, paused ? "Resume" : "Pause", false, 0, 0.4f); button(B_RIGHT, "Next", false, 2, 0.4f);
        return;
    }
    if (scr != S_SCENE && scr != S_LINE) { button(B_UP, "^", false, 0, 0.4f); button(B_DN, "v", false, 0, 0.4f); }
    button(B_BACK, scr == S_SERIES ? "Save & Exit" : "Back", false, 1, 0.34f);
    (void)t;
}
