// Custom Content: plain-text CC files read from the SD card. Anyone (or any AI) can write them:
// furniture, hair, clothing, accessories and whole characters, drawn with simple shapes.
// Format: CC_GUIDE.txt (shipped in the SD-card zip under 3ds/StickFigureVer/cc/).
#include "cc.hpp"
#include "catalog.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "people.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "lifestory.hpp"
#include "sprites.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
using namespace col;
void toast(const std::string& m);

int g_ccBase[CC_SLOTS];
float g_ccTime = 0;

namespace {
enum { T_FURN, T_HAIR, T_CLOTH, T_ACC, T_CHAR, T_BG, T_ANIM, T_STORY, T_UI, T_SPRITE, T_COUNT };
const char* TYPE_N[T_COUNT] = {"Furniture", "Hair", "Clothing", "Accessory", "Character", "Background", "Animation", "Story", "UI", "Sprites"};
const char* FOLDER_N[T_COUNT] = {"furniture", "hair", "clothing", "accessories", "characters", "backgrounds", "animations", "stories", "ui", "sprites"};
enum { K_RECT, K_RRECT, K_CIRCLE, K_ELLIPSE, K_LINE, K_TRI, K_QUAD, K_HEART };
enum { CK_LIT, CK_MAIN, CK_LIGHT, CK_DARK, CK_HAIR, CK_SKIN };
struct Shape { uint8_t kind; float a[8]; RGB rgb; uint8_t alpha = 255, key = CK_LIT; };
struct Item {
    int type = T_FURN; std::string name, file, desc;
    int price = 100; float w = 60, h = 60; bool wall = false; int base = -1; bool enabled = true;
    RGB tint{255, 120, 180};
    std::vector<Shape> back, front;
    std::vector<std::pair<std::string, std::string>> kv;   // everything else (characters)
    std::string get(const char* k) const { for (auto& p : kv) if (p.first == k) return p.second; return ""; }
};
std::vector<Item> items;
std::vector<std::string> errors;

std::string trim(std::string s) { size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n"); return a == std::string::npos ? "" : s.substr(a, b - a + 1); }
std::string lower(std::string s) { for (auto& c : s) c = (char)tolower(c); return s; }

bool parseColor(const std::string& tok, Shape& sh) {
    std::string t = lower(tok);
    if (t == "main") { sh.key = CK_MAIN; return true; } if (t == "light") { sh.key = CK_LIGHT; return true; }
    if (t == "dark") { sh.key = CK_DARK; return true; } if (t == "hair") { sh.key = CK_HAIR; return true; }
    if (t == "skin") { sh.key = CK_SKIN; return true; }
    static const struct { const char* n; RGB c; } NAMED[] = {{"white", {255, 255, 255}}, {"black", {30, 26, 34}}, {"pink", {255, 140, 190}}, {"hotpink", {255, 60, 150}},
        {"red", {230, 50, 60}}, {"orange", {255, 150, 60}}, {"yellow", {255, 220, 80}}, {"green", {90, 190, 110}}, {"blue", {80, 140, 230}}, {"lightblue", {170, 210, 255}},
        {"purple", {170, 110, 220}}, {"lavender", {210, 190, 255}}, {"brown", {140, 95, 60}}, {"tan", {220, 190, 150}}, {"gray", {150, 150, 160}}, {"grey", {150, 150, 160}},
        {"gold", {240, 200, 90}}, {"silver", {200, 205, 215}}, {"cream", {255, 246, 230}}, {"mint", {160, 235, 200}}, {"teal", {60, 180, 180}}, {"navy", {40, 50, 110}}};
    for (auto& n : NAMED) if (t == n.n) { sh.rgb = n.c; return true; }
    if (t.size() >= 7 && t[0] == '#') {
        unsigned v = (unsigned)strtoul(t.substr(1, 6).c_str(), nullptr, 16);
        sh.rgb = RGB{(uint8_t)(v >> 16), (uint8_t)(v >> 8), (uint8_t)v};
        if (t.size() >= 9) sh.alpha = (uint8_t)strtoul(t.substr(7, 2).c_str(), nullptr, 16);
        return true;
    }
    return false;
}
bool parseShape(const std::string& line, Shape& sh) {
    char kw[16] = {0}; float v[8] = {0}; char col[24] = {0};
    std::string L = line; for (auto& c : L) if (c == ',') c = ' ';
    int n = sscanf(L.c_str(), "%15s", kw); if (n < 1) return false;
    std::string k = lower(kw);
    static const struct { const char* n; uint8_t kind; int args; } KW[] = {{"rect", K_RECT, 4}, {"roundrect", K_RRECT, 5}, {"circle", K_CIRCLE, 3}, {"ellipse", K_ELLIPSE, 4},
        {"line", K_LINE, 5}, {"tri", K_TRI, 6}, {"triangle", K_TRI, 6}, {"quad", K_QUAD, 8}, {"heart", K_HEART, 3}};
    for (auto& d : KW) if (k == d.n) {
        const char* p = L.c_str() + L.find(kw) + strlen(kw);
        int got = 0; char* end;
        for (int i = 0; i < d.args; i++) { float f = strtof(p, &end); if (end == p) break; v[i] = f; got++; p = end; }
        if (got < d.args) return false;
        if (sscanf(p, "%23s", col) != 1) return false;
        sh.kind = d.kind; memcpy(sh.a, v, sizeof v);
        return parseColor(col, sh);
    }
    return false;
}
int typeOf(const std::string& v) {
    std::string t = lower(v);
    if (t == "furniture" || t == "decor" || t == "object") return T_FURN;
    if (t == "hair" || t == "hairstyle") return T_HAIR;
    if (t == "clothing" || t == "clothes" || t == "outfit") return T_CLOTH;
    if (t == "accessory" || t == "accessories") return T_ACC;
    if (t == "character" || t == "sim" || t == "person" || t == "characters") return T_CHAR;
    if (t == "background" || t == "backgrounds" || t == "backdrop") return T_BG;
    if (t == "animation" || t == "animations") return T_ANIM;
    if (t == "story" || t == "stories") return T_STORY;
    if (t == "ui" || t == "theme") return T_UI;
    if (t == "accessories") return T_ACC;
    return -1;
}
int baseOf(const std::string& v) {
    static const struct { const char* n; int sh; } B[] = {{"bed", SH_BED}, {"sofa", SH_SOFA}, {"couch", SH_SOFA}, {"chair", SH_CHAIR}, {"beanbag", SH_BEANBAG},
        {"bookshelf", SH_BOOKSHELF}, {"tv", SH_TV}, {"lamp", SH_LAMP}, {"plant", SH_PLANT}, {"dresser", SH_DRESSER}, {"wardrobe", SH_DRESSER}, {"vanity", SH_VANITY},
        {"speaker", SH_SPEAKER}, {"stereo", SH_SPEAKER}, {"painting", SH_PAINTING}, {"poster", SH_PAINTING}, {"rug", SH_RUG}, {"desk", SH_DESK}, {"table", SH_DESK},
        {"fridge", SH_FRIDGE}, {"oven", SH_OVEN}, {"counter", SH_COUNTER}};
    std::string t = lower(v);
    for (auto& b : B) if (t == b.n) return b.sh;
    return -1;
}
void loadFile(const std::string& path, const std::string& fname, int folderType) {
    FILE* f = fopen(path.c_str(), "r"); if (!f) return;
    Item it; it.file = fname; bool front = true; int lineNo = 0; char buf[256]; bool hasType = folderType >= 0; if (hasType) it.type = folderType;
    while (fgets(buf, sizeof buf, f)) {
        lineNo++;
        std::string l = trim(buf);
        if (l[0] == '#' || (l.size() > 1 && l[0] == '/' && l[1] == '/')) continue;   // comment line
        if (l.empty()) continue;
        if (l == "[back]") { front = false; continue; } if (l == "[front]") { front = true; continue; }
        size_t eq = l.find('=');
        if (eq != std::string::npos) {
            std::string k = lower(trim(l.substr(0, eq))), v = trim(l.substr(eq + 1));
            if (k == "type") { int t = typeOf(v); if (t < 0) { errors.push_back(fname + ": unknown type '" + v + "'"); fclose(f); return; }
                if (folderType >= 0 && t != folderType && errors.size() < 30) errors.push_back(fname + ": says " + TYPE_N[t] + " but is in the " + TYPE_N[folderType] + " folder (using " + TYPE_N[t] + ")");
                it.type = t; hasType = true; }
            else if (k == "name") it.name = v.substr(0, 40);
            else if (k == "description" || k == "desc") it.desc = v.substr(0, 80);
            else if (k == "price") it.price = std::max(0, std::min(99999, atoi(v.c_str())));
            else if (k == "size") { float w = 0, h = 0; if (sscanf(v.c_str(), "%f %f", &w, &h) == 2) { it.w = std::max(4.f, std::min(160.f, w)); it.h = std::max(4.f, std::min(160.f, h)); } }
            else if (k == "wall") it.wall = lower(v) == "yes" || lower(v) == "true";
            else if (k == "works_like") it.base = baseOf(v);
            else if (k == "tint" || k == "color") { Shape t; if (parseColor(v, t)) it.tint = t.rgb; }
            else it.kv.push_back({k, v});
            continue;
        }
        Shape sh;
        if (parseShape(l, sh)) { auto& L = front ? it.front : it.back; if (L.size() < 120) L.push_back(sh); }
        else { char e[96]; snprintf(e, sizeof e, "%s line %d: can't read '%.30s'", fname.c_str(), lineNo, l.c_str()); if (errors.size() < 30) errors.push_back(e); }
    }
    fclose(f);
    if (!hasType) { errors.push_back(fname + ": missing 'type ='"); return; }
    // validate
    if ((it.type <= T_ACC || it.type == T_BG) && it.type != T_SPRITE && it.front.empty() && it.back.empty()) { errors.push_back(fname + ": has no shapes to draw"); return; }
    if (it.type == T_CHAR && it.get("gender").empty()) it.kv.push_back({"gender", "female"});
    if (it.name.empty()) it.name = fname.substr(0, fname.rfind('.'));
    for (auto& o : items) if (o.type == it.type && o.name == it.name) { errors.push_back(fname + ": duplicate name '" + it.name + "'"); return; }
    if (items.size() < 300) items.push_back(it);
}
void scanDir(const std::string& dir, int depth, int folderType) {
    DIR* d = opendir(dir.c_str()); if (!d) return;
    std::vector<std::string> names;
    while (dirent* e = readdir(d)) { if (e->d_name[0] == '.') continue; names.push_back(e->d_name); }
    closedir(d);
    std::sort(names.begin(), names.end());
    for (auto& n : names) {
        std::string p = dir + "/" + n;
        struct stat st; if (stat(p.c_str(), &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) { if (depth < 2) { int ft = folderType; for (int k = 0; k < T_COUNT; k++) if (lower(n) == FOLDER_N[k]) ft = k; scanDir(p, depth + 1, ft); } continue; }
        std::string ln = lower(n);
        if (ln.size() > 4 && ln.substr(ln.size() - 4) == ".txt" && ln != "cc_guide.txt" && ln != "ai_prompt.txt" && ln != "disabled.txt" && ln != "readme.txt") loadFile(p, n, folderType);
    }
}
std::vector<std::string> disabledList() {
    std::vector<std::string> v; FILE* f = fopen(CC_DIR "/disabled.txt", "r"); if (!f) return v;
    char b[128]; while (fgets(b, sizeof b, f)) { std::string s = trim(b); if (!s.empty()) v.push_back(s); }
    fclose(f); return v;
}
void saveDisabled() {
    mkdir(CC_ROOT, 0777); mkdir(CC_DIR, 0777);
    FILE* f = fopen(CC_DIR "/disabled.txt", "w"); if (!f) return;
    for (auto& it : items) if (!it.enabled) fprintf(f, "%s|%s\n", TYPE_N[it.type], it.name.c_str());
    fclose(f);
}
const Item* find(int type, const std::string& name) { if (name.empty()) return nullptr; for (auto& it : items) if (it.type == type && it.name == name && it.enabled) return &it; return nullptr; }
const Item* slotItem(int slot) {
    if (slot < 0 || slot >= CC_SLOTS || slot >= (int)G.ccSlots.size()) return nullptr;
    return find(T_FURN, G.ccSlots[slot]);
}
u32 shapeColor(const Shape& sh, RGB main, RGB hair, RGB skin) {
    RGB c = sh.rgb;
    switch (sh.key) { case CK_MAIN: c = main; break; case CK_LIGHT: c = shade(main, 1.25f); break; case CK_DARK: c = shade(main, 0.72f); break; case CK_HAIR: c = hair; break; case CK_SKIN: c = skin; break; default: break; }
    return C(c.r, c.g, c.b, sh.alpha);
}
// draw a shape list in a local box: X(x) Y(y) map local units to the screen
template <class FX, class FY> void drawShapes(const std::vector<Shape>& L, FX X, FY Y, float k, RGB main, RGB hair, RGB skin, bool flip) {
    for (const Shape& s : L) {
        u32 c = shapeColor(s, main, hair, skin); const float* a = s.a;
        switch (s.kind) {
        case K_RECT: { float x0 = X(a[0]), x1 = X(a[0] + a[2]); rect(std::min(x0, x1), Y(a[1]), std::fabs(x1 - x0), a[3] * k, c); break; }
        case K_RRECT: { float x0 = X(a[0]), x1 = X(a[0] + a[2]); rrect(std::min(x0, x1), Y(a[1]), std::fabs(x1 - x0), a[3] * k, a[4] * k, c); break; }
        case K_CIRCLE: circle(X(a[0]), Y(a[1]), a[2] * k, c); break;
        case K_ELLIPSE: ellipse(X(a[0]), Y(a[1]), a[2] * k, a[3] * k, c); break;
        case K_LINE: line(X(a[0]), Y(a[1]), X(a[2]), Y(a[3]), std::max(0.5f, a[4] * k), c); break;
        case K_TRI: tri(X(a[0]), Y(a[1]), X(a[2]), Y(a[3]), X(a[4]), Y(a[5]), c); break;
        case K_QUAD: quad(X(a[0]), Y(a[1]), X(a[2]), Y(a[3]), X(a[4]), Y(a[5]), X(a[6]), Y(a[7]), c); break;
        case K_HEART: heart(X(a[0]), Y(a[1]), a[2] * k, c); break;
        }
    }
    (void)flip;
}
float stageK(int st) { return st == ST_CHILD ? 0.74f : st == ST_PRETEEN ? 0.84f : st == ST_TEEN ? 0.94f : 1.f; }   // matches the doll renderer
int simIndex(const Sim& s) {
    if (!G.sims.empty() && &s >= &G.sims[0] && &s <= &G.sims.back()) return (int)(&s - &G.sims[0]);
    for (size_t i = 0; i < G.sims.size(); i++) if (G.sims[i].name == s.name) return (int)i;
    return -1;
}
std::string& wearOf(int sim, int slot) { size_t need = (G.sims.size()) * 3; if (G.ccWear.size() < need) G.ccWear.resize(need); return G.ccWear[sim * 3 + slot]; }
}

// ---------------------------------------------------------------- public
int ccCount() { return (int)items.size(); }
void ccLoadAll() {
    items.clear(); errors.clear();
    scanDir(CC_DIR, 0, -1);
    scanDir(APP_DIR "/cc", 0, -1);   // older location still works
    spriteScan();
    for (auto& n : spriteNames()) { Item it; it.type = T_SPRITE; it.name = n; it.file = n + ".png"; it.desc = spriteIsGuy(n) ? "Guy sprite" : "Girl sprite"; items.push_back(it); }
    std::vector<std::string> off = disabledList();
    for (auto& it : items) { std::string key = std::string(TYPE_N[it.type]) + "|" + it.name; if (std::find(off.begin(), off.end(), key) != off.end()) it.enabled = false; }
    ccAssignSlots();
}
bool ccSlotUsed(int slot) { return slotItem(slot) != nullptr; }
void ccAssignSlots() {
    if (G.ccSlots.size() != (size_t)CC_SLOTS) G.ccSlots.resize(CC_SLOTS);
    for (auto& it : items) if (it.type == T_FURN && it.enabled) {
        bool has = false; for (auto& s : G.ccSlots) if (s == it.name) has = true;
        if (!has) for (auto& s : G.ccSlots) if (s.empty() || !find(T_FURN, s)) { bool placed = false;
            // don't steal a slot whose item is still placed somewhere; empty-named slots are always free
            if (s.empty()) { s = it.name; placed = true; } if (placed) break; }
    }
    for (int k = 0; k < CC_SLOTS; k++) {
        const Item* it = slotItem(k); ShapeInfo& si = SHAPES[SH_CC0 + k];
        if (it) { si.name = it->name.c_str(); si.wall = it->wall; si.w = it->w; si.h = it->h; g_ccBase[k] = it->base >= 0 ? it->base : SH_CC0 + k; }
        else { si.name = G.ccSlots[k].empty() ? "" : "(Missing CC)"; si.wall = false; si.w = 30; si.h = 30; g_ccBase[k] = SH_CC0 + k; }
    }
}
void ccDrawFurniture(int slot, float x, float footY, float s, int ci, float t) {
    const Item* it = slotItem(slot);
    const ShapeInfo& si = SHAPES[SH_CC0 + slot];
    Pen p(x, footY, si.w, si.h, s);
    if (!it) { p.R(0, 0, si.w, si.h, C(200, 200, 210)); text("?", p.X(si.w / 2), p.Y(si.h / 2 - 6), 0.5f * s, C(80, 80, 90), 1); return; }
    RGB main = PALETTE[ci % NUM_COLORS];
    drawShapes(it->back, [&](float lx) { return p.X(lx); }, [&](float ly) { return p.Y(ly); }, s, main, main, main, false);
    drawShapes(it->front, [&](float lx) { return p.X(lx); }, [&](float ly) { return p.Y(ly); }, s, main, main, main, false);
    (void)t;
}
// wearables use the doll's own canvas: x -48..48 (0 = body centre), y 0..186 (186 = soles of the feet).
// landmarks: head top 15, eyes 30, chin 41, shoulders 51, chest 60, waist 72, hips 88, knees 132, ankles 174.
// ---- CC wear baked straight into the doll's own picture, so it moves, flips, bobs and scales with the body
namespace {
bool inShape(const Shape& s, float lx, float ly) {
    const float* a = s.a;
    auto tri = [](float px, float py, float x0, float y0, float x1, float y1, float x2, float y2) {
        float d1 = (px - x1) * (y0 - y1) - (x0 - x1) * (py - y1), d2 = (px - x2) * (y1 - y2) - (x1 - x2) * (py - y2), d3 = (px - x0) * (y2 - y0) - (x2 - x0) * (py - y0);
        bool neg = d1 < 0 || d2 < 0 || d3 < 0, pos = d1 > 0 || d2 > 0 || d3 > 0; return !(neg && pos); };
    switch (s.kind) {
    case K_RECT: return lx >= std::min(a[0], a[0] + a[2]) && lx <= std::max(a[0], a[0] + a[2]) && ly >= a[1] && ly <= a[1] + a[3];
    case K_RRECT: {
        float x0 = std::min(a[0], a[0] + a[2]), x1 = std::max(a[0], a[0] + a[2]), y0 = a[1], y1 = a[1] + a[3], r = std::min(a[4], std::min((x1 - x0), (y1 - y0)) / 2);
        if (lx < x0 || lx > x1 || ly < y0 || ly > y1) return false;
        float cx = std::clamp(lx, x0 + r, x1 - r), cy = std::clamp(ly, y0 + r, y1 - r);
        return (lx - cx) * (lx - cx) + (ly - cy) * (ly - cy) <= r * r; }
    case K_CIRCLE: return (lx - a[0]) * (lx - a[0]) + (ly - a[1]) * (ly - a[1]) <= a[2] * a[2];
    case K_ELLIPSE: { if (a[2] <= 0 || a[3] <= 0) return false; float dx = (lx - a[0]) / a[2], dy = (ly - a[1]) / a[3]; return dx * dx + dy * dy <= 1; }
    case K_LINE: {
        float vx = a[2] - a[0], vy = a[3] - a[1], L2 = vx * vx + vy * vy, t = L2 > 0 ? std::clamp(((lx - a[0]) * vx + (ly - a[1]) * vy) / L2, 0.f, 1.f) : 0;
        float dx = lx - (a[0] + vx * t), dy = ly - (a[1] + vy * t), w = std::max(0.5f, a[4] * 0.5f); return dx * dx + dy * dy <= w * w; }
    case K_TRI: return tri(lx, ly, a[0], a[1], a[2], a[3], a[4], a[5]);
    case K_QUAD: return tri(lx, ly, a[0], a[1], a[2], a[3], a[4], a[5]) || tri(lx, ly, a[0], a[1], a[4], a[5], a[6], a[7]);
    case K_HEART: { float r = a[2], h = r * 0.5f; float d1 = (lx - a[0] + h) * (lx - a[0] + h) + (ly - a[1] + r * 0.25f) * (ly - a[1] + r * 0.25f), d2 = (lx - a[0] - h) * (lx - a[0] - h) + (ly - a[1] + r * 0.25f) * (ly - a[1] + r * 0.25f);
        return d1 <= h * h || d2 <= h * h || tri(lx, ly, a[0] - r, a[1] - r * 0.12f, a[0] + r, a[1] - r * 0.12f, a[0], a[1] + r); }
    }
    return false;
}
// paint one shape list into an RGBA buffer; s = canvas pixels per doll unit; under = draw behind what's there
void bake(const std::vector<Shape>& L, uint8_t* px, int W, int H, float s, float sk, RGB main, RGB hair, RGB skin, bool under) {
    for (const Shape& sh : L) {
        u32 c = shapeColor(sh, main, hair, skin);
        const int cr = c & 255, cg = (c >> 8) & 255, cb = (c >> 16) & 255, ca = (c >> 24) & 255;
        if (!ca) continue;
        for (int y = 0; y < H; y++) {
            const float ly = 186.f - (186.f - (y + 0.5f) / s) / sk;   // stage scale keeps the feet planted
            for (int x = 0; x < W; x++) {
                const float lx = ((x + 0.5f) / s - 48.f) / sk;
                if (!inShape(sh, lx, ly)) continue;
                uint8_t* p = px + (y * W + x) * 4;
                if (under) { if (p[3] >= 250) continue; if (!p[3]) { p[0] = (uint8_t)cr; p[1] = (uint8_t)cg; p[2] = (uint8_t)cb; p[3] = (uint8_t)ca; } continue; }
                const int ia = 255 - ca;
                p[0] = (uint8_t)((cr * ca + p[0] * ia) / 255); p[1] = (uint8_t)((cg * ca + p[1] * ia) / 255); p[2] = (uint8_t)((cb * ca + p[2] * ia) / 255);
                p[3] = (uint8_t)std::max<int>(p[3], ca);
            }
        }
    }
}
}
uint32_t ccWearKey(const Sim& s) {
    int si = simIndex(s); if (si < 0 || (size_t)(si * 3 + 2) >= G.ccWear.size()) return 0;
    uint32_t h = 2166136261u; for (int k = 0; k < 3; k++) for (char ch : G.ccWear[si * 3 + k]) h = (h ^ (uint8_t)ch) * 16777619u;
    return h == 2166136261u ? 0 : h;
}
void ccBakeWear(const Sim& s, uint8_t* rgba, int W, int H, int frames, float scale) {
    if (items.empty()) return;
    int si = simIndex(s); if (si < 0) return;
    // the doll canvas is drawn at full size for every stage; the renderer scales it at draw time, so bake at stage 1.0
    RGB hair = hairRGB(s.look.hairCol), skin = skinRGB(s.look.skin);
    static const int ORDER[3] = {T_CLOTH, T_HAIR, T_ACC};
    for (int f = 0; f < frames; f++) {
        uint8_t* px = rgba + (size_t)f * W * H * 4;
        for (int pass = 0; pass < 2; pass++)
            for (int o = 0; o < 3; o++) {
                int slot = ORDER[o] == T_HAIR ? 0 : ORDER[o] == T_CLOTH ? 1 : 2;
                if ((size_t)(si * 3 + slot) >= G.ccWear.size()) continue;
                const Item* it = find(ORDER[o], G.ccWear[si * 3 + slot]); if (!it) continue;
                bake(pass ? it->front : it->back, px, W, H, scale, 1.f, it->tint, hair, skin, pass == 0);
            }
    }
}
void ccDrawWear(const Sim& s, float x, float footY, float scale, bool flip, bool frontLayer, int pose) {
    return;   // CC is baked into the doll texture now (ccBakeWear) - kept for API compatibility
    if (items.empty() || s.stage < ST_CHILD) return;
    if (pose == 3 || pose == 6 || pose == 7 || pose == 8) return;   // sitting / lying: the body is folded, skip CC
    int si = simIndex(s); if (si < 0) return;
    const float k = 0.5f * scale * stageK(s.stage), sc = 2.f * k;
    // follow the doll exactly: same pixel snapping, walk bob and dance sway/turns (oc.cpp ocDrawSim)
    float bob = 0, sway = 0;
    if (pose == 5) bob = -std::floor(std::fabs(std::sin(g_walkPhase * 6.2832f)) * 1.6f + 0.5f);
    if (pose == 2) { float tt = g_ccTime; bob = -std::floor(std::fabs(std::sin(tt * 7.f)) * 3.f); sway = std::floor(std::sin(tt * 3.5f) * 3.f); flip = std::sin(tt * 1.75f) > 0; }
    const float bx = sc <= 1.2f ? std::floor(x - 24 * sc + sway) + 24 * sc : x;   // world dolls snap to whole pixels
    const float by = sc <= 1.2f ? std::floor(footY - 93 * sc + bob) + 93 * sc : footY;
    auto X = [&](float lx) { return bx + (flip ? -lx : lx) * k; };
    auto Y = [&](float ly) { return by + (ly - 186.f) * k; };
    RGB hair = hairRGB(s.look.hairCol), skin = skinRGB(s.look.skin);
    static const int ORDER[3] = {T_CLOTH, T_HAIR, T_ACC};
    for (int o = 0; o < 3; o++) {
        int slot = ORDER[o] == T_HAIR ? 0 : ORDER[o] == T_CLOTH ? 1 : 2;
        if ((size_t)(si * 3 + slot) >= G.ccWear.size()) continue;
        const Item* it = find(ORDER[o], G.ccWear[si * 3 + slot]); if (!it) continue;
        drawShapes(frontLayer ? it->front : it->back, X, Y, k, it->tint, hair, skin, flip);
    }
}

int ccBackgroundCount() { int n = 0; for (auto& it : items) if (it.type == T_BG && it.enabled) n++; return n; }
static const Item* bgItem(int k) { int n = 0; for (auto& it : items) if (it.type == T_BG && it.enabled) { if (n == k) return &it; n++; } return nullptr; }
const char* ccBackgroundName(int k) { const Item* it = bgItem(k); return it ? it->name.c_str() : "(Missing Background)"; }
// backgrounds fill the whole top screen: x 0..400, y 0..240
void ccDrawBackground(int k, float t) {
    const Item* it = bgItem(k);
    if (!it) { rect(0, 0, TOP_W, TOP_H, C(200, 200, 210)); text("Missing custom background", 200, 110, 0.5f, C(80, 80, 90), 1); return; }
    rect(0, 0, TOP_W, TOP_H, C(it->tint));
    drawShapes(it->back, [](float lx) { return lx; }, [](float ly) { return ly; }, 1.f, it->tint, it->tint, it->tint, false);
    drawShapes(it->front, [](float lx) { return lx; }, [](float ly) { return ly; }, 1.f, it->tint, it->tint, it->tint, false);
    (void)t;
}

// ---------------------------------------------------------------- CC Manager screen
namespace {
int tab = 0, sel = 0, scroll = 0, target = 0, armDel = -1;
std::vector<int> listOf(int type) { std::vector<int> v; for (size_t i = 0; i < items.size(); i++) if (items[i].type == type) v.push_back((int)i); return v; }
std::vector<int> household() { std::vector<int> v{G.mc}; for (size_t i = 0; i < G.sims.size(); i++) if ((int)i != G.mc && G.sims[i].household) v.push_back((int)i); return v; }
int tgt() { auto h = household(); return h[target % h.size()]; }
Btn tabB(int k) { return k < 5 ? Btn{2 + k * 63.6f, 1, 62, 18} : Btn{2 + (k - 5) * 63.6f, 20, 62, 18}; }
Btn rowB(int k) { return Btn{4, 40 + k * 24.f, 256, 22}; }
const Btn B_UP{264, 40, 52, 34}, B_DN{264, 78, 52, 34}, B_BACK{4, 208, 60, 28}, B_TOGGLE{68, 208, 72, 28}, B_USE{144, 208, 100, 28}, B_MORE{248, 208, 68, 28},
          B_RELOAD{264, 112, 52, 26}, B_EXPORT{264, 142, 52, 26}, B_WHO{4, 186, 312, 18};
int slotFor(int type) { return type == T_HAIR ? 0 : type == T_CLOTH ? 1 : 2; }

int stageFrom(const std::string& v) { std::string t = lower(v);
    if (t == "infant" || t == "baby") return ST_INFANT; if (t == "toddler") return ST_TODDLER; if (t == "child" || t == "kid") return ST_CHILD; if (t == "preteen" || t == "tween") return ST_PRETEEN;
    if (t == "teen") return ST_TEEN; if (t == "adult") return ST_ADULT; if (t == "elder") return ST_ELDER; return ST_YA; }
int addCharacter(const Item& it, bool moveInToo) {
    bool guy = lower(it.get("gender")) == "male" || lower(it.get("gender")) == "boy" || lower(it.get("gender")) == "guy";
    Sim s = randomSim((Stage)stageFrom(it.get("age")), guy ? 1 : 0);
    s.name = it.name; s.look.guy = guy;
    std::string v;
    if (!(v = it.get("skin")).empty()) s.look.skin = (uint8_t)(atoi(v.c_str()) % NUM_SKINS);
    if (!(v = it.get("hair_color")).empty() || !(v = it.get("haircolor")).empty()) s.look.hairCol = (uint8_t)(atoi(v.c_str()) % NUM_HAIRCOLS);
    if (!(v = it.get("hairstyle")).empty() || !(v = it.get("hair")).empty()) s.look.hairStyle = (uint8_t)(atoi(v.c_str()) % NUM_HAIRSTYLES);
    if (!(v = it.get("outfit")).empty()) s.outfit = (uint8_t)(atoi(v.c_str()) % NUM_OUTFITS);
    if (!(v = it.get("bio")).empty()) s.bio = v.substr(0, 120);
    if (!(v = it.get("interest")).empty()) { std::string t = lower(v); s.interest = t == "guys" || t == "men" ? 0 : t == "girls" || t == "women" ? 1 : 2; }
    if (!(v = it.get("traits")).empty()) { int n = 0; std::string t = lower(v); for (int k = 1; k < TR_COUNT && n < 3; k++) if (t.find(lower(TRAIT_NAMES[k])) != std::string::npos) s.traits[n++] = (uint8_t)k; }
    for (auto& o : G.sims) if (o.name == s.name) { toast(s.name + " is already in town!"); return -1; }
    G.sims.push_back(s); int idx = (int)G.sims.size() - 1;
    const char* WK[3] = {"cc_hair", "cc_clothing", "cc_accessory"};
    for (int k = 0; k < 3; k++) if (!(v = it.get(WK[k])).empty()) wearOf(idx, k) = v;
    // relationships = Maya Cole: best friend; Jade Monroe: crush 40; Theo: enemy
    if (!(v = it.get("relationships")).empty()) {
        size_t a = 0;
        while (a < v.size()) {
            size_t b = v.find(';', a); std::string part = trim(v.substr(a, b == std::string::npos ? std::string::npos : b - a)); a = b == std::string::npos ? v.size() : b + 1;
            size_t c = part.find(':'); if (c == std::string::npos) continue;
            std::string who = trim(part.substr(0, c)), what = lower(trim(part.substr(c + 1)));
            int other = -1; for (size_t q = 0; q < G.sims.size(); q++) if ((int)q != idx && (lower(G.sims[q].name) == lower(who) || lower(G.sims[q].name).rfind(lower(who) + " ", 0) == 0)) { other = (int)q; break; }
            if (lower(who) == "me" || lower(who) == "player") other = G.mc;
            if (other < 0) { if (errors.size() < 30) errors.push_back(it.name + ": no Sim named '" + who + "' in this town yet"); continue; }
            int num = 0; for (char ch : what) if (isdigit((unsigned char)ch)) num = num * 10 + (ch - '0');
            Rel& r = rel(idx, other); r.met = true;
            int fr = 30, ro = 0;
            if (what.find("best") != std::string::npos) fr = 85;
            else if (what.find("enemy") != std::string::npos || what.find("rival") != std::string::npos) fr = -60;
            else if (what.find("friend") != std::string::npos) fr = 55;
            if (what.find("crush") != std::string::npos) { ro = 45; fr = std::max(fr, 30); }
            if (what.find("dating") != std::string::npos || what.find("girlfriend") != std::string::npos || what.find("boyfriend") != std::string::npos) { r.status = RS_DATING; fr = 60; ro = 70; }
            if (what.find("engaged") != std::string::npos) { r.status = RS_ENGAGED; fr = 70; ro = 80; }
            if (what.find("married") != std::string::npos || what.find("spouse") != std::string::npos) { r.status = RS_MARRIED; fr = 75; ro = 85; }
            if (what.find("family") != std::string::npos || what.find("sibling") != std::string::npos || what.find("cousin") != std::string::npos) { r.status = RS_FAMILY; fr = 60; }
            if (num > 0) fr = (fr < 0 ? -1 : 1) * std::min(100, num);
            changeRel(idx, other, fr, ro);
        }
    }
    if (moveInToo) { if (householdCount() >= 8) toast("House is full (8 max) - added to town instead."); else moveIn(idx); }
    return idx;
}
void exportHousehold() {
    mkdir(CC_ROOT, 0777); mkdir(CC_DIR, 0777); mkdir(CC_DIR "/Characters", 0777); mkdir(CC_DIR "/Characters/Export", 0777);
    int n = 0;
    for (int i : household()) {
        const Sim& s = G.sims[i];
        std::string fn = s.name; for (auto& c : fn) if (!isalnum((unsigned char)c)) c = '_';
        std::string path = std::string(CC_DIR "/Characters/Export/") + fn + ".txt";
        FILE* f = fopen(path.c_str(), "w"); if (!f) continue;
        static const char* AGE[ST_COUNT] = {"infant", "toddler", "child", "preteen", "teen", "youngadult", "adult", "elder"};
        fprintf(f, "# Exported from Sugar Bunny Life 2. Share it! It loads from any Mods/Characters folder.\ntype = character\nname = %s\ngender = %s\nage = %s\n", s.name.c_str(), s.look.guy ? "male" : "female", AGE[s.stage % ST_COUNT]);
        fprintf(f, "skin = %d\nhair_color = %d\nhairstyle = %d\noutfit = %d\n", s.look.skin, s.look.hairCol, s.look.hairStyle, s.outfit);
        std::string tr; for (int k = 0; k < 3; k++) if (s.traits[k] != TR_NONE) { if (!tr.empty()) tr += ", "; tr += TRAIT_NAMES[s.traits[k]]; }
        if (!tr.empty()) fprintf(f, "traits = %s\n", tr.c_str());
        if (!s.bio.empty()) fprintf(f, "bio = %s\n", s.bio.c_str());
        const char* WK[3] = {"cc_hair", "cc_clothing", "cc_accessory"};
        for (int k = 0; k < 3; k++) { const std::string& w = wearOf(i, k); if (!w.empty()) fprintf(f, "%s = %s\n", WK[k], w.c_str()); }
        fclose(f); n++;
    }
    toast("Exported " + std::to_string(n) + " Sims to LifeSim/Mods/Characters/Export on your SD card!");
}
}

void ccOpen() { tab = 0; sel = 0; scroll = 0; target = 0; armDel = -1; }

bool ccUpdate(const Input& in) {
    if (in.down & KEY_B) return false;
    if (in.down & KEY_L) target = (target + (int)household().size() - 1) % (int)household().size();
    if (in.down & KEY_R) target = (target + 1) % (int)household().size();
    if (!in.touch) return true;
    const touchPosition& tp = in.tp;
    for (int k = 0; k < T_COUNT; k++) if (tabB(k).in(tp)) { tab = k; sel = 0; scroll = 0; sfxTap(); return true; }
    if (B_BACK.in(tp)) { sfxTap(); return false; }
    if (B_UP.in(tp)) { if (scroll > 0) scroll--; sfxTap(); return true; }
    if (B_DN.in(tp)) { scroll++; sfxTap(); return true; }
    if (B_RELOAD.in(tp)) { ccLoadAll(); toast("Reloaded " + std::to_string(items.size()) + " CC files from the SD card" + (errors.empty() ? "!" : " (" + std::to_string(errors.size()) + " problems - see top screen)")); sfxYay(); return true; }
    if (B_EXPORT.in(tp)) { exportHousehold(); sfxYay(); return true; }
    if (B_WHO.in(tp)) { target = (target + 1) % (int)household().size(); sfxTap(); return true; }
    std::vector<int> L = listOf(tab);
    for (int k = 0; k < 6; k++) { int i = scroll * 6 + k; if (i < (int)L.size() && rowB(k).in(tp)) { sel = i; sfxTap(); return true; } }
    if (sel >= (int)L.size()) return true;
    Item& it = items[L[sel]];
    if (B_TOGGLE.in(tp)) { it.enabled = !it.enabled; saveDisabled(); ccAssignSlots(); toast(it.name + (it.enabled ? " turned ON" : " turned OFF")); sfxTap(); return true; }
    if (B_USE.in(tp)) {
        if (!it.enabled) { toast("Turn it on first!"); sfxNo(); return true; }
        if (tab == T_SPRITE) { int s = tgt(); if (G.spriteOf.size() < G.sims.size()) G.spriteOf.resize(G.sims.size());
            if (G.spriteOf[s] == it.name) { G.spriteOf[s] = "-"; toast(G.sims[s].name + " is back to the drawn doll."); } else { G.spriteOf[s] = it.name; toast(G.sims[s].name + " now uses the " + it.name + " sprite!"); } sfxYay(); return true; }
        if (tab >= T_BG) { toast(tab == T_BG ? "Use it as a location in Movie Maker (after the regular places)." : "Installed! This kind of mod isn't used by the game yet."); return true; }
        if (tab == T_FURN) { bool slotted = false; for (auto& s : G.ccSlots) if (s == it.name) slotted = true; toast(slotted ? "Find \"" + it.name + "\" on the last pages of Build mode!" : "All 32 CC furniture slots are full - turn some off."); }
        else if (tab == T_CHAR) { int idx = addCharacter(it, false); if (idx >= 0) { toast(it.name + " moved to town! Say hi around the neighborhood."); sfxYay(); } }
        else { std::string& w = wearOf(tgt(), slotFor(tab)); if (w == it.name) { w.clear(); toast("Took it off."); } else { w = it.name; toast(G.sims[tgt()].name + " is wearing " + it.name + "!"); } sfxYay(); }
        return true;
    }
    if (B_MORE.in(tp)) {
        if (tab == T_CHAR) { int idx = addCharacter(it, true); if (idx >= 0) { toast(it.name + " moved into your house!"); sfxYay(); } }
        else if (tab == T_SPRITE) { G.spriteAuto = !G.spriteAuto; toast(G.spriteAuto ? "Sprite Town ON: everyone without a pick gets a matching sprite." : "Sprite Town OFF: only Sims you picked use sprites."); sfxTap(); }
        else if (tab >= T_HAIR && tab <= T_ACC) { for (int i : household()) wearOf(i, slotFor(tab)).clear(); toast("Cleared that slot for the whole household."); sfxTap(); }
        return true;
    }
    return true;
}

void ccDrawTop(float t) {
    grad(0, 0, TOP_W, TOP_H, C(255, 240, 248), C(230, 236, 255));
    textOutline("CC MANAGER", 200, 2, 0.6f, C(255, 255, 255), C(255, 70, 150), 1);
    std::vector<int> L = listOf(tab);
    if (!errors.empty()) {
        text("Some CC files had problems:", 200, 22, 0.36f, C(200, 40, 70), 1);
        for (size_t k = 0; k < errors.size() && k < 3; k++) text(errors[k].c_str(), 200, 34 + k * 11.f, 0.28f, C(160, 60, 80), 1);
    }
    if (L.empty() || sel >= (int)L.size()) {
        text("No custom content here yet.", 200, 100, 0.48f, C(UI_INK), 1);
        text("Put CC .txt files in LifeSim/Mods/<folder>/ on your SD card,", 200, 128, 0.36f, C(120, 100, 130), 1);
        text("then tap Reload. Read LifeSim/CC_GUIDE.txt to make your own!", 200, 142, 0.36f, C(120, 100, 130), 1);
        if (tab == T_SPRITE) text("Put character PNGs in LifeSim/Sprites/ (m_ at the start = guy).", 200, 160, 0.3f, C(150, 120, 150), 1);
        else if (tab >= T_ANIM) text("(Animations, Stories & UI mods are listed and checked, but the game doesn't use them yet.)", 200, 160, 0.28f, C(150, 120, 150), 1);
        return;
    }
    const Item& it = items[L[sel]];
    textOutline(it.name.c_str(), 200, 70, 0.55f, C(255, 220, 120), C(UI_INK), 1);
    char b[96]; snprintf(b, sizeof b, "%s  -  %s  -  %s", TYPE_N[it.type], it.file.c_str(), it.enabled ? "ON" : "OFF"); text(b, 200, 90, 0.32f, C(120, 100, 130), 1);
    if (!it.desc.empty()) text(it.desc.c_str(), 200, 102, 0.32f, C(120, 100, 130), 1);
    rrect(100, 116, 200, 120, 8, C(255, 255, 255, 200));
    if (it.type == T_FURN) {
        float sc = std::min(1.6f, std::min(180.f / it.w, 110.f / it.h));
        Pen p(200, 230, it.w, it.h, sc);
        drawShapes(it.back, [&](float lx) { return p.X(lx); }, [&](float ly) { return p.Y(ly); }, sc, PALETTE[0], PALETTE[0], PALETTE[0], false);
        drawShapes(it.front, [&](float lx) { return p.X(lx); }, [&](float ly) { return p.Y(ly); }, sc, PALETTE[0], PALETTE[0], PALETTE[0], false);
        snprintf(b, sizeof b, "$%d  -  %s", it.price, it.wall ? "goes on the wall" : "floor item"); text(b, 200, 120, 0.3f, C(UI_INK), 1);
    } else if (it.type == T_SPRITE) {
        if (G.spriteOf.size() < G.sims.size()) G.spriteOf.resize(G.sims.size());
        std::string keep = G.spriteOf[tgt()]; G.spriteOf[tgt()] = it.name;
        drawSim(G.sims[tgt()], 200, 232, 1.2f, t, 0, PO_IDLE, false);
        G.spriteOf[tgt()] = keep;
    } else if (it.type == T_BG) {
        float k = 0.5f; for (const auto* L2 : {&it.back, &it.front}) (void)L2;
        rect(100, 116, 200, 120, C(it.tint));
        drawShapes(it.back, [&](float lx) { return 100 + lx * k; }, [&](float ly) { return 116 + ly * k; }, k, it.tint, it.tint, it.tint, false);
        drawShapes(it.front, [&](float lx) { return 100 + lx * k; }, [&](float ly) { return 116 + ly * k; }, k, it.tint, it.tint, it.tint, false);
    } else if (it.type >= T_ANIM) {
        int y = 122; for (auto& p : it.kv) { if (y > 222) break; std::string l = p.first + ": " + p.second; if (l.size() > 48) l = l.substr(0, 46) + ".."; text(l.c_str(), 110, (float)y, 0.3f, C(UI_INK)); y += 11; }
        text("Installed - not used by the game yet", 200, 222, 0.3f, C(160, 120, 150), 1);
    } else if (it.type == T_CHAR) {
        int y = 122;
        for (auto& p : it.kv) { if (y > 222) break; std::string l = p.first + ": " + p.second; if (l.size() > 48) l = l.substr(0, 46) + ".."; text(l.c_str(), 110, (float)y, 0.3f, C(UI_INK)); y += 11; }
    } else {
        const Sim& s = G.sims[tgt()];
        std::string& w = wearOf(tgt(), slotFor(tab)); std::string keep = w; w = it.name;   // preview on the chosen Sim
        drawSim(s, 200, 232, 1.15f, t, 0, PO_IDLE, false);
        w = keep;
    }
}
void ccDrawBottom(float t) {
    rect(0, 0, BOT_W, BOT_H, C(255, 246, 250));
    for (int k = 0; k < T_COUNT; k++) button(tabB(k), TYPE_N[k], tab == k, 0, 0.28f);
    std::vector<int> L = listOf(tab);
    for (int k = 0; k < 6; k++) { int i = scroll * 6 + k; if (i >= (int)L.size()) break; const Item& it = items[L[i]];
        std::string l = it.name + (it.enabled ? "" : "  (off)");
        if (tab >= T_HAIR && tab <= T_ACC && wearOf(tgt(), slotFor(tab)) == it.name) l += "  [wearing]";
        if (tab == T_SPRITE && tgt() < (int)G.spriteOf.size() && G.spriteOf[tgt()] == it.name) l += "  [using]";
        button(rowB(k), l.c_str(), sel == i, it.enabled ? 0 : 1, 0.34f); }
    if (L.empty()) text("(empty)", 130, 80, 0.4f, C(150, 130, 150), 1);
    button(B_UP, "^", false, 0, 0.45f); button(B_DN, "v", false, 0, 0.45f);
    button(B_RELOAD, "Reload", false, 2, 0.3f); button(B_EXPORT, "Export", false, 3, 0.3f);
    if ((tab >= T_HAIR && tab <= T_ACC) || tab == T_SPRITE) { char b[64]; snprintf(b, sizeof b, tab == T_SPRITE ? "Sprite for: %s  (tap or L/R to switch)" : "Dressing: %s  (tap or L/R to switch)", G.sims[tgt()].name.c_str()); button(B_WHO, b, false, 0, 0.3f); }
    else text(tab == T_CHAR ? "Export saves your household as character files" : tab == T_FURN ? "Custom furniture shows up at the end of Build mode" : tab == T_BG ? "Backgrounds are Movie Maker locations" : tab == T_SPRITE ? "" : "Listed & checked - not used by the game yet", 160, 190, 0.28f, C(140, 120, 150), 1);
    button(B_BACK, "Back", false, 1, 0.36f);
    bool has = sel < (int)L.size();
    button(B_TOGGLE, has && !items[L[sel]].enabled ? "Turn On" : "Turn Off", false, 0, 0.32f);
    button(B_USE, tab == T_FURN ? "Where is it?" : tab == T_CHAR ? "Add to Town" : tab == T_SPRITE ? "Use / Undo" : tab >= T_BG ? "How to use" : "Wear / Remove", false, 2, 0.32f);
    if (tab == T_CHAR) button(B_MORE, "Move In", false, 3, 0.32f);
    else if (tab == T_SPRITE) button(B_MORE, G.spriteAuto ? "Town: ON" : "Town: OFF", G.spriteAuto, 3, 0.3f);
    else if (tab >= T_HAIR && tab <= T_ACC) button(B_MORE, "Clear All", false, 3, 0.3f);
    (void)t;
}
