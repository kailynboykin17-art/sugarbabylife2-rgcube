#include "rivals.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include <cmath>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
using namespace col;

namespace {
enum Ab { AB_DASH, AB_BOMB, AB_HEALPULSE, AB_TEAMSPEED, AB_ARMOR, AB_CHARGE, AB_BLINK, AB_INVIS, AB_HYPE, AB_PUSH, AB_FREEZE, AB_SELFHEAL, AB_FLY, AB_HEX, AB_ALLYSHIELD };
enum Ul { U_RING, U_TEAMSHIELD, U_SLAM, U_STORM, U_TEAMHEAL, U_ZONE, U_SNIPE };
struct Hero { const char* name; const char* role; RGB col; int hp; float spd, rate, range; int dmg; const char* a1; float cd1; const char* a2; float cd2; const char* ult; const char* blurb; uint8_t t1, t2, tu; bool ice; };
const Hero HEROES[] = {
    {"Glitter Gunner", "Duelist", {255, 90, 170}, 250, 96, 0.15f, 170, 11, "Dash", 5, "Glitter Bomb", 8, "Disco Barrage", "Rapid sparkle blaster. Dash in, bomb, barrage.", AB_DASH, AB_BOMB, U_RING, false},
    {"Bubble Medic", "Strategist", {120, 210, 255}, 220, 92, 0.25f, 160, 9, "Heal Pulse", 7, "Speed Bubble", 10, "Bubble Shield", "Heals the team, shields everyone with her ult.", AB_HEALPULSE, AB_TEAMSPEED, U_TEAMSHIELD, false},
    {"Velvet Brawler", "Vanguard", {150, 70, 200}, 450, 80, 0.5f, 34, 32, "Barrier", 8, "Charge", 7, "Ground Slam", "Big tank. Block damage, charge in, slam & stun.", AB_ARMOR, AB_CHARGE, U_SLAM, false},
    {"Neon Ninja", "Duelist", {80, 255, 160}, 230, 112, 0.3f, 150, 18, "Blink", 4, "Smoke Veil", 10, "Blade Storm", "Fast flanker. Blink, go invisible, spin to win.", AB_BLINK, AB_INVIS, U_STORM, false},
    {"Y2K Popstar", "Strategist", {255, 200, 60}, 230, 94, 0.2f, 150, 10, "Hype Up", 10, "Sonic Push", 6, "Encore", "Buffs allies' damage, pushes enemies, full-team heal.", AB_HYPE, AB_PUSH, U_TEAMHEAL, false},
    {"Frost Princess", "Vanguard", {180, 230, 255}, 320, 86, 0.3f, 150, 14, "Freeze", 9, "Ice Armor", 8, "Blizzard", "Slows with ice, freezes a target, blizzard zone.", AB_FREEZE, AB_SELFHEAL, U_ZONE, true},
    {"Hex Queen", "Duelist", {230, 40, 90}, 240, 90, 0.1f, 140, 7, "Hex Field", 7, "Phase Step", 6, "Chaos Burst", "Chaos-magic zoner: melts anyone who stands in her hex.", AB_HEX, AB_BLINK, U_SLAM, false},
    {"Diamond Duchess", "Vanguard", {235, 240, 255}, 480, 82, 0.35f, 120, 16, "Diamond Form", 8, "Mind Shove", 7, "Crystal Wall", "Diamond-skin tank. Shrugs off hits, shoves, shields the team.", AB_ARMOR, AB_PUSH, U_TEAMSHIELD, false},
    {"Snow Idol", "Strategist", {170, 210, 255}, 230, 94, 0.25f, 160, 11, "Ice Encore", 7, "Flash Freeze", 9, "Fan Service", "K-pop ice idol: heals with songs, freezes with style.", AB_HEALPULSE, AB_FREEZE, U_TEAMHEAL, true},
    {"Black Lace", "Duelist", {60, 50, 70}, 220, 92, 1.1f, 260, 55, "Grapple", 6, "Vanish", 11, "Kill Shot", "Long-range sniper. Huge hits, slow trigger, one-shot ult.", AB_DASH, AB_INVIS, U_SNIPE, false},
    {"Tempest", "Duelist", {200, 220, 255}, 250, 94, 0.25f, 160, 12, "Lightning", 6, "Tailwind", 10, "Superstorm", "Weather queen: lightning strikes and a map-wide storm.", AB_BOMB, AB_TEAMSPEED, U_ZONE, false},
    {"Veil", "Strategist", {170, 120, 255}, 230, 92, 0.22f, 150, 9, "Force Bubble", 8, "Fade Out", 10, "Force Dome", "Force fields & stealth: shields allies, turns invisible.", AB_ALLYSHIELD, AB_INVIS, U_TEAMSHIELD, false},
    {"Silk Swinger", "Duelist", {255, 70, 70}, 240, 110, 0.25f, 120, 14, "Swing", 3, "Zip Kick", 6, "Combo Rush", "Super-mobile brawler: swing in, kick, burst combo.", AB_DASH, AB_CHARGE, U_RING, false},
    {"Jet Glam", "Duelist", {255, 160, 50}, 260, 96, 0.18f, 200, 13, "Take Flight", 9, "Mini Missiles", 7, "Mega Beam", "Flying blaster: soars above fights and rains down beams.", AB_FLY, AB_BOMB, U_RING, false},
    {"Panthera", "Duelist", {110, 60, 160}, 280, 108, 0.35f, 34, 28, "Pounce", 3, "Claw Dive", 5, "Spirit Rend", "Melee assassin: dash resets and huge close-up burst.", AB_DASH, AB_CHARGE, U_SLAM, false},
    {"Tide King", "Duelist", {40, 170, 170}, 300, 88, 0.3f, 160, 16, "Whirlpool", 8, "Tidal Wave", 7, "Ocean's Wrath", "Trident king: whirlpools slow, waves push, tsunami ult.", AB_HEX, AB_PUSH, U_ZONE, false},
    {"Razor Claw", "Vanguard", {240, 200, 60}, 400, 96, 0.3f, 34, 24, "Leap", 6, "Regen", 9, "Berserk", "Regenerating berserker: leap in, heal up, shred everyone.", AB_CHARGE, AB_SELFHEAL, U_STORM, false},
};
constexpr int NHERO = sizeof HEROES / sizeof HEROES[0];
Sim* heroDoll(int k) {   // a real SBL2 doll for each hero (built once, drawn with the game's doll painter)
    static Sim D[NHERO]; static bool made[NHERO] = {};
    if (!made[k]) { const bool guy = k == 15 || k == 16; uint32_t save = 0; (void)save;
        D[k] = randomSim(ST_YA, guy ? 1 : 0); D[k].name = HEROES[k].name; D[k].bio = "Sugar Rivals hero"; D[k].household = false; made[k] = true; }
    return &D[k];
}
struct Unit { int hero, team; float x, y, hp, cd = 0, c1 = 0, c2 = 0, ult = 0, respawn = 0, stun = 0, slow = 0, shield = 0, armor = 0, invis = 0, fast = 0, hype = 0, storm = 0, strafe = 0; bool bot = true; int kos = 0, deaths = 0; float fx = 1, fy = 0; };
struct Shot { float x, y, vx, vy, life; int team, dmg, owner; bool ice; };
struct Pop { float x, y, life; std::string s; RGB c; };
struct Zone { float x, y, r, life; int team, owner; };   // blizzard
std::vector<Unit> U; std::vector<Shot> S; std::vector<Pop> P; std::vector<Zone> Z;
int selPage = 0; int screen = 0, pickHero = 0, diff = 1, score[2] = {0, 0}, winner = -1; float timeLeft = 0, shake = 0;
const int GOAL = 12;
struct Wall { float x, y, w, h; };
const Wall WALLS[] = {{120, 50, 22, 60}, {258, 130, 22, 60}, {180, 105, 40, 30}, {60, 170, 50, 16}, {290, 54, 50, 16}, {0, 0, 400, 6}, {0, 234, 400, 6}, {0, 0, 6, 240}, {394, 0, 6, 240}};
constexpr int NW = sizeof WALLS / sizeof WALLS[0];

bool inWall(float x, float y, float r) { for (const Wall& w : WALLS) if (x + r > w.x && x - r < w.x + w.w && y + r > w.y && y - r < w.y + w.h) return true; return false; }
void moveU(Unit& u, float dx, float dy) {
    const float r = 9; if (!inWall(u.x + dx, u.y, r)) u.x += dx; if (!inWall(u.x, u.y + dy, r)) u.y += dy;
}
bool alive(const Unit& u) { return u.respawn <= 0; }
float dist(const Unit& a, const Unit& b) { return std::hypot(a.x - b.x, a.y - b.y); }
bool los(float x0, float y0, float x1, float y1) { for (int i = 1; i < 12; i++) { const float k = i / 12.f; if (inWall(x0 + (x1 - x0) * k, y0 + (y1 - y0) * k, 1)) return false; } return true; }
int nearestEnemy(int i, float maxR = 9999) {
    int best = -1; float bd = maxR;
    for (int j = 0; j < (int)U.size(); j++) { if (U[j].team == U[i].team || !alive(U[j]) || U[j].invis > 0) continue; const float d = dist(U[i], U[j]); if (d < bd && los(U[i].x, U[i].y, U[j].x, U[j].y)) { bd = d; best = j; } }
    return best;
}
void spawnPos(Unit& u) { u.x = u.team == 0 ? 30 + irand(20) : 350 + irand(20); u.y = 60 + irand(120); while (inWall(u.x, u.y, 9)) u.y += 7; }
void pop(float x, float y, const std::string& s, RGB c) { if (P.size() < 40) P.push_back({x, y, 0.9f, s, c}); }
void hurt(int t, float d, int from) {
    Unit& v = U[t]; if (!alive(v) || v.shield > 0) { if (v.shield > 0) pop(v.x, v.y - 14, "BLOCK", {200, 230, 255}); return; }
    if (from >= 0 && U[from].hype > 0) d *= 1.3f;
    if (v.armor > 0) d *= 0.2f;
    v.hp -= d; pop(v.x + irand(8) - 4, v.y - 16, std::to_string((int)d), {255, 240, 120});
    if (from >= 0) U[from].ult = std::min(100.f, U[from].ult + d * 0.12f);
    if (v.hp <= 0) {
        v.respawn = 3.5f; v.deaths++; if (from >= 0) { U[from].kos++; score[U[from].team]++; }
        pop(v.x, v.y - 24, "KO!", {255, 80, 120}); shake = 0.25f; if (t == 0 || from == 0) sfxNo();
    }
}
void heal(int t, float h, int from) { Unit& v = U[t]; if (!alive(v)) return; const float mx = HEROES[v.hero].hp; const float before = v.hp; v.hp = std::min(mx, v.hp + h); if (v.hp > before) { pop(v.x, v.y - 16, "+" + std::to_string((int)(v.hp - before)), {120, 255, 150}); if (from >= 0) U[from].ult = std::min(100.f, U[from].ult + (v.hp - before) * 0.15f); } }

void fire(int i, int tgt) {
    Unit& u = U[i]; const Hero& h = HEROES[u.hero]; if (u.cd > 0 || u.stun > 0) return;
    u.cd = h.rate;
    if (tgt < 0) return;
    Unit& v = U[tgt]; const float dx = v.x - u.x, dy = v.y - u.y, d = std::max(1.f, std::hypot(dx, dy));
    u.fx = dx / d; u.fy = dy / d;
    if (h.range < 50) { if (d < h.range + 10) hurt(tgt, h.dmg, i); return; }   // melee punch
    float spread = diff == 0 && u.bot ? 0.25f : diff == 1 && u.bot ? 0.12f : 0.05f; const float a = std::atan2(dy, dx) + (irand(100) - 50) / 50.f * spread;
    if (S.size() < 120) S.push_back({u.x, u.y, std::cos(a) * 300, std::sin(a) * 300, h.range / 300.f, u.team, h.dmg, i, u.hero == 5});
}
void ability(int i, int k) {
    Unit& u = U[i]; const Hero& h = HEROES[u.hero]; float& cd = k == 1 ? u.c1 : u.c2; if (cd > 0 || u.stun > 0 || !alive(u)) return;
    cd = k == 1 ? h.cd1 : h.cd2; const int e = nearestEnemy(i, 220); const char* nm = k == 1 ? h.a1 : h.a2;
    switch (k == 1 ? h.t1 : h.t2) {
    case AB_DASH: for (int s = 0; s < 14; s++) moveU(u, u.fx * 10, u.fy * 10); pop(u.x, u.y - 14, nm, h.col); break;
    case AB_BOMB: if (e >= 0) { for (int j = 0; j < (int)U.size(); j++) if (U[j].team != u.team && std::hypot(U[j].x - U[e].x, U[j].y - U[e].y) < 40) hurt(j, 60, i); pop(U[e].x, U[e].y, "BOOM", h.col); } else cd = 0.5f; break;
    case AB_HEALPULSE: for (int j = 0; j < (int)U.size(); j++) if (U[j].team == u.team && dist(u, U[j]) < 80) heal(j, 70, i); break;
    case AB_TEAMSPEED: for (auto& a : U) if (a.team == u.team && dist(u, a) < 90) a.fast = 3; pop(u.x, u.y - 14, nm, h.col); break;
    case AB_ARMOR: u.armor = 2.5f; pop(u.x, u.y - 14, nm, h.col); break;
    case AB_CHARGE: if (e >= 0) { const float dx = U[e].x - u.x, dy = U[e].y - u.y, d = std::max(1.f, std::hypot(dx, dy)); for (int s = 0; s < 16 && std::hypot(U[e].x - u.x, U[e].y - u.y) > 18; s++) moveU(u, dx / d * 10, dy / d * 10); if (dist(u, U[e]) < 26) { hurt(e, 45, i); U[e].stun = 0.6f; } } else cd = 0.5f; break;
    case AB_BLINK: { const float ox = u.x, oy = u.y; for (int s = 0; s < 10; s++) moveU(u, u.fx * 10, u.fy * 10); if (u.x == ox && u.y == oy) cd = 0.5f; pop(u.x, u.y - 14, nm, h.col); break; }
    case AB_INVIS: u.invis = 2.5f; break;
    case AB_HYPE: for (auto& a : U) if (a.team == u.team && dist(u, a) < 100) a.hype = 4; pop(u.x, u.y - 14, nm, h.col); break;
    case AB_PUSH: for (int j = 0; j < (int)U.size(); j++) if (U[j].team != u.team && alive(U[j]) && dist(u, U[j]) < 65) { const float dx = U[j].x - u.x, dy = U[j].y - u.y, d = std::max(1.f, std::hypot(dx, dy)); for (int s = 0; s < 6; s++) moveU(U[j], dx / d * 10, dy / d * 10); hurt(j, 25, i); } break;
    case AB_FREEZE: if (e >= 0) { U[e].stun = 1.3f; pop(U[e].x, U[e].y - 14, "FROZEN", {180, 230, 255}); } else cd = 0.5f; break;
    case AB_SELFHEAL: heal(i, 90, i); u.armor = 1.5f; break;
    case AB_FLY: u.armor = 3; u.fast = 3; pop(u.x, u.y - 14, "FLYING!", h.col); break;
    case AB_HEX: if (e >= 0) Z.push_back({U[e].x, U[e].y, 45, 3, u.team, i}); else cd = 0.5f; break;
    case AB_ALLYSHIELD: { int best = i; float lo = 2; for (int j = 0; j < (int)U.size(); j++) if (U[j].team == u.team && alive(U[j]) && dist(u, U[j]) < 120) { const float f = U[j].hp / HEROES[U[j].hero].hp; if (f < lo) { lo = f; best = j; } } U[best].shield = 2; pop(U[best].x, U[best].y - 14, "SHIELDED", h.col); break; }
    }
}
void ultimate(int i) {
    Unit& u = U[i]; if (u.ult < 100 || !alive(u) || u.stun > 0) return; u.ult = 0; shake = 0.3f; sfxYay();
    pop(u.x, u.y - 26, std::string(HEROES[u.hero].ult) + "!", {255, 255, 255});
    switch (HEROES[u.hero].tu) {
    case U_RING: for (int s = 0; s < 18; s++) { const float a = s * 6.2832f / 18; if (S.size() < 140) S.push_back({u.x, u.y, std::cos(a) * 260, std::sin(a) * 260, 0.7f, u.team, 30, i, false}); } break;
    case U_TEAMSHIELD: for (auto& a : U) if (a.team == u.team) a.shield = 3; break;
    case U_SLAM: for (int j = 0; j < (int)U.size(); j++) if (U[j].team != u.team && dist(u, U[j]) < 95) { hurt(j, 90, i); U[j].stun = 1.5f; } break;
    case U_STORM: u.storm = 3; break;
    case U_TEAMHEAL: for (int j = 0; j < (int)U.size(); j++) if (U[j].team == u.team) heal(j, 999, i); break;
    case U_ZONE: Z.push_back({u.x, u.y, 100, 3.5f, u.team, i}); break;
    case U_SNIPE: { int best = -1; float bd = 1e9f; for (int j = 0; j < (int)U.size(); j++) if (U[j].team != u.team && alive(U[j]) && los(u.x, u.y, U[j].x, U[j].y)) { const float d = dist(u, U[j]); if (d < bd) { bd = d; best = j; } }
        if (best >= 0) { hurt(best, 180, i); pop(U[best].x, U[best].y - 20, "HEADSHOT", {255, 80, 80}); } else u.ult = 100; break; }
    }
}

void startMatch() {
    U.clear(); S.clear(); P.clear(); Z.clear(); score[0] = score[1] = 0; winner = -1; timeLeft = 240;
    std::vector<int> pool; for (int k = 0; k < NHERO; k++) pool.push_back(k);
    auto take = [&](int h) { pool.erase(std::remove(pool.begin(), pool.end(), h), pool.end()); };
    take(pickHero);
    auto rnd = [&]() { if (pool.empty()) return irand(NHERO); const int k = irand((int)pool.size()); const int h = pool[k]; pool.erase(pool.begin() + k); return h; };
    const int hs[6] = {pickHero, rnd(), rnd(), rnd(), rnd(), rnd()};
    for (int i = 0; i < 6; i++) { Unit u; u.hero = hs[i]; u.team = i < 3 ? 0 : 1; u.hp = HEROES[u.hero].hp; u.bot = i != 0; spawnPos(u); u.strafe = irand(628) / 100.f; U.push_back(u); }
    screen = 1;
}

void botThink(int i, float dt) {
    Unit& u = U[i]; const Hero& h = HEROES[u.hero];
    // support: chase a hurt ally
    if (h.role[0] == 'S') { for (int j = 0; j < (int)U.size(); j++) if (U[j].team == u.team && j != i && alive(U[j]) && U[j].hp < HEROES[U[j].hero].hp * 0.55f && dist(u, U[j]) < 90) { ability(i, 1); break; } }
    const int e = nearestEnemy(i);
    const float react = diff == 0 ? 0.45f : diff == 1 ? 0.75f : 1.f;
    float mx = 0, my = 0;
    if (e >= 0) {
        const Unit& v = U[e]; const float d = dist(u, v), want = std::max(24.f, h.range * 0.7f);
        const float dx = (v.x - u.x) / std::max(1.f, d), dy = (v.y - u.y) / std::max(1.f, d);
        if (d > want) { mx += dx; my += dy; } else if (d < want * 0.6f) { mx -= dx; my -= dy; }
        u.strafe += dt * 1.7f; mx += -dy * std::sin(u.strafe) * 0.8f; my += dx * std::sin(u.strafe) * 0.8f;
        if (d < h.range + 5 && irand(100) < 100 * react) fire(i, e);
        if (irand(1000) < 8 * react) ability(i, 1); if (irand(1000) < 6 * react) ability(i, 2);
        if (u.ult >= 100 && d < 90) ultimate(i);
    } else {   // nobody in sight: hunt the nearest enemy (through walls) and steer around obstacles
        int any = -1; float bd = 1e9f; for (int j = 0; j < (int)U.size(); j++) if (U[j].team != u.team && alive(U[j])) { const float d = dist(u, U[j]); if (d < bd) { bd = d; any = j; } }
        const float gx = (any >= 0 ? U[any].x : 200) - u.x, gy = (any >= 0 ? U[any].y : 120) - u.y, d = std::max(1.f, std::hypot(gx, gy)); mx = gx / d; my = gy / d;
    }
    const float l = std::max(1.f, std::hypot(mx, my)); const float sp = h.spd * (u.slow > 0 ? 0.55f : 1) * (u.fast > 0 ? 1.4f : 1) * dt;
    const float ox = u.x, oy = u.y;
    moveU(u, mx / l * sp, my / l * sp);
    if (std::fabs(u.x - ox) + std::fabs(u.y - oy) < sp * 0.3f) {   // blocked: slide along the wall (keep the same side for a while)
        const float side = std::sin(u.strafe * 0.3f) >= 0 ? 1.f : -1.f;
        moveU(u, -my / l * sp * side * 1.5f, mx / l * sp * side * 1.5f);
        if (std::fabs(u.x - ox) + std::fabs(u.y - oy) < sp * 0.3f) moveU(u, my / l * sp * side * 1.5f, -mx / l * sp * side * 1.5f);
    }
}

Btn HB(int k) { return Btn{8.f + (k % 2) * 156, 34.f + (k / 2) * 46, 150, 42}; }
Btn AB(int k) { static const float X[4] = {6, 84, 162, 240}; return Btn{X[k], 196, 74, 40}; }
}

void rivalsOpen() { screen = 0; }

bool rivalsUpdate(const Input& in) {
    const float dt = std::min(0.05f, in.dt); const touchPosition tp = in.tp;
    if (screen == 0) {
        if (in.down & KEY_B) return false;
        for (int k = 0; k < 6; k++) if (in.touch && HB(k).in(tp) && selPage * 6 + k < NHERO) { pickHero = selPage * 6 + k; sfxTap(); }
        if ((in.down & KEY_R) || (in.touch && Btn{262, 4, 52, 24}.in(tp))) { selPage = (selPage + 1) % ((NHERO + 5) / 6); sfxTap(); }
        if ((in.down & KEY_L) || (in.touch && Btn{6, 4, 52, 24}.in(tp))) { selPage = (selPage + (NHERO + 5) / 6 - 1) % ((NHERO + 5) / 6); sfxTap(); }
        if (in.touch && Btn{8, 172, 100, 30}.in(tp)) { diff = (diff + 1) % 3; sfxTap(); }
        if ((in.down & KEY_A) || (in.touch && Btn{170, 172, 142, 30}.in(tp))) { startMatch(); sfxYay(); }
        if (in.touch && Btn{8, 206, 100, 28}.in(tp)) return false;
        return true;
    }
    if (screen == 2) { if ((in.down & (KEY_A | KEY_B)) || in.touch) screen = 0; return true; }
    if (in.down & KEY_START) { screen = 0; return true; }
    timeLeft -= dt; shake = std::max(0.f, shake - dt);
    for (int i = 0; i < (int)U.size(); i++) {
        Unit& u = U[i];
        for (float* f : {&u.cd, &u.c1, &u.c2, &u.stun, &u.slow, &u.shield, &u.armor, &u.invis, &u.fast, &u.hype}) *f = std::max(0.f, *f - dt);
        u.ult = std::min(100.f, u.ult + dt * 1.2f);
        if (u.respawn > 0) { u.respawn -= dt; if (u.respawn <= 0) { u.hp = HEROES[u.hero].hp; spawnPos(u); u.stun = 0; } continue; }
        if (u.storm > 0) { u.storm -= dt; static float tick = 0; tick += dt; if (tick > 0.2f) { tick = 0; for (int j = 0; j < (int)U.size(); j++) if (U[j].team != u.team && dist(u, U[j]) < 52) hurt(j, 16, i); } }
        if (u.stun > 0) continue;
        if (u.bot) { botThink(i, dt); continue; }
        // player
        float mx = 0, my = 0;
        if (std::abs(in.cp.dx) > 15 || std::abs(in.cp.dy) > 15) { mx = in.cp.dx / 156.f; my = -in.cp.dy / 156.f; }
        if (in.held & KEY_DLEFT) mx = -1; if (in.held & KEY_DRIGHT) mx = 1; if (in.held & KEY_DUP) my = -1; if (in.held & KEY_DDOWN) my = 1;
        const float l = std::hypot(mx, my); if (l > 0.1f) { const float sp = HEROES[u.hero].spd * (u.slow > 0 ? 0.55f : 1) * (u.fast > 0 ? 1.4f : 1) * dt; moveU(u, mx / std::max(1.f, l) * sp, my / std::max(1.f, l) * sp); u.fx = mx / l; u.fy = my / l; }
        const int e = nearestEnemy(i, HEROES[u.hero].range + 10);
        if ((in.held & KEY_A) || (in.held & KEY_ZR) || (in.touch && AB(0).in(tp))) fire(i, e);
        if ((in.down & KEY_X) || (in.touch && AB(1).in(tp))) ability(i, 1);
        if ((in.down & KEY_Y) || (in.touch && AB(2).in(tp))) ability(i, 2);
        if ((in.down & KEY_R) || (in.touch && AB(3).in(tp))) ultimate(i);
    }
    for (auto& s : S) {
        s.x += s.vx * dt; s.y += s.vy * dt; s.life -= dt;
        if (inWall(s.x, s.y, 1)) { s.life = 0; continue; }
        for (int j = 0; j < (int)U.size(); j++) if (U[j].team != s.team && alive(U[j]) && std::hypot(U[j].x - s.x, U[j].y - s.y) < 10) { hurt(j, s.dmg, s.owner); if (s.ice) U[j].slow = 1.2f; s.life = 0; break; }
    }
    S.erase(std::remove_if(S.begin(), S.end(), [](const Shot& s) { return s.life <= 0; }), S.end());
    for (auto& z : Z) { z.life -= dt; static float zt = 0; zt += dt; if (zt > 0.3f) { zt = 0; for (int j = 0; j < (int)U.size(); j++) if (U[j].team != z.team && alive(U[j]) && std::hypot(U[j].x - z.x, U[j].y - z.y) < z.r) { hurt(j, 10, z.owner); U[j].slow = 0.6f; } } }
    Z.erase(std::remove_if(Z.begin(), Z.end(), [](const Zone& z) { return z.life <= 0; }), Z.end());
    for (auto& p : P) { p.life -= dt; p.y -= 18 * dt; }
    P.erase(std::remove_if(P.begin(), P.end(), [](const Pop& p) { return p.life <= 0; }), P.end());
    if (score[0] >= GOAL || score[1] >= GOAL || timeLeft <= 0) {
        winner = score[0] > score[1] ? 0 : score[1] > score[0] ? 1 : 2; screen = 2;
        const int pay = winner == 0 ? 300 : winner == 2 ? 150 : 75; G.money += pay;
        toast(winner == 0 ? "VICTORY! +$" + std::to_string(pay) : winner == 2 ? "Draw! +$150" : "Defeat... +$75 for trying"); if (winner == 0) sfxYay();
    }
    return true;
}

void rivalsDrawTop(float t) {
    const float sx = shake > 0 ? (irand(5) - 2) : 0;
    grad(0, 0, TOP_W, TOP_H, C(255, 214, 236), C(200, 170, 255));
    for (int x = 0; x < TOP_W; x += 25) rect(x + sx, 0, 1, TOP_H, C(255, 255, 255, 60));
    for (int y = 0; y < TOP_H; y += 25) rect(0, y, TOP_W, 1, C(255, 255, 255, 60));
    rect(6, 6, 40, 228, C(90, 160, 255, 50)); rect(354, 6, 40, 228, C(255, 90, 120, 50));
    if (screen == 0) {
        const Hero& h = HEROES[pickHero];
        textOutline("SUGAR RIVALS", 200, 18, 1.0f, C(255, 90, 170), C(255, 255, 255), 1);
        text("3v3 hero arena - first team to 12 KOs", 200, 50, 0.45f, C(80, 40, 90), 1);
        circle(200, 118, 38, C(h.col)); drawSimPortrait(*heroDoll(pickHero), 200, 118, 34, t);
        textOutline(h.name, 200, 160, 0.7f, C(255, 255, 255), C(h.col.r / 2, h.col.g / 2, h.col.b / 2), 1);
        text((std::string(h.role) + "  -  HP " + std::to_string(h.hp)).c_str(), 200, 182, 0.42f, C(80, 40, 90), 1);
        text(h.blurb, 200, 198, 0.38f, C(80, 40, 90), 1);
        text((std::string("X: ") + h.a1 + "   Y: " + h.a2 + "   R: " + h.ult).c_str(), 200, 214, 0.38f, C(120, 40, 120), 1);
        return;
    }
    for (const Wall& w : WALLS) if (w.w < 300 && w.h < 200 && w.w > 8 && w.h > 8) { rrect(w.x + sx, w.y, w.w, w.h, 5, C(120, 80, 170)); rrect(w.x + 2 + sx, w.y + 2, w.w - 4, w.h * 0.4f, 4, C(170, 130, 220)); }
    for (const Zone& z : Z) circle(z.x + sx, z.y, z.r, C(200, 240, 255, 70));
    for (const Shot& s : S) circle(s.x + sx, s.y, s.ice ? 3 : 2.5f, C(s.team ? RGB{255, 90, 110} : RGB{90, 170, 255}));
    for (int i = 0; i < (int)U.size(); i++) {
        const Unit& u = U[i]; if (!alive(u)) continue; const Hero& h = HEROES[u.hero];
        const int a = u.invis > 0 ? (u.team == 0 ? 90 : 25) : 255;
        const RGB tc = u.team ? RGB{255, 80, 110} : RGB{80, 150, 255};
        if (u.storm > 0) circle(u.x + sx, u.y, 52, C(80, 255, 160, 50));
        if (u.shield > 0) circle(u.x + sx, u.y, 14, C(150, 220, 255, 90));
        if (u.armor > 0) circle(u.x + sx, u.y, 13, C(200, 150, 255, 90));
        circle(u.x + sx, u.y, 12, C(tc, a)); circle(u.x + sx, u.y, 10, C(h.col, a));
        if (u.invis <= 0 || u.team == 0) drawSimPortrait(*heroDoll(u.hero), u.x + sx, u.y, 10, t);
        if (u.stun > 0) text("*", u.x + sx, u.y - 24, 0.5f, C(255, 255, 120), 1);
        const float hp = std::max(0.f, u.hp / h.hp); rect(u.x - 12 + sx, u.y - 17, 24, 3, C(40, 30, 40, a)); rect(u.x - 12 + sx, u.y - 17, 24 * hp, 3, C(u.team ? RGB{255, 90, 110} : RGB{120, 240, 140}, a));
        if (i == 0) { text("YOU", u.x + sx, u.y + 11, 0.3f, C(255, 255, 255), 1); }
    }
    for (const Pop& p : P) text(p.s.c_str(), p.x + sx, p.y, 0.36f, C(p.c, (int)(std::min(1.f, p.life * 2) * 255)), 1);
    char b[48]; snprintf(b, sizeof b, "BLUE %d   -   %d RED", score[0], score[1]);
    rrect(130, 8, 140, 18, 8, C(40, 30, 60, 200)); text(b, 200, 10, 0.42f, C(WHITE), 1);
    snprintf(b, sizeof b, "%d:%02d", (int)std::max(0.f, timeLeft) / 60, (int)std::max(0.f, timeLeft) % 60); text(b, 200, 28, 0.36f, C(80, 40, 90), 1);
    if (!alive(U[0])) { char r[32]; snprintf(r, sizeof r, "Respawning in %.1f", U[0].respawn); textOutline(r, 200, 110, 0.6f, C(WHITE), C(BLACK), 1); }
    if (screen == 2) { rect(0, 0, TOP_W, TOP_H, C(20, 10, 30, 150)); textOutline(winner == 0 ? "VICTORY!" : winner == 2 ? "DRAW" : "DEFEAT", 200, 90, 1.3f, C(winner == 0 ? RGB{255, 220, 90} : RGB{255, 120, 150}), C(BLACK), 1); }
}

void rivalsDrawBottom(float t) { rect(0, 0, 320, 240, C(40, 28, 56));
    auto btn = [&](Btn b, const std::string& s, RGB c, float sc = 0.38f) { rrect(b.x, b.y, b.w, b.h, 8, C(c)); text(s.c_str(), b.x + b.w / 2, b.y + b.h / 2 - 6, sc, C(WHITE), 1); };
    if (screen == 0) {
        char pg[24]; snprintf(pg, sizeof pg, "PICK YOUR HERO  %d/%d", selPage + 1, (NHERO + 5) / 6); text(pg, 160, 8, 0.48f, C(255, 160, 210), 1);
        btn(Btn{6, 4, 52, 24}, "< L", {90, 70, 120}); btn(Btn{262, 4, 52, 24}, "R >", {90, 70, 120});
        for (int k = 0; k < 6 && selPage * 6 + k < NHERO; k++) { const int hk = selPage * 6 + k; const Hero& h = HEROES[hk]; Btn b = HB(k); rrect(b.x, b.y, b.w, b.h, 8, C(hk == pickHero ? RGB{255, 255, 255} : RGB{70, 55, 90})); rrect(b.x + 2, b.y + 2, b.w - 4, b.h - 4, 7, C(h.col.r / 3, h.col.g / 3, h.col.b / 3));
            circle(b.x + 18, b.y + 21, 12, C(h.col)); drawSimPortrait(*heroDoll(hk), b.x + 18, b.y + 21, 11, t); text(h.name, b.x + 34, b.y + 7, 0.38f, C(WHITE)); text(h.role, b.x + 34, b.y + 23, 0.34f, C(h.col)); }
        static const char* D[3] = {"Bots: Easy", "Bots: Normal", "Bots: Hard"};
        btn(Btn{8, 172, 100, 30}, D[diff], {90, 70, 120}); btn(Btn{170, 172, 142, 30}, "FIGHT! (A)", {255, 90, 160}, 0.45f); btn(Btn{8, 206, 100, 28}, "Back", {80, 70, 90});
        text("Move: Circle Pad / D-pad", 214, 210, 0.32f, C(220, 200, 230), 1); text("Auto-aims at the nearest enemy", 214, 222, 0.32f, C(220, 200, 230), 1);
        return;
    }
    const Unit& u = U[0]; const Hero& h = HEROES[u.hero];
    text(h.name, 160, 4, 0.5f, C(h.col), 1);
    rect(20, 24, 280, 12, C(60, 50, 70)); rect(20, 24, 280 * std::max(0.f, u.hp / h.hp), 12, C(120, 240, 140)); char b[48]; snprintf(b, sizeof b, "%d / %d HP", (int)std::max(0.f, u.hp), h.hp); text(b, 160, 25, 0.32f, C(BLACK), 1);
    rect(20, 40, 280, 8, C(60, 50, 70)); rect(20, 40, 280 * u.ult / 100, 8, C(255, 200, 60)); text(u.ult >= 100 ? "ULTIMATE READY - press R!" : "Ultimate charging...", 160, 50, 0.34f, C(255, 220, 120), 1);
    // scoreboard
    for (int i = 0; i < (int)U.size(); i++) { const Unit& x = U[i]; snprintf(b, sizeof b, "%s%s  %d/%d", i == 0 ? "> " : "", HEROES[x.hero].name, x.kos, x.deaths);
        text(b, x.team ? 238 : 82, 72 + (i % 3) * 16, 0.32f, C(x.team ? RGB{255, 130, 150} : RGB{130, 190, 255}), 1); }
    text("BLUE TEAM", 82, 62, 0.32f, C(130, 190, 255), 1); text("RED TEAM", 238, 62, 0.32f, C(255, 130, 150), 1);
    auto cdTxt = [&](const char* n, float c) { if (c <= 0) return std::string(n); char s[24]; snprintf(s, sizeof s, "%s %.0f", n, std::ceil(c)); return std::string(s); };
    btn(AB(0), "A: Attack", {200, 80, 140}); btn(AB(1), cdTxt(h.a1, u.c1), u.c1 > 0 ? RGB{70, 60, 80} : RGB{80, 140, 220}, 0.32f);
    btn(AB(2), cdTxt(h.a2, u.c2), u.c2 > 0 ? RGB{70, 60, 80} : RGB{80, 180, 140}, 0.32f); btn(AB(3), u.ult >= 100 ? h.ult : "Ult (R)", u.ult >= 100 ? RGB{255, 180, 40} : RGB{70, 60, 80}, 0.32f);
    text("X / Y abilities  -  START = quit match", 160, 180, 0.32f, C(220, 200, 230), 1);
    if (screen == 2) text("Tap to return to hero select", 160, 160, 0.4f, C(WHITE), 1);
}
