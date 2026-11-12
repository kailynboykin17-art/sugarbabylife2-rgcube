#include "minigames.hpp"
#include "draw.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "audio.hpp"
#include "goals.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace col;

const char* MG_NAMES[10] = {"Glam Match", "Pinata Smash", "Treat Catch", "Color Dance", "Tic-Tac-Toe", "Runway Walk",
                            "Nail Salon", "Karaoke Star", "Beach Volley", "Smoothie Rush"};
const void* g_mgContest = nullptr;
int g_mgLastScore = 0;

static int cur = 0, score = 0, lives = 3;
static float timer = 0, waitT = 0;
static bool over = false;
static std::string note;

// memory
static int cards[16], cstate[16], first = -1, second = -1, moves = 0, matched = 0;
// pinata
static int pinCell = -1; static float pinT = 0;
// catch
struct Drop { float x, y, vy; int kind; };
static std::vector<Drop> drops; static float basketX = 200, spawnT = 0;
// simon
static std::vector<int> seq; static int inIdx = 0, phase = 0, showIdx = 0, lit = -1; static float showT = 0;
// ttt
static int board[9], tttResult = 0;
// runway
static int want = 0; static float beatT = 0, beatLen = 1.2f; static int streak = 0;

// nail salon
static const RGB NAIL_COLS[6] = {{255, 58, 140}, {220, 40, 60}, {200, 166, 230}, {120, 190, 240}, {224, 184, 104}, {40, 32, 36}};
static int nailWant[5], nailHave[5], nailIdx = 0; static float nailFlash = 0;
// karaoke
struct KNote { float x; int dir; bool hit; };
static std::vector<KNote> knotes; static float kSpawn = 0; static int combo = 0, misses = 0; static float kFlash = 0;
// volley
static float bx = 200, by = 60, bvx = 60, bvy = 0, padX = 200;
// smoothie
static const char* FRUIT[8] = {"Strawberry", "Banana", "Mango", "Blueberry", "Kiwi", "Milk", "Ice", "Yogurt"};
static const RGB FRUITC[8] = {{230, 50, 70}, {250, 220, 90}, {255, 160, 40}, {80, 90, 200}, {130, 190, 60}, {250, 250, 245}, {200, 230, 250}, {255, 230, 240}};
static int order[3], blend[3], blendN = 0; static float smFlash = 0; static bool smGood = false;

static void newNails() { for (int i = 0; i < 5; i++) { nailWant[i] = irand(6); nailHave[i] = -1; } nailIdx = 0; }
static void newOrder() {
    for (int i = 0; i < 3; i++) { int f; bool dup; do { f = irand(8); dup = false; for (int k = 0; k < i; k++) if (order[k] == f) dup = true; } while (dup); order[i] = f; }
    blendN = 0;
}

static void finish() {
    over = true;
    g_mgLastScore = score;
    int32_t& best = G.best[cur];
    bool better = cur == MG_MEMORY ? (best == 0 || score < best) : score > best;
    if (better && (cur != MG_MEMORY || score > 0)) { best = score; note = "NEW BEST!"; sfxYay(); }
    Sim& me = MC();
    me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 30);
    addMoodlet(me, "Game Night", EM_PLAYFUL, 1, 120);
    if (cur == MG_SIMON || cur == MG_MEMORY) skillGain(me, SK_LOGIC, 2);
    if (cur == MG_RUNWAY) skillGain(me, SK_STYLE, 2);
    goalNotify(GE_MINIGAME, cur);
}

static void simonAdd() { seq.push_back(irand(4)); phase = 0; showIdx = 0; showT = 0.6f; inIdx = 0; lit = -1; }

void mgStart(int g) {
    cur = g; score = 0; lives = 3; over = false; note.clear(); timer = 30; waitT = 0;
    switch (g) {
    case MG_MEMORY:
        for (int i = 0; i < 16; i++) { cards[i] = i / 2; cstate[i] = 0; }
        for (int i = 15; i > 0; i--) std::swap(cards[i], cards[irand(i + 1)]);
        first = second = -1; moves = matched = 0;
        break;
    case MG_PINATA: pinCell = -1; pinT = 0; break;
    case MG_CATCH: drops.clear(); basketX = 200; spawnT = 0; break;
    case MG_SIMON: seq.clear(); simonAdd(); break;
    case MG_TTT: for (int& b : board) b = 0; tttResult = 0; break;
    case MG_RUNWAY: want = irand(4); beatT = 1.5f; beatLen = 1.3f; streak = 0; break;
    case MG_NAILS: timer = 40; newNails(); nailFlash = 0; break;
    case MG_KARAOKE: timer = 32; knotes.clear(); kSpawn = 1.0f; combo = 0; misses = 0; kFlash = 0; break;
    case MG_VOLLEY: bx = 200; by = 70; bvx = 50; bvy = 0; padX = 200; break;
    case MG_SMOOTHIE: timer = 45; newOrder(); smFlash = 0; break;
    }
}

static int tttWin(int p) {
    static const int L[8][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}};
    for (auto& l : L) if (board[l[0]] == p && board[l[1]] == p && board[l[2]] == p) return 1;
    return 0;
}
static void tttAI() {
    for (int pass = 0; pass < 2; pass++) {   // win, then block
        int me = pass == 0 ? 2 : 1;
        for (int i = 0; i < 9; i++) if (!board[i]) { board[i] = me; bool w = tttWin(me); board[i] = 0; if (w) { board[i] = 2; return; } }
    }
    if (!board[4]) { board[4] = 2; return; }
    std::vector<int> e; for (int i = 0; i < 9; i++) if (!board[i]) e.push_back(i);
    if (!e.empty()) board[e[irand((int)e.size())]] = 2;
}

static const RGB PADS[4] = {HOT, GOLD, LILAC, BABYBLUE};
static Btn pad(int i) { return Btn{10 + (i % 2) * 152.f, 10 + (i / 2) * 96.f, 146, 90}; }
static Btn cardB(int i) { return Btn{8 + (i % 4) * 77.f, 6 + (i / 4) * 50.f, 72, 46}; }
static Btn cellB(int i) { return Btn{10 + (i % 3) * 102.f, 6 + (i / 3) * 66.f, 96, 62}; }
static Btn tttB(int i) { return Btn{70 + (i % 3) * 62.f, 14 + (i / 3) * 62.f, 58, 58}; }
static const Btn bQuit{232, 208, 84, 28}, bAgain{4, 208, 84, 28};
static const u32 RUNKEY[4] = {KEY_A, KEY_B, KEY_X, KEY_Y};
static const char* RUNNAME[4] = {"A", "B", "X", "Y"};

bool mgUpdate(const Input& in) {
    if ((in.down & KEY_START) || (in.touch && bQuit.in(in.tp))) { if (!over) g_mgLastScore = score; return false; }
    if (over) {
        if ((in.down & KEY_A) || (in.touch && bAgain.in(in.tp))) mgStart(cur);
        if (in.down & KEY_B) return false;
        return true;
    }
    const touchPosition& tp = in.tp;
    switch (cur) {
    case MG_MEMORY:
        if (waitT > 0) { waitT -= in.dt; if (waitT <= 0) { cstate[first] = cstate[second] = 0; first = second = -1; } break; }
        if (in.touch)
            for (int i = 0; i < 16; i++)
                if (cardB(i).in(tp) && cstate[i] == 0) {
                    sfxTap();
                    cstate[i] = 1;
                    if (first < 0) first = i;
                    else {
                        second = i; moves++;
                        if (cards[first] == cards[second]) { cstate[first] = cstate[second] = 2; first = second = -1; matched++; sfxYay(); if (matched == 8) { score = moves; finish(); } }
                        else waitT = 0.8f;
                    }
                    break;
                }
        break;
    case MG_PINATA:
        timer -= in.dt; pinT -= in.dt;
        if (pinT <= 0) { pinCell = irand(9); pinT = std::fmax(0.45f, 1.0f - score * 0.02f); }
        if (in.touch && pinCell >= 0 && cellB(pinCell).in(tp)) { score++; pinCell = -1; pinT = 0.25f; sfxYay(); fxBurst(200, 120, 10); }
        else if (in.touch) sfxNo();
        if (timer <= 0) finish();
        break;
    case MG_CATCH: {
        float mv = (in.held & KEY_LEFT ? -1.f : 0) + (in.held & KEY_RIGHT ? 1.f : 0);
        if (std::abs(in.cp.dx) > 20) mv = in.cp.dx / 156.f;
        if (in.held & KEY_TOUCH) basketX = in.tp.px * 1.25f;
        basketX = clampv(basketX + mv * 260 * in.dt, 24.f, 376.f);
        spawnT -= in.dt;
        if (spawnT <= 0) { drops.push_back({20 + frand() * 360, -10, 70 + score * 3.f + frand() * 40, irand(5) == 0 ? 1 : 0}); spawnT = std::fmax(0.35f, 0.9f - score * 0.015f); }
        for (size_t i = 0; i < drops.size();) {
            Drop& d = drops[i];
            d.y += d.vy * in.dt;
            if (d.y > 200 && d.y < 222 && std::fabs(d.x - basketX) < 28) {
                if (d.kind == 0) { score++; sfxTap(); } else { lives--; sfxNo(); }
                drops.erase(drops.begin() + i); continue;
            }
            if (d.y > 250) { if (d.kind == 0) lives--; drops.erase(drops.begin() + i); continue; }
            i++;
        }
        if (lives <= 0) finish();
        break;
    }
    case MG_SIMON:
        if (phase == 0) {
            showT -= in.dt;
            if (showT <= 0) {
                if (lit >= 0) { lit = -1; showT = 0.18f; showIdx++; }
                else if (showIdx < (int)seq.size()) { lit = seq[showIdx]; sfx(400 + lit * 150, 400 + lit * 150, 250); showT = 0.45f; }
                else phase = 1;
            }
        } else if (phase == 1) {
            if (waitT > 0) { waitT -= in.dt; if (waitT <= 0) lit = -1; }
            if (in.touch)
                for (int i = 0; i < 4; i++)
                    if (pad(i).in(tp)) {
                        lit = i; waitT = 0.2f;
                        if (seq[inIdx] == i) {
                            sfx(400 + i * 150, 400 + i * 150, 150);
                            if (++inIdx == (int)seq.size()) { score = (int)seq.size(); simonAdd(); showT = 0.9f; }
                        } else { sfxNo(); finish(); }
                    }
        }
        break;
    case MG_TTT:
        if (tttResult) break;
        if (in.touch)
            for (int i = 0; i < 9; i++)
                if (tttB(i).in(tp) && !board[i]) {
                    board[i] = 1; sfxTap();
                    bool full = std::none_of(board, board + 9, [](int b) { return b == 0; });
                    if (tttWin(1)) { tttResult = 1; score = 1; G.best[MG_TTT]++; note = "You win!"; over = true; sfxYay(); MC().needs[N_FUN] += 20; }
                    else if (full) { tttResult = 3; note = "Draw!"; over = true; }
                    else { tttAI(); if (tttWin(2)) { tttResult = 2; note = "Kiki wins!"; over = true; sfxNo(); } }
                    break;
                }
        break;
    case MG_NAILS: {
        timer -= in.dt; if (nailFlash > 0) nailFlash -= in.dt;
        if (in.touch)
            for (int i = 0; i < 6; i++) {
                Btn b{8 + (i % 3) * 102.f, 100 + (i / 3) * 50.f, 96, 44};
                if (!b.in(in.tp) || nailIdx >= 5) continue;
                nailHave[nailIdx++] = i; sfxTap();
                if (nailIdx == 5) {
                    bool ok = true; for (int k = 0; k < 5; k++) ok &= nailHave[k] == nailWant[k];
                    if (ok) { score++; sfxYay(); note = "Gorgeous!"; } else { sfxNo(); note = "Oops, smudged!"; }
                    nailFlash = 0.8f; newNails();
                }
            }
        if (timer <= 0) { note = ""; finish(); }
        break;
    }
    case MG_KARAOKE: {
        timer -= in.dt; kSpawn -= in.dt; if (kFlash > 0) kFlash -= in.dt;
        if (kSpawn <= 0 && timer > 2) { knotes.push_back({420, irand(4), false}); kSpawn = 0.45f + frand() * 0.5f; }
        static const u32 KK[4] = {KEY_DUP | KEY_X, KEY_DDOWN | KEY_B, KEY_DLEFT | KEY_Y, KEY_DRIGHT | KEY_A};
        int pressed = -1;
        for (int k = 0; k < 4; k++) if (in.down & KK[k]) pressed = k;
        if (in.touch) for (int k = 0; k < 4; k++) { Btn b{8 + k * 77.f, 110, 72, 72}; if (b.in(in.tp)) pressed = k; }
        for (KNote& n : knotes) n.x -= 140 * in.dt;
        if (pressed >= 0) {
            KNote* best = nullptr;
            for (KNote& n : knotes) if (!n.hit && std::fabs(n.x - 80) < 26 && (!best || std::fabs(n.x - 80) < std::fabs(best->x - 80))) best = &n;
            if (best && best->dir == pressed) { best->hit = true; combo++; score += std::fabs(best->x - 80) < 10 ? 3 : 1; note = std::fabs(best->x - 80) < 10 ? "PERFECT!" : "Good"; kFlash = 0.4f; sfx(500 + pressed * 120, 700 + pressed * 120, 120); }
            else { combo = 0; misses++; note = "Off-key!"; kFlash = 0.4f; sfxNo(); }
        }
        for (size_t i = 0; i < knotes.size();) {
            if (knotes[i].x < 40) { if (!knotes[i].hit) { combo = 0; misses++; } knotes.erase(knotes.begin() + i); }
            else i++;
        }
        if (timer <= 0) { note = combo > 5 ? "Encore!" : ""; finish(); }
        break;
    }
    case MG_VOLLEY: {
        float mv = (in.held & KEY_LEFT ? -1.f : 0) + (in.held & KEY_RIGHT ? 1.f : 0);
        if (std::abs(in.cp.dx) > 20) mv = in.cp.dx / 156.f;
        if (in.held & KEY_TOUCH) padX = in.tp.px * 1.25f;
        padX = clampv(padX + mv * 280 * in.dt, 30.f, 370.f);
        bvy += 220 * in.dt;
        bx += bvx * in.dt; by += bvy * in.dt;
        if (bx < 12 || bx > 388) { bvx = -bvx; bx = clampv(bx, 12.f, 388.f); }
        if (bvy > 0 && by > 186 && by < 204 && std::fabs(bx - padX) < 30) {
            score++; sfxTap();
            bvy = -(230 + std::min(score, 20) * 6.f);
            bvx = (bx - padX) * 4.f + (frand() - 0.5f) * 60;
            by = 186;
        }
        if (by > 250) { lives--; sfxNo(); bx = 200; by = 60; bvx = (frand() - 0.5f) * 120; bvy = 0; if (lives <= 0) finish(); }
        break;
    }
    case MG_SMOOTHIE: {
        timer -= in.dt; if (smFlash > 0) smFlash -= in.dt;
        if (in.touch) {
            for (int i = 0; i < 8; i++) {
                Btn b{6 + (i % 4) * 77.f, 30 + (i / 4) * 56.f, 72, 50};
                if (b.in(in.tp) && blendN < 3) { bool dup = false; for (int k = 0; k < blendN; k++) dup |= blend[k] == i; if (!dup) { blend[blendN++] = i; sfxTap(); } }
            }
            if (Btn{6, 150, 150, 40}.in(in.tp) && blendN == 3) {
                int match = 0; for (int a = 0; a < 3; a++) for (int c = 0; c < 3; c++) match += blend[a] == order[c];
                smGood = match == 3; smFlash = 0.8f;
                if (smGood) { score++; sfxYay(); note = "Delicious!"; } else { sfxNo(); note = "Wrong recipe!"; }
                newOrder();
            }
            if (Btn{164, 150, 150, 40}.in(in.tp)) { blendN = 0; sfxTap(); }
        }
        if (timer <= 0) { note = ""; finish(); }
        break;
    }
    case MG_RUNWAY: {
        beatT -= in.dt;
        u32 pressed = in.down & (KEY_A | KEY_B | KEY_X | KEY_Y);
        if (pressed) {
            if (pressed & RUNKEY[want]) { float acc = 1.f - std::fabs(beatT) / beatLen; score += acc > 0.7f ? 3 : 1; streak++; sfxTap(); note = acc > 0.7f ? "FIERCE!" : "Nice"; }
            else { lives--; streak = 0; sfxNo(); note = "Stumble!"; }
            want = irand(4); beatLen = std::fmax(0.55f, beatLen * 0.97f); beatT = beatLen;
        } else if (beatT < -beatLen * 0.5f) { lives--; streak = 0; note = "Too slow!"; want = irand(4); beatT = beatLen; sfxNo(); }
        if (lives <= 0) finish();
        break;
    }
    }
    return true;
}

static void cardSymbol(int k, float cx, float cy) {
    static const RGB cc[8] = {HOT, GOLD, LILAC, BABYBLUE, RED, MINT, BERRY, CHOC};
    u32 c = C(cc[k]);
    switch (k) {
    case 0: heart(cx, cy, 8, c); break;
    case 1: sparkle(cx, cy, 13, c); break;
    case 2: { Pen p(cx, cy + 10, 20, 20, 1.f); p.Q(0, 8, 14, 12, 16, 17, 2, 17, c); p.T(0, 4, 4, 8, 0, 17, c); p.L(2, 17, 0, 21, 2, c); break; }   // heel
    case 3: circle(cx, cy, 10, c); circle(cx, cy, 5, C(WHITE)); break;
    case 4: { Pen p(cx, cy + 12, 20, 24, 1.f); p.Q(4, 6, 16, 6, 18, 24, 2, 24, c); p.R(7, 0, 6, 6, C(GOLD)); break; }   // lipstick
    case 5: quad(cx, cy - 11, cx + 9, cy, cx, cy + 11, cx - 9, cy, c); break;   // gem
    case 6: { rect(cx - 10, cy - 4, 20, 14, c); line(cx - 6, cy - 4, cx, cy - 12, 2, c); line(cx + 6, cy - 4, cx, cy - 12, 2, c); break; }   // purse
    case 7: { circle(cx - 4, cy, 6, c); circle(cx + 5, cy - 2, 5, c); circle(cx, cy + 4, 5, c); break; }   // bonbons
    }
}

void mgDrawTop(float t) {
    grad(0, 0, TOP_W, TOP_H, C(UI2), C(UI1));
    for (int i = 0; i < 6; i++) swirl(30 + i * 70, 220, 14, 1.5f, 1.3f, C(CHAMP, 40), i % 2);
    textShadow(MG_NAMES[cur], 200, 8, 0.8f, C(BLUSH), C(BLACK), 1);
    char b[64];
    switch (cur) {
    case MG_MEMORY: snprintf(b, sizeof b, "Moves: %d   Pairs: %d/8   Best: %d", moves, matched, (int)G.best[cur]); break;
    case MG_PINATA: case MG_CATCH: case MG_RUNWAY: snprintf(b, sizeof b, "Score: %d   Best: %d", score, (int)G.best[cur]); break;
    case MG_SIMON: snprintf(b, sizeof b, "Combo: %d   Best: %d", score, (int)G.best[cur]); break;
    case MG_NAILS: case MG_KARAOKE: case MG_VOLLEY: case MG_SMOOTHIE: snprintf(b, sizeof b, "Score: %d   Best: %d", score, (int)G.best[cur]); break;
    default: snprintf(b, sizeof b, "Total wins: %d", (int)G.best[MG_TTT]); break;
    }
    text(b, 200, 38, 0.5f, C(CREAM), 1);
    if (cur == MG_NAILS) {
        snprintf(b, sizeof b, "Time: %d", (int)std::ceil(std::fmax(0.f, timer))); text(b, 340, 12, 0.5f, C(GOLD), 1);
        // customer's order card
        rrect(20, 64, 150, 58, 10, C(CREAM)); text("Order:", 95, 68, 0.45f, C(UI1), 1);
        for (int i = 0; i < 5; i++) rrect(30 + i * 27, 90, 20, 26, 8, C(NAIL_COLS[nailWant[i]]));
        // the hand
        rrect(200, 120, 150, 90, 30, C(242, 196, 162));
        for (int i = 0; i < 5; i++) {
            float fx = 206 + i * 28, fy = 70 + (i == 0 ? 50 : std::abs(2 - i) * 8);
            rrect(fx, fy, 22, 70, 10, C(242, 196, 162));
            rrect(fx + 3, fy + 3, 16, 20, 7, nailHave[i] >= 0 ? C(NAIL_COLS[nailHave[i]]) : C(255, 235, 230));
            if (i == nailIdx) sparkle(fx + 11, fy - 6, 5 + std::sin(t * 8) * 2, C(GOLD));
        }
        if (nailFlash > 0) text(note.c_str(), 95, 160, 0.6f, C(BLUSH), 1);
        text("Paint each nail to match the order!", 200, 222, 0.42f, C(CREAM), 1);
    } else if (cur == MG_KARAOKE) {
        snprintf(b, sizeof b, "Time: %d  Combo: %d", (int)std::ceil(std::fmax(0.f, timer)), combo); text(b, 200, 56, 0.45f, C(GOLD), 1);
        for (int i = 0; i < 4; i++) { float sx = 60 + i * 95 + std::sin(t * 2 + i) * 30; tri(sx, 0, sx - 40, 240, sx + 40, 240, C(i % 2 ? HOT : GOLD, 35)); }
        rect(0, 80, TOP_W, 50, C(30, 18, 30, 200));
        circle(80, 105, 22, C(255, 255, 255, (uint8_t)(kFlash > 0 ? 200 : 90)));
        static const char* AR[4] = {"^", "v", "<", ">"};
        static const RGB AC[4] = {{255, 58, 140}, {120, 190, 240}, {224, 184, 104}, {130, 220, 140}};
        for (const KNote& n : knotes) if (!n.hit) { circle(n.x, 105, 16, C(AC[n.dir])); text(AR[n.dir], n.x, 92, 0.7f, C(WHITE), 1); }
        drawSim(MC(), 320, 236, 0.75f, t, 0, kFlash > 0 ? PO_DANCE : PO_IDLE, true);
        rect(300, 150, 4, 40, C(GRAY)); circle(302, 148, 5, C(BLACK));
        if (kFlash > 0) text(note.c_str(), 160, 150, 0.7f, C(BLUSH), 1);
        text("Press the arrow (D-pad) as notes hit the circle!", 200, 222, 0.4f, C(CREAM), 1);
    } else if (cur == MG_VOLLEY) {
        grad(0, 0, TOP_W, 120, C(255, 200, 210), C(255, 230, 200));
        rect(0, 120, TOP_W, 120, C(250, 226, 180));
        rect(0, 110, TOP_W, 14, C(110, 190, 235));
        for (int i = 0; i < lives; i++) heart(370 - i * 16, 14, 5, C(HOT));
        text(MG_NAMES[cur], 200, 8, 0.8f, C(BERRY), 1);
        snprintf(b, sizeof b, "Bumps: %d   Best: %d", score, (int)G.best[cur]); text(b, 200, 38, 0.5f, C(UI1), 1);
        circle(bx, by, 9, C(255, 255, 255)); line(bx - 9, by, bx + 9, by, 1.5f, C(HOT)); line(bx, by - 9, bx, by + 9, 1.5f, C(BABYBLUE));
        ellipse(bx, 206, 18, 4, C(0, 0, 0, 40));
        drawSim(MC(), padX, 236, 0.45f, t, 0, PO_DANCE, false);
        text("Move to bump the ball up!", 200, 224, 0.42f, C(UI1), 1);
    } else if (cur == MG_SMOOTHIE) {
        snprintf(b, sizeof b, "Time: %d", (int)std::ceil(std::fmax(0.f, timer))); text(b, 340, 12, 0.5f, C(GOLD), 1);
        rrect(16, 64, 190, 90, 12, C(CREAM)); text("Customer wants:", 111, 68, 0.45f, C(UI1), 1);
        for (int i = 0; i < 3; i++) { circle(50 + i * 60, 110, 16, C(FRUITC[order[i]])); text(FRUIT[order[i]], 50 + i * 60, 128, 0.32f, C(UI1), 1); }
        // blender
        rrect(260, 70, 80, 110, 10, C(255, 255, 255, 120)); rect(270, 180, 60, 30, C(HOT));
        for (int i = 0; i < blendN; i++) rect(264, 170 - (i + 1) * 30, 72, 30, C(FRUITC[blend[i]], 230));
        if (smFlash > 0) text(note.c_str(), 111, 170, 0.6f, smGood ? C(GOLD) : C(BLUSH), 1);
        text("Tap 3 ingredients, then Blend!", 200, 222, 0.42f, C(CREAM), 1);
    } else if (cur == MG_CATCH) {
        for (int i = 0; i < lives; i++) heart(370 - i * 16, 14, 5, C(HOT));
        for (const Drop& d : drops) {
            if (d.kind == 0) { circle(d.x, d.y, 8, C(BLUSH)); circle(d.x, d.y - 4, 5, C(255, 255, 255)); sparkle(d.x + 6, d.y - 8, 3, C(GOLD)); }
            else { circle(d.x, d.y, 8, C(GREEN)); circle(d.x - 4, d.y - 4, 5, C(shade(GREEN, 0.8f))); text("x", d.x, d.y - 9, 0.5f, C(WHITE), 1); }
        }
        Pen p(basketX, 222, 56, 22, 1.f);
        p.Q(0, 0, 56, 0, 50, 22, 6, 22, C(HOT)); p.L(8, 0, 28, -14, 2, C(GOLD)); p.L(48, 0, 28, -14, 2, C(GOLD));
        zebraRect(basketX - 22, 208, 44, 6, C(CREAM), C(BLACK), 1);
        text("Move: D-pad / Circle Pad / stylus", 200, 226, 0.38f, C(CREAM, 160), 1);
    } else if (cur == MG_PINATA) {
        float sw = std::sin(t * 3) * 0.3f;
        float px = 200 + std::sin(sw) * 60, py = 140 + std::cos(sw) * 20;
        line(200, 60, px, py - 20, 1.5f, C(CREAM));
        Pen p(px, py + 20, 50, 40, 1.f);
        p.E(25, 22, 40, 26, C(HOT)); p.R(5, 16, 40, 4, C(GOLD)); p.R(5, 26, 40, 4, C(BLUSH)); p.Ci(42, 12, 7, C(HOT)); p.Ci(44, 10, 1.5f, C(BLACK));
        text("Tap the pinata when it pops up below!", 200, 200, 0.45f, C(CREAM), 1);
        snprintf(b, sizeof b, "Time: %d", (int)std::ceil(std::fmax(0.f, timer))); text(b, 200, 60, 0.5f, C(GOLD), 1);
    } else if (cur == MG_RUNWAY) {
        for (int i = 0; i < lives; i++) heart(370 - i * 16, 14, 5, C(HOT));
        rect(140, 60, 120, 180, C(240, 230, 230)); for (int i = 0; i < 6; i++) circle(130, 70 + i * 30, 3, C(255, 240, 200)), circle(270, 70 + i * 30, 3, C(255, 240, 200));
        Sim& me = MC();
        drawSim(me, 200, 150 + std::fmod(t, beatLen) * 30, 0.9f, t, 0, PO_IDLE, false);
        float k = clampv(beatT / beatLen, 0.f, 1.f);
        circle(330, 140, 26 + k * 20, C(HOT, 80)); circle(330, 140, 24, C(HOT));
        text(RUNNAME[want], 330, 126, 0.9f, C(WHITE), 1);
        text("Press the button as the ring closes!", 200, 222, 0.42f, C(CREAM), 1);
        if (!note.empty() && !over) text(note.c_str(), 70, 120, 0.6f, C(GOLD), 1);
    } else {
        text(cur == MG_SIMON ? (phase == 0 ? "Watch the pattern..." : "Your turn! Repeat it.") :
             cur == MG_MEMORY ? "Find the matching glam pairs!" : "You're X. Beat Kiki!", 200, 120, 0.6f, C(CREAM), 1);
        for (int i = 0; i < 5; i++) sparkle(60 + i * 70, 170 + (i % 2) * 20, 3 + std::fabs(std::sin(t * 3 + i)) * 4, C(GOLD));
    }
    if (over) {
        rrect(90, 80, 220, 80, 12, C(GOLD)); rrect(93, 83, 214, 74, 10, C(UI1));
        text(note.empty() ? "Game Over" : note.c_str(), 200, 92, 0.7f, C(BLUSH), 1);
        snprintf(b, sizeof b, cur == MG_MEMORY ? "Finished in %d moves" : "Score: %d", score);
        if (cur != MG_TTT) text(b, 200, 118, 0.5f, C(CREAM), 1);
        text("A: again   B: back", 200, 138, 0.45f, C(CHAMP), 1);
    }
}

void mgDrawBottom(float t) {
    bottomBackdrop(t);
    switch (cur) {
    case MG_MEMORY:
        for (int i = 0; i < 16; i++) {
            Btn c = cardB(i);
            if (cstate[i] == 0) { rrect(c.x, c.y, c.w, c.h, 6, C(GOLD)); rrect(c.x + 2, c.y + 2, c.w - 4, c.h - 4, 5, C(HOT)); zebraRect(c.x + 8, c.y + 8, c.w - 16, c.h - 16, C(BLUSH), C(BERRY, 120), i); }
            else { rrect(c.x, c.y, c.w, c.h, 6, C(cstate[i] == 2 ? GOLD : CREAM)); rrect(c.x + 2, c.y + 2, c.w - 4, c.h - 4, 5, C(CREAM)); cardSymbol(cards[i], c.x + c.w / 2, c.y + c.h / 2); }
        }
        break;
    case MG_PINATA:
        for (int i = 0; i < 9; i++) {
            Btn c = cellB(i);
            rrect(c.x, c.y, c.w, c.h, 10, C(UI2)); ellipse(c.x + c.w / 2, c.y + c.h - 12, 70, 16, C(BLACK, 160));
            if (i == pinCell) {
                Pen p(c.x + c.w / 2, c.y + c.h - 8, 50, 40, 1.f);
                p.E(25, 22, 40, 26, C(HOT)); p.R(5, 16, 40, 4, C(GOLD)); p.R(5, 26, 40, 4, C(BLUSH)); p.Ci(42, 12, 7, C(HOT)); p.Ci(44, 10, 1.5f, C(BLACK));
            }
        }
        break;
    case MG_SIMON:
        for (int i = 0; i < 4; i++) { Btn p = pad(i); rrect(p.x, p.y, p.w, p.h, 16, C(PADS[i], lit == i ? 255 : 130)); if (lit == i) sparkle(p.x + p.w / 2, p.y + p.h / 2, 18, C(255, 255, 255, 220)); }
        break;
    case MG_TTT:
        for (int i = 0; i < 9; i++) {
            Btn c = tttB(i);
            rrect(c.x, c.y, c.w, c.h, 8, C(UI2));
            float cx = c.x + c.w / 2, cy = c.y + c.h / 2;
            if (board[i] == 1) { line(cx - 16, cy - 16, cx + 16, cy + 16, 5, C(HOT)); line(cx + 16, cy - 16, cx - 16, cy + 16, 5, C(HOT)); }
            if (board[i] == 2) heart(cx, cy, 14, C(GOLD));
        }
        break;
    case MG_NAILS:
        text("Polish colors", 160, 80, 0.5f, C(CREAM), 1);
        for (int i = 0; i < 6; i++) {
            Btn b{8 + (i % 3) * 102.f, 100 + (i / 3) * 50.f, 96, 44};
            rrect(b.x, b.y, b.w, b.h, 10, C(GOLD)); rrect(b.x + 2, b.y + 2, b.w - 4, b.h - 4, 9, C(NAIL_COLS[i]));
            rrect(b.x + 40, b.y - 6, 16, 10, 3, C(UI1));
        }
        break;
    case MG_KARAOKE: {
        static const char* AR[4] = {"^", "v", "<", ">"};
        static const RGB AC[4] = {{255, 58, 140}, {120, 190, 240}, {224, 184, 104}, {130, 220, 140}};
        for (int k = 0; k < 4; k++) { Btn b{8 + k * 77.f, 110, 72, 72}; rrect(b.x, b.y, b.w, b.h, 16, C(AC[k])); text(AR[k], b.x + 36, b.y + 16, 1.2f, C(WHITE), 1); }
        text("D-pad or tap the arrows", 160, 80, 0.5f, C(CREAM), 1);
        break;
    }
    case MG_VOLLEY:
        text("D-pad, Circle Pad or drag here", 160, 90, 0.5f, C(CREAM), 1);
        circle(160, 150, 30, C(255, 255, 255)); line(130, 150, 190, 150, 3, C(HOT)); line(160, 120, 160, 180, 3, C(BABYBLUE));
        break;
    case MG_SMOOTHIE:
        for (int i = 0; i < 8; i++) {
            Btn b{6 + (i % 4) * 77.f, 30 + (i / 4) * 56.f, 72, 50};
            rrect(b.x, b.y, b.w, b.h, 10, C(UI2)); circle(b.x + 36, b.y + 20, 13, C(FRUITC[i]));
            text(FRUIT[i], b.x + 36, b.y + 34, 0.34f, C(CREAM), 1);
        }
        button(Btn{6, 150, 150, 40}, "Blend!", blendN == 3, 2, 0.55f);
        button(Btn{164, 150, 150, 40}, "Dump", false, 1, 0.5f);
        break;
    case MG_CATCH: case MG_RUNWAY:
        text(cur == MG_CATCH ? "Catch sweets, dodge the broccoli!" : "Strut to the beat!", 160, 90, 0.55f, C(CREAM), 1);
        heart(160, 140, 20, C(HOT, 120));
        break;
    }
    button(bAgain, "Restart", false, 2, 0.45f);
    button(bQuit, "Quit", false, 1, 0.45f);
}
