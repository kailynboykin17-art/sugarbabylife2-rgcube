// Sprite art: a tiny PNG decoder (zlib inflate + PNG filters), background cut-out, fit-to-doll-canvas,
// and walking frames, so painted character art can replace the drawn dolls anywhere in the game.
#include "sprites.hpp"
#include "game.hpp"
#include "sim.hpp"
#include <dirent.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <map>

// ---------------------------------------------------------------- inflate (RFC 1951)
namespace {
struct Huff { uint16_t counts[16], syms[320]; };
struct InBits {
    const uint8_t* p; size_t n, i = 0; uint32_t bb = 0; int bc = 0; bool bad = false;
    int bit() { if (!bc) { if (i >= n) { bad = true; return 0; } bb = p[i++]; bc = 8; } int b = bb & 1; bb >>= 1; bc--; return b; }
    uint32_t bits(int k) { uint32_t v = 0; for (int j = 0; j < k; j++) v |= (uint32_t)bit() << j; return v; }
};
void build(Huff& h, const uint8_t* lens, int n) {
    uint16_t offs[16]; memset(h.counts, 0, sizeof h.counts);
    for (int i = 0; i < n; i++) h.counts[lens[i]]++;
    h.counts[0] = 0;
    uint16_t sum = 0; for (int i = 0; i < 16; i++) { offs[i] = sum; sum += h.counts[i]; }
    for (int i = 0; i < n; i++) if (lens[i]) h.syms[offs[lens[i]]++] = (uint16_t)i;
}
int decodeSym(InBits& in, const Huff& h) {
    int code = 0, first = 0, index = 0;
    for (int len = 1; len < 16; len++) {
        code |= in.bit(); int cnt = h.counts[len];
        if (code - first < cnt) return h.syms[index + code - first];
        index += cnt; first += cnt; first <<= 1; code <<= 1;
        if (in.bad) return -1;
    }
    return -1;
}
const uint16_t LBASE[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
const uint8_t LEXT[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
const uint16_t DBASE[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
const uint8_t DEXT[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
bool inflate(const uint8_t* src, size_t n, std::vector<uint8_t>& out, size_t limit) {
    InBits in; in.p = src; in.n = n;
    int last = 0;
    while (!last) {
        last = in.bit(); int type = (int)in.bits(2);
        if (in.bad) return false;
        if (type == 0) {
            in.bc = 0; if (in.i + 4 > n) return false;
            uint16_t len = (uint16_t)(src[in.i] | src[in.i + 1] << 8); in.i += 4;
            if (in.i + len > n || out.size() + len > limit) return false;
            out.insert(out.end(), src + in.i, src + in.i + len); in.i += len; continue;
        }
        if (type == 3) return false;
        Huff lt, dt; uint8_t lens[320];
        if (type == 1) {
            for (int i = 0; i < 288; i++) lens[i] = i < 144 ? 8 : i < 256 ? 9 : i < 280 ? 7 : 8;
            build(lt, lens, 288); for (int i = 0; i < 30; i++) lens[i] = 5; build(dt, lens, 30);
        } else {
            int hlit = (int)in.bits(5) + 257, hdist = (int)in.bits(5) + 1, hclen = (int)in.bits(4) + 4;
            static const uint8_t ORD[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
            uint8_t cl[19] = {0}; for (int i = 0; i < hclen; i++) cl[ORD[i]] = (uint8_t)in.bits(3);
            Huff ct; build(ct, cl, 19);
            int k = 0;
            while (k < hlit + hdist) {
                int sym = decodeSym(in, ct); if (sym < 0) return false;
                if (sym < 16) lens[k++] = (uint8_t)sym;
                else {
                    int rep = 0; uint8_t val = 0;
                    if (sym == 16) { if (!k) return false; val = lens[k - 1]; rep = 3 + (int)in.bits(2); }
                    else if (sym == 17) rep = 3 + (int)in.bits(3);
                    else rep = 11 + (int)in.bits(7);
                    if (k + rep > hlit + hdist) return false;
                    while (rep--) lens[k++] = val;
                }
            }
            build(lt, lens, hlit); build(dt, lens + hlit, hdist);
        }
        for (;;) {
            int sym = decodeSym(in, lt); if (sym < 0) return false;
            if (sym < 256) { if (out.size() >= limit) return false; out.push_back((uint8_t)sym); continue; }
            if (sym == 256) break;
            sym -= 257; if (sym >= 29) return false;
            int len = LBASE[sym] + (int)in.bits(LEXT[sym]);
            int ds = decodeSym(in, dt); if (ds < 0 || ds >= 30) return false;
            size_t dist = DBASE[ds] + in.bits(DEXT[ds]);
            if (dist > out.size() || out.size() + len > limit) return false;
            size_t from = out.size() - dist;
            for (int j = 0; j < len; j++) out.push_back(out[from + j]);
        }
    }
    return true;
}
uint32_t be32(const uint8_t* p) { return (uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]; }
}

bool spriteDecode(const std::string& path, std::vector<uint8_t>& rgba, int& W, int& H, std::string* err) {
    auto fail = [&](const char* m) { if (err) *err = m; return false; };
    FILE* f = fopen(path.c_str(), "rb"); if (!f) return fail("can't open");
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz < 57 || sz > 8 * 1024 * 1024) { fclose(f); return fail("file size"); }
    std::vector<uint8_t> d((size_t)sz); size_t got = fread(d.data(), 1, d.size(), f); fclose(f);
    if (got != d.size() || memcmp(d.data(), "\x89PNG\r\n\x1a\n", 8)) return fail("not a PNG");
    int depth = 0, ctype = 0, inter = 0; std::vector<uint8_t> idat, pal, trns;
    W = H = 0;
    for (size_t p = 8; p + 12 <= d.size();) {
        uint32_t len = be32(&d[p]); const uint8_t* type = &d[p + 4];
        if (p + 12 + len > d.size()) return fail("broken chunk");
        const uint8_t* c = &d[p + 8];
        if (!memcmp(type, "IHDR", 4)) { W = (int)be32(c); H = (int)be32(c + 4); depth = c[8]; ctype = c[9]; inter = c[12]; }
        else if (!memcmp(type, "PLTE", 4)) pal.assign(c, c + len);
        else if (!memcmp(type, "tRNS", 4)) trns.assign(c, c + len);
        else if (!memcmp(type, "IDAT", 4)) idat.insert(idat.end(), c, c + len);
        else if (!memcmp(type, "IEND", 4)) break;
        p += 12 + len;
    }
    if (W <= 0 || H <= 0 || W > 2048 || H > 2048) return fail("size must be up to 2048x2048");
    if (depth != 8) return fail("only 8-bit PNGs");
    if (inter) return fail("interlaced PNGs aren't supported");
    int ch = ctype == 0 ? 1 : ctype == 2 ? 3 : ctype == 3 ? 1 : ctype == 4 ? 2 : ctype == 6 ? 4 : 0;
    if (!ch) return fail("unknown color type");
    if (idat.size() < 3) return fail("no image data");
    const size_t stride = (size_t)W * ch, need = (stride + 1) * H;
    std::vector<uint8_t> raw; raw.reserve(need);
    if (!inflate(idat.data() + 2, idat.size() - 2, raw, need) || raw.size() < need) return fail("couldn't decompress");
    std::vector<uint8_t> cur(stride), prev(stride, 0);
    rgba.assign((size_t)W * H * 4, 255);
    for (int y = 0; y < H; y++) {
        const uint8_t* line = &raw[y * (stride + 1)]; int ft = line[0]; const uint8_t* s = line + 1;
        for (size_t x = 0; x < stride; x++) {
            int a = x >= (size_t)ch ? cur[x - ch] : 0, b = prev[x], c = x >= (size_t)ch ? prev[x - ch] : 0, v = s[x];
            switch (ft) {
            case 1: v += a; break; case 2: v += b; break; case 3: v += (a + b) / 2; break;
            case 4: { int pp = a + b - c, pa = std::abs(pp - a), pb = std::abs(pp - b), pc = std::abs(pp - c); v += (pa <= pb && pa <= pc) ? a : pb <= pc ? b : c; break; }
            default: break;
            }
            cur[x] = (uint8_t)v;
        }
        for (int x = 0; x < W; x++) {
            uint8_t* o = &rgba[((size_t)y * W + x) * 4]; const uint8_t* q = &cur[(size_t)x * ch];
            switch (ctype) {
            case 0: o[0] = o[1] = o[2] = q[0]; break;
            case 2: o[0] = q[0]; o[1] = q[1]; o[2] = q[2]; if (trns.size() >= 6 && q[0] == trns[1] && q[1] == trns[3] && q[2] == trns[5]) o[3] = 0; break;
            case 3: { int i = q[0]; if ((size_t)i * 3 + 2 < pal.size()) { o[0] = pal[i * 3]; o[1] = pal[i * 3 + 1]; o[2] = pal[i * 3 + 2]; } o[3] = (size_t)i < trns.size() ? trns[i] : 255; break; }
            case 4: o[0] = o[1] = o[2] = q[0]; o[3] = q[1]; break;
            case 6: memcpy(o, q, 4); break;
            }
        }
        std::swap(cur, prev);
    }
    return true;
}

// ---------------------------------------------------------------- sprite library
// Two layouts work side by side:
//   flat:    Sprites/london.png                  (one standing image; "m_" prefix = guy)
//   folders: Sprites/Rio/front.png, side.png (faces right), back.png, outfits/<Category>_<NN>.png, info.txt
namespace {
struct Char { std::string id, dir, file; bool folder = false, guy = false, kid = false, side = false; int tiny = 0; std::map<std::string, std::vector<std::string>> outfits; };
std::vector<Char> chars;
std::vector<std::string> names;
struct Cached { std::vector<uint8_t> big; uint32_t lastUse = 0; bool ok = false; };
std::map<std::string, Cached> cache;
uint32_t useTick = 0;
constexpr int CW = 96, CH = 208, WCW = 48, WCH = 104, FEET = 186;
std::string lower(std::string v) { for (auto& c : v) c = (char)tolower(c); return v; }
std::string norm(const std::string& id) { std::string l = lower(id); if (l.rfind("m_", 0) == 0) l = l.substr(2); size_t u = l.find('_'); return u == std::string::npos ? l : l.substr(0, u); }
bool exists(const std::string& p) { FILE* f = fopen(p.c_str(), "rb"); if (!f) return false; fclose(f); return true; }
bool isDir(const std::string& p) { DIR* d = opendir(p.c_str()); if (!d) return false; closedir(d); return true; }

void cutBackground(std::vector<uint8_t>& px, int W, int H) {
    bool anyAlpha = false; for (size_t i = 3; i < px.size(); i += 4) if (px[i] < 250) { anyAlpha = true; break; }
    if (anyAlpha) return;
    const uint8_t* c0 = &px[0]; int br = c0[0], bgc = c0[1], bb = c0[2];
    std::vector<int> stack; std::vector<uint8_t> seen((size_t)W * H, 0);
    auto push = [&](int x, int y) { if (x < 0 || y < 0 || x >= W || y >= H) return; size_t i = (size_t)y * W + x; if (seen[i]) return; seen[i] = 1;
        const uint8_t* p = &px[i * 4]; int d = std::abs(p[0] - br) + std::abs(p[1] - bgc) + std::abs(p[2] - bb); int lum = (p[0] + p[1] + p[2]) / 3;
        if (d < 90 && lum > 150) stack.push_back((int)i); };
    for (int x = 0; x < W; x++) { push(x, 0); push(x, H - 1); }
    for (int y = 0; y < H; y++) { push(0, y); push(W - 1, y); }
    while (!stack.empty()) { int i = stack.back(); stack.pop_back(); px[(size_t)i * 4 + 3] = 0; int x = i % W, y = i / W; push(x + 1, y); push(x - 1, y); push(x, y + 1); push(x, y - 1); }
}
bool fitToCanvas(const std::vector<uint8_t>& px, int W, int H, uint8_t* out) {
    int x0 = W, y0 = H, x1 = -1, y1 = -1;
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (px[((size_t)y * W + x) * 4 + 3] > 24) { x0 = std::min(x0, x); x1 = std::max(x1, x); y0 = std::min(y0, y); y1 = std::max(y1, y); }
    if (x1 < 0) return false;
    const int sw = x1 - x0 + 1, sh = y1 - y0 + 1;
    const float s = std::min(176.f / sh, 94.f / sw);   // small art (64x128 outfits) scales up, big art scales down; never stretched
    const int dw = std::max(1, (int)std::lround(sw * s)), dh = std::max(1, (int)std::lround(sh * s));
    const int ox = 48 - dw / 2, oy = FEET - dh;
    memset(out, 0, CW * CH * 4);
    for (int dy = 0; dy < dh; dy++) for (int dx = 0; dx < dw; dx++) {
        float fx0 = x0 + dx / s, fx1 = x0 + (dx + 1) / s, fy0 = y0 + dy / s, fy1 = y0 + (dy + 1) / s;
        float r = 0, g = 0, b = 0, a = 0, wsum = 0;
        for (int sy = (int)fy0; sy < (int)std::ceil(fy1) && sy < H; sy++) for (int sx = (int)fx0; sx < (int)std::ceil(fx1) && sx < W; sx++) {
            float wx = std::min(fx1, sx + 1.f) - std::max(fx0, (float)sx), wy = std::min(fy1, sy + 1.f) - std::max(fy0, (float)sy);
            float w = wx * wy; if (w <= 0) continue;
            const uint8_t* p = &px[((size_t)sy * W + sx) * 4]; float al = p[3] / 255.f;
            r += p[0] * al * w; g += p[1] * al * w; b += p[2] * al * w; a += al * w; wsum += w;
        }
        int X = ox + dx, Y = oy + dy; if (X < 0 || Y < 0 || X >= CW || Y >= CH || wsum <= 0) continue;
        uint8_t* o = out + (Y * CW + X) * 4; float A = a / wsum;
        if (A > 0.01f) { o[0] = (uint8_t)std::min(255.f, r / a); o[1] = (uint8_t)std::min(255.f, g / a); o[2] = (uint8_t)std::min(255.f, b / a); o[3] = (uint8_t)std::lround(A * 255); }
    }
    return true;
}
const uint8_t* bigFor(const std::string& path) {
    auto it = cache.find(path);
    if (it == cache.end()) {
        if (cache.size() >= 24) { auto old = cache.begin(); for (auto c = cache.begin(); c != cache.end(); ++c) if (c->second.lastUse < old->second.lastUse) old = c; cache.erase(old); }
        Cached c; c.big.resize(CW * CH * 4);
        std::vector<uint8_t> px; int W, H;
        if (spriteDecode(path, px, W, H, nullptr)) { cutBackground(px, W, H); c.ok = fitToCanvas(px, W, H, c.big.data()); }
        it = cache.emplace(path, std::move(c)).first;
    }
    it->second.lastUse = ++useTick;
    return it->second.ok ? it->second.big.data() : nullptr;
}
uint32_t hashStr(const std::string& s) { uint32_t h = 2166136261u; for (char c : s) h = (h ^ (uint8_t)c) * 16777619u; return h; }
const Char* findChar(const std::string& id) { for (auto& c : chars) if (c.id == id) return &c; return nullptr; }
void addFolder(const std::string& dir, const std::string& id) {
    Char c; c.id = id; c.dir = dir; c.folder = true;
    c.side = exists(dir + "/side.png");
    DIR* d = opendir((dir + "/outfits").c_str());
    if (d) { while (dirent* e = readdir(d)) { std::string n = e->d_name; if (n.size() < 5 || lower(n.substr(n.size() - 4)) != ".png") continue; size_t u = n.find('_'); c.outfits[lower(u == std::string::npos ? n.substr(0, n.size() - 4) : n.substr(0, u))].push_back(n); } closedir(d); }
    for (auto& o : c.outfits) std::sort(o.second.begin(), o.second.end());
    if (exists(dir + "/front.png")) c.file = dir + "/front.png";
    else if (c.outfits.count("everyday")) c.file = dir + "/outfits/" + c.outfits["everyday"][0];
    else return;   // nothing to stand with
    c.guy = lower(id).rfind("m_", 0) == 0;
    if (FILE* f = fopen((dir + "/info.txt").c_str(), "r")) {
        char b[128];
        while (fgets(b, sizeof b, f)) { std::string l = lower(b);
            if (l.find("gender") == 0) c.guy = l.find("guy") != std::string::npos || l.find("male") != std::string::npos && l.find("female") == std::string::npos || l.find("boy") != std::string::npos;
            if (l.find("age") == 0) { c.kid = l.find("kid") != std::string::npos || l.find("child") != std::string::npos; c.tiny = l.find("toddler") != std::string::npos ? 1 : (l.find("baby") != std::string::npos || l.find("infant") != std::string::npos) ? 2 : 0; if (c.tiny) c.kid = false; } }
        fclose(f);
    }
    chars.push_back(c);
}
// what's the Sim wearing right now? place-based outfits when the character has them
std::string lookFor(const Sim& s, const Char& c, bool* isFront) {
    *isFront = true;
    if (!c.folder || c.outfits.empty()) return c.file;
    const char* cat = nullptr; const int r = G.room;
    if (r == R_BEACH || r == R_ISLAND || r == R_ISLANDTOWN) cat = "swimwear";
    else if (r == R_GYM || r == R_TENNIS || r == R_RINK) cat = "sportswear";
    else if (r == R_CLUB || r == R_FRAT || r == R_BASEMENT) cat = "party";
    else if (r == R_CHAPEL || r == R_RESTAURANT || r == R_TOWER) cat = "formal";
    else if (r == R_SLOPES || r == R_LODGE || (seasonOf(G.day) == 3 && (r == R_PARK || r == R_STREET || r == R_YARD))) cat = "winter";
    if (cat) { auto it = c.outfits.find(cat); if (it != c.outfits.end() && !it->second.empty()) { *isFront = false; return c.dir + "/outfits/" + it->second[hashStr(s.name + cat) % it->second.size()]; } }
    auto ev = c.outfits.find("everyday");   // everyday: the outfit the Sim picked chooses among front + everyday looks
    int n = ev == c.outfits.end() ? 0 : (int)ev->second.size();
    int k = n ? s.outfit % (n + 1) : 0;
    if (k == 0 || !n) return c.file;
    *isFront = false; return c.dir + "/outfits/" + ev->second[k - 1];
}
}

void spriteScan() {
    names.clear(); cache.clear(); chars.clear();
#ifdef SBL2_CAST
    for (const char* dir : {SPRITE_DIR, "romfs:/Sprites", "romfs:/SBLCast", "sdmc:/LifeSim/SBLCast"}) {   // SD card first, then the packs built into the game
#else
    for (const char* dir : {SPRITE_DIR, "romfs:/Sprites"}) {   // SD card first, then the pack built into the game
#endif
        DIR* d = opendir(dir); if (!d) continue;
        std::vector<std::string> ents; while (dirent* e = readdir(d)) if (e->d_name[0] != '.') ents.push_back(e->d_name);
        closedir(d); std::sort(ents.begin(), ents.end());
        for (auto& n : ents) {
            std::string p = std::string(dir) + "/" + n;
            bool dup = false; std::string key = norm(n.size() > 4 && lower(n.substr(n.size() - 4)) == ".png" ? n.substr(0, n.size() - 4) : n);
            if (isDir(p)) { for (auto& c : chars) if (lower(c.id) == lower(n)) dup = true; if (!dup && chars.size() < 400) addFolder(p, n); continue; }
            if (n.size() < 5 || lower(n.substr(n.size() - 4)) != ".png") continue;
            std::string id = n.substr(0, n.size() - 4);
            for (auto& c : chars) if (lower(c.id) == lower(id) || (c.folder && norm(c.id) == key && !c.kid)) dup = true;   // a folder character wins over an old flat file
            if (dup || chars.size() >= 400) continue;
            Char c; c.id = id; c.dir = dir; c.file = p; c.guy = lower(id).rfind("m_", 0) == 0; chars.push_back(c);
        }
    }
    std::sort(chars.begin(), chars.end(), [](const Char& a, const Char& b) { return lower(a.id) < lower(b.id); });
    for (auto& c : chars) names.push_back(c.id);
}
const std::vector<std::string>& spriteNames() { return names; }
bool spriteIsGuy(const std::string& n) { const Char* c = findChar(n); return c ? c->guy : (n.size() > 2 && (n[0] == 'm' || n[0] == 'M') && n[1] == '_'); }
std::string spriteFor(const Sim& s) {
    if (chars.empty()) return "";
    if (s.stage < ST_CHILD) {   // babies and toddlers: their own little sprites
        const int want = s.stage == ST_TODDLER ? 1 : 2;
        std::vector<const Char*> pool;
        for (auto& c : chars) if (c.tiny == want && c.guy == (bool)s.look.guy) pool.push_back(&c);
        if (pool.empty()) for (auto& c : chars) if (c.tiny == want) pool.push_back(&c);
        if (pool.empty()) for (auto& c : chars) if (c.tiny) pool.push_back(&c);
        if (pool.empty()) return "";
        return pool[hashStr(s.name) % pool.size()]->id;
    }
    const bool kidSim = s.stage <= ST_PRETEEN;
    int idx = -1;
    if (!G.sims.empty() && &s >= &G.sims[0] && &s <= &G.sims.back()) idx = (int)(&s - &G.sims[0]);
    else for (size_t i = 0; i < G.sims.size(); i++) if (G.sims[i].name == s.name) { idx = (int)i; break; }
    if (idx >= 0 && idx < (int)G.spriteOf.size() && !G.spriteOf[idx].empty()) {
        if (G.spriteOf[idx] == "-") return "";   // explicitly "use the doll"
        if (findChar(G.spriteOf[idx])) return G.spriteOf[idx];
    }
    if (!G.spriteAuto) return "";
    {   // name match: "London Sterling" -> london.png, "Priya" (kid) -> Priya_Child
        std::string first = lower(s.name.substr(0, s.name.find(' ')));
        const Char* best = nullptr;
        for (auto& c : chars) if (!c.tiny && norm(c.id) == first) { if (!best || (c.kid == kidSim && best->kid != kidSim)) best = &c; }
        if (best && best->kid == kidSim) return best->id;
    }
    std::vector<const Char*> pool;
    for (auto& c : chars) if (!c.tiny && c.guy == (bool)s.look.guy && c.kid == kidSim) pool.push_back(&c);
    if (pool.empty()) for (auto& c : chars) if (!c.tiny && c.guy == (bool)s.look.guy) pool.push_back(&c);
    if (pool.empty()) for (auto& c : chars) if (!c.tiny) pool.push_back(&c);
    if (pool.empty()) return "";
    return pool[hashStr(s.name) % pool.size()]->id;
}
uint32_t spriteKey(const Sim& s) {
    std::string n = spriteFor(s); if (n.empty()) return 0;
    const Char* c = findChar(n); bool fr; std::string look = c ? lookFor(s, *c, &fr) : n;
    return hashStr(look) | 1;
}
bool spriteCanvas(const std::string& name, uint8_t* out) { const Char* c = findChar(name); const uint8_t* b = c ? bigFor(c->file) : nullptr; if (!b) return false; memcpy(out, b, CW * CH * 4); return true; }
// Sugar Bunny Life 2's own cast: pixel-doll NPCs drawn in the official SBL2 style (Sprites/SBLCast)
int spriteEnsureCast() {
#ifdef SBL2_CAST
    if (G.sims.empty()) return 0;
    static const char* SUR[] = {"Monroe", "Rivers", "Hart", "Bloom", "Sterling", "Vale", "Rose", "Cruz", "Banks", "Lane", "Knight", "Diaz", "Grant", "Reyes", "Blake", "Shaw"};
    int added = 0;
    for (const Char& c : chars) {
        if (c.dir.find("SBLCast") == std::string::npos) continue;
        bool have = false;
        for (size_t i = 0; i < G.spriteOf.size() && i < G.sims.size(); i++) if (G.spriteOf[i] == c.id) { have = true; break; }
        if (have) continue;
        std::string first = c.guy ? c.id.substr(2) : c.id;
        Sim s = randomSim(irand(4) == 0 ? ST_TEEN : ST_YA, c.guy ? 1 : 0);
        s.name = first + " " + SUR[hashStr(c.id) % 16];
        s.household = false;
        G.sims.push_back(s);
        if (G.spriteOf.size() < G.sims.size()) G.spriteOf.resize(G.sims.size());
        G.spriteOf[G.sims.size() - 1] = c.id;
        added++;
    }
    return added;
#else
    return 0;
#endif
}
bool spriteCast(uint32_t seed, bool guy, uint8_t* out) {   // story/app cast models: any adult sprite of that gender
    std::vector<const Char*> pool;
    for (auto& c : chars) if (c.guy == guy && !c.kid && !c.tiny) pool.push_back(&c);
    if (pool.empty()) return false;
    const Char* c = pool[(seed * 2654435761u >> 7) % pool.size()];
    const uint8_t* b = bigFor(c->file); if (!b) return false;
    memcpy(out, b, CW * CH * 4); return true;
}
bool spriteBig(const Sim& s, uint8_t* out) {
    std::string n = spriteFor(s); const Char* c = n.empty() ? nullptr : findChar(n); if (!c) return false;
    bool fr; const uint8_t* b = bigFor(lookFor(s, *c, &fr)); if (!b) b = bigFor(c->file); if (!b) return false;
    memcpy(out, b, CW * CH * 4); return true;
}
static void downsample(const uint8_t* src, uint8_t* cell) {
    for (int oy = 0; oy < WCH; oy++) for (int ox = 0; ox < WCW; ox++) {
        int r = 0, g = 0, b = 0, a = 0;
        for (int j = 0; j < 2; j++) for (int i = 0; i < 2; i++) { const uint8_t* p = src + ((oy * 2 + j) * CW + ox * 2 + i) * 4; r += p[0] * p[3]; g += p[1] * p[3]; b += p[2] * p[3]; a += p[3]; }
        uint8_t* o = cell + (oy * WCW + ox) * 4;
        if (a > 0) { o[0] = (uint8_t)(r / a); o[1] = (uint8_t)(g / a); o[2] = (uint8_t)(b / a); o[3] = (uint8_t)std::min(255, a / 4 + (a > 300 ? 40 : 0)); } else o[0] = o[1] = o[2] = o[3] = 0;
    }
}
static void stride(const uint8_t* big, uint8_t* walk, int fr) {   // shift the legs apart a little
    memset(walk, 0, CW * CH * 4);
    for (int y = 0; y < CH; y++) for (int x = 0; x < CW; x++) {
        const uint8_t* p = big + (y * CW + x) * 4; if (!p[3]) continue;
        int dx = 0; if (y >= 138) { int sw = (int)std::lround(std::min(1.f, (y - 138) / 48.f) * 4.f); bool left = x < 48; dx = (fr == 1) == left ? -sw : sw; }
        int nx = x + dx; if (nx < 0 || nx >= CW) continue;
        uint8_t* q = walk + (y * CW + nx) * 4; if (!q[3] || (fr == 1 ? x >= 48 : x < 48)) memcpy(q, p, 4);
    }
}
bool spriteWorld(const Sim& s, uint8_t* out) {
    std::string n = spriteFor(s); const Char* c = n.empty() ? nullptr : findChar(n); if (!c) return false;
    bool front; std::string look = lookFor(s, *c, &front);
    const uint8_t* big = bigFor(look); if (!big) big = bigFor(c->file); if (!big) return false;
    static uint8_t walk[CW * CH * 4], copy[CW * CH * 4];
    memcpy(copy, big, sizeof copy);   // the cache may evict while we load the side view
    downsample(copy, out);
    const uint8_t* side = (c->side && front) ? bigFor(c->dir + "/side.png") : nullptr;   // real side art for walking (front outfit only)
    for (int fr = 1; fr < 3; fr++) {
        stride(side ? side : copy, walk, side ? 3 - fr : fr);
        downsample(walk, out + (size_t)fr * WCW * WCH * 4);
    }
    return true;
}
