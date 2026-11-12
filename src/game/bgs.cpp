// Painted backgrounds: decode a 400x240 PNG, upload it as a 512x256 texture, draw it behind a place.
#include "bgs.hpp"
#include "game.hpp"
#include "sprites.hpp"
#include "draw.hpp"
#include <citro2d.h>
#include <dirent.h>
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
// RGBA -> 3DS tiled (Morton 8x8) texture, top row first
static void uploadBG(C3D_Tex& tx, const uint8_t* buf, int w, int h) {
    uint32_t* px = (uint32_t*)tx.data; const int TW = 512;
    memset(px, 0, tx.size);
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
        const uint8_t* s = buf + (y * w + x) * 4;
        uint32_t off = ((y >> 3) * (TW >> 3) + (x >> 3)) * 64;
        uint32_t m = (x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3);
        px[off + m] = (uint32_t)s[0] << 24 | (uint32_t)s[1] << 16 | (uint32_t)s[2] << 8 | s[3];
    }
    GSPGPU_FlushDataCache(tx.data, tx.size);
}

bool g_spotMarkers = false;
namespace {
std::map<std::string, std::string> rooms;                       // sanitized room name -> file
std::vector<std::pair<std::string, std::string>> extras;       // display name -> file
struct Slot { C3D_Tex tex; bool ready = false; std::string path; uint32_t used = 0; bool ok = false; };
Slot slots[2]; uint32_t tick = 0;
std::string key(const char* name) { std::string k; for (const char* p = name; *p; p++) { char c = *p; k += (isalnum((unsigned char)c) ? (char)tolower(c) : '_'); } while (k.find("__") != std::string::npos) k.erase(k.find("__"), 1); return k; }
void scanDir(const std::string& dir, bool extra) {
    DIR* d = opendir(dir.c_str()); if (!d) return;
    while (dirent* e = readdir(d)) {
        std::string n = e->d_name; if (n.size() < 5 || n[0] == '.') continue;
        std::string ext = n.substr(n.size() - 4); for (auto& c : ext) c = (char)tolower(c);
        if (ext != ".png") continue;
        std::string base = n.substr(0, n.size() - 4), path = dir + "/" + n;
        if (extra) { std::string disp = base; for (auto& c : disp) if (c == '_') c = ' '; bool dup = false; for (auto& x : extras) if (x.first == disp) dup = true; if (!dup && extras.size() < 50) extras.push_back({disp, path}); }
        else if (!rooms.count(key(base.c_str()))) rooms[key(base.c_str())] = path;
    }
    closedir(d);
}
Slot* load(const std::string& path) {
    for (Slot& s : slots) if (s.ready && s.path == path) { s.used = ++tick; return s.ok ? &s : nullptr; }
    Slot* s = &slots[0]; for (Slot& q : slots) if (!q.ready || q.used < s->used) s = &q;
    if (!s->ready) { if (!C3D_TexInit(&s->tex, 512, 256, GPU_RGBA8)) return nullptr; C3D_TexSetFilter(&s->tex, GPU_LINEAR, GPU_LINEAR); s->ready = true; }
    s->path = path; s->used = ++tick; s->ok = false;
    std::vector<uint8_t> px; int w = 0, h = 0;
    if (!spriteDecode(path, px, w, h, nullptr)) return nullptr;
    static uint8_t buf[400 * 240 * 4];
    for (int y = 0; y < 240; y++) for (int x = 0; x < 400; x++) {   // any size: scale to 400x240 (nearest)
        const uint8_t* p = &px[((size_t)(y * h / 240) * w + (x * w / 400)) * 4]; uint8_t* o = &buf[(y * 400 + x) * 4];
        o[0] = p[0]; o[1] = p[1]; o[2] = p[2]; o[3] = 255;
    }
    uploadBG(s->tex, buf, 400, 240);
    s->ok = true; return s;
}
void draw(Slot* s) {
    static Tex3DS_SubTexture sub = {400, 240, 0.f, 1.f, 400 / 512.f, 1.f - 240 / 256.f};
    C2D_Image img = {&s->tex, &sub};
    C2D_DrawImageAt(img, 0, 0, 0.5f, nullptr, 1.f, 1.f);
}
}

void bgScan() {
    rooms.clear(); extras.clear();
#ifdef MAPLE_VALLEY
    scanDir("sdmc:/LifeSim/Backgrounds", false); scanDir("romfs:/Backgrounds", false);
    scanDir("sdmc:/LifeSim/Backgrounds/Extra", true); scanDir("romfs:/Backgrounds/Extra", true);
    std::sort(extras.begin(), extras.end());
#endif
}
bool bgDrawRoom(int room, bool night) {
    if (room < 0 || room >= R_COUNT || rooms.empty()) return false;
    auto it = rooms.find(key(ROOM_NAMES[room])); if (it == rooms.end()) return false;
    Slot* s = load(it->second); if (!s) return false;
    draw(s);
    if (night) rect(0, 0, TOP_W, TOP_H, C(20, 12, 50, 110));
    return true;
}
int bgExtraCount() { return (int)extras.size(); }
const char* bgExtraName(int k) { return k >= 0 && k < (int)extras.size() ? extras[k].first.c_str() : "(Missing Background)"; }
void bgDrawExtra(int k) {
    if (k < 0 || k >= (int)extras.size()) { rect(0, 0, TOP_W, TOP_H, C(200, 200, 210)); return; }
    Slot* s = load(extras[k].second); if (s) draw(s); else rect(0, 0, TOP_W, TOP_H, C(200, 200, 210));
}
