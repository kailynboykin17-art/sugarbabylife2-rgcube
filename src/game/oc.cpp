#include "designer.hpp"
#include "lifeext.hpp"
#include "cc.hpp"
// "Starlight Cast": original characters drawn in a detailed dress-up-doll pixel style
// (tall and slim, big dark glossy eyes with heavy lashes, overlined glossy lips, pink
// nose tips and knees, soft smooth shading, warm coloured line-art). They're static
// portraits: bodies, clothes and hair are painted at 2x and filtered down (soft,
// hand-anti-aliased edges), the face is placed pixel by pixel at full size, then the
// whole figure gets coloured line-art. Uploaded once as a nearest-filtered texture.
#include "sprites.hpp"
#include "oc.hpp"
#include <cmath>
#include <dirent.h>
#include <cstring>
#include <vector>
#include <algorithm>

// ============================================================== software canvas
namespace {
constexpr int CW = 96, CH = 208, SS = 2;
struct Px { uint8_t r, g, b, a; };
struct Canvas {
    int W = 0, H = 0;
    std::vector<Px> p;
    std::vector<uint8_t> part;
    void init(int w, int h) { W = w; H = h; p.assign(w * h, Px{0, 0, 0, 0}); part.assign(w * h, 0); }
    Px& at(int x, int y) { return p[y * W + x]; }
    const Px& at(int x, int y) const { return p[y * W + x]; }
    uint8_t& pt(int x, int y) { return part[y * W + x]; }
    void set(int x, int y, Px c, uint8_t id) {
        if (x < 0 || y < 0 || x >= W || y >= H || c.a == 0) return;
        Px& d = at(x, y);
        if (d.a == 0 && c.a < 255) return;   // translucent touches only land on something
        if (c.a == 255) { d = c; pt(x, y) = id; return; }
        float a = c.a / 255.f;
        d.r = (uint8_t)(d.r + (c.r - d.r) * a); d.g = (uint8_t)(d.g + (c.g - d.g) * a); d.b = (uint8_t)(d.b + (c.b - d.b) * a);
    }
    bool solid(int x, int y) const { return x >= 0 && y >= 0 && x < W && y < H && at(x, y).a > 0; }
};
Canvas g_final, g_big;
Canvas* C_ = &g_final;
float g_ss = 1.f;      // logical -> canvas pixel scale of the current canvas
uint8_t g_part = 1;
bool g_skin = false;   // skin shading gets a warm, rosy shadow
inline Px rgb(int r, int g, int b, int a = 255) { return Px{(uint8_t)std::clamp(r, 0, 255), (uint8_t)std::clamp(g, 0, 255), (uint8_t)std::clamp(b, 0, 255), (uint8_t)std::clamp(a, 0, 255)}; }
inline Px rgb(const RGB& c, int a = 255) { return rgb(c.r, c.g, c.b, a); }
inline Px sh(Px c, float f) { return rgb((int)(c.r * f), (int)(c.g * f), (int)(c.b * f), c.a); }
inline Px lit(Px c, float t) { return rgb((int)(c.r + (255 - c.r) * t), (int)(c.g + (255 - c.g) * t), (int)(c.b + (255 - c.b) * t), c.a); }
inline Px mixp(Px a, Px b, float t) { return rgb((int)(a.r + (b.r - a.r) * t), (int)(a.g + (b.g - a.g) * t), (int)(a.b + (b.b - a.b) * t), a.a); }
inline Px alpha(Px c, int a) { c.a = (uint8_t)a; return c; }
// volume shading: u in [-1,1] across the shape (left -> right), light from the top-left
inline Px vol(Px base, float u, float v = 0.f) {
    float d = std::clamp(0.62f * std::max(0.f, u) + 0.12f * std::max(0.f, u * u) + 0.25f * std::max(0.f, v), 0.f, 1.f);
    Px dark = g_skin ? rgb((int)(base.r * 0.86f), (int)(base.g * 0.70f), (int)(base.b * 0.70f)) : sh(base, 0.74f);
    Px c = mixp(base, dark, d);
    if (u < -0.35f) c = lit(c, (g_skin ? 0.16f : 0.12f) * (-u - 0.35f) / 0.65f);
    return c;
}
void plot(int x, int y, Px c) { C_->set(x, y, c, g_part); }
// ---- Chibi 2.0 body warp: a big head on a short body. Every shape is mapped through it, so all
// hairstyles/outfits keep working. Head (y < ~41) grows, the body below the neck is squashed; feet stay on 186.
bool g_chibiOn = false;
}   // namespace
bool g_dollV2 = false;   // SBL2 doll proportions: bigger head & eyes, longer slimmer body (toggle in Settings)
#ifdef CHIBI2
bool g_chibi2 = true;    // Sugar Bunny Life 2.0: chibi art
#else
bool g_chibi2 = false;
#endif
namespace {
float CHI_HEAD = 1.62f, CHI_BODY = 0.56f, CHI_BODYX = 1.18f, CHI_N0 = 40.f, CHI_N1 = 50.f;
inline float chiT(float y) { float t = std::clamp((y - CHI_N0) / (CHI_N1 - CHI_N0), 0.f, 1.f); return t * t * (3 - 2 * t); }
inline float chiSX(float y) { return CHI_HEAD + (CHI_BODYX - CHI_HEAD) * chiT(y); }
inline float chiSY(float y) { return CHI_HEAD + (CHI_BODY - CHI_HEAD) * chiT(y); }
float g_chiY[420];   // y (in half-units, 0..209) -> warped y
bool g_chiInit = false;
void chiInit() {
    static float last = 0; if (g_chiInit && last == CHI_HEAD * 7 + CHI_BODY) return; g_chiInit = true; last = CHI_HEAD * 7 + CHI_BODY;
    // integrate upward from the feet so row 186 stays put
    float acc = 186.f; g_chiY[372] = 186.f;
    for (int i = 371; i >= 0; i--) { float y = (i + 0.5f) * 0.5f; acc -= chiSY(y) * 0.5f; g_chiY[i] = acc; }
    acc = 186.f; for (int i = 373; i < 420; i++) { float y = (i - 0.5f) * 0.5f; acc += chiSY(y) * 0.5f; g_chiY[i] = acc; }
}
inline float chiY(float y) { float f = std::clamp(y * 2.f, 0.f, 418.f); int i = (int)f; float t = f - i; return g_chiY[i] + (g_chiY[i + 1] - g_chiY[i]) * t; }
inline float chiX(float x, float y) { return 48.f + (x - 48.f) * chiSX(y); }

// a logical pixel (a 2x2 block on the big canvas)
void dot(int x, int y, Px c) {
    if (g_chibiOn) {   // warped: cover the whole stretched cell so faces have no gaps
        const float x0 = chiX((float)x, y + 0.5f), x1 = chiX(x + 1.f, y + 0.5f), y0 = chiY((float)y), y1 = chiY(y + 1.f);
        for (int py = (int)std::floor(y0 * g_ss); py < (int)std::ceil(y1 * g_ss); py++) for (int px = (int)std::floor(x0 * g_ss); px < (int)std::ceil(x1 * g_ss); px++) plot(px, py, c);
        return;
    }
    int k = (int)g_ss; for (int j = 0; j < k; j++) for (int i = 0; i < k; i++) plot(x * k + i, y * k + j, c); }
// --- shape primitives take logical (final-size) coordinates and scale to the canvas
void ell(float cx, float cy, float rx, float ry, Px c, bool shade = true) {
    if (g_chibiOn) { rx *= chiSX(cy); ry *= chiSY(cy); cx = chiX(cx, cy); cy = chiY(cy); }
    cx *= g_ss; cy *= g_ss; rx *= g_ss; ry *= g_ss;
    for (int y = (int)std::floor(cy - ry); y <= (int)std::ceil(cy + ry); y++)
        for (int x = (int)std::floor(cx - rx); x <= (int)std::ceil(cx + rx); x++) {
            float dx = (x + 0.5f - cx) / rx, dy = (y + 0.5f - cy) / ry;
            if (dx * dx + dy * dy <= 1.f) plot(x, y, shade ? vol(c, dx, dy) : c);
        }
}
void poly(std::vector<float> xy, Px c, bool shade = true) {
    if (g_chibiOn) for (size_t i = 0; i + 1 < xy.size(); i += 2) { float x = xy[i], y = xy[i + 1]; xy[i] = chiX(x, y); xy[i + 1] = chiY(y); }
    for (float& v : xy) v *= g_ss;
    int n = (int)xy.size() / 2;
    float y0 = 1e9f, y1 = -1e9f;
    for (int i = 0; i < n; i++) { y0 = std::min(y0, xy[2 * i + 1]); y1 = std::max(y1, xy[2 * i + 1]); }
    for (int y = (int)std::floor(y0); y <= (int)std::ceil(y1); y++) {
        float fy = y + 0.5f;
        std::vector<float> xs;
        for (int i = 0; i < n; i++) {
            float ax = xy[2 * i], ay = xy[2 * i + 1], bx = xy[2 * ((i + 1) % n)], by = xy[2 * ((i + 1) % n) + 1];
            if ((ay <= fy && by > fy) || (by <= fy && ay > fy)) xs.push_back(ax + (fy - ay) / (by - ay) * (bx - ax));
        }
        std::sort(xs.begin(), xs.end());
        for (size_t k = 0; k + 1 < xs.size(); k += 2) {
            int a = (int)std::lround(xs[k]), b = (int)std::lround(xs[k + 1]) - 1;
            for (int x = a; x <= b; x++) { float u = b > a ? (x - a) / (float)(b - a) * 2.f - 1.f : 0.f; plot(x, y, shade ? vol(c, u) : c); }
        }
    }
}
// tapered capsule (limbs, braids, straps) with cross-section shading
void limb(float x0, float y0, float r0, float x1, float y1, float r1, Px c, bool shade = true) {
    if (g_chibiOn) { r0 *= chiSX(y0); r1 *= chiSX(y1); x0 = chiX(x0, y0); x1 = chiX(x1, y1); y0 = chiY(y0); y1 = chiY(y1); }
    x0 *= g_ss; y0 *= g_ss; r0 *= g_ss; x1 *= g_ss; y1 *= g_ss; r1 *= g_ss;
    float minx = std::min(x0 - r0, x1 - r1), maxx = std::max(x0 + r0, x1 + r1), miny = std::min(y0 - r0, y1 - r1), maxy = std::max(y0 + r0, y1 + r1);
    float dx = x1 - x0, dy = y1 - y0, L2 = dx * dx + dy * dy, L = std::sqrt(L2) + 1e-4f;
    for (int y = (int)std::floor(miny); y <= (int)std::ceil(maxy); y++)
        for (int x = (int)std::floor(minx); x <= (int)std::ceil(maxx); x++) {
            float px = x + 0.5f, py = y + 0.5f;
            float t = L2 > 0 ? std::clamp(((px - x0) * dx + (py - y0) * dy) / L2, 0.f, 1.f) : 0.f;
            float cx = x0 + dx * t, cy = y0 + dy * t, r = r0 + (r1 - r0) * t;
            float ex = px - cx, ey = py - cy;
            if (ex * ex + ey * ey <= r * r) {
                float u = (ex * (dy / L) - ey * (dx / L)) / std::max(0.5f, r);   // signed distance across the limb
                if (dy < 0) u = -u;
                plot(x, y, shade ? vol(c, -u) : c);
            }
        }
}
void line(float x0, float y0, float x1, float y1, Px c) {
    if (g_chibiOn && g_ss <= 1.f) { x0 = chiX(x0, y0); x1 = chiX(x1, y1); y0 = chiY(y0); y1 = chiY(y1); }
    if (g_ss > 1.f) { limb(x0, y0, 0.5f, x1, y1, 0.5f, c, false); return; }
    int n = (int)std::ceil(std::max(std::fabs(x1 - x0), std::fabs(y1 - y0))) + 1;
    for (int i = 0; i <= n; i++) { float t = n ? i / (float)n : 0; plot((int)std::lround(x0 + (x1 - x0) * t - 0.5f), (int)std::lround(y0 + (y1 - y0) * t - 0.5f), c); }
}
// Chibi 2.0: reshape the finished doll (big head, short body), resampling the final canvas
float chiInvY(float yp) {   // warped y -> original y
    int lo = 0, hi = 418;
    if (yp <= g_chiY[0]) return -1.f;
    if (yp >= g_chiY[418]) return 209.f;
    while (hi - lo > 1) { int m = (lo + hi) / 2; if (g_chiY[m] <= yp) lo = m; else hi = m; }
    float t = (yp - g_chiY[lo]) / std::max(1e-4f, g_chiY[hi] - g_chiY[lo]);
    return (lo + t) * 0.5f;
}
void chibiWarp() {
    static Canvas src; src = g_final;
    for (int y = 0; y < CH; y++) {
        float sy = chiInvY(y + 0.5f);
        for (int x = 0; x < CW; x++) {
            Px& d = g_final.at(x, y); d = Px{0, 0, 0, 0}; g_final.pt(x, y) = 0;
            if (sy < 0 || sy >= CH) continue;
            float sx = 48.f + (x + 0.5f - 48.f) / chiSX(sy);
            // 2x2 supersample for smooth enlarged pixels; keep hard alpha edges
            int r = 0, g = 0, b = 0, a = 0, n = 0; uint8_t id = 0;
            for (int j = 0; j < 2; j++) for (int i = 0; i < 2; i++) {
                float qx = sx + (i - 0.5f) * 0.5f / chiSX(sy), qy = sy + (j - 0.5f) * 0.5f / chiSY(sy);
                int ix = (int)std::floor(qx), iy = (int)std::floor(qy);
                if (ix < 0 || iy < 0 || ix >= CW || iy >= CH) continue;
                const Px& s2 = src.at(ix, iy); a += s2.a; if (s2.a) { r += s2.r; g += s2.g; b += s2.b; n++; id = src.pt(ix, iy); }
            }
            if (n && a / 4 >= 100) { d = rgb(r / n, g / n, b / n); g_final.pt(x, y) = id; }
        }
    }
}
// a wavy strand of hair: a thick sine-curve from (x,y0) down to y1
void strand(float x, float y0, float y1, float amp, float freq, float ph, float w, Px c) {
    for (float y = y0; y <= y1; y += 0.25f) {
        float k = (y - y0) / std::max(1.f, y1 - y0);
        float cx = x + std::sin(ph + y * freq) * amp * (0.4f + 0.6f * k);
        float ww = w * (1.f - 0.45f * k * k);
        for (float xx = cx - ww; xx <= cx + ww; xx += 0.25f) { float u = (xx - cx) / ww;
            float px = xx, py = y; if (g_chibiOn) { px = chiX(xx, y); py = chiY(y); }
            plot((int)std::floor(px * g_ss), (int)std::floor(py * g_ss), vol(c, u));
            if (g_chibiOn && chiSY(y) > 1.f) plot((int)std::floor(px * g_ss), (int)std::floor((py + 0.25f * chiSY(y)) * g_ss), vol(c, u)); }
    }
}
// big canvas -> final canvas (2x2 box filter); hard alpha edges, soft colours
void downsample(bool over) {
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            int r = 0, g = 0, b = 0, a = 0, n = 0; uint8_t id = 0; int best = -1;
            for (int j = 0; j < SS; j++) for (int i = 0; i < SS; i++) {
                const Px& s = g_big.at(x * SS + i, y * SS + j);
                a += s.a;
                if (s.a) { r += s.r; g += s.g; b += s.b; n++; if (s.a > best) { best = s.a; id = g_big.pt(x * SS + i, y * SS + j); } }
            }
            if (!n) continue;
            a /= SS * SS;
            Px c = rgb(r / n, g / n, b / n);
            Px& d = g_final.at(x, y);
            if (!over || d.a == 0) { if (a >= 128) { d = c; g_final.pt(x, y) = id; } }
            else { float k = a / 255.f; d = mixp(d, c, k); d.a = 255; if (a >= 128) g_final.pt(x, y) = id; }
        }
}
void useBig() { g_big.init(CW * SS, CH * SS); C_ = &g_big; g_ss = (float)SS; }
void useFinal() { C_ = &g_final; g_ss = 1.f; }
// coloured line-art around the figure and between overlapping parts
void inkPass() {
    Canvas& F = g_final;
    std::vector<Px> out = F.p;
    auto lineCol = [&](const Px& c, uint8_t id) {
        if (id == 20 || id == 21) return rgb((int)(c.r * 0.74f), (int)(c.g * 0.52f), (int)(c.b * 0.52f));   // soft rosy skin line
        return sh(c, 0.46f);
    };
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            const Px& c = F.at(x, y);
            if (c.a == 0) {
                const int nx[4] = {x - 1, x + 1, x, x}, ny[4] = {y, y, y - 1, y + 1};
                for (int k = 0; k < 4; k++) if (F.solid(nx[k], ny[k])) { out[y * CW + x] = lineCol(F.at(nx[k], ny[k]), F.pt(nx[k], ny[k])); break; }
                continue;
            }
            const int nx[2] = {x, x - 1}, ny[2] = {y - 1, y};
            for (int k = 0; k < 2; k++) {
                if (!F.solid(nx[k], ny[k])) continue;
                uint8_t a = F.pt(x, y), b = F.pt(nx[k], ny[k]);
                if (a == b || a >= 100 || b >= 100) continue;   // face details & jewellery don't get line-art
                const Px& o = F.at(nx[k], ny[k]);
                int d = std::abs(c.r - o.r) + std::abs(c.g - o.g) + std::abs(c.b - o.b);
                if (d > 70) { out[y * CW + x] = mixp(c, lineCol(c, a), 0.55f); break; }
            }
        }
    F.p = out;
}
// ============================================================== the figure
struct Look {
    RGB skin, hair, eyes, lips;
    int hair_style;   // 0 long curls, 1 messy bun, 2 box braids, 3 high ponytail, 4 bob+bangs, 5 afro puff, 6 short waves, 7 spiky frosted
    int top, bottom, shoes;
    RGB topC, topC2, botC, botC2, shoeC;
    uint64_t extras;
    bool guy;
    RGB shadow;   // coloured eyeshadow (black = none)
    RGB accC;     // leg warmers / hat colour (black = default)
    uint8_t beard = 0;   // 0 none .. 9 (see Create-a-Sim facial hair names)
    uint8_t eyeShape = 0, brow = 0, nose = 0, lipSh = 0;   // Create-a-Sim face sliders (0 = the classic doll face)
    uint8_t tattoos = 0;   // bitmask (lifeext Body app): sleeve, love/hate, heart, cross, rose, face stars, butterfly, ankle vine
    uint8_t nails = 0;  // 0 natural pink, 1.. polish colours (salon)
    uint8_t body = 0;   // 0 slim, 1 mid-size, 2 plus-size, 3 Body Tea (snatched waist, long torso)
    uint8_t liner = 0, lash = 0, contour = 0, blushStyle = 0, glow = 0, blushOn = 0; RGB blushC{246, 140, 150};   // makeup (Create-a-Sim "Pretty" page)
    int design = -1;   // Design Studio slot painted onto the top (-1 none)
    uint8_t chead = 0; // Custom Head (Create-a-Sim > Pretty): 0 none, 1..7 = Heads/ folder entry
};
enum : uint64_t { X_FLOWER = 1, X_BAG = 2, X_NECK = 4, X_HOOPS = 8, X_BANGLES = 16, X_CHAINBELT = 32, X_PHONE = 64, X_VISOR = 128, X_CHOKER = 256, X_FISHNET = 512,
                  X_SOCKS = 1024, X_TINTS = 2048, X_CUFFS = 4096, X_STUDS = 8192, X_HEADPH = 16384,
                  X_STARNECK = 1u << 15, X_STARCLIP = 1u << 16, X_FLOWERCLIPS = 1u << 17, X_STARHOOPS = 1u << 18, X_PEARLBELLY = 1u << 19, X_BEADS = 1u << 20,
                  X_NAVEL = 1u << 21, X_ANKLET = 1u << 22, X_PEACE = 1u << 23, X_LIPRING = 1u << 24, X_NAILS = 1u << 25, X_HAIRCLIP = 1u << 26, X_RINGBELT = 1u << 27,
                  X_CROSS = 1ull << 28, X_BUCKLE = 1ull << 29, X_CAMERA = 1ull << 30, X_HEART = 1ull << 31, X_STUBBLE = 1ull << 32, X_TOTE = 1ull << 33, X_STUDBELT = 1ull << 34,
                  X_WRISTBAND = 1ull << 35, X_GLASSES = 1ull << 36, X_FRECKLES = 1ull << 37, X_HEARTHAIR = 1ull << 38, X_ZEBRA = 1ull << 39, X_WHITEPH = 1ull << 40,
                  X_NOSERING = 1ull << 41, X_HIGHLIGHT = 1ull << 42, X_SHADES = 1ull << 43,
                  X_FURHAT = 1ull << 44, X_NEWSBOY = 1ull << 45, X_HEADBAND = 1ull << 46, X_WHITEFLOWER = 1ull << 47, X_BIGHOOPS = 1ull << 48,
                  X_GLOVES = 1ull << 49, X_SILVERCHAIN = 1ull << 50, X_LEGWARM = 1ull << 51, X_TIGHTS = 1ull << 52, X_LEOPARD = 1ull << 53,
                  X_STARSKIRT = 1ull << 54, X_COFFEE = 1ull << 55, X_CHAINLACE = 1ull << 56, X_GOLDCLIPS = 1ull << 57, X_CAP = 1ull << 58 };
// tops
enum { T_BABYTEE, T_JERSEY, T_HALTER, T_TRACKZIP, T_SLIP, T_BUTTERFLY, T_TUBE, T_BIGTEE, T_BASEBALL, T_CAMI, T_GRAPHIC, T_SWIRL, T_POLKATUBE, T_CRISSCROSS, T_TEXTTEE, T_MOTO, T_SHRUG, T_POLO,
       T_HOODIE, T_STARHOODIE, T_FURCOAT, T_STRIPEPOLO, T_SKULLTEE, T_BODYCON, T_BABYDOLL, T_CROPTEE,
       T_DENIMVEST, T_JERSEYTANK, T_FACETEE, T_STRAPLESS, T_TURTLENECK, T_PINCARDIGAN, T_GINGHAMTUBE, T_WEBTOP, T_BOWTEE };
// bottoms
enum { B_RUFFLE, B_JORTS, B_FLARE, B_TRACKPANT, B_NONE, B_DENIMMINI, B_CARGO, B_BAGGYJEAN, B_CARGOSHORT, B_WIDELEG, B_CAPRI, B_MICRO, B_MAXI, B_WRAP, B_MIDI, B_PLAID, B_GOWN };
// shoes
enum { S_STRAPPY, S_SNEAK, S_PLATFORM, S_CHUNKY, S_COMBAT, S_KNEEBOOT, S_WORKBOOT, S_FLIPFLOP, S_WEDGE, S_STUDHEEL };
// recolour the pixels a part just painted (big canvas) with a pattern function
template <class F> void pattern(uint8_t id, float x0, float y0, float x1, float y1, F fn) {
    for (int y = (int)(y0 * g_ss); y < (int)(y1 * g_ss); y++) for (int x = (int)(x0 * g_ss); x < (int)(x1 * g_ss); x++)
        if (x >= 0 && y >= 0 && x < C_->W && y < C_->H && C_->pt(x, y) == id && C_->at(x, y).a) { Px o = C_->at(x, y); Px n; if (fn(x / g_ss, y / g_ss, o, n)) C_->at(x, y) = n; }
}

const float HX = 48, HY = 28;   // head centre

void hibiscus(float cx, float cy, float r, Px c);
// a three-strand braid from (x0,y0) to (x1,y1): alternating little lobes along the path
void plait(float x0, float y0, float x1, float y1, float w, Px H, Px D, Px Hi) {
    float dx = x1 - x0, dy = y1 - y0, len = std::sqrt(dx * dx + dy * dy); if (len < 1) return;
    float nx = -dy / len, ny = dx / len; int n = (int)(len / (w * 1.15f)) + 1;
    for (int i = 0; i <= n; i++) {
        float k = i / (float)n, s = (i & 1) ? 1.f : -1.f, ww = w * (1.f - 0.25f * k);
        float cx = x0 + dx * k + nx * s * ww * 0.35f, cy = y0 + dy * k + ny * s * ww * 0.35f;
        ell(cx, cy, ww * 0.75f, ww * 0.6f, (i & 1) ? H : D);
        line(cx - nx * ww * 0.5f, cy - ny * ww * 0.5f, cx + nx * ww * 0.1f + dx / len, cy + ny * ww * 0.1f + dy / len, Hi);
    }
}
// big soft waves down to y1 (shared by several new styles)
static void wavesBack(Px H, Px D, Px Hi, float w, float y1) {
    ell(HX, HY - 2, w, 15.5f, H);
    for (int i = 0; i < 9; i++) { float x = HX - w + 2 + i * (2 * w - 4) / 8.f; strand(x, HY, y1 - std::fabs(i - 4) * 3.f, 2.8f, 0.26f, i * 1.2f, 3.6f, i % 3 == 1 ? D : H); }
    for (int i = 0; i < 5; i++) strand(HX - w + 5 + i * (2 * w - 10) / 4.f, HY + 8, y1 - 6, 2.6f, 0.26f, i * 1.7f, 1.0f, Hi);
}
static void curlsBack(Px H, Px D, Px Hi, float w, float y1) {
    ell(HX, HY - 2, w, 16, H);
    for (int i = 0; i < 70; i++) { float k = (i * 37 % 70) / 70.f, x = HX + std::sin(i * 2.3f) * (w - 2 + 3 * k), y = HY + 2 + k * (y1 - HY - 4); ell(x, y, 3.6f, 3.2f, i % 3 == 0 ? D : i % 3 == 1 ? H : Hi); }
}
void drawHairBack(const Look& L) {
    if (getenv("HSDBG")) fprintf(stderr, "back %d\n", L.hair_style);
    Px H = rgb(L.hair), D = sh(H, 0.78f), Hi = lit(H, 0.28f);
    g_part = 10;
    switch (L.hair_style) {
    case 0:   // big glossy curls past the chest
        ell(HX, HY - 2, 17, 16, H);
        for (int i = 0; i < 9; i++) { float x = 30 + i * 4.5f; strand(x, HY, 104 - std::fabs(i - 4) * 4.f, 3.2f, 0.32f, i * 1.3f, 3.6f, i % 3 == 1 ? D : H); }
        for (int i = 0; i < 5; i++) strand(33 + i * 7.5f, HY + 6, 92, 2.6f, 0.36f, i * 2.1f, 1.0f, Hi);
        break;
    case 1:   // messy high bun with face-framing pieces
        ell(HX, HY - 3, 13.5f, 13, H);
        ell(HX + 3, HY - 17, 7, 6, H); strand(HX + 5, HY - 25, HY - 19, 2.5f, 0.8f, 0.f, 1.2f, H); strand(HX - 1, HY - 25, HY - 19, 2.f, 0.9f, 1.f, 1.f, Hi);
        break;
    case 2:   // waist-length box braids
        ell(HX, HY - 2, 15, 15, H);
        for (int i = 0; i < 11; i++) { float x = 32 + i * 3.2f; limb(x, HY, 1.8f, x + (i - 5) * 0.6f, 120 - std::fabs(i - 5) * 2.f, 1.4f, i % 2 ? D : H); }
        break;
    case 3:   // sleek high ponytail
        ell(HX, HY - 1, 12.5f, 13, H);
        ell(HX + 8, HY - 12, 3, 3, H); strand(HX + 15, HY - 10, 92, 2.5f, 0.12f, 0.6f, 3.6f, H); strand(HX + 15, HY - 8, 88, 2.2f, 0.12f, 0.6f, 1.0f, Hi);
        break;
    case 4:   // chin-length bob
        ell(HX, HY - 1, 15, 15, H); poly({HX - 15, HY, HX + 15, HY, HX + 16, HY + 14, HX - 16, HY + 14}, H);
        break;
    case 5:   // big afro puff
        ell(HX, HY - 4, 13, 12, H); ell(HX, HY - 20, 13, 10, H);
        for (int i = 0; i < 40; i++) { float a = i * 0.7f, r = 4 + (i * 37 % 9); dot((int)(HX + std::cos(a) * r), (int)(HY - 20 + std::sin(a) * r * 0.7f), i % 2 ? D : Hi); }
        break;
    case 8: case 9:   // long sleek straight hair
        ell(HX, HY - 2, 14.5f, 14.5f, H);
        poly({HX - 14, HY - 2, HX + 14, HY - 2, HX + 15, 118, HX - 15, 118}, H);
        for (int i = 0; i < 8; i++) { float x = HX - 13 + i * 3.7f; strand(x, HY, 120, 0.3f, 0.2f, i * 1.f, 0.6f, i % 2 ? D : Hi); }
        break;
    case 10:  // big round afro
        ell(HX, HY - 4, 23, 21, H);
        for (int i = 0; i < 110; i++) { float a = i * 2.39996f, r = std::sqrt((float)i / 110.f) * 21.f; ell(HX + std::cos(a) * r, HY - 4 + std::sin(a) * r * 0.92f, 2.4f, 2.2f, i % 3 ? D : lit(H, 0.18f)); }
        break;
    case 11: ell(HX, HY - 3, 13.5f, 13, H); poly({HX - 13, HY - 2, HX + 13, HY - 2, HX + 12, HY + 8, HX - 12, HY + 8}, H); break;   // shaggy
    case 12: ell(HX, HY - 4, 14, 12.5f, H); for (int i = 0; i < 40; i++) { float a = i * 2.39996f, r = std::sqrt((float)i / 40.f) * 12.f; ell(HX + std::cos(a) * r, HY - 6 + std::sin(a) * r * 0.8f, 2.f, 1.8f, i % 2 ? D : lit(H, 0.15f)); } break;   // curls
    case 6: ell(HX, HY - 3, 12.5f, 12, H); break;   // short waves
    case 13:  // two long braids (they hang in front; see drawHairFront)
        ell(HX, HY - 2, 14, 14, H); poly({HX - 13, HY - 2, HX + 13, HY - 2, HX + 12, HY + 12, HX - 12, HY + 12}, H);
        break;
    case 14: break;   // buzz cut: all in front
    case 15: ell(HX, HY - 3, 13, 12.5f, H); break;   // space buns (the buns are in front)
    case 16:  // big afro with curly bangs
        ell(HX, HY - 4, 23, 21, H);
        for (int i = 0; i < 110; i++) { float a = i * 2.39996f, r = std::sqrt((float)i / 110.f) * 21.f; ell(HX + std::cos(a) * r, HY - 4 + std::sin(a) * r * 0.92f, 2.4f, 2.2f, i % 3 ? D : lit(H, 0.18f)); }
        break;
    case 17: ell(HX, HY - 2, 12.8f, 13, H); break;   // slicked back (braid in front)
    // ---- new styles (Sugar Bunny Life)
    case 18:   // Y2K flip: chin bob with flipped-out ends
        ell(HX, HY - 2, 15, 15, H); poly({HX - 15, HY, HX + 15, HY, HX + 15.5f, HY + 12, HX - 15.5f, HY + 12}, H);
        ell(HX - 17, HY + 12, 4.5f, 2.6f, H); ell(HX + 17, HY + 12, 4.5f, 2.6f, H);
        line(HX - 19, HY + 11, HX - 15, HY + 11, Hi); line(HX + 15, HY + 11, HX + 19, HY + 11, Hi);
        break;
    case 19: case 26: case 23:   // long straight / layered
        ell(HX, HY - 2, L.hair_style == 23 ? 16.f : 14.5f, L.hair_style == 23 ? 16.f : 14.5f, H);
        if (L.hair_style == 23) poly({HX - 16, HY, HX + 16, HY, HX + 19, HY + 30, HX + 15, 106, HX - 15, 106, HX - 19, HY + 30}, H);
        else poly({HX - 14, HY - 2, HX + 14, HY - 2, HX + 15, 116, HX - 15, 116}, H);
        for (int i = 0; i < 8; i++) { float x = HX - 13 + i * 3.7f; strand(x, HY, L.hair_style == 23 ? 104.f : 114.f, L.hair_style == 23 ? 1.4f : 0.3f, 0.2f, i * 1.f, 0.6f, i % 2 ? D : Hi); }
        if (L.hair_style == 23) for (int sd = -1; sd <= 1; sd += 2) for (int k = 0; k < 3; k++) strand(HX + sd * (16 + k), HY + 10 + k * 14, HY + 24 + k * 14, 1.2f, 0.5f, k * 1.f, 1.6f, k % 2 ? D : H);
        break;
    case 20:   // twisted updo with a claw clip
        ell(HX, HY - 3, 13.5f, 13, H); ell(HX + 2, HY - 14, 8.5f, 6.5f, H);
        for (int i = 0; i < 4; i++) strand(HX - 2 + i * 3, HY - 21, HY - 9, 1.5f, 0.8f, i * 1.3f, 1.2f, i % 2 ? D : Hi);
        break;
    case 21:   // low pigtail puffs
        ell(HX, HY - 3, 13, 12.5f, H);
        for (int sd = -1; sd <= 1; sd += 2) {
            ell(HX + sd * 17, HY + 6, 7.5f, 7.5f, H);
            for (int i = 0; i < 18; i++) { float a = i * 2.39996f, r = std::sqrt((float)i / 18.f) * 6.5f; ell(HX + sd * 17 + std::cos(a) * r, HY + 6 + std::sin(a) * r, 1.9f, 1.7f, i % 2 ? D : lit(H, 0.15f)); }
        }
        break;
    case 22:   // braids in a high ponytail
        ell(HX, HY - 1, 12.5f, 13, H); ell(HX + 6, HY - 13, 4, 3.5f, H);
        for (int i = 0; i < 7; i++) limb(HX + 6 + (i - 3) * 1.4f, HY - 13, 1.4f, HX + 14 + (i - 3) * 2.2f, 104 - std::abs(i - 3) * 3.f, 1.2f, i % 2 ? D : H);
        break;
    case 24:   // wolf cut: choppy layers to the neck
        ell(HX, HY - 3, 15, 14, H);
        poly({HX - 15, HY - 2, HX + 15, HY - 2, HX + 14, HY + 18, HX + 8, HY + 14, HX + 3, HY + 20, HX - 3, HY + 15, HX - 8, HY + 19, HX - 14, HY + 16}, H);
        for (int i = 0; i < 5; i++) line(HX - 12 + i * 6, HY + 6, HX - 13 + i * 6, HY + 16, D);
        break;
    case 25:   // high twin tails
        ell(HX, HY - 2, 13, 13, H);
        for (int sd = -1; sd <= 1; sd += 2) {
            ell(HX + sd * 12, HY - 12, 3.5f, 3.5f, H);
            strand(HX + sd * 19, HY - 10, 100, 2.5f, 0.12f, sd * 1.f, 4.2f, H);
            strand(HX + sd * 19, HY - 8, 96, 2.2f, 0.12f, sd * 1.f, 1.2f, Hi);
        }
        break;
    case 27:   // low curly bun
        ell(HX, HY - 3, 13, 12.5f, H); ell(HX, HY + 9, 8.5f, 7, H);
        for (int i = 0; i < 20; i++) { float a = i * 2.39996f, r = std::sqrt((float)i / 20.f) * 7.f; ell(HX + std::cos(a) * r, HY + 9 + std::sin(a) * r * 0.8f, 1.9f, 1.7f, i % 2 ? D : lit(H, 0.15f)); }
        break;
    case 28:   // curly frohawk (the curls are in front)
        ell(HX, HY - 2, 12.2f, 12.6f, sh(H, 0.7f));
        break;
    case 29:   // short two-strand twists
        ell(HX, HY - 4, 14, 13, H);
        for (int i = 0; i < 9; i++) { float x = HX - 12 + i * 3; limb(x, HY - 6, 1.6f, x + (i - 4) * 0.5f, HY - 18 - (i % 3), 1.4f, i % 2 ? D : H); }
        break;
    // ---- 18 more (Sugar Bunny Life, batch 2)
    case 30: case 31:   // goddess locs / knotless braids with curly ends
        ell(HX, HY - 2, 15, 15, H);
        for (int i = 0; i < 12; i++) { float x = HX - 14 + i * 2.6f; float y1 = 116 - std::fabs(i - 5.5f) * 2.f; limb(x, HY, 1.4f, x + (i - 5.5f) * 0.7f, y1, 1.2f, i % 2 ? D : H);
            if (L.hair_style == 30) { if (i % 3 == 0) ell(x + (i - 5.5f) * 0.35f, HY + 40 + (i % 4) * 6, 1.6f, 1.2f, rgb(226, 186, 96), false); ell(x + (i - 5.5f) * 0.7f, y1 + 2, 2.2f, 2.2f, i % 2 ? H : Hi); }
            else { for (int k = 0; k < 3; k++) dot((int)(x + (i - 5.5f) * 0.7f + (k - 1)), (int)(y1 + 2 + k), Hi); } }
        break;
    case 32:   // fulani braids: long side braids, beads
        ell(HX, HY - 2, 14.5f, 14.5f, H);
        for (int i = 0; i < 10; i++) { float x = HX - 13 + i * 2.9f; limb(x, HY + 2, 1.5f, x + (i - 4.5f) * 0.8f, 112, 1.3f, i % 2 ? D : H); ell(x + (i - 4.5f) * 0.6f, 96 + (i % 3) * 5, 1.8f, 1.5f, i % 2 ? rgb(232, 196, 110) : rgb(250, 250, 245), false); }
        break;
    case 33:   // half-up top knot, long waves underneath
        ell(HX, HY - 2, 15, 15, H); ell(HX, HY - 17, 5.5f, 4.5f, H);
        for (int i = 0; i < 8; i++) { float x = 33 + i * 4.3f; strand(x, HY + 2, 100 - std::fabs(i - 3.5f) * 3.f, 2.2f, 0.3f, i * 1.1f, 3.2f, i % 3 == 1 ? D : H); }
        break;
    case 34:   // low ponytail with a bandana
        ell(HX, HY - 1, 13, 13, H); strand(HX - 2, HY + 10, 82, 1.2f, 0.2f, 0.3f, 3.0f, H);
        break;
    case 35: ell(HX, HY - 3, 13, 12.5f, H); break;   // pixie cut
    case 36: ell(HX, HY - 2, 12.6f, 12.8f, sh(H, 0.8f)); break;   // bantu knots (knots in front)
    case 37:   // middle-part body-wave sew-in, very full
        ell(HX, HY - 1, 17, 16, H);
        poly({HX - 17, HY, HX + 17, HY, HX + 21, HY + 40, HX + 18, 112, HX - 18, 112, HX - 21, HY + 40}, H);
        for (int i = 0; i < 9; i++) { float x = HX - 17 + i * 4.2f; strand(x, HY + 4, 110, 2.6f, 0.26f, i * 1.4f, 1.0f, i % 2 ? D : Hi); }
        break;
    case 38:   // bubble ponytail
        ell(HX, HY - 1, 12.5f, 13, H); ell(HX + 8, HY - 12, 3, 3, H);
        for (int k = 0; k < 5; k++) { float y = HY - 4 + k * 13; ell(HX + 15 + (k & 1), y, 5.f - k * 0.3f, 6.f, H); line(HX + 11 + (k & 1), y + 6, HX + 19, y + 6, rgb(255, 110, 190)); line(HX + 12, y - 2, HX + 15, y - 4, Hi); }
        break;
    case 39:   // side-swept curls over one shoulder
        ell(HX, HY - 2, 15.5f, 15.5f, H);
        for (int i = 0; i < 26; i++) { float k = i / 25.f; ell(HX + 12 + std::sin(i * 1.7f) * 4, HY + 4 + k * 66, 4.2f, 3.8f, i % 3 ? H : D); }
        break;
    case 40:   // mermaid waves, extra long
        ell(HX, HY - 2, 16, 15.5f, H);
        for (int i = 0; i < 10; i++) { float x = 30 + i * 4.f; strand(x, HY, 124 - std::fabs(i - 4.5f) * 2.f, 3.4f, 0.22f, i * 0.9f, 3.8f, i % 3 == 1 ? D : H); }
        for (int i = 0; i < 5; i++) strand(33 + i * 7.5f, HY + 8, 116, 3.0f, 0.22f, i * 1.7f, 1.0f, Hi);
        break;
    case 41:   // half-up space buns, long hair down
        ell(HX, HY - 2, 14.5f, 14.5f, H);
        poly({HX - 14, HY - 2, HX + 14, HY - 2, HX + 15, 108, HX - 15, 108}, H);
        for (int i = 0; i < 7; i++) strand(HX - 12 + i * 4, HY + 4, 106, 0.6f, 0.3f, i * 1.f, 0.6f, i % 2 ? D : Hi);
        break;
    case 42: ell(HX, HY - 2, 12.8f, 13, H); ell(HX, HY - 15, 6.5f, 5.5f, H); line(HX - 5, HY - 17, HX + 5, HY - 17, Hi); break;   // sleek top bun + swoop
    case 43:   // crimped Y2K waves
        ell(HX, HY - 2, 15.5f, 15, H);
        poly({HX - 15, HY, HX + 15, HY, HX + 17, 104, HX - 17, 104}, H);
        for (int r = 0; r < 14; r++) for (int i = -15; i <= 15; i += 3) dot((int)(HX + i + ((r & 1) ? 1 : -1)), (int)(HY + 6 + r * 7), (i / 3) % 2 ? D : Hi);
        break;
    case 44: ell(HX, HY - 3, 13, 12.5f, sh(H, 0.75f)); break;   // taper fade + curly top
    case 45:   // locs in a high bun
        ell(HX, HY - 2, 13, 13, H); ell(HX, HY - 18, 8, 6.5f, H);
        for (int i = 0; i < 9; i++) { float a = i * 0.7f; limb(HX + std::cos(a) * 6, HY - 18 + std::sin(a) * 4, 1.3f, HX + std::cos(a) * 9, HY - 13 + std::sin(a) * 4, 1.2f, i % 2 ? D : Hi); }
        break;
    case 46: ell(HX, HY - 2, 12.6f, 12.8f, sh(H, 0.85f)); break;   // cornrows (rows in front)
    case 47:   // soft mullet / e-boy curtains
        ell(HX, HY - 3, 13.5f, 13, H); poly({HX - 12, HY, HX + 12, HY, HX + 11, HY + 20, HX - 11, HY + 20}, H);
        for (int i = 0; i < 5; i++) line(HX - 9 + i * 4.5f, HY + 8, HX - 10 + i * 4.5f, HY + 19, D);
        break;
    // ---- Sugar Bunny Life 2: downtown girl, curly school styles, stitch & fulani braids
    case 48:   // claw clip half-up: long waves, tortoise clip peeking over the crown
        wavesBack(H, D, Hi, 15.5f, 112);
        { Px T1 = rgb(120, 70, 40), T2 = rgb(200, 140, 70); poly({HX - 7, HY - 19, HX + 7, HY - 19, HX + 6, HY - 13, HX - 6, HY - 13}, T1); for (int i = -6; i <= 6; i += 3) line(HX + i, HY - 19, HX + i * 0.85f, HY - 13, T2); }
        break;
    case 49: wavesBack(H, D, Hi, 15.5f, 110); break;   // braided half-up: waves (braids in front)
    case 50:   // ribbon braid: hair gathered into one plait (plait is drawn in front)
        ell(HX, HY - 2, 13.5f, 14, H); poly({HX + 4, HY + 2, HX + 13, HY, HX + 13, HY + 14, HX + 6, HY + 14}, H);
        break;
    case 51:   // blowout curls: big bouncy volume
        ell(HX, HY - 3, 19, 18, H);
        for (int i = 0; i < 8; i++) { float x = HX - 18 + i * 5.1f; for (int k = 0; k < 4; k++) { float y = HY + 4 + k * 14 + (i % 2) * 5; if (y > 92) break; ell(x + (k % 2 ? 1.5f : -1.5f), y, 5.2f, 6.f, (i + k) % 3 ? H : D); line(x - 2, y - 3, x + 1, y - 5, Hi); } }
        break;
    case 52: ell(HX, HY - 2, 13, 13.5f, H); break;   // twin french braids (braids in front)
    case 53:   // messy low bun peeking out at the nape
        ell(HX, HY - 2, 13.5f, 13.5f, H);
        for (int sd = -1; sd <= 1; sd += 2) { ell(HX + sd * 11, HY + 14, 5.5f, 5.f, H); line(HX + sd * 9, HY + 12, HX + sd * 14, HY + 16, Hi); strand(HX + sd * 13, HY + 14, HY + 24, 1.f, 0.6f, sd * 1.f, 0.7f, D); }
        break;
    case 54:   // velvet bow waves: long waves, big black bow at the back
        wavesBack(H, D, Hi, 15.5f, 114);
        { Px B = rgb(26, 20, 26), Bh = rgb(70, 56, 70); for (int sd = -1; sd <= 1; sd += 2) { poly({HX, HY - 15, HX + sd * 14, HY - 23, HX + sd * 16, HY - 12, HX, HY - 11}, B); line(HX + sd * 3, HY - 15, HX + sd * 12, HY - 20, Bh); } ell(HX, HY - 14, 3, 3, B); }
        break;
    case 55: curlsBack(H, D, Hi, 16.5f, 100); { Px T1 = rgb(30, 26, 30); poly({HX - 6, HY - 20, HX + 6, HY - 20, HX + 5, HY - 14, HX - 5, HY - 14}, T1); } break;   // curly half-up + claw clip
    case 56:   // sleek stitch braids: bone-straight long hair
        ell(HX, HY - 2, 14.5f, 14.5f, H);
        poly({HX - 14.5f, HY - 2, HX + 14.5f, HY - 2, HX + 16, 122, HX - 16, 122}, H);
        for (int i = 0; i < 8; i++) line(HX - 13 + i * 3.7f, HY + 6, HX - 14 + i * 4.f, 120, i % 2 ? D : lit(H, 0.18f));
        break;
    case 57:   // fulani flower braids: long braids with curly boho ends
        ell(HX, HY - 2, 15, 15, H);
        for (int i = 0; i < 12; i++) { float x = HX - 14 + i * 2.6f; float y1 = 118 - std::fabs(i - 5.5f) * 2.f; limb(x, HY, 1.4f, x + (i - 5.5f) * 0.7f, y1, 1.2f, i % 2 ? D : H);
            for (int k = 0; k < 4; k++) ell(x + (i - 5.5f) * 0.5f + ((k + i) % 2 ? 2.2f : -2.2f), 70 + i % 3 * 9 + k * 11, 2.0f, 1.6f, k % 2 ? Hi : H, false); }
        break;
    case 58: curlsBack(H, D, Hi, 16.5f, 112); break;   // Mia: long spiral curls
    case 59: curlsBack(H, D, Hi, 20.f, 96); ell(HX, HY - 6, 19, 16, H); break;   // Eli: big round curly volume
    case 60: curlsBack(H, D, Hi, 18.f, 116); for (int i = 0; i < 10; i++) ell(HX + 16 + (i % 2) * 3, HY + 6 + i * 9, 3.6f, 3.4f, i % 2 ? D : Hi); break;   // Sunday: wild side-swept curls
    case 61:   // bouncy high pony: big curled ponytail swinging out behind the crown
        ell(HX, HY - 2, 13.5f, 14, H);
        for (int k = 0; k < 7; k++) { float x = HX + 10 + std::sin(k * 0.9f) * 5, y = HY - 10 + k * 9.f; ell(x, y, 7.5f - k * 0.5f, 6.f, k % 2 ? D : H); line(x - 3, y - 3, x + 2, y - 5, Hi); }
        break;
    case 62:   // wavy low pigtails: sleek top, two long wavy ponytails behind the shoulders
        ell(HX, HY - 2, 13.5f, 14, H);
        for (int sd = -1; sd <= 1; sd += 2) for (int k = 0; k < 9; k++) { float x = HX + sd * (16 + std::sin(k * 1.3f) * 2.5f), y = HY + 12 + k * 9.f; ell(x, y, 5.5f - k * 0.2f, 5.f, k % 2 ? D : H); if (k % 2 == 0) line(x - 2, y - 2, x + 1, y - 3, Hi); }
        break;
    case 63: case 64: case 65: ell(HX, HY - 4, 12.5f, 11.5f, H); if (L.hair_style == 65) ell(HX, HY - 10, 13.f, 8.f, H); break;   // short guy cuts
    case 66:   // glam blowout: huge glossy volume, big barrel curls at the ends
        wavesBack(H, D, Hi, 18.5f, 108); ell(HX, HY - 4, 18.5f, 16.5f, H);
        for (int i = 0; i < 9; i++) { float x = HX - 17 + i * 4.3f, y = 100 - std::fabs(i - 4) * 3.f; ell(x, y, 3.8f, 4.6f, i % 2 ? D : H); ell(x - 1, y - 1.5f, 1.6f, 2.f, Hi); }
        break;
    case 67: curlsBack(H, D, Hi, 19.f, 120); ell(HX, HY - 4, 17.5f, 16, H); break;   // swoop spiral curls: long tight ringlets
    case 68:   // curly pigtail bunches: two high curly bunches + a few loose curls
        ell(HX, HY - 2, 13.2f, 13.2f, H);
        for (int sd = -1; sd <= 1; sd += 2) for (int i = 0; i < 26; i++) { float k = (i * 7 % 26) / 26.f, x = HX + sd * (17 + std::sin(i * 2.1f) * 5 + k * 3), y = HY - 8 + k * 70; ell(x, y, 3.8f, 3.4f, i % 3 == 0 ? D : i % 3 == 1 ? H : Hi); }
        break;
    case 84: {   // Curly Cloud: big rounded shoulder-length volume made of springy curl clumps
        ell(HX, HY + 1, 20.5f, 19.5f, D); ell(HX, HY - 2, 19, 17.5f, H);
        for (int i = 0; i < 30; i++) { const float a = -0.35f + i * 0.13f; const float r = 19.5f + (i % 3) * 1.2f;
            const float x = HX + std::cos(a) * r, y = HY + 1 - std::sin(a) * (r * 0.95f);
            ell(x, y, 3.6f, 3.3f, i % 2 ? D : H); ell(x - 0.8f, y - 1.0f, 1.6f, 1.2f, Hi); }
        for (int s2 = -1; s2 <= 1; s2 += 2) for (int k = 0; k < 4; k++) { const float x = HX + s2 * (15.5f - k * 1.2f), y = HY + 12 + k * 3.6f; ell(x, y, 3.8f, 3.4f, k % 2 ? D : H); ell(x - 0.8f, y - 0.9f, 1.5f, 1.1f, Hi); }   // fuller at the shoulders
        break; }
    case 69: curlsBack(H, D, Hi, 22.f, 102); ell(HX, HY - 7, 21, 17, H); for (int i = 0; i < 24; i++) { float a = i * 0.27f; ell(HX + std::cos(a) * 21, HY - 7 - std::sin(a) * 16, 3.4f, 3.2f, i % 2 ? D : Hi); } break;   // big curly mane
    case 80:   // spiky space buns: two little buns with flyaway spikes
        ell(HX, HY - 2, 13.2f, 13.2f, H);
        for (int s2 = -1; s2 <= 1; s2 += 2) { ell(HX + s2 * 9, HY - 14, 5.f, 4.5f, H); ell(HX + s2 * 9, HY - 15, 2.5f, 2.f, Hi); for (int k = 0; k < 6; k++) { const float a = -2.6f + k * 0.45f; line(HX + s2 * 9, HY - 14, HX + s2 * 9 + std::cos(a) * 9 * s2, HY - 14 + std::sin(a) * 8, D); } }
        break;
    case 83:   // twin braids: centre part, a long braid down each side
        ell(HX, HY - 3, 13.6f, 13.6f, H);
        for (int s2 = -1; s2 <= 1; s2 += 2) for (int k = 0; k < 13; k++) { const float y = HY + 6 + k * 5.f, x = HX + s2 * 12.5f; ell(x - 1.4f, y, 2.4f, 2.7f, k % 2 ? H : D); ell(x + 1.4f, y + 2.5f, 2.4f, 2.7f, k % 2 ? D : Hi); }
        break;
    case 81:   // sleek side braid over one shoulder
        ell(HX, HY - 3, 13.6f, 13.6f, H);
        for (int k = 0; k < 14; k++) { const float y = HY + 8 + k * 5.f, x = HX + 12 + std::sin(k * 0.4f) * 1.5f; ell(x - 1.5f, y, 2.6f, 2.8f, k % 2 ? H : D); ell(x + 1.5f, y + 2.5f, 2.6f, 2.8f, k % 2 ? D : Hi); }
        break;
    case 82:   // top knot with a long side swoop
        ell(HX, HY - 3, 13.4f, 13.4f, H); ell(HX, HY - 18, 7.f, 6.5f, H); ell(HX - 2, HY - 20, 3.f, 2.f, Hi);
        for (int y = 0; y < 26; y += 2) ell(HX - 13 + std::sin(y * 0.2f), HY + y, 1.6f, 1.8f, y % 4 ? H : D);
        break;
    case 75:   // long locs: many thin ropes falling past the waist
        ell(HX, HY - 4, 16.f, 15.f, H);
        for (int k = 0; k < 18; k++) { const float x = HX - 17 + k * 2.f, len = 118 + (k * 37) % 14; for (float y = HY - 2; y < len; y += 2.f) ell(x + std::sin(y * 0.15f + k) * 1.2f, y, 1.2f, 1.3f, ((int)y / 2 + k) % 3 == 0 ? D : ((int)y / 2 + k) % 3 == 1 ? H : Hi); }
        break;
    case 76:   // messy curly bun: a big curly bun up top + loose tendrils
        ell(HX, HY - 2, 13.4f, 13.4f, H);
        for (int i = 0; i < 16; i++) { const float a = i * 0.39f; ell(HX + std::cos(a) * 8, HY - 17 + std::sin(a) * 6, 3.2f, 3.f, i % 2 ? D : Hi); }
        ell(HX, HY - 17, 7.f, 5.f, H);
        for (int s2 = -1; s2 <= 1; s2 += 2) for (int y = 0; y < 18; y += 2) ell(HX + s2 * (13 + std::sin(y * 0.5f) * 1.5f), HY + y, 1.1f, 1.3f, D);
        break;
    case 77: wavesBack(H, D, Hi, 15.f, 120); ell(HX, HY - 4, 16.f, 15.f, H); break;   // sleek blunt bangs: long & straight
    case 78: curlsBack(H, D, Hi, 18.f, 48); ell(HX, HY - 6, 17.5f, 15.f, H); break;    // curly bob
    case 79: wavesBack(H, D, Hi, 17.f, 112); ell(HX, HY - 4, 17.f, 16.f, H); for (int k = 0; k < 4; k++) ell(HX + 15, HY + 30 + k * 18, 4.f, 3.f, k % 2 ? Hi : H); break;   // side-part glam waves
    case 71: wavesBack(H, D, Hi, 14.5f, 124); ell(HX, HY - 4, 16.5f, 15.5f, H); break;   // sleek slick-back: long, straight, glossy
    case 72: curlsBack(H, D, Hi, 24.f, 126); ell(HX, HY - 8, 22.f, 17.f, H); for (int i = 0; i < 28; i++) { float a = i * 0.24f; ell(HX + std::cos(a) * 22, HY - 8 - std::sin(a) * 15, 3.6f, 3.4f, i % 2 ? D : Hi); } break;   // voluminous curls: huge & long
    case 73: case 74: { const bool fro = L.hair_style == 73; curlsBack(H, D, Hi, fro ? 23.f : 22.f, fro ? 62 : 92); ell(HX, HY - 9, fro ? 24.f : 22.f, fro ? 20.f : 18.f, H);
        for (int i = 0; i < 34; i++) { float a = i * 0.19f; ell(HX + std::cos(a) * (fro ? 23 : 21), HY - 9 - std::sin(a) * (fro ? 19 : 17), 3.4f, 3.2f, i % 3 == 0 ? D : i % 3 == 1 ? H : Hi); } break; }   // afro / curly fro
    case 70:   // layered flip waves: long layers, ends flipped out
        wavesBack(H, D, Hi, 16.5f, 108);
        for (int sd = -1; sd <= 1; sd += 2) for (int k = 0; k < 3; k++) ell(HX + sd * (17 + k * 2), HY + 34 + k * 22, 4.2f, 3.f, k % 2 ? Hi : H);
        break;
    default: ell(HX, HY - 3, 12.5f, 12, H); break;   // spiky
    }
}
void drawHairFront(const Look& L) {
    Px H = rgb(L.hair), D = sh(H, 0.78f), Hi = lit(H, 0.32f);
    g_part = 11;
    switch (L.hair_style) {
    case 84: {   // Curly Cloud hairline: soft middle part, little curls across the forehead and down the cheeks
        poly({HX - 13, HY - 4, HX - 8, HY - 14, HX, HY - 15, HX + 8, HY - 14, HX + 13, HY - 4, HX + 8, HY - 10, HX + 1, HY - 12, HX - 1, HY - 12, HX - 8, HY - 10}, H);
        line(HX, HY - 15, HX, HY - 12, D);
        for (int i = 0; i < 7; i++) { const float x = HX - 11 + i * 3.6f, y = HY - 11 + std::fabs(i - 3) * 0.9f; ell(x, y, 2.2f, 2.0f, i % 2 ? D : H); plot((int)x - 1, (int)y - 1, Hi); }
        for (int s2 = -1; s2 <= 1; s2 += 2) for (int k = 0; k < 4; k++) { const float x = HX + s2 * (12.5f + (k % 2) * 0.8f), y = HY - 5 + k * 4.2f; ell(x, y, 2.3f, 2.2f, k % 2 ? D : H); plot((int)x - 1, (int)y - 1, Hi); }
        break; }
    case 82: case 79: case 76: case 66: case 0:   // deep side part with a swoop over the forehead + curtain pieces
        poly({HX - 12, HY - 6, HX - 4, HY - 16, HX + 10, HY - 15, HX + 13, HY - 6, HX + 6, HY - 10, HX - 3, HY - 9, HX - 9, HY - 2}, H);
        strand(HX - 13, HY - 6, HY + 26, 1.6f, 0.5f, 0.3f, 2.2f, H); strand(HX + 14, HY - 6, HY + 28, 1.8f, 0.5f, 1.6f, 2.2f, H);
        line(HX - 6, HY - 13, HX + 6, HY - 13, Hi); line(HX - 8, HY - 10, HX - 3, HY - 12, Hi);
        break;
    case 1:   // slicked back with loose wisps
        poly({HX - 12, HY - 5, HX - 7, HY - 14, HX + 7, HY - 14, HX + 12, HY - 5, HX + 6, HY - 10, HX - 6, HY - 10}, H);
        strand(HX - 12, HY - 4, HY + 14, 1.0f, 0.7f, 0.f, 0.8f, H); strand(HX + 12, HY - 4, HY + 12, 1.0f, 0.7f, 1.f, 0.8f, H);
        line(HX - 5, HY - 12, HX + 4, HY - 12, Hi);
        break;
    case 2:   // middle part, braids framing the face
        poly({HX - 13, HY - 4, HX - 6, HY - 15, HX, HY - 13, HX + 6, HY - 15, HX + 13, HY - 4, HX + 7, HY - 10, HX, HY - 11, HX - 7, HY - 10}, H);
        for (int s = -1; s <= 1; s += 2) for (int i = 0; i < 2; i++) limb(HX + s * (11 + i * 2.5f), HY - 4, 1.6f, HX + s * (12 + i * 2.5f), HY + 40, 1.4f, i ? D : H);
        dot((int)HX + 12, (int)HY + 30, rgb(224, 184, 104)); dot((int)HX - 12, (int)HY + 34, rgb(224, 184, 104));
        break;
    case 3: poly({HX - 12, HY - 5, HX - 6, HY - 14, HX + 8, HY - 14, HX + 12, HY - 5, HX + 5, HY - 11, HX - 6, HY - 10}, H); line(HX - 5, HY - 12, HX + 6, HY - 13, Hi); break;
    case 4:   // blunt bangs
        poly({HX - 14, HY - 2, HX - 10, HY - 14, HX + 10, HY - 14, HX + 14, HY - 2, HX + 11, HY - 6, HX - 11, HY - 6}, H);
        for (int x = -10; x <= 10; x += 3) line(HX + x, HY - 13, HX + x, HY - 7, D);
        line(HX - 8, HY - 12, HX + 2, HY - 12, Hi);
        break;
    case 5: poly({HX - 12, HY - 6, HX - 6, HY - 13, HX + 6, HY - 13, HX + 12, HY - 6, HX + 6, HY - 9, HX - 6, HY - 9}, H); break;
    case 6:   // short waves / fade
        poly({HX - 12, HY - 4, HX - 8, HY - 14, HX + 8, HY - 14, HX + 12, HY - 4, HX + 9, HY - 8, HX - 9, HY - 8}, H);
        for (int r = 0; r < 3; r++) for (int x = -8; x <= 8; x += 4) dot((int)(HX + x + r), (int)(HY - 12 + r * 2), D);
        break;
    case 75: case 71: case 70: case 8:   // middle part, sleek curtains framing the face
        poly({HX - 13, HY - 2, HX - 7, HY - 14, HX, HY - 13, HX + 7, HY - 14, HX + 13, HY - 2, HX + 8, HY - 9, HX, HY - 11, HX - 8, HY - 9}, H);
        strand(HX - 12.5f, HY - 4, HY + 40, 0.3f, 0.3f, 0.f, 2.f, H); strand(HX + 12.5f, HY - 4, HY + 40, 0.3f, 0.3f, 1.f, 2.f, H);
        line(HX - 6, HY - 12, HX - 2, HY - 12, Hi); line(HX + 2, HY - 12, HX + 6, HY - 12, Hi);
        break;
    case 9:   // blunt bangs + curtains
        poly({HX - 13, HY - 3, HX - 10, HY - 14, HX + 10, HY - 14, HX + 13, HY - 3, HX + 11, HY - 6, HX - 11, HY - 6}, H);
        for (int x = -10; x <= 10; x += 3) line(HX + x, HY - 13, HX + x + 0.5f, HY - 7, D);
        strand(HX - 12.5f, HY - 4, HY + 40, 0.3f, 0.3f, 0.f, 2.f, H); strand(HX + 12.5f, HY - 4, HY + 40, 0.3f, 0.3f, 1.f, 2.f, H);
        line(HX - 8, HY - 12, HX + 4, HY - 12, Hi);
        break;
    case 10: poly({HX - 12, HY - 6, HX - 6, HY - 13, HX + 6, HY - 13, HX + 12, HY - 6, HX + 6, HY - 9, HX - 6, HY - 9}, H); break;
    case 11:   // shaggy layered fringe
        poly({HX - 13, HY - 2, HX - 9, HY - 14, HX + 9, HY - 14, HX + 13, HY - 2, HX + 9, HY - 5, HX + 4, HY - 8, HX - 1, HY - 5, HX - 6, HY - 8, HX - 10, HY - 4}, H);
        for (int i = 0; i < 5; i++) line(HX - 8 + i * 4, HY - 12, HX - 9 + i * 4, HY - 6, D);
        line(HX - 6, HY - 12, HX + 2, HY - 12, Hi);
        break;
    case 12: poly({HX - 12, HY - 5, HX - 7, HY - 13, HX + 7, HY - 13, HX + 12, HY - 5, HX + 6, HY - 9, HX - 6, HY - 9}, H); for (int i = 0; i < 6; i++) ell(HX - 8 + i * 3.2f, HY - 9 + (i & 1), 1.8f, 1.6f, i % 2 ? D : lit(H, 0.15f)); break;
    case 13:  // middle part + two long braids over the shoulders
        poly({HX - 13, HY - 2, HX - 7, HY - 14, HX, HY - 13, HX + 7, HY - 14, HX + 13, HY - 2, HX + 8, HY - 9, HX, HY - 11, HX - 8, HY - 9}, H);
        strand(HX - 12.5f, HY - 4, HY + 14, 0.3f, 0.3f, 0.f, 1.6f, H); strand(HX + 12.5f, HY - 4, HY + 14, 0.3f, 0.3f, 1.f, 1.6f, H);
        for (int s = -1; s <= 1; s += 2) for (int k = 0; k < 18; k++) {
            float y = HY + 12 + k * 4.6f, x = HX + s * (12.5f + std::min(k, 4) * 0.4f) + ((k & 1) ? 0.9f : -0.9f);
            ell(x, y, 3.2f - k * 0.05f, 2.8f, (k & 1) ? D : H);
            line(x - 2, y - 1, x + 1, y + 1, Hi);
        }
        line(HX - 6, HY - 12, HX - 2, HY - 12, Hi); line(HX + 2, HY - 12, HX + 6, HY - 12, Hi);
        break;
    case 15: {   // two spiky space buns with baby hairs
        poly({HX - 12, HY - 4, HX - 7, HY - 13, HX, HY - 12, HX + 7, HY - 13, HX + 12, HY - 4, HX + 8, HY - 9, HX, HY - 10, HX - 8, HY - 9}, H);
        for (int sd = -1; sd <= 1; sd += 2) {
            float bx = HX + sd * 9.5f, by = HY - 14;
            for (int k = 0; k < 9; k++) { float a = -3.0f + k * 0.42f; line(bx, by, bx + std::cos(a) * 9 * sd, by + std::sin(a) * 9, D); }
            ell(bx, by, 5.5f, 5, H); ell(bx - 1, by - 1.5f, 2.5f, 2, lit(H, 0.25f));
        }
        line(HX - 6, HY - 11, HX - 2, HY - 11, Hi); line(HX + 2, HY - 11, HX + 6, HY - 11, Hi);
        break;
    }
    case 78: case 72: case 74: case 69: case 16:   // thick curly bangs
        for (int i = 0; i < 9; i++) ell(HX - 11 + i * 2.75f, HY - 9 + (i % 2) * 1.5f, 3.2f, 3.4f, i % 2 ? D : H);
        for (int i = 0; i < 6; i++) ell(HX - 8 + i * 3.2f, HY - 6, 2.2f, 2.4f, i % 2 ? H : lit(H, 0.15f));
        break;
    case 17:   // sleek centre part + one long braid over the shoulder
        poly({HX - 12.5f, HY - 3, HX - 8, HY - 13, HX, HY - 13.5f, HX + 8, HY - 13, HX + 12.5f, HY - 3, HX + 9, HY - 10, HX, HY - 12, HX - 9, HY - 10}, H);
        line(HX, HY - 13, HX, HY - 9, sh(H, 0.6f)); line(HX - 5, HY - 11, HX - 2, HY - 12, Hi); line(HX + 2, HY - 12, HX + 5, HY - 11, Hi);
        for (int k = 0; k < 16; k++) { float y = HY + 8 + k * 4.6f, x = HX - 11.5f - std::min(k, 5) * 0.5f + ((k & 1) ? 0.8f : -0.8f); ell(x, y, 2.8f - k * 0.06f, 2.6f, (k & 1) ? D : H); line(x - 1.5f, y - 1, x + 1, y + 1, Hi); }
        break;
    case 14: {   // buzz cut: a close crop that hugs the top of the head
        for (int y = (int)((HY - 14) * g_ss); y <= (int)((HY + 1) * g_ss); y++)
            for (int x = (int)((HX - 13) * g_ss); x <= (int)((HX + 13) * g_ss); x++) {
                float fx = (x + 0.5f) / g_ss, fy = (y + 0.5f) / g_ss;
                float dx = (fx - HX) / 12.2f, dy = (fy - (HY - 1)) / 12.6f;
                float edge = HY - 7 + (std::fabs(fx - HX) > 9 ? 5.f : 0.f);
                if (dx * dx + dy * dy <= 1.f && fy < edge) plot(x, y, vol(H, dx * 0.8f, dy * 0.3f));
            }
        for (int i = 0; i < 7; i++) dot((int)HX - 9 + i * 3, (int)HY - 10 + (i & 1), Hi);
        break;
    }
    // ---- new styles (Sugar Bunny Life)
    case 18: {   // swoopy side bangs + butterfly clips
        poly({HX - 13, HY - 4, HX - 7, HY - 15, HX + 9, HY - 15, HX + 13, HY - 4, HX + 7, HY - 9, HX - 1, HY - 10, HX - 8, HY - 5}, H);
        line(HX - 5, HY - 13, HX + 5, HY - 13, Hi);
        Px c1 = rgb(255, 120, 200), c2 = rgb(120, 200, 255);
        for (int k = 0; k < 2; k++) { float bx = HX + 6 + k * 5, by = HY - 11 + k * 3; Px c = k ? c2 : c1; ell(bx - 1.4f, by, 1.6f, 1.3f, c, false); ell(bx + 1.4f, by, 1.6f, 1.3f, c, false); dot((int)bx, (int)by, rgb(70, 30, 80)); }
        break;
    }
    case 19:   // see-through wispy bangs + curtains
        poly({HX - 13, HY - 3, HX - 8, HY - 15, HX + 8, HY - 15, HX + 13, HY - 3, HX + 10, HY - 10, HX - 10, HY - 10}, H);
        for (int x = -9; x <= 9; x += 3) line(HX + x, HY - 11, HX + x + 0.6f, HY - 5 - (x & 1), H);
        strand(HX - 12.5f, HY - 4, HY + 40, 0.3f, 0.3f, 0.f, 2.f, H); strand(HX + 12.5f, HY - 4, HY + 40, 0.3f, 0.3f, 1.f, 2.f, H);
        line(HX - 6, HY - 13, HX + 4, HY - 13, Hi);
        break;
    case 20: {   // pulled back, face-framing tendrils, tortoiseshell claw clip
        poly({HX - 12, HY - 5, HX - 7, HY - 14, HX + 7, HY - 14, HX + 12, HY - 5, HX + 6, HY - 10, HX - 6, HY - 10}, H);
        strand(HX - 12, HY - 4, HY + 16, 1.2f, 0.7f, 0.f, 0.9f, H); strand(HX + 12, HY - 4, HY + 14, 1.2f, 0.7f, 1.f, 0.9f, H);
        Px tc = rgb(150, 90, 50), td = rgb(90, 50, 30);
        for (int i = 0; i < 5; i++) limb(HX - 2 + i * 2.2f, HY - 19, 0.9f, HX - 1 + i * 2.2f, HY - 13, 0.8f, i % 2 ? td : tc, false);
        line(HX - 3, HY - 19, HX + 8, HY - 19, td);
        break;
    }
    case 77: {   // blunt straight-across bangs
        poly({HX - 14, HY - 12, HX + 14, HY - 12, HX + 14, HY - 3, HX - 14, HY - 3}, H); line(HX - 14, HY - 3, HX + 14, HY - 3, D);
        for (int x = -12; x <= 12; x += 4) line(HX + x, HY - 11, HX + x, HY - 4, Hi);
        break; }
    case 73: {   // afro & headband: a soft hairline under a wide headband
        const Px HB = rgb(L.topC), HBd = sh(HB, 0.8f);
        poly({HX - 14, HY - 12, HX + 14, HY - 12, HX + 15, HY - 8, HX - 15, HY - 8}, HB); line(HX - 14, HY - 8, HX + 14, HY - 8, HBd);
        for (int k = 0; k < 4; k++) { ell(HX - 12 + k * 1.2f, HY - 6 + k * 3, 1.6f, 1.8f, H); ell(HX + 12 - k * 1.2f, HY - 6 + k * 3, 1.6f, 1.8f, H); }
        break; }
    case 67:   // big glossy side swoop over one eye + ringlets framing the face
        poly({HX - 13, HY - 4, HX - 6, HY - 16, HX + 8, HY - 16, HX + 14, HY - 4, HX + 12, HY + 6, HX + 6, HY - 2, HX - 2, HY - 9, HX - 9, HY - 4}, H);
        line(HX - 4, HY - 13, HX + 8, HY - 10, Hi); line(HX + 2, HY - 14, HX + 11, HY - 6, Hi);
        for (int k = 0; k < 4; k++) { ell(HX - 14, HY + k * 7, 2.6f, 2.8f, k % 2 ? D : H); ell(HX + 15, HY + 4 + k * 7, 2.6f, 2.8f, k % 2 ? D : H); }
        break;
    case 83: case 80: case 81: case 68: case 21: case 25:   // middle part (puffs / twin tails), little bangs for twin tails
        poly({HX - 13, HY - 4, HX - 6, HY - 15, HX, HY - 13, HX + 6, HY - 15, HX + 13, HY - 4, HX + 7, HY - 10, HX, HY - 11, HX - 7, HY - 10}, H);
        if (L.hair_style == 25) { poly({HX - 10, HY - 13, HX + 10, HY - 13, HX + 9, HY - 7, HX - 9, HY - 7}, H); for (int x = -8; x <= 8; x += 4) line(HX + x, HY - 12, HX + x, HY - 8, D); }
        for (int sd = -1; sd <= 1; sd += 2) { Px bc = rgb(255, 90, 170); float bx = HX + sd * (L.hair_style == 25 ? 12.f : 15.f), by = L.hair_style == 25 ? HY - 13.f : HY + 2.f; ell(bx - 2, by, 2.2f, 1.7f, bc, false); ell(bx + 2, by, 2.2f, 1.7f, bc, false); dot((int)bx, (int)by, rgb(200, 40, 120)); }
        line(HX - 6, HY - 12, HX - 2, HY - 12, Hi); line(HX + 2, HY - 12, HX + 6, HY - 12, Hi);
        break;
    case 22: case 27:   // sleek with laid edges
        poly({HX - 12, HY - 5, HX - 6, HY - 14, HX + 8, HY - 14, HX + 12, HY - 5, HX + 5, HY - 11, HX - 6, HY - 10}, H);
        for (int sd = -1; sd <= 1; sd += 2) for (int k = 0; k < 3; k++) dot((int)(HX + sd * (9 + k)), (int)(HY - 6 + k), D);
        line(HX - 5, HY - 12, HX + 6, HY - 13, Hi);
        break;
    case 23:   // long curtain bangs
        poly({HX - 13, HY - 2, HX - 7, HY - 14, HX, HY - 13, HX + 7, HY - 14, HX + 13, HY - 2, HX + 8, HY - 8, HX, HY - 11, HX - 8, HY - 8}, H);
        strand(HX - 13, HY - 4, HY + 30, 1.2f, 0.3f, 0.f, 2.2f, H); strand(HX + 13, HY - 4, HY + 30, 1.2f, 0.3f, 1.f, 2.2f, H);
        line(HX - 6, HY - 12, HX - 2, HY - 12, Hi); line(HX + 2, HY - 12, HX + 6, HY - 12, Hi);
        break;
    case 24:   // choppy wolf-cut fringe
        poly({HX - 14, HY - 1, HX - 9, HY - 15, HX + 9, HY - 15, HX + 14, HY - 1, HX + 10, HY - 6, HX + 6, HY - 3, HX + 3, HY - 8, HX - 1, HY - 4, HX - 5, HY - 8, HX - 9, HY - 3, HX - 11, HY - 6}, H);
        for (int i = 0; i < 6; i++) line(HX - 9 + i * 3.6f, HY - 13, HX - 10 + i * 3.6f, HY - 6, D);
        line(HX - 6, HY - 13, HX + 3, HY - 13, Hi);
        break;
    case 26:   // hime cut: blunt bangs + sidelocks
        poly({HX - 14, HY - 2, HX - 10, HY - 14, HX + 10, HY - 14, HX + 14, HY - 2, HX + 11, HY - 6, HX - 11, HY - 6}, H);
        for (int x = -10; x <= 10; x += 3) line(HX + x, HY - 13, HX + x, HY - 7, D);
        poly({HX - 15, HY - 6, HX - 10.5f, HY - 6, HX - 10.5f, HY + 14, HX - 15, HY + 14}, H); poly({HX + 10.5f, HY - 6, HX + 15, HY - 6, HX + 15, HY + 14, HX + 10.5f, HY + 14}, H);
        line(HX - 8, HY - 12, HX + 2, HY - 12, Hi);
        break;
    case 28:   // curly frohawk on top, short sides
        for (int i = 0; i < 34; i++) { float k = i / 33.f; float y = HY - 23 + k * 15, x = HX + std::sin(i * 2.1f) * (4.5f + 2.f * std::sin(k * 3.1f)); ell(x, y, 3.9f, 3.5f, i % 3 ? D : lit(H, 0.2f)); }
        for (int i = 0; i < 7; i++) dot((int)HX - 9 + i * 3, (int)HY - 9 + (i & 1), sh(H, 0.85f));
        break;
    case 29:   // twisted fringe
        for (int i = 0; i < 8; i++) ell(HX - 10 + i * 2.9f, HY - 11 + (i % 2) * 1.5f, 1.8f, 3.2f, i % 2 ? D : H);
        break;
    // ---- 18 more (Sugar Bunny Life, batch 2)
    case 30: case 31: case 32: case 37: case 40: case 43:   // middle part + face-framing pieces
        poly({HX - 13, HY - 3, HX - 7, HY - 15, HX, HY - 13, HX + 7, HY - 15, HX + 13, HY - 3, HX + 8, HY - 10, HX, HY - 11, HX - 8, HY - 10}, H);
        if (L.hair_style == 32) { for (int k = 0; k < 6; k++) ell(HX + ((k & 1) ? 0.6f : -0.6f), HY - 14 + k * 1.2f, 1.6f, 1.2f, (k & 1) ? D : H); ell(HX, HY - 6, 1.5f, 1.2f, rgb(232, 196, 110), false); }
        strand(HX - 13, HY - 4, HY + (L.hair_style == 43 ? 24 : 34), 1.f, 0.3f, 0.f, 2.f, H); strand(HX + 13, HY - 4, HY + (L.hair_style == 43 ? 24 : 34), 1.f, 0.3f, 1.f, 2.f, H);
        line(HX - 6, HY - 12, HX - 2, HY - 12, Hi); line(HX + 2, HY - 12, HX + 6, HY - 12, Hi);
        break;
    case 33:   // half-up: soft side part
        poly({HX - 13, HY - 4, HX - 6, HY - 15, HX + 9, HY - 15, HX + 13, HY - 4, HX + 6, HY - 10, HX - 6, HY - 9}, H);
        strand(HX - 13, HY - 4, HY + 22, 1.4f, 0.5f, 0.f, 1.8f, H); line(HX - 4, HY - 13, HX + 6, HY - 13, Hi);
        break;
    case 34: {   // centre part + bandana
        poly({HX - 12, HY - 4, HX - 6, HY - 13, HX + 6, HY - 13, HX + 12, HY - 4, HX + 7, HY - 9, HX - 7, HY - 9}, H);
        Px B = L.accC.r | L.accC.g | L.accC.b ? rgb(L.accC) : rgb(240, 60, 130), Bd = sh(B, 0.8f);
        poly({HX - 14, HY - 9, HX - 8, HY - 15, HX + 8, HY - 15, HX + 14, HY - 9, HX + 12, HY - 6, HX - 12, HY - 6}, B);
        for (int i = -10; i <= 10; i += 4) dot((int)(HX + i), (int)(HY - 11), rgb(255, 255, 255));
        line(HX - 12, HY - 7, HX + 12, HY - 7, Bd);
        break;
    }
    case 35:   // pixie: soft side-swept fringe, tapered sides
        poly({HX - 13, HY - 1, HX - 9, HY - 15, HX + 10, HY - 15, HX + 13, HY - 4, HX + 6, HY - 8, HX - 3, HY - 6, HX - 10, HY - 2}, H);
        for (int i = 0; i < 4; i++) line(HX - 6 + i * 4, HY - 13, HX - 9 + i * 4, HY - 7, D);
        line(HX - 5, HY - 13, HX + 6, HY - 13, Hi);
        break;
    case 36:   // bantu knots
        for (int i = 0; i < 7; i++) { float a = 3.4f + i * 0.45f; float bx = HX + std::cos(a) * 12, by = HY - 3 + std::sin(a) * 12; ell(bx, by, 3.8f, 3.6f, H); line(bx - 2, by - 1, bx + 1, by - 3, Hi); line(bx - 1, by + 1, bx + 2, by, D); }
        break;
    case 38: case 42:   // sleek pulled back, laid swoop
        poly({HX - 12, HY - 5, HX - 6, HY - 14, HX + 8, HY - 14, HX + 12, HY - 5, HX + 5, HY - 11, HX - 6, HY - 10}, H);
        if (L.hair_style == 42) for (int k = 0; k < 5; k++) { float a = k * 0.9f; line(HX - 12 + k * 1.2f, HY - 5 - std::sin(a) * 2, HX - 9 + k * 1.2f, HY - 6 - std::cos(a), D); }
        line(HX - 5, HY - 12, HX + 6, HY - 13, Hi);
        break;
    case 39:   // deep side part sweeping one way
        poly({HX - 13, HY - 5, HX - 9, HY - 14, HX + 6, HY - 15, HX + 14, HY - 2, HX + 9, HY - 7, HX + 1, HY - 10, HX - 7, HY - 9}, H);
        line(HX - 4, HY - 13, HX + 7, HY - 12, Hi);
        break;
    case 41:   // two little space buns on top + curtain bangs
        poly({HX - 13, HY - 3, HX - 7, HY - 14, HX, HY - 12, HX + 7, HY - 14, HX + 13, HY - 3, HX + 8, HY - 8, HX, HY - 11, HX - 8, HY - 8}, H);
        for (int sd = -1; sd <= 1; sd += 2) { ell(HX + sd * 9, HY - 15, 5, 4.6f, H); ell(HX + sd * 9 - 1, HY - 16.5f, 2.2f, 1.8f, Hi); }
        break;
    case 44:   // curly top on a fade
        for (int i = 0; i < 16; i++) { float a = i * 2.39996f, r = std::sqrt((float)i / 16.f) * 9.f; ell(HX + std::cos(a) * r, HY - 13 + std::sin(a) * r * 0.5f, 2.6f, 2.4f, i % 2 ? D : lit(H, 0.18f)); }
        for (int sd = -1; sd <= 1; sd += 2) for (int y = 0; y < 3; y++) dot((int)(HX + sd * 11), (int)(HY - 6 + y * 2), sh(H, 0.7f));
        break;
    case 45:   // locs: a few hanging at the front
        poly({HX - 12, HY - 5, HX - 6, HY - 13, HX + 6, HY - 13, HX + 12, HY - 5, HX + 6, HY - 10, HX - 6, HY - 10}, H);
        for (int i = 0; i < 3; i++) limb(HX + 4 + i * 3, HY - 12, 1.2f, HX + 6 + i * 3.5f, HY - 2 + i * 2, 1.1f, i % 2 ? D : H);
        break;
    case 46:   // cornrows: straight-back rows
        for (int r = -4; r <= 4; r++) for (int k = 0; k < 6; k++) { float x = HX + r * 2.6f, y = HY - 15 + k * 2.4f; if ((x - HX) * (x - HX) / 150.f + (y - HY + 2) * (y - HY + 2) / 170.f < 1.f) ell(x, y, 1.2f, 1.3f, (k & 1) ? D : lit(H, 0.12f)); }
        break;
    case 47:   // e-boy curtains
        poly({HX - 13, HY - 2, HX - 7, HY - 15, HX, HY - 12, HX + 7, HY - 15, HX + 13, HY - 2, HX + 9, HY - 6, HX + 3, HY - 9, HX, HY - 6, HX - 3, HY - 9, HX - 9, HY - 6}, H);
        line(HX - 6, HY - 12, HX - 2, HY - 11, Hi); line(HX + 2, HY - 11, HX + 6, HY - 12, Hi);
        break;
    case 48: case 54:   // soft middle part / side part with face-framing waves
        if (L.hair_style == 48) poly({HX - 13, HY - 3, HX - 7, HY - 15, HX, HY - 13, HX + 7, HY - 15, HX + 13, HY - 3, HX + 8, HY - 10, HX, HY - 11, HX - 8, HY - 10}, H);
        else poly({HX - 13, HY - 5, HX - 9, HY - 14, HX + 6, HY - 15, HX + 14, HY - 2, HX + 9, HY - 7, HX + 1, HY - 10, HX - 7, HY - 9}, H);
        strand(HX - 13, HY - 4, HY + 36, 1.8f, 0.3f, 0.f, 2.2f, H); strand(HX + 13, HY - 4, HY + 36, 1.8f, 0.3f, 1.f, 2.2f, H);
        line(HX - 6, HY - 12, HX - 2, HY - 12, Hi); line(HX + 2, HY - 12, HX + 6, HY - 12, Hi);
        break;
    case 49:   // middle part + two little braids pulled back from the temples
        poly({HX - 13, HY - 3, HX - 7, HY - 15, HX, HY - 13, HX + 7, HY - 15, HX + 13, HY - 3, HX + 8, HY - 10, HX, HY - 11, HX - 8, HY - 10}, H);
        plait(HX - 12, HY - 5, HX - 3, HY - 15, 3.0f, H, D, Hi); plait(HX + 12, HY - 5, HX + 3, HY - 15, 3.0f, H, D, Hi);
        strand(HX - 14, HY - 2, HY + 30, 1.6f, 0.3f, 0.f, 1.8f, H); strand(HX + 14, HY - 2, HY + 30, 1.6f, 0.3f, 1.f, 1.8f, H);
        break;
    case 50: {   // side part + chunky plait over the shoulder, ribbon bow at the end
        poly({HX - 13, HY - 5, HX - 9, HY - 14, HX + 6, HY - 15, HX + 14, HY - 2, HX + 9, HY - 7, HX + 1, HY - 10, HX - 7, HY - 9}, H);
        plait(HX + 11, HY + 4, HX + 14, 96, 4.4f, H, D, Hi);
        Px R = L.accC.r | L.accC.g | L.accC.b ? rgb(L.accC) : rgb(40, 50, 100), Rd = sh(R, 0.7f);
        poly({HX + 14, 98, HX + 7, 94, HX + 8, 102}, R); poly({HX + 14, 98, HX + 21, 94, HX + 20, 102}, R); ell(HX + 14, 98, 1.8f, 1.8f, Rd, false);
        line(HX + 13, 99, HX + 10, 110, R); line(HX + 15, 99, HX + 18, 110, R);
        strand(HX + 7, 98, 104, 0.8f, 0.6f, 0.f, 1.2f, H);
        break;
    }
    case 51:   // blowout: swoopy side part with volume
        poly({HX - 15, HY - 2, HX - 11, HY - 16, HX + 8, HY - 17, HX + 16, HY - 2, HX + 10, HY - 8, HX + 1, HY - 11, HX - 8, HY - 9}, H);
        for (int sd = -1; sd <= 1; sd += 2) for (int k = 0; k < 3; k++) { ell(HX + sd * 15, HY + 2 + k * 10, 4.5f, 5.f, k % 2 ? D : H); line(HX + sd * 14 - 1, HY - 1 + k * 10, HX + sd * 16, HY - 3 + k * 10, Hi); }
        line(HX - 6, HY - 14, HX + 4, HY - 15, Hi);
        break;
    case 52:   // twin french braids: centre part, a braid down each side over the shoulders
        poly({HX - 12, HY - 4, HX - 6, HY - 14, HX, HY - 12, HX + 6, HY - 14, HX + 12, HY - 4, HX + 6, HY - 10, HX, HY - 11, HX - 6, HY - 10}, H);
        line(HX, HY - 15, HX, HY - 11, D);
        plait(HX - 9, HY - 12, HX - 14, HY + 2, 3.2f, H, D, Hi); plait(HX + 9, HY - 12, HX + 14, HY + 2, 3.2f, H, D, Hi);
        plait(HX - 14, HY + 2, HX - 13, 88, 3.6f, H, D, Hi); plait(HX + 14, HY + 2, HX + 13, 88, 3.6f, H, D, Hi);
        for (int sd = -1; sd <= 1; sd += 2) { ell(HX + sd * 13, 90, 2.f, 1.4f, rgb(250, 240, 230), false); strand(HX + sd * 13, 91, 98, 1.2f, 0.7f, 0.f, 1.6f, H); }
        break;
    case 53:   // messy bun front: centre part + loose curtain wisps
        poly({HX - 12, HY - 4, HX - 6, HY - 14, HX, HY - 12, HX + 6, HY - 14, HX + 12, HY - 4, HX + 6, HY - 10, HX, HY - 11, HX - 6, HY - 10}, H);
        strand(HX - 12, HY - 6, HY + 18, 1.6f, 0.6f, 0.f, 1.f, H); strand(HX + 12, HY - 6, HY + 16, 1.6f, 0.6f, 2.f, 1.f, H);
        strand(HX - 10, HY - 8, HY + 4, 1.2f, 0.9f, 1.f, 0.6f, D);
        break;
    case 55:   // curly half-up: curly fringe pieces + springy sides
        for (int i = 0; i < 9; i++) ell(HX - 11 + i * 2.75f, HY - 12 + (i % 2) * 2, 2.6f, 2.6f, i % 3 ? H : D);
        for (int sd = -1; sd <= 1; sd += 2) for (int k = 0; k < 6; k++) ell(HX + sd * (14 + (k % 2)), HY - 4 + k * 6, 2.6f, 2.8f, k % 2 ? H : Hi);
        break;
    case 56:   // stitch braids: two neat braids from the hairline back, sleek side part + laid edges
        poly({HX - 12, HY - 5, HX - 7, HY - 14, HX + 7, HY - 14, HX + 12, HY - 5, HX + 6, HY - 10, HX - 6, HY - 10}, H);
        plait(HX - 12, HY - 6, HX - 2, HY - 15, 2.6f, D, sh(H, 0.6f), Hi); plait(HX - 11, HY - 2, HX + 4, HY - 15, 2.6f, D, sh(H, 0.6f), Hi);
        strand(HX + 13, HY - 6, HY + 40, 0.4f, 0.3f, 0.f, 2.2f, H); line(HX + 6, HY - 13, HX + 11, HY - 9, Hi);
        for (int k = 0; k < 4; k++) { float a = k * 1.1f; dot((int)(HX - 12 + std::cos(a) * 1.5f), (int)(HY - 4 + std::sin(a) * 1.5f), D); }
        break;
    case 57: {   // fulani: zig-zag cornrows on top, hibiscus behind the ear
        for (int r = -4; r <= 4; r++) for (int k = 0; k < 6; k++) { float x = HX + r * 2.6f + ((k / 2) % 2 ? 1.2f : -1.2f) * (r < 0 ? 1 : -1), y = HY - 15 + k * 2.4f; if ((x - HX) * (x - HX) / 150.f + (y - HY + 2) * (y - HY + 2) / 170.f < 1.f) ell(x, y, 1.2f, 1.3f, (k & 1) ? D : lit(H, 0.12f)); }
        for (int sd = -1; sd <= 1; sd += 2) strand(HX + sd * 13, HY - 4, HY + 40, 1.4f, 0.5f, sd * 1.f, 1.2f, H);
        hibiscus(HX + 13, HY + 2, 5, rgb(252, 244, 246)); ell(HX + 13, HY + 2, 1.4f, 1.4f, rgb(255, 150, 190), false);
        break;
    }
    case 58:   // Mia: middle part, curly face-framing pieces
        poly({HX - 13, HY - 3, HX - 7, HY - 15, HX, HY - 13, HX + 7, HY - 15, HX + 13, HY - 3, HX + 8, HY - 10, HX, HY - 11, HX - 8, HY - 10}, H);
        for (int sd = -1; sd <= 1; sd += 2) for (int k = 0; k < 6; k++) ell(HX + sd * (13 + (k % 2)), HY - 4 + k * 6, 2.5f, 2.7f, k % 2 ? H : Hi);
        break;
    case 59:   // Eli: curly bangs all the way across
        for (int i = 0; i < 11; i++) ell(HX - 13 + i * 2.6f, HY - 12 + (i % 2) * 2.5f, 2.8f, 2.8f, i % 3 == 0 ? D : i % 3 == 1 ? H : Hi);
        for (int sd = -1; sd <= 1; sd += 2) for (int k = 0; k < 5; k++) ell(HX + sd * (15 + (k % 2) * 2), HY - 6 + k * 6, 3.f, 3.f, k % 2 ? H : D);
        break;
    case 60:   // Sunday: deep side part, curls tumbling over one eye
        poly({HX - 13, HY - 5, HX - 9, HY - 14, HX + 6, HY - 15, HX + 14, HY - 2, HX + 9, HY - 7, HX + 1, HY - 10, HX - 7, HY - 9}, H);
        for (int k = 0; k < 7; k++) ell(HX + 6 + (k % 2) * 3, HY - 12 + k * 4, 2.7f, 2.7f, k % 2 ? Hi : H);
        for (int k = 0; k < 5; k++) ell(HX - 14 - (k % 2), HY - 4 + k * 6, 2.5f, 2.6f, k % 2 ? H : D);
        break;
    case 61:   // curtain bangs swept to both sides + a scrunchie at the crown
        poly({HX - 13, HY - 4, HX - 8, HY - 15, HX, HY - 15, HX + 8, HY - 15, HX + 13, HY - 4, HX + 6, HY - 11, HX, HY - 12, HX - 6, HY - 11}, H);
        for (int sd = -1; sd <= 1; sd += 2) { poly({HX + sd * 1.f, HY - 12, HX + sd * 12.f, HY - 6, HX + sd * 14.f, HY + 4, HX + sd * 10.f, HY - 2, HX + sd * 4.f, HY - 8}, H); line(HX + sd * 3.f, HY - 11, HX + sd * 11.f, HY - 4, Hi); }
        ell(HX + 8, HY - 15, 3.f, 2.5f, sh(H, 0.6f), false);
        break;
    case 62:   // sleek middle part, laid flat, ties at the nape
        poly({HX - 13, HY - 2, HX - 7, HY - 14, HX, HY - 13, HX + 7, HY - 14, HX + 13, HY - 2, HX + 8, HY - 9, HX, HY - 11, HX - 8, HY - 9}, H);
        line(HX, HY - 15, HX, HY - 11, D); line(HX - 6, HY - 12, HX - 11, HY - 5, Hi); line(HX + 6, HY - 12, HX + 11, HY - 5, Hi);
        for (int sd = -1; sd <= 1; sd += 2) ell(HX + sd * 12.f, HY + 10, 2.2f, 1.6f, sh(H, 0.55f), false);
        break;
    case 63:   // 360 waves: crisp lineup with rippling wave pattern
        poly({HX - 12, HY - 4, HX - 10, HY - 13, HX + 10, HY - 13, HX + 12, HY - 4, HX + 9, HY - 8, HX - 9, HY - 8}, H);
        for (int r = 0; r < 3; r++) for (int x = -9; x <= 9; x += 2) plot((int)(HX + x), (int)(HY - 12 + r * 2 + ((x / 2) & 1)), r % 2 ? Hi : D);
        break;
    case 64:   // short twists: little coils all over the top
        for (int r = 0; r < 3; r++) for (int i = -4; i <= 4; i++) { float x = HX + i * 2.7f + (r % 2) * 1.3f, y = HY - 14 + r * 3.2f; ell(x, y, 1.7f, 2.4f, (i + r) % 2 ? H : D); plot((int)x, (int)y - 1, Hi); }
        break;
    case 65:   // curly top, faded sides
        for (int i = 0; i < 9; i++) ell(HX - 10 + i * 2.5f, HY - 15 + (i % 2) * 2, 3.f, 3.f, i % 3 ? H : D);
        for (int sd = -1; sd <= 1; sd += 2) for (int y = 0; y < 6; y++) plot((int)(HX + sd * 12), (int)(HY - 8 + y), alpha(H, (uint8_t)(200 - y * 30)));
        break;
    default:  // spiky frosted tips
        poly({HX - 12, HY - 4, HX - 9, HY - 13, HX + 9, HY - 13, HX + 12, HY - 4, HX + 8, HY - 8, HX - 8, HY - 8}, H);
        for (int i = 0; i < 6; i++) { float x = HX - 10 + i * 4; poly({x - 2, HY - 12, x + 2, HY - 12, x + 1, HY - 20 - (i % 2) * 2}, i % 2 ? lit(H, 0.55f) : H); }
        break;
    }
}
static void lipShape(int k, int my, Px LIP, Px LIPd, Px LIPh, Px S) {   // Create-a-Sim lip shapes, drawn over the classic mouth
    auto clear = [&](int x0, int x1, int y0, int y1) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) plot(x, y, S); };
    switch (k) {
    case 1: plot(46, my - 1, LIP); plot(49, my - 1, LIP); plot(47, my, sh(LIP, 0.85f)); plot(48, my, sh(LIP, 0.85f)); clear(44, 44, my, my); clear(51, 51, my, my); break;   // heart
    case 2: clear(44, 51, my, my); clear(44, 51, my + 3, my + 3); break;   // thin
    case 3: for (int x = 45; x <= 50; x++) plot(x, my + 3, LIP); for (int x = 46; x <= 49; x++) plot(x, my + 4, sh(LIP, 0.92f)); plot(47, my + 3, LIPh); break;   // pouty
    case 4: clear(44, 51, my, my + 3); plot(44, my - 1, LIPd); plot(51, my - 1, LIPd); for (int x = 45; x <= 50; x++) plot(x, my, LIPd);
            for (int x = 45; x <= 50; x++) plot(x, my + 1, rgb(250, 248, 246)); for (int x = 46; x <= 49; x++) plot(x, my + 2, LIP); break;   // big smile
    case 5: clear(44, 45, my, my + 3); clear(50, 51, my, my + 3); break;   // small
    case 6: plot(46, my - 1, LIP); plot(49, my - 1, LIP); plot(45, my - 1, alpha(LIP, 120)); plot(50, my - 1, alpha(LIP, 120)); plot(47, my, LIPd); plot(48, my, LIPd); break;   // cupid's bow
    case 7: plot(48, my + 2, LIPh); plot(49, my + 2, LIPh); plot(47, my + 3, LIPh); plot(46, my, lit(LIP, 0.4f)); break;   // glossy
    case 8: clear(44, 44, my, my); plot(51, my - 1, LIPd); plot(50, my - 1, alpha(LIPd, 140)); break;   // smirk
    case 9: clear(44, 45, my, my + 3); clear(50, 51, my, my + 3); plot(47, my + 1, sh(LIPd, 0.7f)); plot(48, my + 1, sh(LIPd, 0.7f)); plot(46, my + 4, LIP); plot(49, my + 4, LIP); plot(47, my + 4, LIP); plot(48, my + 4, LIP); break;   // round
    default: break;
    }
}
void drawFace(const Look& L) {
    // drawn at full size, pixel by pixel: bright, warm and friendly
    Px S = g_final.at(48, (int)HY + 4);   // the shaded skin under the face
    if (S.a == 0) S = rgb(L.skin);
    Px E = lit(rgb(L.eyes), 0.28f);                                    // warm, readable iris colour
    Px LIP = mixp(rgb(L.lips), rgb(244, 150, 160), 0.35f), LIPd = sh(LIP, 0.8f), LIPh = lit(LIP, 0.55f);
    Px LASH = rgb(58, 32, 38), BROW = alpha(mixp(sh(rgb(L.hair), 0.7f), rgb(92, 62, 52), 0.45f), 190);
    Px PINK = rgb(246, 140, 150);
    g_part = 120;
    const int ey = (int)HY + 2;
    // a soft rosy glow on the cheeks and nose
    {   // contour: soft shadow under the cheekbones, along the jaw and beside the nose (Create-a-Sim > Pretty)
        const int c = L.contour; const Px CT = sh(S, 0.8f);
        if (c) for (int s = -1; s <= 1; s += 2) {
            for (int k = 0; k <= 4; k++) plot(HX + s * (10 - k * 0.6f) - 0.5f, ey + 4 + k, alpha(CT, 40 + c * 25));          // cheekbone hollow, sweeping toward the mouth
            for (int k = 0; k <= 5; k++) plot(HX + s * (9 - k * 1.2f) - 0.5f, ey + 11 + k, alpha(CT, 25 + c * 20));         // jawline
            plot(HX + s * 1.5f - 0.5f, ey + 1, alpha(CT, 30 + c * 15)); plot(HX + s * 1.5f - 0.5f, ey + 2, alpha(CT, 30 + c * 15));   // nose sides
            if (c >= 2) { plot(HX + s * 9.5f - 0.5f, ey - 7, alpha(CT, 50)); plot(HX + s * 10 - 0.5f, ey - 6, alpha(CT, 50)); }   // temples
        }
        if (L.glow || c >= 2) { const Px HL = lit(S, 0.45f);   // highlighter: cheekbone tops, nose tip, cupid's bow
            for (int s = -1; s <= 1; s += 2) { plot(HX + s * 7.5f - 0.5f, ey + 3, alpha(HL, 150)); plot(HX + s * 8.5f - 0.5f, ey + 2, alpha(HL, 120)); }
            plot(47, ey + 3, alpha(HL, 160)); }
    }
    if (L.blushOn || !L.guy) {   // blush: colour from the Glam page, placement from the Pretty page
        const Px BL = L.blushOn ? rgb(L.blushC) : PINK; const int st = L.blushStyle;
        for (int s = -1; s <= 1; s += 2) for (int y = -1; y <= 1; y++) for (int x = -2; x <= 2; x++) {
            const int d = x * x + y * y * 2; if (d >= 6) continue; const int a = d == 0 ? 70 : d < 3 ? 46 : 26;
            if (st == 0) plot((s < 0 ? 41 : 54) + x, ey + 4 + y, alpha(BL, a));                                  // apples of the cheeks
            if (st == 1) plot((s < 0 ? 39 : 56) + x, ey + 1 + y - x * s / 2, alpha(BL, a));                      // draped up to the temples
            if (st == 2) { plot((s < 0 ? 42 : 53) + x, ey + 3 + y, alpha(BL, a)); }                              // sunburnt...
            if (st == 3) plot((s < 0 ? 40 : 55) + x, ey + 6 + y, alpha(BL, a - 6));                              // low & soft
        }
        if (st == 2) for (int x = 45; x <= 50; x++) plot(x, ey + 3, alpha(BL, 60));                               // ...across the nose
    }
    for (int s = -1; s <= 1; s += 2) {
        const int cx = s < 0 ? 42 : 53;          // mirrored about the centre line (47.5)
        auto P = [&](int dx, int dy, Px c) { plot(cx + dx * s, ey + dy, c); };   // dx > 0 = outer corner
        if (!L.guy) for (int x = -1; x <= 3; x++) P(x, -3, alpha(rgb(232, 170, 170), 45));   // a whisper of peachy shadow
        if (L.shadow.r | L.shadow.g | L.shadow.b) { Px SHc = rgb(L.shadow); for (int x = -2; x <= 4; x++) { P(x, -3, alpha(SHc, 170)); P(x, -4, alpha(SHc, 90)); } P(5, -4, alpha(SHc, 120)); }
        if (L.chead) {   // Custom Head: almond / siren eyes - low tapered inner corner, lifted outer corner, lid resting on the iris
            const Px W1 = rgb(246, 242, 240), W2 = rgb(220, 212, 212), RING = sh(E, 0.42f), CREASE = alpha(sh(S, 0.72f), 140), LOW = alpha(sh(S, 0.66f), 170);
            for (int x = -1; x <= 4; x++) P(x, x >= 3 ? -5 : -4, CREASE);                                   // crease, lifting outward
            for (int x = -2; x <= 3; x++) P(x, -3, alpha(sh(S, 0.88f), 100));                               // lid shadow
            // upper lash line: rises from the inner corner and flicks up at the outer corner
            P(-3, 0, alpha(LASH, 170)); P(-2, -1, LASH); for (int x = -1; x <= 3; x++) P(x, -2, LASH); P(4, -3, LASH); P(5, -3, LASH); P(6, -4, alpha(LASH, 170));
            P(4, -2, alpha(LASH, 200));
            // whites + iris (top of the iris tucked under the lid)
            P(-2, 0, W2); P(2, -1, W2); P(3, -1, W1); P(4, -1, alpha(W2, 220)); P(2, 0, W1); P(3, 0, W2);
            for (int x = -1; x <= 1; x++) P(x, -1, RING);
            P(-1, 0, sh(E, 0.8f)); P(0, 0, rgb(22, 14, 16)); P(1, 0, sh(E, 0.88f));
            P(-1, 1, E); P(0, 1, lit(E, 0.14f)); P(1, 1, E); P(2, 1, alpha(W2, 160));
            P(1, -1, alpha(rgb(255, 255, 255), 230));                                                       // catchlight
            // lower lid: tight under the iris, meeting the lifted outer corner
            P(-2, 1, LOW); for (int x = -1; x <= 1; x++) P(x, 2, LOW); P(2, 2, alpha(LASH, 70)); P(3, 1, alpha(LASH, 120)); P(4, 0, alpha(LASH, 150));
        } else {
        // round, open eyes: a tall warm iris, whites at the corners, arched lash line
        // round, open eyes: a tall warm iris, whites at the corners, arched lash line
        P(-3, 0, rgb(250, 247, 247)); P(-3, 1, rgb(244, 238, 240)); P(3, 0, rgb(250, 247, 247)); P(3, 1, rgb(244, 238, 240));
        for (int y = -1; y <= 2; y++) for (int x = -2; x <= 2; x++) {
            if ((y == 2) && std::abs(x) == 2) continue;
            P(x, y, y < 0 ? sh(E, 0.6f) : (y == 0 ? sh(E, 0.8f) : (y == 1 ? E : lit(E, 0.18f))));
        }
        P(0, 0, rgb(30, 18, 20)); P(0, 1, rgb(40, 24, 26));             // pupil
        P(-1, -1, rgb(255, 255, 255)); P(-1, 0, rgb(255, 255, 255)); P(-2, -1, rgb(236, 232, 236));   // big sparkle
        P(1, 2, lit(E, 0.65f));                                          // little sparkle
        // lashes: an arched soft dark-brown line with a cute outer flick
        for (int x = -2; x <= 2; x++) P(x, -2, LASH);
        P(-3, -1, LASH); P(3, -1, LASH); P(4, -2, LASH);
        if (!L.guy) { P(4, -3, LASH); P(5, -3, alpha(LASH, 150)); P(1, -3, alpha(LASH, 140)); P(-1, -3, alpha(LASH, 110)); }
        P(-1, 3, alpha(LASH, 40)); P(0, 3, alpha(LASH, 40)); P(1, 3, alpha(LASH, 40));
        P(3, 2, alpha(LASH, L.guy ? 0 : 60));
        // eye shape (Create-a-Sim): reshape the lids around the classic eye
        const Px LID = sh(S, 0.9f), LIDd = sh(S, 0.8f), WH = rgb(250, 247, 247);
        switch (L.eyeShape) {
        case 1: for (int x = -1; x <= 1; x++) P(x, 3, x == 0 ? E : lit(E, 0.25f)); for (int x = -1; x <= 1; x++) P(x, -3, LASH); P(-2, -2, LASH); P(2, -2, LASH); break;   // round
        case 2: P(4, -3, LASH); P(5, -4, LASH); P(6, -4, alpha(LASH, 200)); P(3, -2, LASH); P(-3, 1, alpha(LASH, 120)); P(3, 1, alpha(LASH, 90)); break;   // cat eye
        case 3: for (int x = -2; x <= 2; x++) { P(x, -2, sh(E, 0.55f)); P(x, -3, LASH); } P(-3, -2, LASH); P(3, -2, LASH); P(-1, -2, WH); P(-2, 3, alpha(LASH, 90)); P(2, 3, alpha(LASH, 90)); break;   // doe
        case 4: for (int x = -3; x <= 4; x++) { P(x, -2, LIDd); P(x, -3, LID); } for (int x = -2; x <= 3; x++) P(x, -1, LASH); break;   // hooded
        case 5: for (int x = -3; x <= 3; x++) { P(x, -2, S); P(x, -3, S); P(x, 2, S); } for (int x = -3; x <= 4; x++) P(x, -1, LASH); P(-1, 0, WH); break;   // monolid
        case 6: for (int x = -3; x <= 4; x++) { P(x, -1, LIDd); P(x, -2, LID); P(x, -3, S); } for (int x = -3; x <= 4; x++) P(x, 0, LASH); P(-1, 1, WH); break;   // sleepy
        case 7: P(3, -2, LASH); P(4, -3, LASH); P(5, -4, alpha(LASH, 180)); P(3, 0, WH); P(-3, 1, S); break;   // upturned
        case 8: P(1, 0, WH); P(-1, 1, WH); P(1, 1, lit(E, 0.8f)); P(-2, -3, alpha(LASH, 160)); P(0, -3, alpha(LASH, 160)); P(2, -3, alpha(LASH, 160)); P(5, -4, LASH); break;   // sparkle
        case 9: P(-4, 0, WH); P(-4, 1, rgb(244, 238, 240)); P(4, 0, WH); P(4, 1, rgb(244, 238, 240)); P(-3, -2, LASH); P(3, -2, LASH); P(-4, -1, LASH); P(5, -1, LASH); break;   // wide
        case 10: P(-3, 1, LASH); P(-4, 2, alpha(LASH, 160)); P(3, -2, LASH); P(4, -3, LASH); P(5, -3, LASH); P(6, -4, LASH); P(-2, -2, LID); P(-3, -1, LIDd); P(3, 1, alpha(LASH, 160)); P(4, 0, alpha(LASH, 140)); break;   // siren: low inner corner, long lifted outer
        case 11: P(3, -1, LASH); P(4, 0, LASH); P(5, 1, alpha(LASH, 170)); P(3, 2, alpha(LASH, 110)); P(-3, -2, LASH); P(4, -2, LIDd); P(3, -3, LID); break;   // downturned: outer corners dip down
        default: break;
        }
        }
        // eyeliner (Glam page)
        if (!L.guy) { const Px LN = rgb(24, 14, 18), LNs = alpha(rgb(60, 40, 40), 110);
            switch (L.liner) {
            case 1: for (int x = -3; x <= 4; x++) P(x, -2, LN); break;                                                            // thin
            case 2: for (int x = -3; x <= 4; x++) P(x, -2, LN); P(5, -3, LN); P(6, -4, alpha(LN, 200)); break;       // winged
            case 3: for (int x = -3; x <= 4; x++) P(x, -2, LN); P(5, -3, LN); P(6, -4, LN); P(-4, 1, alpha(LN, 200)); P(-3, 2, alpha(LN, 140)); break;   // cat eye (+ inner corner)
            case 4: for (int x = -3; x <= 5; x++) { P(x, -3, LNs); P(x, 3, LNs); } P(6, -2, LNs); break;                            // smudged
            case 5: for (int x = -1; x <= 3; x++) P(x, 3, alpha(LN, x >= 2 ? 170 : 110)); break;                                                             // lower line
            case 6: for (int x = 0; x <= 6; x++) P(x, -5 - x / 4, LN); break;                                                     // graphic floating line
            case 7: for (int x = -2; x <= 4; x += 2) P(x, -3, rgb(255, 230, 160)); P(5, -3, rgb(255, 255, 255)); break;           // glitter
            case 8: for (int x = -1; x <= 4; x++) P(x, 3, alpha(rgb(40, 190, 190), 190)); break;                                              // teal pop
            case 9: for (int x = -3; x <= 4; x++) P(x, -2, LN); P(5, -3, LN); P(6, -4, LN); P(4, 1, LN); P(5, 1, LN); P(6, 0, alpha(LN, 170)); break;   // double wing
            }
        }
        // lash styles (Pretty page)
        if (!L.guy) switch (L.lash) {
            case 1: for (int x = -3; x <= 5; x++) { P(x, -3, alpha(LASH, 210)); P(x, -4, alpha(LASH, x >= 1 ? 170 : 90)); } P(6, -4, LASH); P(6, -5, alpha(LASH, 140)); break;   // mink: dense & fluffy
            case 2: for (int x = -2; x <= 4; x += 2) { P(x, -3, LASH); P(x, -4, LASH); P(x + (x > 0), -5, alpha(LASH, 150)); } for (int x = -1; x <= 3; x += 2) P(x, 4, LASH); break;   // doll: spaced spikes + bottom spikes
            case 3: P(-2, -3, alpha(LASH, 120)); P(0, -4, alpha(LASH, 150)); P(1, -3, LASH); P(3, -4, LASH); P(4, -5, alpha(LASH, 160)); P(5, -4, LASH); P(6, -5, alpha(LASH, 120)); break;   // wispy
            case 4: for (int x = -1; x <= 5; x += 3) { P(x, -3, LASH); P(x, -4, LASH); P(x, -5, LASH); } break;                                                                    // spiky clumps
            case 5: for (int x = -2; x <= 3; x++) P(x, 3, alpha(LASH, 150)); for (int x = -1; x <= 3; x += 2) P(x, 4, alpha(LASH, 170)); break;                                  // bottom lashes
            case 6: for (int x = 2; x <= 6; x++) { P(x, -3, LASH); P(x + 1, -4, alpha(LASH, 170)); } break;                                                                      // fluttery outer
            default: break;
        }
        // brows (Create-a-Sim)
        const Px B2 = alpha(BROW, 120), B3 = alpha(BROW, 230);
        switch (L.brow) {
        case 1: for (int x = -2; x <= 3; x++) P(x, -6, BROW); break;                                                     // straight
        case 2: for (int x = -2; x <= 3; x++) { P(x, -7, B3); P(x, -6, B3); } break;                                     // bold
        case 3: P(-2, -6, B2); P(-1, -7, BROW); P(0, -8, BROW); P(1, -8, BROW); P(2, -7, BROW); P(3, -6, B2); break;      // high arch
        case 4: for (int x = -2; x <= 3; x++) { P(x, -7, B3); P(x, -6, BROW); } P(-1, -8, B2); P(1, -8, B2); break;      // bushy
        case 5: P(-1, -7, B2); P(0, -7, B2); P(1, -7, B2); P(2, -7, alpha(BROW, 90)); break;                             // thin
        case 6: for (int x = -2; x <= 3; x++) P(x, -7 + (x & 1), alpha(BROW, (x & 1) ? 110 : 200)); P(-1, -8, B2); break; // feathered
        case 7: P(-2, -5, BROW); P(-1, -6, BROW); P(0, -6, BROW); P(1, -7, BROW); P(2, -7, BROW); P(3, -8, B2); break;    // angled
        case 8: P(-2, -6, B2); P(-1, -7, BROW); P(0, -7, BROW); P(2, -7, alpha(BROW, 150)); P(3, -6, B2); break;          // slit
        case 9: P(-2, -5, B2); P(-1, -6, BROW); P(0, -7, BROW); P(1, -7, BROW); P(2, -6, BROW); P(3, -5, B2); break;      // rounded
        default:   // defined, softly arched brows: fuller at the front, tapering past the arch
            P(-3, -6, alpha(BROW, 140)); P(-2, -6, B3); P(-2, -7, alpha(BROW, 150)); P(-1, -7, B3); P(0, -8, BROW); P(1, -8, BROW); P(0, -7, alpha(BROW, 160)); P(2, -8, alpha(BROW, 200)); P(3, -7, alpha(BROW, 170)); P(4, -6, alpha(BROW, 110)); break;
        }
        if (L.guy && L.brow != 5) for (int x = -3; x <= 4; x++) { P(x, L.brow == 3 ? -7 : -6, BROW); P(x, L.brow == 3 ? -6 : -5, alpha(BROW, x < 3 ? 200 : 120)); }   // thick, low, straight brows
    }
    // nose: just a tiny shadow and a rosy tip
    {   const Px NS = alpha(sh(S, 0.82f), 160), NT = mixp(S, PINK, 0.28f);
        if (L.chead) {   // Custom Head: a more realistic nose - shaded bridge, lit tip, nostril wings, soft shadow under the tip
            const bool wide = L.nose == 2 || L.nose == 8; const int w = wide ? 2 : 1;
            const Px D1 = alpha(sh(S, 0.70f), 190), D2 = alpha(sh(S, 0.50f), 235);
            for (int y = -3; y <= 2; y++) plot(49, ey + y, alpha(sh(S, 0.78f), 90 + (y + 3) * 22));   // bridge shadow (light from the left)
            for (int y = -2; y <= 2; y++) plot(47, ey + y, alpha(lit(S, 0.28f), 90 + y * 10));          // bridge highlight
            plot(47, ey + 3, alpha(lit(S, 0.35f), 200)); plot(48, ey + 3, alpha(lit(S, 0.15f), 160));   // rounded lit tip
            plot(46 - w, ey + 3, alpha(sh(S, 0.76f), 170)); plot(49 + w, ey + 3, alpha(sh(S, 0.72f), 200));   // nostril wings
            for (int x = 46 - w; x <= 49 + w; x++) if (x != 47 && x != 48) plot(x, ey + 4, (x == 46 || x == 49) ? D2 : D1);   // nostrils
            plot(47, ey + 4, alpha(sh(S, 0.85f), 140)); plot(48, ey + 4, alpha(sh(S, 0.85f), 140));
            plot(47, ey + 5, alpha(sh(S, 0.84f), 120)); plot(48, ey + 5, alpha(sh(S, 0.84f), 120));      // shadow under the tip
        } else
        switch (L.nose) {
        case 1: plot(49, ey + 1, alpha(sh(S, 0.85f), 110)); plot(49, ey + 2, NS); plot(49, ey + 3, NS); plot(47, ey + 4, NT); plot(48, ey + 4, NT); break;   // straight
        case 2: plot(45, ey + 4, NS); plot(50, ey + 4, NS); plot(46, ey + 4, NT); plot(47, ey + 4, NT); plot(48, ey + 4, NT); plot(49, ey + 4, NT); plot(49, ey + 3, NS); break;   // wide
        case 3: plot(47, ey + 3, lit(S, 0.2f)); plot(46, ey + 4, alpha(sh(S, 0.7f), 170)); plot(49, ey + 4, alpha(sh(S, 0.7f), 170)); plot(47, ey + 4, NT); plot(48, ey + 4, NT); break;   // upturned
        case 4: plot(48, ey + 4, NT); plot(49, ey + 4, alpha(sh(S, 0.85f), 110)); break;   // small
        case 5: for (int y = 0; y <= 3; y++) plot(49, ey + y, alpha(sh(S, 0.8f), 120 + y * 15)); plot(48, ey + 2, alpha(sh(S, 0.85f), 90)); plot(47, ey + 4, NT); plot(48, ey + 4, NT); break;   // roman
        case 6: for (int x = 46; x <= 49; x++) plot(x, ey + 4, NT); plot(46, ey + 5, alpha(sh(S, 0.8f), 90)); plot(49, ey + 5, alpha(sh(S, 0.8f), 90)); break;   // snub
        case 7: plot(49, ey + 3, NS); plot(49, ey + 4, NS); plot(48, ey + 5, NT); plot(47, ey + 5, alpha(sh(S, 0.85f), 120)); break;   // pointed
        case 8: for (int x = 44; x <= 51; x++) plot(x, ey + 4, (x == 44 || x == 51) ? NS : NT); plot(49, ey + 3, NS); break;   // broad
        case 9: for (int y = -1; y <= 4; y++) plot(49, ey + y, alpha(sh(S, 0.84f), 90 + y * 12)); plot(47, ey + 5, NT); plot(48, ey + 5, NT); break;   // long
        default:   // small, slim doll nose: a long soft highlight down the bridge, a tiny shaded tip
            for (int y = -1; y <= 3; y++) plot(47, ey + y, alpha(lit(S, 0.3f), 90 + y * 15)); plot(48, ey + 3, alpha(lit(S, 0.2f), 110));
            plot(47, ey + 4, alpha(sh(S, 0.86f), 120)); plot(48, ey + 4, alpha(sh(S, 0.82f), 150)); plot(46, ey + 4, alpha(sh(S, 0.88f), 70)); plot(49, ey + 4, alpha(sh(S, 0.88f), 70)); break;
        }
    }
    // small, glossy, smiling lips
    const int my = ey + 7;
    if (L.chead && !L.guy) {   // Custom Head: realistic lips - shaded upper lip with a cupid's bow, lip line, full lit lower lip, shaded corners
        const Px U = sh(LIP, 0.80f), Ud = sh(LIP, 0.66f), LN = alpha(sh(LIP, 0.45f), 230), LO = LIP, HI = alpha(lit(LIP, 0.45f), 210), CR = alpha(sh(S, 0.62f), 170);
        const int w = (L.lipSh == 0) ? 3 : 2;                 // full lips are a pixel wider each side
        plot(46, my - 1, alpha(U, 160)); plot(49, my - 1, alpha(U, 160));   // cupid's bow peaks
        for (int x = 48 - w - 1; x <= 47 + w + 1; x++) plot(x, my, (x == 47 || x == 48) ? Ud : (std::abs(2 * x - 95) >= 2 * w + 1 ? alpha(U, 150) : U));   // upper lip
        for (int x = 48 - w - 1; x <= 47 + w + 1; x++) plot(x, my + 1, LN);                          // lip line
        plot(48 - w - 2, my + 1, CR); plot(47 + w + 2, my + 1, CR);                                   // corners
        for (int x = 48 - w; x <= 47 + w; x++) plot(x, my + 2, LO);                                   // lower lip
        for (int x = 48 - w + 1; x <= 47 + w - 1; x++) plot(x, my + 3, sh(LO, 0.86f));
        plot(47, my + 2, HI); plot(48, my + 2, alpha(lit(LIP, 0.3f), 170));                          // soft shine on the lower lip
        for (int x = 47 - 1; x <= 48 + 1; x++) plot(x, my + 4, alpha(sh(S, 0.82f), 100));            // shadow under the lower lip
        plot(47, my - 2, alpha(sh(S, 0.86f), 90)); plot(48, my - 2, alpha(sh(S, 0.86f), 90));        // philtrum
    } else if (!L.guy) {
        // full, glossy, overlined pout: a cupid's bow, a deep middle line and a wet shine on the bottom lip
        plot(46, my, LIP); plot(49, my, LIP); plot(47, my, sh(LIP, 0.92f)); plot(48, my, sh(LIP, 0.92f));   // cupid's bow
        plot(45, my, alpha(LIP, 140)); plot(50, my, alpha(LIP, 140));
        for (int x = 45; x <= 50; x++) plot(x, my + 1, (x == 45 || x == 50) ? alpha(LIPd, 170) : alpha(LIPd, 230));   // middle line
        plot(44, my + 1, alpha(LIPd, 80)); plot(51, my + 1, alpha(LIPd, 80));
        for (int x = 45; x <= 50; x++) plot(x, my + 2, LIP);
        for (int x = 46; x <= 49; x++) plot(x, my + 3, sh(LIP, 0.94f));
        plot(46, my + 2, LIPh); plot(47, my + 2, LIPh); plot(48, my + 2, alpha(LIPh, 140));   // gloss
        plot(47, my - 1, alpha(lit(S, 0.3f), 110)); plot(48, my - 1, alpha(lit(S, 0.3f), 110));   // highlight above the bow
        lipShape(L.lipSh, my, LIP, LIPd, LIPh, S);
    } else {   // fuller nude lips, matte and masculine (teen to elder)
        Px N = rgb(L.lips), Nd = sh(N, 0.78f), Nl = sh(N, 0.9f);
        for (int x = 46; x <= 49; x++) plot(x, my, alpha(Nl, 200));
        plot(45, my, alpha(Nl, 120)); plot(50, my, alpha(Nl, 120));
        for (int x = 45; x <= 50; x++) plot(x, my + 1, (x == 45 || x == 50) ? alpha(Nd, 200) : Nd);
        plot(44, my + 1, alpha(Nd, 110)); plot(51, my + 1, alpha(Nd, 110));
        for (int x = 45; x <= 50; x++) plot(x, my + 2, N);
        for (int x = 46; x <= 49; x++) plot(x, my + 3, Nl);
        plot(47, my + 2, alpha(lit(N, 0.12f), 200));
        lipShape(L.lipSh, my, N, Nd, Nl, S);
    }
}
void drawBody(const Look& L) {
    Px S = rgb(L.skin), PINK = rgb(236, 130, 140);
    g_part = 20; g_skin = true;
    const float hipW = L.guy ? 8.5f : 9.5f, shW = L.guy ? 12.f : 9.5f;
    // long slim legs: thigh -> knee -> calf -> ankle, with rosy knees
    for (int s = -1; s <= 1; s += 2) {
        float hx = HX + s * 4.6f, kx = HX + s * 4.1f, ax = HX + s * 4.4f;
        limb(hx, 88, L.guy ? 5.4f : 6.0f, kx, 132, 3.3f, S);
        limb(kx, 132, 3.3f, kx + s * 0.3f, 148, 3.4f, S);
        limb(kx + s * 0.3f, 148, 3.4f, ax, 174, 1.9f, S);
        ell(kx - 0.3f, 131, 2.2f, 2.0f, alpha(PINK, 70), false);
    }
    // arms
    for (int s = -1; s <= 1; s += 2) {
        float sx = HX + s * (shW + 1.f);
        const float aw = L.guy ? 1.25f : 1.f;   // guys: fuller shoulders, biceps and forearms
        limb(sx, 54, 3.1f * aw, sx + s * 2.2f, 78, 2.3f * aw, S);
        limb(sx + s * 2.2f, 78, 2.3f * aw, sx + s * 2.7f, 98, 1.7f * aw, S);
        ell(sx + s * 2.2f, 78, 1.6f, 1.4f, alpha(PINK, 50), false);
        ell(sx + s * 2.9f, 102, 2.1f, 4.0f, S);   // hand
        { static const RGB NC[8] = {{255, 170, 190}, {255, 70, 150}, {210, 20, 40}, {30, 25, 35}, {255, 255, 255}, {225, 180, 160}, {190, 150, 240}, {235, 200, 90}};
          const RGB nc = NC[L.nails % 8]; if (L.nails) ell(sx + s * 2.9f, 105, 1.6f, 1.2f, Px{nc.r, nc.g, nc.b, 255}, false); else ell(sx + s * 2.9f, 105, 1.4f, 1.0f, alpha(PINK, 90), false); }   // nails (salon polish)
    }
    // neck & torso
    limb(HX, 38, L.guy ? 4.3f : 3.1f, HX, 52, L.guy ? 4.8f : 3.4f, S);   // guys: thicker neck
    if (L.guy) ell(HX, 46, 1.0f, 1.4f, alpha(sh(S, 0.8f), 90), false);   // adam's apple
    ell(HX, 41.0f, 4.2f, 1.2f, alpha(rgb((int)(L.skin.r * 0.86f), (int)(L.skin.g * 0.72f), (int)(L.skin.b * 0.72f)), 150), false);   // soft shadow under the chin
    poly({HX - shW, 51, HX + shW, 51, HX + shW - 1.f, 58, HX + (L.guy ? 8.f : 6.f), 72, HX + hipW, 88, HX - hipW, 88, HX - (L.guy ? 8.f : 6.f), 72, HX - shW + 1.f, 58}, S);
    ell(HX - 2, 52, 3, 1, alpha(lit(S, 0.4f), 80), false);   // collarbone light
    // head: one smooth oval with a soft tapered chin, small ears
    g_part = 21;
    ell(HX - 11.2f, HY + 2, 1.9f, 2.9f, S); ell(HX + 11.2f, HY + 2, 1.9f, 2.9f, S);
    for (int y = (int)((HY - 13) * g_ss); y <= (int)((HY + 15) * g_ss); y++)
        for (int x = (int)((HX - 13) * g_ss); x <= (int)((HX + 13) * g_ss); x++) {
            float fx = (x + 0.5f) / g_ss, fy = (y + 0.5f) / g_ss;
            float dy = (fy - HY) / (fy < HY ? 12.0f : 13.0f);   // glam doll face: a slightly longer lower face
            // heart shape: full cheekbones high up, then a V jawline tapering to a small soft chin
            float rx = L.guy ? 11.8f * (dy > 0 ? std::max(0.5f, 1.f - 0.05f * dy - 0.42f * dy * dy * dy * dy) : 1.f)   // guys: square, strong jaw and a broad chin
                             : 11.4f * (dy > 0 ? std::max(0.2f, 1.f - 0.18f * dy - 0.5f * dy * dy * dy) : 1.f);
            float dx = (fx - HX) / rx;
            bool in = dy < 0 ? dx * dx + dy * dy <= 1.f : std::fabs(dx) <= 1.f && dy <= 1.f;
            if (in && std::fabs(dy) <= 1.f) {
                Px c = vol(S, dx * 0.9f, dy * 0.4f);
                if (L.guy && dy > 0.45f && std::fabs(dx) > 0.78f) c = sh(c, 0.88f);   // jawline shadow
                plot(x, y, c);
            }
        }
    g_skin = false;
}
void drawShoes(const Look& L) {
    Px C = rgb(L.shoeC), W = rgb(244, 242, 240), K = rgb(30, 28, 34);
    g_part = 30;
    for (int s = -1; s <= 1; s += 2) {
        float ax = HX + s * 4.5f;
        switch (L.shoes) {
        case S_STRAPPY:   // lace-up heels
            poly({ax - 2.5f, 172, ax + 2.5f, 172, ax + 3.f + s * 0.5f, 182, ax - 2.f, 182}, rgb(L.skin));
            for (int i = 0; i < 4; i++) line(ax - 3, 164 + i * 3, ax + 3, 166 + i * 3, C);
            line(ax - 3, 182, ax + 4, 182, sh(C, 0.8f)); line(ax + s * 2.5f, 178, ax + s * 2.5f, 184, sh(C, 0.7f));
            break;
        case S_SNEAK: case S_CHUNKY:
            ell(ax + s * 1.f, 178, 5.5f, 4.f, L.shoes == S_CHUNKY ? W : C); poly({ax - 5, 179, ax + 6, 179, ax + 6, 183, ax - 5, 183}, W);
            line(ax - 3, 176, ax + 3, 174, K); line(ax - 2, 178, ax + 3, 177, K);
            break;
        case S_PLATFORM:
            poly({ax - 4, 174, ax + 4, 174, ax + 4.5f, 180, ax - 4.5f, 180}, C); poly({ax - 5, 180, ax + 5, 180, ax + 5, 186, ax - 5, 186}, sh(C, 0.8f));
            break;
        case S_COMBAT: case S_WORKBOOT:
            limb(ax, 160, 3.6f, ax, 176, 4.f, C); ell(ax + s * 1.f, 179, 5.5f, 4, C); poly({ax - 5, 181, ax + 6, 181, ax + 6, 185, ax - 5, 185}, L.shoes == S_COMBAT ? K : sh(C, 0.6f));
            for (int i = 0; i < 3; i++) line(ax - 2, 163 + i * 4, ax + 2, 163 + i * 4, L.shoes == S_COMBAT ? rgb(200, 200, 200) : sh(C, 0.7f));
            break;
        case S_FLIPFLOP: case S_WEDGE: case S_STUDHEEL: {
            g_skin = true; ell(ax + s * 0.8f, 177, 3.0f, 3.6f, rgb(L.skin)); g_skin = false;   // the foot
            if (L.shoes == S_FLIPFLOP) { poly({ax - 3.5f, 180, ax + 4.5f, 180, ax + 4.5f, 182, ax - 3.5f, 182}, C); line(ax + s * 0.5f, 180, ax - 2, 176, sh(C, 0.8f)); line(ax + s * 0.5f, 180, ax + 3, 176, sh(C, 0.8f)); }
            else if (L.shoes == S_WEDGE) { poly({ax - 4, 180, ax + 5, 180, ax + 5, 186, ax - 4, 183}, C); line(ax - 3, 176, ax + 4, 176, sh(C, 0.8f)); line(ax - 3, 173, ax + 3, 173, sh(C, 0.8f)); }
            else { line(ax - 3, 176, ax + 4, 177, C); line(ax - 3, 173, ax + 3, 174, C); line(ax - 2, 170, ax + 2, 170, C); line(ax - 3, 182, ax + 4, 182, sh(C, 0.8f)); line(ax + s * 3, 178, ax + s * 3, 185, sh(C, 0.7f));
                   for (int k = 0; k < 3; k++) dot((int)(ax - 2 + k * 2), 176, rgb(250, 250, 250)); }
            break;
        }
        case S_KNEEBOOT:
            limb(ax - s * 0.2f, 136, 4.f, ax, 176, 3.f, C); ell(ax + s * 1.f, 179, 4.5f, 3.5f, C); line(ax + s * 2, 180, ax + s * 2, 186, sh(C, 0.6f));
            break;
        }
    }
}
// little bitmap digits for jersey numbers
const uint8_t DIG[10][5] = {{7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 3, 1, 7}, {5, 5, 7, 1, 1}, {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 2, 2, 2}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7}};
void digits(int n, int x, int y, Px c) { char b[4]; snprintf(b, sizeof b, "%02d", n % 100); for (int i = 0; i < 2; i++) for (int r = 0; r < 5; r++) for (int k = 0; k < 3; k++) if (DIG[b[i] - '0'][r] >> (2 - k) & 1) dot(x + i * 4 + k, y + r, c); }
void hibiscus(float cx, float cy, float r, Px c) {
    for (int i = 0; i < 5; i++) { float a = i * 1.2566f - 1.57f; ell(cx + std::cos(a) * r * 0.55f, cy + std::sin(a) * r * 0.55f, r * 0.5f, r * 0.5f, c); }
    ell(cx, cy, r * 0.25f, r * 0.25f, sh(c, 0.7f), false);
}
// clear the skin peeking through between trouser legs (big canvas)
void clearGap(float y0, float y1, float hw) {
    for (int y = (int)(y0 * g_ss); y < (int)(y1 * g_ss); y++)
        for (int x = (int)((HX - hw) * g_ss); x < (int)((HX + hw) * g_ss); x++)
            if (C_->solid(x, y) && C_->pt(x, y) == 20) { C_->at(x, y) = Px{0, 0, 0, 0}; C_->pt(x, y) = 0; }
}
void drawBottom(const Look& L) {
    Px B = rgb(L.botC), B2 = rgb(L.botC2);
    g_part = 40;
    switch (L.bottom) {
    case B_RUFFLE:   // low-rise ruffle mini
        poly({HX - 9, 86, HX + 9, 86, HX + 13, 100, HX - 13, 100}, B);
        for (int i = 0; i < 9; i++) { float x = HX - 13 + i * 3.2f; ell(x + 1.5f, 101, 2.2f, 2.5f, sh(B, 0.85f)); line(x + 1, 90, x + 1.5f, 100, sh(B, 0.82f)); }
        line(HX - 9, 88, HX + 9, 88, lit(B, 0.15f));
        break;
    case B_JORTS: case B_CARGOSHORT:   // baggy knee-length denim
        poly({HX - 10, 84, HX + 10, 84, HX + 14, 130, HX + 1, 130, HX, 104, HX - 1, 130, HX - 14, 130}, B);
        for (int i = 0; i < 4; i++) { line(HX - 12 + i, 112 + i * 4, HX - 7 + i, 110 + i * 4, lit(B, 0.18f)); line(HX + 7, 112 + i * 4, HX + 12, 110 + i * 4, lit(B, 0.18f)); }
        line(HX - 14, 129, HX - 1, 129, lit(B, 0.25f)); line(HX + 1, 129, HX + 14, 129, lit(B, 0.25f));
        clearGap(104, 131, 1.f);
        if (L.bottom == B_CARGOSHORT) { poly({HX - 14, 112, HX - 9, 112, HX - 9, 122, HX - 14, 122}, sh(B, 0.85f)); poly({HX + 9, 112, HX + 14, 112, HX + 14, 122, HX + 9, 122}, sh(B, 0.85f)); }
        break;
    case B_FLARE:
        poly({HX - 9, 86, HX + 9, 86, HX + 8, 132, HX + 11, 176, HX + 1, 176, HX + 1, 104, HX - 1, 104, HX - 1, 176, HX - 11, 176, HX - 8, 132}, B);
        line(HX - 6, 90, HX - 6, 170, lit(B, 0.15f)); line(HX + 5, 90, HX + 5, 170, sh(B, 0.85f));
        clearGap(104, 180, 1.f);
        break;
    case B_TRACKPANT:
        poly({HX - 9, 82, HX + 9, 82, HX + 9, 130, HX + 9, 176, HX + 1, 176, HX + 1, 102, HX - 1, 102, HX - 1, 176, HX - 9, 176, HX - 9, 130}, B);
        line(HX - 9, 92, HX - 9, 174, B2); line(HX + 8, 92, HX + 8, 174, B2);
        clearGap(102, 180, 1.f);
        break;
    case B_DENIMMINI:
        poly({HX - 9, 86, HX + 9, 86, HX + 11, 104, HX - 11, 104}, B); line(HX, 88, HX, 104, sh(B, 0.75f)); line(HX - 11, 103, HX + 11, 103, lit(B, 0.3f));
        break;
    case B_CARGO: case B_BAGGYJEAN:
        poly({HX - 10, 84, HX + 10, 84, HX + 12, 132, HX + 12, 176, HX + 1, 176, HX + 1, 104, HX - 1, 104, HX - 1, 176, HX - 12, 176, HX - 12, 132}, B);
        if (L.bottom == B_CARGO) { poly({HX - 13, 118, HX - 7, 118, HX - 7, 130, HX - 13, 130}, sh(B, 0.85f)); poly({HX + 7, 118, HX + 13, 118, HX + 13, 130, HX + 7, 130}, sh(B, 0.85f)); for (int i = 0; i < 8; i++) dot((int)HX - 10 + i % 4 * 6, 94 + i * 9, B2); }
        else for (int i = 0; i < 3; i++) { line(HX - 10, 168 + i * 3, HX - 2, 168 + i * 3, sh(B, 0.85f)); line(HX + 2, 168 + i * 3, HX + 10, 168 + i * 3, sh(B, 0.85f)); }
        clearGap(104, 180, 1.f);
        break;
    case B_WIDELEG:   // fold-over wide-leg lounge pants
        poly({HX - 9.5f, 84, HX + 9.5f, 84, HX + 13, 176, HX + 1.5f, 176, HX + 1, 104, HX - 1, 104, HX - 1.5f, 176, HX - 13, 176}, B);
        poly({HX - 9.5f, 84, HX + 9.5f, 84, HX + 9.8f, 89, HX - 9.8f, 89}, lit(B, 0.12f));
        line(HX - 7, 110, HX - 9, 172, sh(B, 0.88f)); line(HX + 7, 110, HX + 9, 172, sh(B, 0.88f));
        clearGap(104, 180, 1.f);
        break;
    case B_CAPRI:   // cuffed denim capris
        poly({HX - 9, 86, HX + 9, 86, HX + 9.5f, 150, HX + 1, 150, HX + 0.5f, 104, HX - 0.5f, 104, HX - 1, 150, HX - 9.5f, 150}, B);
        poly({HX - 9.6f, 145, HX - 1, 145, HX - 1, 151, HX - 9.8f, 151}, lit(B, 0.2f)); poly({HX + 1, 145, HX + 9.6f, 145, HX + 9.8f, 151, HX + 1, 151}, lit(B, 0.2f));
        line(HX - 6, 92, HX - 6, 140, lit(B, 0.15f)); line(HX + 6, 92, HX + 6, 140, sh(B, 0.85f)); line(HX - 8, 92, HX - 4, 96, sh(B, 0.8f));
        clearGap(104, 152, 1.f);
        break;
    case B_MICRO:   // frayed denim micro shorts
        poly({HX - 9, 86, HX + 9, 86, HX + 10.5f, 100, HX + 1, 100, HX, 97, HX - 1, 100, HX - 10.5f, 100}, B);
        for (int x = -10; x <= 10; x += 2) dot((int)HX + x, 100 + (x & 2 ? 1 : 0), rgb(236, 240, 248));
        line(HX - 7, 89, HX - 4, 93, sh(B, 0.8f)); line(HX + 7, 89, HX + 4, 93, sh(B, 0.8f));
        break;
    case B_MAXI: {   // low-rise maxi skirt with little flowers
        poly({HX - 9.5f, 86, HX + 9.5f, 86, HX + 14, 178, HX - 14, 178}, B);
        for (int i = 0; i < 9; i++) { float x = HX - 10 + (i * 37 % 20), y = 110 + i * 7.f; hibiscus(x, y, 2.6f, B2); }
        line(HX - 2, 100, HX - 4, 176, sh(B, 0.9f)); line(HX + 5, 100, HX + 8, 176, sh(B, 0.9f));
        break;
    }
    case B_MIDI:   // light denim midi skirt with lace trim
        poly({HX - 9.5f, 86, HX + 9.5f, 86, HX + 11.5f, 152, HX - 11.5f, 152}, B);
        for (int y = 90; y < 150; y += 3) { dot((int)HX + 4 + ((y / 3) % 2), y, B2); dot((int)HX + 5 - ((y / 3) % 2), y + 1, B2); }
        for (int x = -11; x <= 11; x += 2) ell(HX + x, 152, 1.3f, 1.2f, B2, false);
        line(HX - 6, 92, HX - 7, 148, sh(B, 0.88f));
        break;
    case B_PLAID:   // pleated plaid mini
        poly({HX - 9, 86, HX + 9, 86, HX + 13, 104, HX - 13, 104}, B);
        pattern(40, HX - 14, 84, HX + 14, 106, [&](float x, float y, Px o, Px& n) { int xi = (int)std::floor(x), yi = (int)std::floor(y); if (xi % 5 == 0 || yi % 5 == 0) { n = mixp(o, B2, 0.65f); return true; } if ((xi + yi) % 7 == 0) { n = sh(o, 0.9f); return true; } return false; });
        for (int i = 0; i < 6; i++) line(HX - 10 + i * 4, 90, HX - 12 + i * 4.8f, 104, sh(B, 0.8f));
        break;
    case B_WRAP:   // low-rise wrap maxi with a high-low hem
        poly({HX - 9.5f, 86, HX + 9.5f, 86, HX + 14, 152, HX + 10, 160, HX + 2, 170, HX - 8, 178, HX - 14, 176}, B);
        line(HX + 9, 88, HX - 4, 176, sh(B, 0.78f)); line(HX + 6, 96, HX + 12, 150, lit(B, 0.12f)); line(HX - 9, 100, HX - 12, 172, sh(B, 0.88f));
        break;
    case B_GOWN: {   // floor-length ball gown: fitted waist, big flared skirt, sparkly hem
        poly({HX - 8.5f, 62, HX + 8.5f, 62, HX + 9, 76, HX + 11, 92, HX + 22, 160, HX + 25, 184, HX - 25, 184, HX - 22, 160, HX - 11, 92}, B);
        poly({HX - 8.5f, 62, HX + 8.5f, 62, HX + 9, 66, HX - 9, 66}, sh(B, 0.8f));
        for (int i = -2; i <= 2; i++) line(HX + i * 2.5f, 84, HX + i * 9.f, 182, i % 2 ? sh(B, 0.84f) : lit(B, 0.14f));
        for (int x = -24; x <= 24; x += 2) ell(HX + x, 183, 1.4f, 1.3f, sh(B, 0.75f), false);
        for (int i = 0; i < 14; i++) { float x = HX - 18 + (i * 29 % 36), y = 100 + (i * 53 % 78); if (std::fabs(x - HX) < 6 + (y - 92) * 0.2f) dot((int)x, (int)y, rgb(255, 250, 230)); }
        break;
    }
    default: break;
    }
    if (L.extras & X_LEOPARD) pattern(40, HX - 16, 84, HX + 16, 180, [&](float x, float y, Px o, Px& n) { float v = std::sin(x * 1.3f + std::sin(y * 0.9f) * 2.f) * std::cos(y * 1.1f - x * 0.4f); if (v > 0.55f) { n = rgb(70, 46, 30); return true; } if (v > 0.35f) { n = rgb(150, 104, 60); return true; } return false; });
}
static void drawDesign(const Look& L) {   // Design Studio: map the 32x32 drawing over the top, keeping the fabric shading
    if (L.design < 0) return;
    const Px base = rgb(L.topC); const float bl = std::max(30.f, (base.r * 3 + base.g * 6 + base.b) / 10.f);
    pattern(50, HX - 34, 56, HX + 34, 190, [&](float x, float y, Px o, Px& n) {
        uint8_t r, g, b; if (!dsPixel(L.design, (x - (HX - 26)) / 52.f, (y - 60) / 72.f, r, g, b)) return false;
        const float lum = (o.r * 3 + o.g * 6 + o.b) / 10.f, k = std::max(0.55f, std::min(1.25f, lum / bl));
        n = Px{(uint8_t)std::min(255.f, r * k), (uint8_t)std::min(255.f, g * k), (uint8_t)std::min(255.f, b * k), o.a}; return true; });
}
void drawTop(const Look& L) {
    if (L.top < 0) return;   // shirtless
    Px T = rgb(L.topC), T2 = rgb(L.topC2);
    g_part = 50;
    const float shW = L.guy ? 12.f : 9.5f;
    switch (L.top) {
    case T_BABYTEE: case T_BUTTERFLY:   // fitted cropped tee with cap sleeves
        poly({HX - shW - 1, 51, HX + shW + 1, 51, HX + shW, 62, HX + 7, 76, HX - 7, 76, HX - shW, 62}, T);
        for (int s = -1; s <= 1; s += 2) limb(HX + s * (shW + 1), 54, 4.2f, HX + s * (shW + 2.5f), 62, 3.8f, T);
        ell(HX, 52, 3.5f, 2.f, rgb(L.skin), false);
        if (L.top == T_BABYTEE) { hibiscus(HX + 2, 64, 7, T2); hibiscus(HX + 6, 57, 3.5f, T2); }
        else { Px b = T2; ell(HX - 4, 62, 4, 5, b); ell(HX + 4, 62, 4, 5, b); ell(HX - 3, 69, 2.5f, 3, sh(b, 0.85f)); ell(HX + 3, 69, 2.5f, 3, sh(b, 0.85f)); line(HX, 58, HX, 72, sh(b, 0.5f)); }
        break;
    case T_JERSEY: case T_BASEBALL: case T_BIGTEE:   // oversized
        poly({HX - shW - 2, 51, HX + shW + 2, 51, HX + shW + 1, 70, HX + 11, 92, HX - 11, 92, HX - shW - 1, 70}, T);
        for (int s = -1; s <= 1; s += 2) limb(HX + s * (shW + 1.5f), 54, 5.f, HX + s * (shW + 4.f), 70, 4.5f, T);
        if (L.top == T_JERSEY) { digits(7, (int)HX - 4, 62, T2); line(HX - 3, 52, HX + 3, 52, T2); }
        if (L.top == T_BASEBALL) { for (int y = 53; y <= 90; y += 4) dot((int)HX, y, T2); line(HX - shW - 2, 54, HX - 3, 52, T2); line(HX + shW + 2, 54, HX + 3, 52, T2); digits(23, (int)HX + 2, 60, T2); }
        break;
    case T_HALTER:
        poly({HX - 7, 54, HX + 7, 54, HX + 8, 66, HX + 6, 74, HX - 6, 74, HX - 8, 66}, T); line(HX - 6, 54, HX - 2, 44, sh(T, 0.8f)); line(HX + 6, 54, HX + 2, 44, sh(T, 0.8f));
        for (int i = 0; i < 6; i++) dot((int)HX - 5 + i * 2, 71, lit(T, 0.4f));
        break;
    case T_TRACKZIP:   // velour zip-up, cropped
        poly({HX - shW - 1, 51, HX + shW + 1, 51, HX + shW, 62, HX + 8, 78, HX - 8, 78, HX - shW, 62}, T);
        for (int s = -1; s <= 1; s += 2) { limb(HX + s * (shW + 1), 54, 4.2f, HX + s * (shW + 3.f), 80, 3.4f, T); limb(HX + s * (shW + 3.f), 80, 3.4f, HX + s * (shW + 3.5f), 99, 3.f, T); }
        line(HX, 52, HX, 78, T2); line(HX - 3, 52, HX, 56, sh(T, 0.7f)); line(HX + 3, 52, HX, 56, sh(T, 0.7f));
        break;
    case T_SLIP:   // satin slip dress to mid-thigh
        poly({HX - 7, 54, HX + 7, 54, HX + 8, 72, HX + 12, 108, HX - 12, 108, HX - 8, 72}, T);
        line(HX - 6, 54, HX - 6, 46, T2); line(HX + 6, 54, HX + 6, 46, T2);
        for (int i = 0; i < 5; i++) line(HX - 8 + i * 4, 80, HX - 10 + i * 5, 106, lit(T, 0.12f));
        for (int i = 0; i < 12; i++) dot((int)HX - 11 + i * 2, 107, T2);
        break;
    case T_CAMI:   // striped cami with thin straps
        poly({HX - 8, 54, HX + 8, 54, HX + 8.5f, 66, HX + 7, 78, HX - 7, 78, HX - 8.5f, 66}, T);
        line(HX - 6, 54, HX - 7, 48, sh(T, 0.8f)); line(HX + 6, 54, HX + 7, 48, sh(T, 0.8f));
        pattern(50, HX - 10, 52, HX + 10, 80, [&](float, float y, Px o, Px& n) { if (((int)y % 4) < 2) { n = mixp(o, T2, 0.7f); return true; } return false; });
        break;
    case T_GRAPHIC: case T_TEXTTEE:   // fitted cropped baby tee with a cute graphic
        poly({HX - shW - 1, 51, HX + shW + 1, 51, HX + shW, 62, HX + 7, 74, HX - 7, 74, HX - shW, 62}, T);
        for (int s = -1; s <= 1; s += 2) limb(HX + s * (shW + 1), 54, 4.2f, HX + s * (shW + 2.5f), 62, 3.8f, T);
        ell(HX, 52, 3.5f, 2.f, rgb(L.skin), false);
        if (L.top == T_TEXTTEE) {
            line(HX - 4, 51, HX + 4, 51, T2); for (int s = -1; s <= 1; s += 2) line(HX + s * (shW + 0.5f), 61, HX + s * (shW + 4.5f), 63, T2);
            static const uint8_t GL[4][5] = {{17, 17, 21, 21, 10}, {14, 17, 17, 17, 14}, {14, 17, 31, 17, 17}, {17, 17, 31, 17, 17}};   // W O A H (5x5)
            for (int c = 0; c < 4; c++) for (int r = 0; r < 5; r++) for (int k = 0; k < 5; k++) if (GL[c][r] >> (4 - k) & 1) dot((int)HX - 12 + c * 6 + k, 58 + r, (k == 0 || r == 0) ? lit(T2, 0.3f) : T2);
            Px PS = rgb(250, 170, 190);
            for (int i = 0; i < 5; i++) { int x = (int)HX - 8 + i * 4, y = 66 + (i & 1) * 3; Px c = i & 1 ? PS : T2; dot(x, y, c); dot(x - 1, y, c); dot(x + 1, y, c); dot(x, y - 1, c); dot(x, y + 1, c); }
        }
        else {   // an original cheery strawberry buddy
            ell(HX + 1, 64, 4.5f, 4.f, rgb(240, 70, 90)); poly({HX - 2, 60, HX + 4, 60, HX + 1, 58}, rgb(90, 190, 90), false);
            dot((int)HX - 1, 63, rgb(40, 20, 20)); dot((int)HX + 2, 63, rgb(40, 20, 20)); dot((int)HX, 66, rgb(255, 220, 220)); dot((int)HX + 1, 66, rgb(255, 220, 220));
        }
        break;
    case T_SWIRL: {   // slouchy off-shoulder tie-dye swirl top
        poly({HX - 13, 57, HX - 4, 53, HX + 10, 51, HX + 15, 58, HX + 16, 72, HX + 9, 82, HX - 4, 86, HX - 12, 80, HX - 14, 68}, T);
        limb(HX + 12, 55, 4.f, HX + 17, 68, 4.f, T);
        const Px SW[3] = {T, T2, rgb(120, 220, 220)};
        pattern(50, HX - 16, 48, HX + 20, 90, [&](float x, float y, Px o, Px& n) { float v = std::sin(x * 0.35f + std::sin(y * 0.22f) * 2.2f + y * 0.12f); int k = v < -0.33f ? 0 : v < 0.33f ? 1 : 2; n = mixp(SW[k], o, 0.35f); return true; });
        line(HX - 13, 57, HX - 4, 53, sh(T, 0.7f));
        break;
    }
    case T_POLKATUBE:   // polka-dot tube top with a bow
        poly({HX - 9, 56, HX + 9, 56, HX + 8.5f, 76, HX - 8.5f, 76}, T);
        pattern(50, HX - 10, 54, HX + 10, 78, [&](float x, float y, Px o, Px& n) { int ix = (int)std::floor(x), iy = (int)std::floor(y); if (((ix + (iy / 4) * 2) % 4 == 0) && (iy % 4 == 1)) { n = mixp(T2, o, 0.15f); return true; } return false; });
        poly({HX - 5, 55, HX, 59, HX - 5, 63}, sh(T, 0.85f)); poly({HX + 5, 55, HX, 59, HX + 5, 63}, sh(T, 0.85f)); ell(HX, 59, 1.6f, 1.6f, sh(T, 0.7f));
        line(HX - 1, 60, HX - 3, 68, sh(T, 0.75f)); line(HX + 1, 60, HX + 3, 68, sh(T, 0.75f));
        break;
    case T_CRISSCROSS:   // criss-cross halter
        poly({HX - 9, 60, HX - 1, 58, HX - 1, 68, HX - 8, 68}, T); poly({HX + 9, 60, HX + 1, 58, HX + 1, 68, HX + 8, 68}, T);
        limb(HX - 8, 61, 1.f, HX + 3, 44, 1.f, T); limb(HX + 8, 61, 1.f, HX - 3, 44, 1.f, T);
        line(HX - 9, 68, HX + 9, 68, sh(T, 0.85f));
        break;
    case T_MOTO:   // cropped black zip moto jacket over a black top
        poly({HX - 7, 56, HX + 7, 56, HX + 7, 76, HX - 7, 76}, sh(T, 0.8f));
        poly({HX - shW - 1.5f, 51, HX - 2, 51, HX - 3, 64, HX - 4, 80, HX - shW, 78}, T); poly({HX + shW + 1.5f, 51, HX + 2, 51, HX + 3, 64, HX + 4, 80, HX + shW, 78}, T);
        for (int s = -1; s <= 1; s += 2) { limb(HX + s * (shW + 1), 54, 4.f, HX + s * (shW + 3.f), 80, 3.2f, T); limb(HX + s * (shW + 3.f), 80, 3.2f, HX + s * (shW + 3.5f), 98, 2.8f, T); }
        line(HX - 3, 54, HX - 4, 80, lit(T, 0.5f)); poly({HX - 6, 51, HX - 2, 51, HX - 4, 57}, lit(T, 0.2f)); poly({HX + 6, 51, HX + 2, 51, HX + 4, 57}, lit(T, 0.2f));
        break;
    case T_SHRUG:   // white wrap shrug over a butter-yellow cami
        poly({HX - 8, 56, HX + 8, 56, HX + 8.5f, 70, HX + 7, 80, HX - 7, 80, HX - 8.5f, 70}, T2);
        poly({HX - shW - 1, 51, HX + shW + 1, 51, HX + shW, 58, HX + 2, 64, HX - 2, 64, HX - shW, 58}, T);
        line(HX - shW, 56, HX + 2, 63, sh(T, 0.85f)); line(HX + shW, 56, HX - 2, 63, sh(T, 0.85f));
        for (int s = -1; s <= 1; s += 2) { limb(HX + s * (shW + 1), 54, 3.8f, HX + s * (shW + 3.f), 80, 3.0f, T); limb(HX + s * (shW + 3.f), 80, 3.0f, HX + s * (shW + 3.5f), 100, 2.8f, T); }
        break;
    case T_POLO:   // relaxed polo with an open collar
        poly({HX - shW - 2, 51, HX + shW + 2, 51, HX + shW + 1, 70, HX + 10, 90, HX - 10, 90, HX - shW - 1, 70}, T);
        for (int s = -1; s <= 1; s += 2) limb(HX + s * (shW + 1.5f), 54, 5.f, HX + s * (shW + 4.f), 68, 4.5f, T);
        poly({HX - 5, 50, HX - 1, 50, HX, 60, HX - 2, 55}, lit(T, 0.15f)); poly({HX + 5, 50, HX + 1, 50, HX, 60, HX + 2, 55}, lit(T, 0.15f));
        ell(HX, 53, 2.f, 4.f, rgb(L.skin), false);
        break;
    case T_HOODIE: case T_STARHOODIE: {   // oversized zip hoodie (open over a tank, or zipped with stars)
        const bool open = L.top == T_HOODIE;
        if (open) poly({HX - 7, 52, HX + 7, 52, HX + 7.5f, 70, HX + 7, 92, HX - 7, 92, HX - 7.5f, 70}, T2);
        poly({HX - shW - 2.5f, 50, HX + shW + 2.5f, 50, HX + shW + 2, 72, HX + 12, 94, HX - 12, 94, HX - shW - 2, 72}, T);
        if (open) poly({HX - 4, 50, HX + 4, 50, HX + 4.5f, 94, HX - 4.5f, 94}, T2);
        for (int s = -1; s <= 1; s += 2) { limb(HX + s * (shW + 2), 54, 5.f, HX + s * (shW + 4.f), 80, 4.4f, T); limb(HX + s * (shW + 4.f), 80, 4.4f, HX + s * (shW + 4.5f), 100, 4.f, T); }
        poly({HX - 11, 49, HX - 4, 46, HX + 4, 46, HX + 11, 49, HX + 6, 52, HX - 6, 52}, sh(T, 0.82f));   // the hood bunched at the neck
        if (open) { line(HX - 4.5f, 52, HX - 4.5f, 94, sh(T, 0.7f)); line(HX + 4.5f, 52, HX + 4.5f, 94, sh(T, 0.7f)); }
        else { line(HX, 52, HX, 94, lit(T, 0.45f)); ell(HX, 51, 3.f, 1.6f, T2, false); }
        line(HX - 12, 93, HX + 12, 93, sh(T, 0.75f));
        if (!open) {
            Px st = lit(T, 0.35f);
            const float SP[9][2] = {{-9, 60}, {5, 58}, {-4, 70}, {8, 76}, {-10, 84}, {2, 86}, {-shW - 4, 70}, {shW + 4, 64}, {shW + 4.5f, 90}};
            for (auto& q : SP) { int x = (int)(HX + q[0]), y = (int)q[1]; dot(x, y, st); dot(x - 1, y, st); dot(x + 1, y, st); dot(x, y - 1, st); dot(x, y + 1, st); }
        }
        break;
    }
    case T_FURCOAT: {   // long black coat with a big fluffy faux-fur collar and cuffs
        Px F = lit(T, 0.08f);
        poly({HX - 7, 55, HX + 7, 55, HX + 7, 72, HX - 7, 72}, sh(T, 0.7f));   // black crop underneath
        poly({HX - shW - 2.5f, 50, HX - 3, 50, HX - 4, 80, HX - 6, 150, HX - shW - 6, 150, HX - shW - 3, 80}, T);
        poly({HX + shW + 2.5f, 50, HX + 3, 50, HX + 4, 80, HX + 6, 150, HX + shW + 6, 150, HX + shW + 3, 80}, T);
        for (int s = -1; s <= 1; s += 2) { limb(HX + s * (shW + 2), 54, 5.f, HX + s * (shW + 4.f), 80, 4.6f, T); limb(HX + s * (shW + 4.f), 80, 4.6f, HX + s * (shW + 4.5f), 96, 4.2f, T); }
        for (int i = 0; i < 14; i++) { float a = i / 13.f; for (int s = -1; s <= 1; s += 2) ell(HX + s * (11 - a * 7), 50 + a * 24, 3.6f, 3.2f, (i & 1) ? F : sh(F, 0.8f)); }
        for (int s = -1; s <= 1; s += 2) for (int i = 0; i < 4; i++) ell(HX + s * (shW + 4.f) + (i - 1.5f), 94 + (i & 1), 3.f, 3.f, (i & 1) ? F : sh(F, 0.8f));
        line(HX - shW - 6, 149, HX - 6, 149, lit(T, 0.25f)); line(HX + 6, 149, HX + shW + 6, 149, lit(T, 0.25f));
        break;
    }
    case T_STRIPEPOLO:   // boxy striped polo
        poly({HX - shW - 2, 51, HX + shW + 2, 51, HX + shW + 1, 70, HX + 11, 90, HX - 11, 90, HX - shW - 1, 70}, T);
        for (int s = -1; s <= 1; s += 2) limb(HX + s * (shW + 1.5f), 54, 5.f, HX + s * (shW + 4.f), 68, 4.5f, T);
        pattern(50, HX - 20, 56, HX + 20, 92, [&](float, float y, Px o, Px& n) { int b = ((int)y - 56) / 6; if (b % 2 == 1) { n = mixp(o, T2, 0.85f); return true; } return false; });
        poly({HX - 6, 50, HX - 1, 50, HX, 58, HX - 2, 55}, sh(T, 0.6f)); poly({HX + 6, 50, HX + 1, 50, HX, 58, HX + 2, 55}, sh(T, 0.6f));
        ell(HX, 53, 1.8f, 3.f, rgb(L.skin), false);
        break;
    case T_SKULLTEE: case T_CROPTEE: {   // fitted cropped baby tee (plain, or with a little skull)
        poly({HX - shW - 1, 51, HX + shW + 1, 51, HX + shW, 62, HX + 7.5f, 76, HX - 7.5f, 76}, T);
        for (int s = -1; s <= 1; s += 2) limb(HX + s * (shW + 1), 54, 4.2f, HX + s * (shW + 2.5f), 62, 3.8f, T);
        ell(HX, 52, 3.5f, 2.f, rgb(L.skin), false);
        if (L.top == T_SKULLTEE) {
            Px W = T2, K = sh(T, 0.6f);
            ell(HX + 1, 63, 4.f, 3.4f, W, false); poly({HX - 1.5f, 65, HX + 3.5f, 65, HX + 3, 69, HX - 1, 69}, W, false);
            dot((int)HX - 1, 63, K); dot((int)HX + 2, 63, K); dot((int)HX, 66, K); dot((int)HX + 1, 66, K);
        }
        break;
    }
    case T_BODYCON: {   // ankle-length strappy floral bodycon dress
        poly({HX - 7.5f, 54, HX + 7.5f, 54, HX + 8, 66, HX + 6.5f, 80, HX + 10, 96, HX + 9, 130, HX + 8, 172, HX - 8, 172, HX - 9, 130, HX - 10, 96, HX - 6.5f, 80, HX - 8, 66}, T);
        line(HX - 6, 54, HX - 6, 46, sh(T, 0.7f)); line(HX + 6, 54, HX + 6, 46, sh(T, 0.7f));
        pattern(50, HX - 12, 54, HX + 12, 174, [&](float x, float y, Px o, Px& n) { float v = std::sin(x * 0.9f + std::sin(y * 0.3f) * 2.f) * std::cos(y * 0.42f + x * 0.2f); if (v > 0.35f) { n = mixp(o, T2, 0.75f); return true; } return false; });
        line(HX + 1, 100, HX + 2, 170, sh(T, 0.85f)); line(HX - 7, 58, HX + 7, 58, lit(T, 0.15f));
        break;
    }
    case T_BABYDOLL: {   // butter-yellow babydoll mini dress with puff sleeves and a ruffled hem
        poly({HX - 8.5f, 54, HX + 8.5f, 54, HX + 7.5f, 70, HX - 7.5f, 70}, T);
        for (int s = -1; s <= 1; s += 2) ell(HX + s * (shW + 1.5f), 56, 4.2f, 3.6f, T);
        poly({HX - 7.5f, 69, HX + 7.5f, 69, HX + 15, 108, HX - 15, 108}, T);
        for (int i = 0; i < 10; i++) { float x = HX - 15 + i * 3.3f; ell(x + 1.6f, 109, 2.2f, 2.6f, T2); line(x + 1, 74, x + 1.6f, 106, sh(T, 0.88f)); }
        line(HX - 9, 70, HX + 9, 70, T2); line(HX - 13, 96, HX + 13, 96, T2);
        ell(HX, 57, 1.4f, 1.2f, T2, false); line(HX - 7, 55, HX + 7, 55, T2);
        break;
    }
    case T_DENIMVEST: {   // denim vest over a white tee
        poly({HX - shW - 1, 51, HX + shW + 1, 51, HX + shW, 62, HX + 7.5f, 78, HX - 7.5f, 78, HX - shW, 62}, T2);
        for (int s2 = -1; s2 <= 1; s2 += 2) limb(HX + s2 * (shW + 1), 54, 4.2f, HX + s2 * (shW + 2.5f), 62, 3.8f, T2);
        ell(HX, 52, 3.f, 1.8f, rgb(L.skin), false);
        for (int s2 = -1; s2 <= 1; s2 += 2) poly({HX + s2 * 2.5f, 52, HX + s2 * (shW + 0.5f), 52, HX + s2 * (shW - 0.5f), 62, HX + s2 * 8, 78, HX + s2 * 2.5f, 80}, T);
        pattern(50, HX - 14, 50, HX + 14, 82, [&](float x, float y, Px o, Px& n) { if (((int)(x * 3 + y * 7)) % 5 == 0 && (o.r != T2.r || o.g != T2.g)) { n = lit(o, 0.16f); return true; } return false; });
        for (int y = 58; y <= 76; y += 6) { dot((int)HX - 3, y, rgb(230, 230, 236)); dot((int)HX + 3, y, rgb(230, 230, 236)); }
        line(HX - 9, 62, HX - 4, 62, sh(T, 0.7f)); line(HX + 4, 62, HX + 9, 62, sh(T, 0.7f));
        break;
    }
    case T_JERSEYTANK:   // football tank with contrast trim and a little crest
        poly({HX - 8.5f, 52, HX - 4, 52, HX, 56, HX + 4, 52, HX + 8.5f, 52, HX + 9, 66, HX + 7.5f, 80, HX - 7.5f, 80, HX - 9, 66}, T);
        line(HX - 4, 52, HX, 56, T2); line(HX + 4, 52, HX, 56, T2); line(HX - 9, 56, HX - 8, 64, T2); line(HX + 9, 56, HX + 8, 64, T2);
        poly({HX + 2, 64, HX + 5, 62, HX + 8, 64, HX + 5, 67}, T2); ell(HX + 5, 64.5f, 1.f, 0.8f, rgb(60, 90, 200), false);
        for (int i = 0; i < 5; i++) dot((int)HX - 6 + i, 64, T2);
        break;
    case T_FACETEE: case T_BOWTEE: {   // fitted baby tee: a cute face graphic, or a scoop neck with a white bow
        poly({HX - shW - 1, 51, HX + shW + 1, 51, HX + shW, 62, HX + 7.5f, 76, HX - 7.5f, 76, HX - shW, 62}, T);
        for (int s2 = -1; s2 <= 1; s2 += 2) limb(HX + s2 * (shW + 1), 54, 4.2f, HX + s2 * (shW + 2.5f), 62, 3.8f, T);
        if (L.top == T_FACETEE) {
            ell(HX, 52, 3.5f, 2.f, rgb(L.skin), false);
            ell(HX, 64, 4.5f, 4.f, rgb(246, 214, 176)); ell(HX, 61, 4.6f, 2.f, rgb(150, 100, 70));
            dot((int)HX - 2, 64, rgb(40, 30, 30)); dot((int)HX + 2, 64, rgb(40, 30, 30)); dot((int)HX - 3, 66, rgb(250, 160, 170)); dot((int)HX + 3, 66, rgb(250, 160, 170));
            for (int i = 0; i < 4; i++) dot((int)HX - 8 + i * 5, 58 + (i % 2) * 12, lit(T, 0.5f));
        } else {
            ell(HX, 53, 5.5f, 3.f, rgb(L.skin), false);
            line(HX - 5, 56, HX + 5, 56, T2); poly({HX - 3, 56, HX + 3, 56, HX + 2.5f, 59, HX - 2.5f, 59}, T2);
            poly({HX - 2.5f, 57.5f, HX, 58.5f, HX - 2.5f, 60}, rgb(250, 250, 250), false); poly({HX + 2.5f, 57.5f, HX, 58.5f, HX + 2.5f, 60}, rgb(250, 250, 250), false);
            for (int x = -7; x <= 7; x += 2) dot((int)HX + x, 75, lit(T, 0.25f));
        }
        break;
    }
    case T_STRAPLESS: {   // bandeau + a sheer, floaty babydoll layer
        poly({HX - 8.5f, 56, HX + 8.5f, 56, HX + 8.5f, 61, HX - 8.5f, 61}, T);
        poly({HX - 8.5f, 61, HX + 8.5f, 61, HX + 12, 88, HX - 12, 88}, alpha(T, 120), false);
        for (int i = 0; i < 7; i++) line(HX - 6 + i * 2, 62, HX - 9 + i * 3, 87, alpha(sh(T, 0.85f), 120));
        line(HX - 8.5f, 57, HX + 8.5f, 57, lit(T, 0.3f));
        break;
    }
    case T_TURTLENECK:   // fitted long-sleeve turtleneck
        poly({HX - shW - 1, 50, HX + shW + 1, 50, HX + shW, 62, HX + 7.5f, 80, HX - 7.5f, 80, HX - shW, 62}, T);
        poly({HX - 3.4f, 43, HX + 3.4f, 43, HX + 3.8f, 51, HX - 3.8f, 51}, T);
        for (int k = 0; k < 3; k++) line(HX - 3.4f, 45 + k * 2, HX + 3.4f, 45 + k * 2, sh(T, 0.88f));
        for (int s2 = -1; s2 <= 1; s2 += 2) { limb(HX + s2 * (shW + 1), 54, 3.6f, HX + s2 * (shW + 3.f), 80, 2.9f, T); limb(HX + s2 * (shW + 3.f), 80, 2.9f, HX + s2 * (shW + 3.5f), 98, 2.4f, T); }
        break;
    case T_PINCARDIGAN: {   // pinstripe cardigan with faux-fur cuffs, over a cream top
        poly({HX - 7, 54, HX + 7, 54, HX + 7, 78, HX - 7, 78}, rgb(248, 242, 230));
        for (int s2 = -1; s2 <= 1; s2 += 2) poly({HX + s2 * 1.5f, 51, HX + s2 * (shW + 1.5f), 51, HX + s2 * (shW + 0.5f), 62, HX + s2 * 8.5f, 82, HX + s2 * 2.5f, 82, HX + s2 * 3.5f, 64}, T);
        for (int s2 = -1; s2 <= 1; s2 += 2) { limb(HX + s2 * (shW + 1), 54, 4.f, HX + s2 * (shW + 3.f), 80, 3.2f, T); limb(HX + s2 * (shW + 3.f), 80, 3.2f, HX + s2 * (shW + 3.5f), 96, 2.8f, T); }
        pattern(50, HX - 18, 50, HX + 18, 100, [&](float x, float, Px o, Px& n) { if (((int)std::floor(x)) % 2 == 0 && o.r < 230) { n = mixp(o, T2, 0.55f); return true; } return false; });
        for (int s2 = -1; s2 <= 1; s2 += 2) for (int k = 0; k < 4; k++) ell(HX + s2 * (shW + 3.5f) + (k - 1.5f) * 1.2f, 95 + (k % 2), 2.4f, 2.4f, k % 2 ? rgb(246, 238, 226) : rgb(226, 214, 198));
        break;
    }
    case T_GINGHAMTUBE:   // gingham tube top with a little white ruffle
        poly({HX - 9, 56, HX + 9, 56, HX + 8.5f, 74, HX - 8.5f, 74}, T);
        pattern(50, HX - 10, 54, HX + 10, 76, [&](float x, float y, Px o, Px& n) { bool a = ((int)std::floor(x) / 2) % 2, b = ((int)std::floor(y) / 2) % 2; if (a || b) { n = (a && b) ? sh(o, 0.82f) : mixp(o, rgb(255, 255, 255), 0.45f); return true; } return false; });
        for (int x = -9; x <= 9; x += 2) ell(HX + x, 56, 1.3f, 1.f, rgb(252, 250, 248), false);
        break;
    case T_WEBTOP: {   // red web-print top under a cropped red shrug
        poly({HX - 8, 55, HX + 8, 55, HX + 8, 66, HX + 6, 80, HX - 6, 80, HX - 8, 66}, T);
        for (int k = 0; k < 5; k++) { float a = 0.6f + k * 0.48f; line(HX, 64, HX + std::cos(a) * 10, 64 + std::sin(a) * 14, rgb(30, 20, 24)); line(HX, 64, HX - std::cos(a) * 10, 64 - std::sin(a) * 9, rgb(30, 20, 24)); }
        for (int r = 1; r <= 3; r++) for (int k = 0; k < 12; k++) { float a = k * 0.5236f; dot((int)(HX + std::cos(a) * r * 3), (int)(64 + std::sin(a) * r * 3), rgb(30, 20, 24)); }
        for (int s2 = -1; s2 <= 1; s2 += 2) { poly({HX + s2 * 4, 51, HX + s2 * (shW + 1.5f), 51, HX + s2 * (shW + 0.5f), 62, HX + s2 * 8, 66}, sh(T, 0.85f)); limb(HX + s2 * (shW + 1), 54, 4.f, HX + s2 * (shW + 2.8f), 76, 3.2f, sh(T, 0.85f)); }
        break;
    }
    case T_TUBE:
        poly({HX - 9, 56, HX + 9, 56, HX + 8, 72, HX - 8, 72}, T); for (int y = 58; y < 72; y += 3) line(HX - 8, y, HX + 8, y, sh(T, 0.92f));
        break;
    }
}
void drawExtras(const Look& L) {
    Px G = rgb(224, 184, 104), Gd = rgb(170, 120, 50);
    g_part = 60;
    if (L.extras & X_FISHNET) for (int y = 108; y < 172; y++) for (int x = 36; x < 60; x++) if (g_final.pt(x, y) == 20 && g_final.at(x, y).a && ((x + y) % 4 == 0 || (x - y + 400) % 4 == 0)) plot(x, y, rgb(30, 26, 34, 200));
    if (L.extras & X_SOCKS) for (int s = -1; s <= 1; s += 2) { limb(HX + s * 4.5f, 156, 3.2f, HX + s * 4.5f, 174, 3.f, rgb(248, 248, 248)); line(HX + s * 4.5f - 2, 158, HX + s * 4.5f + 2, 158, rgb(220, 220, 224)); }
    if (L.extras & X_CHAINBELT) { for (int x = -10; x <= 10; x++) plot((int)HX + x, 86 + (std::abs(x) % 3 == 0), (x & 1) ? G : Gd); if (L.bottom == B_JORTS) for (int y = 88; y < 112; y += 3) ell(HX - 6, y, 1.5f, 1.4f, G, false); }
    if (L.extras & X_NECK) { for (int x = -5; x <= 4; x++) plot(48 + x, 52 + (x * x + x) / 8, x & 1 ? G : Gd); plot(47, 56, G); plot(48, 56, G); plot(47, 57, Gd); plot(48, 57, Gd); }
    if (L.extras & X_CHOKER) line(HX - 3, 46, HX + 3, 46, rgb(30, 24, 30));
    if (L.extras & X_HOOPS) for (int s = -1; s <= 1; s += 2) { int x = (int)HX + s * 12; plot(x, (int)HY + 6, G); plot(x - 1, (int)HY + 7, G); plot(x + 1, (int)HY + 7, G); plot(x - 1, (int)HY + 8, G); plot(x + 1, (int)HY + 8, G); plot(x, (int)HY + 9, G); }
    if (L.extras & X_STUDS) for (int s = -1; s <= 1; s += 2) plot((int)HX + s * 12, (int)HY + 5, rgb(240, 240, 255));
    if (L.extras & X_BANGLES) for (int s = -1; s <= 1; s += 2) { float x = HX + s * (L.guy ? 16 : 13.5f); line(x - 2, 97, x + 2, 97, G); line(x - 2, 99, x + 2, 99, Gd); }
    if (L.extras & X_CUFFS) for (int i = 0; i < 6; i++) plot((int)HX - 10 + (i % 2) * 20, (int)HY + 26 + i * 3, G);
    if (L.extras & X_FLOWER) { hibiscus(HX - 10, HY - 12, 6, rgb(255, 170, 200)); ell(HX - 10, HY - 12, 1.5f, 1.5f, rgb(255, 240, 120), false); }
    if (L.extras & X_CAP) {   // baseball cap (accC), brim to the front
        Px cc = (L.accC.r | L.accC.g | L.accC.b) ? rgb(L.accC) : rgb(40, 160, 170), cd = sh(cc, 0.75f), cl = lit(cc, 0.35f);
        ell(HX, HY - 9, 14.5f, 9.5f, cc, false);
        poly({HX - 15, HY - 8, HX + 15, HY - 8, HX + 15, HY - 5, HX - 15, HY - 5}, cc, false);
        poly({HX - 14, HY - 6, HX + 14, HY - 6, HX + 18, HY - 2, HX - 18, HY - 2}, cd, false);
        line(HX - 9, HY - 15, HX + 6, HY - 17, cl); ell(HX, HY - 11, 3.f, 2.2f, rgb(245, 245, 245), false); dot((int)HX, (int)HY - 18, cd);
    }
    if (L.extras & X_VISOR) { poly({HX - 12, HY - 12, HX + 12, HY - 12, HX + 11, HY - 9, HX - 11, HY - 9}, rgb(200, 190, 160)); line(HX - 10, HY - 11, HX + 10, HY - 11, lit(rgb(200, 190, 160), 0.6f)); }
    if (L.extras & X_TINTS) for (int s = -1; s <= 1; s += 2) { int cx = (int)HX + s * 5; for (int y = -1; y <= 2; y++) for (int x = -3; x <= 2; x++) plot(cx + x, (int)HY + 1 + y, rgb(255, 150, 190, 130)); line(cx - 3, HY - 1, cx + 2, HY - 1, rgb(60, 40, 50)); }
    if (L.extras & X_HEADPH) { Px hp = (L.extras & X_WHITEPH) ? rgb(240, 240, 246) : rgb(30, 30, 36); for (int a = 0; a <= 20; a++) { float t = 3.14159f * a / 20; plot((int)(HX - std::cos(t) * 13), (int)(HY - 4 - std::sin(t) * 13), hp); } ell(HX - 12, HY + 2, 3, 4, hp, false); ell(HX + 12, HY + 2, 3, 4, hp, false); }
    if (L.extras & X_BAG) {   // little quilted shoulder bag on the right hand
        float bx = HX + (L.guy ? 16 : 15.f), by = 112;
        line(bx - 3, by, bx, by - 8, G); line(bx + 3, by, bx, by - 8, G);
        if (L.extras & X_ZEBRA) { poly({bx - 6, by, bx + 7, by, bx + 6, by + 8, bx - 5, by + 8}, rgb(236, 230, 220)); for (int x = -5; x <= 6; x += 3) line(bx + x, by + 1, bx + x + 1.5f, by + 7, rgb(36, 30, 30)); }
        else {
        poly({bx - 6, by, bx + 7, by, bx + 7, by + 9, bx - 6, by + 9}, rgb(L.extras & X_PHONE ? 240 : 120, L.extras & X_PHONE ? 150 : 80, L.extras & X_PHONE ? 190 : 50));
        for (int x = -4; x <= 5; x += 3) for (int y = 2; y <= 7; y += 3) plot((int)bx + x, (int)by + y, rgb(210, 170, 100));
        }
        line(bx - 6, by, bx + 7, by, G);
    }
    if (L.extras & X_STARNECK) { for (int x = -5; x <= 4; x++) plot(48 + x, 53 + (x * x + x) / 6, rgb(240, 230, 210)); Px st = rgb(236, 196, 120); plot(47, 59, st); plot(48, 59, st); plot(46, 60, st); plot(47, 60, st); plot(48, 60, st); plot(49, 60, st); plot(47, 61, st); plot(48, 61, st); plot(46, 62, st); plot(49, 62, st); }
    if (L.extras & X_PEACE) { for (int x = -5; x <= 4; x++) plot(48 + x, 53 + (x * x + x) / 5, G); for (int y = 0; y < 4; y++) { plot(46, 61 + y, Gd); plot(49, 61 + y, Gd); } plot(47, 60, Gd); plot(48, 60, Gd); plot(47, 65, Gd); plot(48, 65, Gd); plot(47, 62, Gd); plot(48, 62, Gd); }
    if (L.extras & X_BEADS) for (int s = -1; s <= 1; s += 2) { float x = HX + s * 13.5f; for (int k = -2; k <= 2; k++) { plot((int)x + k, 96, (k & 1) ? rgb(64, 200, 200) : rgb(250, 160, 190)); plot((int)x + k, 98, (k & 1) ? rgb(250, 250, 250) : rgb(64, 200, 200)); } }
    if (L.extras & X_NAVEL) { plot(48, 80, rgb(240, 240, 255)); plot(48, 82, rgb(200, 120, 220)); }
    if (L.extras & X_ANKLET) for (int k = -2; k <= 2; k++) plot((int)HX - 5 + k, 170, (k & 1) ? rgb(64, 200, 200) : rgb(240, 100, 120));
    if (L.extras & X_PEARLBELLY) for (int r = 0; r < 3; r++) for (int x = -9 + r; x <= 9 - r; x += 2) plot(48 + x, 82 + r * 2 + (x * x) / 30, rgb(250, 248, 240));
    if (L.extras & X_RINGBELT) for (int x = -10; x <= 10; x += 3) { plot(48 + x, 86, G); plot(48 + x + 1, 86, Gd); plot(48 + x, 87, Gd); plot(48 + x + 1, 87, G); }
    if (L.extras & X_LIPRING) { plot(44, (int)HY + 11, rgb(230, 230, 240)); plot(51, (int)HY + 11, rgb(230, 230, 240)); }
    if (L.extras & X_NAILS) for (int s = -1; s <= 1; s += 2) { float x = HX + s * 13.3f; for (int k = -1; k <= 1; k++) plot((int)x + k, 106, k ? rgb(120, 200, 250) : rgb(250, 150, 190)); }
    if (L.extras & X_STARHOOPS) for (int s = -1; s <= 1; s += 2) { int x = (int)HX + s * 12; Px W = rgb(250, 250, 255); plot(x, (int)HY + 5, W); for (int k = 0; k < 3; k++) { plot(x - 2 + k * 2, (int)HY + 8, W); } plot(x - 1, (int)HY + 7, W); plot(x + 1, (int)HY + 7, W); plot(x, (int)HY + 6, W); plot(x - 2, (int)HY + 10, W); plot(x + 2, (int)HY + 10, W); plot(x, (int)HY + 9, W); }
    if (L.extras & X_STARCLIP) { Px P = rgb(250, 160, 180), Pd = rgb(220, 110, 140); int cx = 38, cy = (int)HY - 10; for (int k = -3; k <= 3; k++) { plot(cx + k, cy, P); plot(cx, cy + k, k > 0 ? Pd : P); } plot(cx - 2, cy + 2, Pd); plot(cx + 2, cy + 2, Pd); plot(cx - 1, cy - 1, P); plot(cx + 1, cy - 1, P); }
    if (L.extras & X_FLOWERCLIPS) for (int k = 0; k < 2; k++) { int cx = 58 + k * 4, cy = (int)HY - 12 + k * 6; Px W = rgb(252, 240, 230); for (int d = -2; d <= 2; d++) { plot(cx + d, cy, W); plot(cx, cy + d, W); } plot(cx, cy, k ? rgb(150, 210, 250) : rgb(255, 220, 90)); }
    if (L.extras & X_HAIRCLIP) for (int y = 0; y < 5; y++) plot(56, (int)HY - 10 + y, y % 2 ? rgb(64, 200, 200) : rgb(250, 250, 250));
    if (L.extras & X_CROSS) { for (int x = -4; x <= 3; x++) plot(48 + x, 52 + (x * x + x) / 4, rgb(200, 200, 210)); for (int y = 0; y < 4; y++) plot(47, 57 + y, rgb(220, 220, 230)); plot(46, 58, rgb(220, 220, 230)); plot(48, 58, rgb(220, 220, 230)); }
    if (L.extras & X_HEART) { for (int x = -4; x <= 3; x++) plot(48 + x, 52 + (x * x + x) / 5, G); Px h = rgb(236, 196, 120); plot(46, 57, h); plot(47, 57, h); plot(48, 57, h); plot(49, 57, h); plot(47, 58, h); plot(48, 58, h); plot(46, 56, h); plot(49, 56, h); }
    if (L.extras & X_BUCKLE) { for (int x = -11; x <= 11; x++) { plot(48 + x, 87, rgb(120, 76, 50)); plot(48 + x, 88, rgb(96, 60, 40)); } for (int y = -2; y <= 2; y++) for (int x = -2; x <= 2; x++) if (x * x + y * y <= 5) plot(46 + x, 87 + y, (x * x + y * y <= 1) ? rgb(250, 220, 140) : G); }
    if (L.extras & X_STUDBELT) for (int x = -11; x <= 11; x++) { plot(48 + x, 87, rgb(30, 26, 30)); plot(48 + x, 88, (x % 3 == 0) ? rgb(220, 220, 230) : rgb(30, 26, 30)); }
    {   // facial hair
        int bd = L.beard ? L.beard : (L.extras & X_STUBBLE) ? 1 : 0;
        const int hy = (int)HY - (bd == 3 || bd == 5 || bd == 7 || bd == 8 ? 0 : 2);
        Px B = sh(rgb(L.hair), 0.8f), Bs = alpha(B, 110);
        auto mustache = [&](int w, bool curl) { for (int x = 48 - w; x <= 47 + w; x++) { plot(x, hy + 9, B); if (std::abs(x - 47.5f) < w - 1) plot(x, hy + 8, alpha(B, 200)); } if (curl) { plot(48 - w - 1, hy + 8, B); plot(47 + w + 1, hy + 8, B); plot(48 - w - 1, hy + 7, alpha(B, 180)); plot(47 + w + 1, hy + 7, alpha(B, 180)); } };
        auto jaw = [&](int top, int bottom, bool chin, bool thick) {
            for (int y = top; y <= bottom; y++) {
                int k = y - (hy + 10); int l = 41 + (k > 0 ? k : 0), r = 54 - (k > 0 ? k : 0);
                plot(l, y, B); plot(r, y, B); if (thick) { plot(l + 1, y, alpha(B, 210)); plot(r - 1, y, alpha(B, 210)); }
            }
            if (chin) for (int x = 44; x <= 51; x++) { plot(x, bottom, B); if (thick) plot(x, bottom - 1, alpha(B, 220)); }
        };
        switch (bd) {
        case 1: for (int x = -5; x <= 5; x++) for (int y = 0; y < 4; y++) if ((x + y) & 1) plot(48 + x, hy + 10 + y + (std::abs(x) > 3 ? -2 : 0), alpha(B, 120)); break;   // stubble
        case 2: mustache(3, false); for (int y = hy + 13; y <= hy + 15; y++) for (int x = 46; x <= 49; x++) plot(x, y, B); break;                                  // goatee
        case 3: mustache(4, false); jaw(hy + 2, hy + 16, true, true); for (int y = hy + 13; y <= hy + 15; y++) for (int x = 43; x <= 52; x++) plot(x, y, B); break;   // full
        case 4: mustache(4, false); break;                                                                                                                     // mustache
        case 5: jaw(hy + 2, hy + 15, true, false); break;                                                                                                       // chin strap
        case 6: plot(47, hy + 13, B); plot(48, hy + 13, B); plot(47, hy + 14, Bs); plot(48, hy + 14, Bs); break;                                                // soul patch
        case 7: mustache(4, false); jaw(hy + 5, hy + 14, true, true); for (int x = 44; x <= 51; x++) plot(x, hy + 13, B); break;                             // boxed
        case 8: for (int y = hy + 2; y <= hy + 12; y++) { int w = y > hy + 6 ? 3 : 2; for (int i = 0; i < w; i++) { plot(41 + i, y, B); plot(54 - i, y, B); } } break;   // mutton chops
        case 9: mustache(4, true); break;                                                                                                                      // handlebar
        default: break;
        }
    }
    if (L.extras & X_CAMERA) { for (int y = 52; y <= 66; y++) plot(48 - (y - 52) / 3, y, rgb(30, 30, 34)); for (int y = 52; y <= 66; y++) plot(48 + (y - 52) / 3, y, rgb(30, 30, 34)); poly({HX - 6, 64, HX + 6, 64, HX + 6, 72, HX - 6, 72}, rgb(40, 40, 46)); ell(HX, 68, 3, 3, rgb(20, 20, 24), false); ell(HX, 68, 1.5f, 1.5f, rgb(120, 140, 170), false); }
    if (L.extras & X_TOTE) { float bx = HX + 13, by = 84; line(bx - 3, by, bx - 2, 54, rgb(240, 236, 240)); poly({bx - 5, by, bx + 6, by, bx + 7, by + 16, bx - 5, by + 16}, rgb(248, 244, 248)); }
    if (L.extras & X_HIGHLIGHT) for (int y = 0; y < CH; y++) for (int x = 0; x < CW; x++) { uint8_t id = g_final.pt(x, y); if ((id == 10 || id == 11) && g_final.at(x, y).a && (x % 4 == 1) && y > HY - 6) { Px& o = g_final.at(x, y); o = mixp(o, rgb(176, 112, 60), 0.55f); } }
    if (L.extras & X_WRISTBAND) for (int s = -1; s <= 1; s += 2) { float x = HX + s * (L.guy ? 16 : 13.5f); for (int y = 94; y <= 98; y++) line(x - 2.5f, y, x + 2.5f, y, rgb(26, 24, 28)); for (int k = -1; k <= 1; k++) { plot((int)x + k * 2, 95, rgb(220, 220, 230)); plot((int)x + k * 2 - 1, 97, rgb(220, 220, 230)); } }
    if (L.extras & X_FRECKLES) { const int FR[7][2] = {{39, 33}, {41, 34}, {43, 33}, {40, 35}, {54, 33}, {56, 34}, {52, 34}}; for (auto& q : FR) plot(q[0], q[1], alpha(rgb(150, 90, 70), 150)); }
    if (L.extras & X_NOSERING) plot(50, (int)HY + 6, rgb(235, 235, 245));
    if (L.extras & X_SHADES) for (int s = -1; s <= 1; s += 2) { int cx = s < 0 ? 42 : 53, ey = (int)HY + 2; for (int y = -2; y <= 2; y++) for (int x = -4; x <= 4; x++) if (std::abs(x) + (y > 0 ? y : 0) < 5) plot(cx + x, ey + y, (y == -2 && x < 0) ? rgb(120, 110, 130) : rgb(20, 18, 24)); plot(47, (int)HY + 1, rgb(20, 18, 24)); plot(48, (int)HY + 1, rgb(20, 18, 24)); }
    if (L.extras & X_GLASSES) {
        Px Fr = rgb(120, 70, 50);
        for (int s = -1; s <= 1; s += 2) { int cx = s < 0 ? 42 : 53, ey = (int)HY + 2;
            for (int x = -3; x <= 3; x++) { plot(cx + x, ey - 3, Fr); plot(cx + x, ey + 3, Fr); }
            for (int y = -2; y <= 2; y++) { plot(cx - 4, ey + y, Fr); plot(cx + 4, ey + y, Fr); }
            plot(cx + s * 5, ey - 2, Fr); plot(cx + s * 6, ey - 2, Fr); }
        plot(47, (int)HY, Fr); plot(48, (int)HY, Fr);
    }
    if (L.extras & X_HEARTHAIR) { Px R = rgb(226, 70, 96); const int HH[4][2] = {{40, 19}, {48, 16}, {55, 19}, {44, 22}}; for (auto& q : HH) { int x = q[0], y = q[1]; plot(x - 1, y, R); plot(x + 1, y, R); plot(x - 1, y + 1, R); plot(x, y + 1, R); plot(x + 1, y + 1, R); plot(x, y + 2, R); } }
    Px AC = (L.accC.r | L.accC.g | L.accC.b) ? rgb(L.accC) : rgb(248, 246, 244);
    if (L.extras & X_TIGHTS) for (int y = 100; y < 174; y++) for (int x = 36; x < 60; x++) if (g_final.pt(x, y) == 20 && g_final.at(x, y).a) { Px& o = g_final.at(x, y); o = mixp(o, rgb(150, 126, 112), 0.45f); if (((x / 4) + (y / 5)) % 3 == 0 && (y % 5) < 3 && (x % 4) < 3) o = mixp(o, rgb(100, 80, 70), 0.5f); }
    if (L.extras & X_LEGWARM) for (int s2 = -1; s2 <= 1; s2 += 2) { float ax = HX + s2 * 4.4f; for (int k = 0; k < 7; k++) { float y = 146 + k * 4.f; ell(ax + ((k & 1) ? 0.8f : -0.8f), y, 5.2f + (k % 3) * 0.4f, 2.8f, (k & 1) ? AC : sh(AC, 0.86f), false); } for (int k = 0; k < 6; k++) plot((int)ax - 4 + k * 2, 144 + (k % 2), lit(AC, 0.3f)); }
    if (L.extras & X_SILVERCHAIN) for (int r = 0; r < 2; r++) for (int x = -11; x <= 11; x++) plot(48 + x, 89 + r * 3 + (std::abs(x) % 3 == 0), (x & 1) ? rgb(230, 232, 240) : rgb(160, 164, 176));
    if (L.extras & X_CHAINLACE) for (int y = 92; y < 148; y += 2) { plot((int)HX - 9 + ((y / 2) % 2), y, G); plot((int)HX - 9 + ((y / 2 + 1) % 2), y + 1, Gd); }
    if (L.extras & X_STARSKIRT) for (int x = -9; x <= 9; x += 4) { int cx = 48 + x, cy = 92; Px W = rgb(250, 250, 255); plot(cx, cy - 1, W); plot(cx - 1, cy, W); plot(cx, cy, W); plot(cx + 1, cy, W); plot(cx - 1, cy + 1, W); plot(cx + 1, cy + 1, W); }
    if (L.extras & X_GLOVES) for (int s2 = -1; s2 <= 1; s2 += 2) { float sx = HX + s2 * ((L.guy ? 12.f : 9.5f) + 1.f); limb(sx + s2 * 2.2f, 76, 2.6f, sx + s2 * 2.7f, 98, 2.0f, rgb(26, 22, 26)); ell(sx + s2 * 2.9f, 102, 2.4f, 4.2f, rgb(26, 22, 26)); line(sx + s2 * 2.2f - 1, 78, sx + s2 * 2.7f - 1, 96, rgb(80, 74, 84)); }
    if (L.extras & X_COFFEE) { float cx = HX + 14, cy = 92; poly({cx - 3.5f, cy, cx + 3.5f, cy, cx + 3, cy + 10, cx - 3, cy + 10}, rgb(230, 236, 240, 255)); poly({cx - 3, cy + 3, cx + 3, cy + 3, cx + 2.6f, cy + 10, cx - 2.6f, cy + 10}, rgb(176, 128, 96)); line(cx - 4, cy, cx + 4, cy, rgb(250, 250, 250)); line(cx + 1, cy - 4, cx, cy, rgb(240, 140, 170)); }
    if (L.extras & X_BIGHOOPS) for (int s2 = -1; s2 <= 1; s2 += 2) { float cx = HX + s2 * 12.5f, cy = HY + 11; for (int k = 0; k < 24; k++) { float a = k * 0.2618f; plot((int)(cx + std::cos(a) * 4.5f), (int)(cy + std::sin(a) * 5.5f), k % 3 ? rgb(226, 228, 236) : rgb(255, 255, 255)); } }
    if (L.extras & X_HEADBAND) for (int k = 0; k <= 20; k++) { float a = 3.14159f * k / 20; int x = (int)(HX - std::cos(a) * 12.5f), y = (int)(HY - 7 - std::sin(a) * 6.5f); plot(x, y, AC); plot(x, y + 1, sh(AC, 0.85f)); }
    if (L.extras & X_GOLDCLIPS) for (int k = 0; k < 2; k++) { int cx = 38 + k * 3, cy = (int)HY - 12 + k; for (int y = 0; y < 5; y++) { plot(cx, cy + y, G); plot(cx + 1, cy + y, (y == 0 || y == 4) ? G : Gd); } }
    if (L.extras & X_WHITEFLOWER) { float fx = HX + 10, fy = HY - 12; for (int k = 0; k < 6; k++) { float a = k * 1.0472f; poly({fx, fy, fx + std::cos(a - 0.3f) * 3, fy + std::sin(a - 0.3f) * 3, fx + std::cos(a) * 6, fy + std::sin(a) * 6, fx + std::cos(a + 0.3f) * 3, fy + std::sin(a + 0.3f) * 3}, k % 2 ? rgb(252, 250, 246) : rgb(236, 232, 226), false); } ell(fx, fy, 1.4f, 1.4f, rgb(250, 210, 90), false); poly({fx - 5, fy + 3, fx - 1, fy + 2, fx - 6, fy + 6}, rgb(110, 160, 90), false); }
    if (L.extras & X_FURHAT) { Px F = AC; ell(HX, HY - 13, 16, 8, F); poly({HX - 16, HY - 12, HX + 16, HY - 12, HX + 15, HY - 6, HX - 15, HY - 6}, sh(F, 0.92f)); for (int i = 0; i < 40; i++) plot((int)HX - 15 + (i * 7 % 31), (int)HY - 19 + (i * 5 % 12), i % 2 ? lit(F, 0.25f) : sh(F, 0.85f)); poly({HX - 3, HY - 11, HX + 3, HY - 11, HX + 3, HY - 8, HX - 3, HY - 8}, rgb(200, 170, 110), false); }
    if (L.extras & X_NEWSBOY) { Px Cb = rgb(96, 58, 40); ell(HX + 1, HY - 13, 17, 8, Cb); for (int k = 0; k < 5; k++) line(HX - 12 + k * 6, HY - 19, HX - 10 + k * 5, HY - 8, sh(Cb, 0.8f)); poly({HX - 14, HY - 8, HX + 14, HY - 8, HX + 11, HY - 5, HX - 11, HY - 5}, sh(Cb, 0.75f)); ell(HX + 1, HY - 20, 1.5f, 1.2f, sh(Cb, 0.7f), false); }
    if (L.extras & X_PHONE) { poly({HX + 13, 82, HX + 19, 82, HX + 19, 93, HX + 13, 93}, rgb(40, 40, 46)); line(HX + 14, 83, HX + 18, 83, rgb(255, 200, 230)); }
}
bool g_world = false;   // painting the half-size in-world doll: no face/ink at full size
// Chibi 2.0 eyes: big, dark, glossy anime eyes with two sparkles (drawn on top after the warp)
// Doll 2.0 face: almond eyes with winged lashes, soft arched brows, glossy overlined lips, rosy blush
void dollFace(const Look& L) {
    const float hy = chiY(HY + 3.f);
    Px SKIN = g_final.at(39, (int)hy + 4); if (!SKIN.a) SKIN = g_final.at(48, (int)hy - 8); if (!SKIN.a) return;
    const Px E = mixp(rgb(L.eyes), rgb(40, 24, 20), 0.55f), LASH = rgb(20, 12, 14), WH = rgb(250, 246, 246);
    const Px BROW = mixp(rgb(L.hair), rgb(40, 26, 22), 0.55f), LIP = L.guy ? mixp(rgb(L.lips), SKIN, 0.5f) : rgb(L.lips), LIPD = sh(LIP, 0.78f);
    g_part = 121;
    auto P = [&](float x, float y, Px c) { plot((int)std::lround(x - 0.5f), (int)std::lround(y - 0.5f), c); };
    auto near = [&](const Px& p) { return p.a && std::abs(p.r - SKIN.r) + std::abs(p.g - SKIN.g) + std::abs(p.b - SKIN.b) < 150 && !(p.r > SKIN.r + 40); };
    for (int y = -7; y <= 15; y++) for (int x = -11; x <= 11; x++) {   // clear the old eyes, brows & mouth, but never the hair
        if (y > 4 && std::abs(x) > 6) continue;
        int px = 48 + x, py = (int)hy + y; if (px < 0 || py < 0 || px >= CW || py >= CH) continue;
        const Px& p = g_final.at(px, py); if (!p.a) continue;
        const bool hairish = std::abs(p.r - L.hair.r) + std::abs(p.g - L.hair.g) + std::abs(p.b - L.hair.b) < 40 && std::abs(x) > 8;
        if (!hairish && (near(p) || (p.r + p.g + p.b < 200) || p.r > 200)) P(px + 0.5f, py + 0.5f, SKIN);
    }
    for (int s = -1; s <= 1; s += 2) {
        const float cx = 48.f + s * 7.f;
        auto Q = [&](int dx, int dy, Px c) { P(cx + dx * s, hy + dy, c); };
        for (int y = -2; y <= 2; y++) for (int x = -4; x <= 4; x++) {   // almond white
            const float u = x / 4.6f, v = (y - (x > 0 ? -0.3f : 0.2f)) / 2.4f; if (u * u + v * v > 1.f) continue;
            Q(x, y, WH);
        }
        for (int y = -2; y <= 2; y++) for (int x = -2; x <= 2; x++) if (x * x + y * y <= 5) Q(x, y, y < 0 ? sh(E, 0.7f) : E);   // iris
        Q(0, 0, rgb(12, 8, 10)); Q(0, 1, rgb(12, 8, 10)); Q(-1, -1, WH); Q(1, 1, lit(E, 0.5f));
        for (int x = -4; x <= 5; x++) Q(x, x > 2 ? -3 - (x - 2) / 2 : -3, LASH);   // upper lash line + wing
        Q(5, -4, LASH); Q(6, -5, LASH); if (!L.guy) { Q(4, -4, LASH); Q(-3, -4, LASH); Q(0, -4, LASH); Q(2, -4, LASH); }
        for (int x = -3; x <= 3; x++) Q(x, 3, alpha(LASH, 90));
        for (int x = -4; x <= 4; x++) Q(x, -6 - (x > -2 && x < 3 ? 1 : 0), BROW);   // brow arch
        if (!L.guy) for (int y = 0; y <= 2; y++) for (int x = -3; x <= 2; x++) if (x * x + y * y < 7) P(48 + s * 10 + x * s, hy + 6 + y, alpha(rgb(250, 140, 150), 70));   // blush
    }
    P(48, hy + 6, sh(SKIN, 0.86f)); P(49, hy + 7, sh(SKIN, 0.82f)); P(47, hy + 6, lit(SKIN, 0.3f));   // nose
    for (int y = 0; y < 5; y++) for (int x = -5; x <= 5; x++) {   // glossy lips
        const float u = x / 5.2f, v = (y - 2.f) / 2.4f; if (u * u + v * v > 1.f) continue;
        Px c = y < 2 ? LIPD : LIP; if (y == 2) c = sh(LIP, 0.6f);
        P(48 + x, hy + 10 + y, c);
    }
    if (!L.guy) { P(46, hy + 13, rgb(255, 240, 245)); P(47, hy + 13, rgb(255, 220, 230)); P(50, hy + 11, rgb(255, 230, 238)); }
}
void chibiEyes(const Look& L) {
    const float hy = chiY(HY + 3.f);
    Px E = mixp(rgb(L.eyes), rgb(36, 22, 20), 0.68f), LASH = rgb(22, 14, 16), SKIN = g_final.at(48, (int)hy + 9);
    g_part = 121;
    for (int s = -1; s <= 1; s += 2) {
        const float cx = 48.f + s * 8.f;
        auto P = [&](int dx, int dy, Px c) { plot((int)std::lround(cx + dx * s - 0.5f), (int)std::lround(hy + dy - 0.5f), c); };
        // clear the old small eye area with skin, then a big glossy eye
        for (int y = -7; y <= 6; y++) for (int x = -6; x <= 6; x++) if (x * x / 36.f + y * y / 49.f <= 1.f && SKIN.a) P(x, y, SKIN);
        for (int y = -6; y <= 5; y++) for (int x = -5; x <= 5; x++) {
            float dx = x / 5.2f, dy = (y + 0.5f) / 6.0f; if (dx * dx + dy * dy > 1.f) continue;
            Px c = y < -3 ? sh(E, 0.5f) : y > 2 ? lit(E, 0.3f) : E;
            if (dx * dx + dy * dy < 0.22f) c = rgb(14, 8, 10);
            P(x, y, c);
        }
        for (int x = -5; x <= 5; x++) { P(x, -7 + (std::abs(x) >= 4 ? 1 : 0), LASH); P(x, -6 + (std::abs(x) >= 4 ? 1 : 0), LASH); }   // thick lash line
        P(6, -6, LASH); P(6, -5, LASH); P(-6, -5, LASH);
        if (!L.guy) { P(7, -8, LASH); P(6, -7, LASH); P(7, -7, LASH); }
        for (int y = -3; y <= -1; y++) for (int x = -3; x <= -1; x++) P(x, y, rgb(255, 255, 255));   // big sparkle
        P(2, 2, rgb(255, 255, 255)); P(3, 2, rgb(255, 255, 255)); P(2, 3, rgb(240, 236, 240));         // small sparkle
        P(-5, 6, alpha(rgb(240, 150, 150), 90)); P(-4, 7, alpha(rgb(240, 150, 150), 90)); P(5, 7, alpha(rgb(240, 150, 150), 90));   // blush
    }
}
// Mid-size / plus-size: widen the body and its clothes row by row about the centre line, following a
// soft hourglass profile (bust, waist, hips, thighs), on the supersampled canvas so edges stay smooth.
static void bodyWarp(int body) {
    Canvas& c = *C_; const float ss = g_ss;
    std::vector<Px> row(c.W); std::vector<uint8_t> prow(c.W);
    auto prof = [&](float y) {   // y in doll units
        struct K { float y, mid, plus, tea; };
        static const K k[] = {{46, 1.f, 1.f, 1.f}, {52, 1.1f, 1.18f, 1.f}, {62, 1.2f, 1.42f, 0.97f}, {74, 1.2f, 1.5f, 0.8f}, {88, 1.3f, 1.62f, 1.1f}, {106, 1.28f, 1.58f, 1.04f}, {130, 1.16f, 1.36f, 0.96f}, {152, 1.12f, 1.28f, 0.95f}, {176, 1.05f, 1.1f, 1.f}, {190, 1.f, 1.f, 1.f}};
        if (y <= k[0].y) return 1.f;
        for (int i = 1; i < 10; i++) if (y <= k[i].y) { const float t = (y - k[i - 1].y) / (k[i].y - k[i - 1].y), a = body == 1 ? k[i - 1].mid : body == 2 ? k[i - 1].plus : k[i - 1].tea, b = body == 1 ? k[i].mid : body == 2 ? k[i].plus : k[i].tea; return a + (b - a) * (t * t * (3 - 2 * t)); }
        return 1.f;
    };
    const float cx = HX * ss;
    for (int y = 0; y < c.H; y++) {
        const float f = prof(y / ss); if (std::fabs(f - 1.f) < 0.002f) continue;
        for (int x = 0; x < c.W; x++) { const float sx = cx + (x + 0.5f - cx) / f; const int ix = (int)sx; row[x] = ix >= 0 && ix < c.W ? c.at(ix, y) : Px{0, 0, 0, 0}; prow[x] = ix >= 0 && ix < c.W ? c.pt(ix, y) : 0; }
        for (int x = 0; x < c.W; x++) { c.at(x, y) = row[x]; c.pt(x, y) = prow[x]; }
    }
}
static void drawTattoos(const Look& L) {   // fine-line ink on the skin (clothes drawn after cover what they cover)
    const Px K = alpha(rgb(40, 28, 32), 190), Kl = alpha(rgb(40, 28, 32), 120); const float shW = L.guy ? 12.f : 9.5f;
    const int t = L.tattoos;
    if (t & 1) for (int y = 56; y <= 96; y += 3) { const float x = HX + shW + 1.f + (y - 54) * 0.11f; plot(x - 1, y, K); plot(x + 1, y + 1, Kl); plot(x, y + 2, K); }   // arm sleeve (right arm)
    if (t & 2) { for (int k = 0; k < 4; k++) { plot(HX - 6 + k * 0.8f, 100 + k, K); plot(HX + 4 + k * 0.8f, 100 + k, K); } }      // "LOVE / HATE" script on the thighs
    if (t & 4) { const float x = HX - 5, y = 112; plot(x - 1, y, K); plot(x + 1, y, K); plot(x - 1.5f, y + 1, K); plot(x + 1.5f, y + 1, K); plot(x, y + 2, K); plot(x - 0.8f, y + 1.6f, Kl); }   // thigh heart
    if (t & 8) { const float x = HX + 5; for (int y = 104; y <= 116; y++) plot(x, y, K); for (int k = -2; k <= 2; k++) plot(x + k, 107, K); }                  // thigh cross
    if (t & 16) { ell(HX - shW - 1, 56, 1.6f, 1.4f, alpha(rgb(170, 40, 60), 170), false); plot(HX - shW - 2, 58, alpha(rgb(40, 110, 50), 170)); }         // shoulder rose
    if (t & 64) { const float x = HX + 7, y = 86; plot(x - 1, y, K); plot(x + 1, y, K); plot(x - 2, y - 1, Kl); plot(x + 2, y - 1, Kl); plot(x, y + 1, K); }   // hip butterfly
    if (t & 128) for (int k = 0; k < 6; k++) plot(HX + 4.4f - 2 + k * 0.8f, 168 + (k % 2), Kl);                                                             // ankle vine
}
// ============================================================== Custom Heads (converted Sims 1 heads)
// romfs:/Heads or sdmc:/LifeSim/Heads: <N_Name>.png (head in front of the body, 96x208 doll frame) + <N_Name>_b.png (hair behind)
static std::vector<std::string> g_heads; static bool g_headsScanned = false;
static void headScan() {
    g_headsScanned = true; g_heads.clear();
    for (const char* dir : {"romfs:/Heads", "sdmc:/LifeSim/Heads"}) {
        DIR* d = opendir(dir); if (!d) continue;
        std::vector<std::string> found;
        while (dirent* e = readdir(d)) {
            std::string n = e->d_name; if (n.size() < 5 || n[0] == '.') continue;
            std::string ext = n.substr(n.size() - 4); for (auto& c : ext) c = (char)tolower(c);
            if (ext != ".png" || (n.size() > 6 && n.substr(n.size() - 6, 2) == "_b")) continue;
            found.push_back(n.substr(0, n.size() - 4));
        }
        closedir(d); std::sort(found.begin(), found.end());
        for (auto& f : found) {
            bool dup = false; for (auto& h : g_heads) if (h.substr(h.rfind('/') + 1) == f) dup = true;
            if (!dup && g_heads.size() < 7) g_heads.push_back(std::string(dir) + "/" + f);
        }
    }
}
static int headCountI() { if (!g_headsScanned) headScan(); return (int)g_heads.size(); }
static std::string headNameI(int id) {   // 1-based; "2_Long_Ginger" -> "Long Ginger"
    if (id <= 0) return "Doll Head"; if (!g_headsScanned) headScan(); if (id > (int)g_heads.size()) return "(missing head)";
    std::string n = g_heads[id - 1].substr(g_heads[id - 1].rfind('/') + 1); size_t u = n.find('_'); if (u != std::string::npos && u < 3) n = n.substr(u + 1);
    for (auto& c : n) if (c == '_') c = ' '; return n;
}
struct HeadImg { std::vector<uint8_t> f, b; int ok = -1; };
static HeadImg g_headImg[8];
static bool headLoad(int id) {
    if (id <= 0 || id > 7) return false; if (!g_headsScanned) headScan();
    HeadImg& h = g_headImg[id]; if (h.ok >= 0) return h.ok == 1;
    h.ok = 0; if (id > (int)g_heads.size()) return false;
    auto fit = [](const std::string& path, std::vector<uint8_t>& out) {   // any size -> 96x208 (nearest)
        std::vector<uint8_t> px; int w = 0, hh = 0; out.assign(CW * CH * 4, 0);
        if (!spriteDecode(path, px, w, hh, nullptr) || w <= 0 || hh <= 0) return false;
        for (int y = 0; y < CH; y++) for (int x = 0; x < CW; x++) memcpy(&out[(y * CW + x) * 4], &px[((size_t)(y * hh / CH) * w + x * w / CW) * 4], 4);
        return true; };
    if (!fit(g_heads[id - 1] + ".png", h.f)) return false;
    fit(g_heads[id - 1] + "_b.png", h.b);   // optional
    h.ok = 1; return true;
}
static void applyHead(int id) {
    if (!headLoad(id)) return;
    const HeadImg& h = g_headImg[id];
    // the doll keeps its own cute face; the custom head is a pixel hairstyle drawn over (front) and behind (back) it
    for (int y = 0; y < CH; y++) for (int x = 0; x < CW; x++) {
        Px& d = g_final.at(x, y); const int i = (y * CW + x) * 4;
        if (!h.b.empty() && h.b[i + 3] && d.a < 255) {   // hair behind the body: the doll stays on top
            const float da = d.a / 255.f, ba = h.b[i + 3] / 255.f, oa = da + ba * (1 - da);
            for (int c = 0; c < 3; c++) { uint8_t& dc = c == 0 ? d.r : c == 1 ? d.g : d.b; dc = (uint8_t)((dc * da + h.b[i + c] * ba * (1 - da)) / oa); }
            d.a = (uint8_t)(oa * 255);
        }
        if (h.f[i + 3]) {   // head in front
            const float fa = h.f[i + 3] / 255.f, da = d.a / 255.f, oa = fa + da * (1 - fa);
            for (int c = 0; c < 3; c++) { uint8_t& dc = c == 0 ? d.r : c == 1 ? d.g : d.b; dc = (uint8_t)((h.f[i + c] * fa + dc * da * (1 - fa)) / std::max(0.001f, oa)); }
            d.a = (uint8_t)(oa * 255);
        }
    }
}

void paint(const Look& L0) {
    Look L = L0;
    g_chibiOn = false; if (g_chibi2) chiInit();
    if (g_dollV2) { CHI_HEAD = 1.22f; CHI_BODY = 0.96f; CHI_BODYX = 0.94f; CHI_N0 = 44.f; CHI_N1 = 54.f; chiInit(); }
    if (g_world) L.extras &= ~(X_TINTS | X_LIPRING | X_FRECKLES | X_NOSERING | X_SHADES | X_GLASSES);
    g_final.init(CW, CH);
    useBig();
    if (!L.chead) drawHairBack(L);   // Custom Head hair replaces the doll's hair (the doll head & face stay)
    drawBody(L);
    if (L.tattoos) drawTattoos(L);
    drawShoes(L);
    drawBottom(L);
    drawTop(L);
    drawDesign(L);
    if (L.body && !(L.guy && L.body == 3)) bodyWarp(L.body);   // guys' 4th body type is Athletic (no waist pinch)
    if (L.guy && L.body == 3) {   // Athletic: pecs, six-pack and V-lines, only where bare skin shows (shirts cover it)
        const Px S2 = rgb(L.skin), D = sh(S2, 0.80f), D2 = sh(S2, 0.88f), Hl = lit(S2, 0.12f);
        auto skinAt = [&](float x, float y) { const int px = (int)(x * g_ss), py = (int)(y * g_ss); if (px < 0 || py < 0 || px >= C_->W || py >= C_->H) return false; const Px& c = C_->at(px, py); return c.a && std::abs(c.r - S2.r) + std::abs(c.g - S2.g) + std::abs(c.b - S2.b) < 46; };
        auto mark = [&](float x0, float y0, float x1, float y1, Px c) { for (int k = 0; k <= 10; k++) { const float x = x0 + (x1 - x0) * k / 10.f, y = y0 + (y1 - y0) * k / 10.f; if (skinAt(x, y)) ell(x, y, 0.55f, 0.55f, c, false); } };
        mark(HX - 10, 64.5f, HX - 1, 66.5f, D); mark(HX + 10, 64.5f, HX + 1, 66.5f, D);          // pec lines
        mark(HX - 9, 61.5f, HX - 3, 62.5f, Hl); mark(HX + 9, 61.5f, HX + 3, 62.5f, Hl);         // chest highlight
        mark(HX, 67, HX, 82, D);                                                                // centre line
        for (int r = 0; r < 3; r++) { const float y = 71 + r * 4.f; mark(HX - 5.5f, y, HX - 1, y + 0.4f, D2); mark(HX + 5.5f, y, HX + 1, y + 0.4f, D2);   // six-pack rows
            mark(HX - 3.5f, y - 1.6f, HX - 2, y - 1.6f, Hl); mark(HX + 3.5f, y - 1.6f, HX + 2, y - 1.6f, Hl); }
        mark(HX - 8, 78, HX - 3, 86, D); mark(HX + 8, 78, HX + 3, 86, D);                       // V-lines
    }
    if (L.body == 3 && !L.guy && !g_world) {   // Body Tea: a little navel + soft ab shading where the tummy shows
        const Px S2 = rgb(L.skin);
        for (int y = 74; y <= 82; y++) { const Px& c = C_->at((int)(HX * g_ss), (int)(y * g_ss)); if (c.a && std::abs(c.r - S2.r) + std::abs(c.g - S2.g) + std::abs(c.b - S2.b) < 40) { if (y == 80) ell(HX, 80, 0.7f, 1.0f, alpha(sh(S2, 0.6f), 200), false); else if (y == 75) line(HX, 74, HX, 79, alpha(sh(S2, 0.85f), 90)); } }
    }
    downsample(false);
    useFinal();
    if (!g_world) drawFace(L);
    if (!g_world && (L.tattoos & 32)) { const Px K = alpha(rgb(40, 28, 32), 200); const int ey = (int)HY + 2; plot(56, ey + 2, K); plot(55, ey + 3, K); plot(57, ey + 3, K); plot(56, ey + 4, K); plot(54, ey + 6, alpha(rgb(40, 28, 32), 140)); }   // little stars by the eye
    useBig();
    if (!L.chead) drawHairFront(L);
    downsample(true);
    useFinal();
    drawExtras(L);
    if (g_chibi2) { chibiWarp(); if (!g_world) chibiEyes(L); }
    else if (g_dollV2) { chibiWarp(); if (!g_world) dollFace(L); }
    if (!g_world) inkPass();
    if (L.chead) applyHead(L.chead);
}

// ============================================================== the cast
const RGB SK[6] = {{252, 224, 210}, {238, 196, 166}, {214, 164, 122}, {176, 118, 82}, {136, 86, 58}, {98, 62, 44}};
struct Entry { OcDef def; Look look; };
const Entry CAST[OC_COUNT] = {
    {{"Bambi", "Glam Girl", "Lives for golden hour, gold jewelry and anything pink. Has never once been on time.", false, ST_YA},
     {SK[0], {214, 172, 112}, {46, 30, 30}, {230, 130, 140}, 0, T_BABYTEE, B_RUFFLE, S_STRAPPY, {92, 64, 48}, {246, 150, 180}, {86, 58, 42}, {0, 0, 0}, {214, 170, 104}, X_FLOWER | X_BAG | X_NECK | X_CHAINBELT | X_BANGLES | X_STUDS, false}},
    {{"Kiki", "Mirror-Selfie Queen", "Posts 40 selfies a day. Baggy jorts, big chains, bigger attitude.", false, ST_YA},
     {SK[1], {236, 222, 186}, {50, 34, 30}, {210, 140, 140}, 1, T_JERSEY, B_JORTS, S_SNEAK, {30, 36, 56}, {250, 250, 250}, {84, 104, 132}, {0, 0, 0}, {40, 44, 56}, X_VISOR | X_NECK | X_CHAINBELT | X_SOCKS | X_BANGLES | X_PHONE | X_HOOPS, false}},
    {{"Destiny", "Trendsetter", "Braids to her waist and a closet full of lilac. Runs the Glitter High fashion club.", false, ST_YA},
     {SK[5], {34, 24, 24}, {40, 24, 20}, {176, 90, 104}, 2, T_HALTER, B_FLARE, S_PLATFORM, {200, 160, 236}, {255, 255, 255}, {104, 134, 190}, {0, 0, 0}, {236, 230, 250}, X_HOOPS | X_CUFFS | X_NECK, false}},
    {{"Mia", "Sporty Spice", "Velour tracksuit, high pony, iced coffee. Will race you anywhere, anytime.", false, ST_YA},
     {SK[2], {28, 22, 22}, {44, 28, 22}, {200, 110, 120}, 3, T_TRACKZIP, B_TRACKPANT, S_CHUNKY, {244, 128, 176}, {255, 255, 255}, {244, 128, 176}, {255, 255, 255}, {250, 250, 250}, X_HOOPS | X_HEADPH, false}},
    {{"Lux", "Night Owl", "Red bob, black slip dress, combat boots. Writes sad songs and runs the cafe open mic.", false, ST_YA},
     {SK[0], {186, 40, 46}, {40, 30, 34}, {150, 40, 60}, 4, T_SLIP, B_NONE, S_COMBAT, {34, 30, 38}, {120, 120, 130}, {34, 30, 38}, {0, 0, 0}, {28, 26, 30}, X_CHOKER | X_FISHNET | X_STUDS, false}},
    {{"Nova", "Sunshine", "Butterfly tops, big puff, bigger laugh. Knows everyone at Sunset Beach by name.", false, ST_TEEN},
     {SK[4], {40, 28, 24}, {50, 32, 24}, {190, 100, 110}, 5, T_BUTTERFLY, B_DENIMMINI, S_KNEEBOOT, {255, 236, 140}, {255, 150, 60}, {110, 150, 210}, {0, 0, 0}, {250, 246, 236}, X_HOOPS | X_BANGLES | X_BAG, false}},
    {{"Dante", "Smooth Talker", "Crisp white tee, gold chain, fresh kicks every Friday. Flirts with literally everyone.", true, ST_YA},
     {SK[4], {26, 20, 20}, {40, 26, 20}, {150, 90, 80}, 6, T_BIGTEE, B_BAGGYJEAN, S_WORKBOOT, {246, 246, 246}, {220, 220, 220}, {60, 84, 120}, {0, 0, 0}, {210, 160, 90}, X_NECK | X_STUDS, true}},
    {{"Jay", "Class Clown", "Frosted tips, baseball jersey, cargo shorts. Has a prank planned for every holiday.", true, ST_TEEN},
     {SK[1], {120, 84, 50}, {60, 90, 120}, {200, 140, 130}, 7, T_BASEBALL, B_CARGOSHORT, S_SNEAK, {250, 250, 250}, {200, 40, 60}, {150, 140, 110}, {80, 70, 50}, {230, 60, 70}, X_NECK | X_SOCKS | X_TINTS, true}},
    // ---- the subway crew (from the player's favourite drawing)
    {{"Raven", "Rock Chick", "Headphones always on, black flares, studded belt. Secretly loves pop music.", false, ST_YA},
     {SK[0], {26, 20, 24}, {60, 60, 64}, {190, 120, 120}, 0, T_MOTO, B_FLARE, S_COMBAT, {34, 32, 38}, {60, 60, 66}, {44, 44, 56}, {0, 0, 0}, {26, 24, 28}, X_HEADPH | X_CROSS | X_STUDBELT, false, {0, 0, 0}}},
    {{"Leila", "Daydreamer", "Pink tank, floral maxi skirt, a big round belt buckle and a phone full of playlists.", false, ST_YA},
     {SK[2], {78, 48, 32}, {60, 40, 30}, {214, 120, 120}, 8, T_CAMI, B_MAXI, S_FLIPFLOP, {250, 170, 200}, {250, 170, 200}, {252, 236, 242}, {246, 150, 190}, {220, 180, 150}, X_NECK | X_BUCKLE | X_BANGLES | X_PHONE | X_TOTE, false, {0, 0, 0}}},
    {{"Kenji", "Photographer", "Never without his camera. Shaggy hair, grey tee, baggy jeans. Quiet until you mention film.", true, ST_YA},
     {SK[1], {28, 24, 30}, {40, 30, 30}, {200, 140, 130}, 11, T_BIGTEE, B_BAGGYJEAN, S_COMBAT, {200, 200, 204}, {170, 170, 176}, {110, 130, 170}, {0, 0, 0}, {30, 28, 32}, X_CAMERA, true, {0, 0, 0}}},
    {{"Amara", "Soft Girl", "Waist-length braids, a white wrap shrug, butter-yellow cami and the comfiest jeans.", false, ST_YA},
     {SK[5], {26, 20, 20}, {50, 30, 24}, {176, 90, 100}, 2, T_SHRUG, B_BAGGYJEAN, S_SNEAK, {246, 244, 240}, {252, 236, 170}, {170, 196, 226}, {0, 0, 0}, {244, 240, 232}, X_HEART | X_BAG | X_STUDS, false, {0, 0, 0}}},
    {{"Marco", "Heartthrob", "Curly hair, a purple polo, gold chain, a little stubble. Knows every subway line by heart.", true, ST_YA},
     {SK[3], {26, 20, 20}, {44, 28, 22}, {160, 100, 90}, 12, T_POLO, B_BAGGYJEAN, S_SNEAK, {90, 80, 160}, {70, 60, 130}, {150, 178, 214}, {0, 0, 0}, {244, 240, 232}, X_NECK | X_STUBBLE, true, {0, 0, 0}}},
    // ---- inspired by the player's own drawings
    {{"Sunny", "Beach Babe", "Starfish necklace, flip-flops, lounge pants. Has never been seen without a smoothie.", false, ST_YA},
     {SK[1], {240, 220, 150}, {70, 110, 120}, {214, 130, 130}, 8, T_CAMI, B_WIDELEG, S_FLIPFLOP, {226, 244, 150}, {250, 252, 220}, {246, 128, 120}, {0, 0, 0}, {120, 220, 210}, X_STARNECK | X_BEADS | X_STUDS, false, {0, 0, 0}}},
    {{"Mari", "Cutie", "Cuffed capris, wedges and a baby tee with a strawberry buddy. Collects anklets.", false, ST_YA},
     {SK[2], {108, 66, 40}, {56, 36, 28}, {200, 110, 110}, 0, T_GRAPHIC, B_CAPRI, S_WEDGE, {70, 190, 220}, {250, 250, 250}, {110, 150, 180}, {0, 0, 0}, {250, 228, 140}, X_NECK | X_NAVEL | X_ANKLET | X_BANGLES, false, {0, 0, 0}}},
    {{"Imani", "Free Spirit", "Swirly off-shoulder tops, studded heels and a peace sign on every necklace.", false, ST_YA},
     {SK[5], {50, 32, 26}, {50, 30, 24}, {170, 80, 90}, 8, T_SWIRL, B_MICRO, S_STUDHEEL, {246, 140, 200}, {170, 130, 240}, {120, 170, 220}, {0, 0, 0}, {246, 110, 120}, X_PEACE | X_BANGLES | X_BEADS, false, {0, 0, 0}}},
    {{"Gigi", "Sweetheart", "Polka dots, a ruffle mini and a gold ring belt. Plans every group hangout.", false, ST_YA},
     {SK[2], {170, 120, 70}, {60, 40, 30}, {220, 110, 130}, 8, T_POLKATUBE, B_RUFFLE, S_STRAPPY, {250, 120, 200}, {246, 236, 150}, {248, 248, 248}, {0, 0, 0}, {240, 236, 150}, X_RINGBELT | X_BEADS | X_ANKLET | X_HAIRCLIP | X_NECK, false, {0, 0, 0}}},
    {{"Starla", "Star Girl", "Big curls full of starfish and flower clips, star hoops and pearls everywhere.", false, ST_YA},
     {SK[4], {84, 50, 34}, {44, 28, 22}, {150, 50, 50}, 10, T_CRISSCROSS, B_MAXI, S_PLATFORM, {250, 246, 236}, {250, 170, 200}, {240, 230, 140}, {250, 160, 200}, {210, 214, 224}, X_STARCLIP | X_FLOWERCLIPS | X_STARHOOPS | X_PEARLBELLY | X_NECK, false, {120, 190, 240}}},
    {{"Bliss", "Pastel Princess", "Platinum bangs, a WOAH tee, pastel nails and lip rings. Says \"woah\" a lot.", false, ST_YA},
     {SK[0], {246, 232, 180}, {50, 34, 30}, {246, 150, 140}, 9, T_TEXTTEE, B_MAXI, S_FLIPFLOP, {252, 236, 140}, {110, 180, 240}, {240, 230, 140}, {250, 170, 200}, {246, 110, 180}, X_STARCLIP | X_LIPRING | X_NAILS | X_PEARLBELLY, false, {120, 190, 240}}},    // ---- the late-night crew (inspired by the player's new reference batch)
    {{"Onyx", "Mall Goth", "Studded cuffs, olive cargos and a stare that could curdle milk. Soft spot for stray cats.", false, ST_YA},
     {SK[2], {22, 18, 20}, {40, 28, 24}, {170, 110, 100}, 8, T_TUBE, B_CARGO, S_SNEAK, {28, 26, 30}, {28, 26, 30}, {74, 72, 44}, {60, 58, 36}, {28, 26, 30}, X_WRISTBAND | X_STUDBELT | X_CHOKER | X_NECK | X_NOSERING, false, {0, 0, 0}}},
    {{"Noemi", "Onyx's Shadow", "Skull baby tee, black wide-legs, blushes when anyone says hi. Onyx's best friend since kindergarten.", false, ST_YA},
     {SK[2], {22, 18, 20}, {44, 30, 24}, {200, 120, 120}, 8, T_SKULLTEE, B_WIDELEG, S_SNEAK, {104, 104, 112}, {246, 244, 240}, {26, 24, 28}, {0, 0, 0}, {246, 244, 240}, X_WRISTBAND | X_NECK, false, {0, 0, 0}}},
    {{"Rocco", "Lover Boy", "Pink buzz cut with hearts dyed in. Open hoodie, gold pendant, falls in love weekly.", true, ST_YA},
     {SK[1], {244, 150, 150}, {44, 30, 26}, {180, 120, 110}, 14, T_HOODIE, B_BAGGYJEAN, S_SNEAK, {110, 108, 112}, {248, 248, 248}, {40, 40, 48}, {0, 0, 0}, {246, 246, 246}, X_HEARTHAIR | X_NECK | X_STUDS | X_STUBBLE, true, {0, 0, 0}}},
    {{"Coco", "It Girl", "Choppy black bob, navy halter, glossy lips and a studded belt. Rocco's on-again, off-again.", false, ST_YA},
     {SK[2], {20, 16, 18}, {40, 26, 22}, {196, 120, 110}, 4, T_HALTER, B_BAGGYJEAN, S_PLATFORM, {36, 38, 66}, {36, 38, 66}, {150, 186, 222}, {0, 0, 0}, {30, 28, 32}, X_STUDBELT | X_STUDS | X_BANGLES, false, {0, 0, 0}}},
    {{"Tavo", "Skater", "Striped polo, black jorts, tall socks. Always looks like he just rolled out of bed. Doesn't care.", true, ST_YA},
     {SK[3], {22, 18, 18}, {40, 26, 20}, {150, 92, 82}, 12, T_STRIPEPOLO, B_JORTS, S_SNEAK, {30, 30, 34}, {222, 222, 226}, {34, 34, 38}, {0, 0, 0}, {248, 248, 248}, X_SOCKS | X_CROSS | X_STUDBELT | X_STUDS, true, {0, 0, 0}}},
    {{"Valentina", "Diva", "Waist-length curls, a plum floral bodycon and zero patience for Tavo's outfits.", false, ST_YA},
     {SK[2], {20, 16, 18}, {40, 24, 20}, {170, 90, 90}, 0, T_BODYCON, B_NONE, S_STRAPPY, {140, 60, 110}, {206, 140, 186}, {0, 0, 0}, {0, 0, 0}, {40, 30, 36}, X_BANGLES | X_NECK | X_NAILS, false, {0, 0, 0}}},
    {{"Paloma", "Sweetie", "Pink tank, pink floral maxi, gold medallion belt. Loves hugs, horses and matchmaking.", false, ST_YA},
     {SK[2], {70, 42, 30}, {54, 34, 26}, {206, 120, 120}, 8, T_CAMI, B_MAXI, S_FLIPFLOP, {250, 186, 212}, {250, 186, 212}, {252, 232, 240}, {246, 160, 196}, {214, 170, 120}, X_NECK | X_BUCKLE | X_BANGLES, false, {0, 0, 0}}},
    {{"Rohan", "Rich Kid", "Curly hair, navy polo, wide-leg jeans and a gold watch he 'forgot' was expensive.", true, ST_YA},
     {SK[3], {24, 18, 18}, {40, 26, 20}, {150, 92, 84}, 12, T_POLO, B_WIDELEG, S_SNEAK, {70, 66, 120}, {60, 56, 110}, {150, 176, 210}, {0, 0, 0}, {220, 204, 176}, X_NECK | X_STUBBLE | X_NOSERING, true, {0, 0, 0}}},
    {{"Clementine", "Bookworm", "Butter-yellow babydoll, slouchy boots and little round glasses. Always reading in the corner.", false, ST_YA},
     {SK[2], {26, 18, 16}, {50, 32, 24}, {200, 120, 120}, 0, T_BABYDOLL, B_NONE, S_COMBAT, {252, 240, 186}, {250, 250, 244}, {0, 0, 0}, {0, 0, 0}, {110, 70, 46}, X_GLASSES | X_HOOPS | X_SOCKS, false, {0, 0, 0}}},
    {{"Skye", "Cool Girl", "Platinum high pony, navy crop tee, charcoal flares. Says 'handle with care' and means it.", false, ST_YA},
     {SK[0], {244, 236, 210}, {90, 120, 160}, {200, 120, 120}, 3, T_CROPTEE, B_FLARE, S_CHUNKY, {44, 52, 96}, {44, 52, 96}, {66, 70, 80}, {0, 0, 0}, {246, 246, 246}, X_HOOPS | X_NAVEL, false, {0, 0, 0}}},
    {{"Nia", "Old Soul", "Hair to her knees, a cream cami, a cocoa wrap skirt and gold everything. Writes letters by hand.", false, ST_YA},
     {SK[5], {20, 16, 16}, {44, 26, 20}, {150, 80, 80}, 8, T_CAMI, B_WRAP, S_FLIPFLOP, {250, 236, 200}, {250, 236, 200}, {84, 50, 40}, {0, 0, 0}, {226, 180, 90}, X_GLASSES | X_HOOPS | X_NECK, false, {0, 0, 0}}},
    {{"Sable", "Interior Designer", "Floor-length faux fur, grey flares, zebra bag. Plays guitar in a pop-punk band after work.", false, ST_YA},
     {SK[1], {28, 24, 28}, {40, 28, 24}, {190, 110, 110}, 8, T_FURCOAT, B_FLARE, S_KNEEBOOT, {30, 28, 32}, {30, 28, 32}, {170, 170, 166}, {0, 0, 0}, {26, 24, 28}, X_HOOPS | X_BAG | X_ZEBRA, false, {0, 0, 0}}},
    {{"Peaches", "Selfie Star", "Caramel highlights, freckles, a grey tee and the tiniest shorts. Her front camera never sleeps.", false, ST_YA},
     {SK[1], {40, 28, 24}, {40, 26, 22}, {236, 140, 150}, 8, T_CROPTEE, B_MICRO, S_SNEAK, {170, 170, 176}, {170, 170, 176}, {196, 214, 232}, {0, 0, 0}, {250, 190, 214}, X_HIGHLIGHT | X_FRECKLES | X_NOSERING | X_NECK | X_BAG | X_PHONE | X_NAVEL, false, {0, 0, 0}}},
    {{"Lulu", "Music Girl", "Two braids to her waist, a star-print zip hoodie, white headphones. Hums everywhere.", false, ST_YA},
     {SK[2], {26, 18, 16}, {44, 28, 22}, {196, 120, 120}, 13, T_STARHOODIE, B_DENIMMINI, S_SNEAK, {90, 112, 130}, {248, 248, 248}, {26, 24, 28}, {0, 0, 0}, {248, 248, 248}, X_HEADPH | X_WHITEPH | X_CROSS, false, {0, 0, 0}}},
    {{"Bella", "Phone Gossip", "High pony, pink polka top, gold hoops and a flip phone that's always ringing.", false, ST_YA},
     {SK[2], {150, 100, 60}, {40, 26, 22}, {236, 110, 150}, 3, T_POLKATUBE, B_DENIMMINI, S_PLATFORM, {246, 100, 170}, {252, 236, 244}, {236, 236, 240}, {0, 0, 0}, {246, 150, 190}, X_HOOPS | X_NECK | X_BANGLES | X_PHONE | X_NAILS, false, {250, 170, 200}}},
    {{"Rosa", "Hopeless Romantic", "Chestnut bangs, a mauve lace cami and a little silver cross. Believes in soulmates.", false, ST_YA},
     {SK[2], {110, 64, 44}, {40, 26, 22}, {170, 104, 96}, 9, T_CAMI, B_WIDELEG, S_STRAPPY, {186, 150, 146}, {204, 172, 168}, {150, 118, 112}, {0, 0, 0}, {120, 86, 80}, X_CROSS | X_FRECKLES, false, {0, 0, 0}}},    // ---- House of Vane (story mode)
    {{"Celestine", "Creative Director", "Runs the House of Vane and GLOSS magazine. Has never once raised her voice. Has never needed to.", false, ST_ADULT},
     {SK[0], {214, 214, 222}, {70, 80, 96}, {150, 40, 56}, 4, T_FURCOAT, B_FLARE, S_STUDHEEL, {44, 40, 48}, {44, 40, 48}, {30, 28, 34}, {0, 0, 0}, {30, 26, 32}, X_SHADES | X_STUDS | X_NECK, false, {0, 0, 0}}},
    {{"Odette", "First Assistant", "Celestine's first assistant. Has a cold, an iron will and a countdown to Lumiere City Fashion Week.", false, ST_YA},
     {SK[0], {176, 70, 44}, {60, 90, 80}, {176, 60, 70}, 1, T_MOTO, B_FLARE, S_KNEEBOOT, {30, 28, 34}, {60, 60, 66}, {30, 28, 34}, {0, 0, 0}, {28, 26, 30}, X_CHOKER | X_STUDS, false, {90, 60, 80}}},
    {{"Julian", "Style Director", "Knows every hem in the archive. Grumpy, brilliant, secretly the kindest person in the building.", true, ST_ADULT},
     {SK[3], {40, 30, 28}, {40, 26, 20}, {150, 92, 84}, 6, T_BIGTEE, B_BAGGYJEAN, S_WORKBOOT, {36, 34, 40}, {36, 34, 40}, {60, 64, 76}, {0, 0, 0}, {80, 56, 40}, X_GLASSES | X_NECK | X_STUBBLE, true, {0, 0, 0}}},    // ---- the doll-girl crew (inspired by the player's reference batch)
    {{"Brielle", "Effortless", "Bouncy blowout, gold hair clips, a denim vest and flares. Always looks like she just left a salon.", false, ST_YA},
     {SK[3], {82, 54, 40}, {50, 34, 28}, {200, 120, 120}, 0, T_DENIMVEST, B_FLARE, S_PLATFORM, {120, 150, 196}, {250, 250, 248}, {170, 196, 222}, {0, 0, 0}, {230, 230, 236}, X_GOLDCLIPS | X_BUCKLE | X_NECK | X_BANGLES | X_BAG, false, {0, 0, 0}}},
    {{"Marisol", "Futbol Fan", "Ash-blonde waves, a flower behind her ear, a yellow jersey tank and green flares. Never misses a match.", false, ST_YA},
     {SK[2], {210, 190, 160}, {50, 34, 28}, {200, 120, 120}, 8, T_JERSEYTANK, B_FLARE, S_SNEAK, {250, 220, 60}, {40, 150, 80}, {50, 92, 80}, {0, 0, 0}, {230, 230, 230}, X_WHITEFLOWER | X_HEADPH | X_WHITEPH | X_CHAINBELT | X_BANGLES | X_NECK, false, {0, 0, 0}}},
    {{"Kenya", "Cozy Queen", "Spiky space buns, a baby-blue tee with a cute face on it, joggers and a belly ring. Comfort is a lifestyle.", false, ST_YA},
     {SK[4], {80, 52, 36}, {44, 28, 22}, {176, 100, 104}, 15, T_FACETEE, B_TRACKPANT, S_SNEAK, {170, 206, 246}, {250, 250, 250}, {60, 60, 66}, {200, 200, 206}, {246, 246, 246}, X_NAVEL | X_NOSERING | X_NECK | X_BANGLES | X_STUDS, false, {0, 0, 0}}},
    {{"Simone", "Apres-Ski", "Huge curls under a cream fur hat, a plum halter, dotted tights and fluffy leg warmers. It's always snow season.", false, ST_YA},
     {SK[2], {70, 46, 32}, {60, 80, 90}, {200, 130, 130}, 0, T_HALTER, B_RUFFLE, S_STUDHEEL, {70, 40, 56}, {70, 40, 56}, {70, 40, 56}, {0, 0, 0}, {246, 240, 232}, X_FURHAT | X_TIGHTS | X_LEGWARM | X_RINGBELT | X_BAG, false, {0, 0, 0}, {240, 232, 220}}},
    {{"Tiana", "Downtown", "Long black hair, star earrings, an aqua tee, a ruffle denim micro and slouchy knee boots.", false, ST_YA},
     {SK[4], {22, 18, 18}, {44, 28, 22}, {176, 104, 104}, 8, T_CROPTEE, B_RUFFLE, S_KNEEBOOT, {150, 214, 214}, {150, 214, 214}, {84, 94, 120}, {0, 0, 0}, {150, 120, 86}, X_STARHOOPS | X_CHOKER | X_CHAINBELT | X_BANGLES | X_LEGWARM, false, {0, 0, 0}, {240, 210, 220}}},
    {{"Ayo", "Island Girl", "A white lily in her hair, a sheer butter-yellow babydoll, ruffled white shorts and gold everything.", false, ST_YA},
     {SK[5], {20, 16, 16}, {40, 24, 20}, {160, 80, 90}, 8, T_STRAPLESS, B_RUFFLE, S_WEDGE, {248, 226, 140}, {248, 226, 140}, {250, 248, 244}, {0, 0, 0}, {248, 236, 200}, X_WHITEFLOWER | X_HOOPS | X_NECK | X_BANGLES | X_NAILS, false, {150, 60, 90}}},
    {{"Gia", "Champagne Problems", "Messy high bun with a white flower, a gold satin cami, laced white flares and a white top-handle bag.", false, ST_YA},
     {SK[2], {20, 16, 16}, {40, 26, 22}, {170, 90, 90}, 1, T_CAMI, B_CAPRI, S_STRAPPY, {226, 206, 150}, {226, 206, 150}, {248, 248, 248}, {0, 0, 0}, {250, 250, 250}, X_WHITEFLOWER | X_CHAINLACE | X_NECK | X_FRECKLES | X_TOTE | X_STUDS, false, {120, 70, 60}}},
    {{"Zuri", "Big Hair Energy", "The biggest curls in Starlight Bay, a pinstripe cardigan with fur cuffs, white shorts and fur-topped boots.", false, ST_YA},
     {SK[2], {84, 56, 38}, {50, 34, 28}, {200, 130, 130}, 16, T_PINCARDIGAN, B_MICRO, S_KNEEBOOT, {90, 64, 52}, {200, 186, 170}, {250, 248, 244}, {0, 0, 0}, {30, 28, 34}, X_CHAINBELT | X_NECK | X_NOSERING | X_LEGWARM | X_TOTE, false, {0, 0, 0}, {110, 80, 56}}},
    {{"Lena", "Coffee Run", "Sleek black hair, freckles, a butter tank, a flower belt and a lace-trim denim midi. Iced latte in hand. Always.", false, ST_YA},
     {SK[2], {22, 18, 18}, {44, 28, 22}, {190, 120, 110}, 8, T_CAMI, B_MIDI, S_STRAPPY, {252, 236, 170}, {252, 236, 170}, {190, 206, 226}, {248, 246, 240}, {250, 250, 250}, X_FRECKLES | X_BUCKLE | X_NECK | X_HOOPS | X_COFFEE | X_BAG, false, {200, 190, 190}}},
    {{"Cleo", "Clean Girl", "Slicked back with one long braid, a white halter, low-rise jeans, silver chains and the biggest hoops.", false, ST_YA},
     {SK[1], {30, 22, 20}, {60, 44, 34}, {220, 150, 150}, 17, T_HALTER, B_BAGGYJEAN, S_SNEAK, {250, 250, 252}, {250, 250, 252}, {50, 74, 120}, {0, 0, 0}, {250, 250, 250}, X_BIGHOOPS | X_SILVERCHAIN | X_BANGLES | X_TOTE, false, {80, 70, 70}}},
    {{"Noor", "Ski Lodge Chic", "Glossy black curls, a white headband, a white turtleneck, a pink plaid mini and fluffy white leg warmers.", false, ST_YA},
     {SK[4], {22, 18, 18}, {40, 26, 22}, {160, 90, 90}, 0, T_TURTLENECK, B_PLAID, S_STUDHEEL, {250, 250, 250}, {250, 250, 250}, {230, 196, 180}, {200, 120, 120}, {250, 250, 250}, X_HEADBAND | X_HOOPS | X_LEGWARM | X_BAG, false, {0, 0, 0}, {252, 252, 252}}},
    {{"Honey", "Cutie Bookworm", "Platinum waves, a bow clip, giant glasses, a chocolate baby tee and leopard everything.", false, ST_YA},
     {SK[2], {240, 226, 190}, {60, 44, 34}, {200, 130, 120}, 0, T_BOWTEE, B_RUFFLE, S_FLIPFLOP, {92, 60, 46}, {250, 250, 250}, {210, 180, 140}, {0, 0, 0}, {150, 110, 70}, X_GLASSES | X_LEOPARD | X_LEGWARM | X_HOOPS | X_FRECKLES | X_BAG, false, {0, 0, 0}, {180, 140, 96}}},
    {{"Daisy", "Strawberry Girl", "Long blonde hair under a newsboy cap, a red gingham tube, a star denim micro and chunky loafers.", false, ST_YA},
     {SK[1], {230, 206, 150}, {80, 130, 200}, {230, 130, 140}, 8, T_GINGHAMTUBE, B_RUFFLE, S_PLATFORM, {230, 70, 80}, {250, 250, 250}, {50, 60, 90}, {0, 0, 0}, {90, 56, 40}, X_NEWSBOY | X_GLASSES | X_STARSKIRT | X_NAVEL | X_LEGWARM | X_TOTE | X_HEADPH | X_WHITEPH, false, {0, 0, 0}, {250, 250, 250}}},
    {{"Scarlett", "Web Witch", "Black waves with blonde streaks, a red shrug over a web-print top, opera gloves and red fur-cuffed boots. Every day is Halloween.", false, ST_YA},
     {SK[2], {30, 26, 28}, {40, 26, 22}, {210, 120, 130}, 9, T_WEBTOP, B_DENIMMINI, S_KNEEBOOT, {200, 30, 46}, {200, 30, 46}, {30, 26, 30}, {0, 0, 0}, {170, 20, 40}, X_GLOVES | X_GLASSES | X_HIGHLIGHT | X_BIGHOOPS | X_NECK | X_LEGWARM, false, {0, 0, 0}, {236, 226, 214}}},
};
}   // namespace

const OcDef& ocDef(int i) { return CAST[((i % OC_COUNT) + OC_COUNT) % OC_COUNT].def; }

// Render into an RGBA8 buffer (row-major, top-down). Returns width/height.
void ocPaint(int i, uint8_t* rgba, int& w, int& h) {
#ifdef MAPLE_VALLEY
    { const int k = ((i % OC_COUNT) + OC_COUNT) % OC_COUNT; w = CW; h = CH; if (spriteCast((uint32_t)k + 1, CAST[k].look.guy, rgba)) return; }
#endif
    paint(CAST[((i % OC_COUNT) + OC_COUNT) % OC_COUNT].look);
    w = CW; h = CH;
    for (int y = 0; y < CH; y++) for (int x = 0; x < CW; x++) { const Px& p = g_final.at(x, y); uint8_t* o = rgba + (y * CW + x) * 4; o[0] = p.r; o[1] = p.g; o[2] = p.b; o[3] = p.a; }
}

// design pieces (story mode): the menu names match these orders
static const int DESIGN_TOPS[12] = {T_BABYTEE, T_HALTER, T_SLIP, T_CAMI, T_TUBE, T_MOTO, T_POLO, T_HOODIE, T_FURCOAT, T_BABYDOLL, T_BODYCON, T_CROPTEE};
static const int DESIGN_BOTS[9] = {B_RUFFLE, B_FLARE, B_CARGO, B_WIDELEG, B_MAXI, B_WRAP, B_MICRO, B_DENIMMINI, B_CAPRI};
static const int DESIGN_SHOES[6] = {S_STRAPPY, S_PLATFORM, S_KNEEBOOT, S_COMBAT, S_STUDHEEL, S_CHUNKY};
void ocPaintDesign(int model, int top, int bottom, int shoes, RGB c1, RGB c2, RGB c3, uint8_t* rgba) {
    Look L = CAST[((model % OC_COUNT) + OC_COUNT) % OC_COUNT].look;
    L.top = DESIGN_TOPS[((top % 12) + 12) % 12];
    bool dress = L.top == T_SLIP || L.top == T_BABYDOLL || L.top == T_BODYCON;
    L.bottom = dress ? B_NONE : DESIGN_BOTS[((bottom % 9) + 9) % 9];
    L.shoes = DESIGN_SHOES[((shoes % 6) + 6) % 6];
    L.topC = c1; L.topC2 = c3; L.botC = c2; L.botC2 = c3; L.shoeC = c2.r + c2.g + c2.b > 600 ? c3 : c2;
    L.extras &= (X_HOOPS | X_STUDS | X_NECK | X_BANGLES | X_GLASSES | X_STUBBLE | X_FRECKLES | X_SHADES | X_CHOKER);
    paint(L);
    for (int y = 0; y < CH; y++) for (int x = 0; x < CW; x++) { const Px& p = g_final.at(x, y); uint8_t* o = rgba + (y * CW + x) * 4; o[0] = p.r; o[1] = p.g; o[2] = p.b; o[3] = p.a; }
}


int ocIndexOfSim(const std::string& name, const std::string& bio) {
    if (bio.rfind("[Cast]", 0) != 0) return -1;
    for (int i = 0; i < OC_COUNT; i++) if (name == CAST[i].def.name) return i;
    return -1;
}


// ================================================================== everyone as a doll
// Townies (and you) get a doll built from their own look. In the world they're drawn at
// exactly half the portrait size: the body is halved with a clean 2x2 filter, then gets a
// fresh 1px outline and a hand-placed pixel face at that size (so nothing goes blurry).
constexpr int WCW = 48, WCH = 104;   // one world frame (half of 96x208); feet on row 93
Look lookFromPerson(const PersonDef& d0, int outfit) {
    PersonDef d = d0;
    if (outfit > 0 && outfit < NUM_OUTFITS) {
        const OutfitDef& o = OUTFITS[outfit];
        d.top = o.top; d.bottom = o.bottom; d.bt = o.bt; d.topPattern = o.topPattern; d.shoes = o.shoes;
        d.topStyle = o.topStyle; d.shoeStyle = o.shoeStyle; d.acc |= o.acc;
        if (o.hat) d.hat = o.hat;
        if (o.acc || o.hat) d.accCol = o.accCol;
    }
    if (d0.topPattern >= 100) d.topPattern = d0.topPattern;   // a custom design stays on over any outfit
    const bool guy = d.guy;
    Look L{};
    L.design = d.topPattern >= 100 && d.topPattern < 100 + DS_SLOTS ? d.topPattern - 100 : -1;
    L.guy = guy;
    L.skin = skinRGB(d.skin); L.hair = hairRGB(d.hairCol); L.eyes = eyeRGB(d.f.eyeCol);
    L.body = d.f.pad % 4; L.nails = (uint8_t)((d.f.pad >> 2) & 7); L.chead = (uint8_t)((d.f.pad >> 5) & 7); if (L.chead && !headLoad(L.chead)) L.chead = 0;
    L.liner = guy ? 0 : d.f.liner % 10; L.blushOn = d.f.blush % 10 != 0; L.blushC = blushRGB(d.f.blush);
    L.contour = d.f.shape & 3; L.lash = (d.f.shape >> 2) & 7; L.blushStyle = (d.f.shape >> 5) & 3; L.glow = (d.f.shape >> 7) & 1;
    L.eyeShape = d.f.eyes % 12; L.brow = d.f.brows % 10; L.nose = d.f.nose % 10; L.lipSh = d.f.lipShape % 10;
    L.lips = guy ? RGB{(uint8_t)((L.skin.r * 5 + 186 * 3) / 8), (uint8_t)((L.skin.g * 5 + 116 * 3) / 8), (uint8_t)((L.skin.b * 5 + 104 * 3) / 8)} : d.lips;   // guys: a natural nude lip
    static const uint8_t HS_G[NUM_HAIRSTYLES] = {8, 0, 3, 4, 1, 6, 12, 2, 10, 5, 11, 2, 13, 1, 6, 12, 6, 14, 12, 0, 7, 9, 5, 12, 14, 11, 8, 7,
                                                  18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                                                  30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
                                                  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84};
    static const uint8_t HS_B[NUM_HAIRSTYLES] = {11, 12, 6, 11, 6, 6, 12, 2, 10, 5, 11, 2, 6, 6, 6, 12, 6, 14, 12, 6, 7, 11, 5, 12, 14, 11, 11, 7,
                                                  11, 19, 22, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                                                  30, 31, 32, 45, 34, 35, 36, 47, 44, 39, 40, 41, 42, 43, 44, 45, 46, 47,
                                                  47, 33, 31, 44, 52, 45, 47, 44, 56, 57, 44, 44, 44, 61, 62, 63, 64, 65, 47, 44, 44, 44, 47, 47, 44, 44, 44, 47, 44, 47, 44, 47, 44, 44, 44, 44, 84};
    { int hsi = d.hairStyle % NUM_HAIRSTYLES; if (hsi < 0 || hsi >= NUM_HAIRSTYLES) hsi = 0; const uint8_t* tab = guy ? HS_B : HS_G;
      L.hair_style = tab[hsi]; }
    if (((d.f.pad >> 5) & 7) && headLoad((d.f.pad >> 5) & 7)) L.hair_style = 63;   // Custom Head: a short cut under it so no doll hair pokes out
    if (getenv("HSDBG")) fprintf(stderr, "hs %d guy %d -> %d\n", d.hairStyle, (int)guy, L.hair_style);
    if (d.hairStyle == 24) L.hair = L.skin;   // bald
    auto lighten = [](RGB c, int k) { return RGB{(uint8_t)std::min(255, c.r + k), (uint8_t)std::min(255, c.g + k), (uint8_t)std::min(255, c.b + k)}; };
    L.topC = d.top; L.topC2 = lighten(d.top, 60); L.botC = d.bottom; L.botC2 = lighten(d.bottom, 50); L.shoeC = d.shoes;
    switch (d.topStyle % NUM_TOPS) {
    case TS_TEE: L.top = guy ? T_BIGTEE : T_CROPTEE; break;
    case TS_TANK: L.top = guy ? T_BIGTEE : T_CAMI; L.topC2 = d.top; break;
    case TS_CROP: L.top = T_CROPTEE; break;
    case TS_HOODIE: case TS_RAINCOAT: L.top = T_HOODIE; L.topC2 = {246, 244, 240}; break;
    case TS_JERSEY: L.top = T_JERSEY; break;
    case TS_BUTTON: case TS_POLO: L.top = T_POLO; break;
    case TS_SWEATER: L.top = T_BIGTEE; break;
    case TS_DRESS: L.top = guy ? T_BIGTEE : T_SLIP; break;
    case TS_JACKET: case TS_BLAZER: L.top = T_MOTO; break;
    case TS_TUBE: L.top = guy ? T_BIGTEE : T_TUBE; break;
    case TS_CARDIGAN: L.top = T_SHRUG; L.topC2 = lighten(d.top, 80); break;
    case TS_SHIRTLESS: L.top = guy ? -1 : T_TUBE; break;   // guys go shirtless (abs show!)
    default: L.top = T_CROPTEE; break;
    }
    if (guy && d.bt % NUM_BOTTOMS == BT_SWIM) L.top = -1;   // swim trunks = shirtless
    if (d.topPattern == 3 && (L.top == T_CROPTEE || L.top == T_BIGTEE)) { L.top = guy ? T_STRIPEPOLO : T_CAMI; L.topC2 = RGB{246, 244, 240}; }
    if (d.topPattern == 8 && L.top == T_CROPTEE) L.top = T_GRAPHIC;
    const bool dress = L.top == T_SLIP;
    switch (d.bt % NUM_BOTTOMS) {
    case BT_PANTS: case BT_OVERALLS: L.bottom = guy ? B_BAGGYJEAN : B_WIDELEG; break;
    case BT_MINI: L.bottom = B_DENIMMINI; break;
    case BT_SHORTS: case BT_SWIM: L.bottom = guy ? B_JORTS : B_MICRO; break;
    case BT_GOWN: case BT_MAXI: L.bottom = guy ? B_BAGGYJEAN : B_MAXI; break;
    case BT_JEANS: L.bottom = guy ? B_BAGGYJEAN : B_FLARE; break;
    case BT_SWEATS: L.bottom = B_TRACKPANT; L.botC2 = {246, 244, 240}; break;
    case BT_PLEATED: L.bottom = guy ? B_CARGO : B_RUFFLE; break;
    default: L.bottom = B_WIDELEG; break;
    }
    if (dress) L.bottom = B_NONE;
    if (!guy && d.bt % NUM_BOTTOMS == BT_GOWN) { L.bottom = B_GOWN; if (L.top == T_SLIP || L.top == T_CROPTEE) L.top = T_TUBE; }
    switch (d.shoeStyle % NUM_SHOES) {
    case SS_SNEAKERS: L.shoes = S_SNEAK; break;
    case SS_SANDALS: L.shoes = guy ? S_FLIPFLOP : S_WEDGE; break;
    case SS_BOOTS: L.shoes = guy ? S_WORKBOOT : S_COMBAT; break;
    case SS_HEELS: L.shoes = guy ? S_SNEAK : S_STRAPPY; break;
    case SS_PLATFORMS: L.shoes = S_PLATFORM; break;
    case SS_LOAFERS: L.shoes = S_WORKBOOT; break;
    default: L.shoes = S_FLIPFLOP; break;
    }
    uint64_t x = 0;
    if (d.acc & ACC_HOOPS) x |= X_HOOPS;
    if (d.acc & (ACC_NECKLACE | ACC_PEARLS)) x |= X_NECK;
    if (d.acc & ACC_BANGLES) x |= X_BANGLES;
    if ((d.acc & ACC_BAG) && !G.hidePurse) x |= X_BAG;
    if (d.acc & ACC_SHADES) x |= X_SHADES;
    if (d.acc & ACC_CHOKER) x |= X_CHOKER;
    if (d.acc & ACC_CLIPS) x |= X_HAIRCLIP;
    if (d.acc & ACC_LEGWARM) x |= X_LEGWARM;
    if (d.acc & ACC_TIGHTS) x |= X_TIGHTS;
    if (d.acc & ACC_TIARA) x |= X_STARCLIP;
    if (d.topPattern == 2) x |= X_LEOPARD;
    if (!((d.f.pad >> 5) & 7)) switch (d.hat) {   // no hats over a Custom Head's hair
    case HT_FLOWERS: x |= X_FLOWER; break;
    case HT_CAP: x |= X_CAP; break;
    case HT_HEADBAND: x |= X_HEADBAND; break;
    default: break;
    }
    if (d.f.glasses) x |= X_GLASSES;
    if (d.f.marks == 1 || d.f.marks == 3) x |= X_FRECKLES;
    if (d.f.pierce == 1) x |= X_STUDS;
    if (d.f.pierce == 2 || d.f.pierce == 4) x |= X_HOOPS;
    if (d.f.pierce >= 3) x |= X_NOSERING;
    if (d.f.beard) x |= X_STUBBLE;
    L.beard = guy ? (uint8_t)(d.f.beard % 10) : 0;
    if ((d.f.pad >> 5) & 7) x &= ~(uint64_t)(X_FLOWER | X_VISOR | X_HEADPH | X_TINTS | X_SHADES | X_STARCLIP | X_FLOWERCLIPS | X_HAIRCLIP | X_HEARTHAIR |
                                             X_FURHAT | X_NEWSBOY | X_HEADBAND | X_WHITEFLOWER | X_GOLDCLIPS | X_CAP);   // Custom Head: nothing on the hair or over the face
    L.extras = x;
    if (x & (X_LEGWARM | X_CAP)) L.accC = d.accCol;
    L.shadow = (!guy && d.f.shadow >= 2) ? shadowRGB(d.f.shadow) : RGB{0, 0, 0};
    return L;
}
static Look castLook(int i) { return CAST[((i % OC_COUNT) + OC_COUNT) % OC_COUNT].look; }
// the tiny face, drawn straight onto a world frame (cell coords; head centre ~ (24, 14))
static void miniFace(const Look& L, uint8_t* f) {
    auto put = [&](int x, int y, Px c, int a = 255) {
        if (x < 0 || y < 0 || x >= WCW || y >= WCH) return;
        uint8_t* o = f + (y * WCW + x) * 4; if (!o[3]) return;
        float k = a / 255.f; o[0] = (uint8_t)(o[0] + (c.r - o[0]) * k); o[1] = (uint8_t)(o[1] + (c.g - o[1]) * k); o[2] = (uint8_t)(o[2] + (c.b - o[2]) * k);
    };
    Px S = rgb(L.skin);
    Px E = sh(lit(rgb(L.eyes), 0.15f), 0.75f), Ed = sh(rgb(L.eyes), 0.45f), LASH = rgb(52, 28, 36), W = rgb(250, 246, 246);
    Px BROW = mixp(sh(rgb(L.hair), 0.65f), rgb(90, 60, 50), 0.4f);
    Px LIP = mixp(rgb(L.lips), rgb(240, 140, 150), 0.3f), LIPd = sh(LIP, 0.8f);
    Px PINK = rgb(246, 140, 150);
    const int ey = 15;
    const bool shades = L.extras & X_SHADES;
    for (int sd = 0; sd < 2; sd++) {
        // left eye pixels at x 19..22, right eye mirrored at 25..28
        auto X = [&](int lx) { return sd == 0 ? lx : 47 - lx; };
        if (!shades) {
            if (L.shadow.r | L.shadow.g | L.shadow.b) for (int lx = 19; lx <= 22; lx++) put(X(lx), ey - 2, rgb(L.shadow), 150);
            put(X(19), ey, W); put(X(20), ey, Ed); put(X(21), ey, E); put(X(22), ey, W, 140);
            put(X(20), ey + 1, E); put(X(21), ey + 1, Ed);
            put(X(21), ey, W);   // catch-light (inner corner)
            put(X(20), ey, Ed);
            if (!L.guy) { for (int lx = 19; lx <= 22; lx++) put(X(lx), ey - 1, LASH); put(X(18), ey - 2, LASH); }
            else for (int lx = 20; lx <= 22; lx++) put(X(lx), ey - 1, LASH, 200);
            {   const int ba = L.guy ? 230 : 170;
                switch (L.brow) {
                case 1: for (int lx = 18; lx <= 21; lx++) put(X(lx), ey - 3, BROW, ba); break;
                case 2: case 4: for (int lx = 18; lx <= 21; lx++) { put(X(lx), ey - 3, BROW, 240); put(X(lx), ey - 4, BROW, L.brow == 4 ? 200 : 120); } break;
                case 3: put(X(21), ey - 3, BROW, ba); put(X(20), ey - 4, BROW, ba); put(X(19), ey - 4, BROW, ba); put(X(18), ey - 3, BROW, ba - 40); break;
                case 5: put(X(20), ey - 3, BROW, 110); put(X(19), ey - 3, BROW, 110); break;
                case 7: put(X(21), ey - 2, BROW, ba); put(X(20), ey - 3, BROW, ba); put(X(19), ey - 4, BROW, ba); break;
                default: for (int lx = 19; lx <= 21; lx++) put(X(lx), ey - 3, BROW, ba); break;
                }
            }
            if (L.eyeShape == 2 || L.eyeShape == 7) put(X(18), ey - 3, LASH, 200);                    // cat / upturned flick
            if (L.eyeShape == 4 || L.eyeShape == 6) for (int lx = 19; lx <= 22; lx++) put(X(lx), ey, sh(S, 0.85f), 120);   // heavy lids
            if (L.eyeShape == 3 || L.eyeShape == 9) put(X(22), ey + 1, W, 160);                       // big doe / wide eyes
            if (!L.guy) put(X(19), ey + 2, PINK, 70);
        } else {
            for (int lx = 18; lx <= 22; lx++) { put(X(lx), ey - 1, rgb(20, 18, 24)); put(X(lx), ey, rgb(20, 18, 24)); if (lx > 18 && lx < 22) put(X(lx), ey + 1, rgb(20, 18, 24)); }
            put(X(19), ey - 1, rgb(120, 110, 130));
        }
        if (L.extras & X_GLASSES) { Px G = rgb(120, 70, 50); for (int lx = 18; lx <= 22; lx++) { put(X(lx), ey - 2, G); put(X(lx), ey + 2, G); } put(X(18), ey - 1, G); put(X(18), ey, G); put(X(18), ey + 1, G); put(X(23), ey - 1, G); put(X(23), ey, G); put(X(23), ey + 1, G); }
        if (L.extras & X_FRECKLES) { put(X(19), ey + 3, rgb(150, 90, 70), 160); put(X(21), ey + 3, rgb(150, 90, 70), 140); }
    }
    if (shades) { put(23, ey - 1, rgb(20, 18, 24)); put(24, ey - 1, rgb(20, 18, 24)); }
    put(23, ey + 2, sh(S, 0.86f), 200); put(24, ey + 2, mixp(S, PINK, 0.25f));   // nose
    if (L.nose == 2 || L.nose == 8) { put(22, ey + 2, sh(S, 0.86f), 140); put(25, ey + 2, sh(S, 0.86f), 140); }
    if (L.nose == 5 || L.nose == 9 || L.nose == 1) put(24, ey + 1, sh(S, 0.88f), 140);
    if (!L.guy) {
        const int w0 = (L.lipSh == 5 || L.lipSh == 9) ? 23 : 22, w1 = (L.lipSh == 5 || L.lipSh == 9) ? 24 : 25;
        for (int x = w0; x <= w1; x++) put(x, ey + 4, L.lipSh == 4 ? rgb(250, 248, 246) : LIP);
        if (L.lipSh != 2) { put(23, ey + 5, LIPd); put(24, ey + 5, LIPd); }
        if (L.lipSh == 3) { put(23, ey + 6, LIP, 160); put(24, ey + 6, LIP, 160); }
        if (L.lipSh == 4) { put(21, ey + 3, LIPd, 160); put(26, ey + 3, LIPd, 160); }
        if (L.lipSh == 8) put(26, ey + 3, LIPd, 160);
        if (L.lipSh != 4) put(23, ey + 4, lit(LIP, 0.35f));
    }
    else { Px N = rgb(L.lips); put(22, ey + 4, sh(N, 0.8f), 160); put(23, ey + 4, sh(N, 0.8f)); put(24, ey + 4, sh(N, 0.8f)); put(25, ey + 4, sh(N, 0.8f), 160); put(23, ey + 5, N); put(24, ey + 5, N);
           if (L.beard >= 2 && L.beard != 6) { Px B = sh(rgb(L.hair), 0.8f); put(22, ey + 6, B, 200); put(23, ey + 6, B); put(24, ey + 6, B); put(25, ey + 6, B, 200); if (L.beard == 3 || L.beard == 7) { put(21, ey + 5, B, 180); put(26, ey + 5, B, 180); } } }
    if (L.extras & X_NOSERING) put(25, ey + 2, rgb(235, 235, 245));
    if (L.extras & X_LIPRING) { put(22, ey + 5, rgb(230, 230, 240)); put(25, ey + 5, rgb(230, 230, 240)); }
}
// 3 frames: stand, walk A, walk B (48x104 each, packed side by side)
void ocBuildWorld(const Look& L, uint8_t* out) {
    g_world = true; paint(L); g_world = false;
    static Px full[CW * CH], walk[CW * CH];
    for (int i = 0; i < CW * CH; i++) full[i] = g_final.p[i];
    for (int fr = 0; fr < 3; fr++) {
        const Px* src = full;
        if (fr) {
            for (auto& p : walk) p = Px{0, 0, 0, 0};
            for (int y = 0; y < CH; y++) for (int x = 0; x < CW; x++) {
                const Px& p = full[y * CW + x];
                if (!p.a) continue;
                int dx = 0;
                if (y >= 112) { int sw = (int)std::lround(std::min(1.f, (y - 112) / 74.f) * 4.f); bool left = x < 48; dx = (fr == 1) == left ? -sw : sw; }
                int nx = x + dx; if (nx < 0 || nx >= CW) continue;
                Px& q = walk[y * CW + nx];
                if (!q.a || (fr == 1 ? x >= 48 : x < 48)) q = p;
            }
            src = walk;
        }
        uint8_t* cell = out + (size_t)fr * WCW * WCH * 4;
        for (int oy = 0; oy < WCH; oy++) for (int ox = 0; ox < WCW; ox++) {
            int r = 0, g = 0, b = 0, n = 0;
            for (int j = 0; j < 2; j++) for (int i = 0; i < 2; i++) { const Px& p = src[(oy * 2 + j) * CW + ox * 2 + i]; if (p.a) { r += p.r; g += p.g; b += p.b; n++; } }
            uint8_t* o = cell + (oy * WCW + ox) * 4;
            if (n >= 2) { o[0] = (uint8_t)(r / n); o[1] = (uint8_t)(g / n); o[2] = (uint8_t)(b / n); o[3] = 255; } else o[0] = o[1] = o[2] = o[3] = 0;
        }
        // crisp 1px outline around the silhouette, tinted from the colour it touches
        static uint8_t tmp[WCW * WCH * 4];
        memcpy(tmp, cell, sizeof tmp);
        for (int y = 0; y < WCH; y++) for (int x = 0; x < WCW; x++) {
            if (tmp[(y * WCW + x) * 4 + 3]) continue;
            const int nx[4] = {x - 1, x + 1, x, x}, ny[4] = {y, y, y - 1, y + 1};
            for (int k = 0; k < 4; k++) {
                if (nx[k] < 0 || ny[k] < 0 || nx[k] >= WCW || ny[k] >= WCH) continue;
                const uint8_t* q = tmp + (ny[k] * WCW + nx[k]) * 4; if (!q[3]) continue;
                uint8_t* o = cell + (y * WCW + x) * 4;
                o[0] = (uint8_t)(q[0] * 0.42f + 18); o[1] = (uint8_t)(q[1] * 0.32f + 8); o[2] = (uint8_t)(q[2] * 0.38f + 14); o[3] = 255;
                break;
            }
        }
        miniFace(L, cell);
    }
}
void ocBuildBig(const Look& L, uint8_t* rgba) {
    paint(L);
    for (int y = 0; y < CH; y++) for (int x = 0; x < CW; x++) { const Px& p = g_final.at(x, y); uint8_t* o = rgba + (y * CW + x) * 4; o[0] = p.r; o[1] = p.g; o[2] = p.b; o[3] = p.a; }
}
Look ocLookForSim(const Sim& sm) {
    int oc = ocIndexOfSim(sm.name, sm.bio);
    if (oc >= 0 && sm.outfit == 0) return castLook(oc);
    Look L = lookFromPerson(sm.look, sm.outfit);
    const int idx = (int)(&sm - G.sims.data()); if (idx >= 0 && idx < (int)G.sims.size()) L.tattoos = (uint8_t)lxTattoos(idx);
    return L;
}
void ocBuildWorldSim(const Sim& sm, uint8_t* out) { ocBuildWorld(ocLookForSim(sm), out); }
void ocPaintSim(const Sim& sm, uint8_t* rgba) { if (!spriteBig(sm, rgba)) { ocBuildBig(ocLookForSim(sm), rgba); ccBakeWear(sm, rgba, CW, CH, 1, 1.f); } }

// ============================================================== Sugar Hearts (otome) portraits
namespace {
const Look OTOME[9] = {
    // Kailyn: long black body waves, black baby tee, phone in hand
    {SK[3], {28, 22, 26}, {78, 48, 32}, {196, 112, 112}, 37, T_CROPTEE, B_BAGGYJEAN, S_SNEAK, {28, 24, 30}, {60, 54, 64}, {90, 110, 150}, {0, 0, 0}, {245, 245, 245}, X_PHONE | X_HOOPS | X_NAILS, false},
    // 1 the Bookworm: glasses, white tank, gold chain
    {SK[5], {22, 18, 18}, {60, 38, 26}, {150, 96, 86}, 6, T_JERSEYTANK, B_BAGGYJEAN, S_SNEAK, {246, 246, 242}, {230, 230, 226}, {60, 70, 100}, {0, 0, 0}, {240, 240, 240}, X_GLASSES | X_NECK | X_STUDS, true},
    // 2 the Calm Friend: messy wolf cut, grey hoodie
    {SK[2], {26, 22, 24}, {70, 46, 32}, {176, 120, 104}, 24, T_HOODIE, B_TRACKPANT, S_SNEAK, {168, 168, 176}, {210, 210, 216}, {70, 70, 80}, {0, 0, 0}, {240, 240, 240}, X_STUDS, true},
    // 3 the Artist: teal cap, clear frames, chains, cream tee
    {SK[4], {30, 22, 20}, {64, 40, 28}, {150, 92, 82}, 12, T_BIGTEE, B_CARGO, S_CHUNKY, {240, 234, 220}, {255, 250, 240}, {80, 80, 70}, {0, 0, 0}, {250, 250, 250}, X_CAP | X_GLASSES | X_NECK | X_SILVERCHAIN | X_STUDS, true, {0, 0, 0}, {40, 160, 170}},
    // 4 the Rebel: curly top, black tee + tactical vest, stubble
    {SK[5], {20, 16, 16}, {54, 34, 24}, {140, 90, 80}, 44, T_DENIMVEST, B_CARGO, S_COMBAT, {34, 34, 38}, {58, 58, 62}, {40, 40, 44}, {0, 0, 0}, {30, 30, 30}, X_STUBBLE | X_STUDS, true},
    // 5 the Sincere Heart: warm smile, diamond studs, black tee
    {SK[4], {24, 18, 18}, {70, 44, 30}, {156, 98, 88}, 6, T_BIGTEE, B_BAGGYJEAN, S_SNEAK, {30, 30, 34}, {50, 50, 56}, {70, 90, 130}, {0, 0, 0}, {245, 245, 245}, X_STUDS, true},
    // 6 the Athlete: navy jersey #1
    {SK[4], {24, 18, 18}, {66, 42, 28}, {150, 94, 84}, 6, T_JERSEY, B_TRACKPANT, S_SNEAK, {28, 44, 120}, {226, 222, 90}, {40, 50, 110}, {0, 0, 0}, {245, 245, 245}, X_WRISTBAND, true},
    // 7 the Romantic: wavy brown hair, light-blue button-down, freckles, green eyes
    {SK[1], {112, 72, 44}, {88, 140, 96}, {200, 130, 120}, 47, T_POLO, B_WIDELEG, S_SNEAK, {176, 204, 236}, {220, 234, 250}, {230, 226, 214}, {0, 0, 0}, {120, 80, 50}, X_FRECKLES, true},
    // 8 (spare)
    {SK[3], {28, 22, 26}, {78, 48, 32}, {196, 112, 112}, 37, T_CROPTEE, B_BAGGYJEAN, S_SNEAK, {28, 24, 30}, {60, 54, 64}, {90, 110, 150}, {0, 0, 0}, {245, 245, 245}, 0, false},
};
// Starlight Bay High (the spin-off): the Season 1 cast
const Look SBH[7] = {
    // 0 (you are imported from Sugar Bunny Life; this is a fallback)
    {SK[3], {28, 22, 26}, {78, 48, 32}, {196, 112, 112}, 37, T_CROPTEE, B_BAGGYJEAN, S_SNEAK, {150, 90, 110}, {200, 150, 170}, {90, 110, 150}, {0, 0, 0}, {245, 245, 245}, X_HOOPS, false},
    // 1 Madison Vale: queen bee, blonde blowout, pink cardigan, pearls
    {SK[0], {232, 204, 140}, {90, 130, 170}, {226, 120, 140}, 19, T_PINCARDIGAN, B_PLAID, S_STRAPPY, {236, 170, 190}, {255, 230, 236}, {170, 90, 110}, {120, 60, 80}, {240, 230, 230}, X_NECK | X_HEADBAND | X_STUDS, false, {0, 0, 0}, {255, 255, 255}},
    // 2 Jade Rivera: your best friend, artsy, knotless braids, denim vest
    {SK[4], {40, 26, 22}, {70, 44, 30}, {200, 110, 110}, 31, T_DENIMVEST, B_WIDELEG, S_CHUNKY, {120, 150, 190}, {250, 240, 220}, {230, 200, 160}, {0, 0, 0}, {250, 250, 250}, X_HOOPS | X_BANGLES | X_FLOWERCLIPS, false},
    // 3 Cole Hart: golden-boy quarterback, letterman red
    {SK[1], {200, 160, 100}, {80, 130, 170}, {200, 140, 130}, 47, T_JERSEY, B_BAGGYJEAN, S_SNEAK, {170, 40, 50}, {250, 250, 250}, {70, 80, 110}, {0, 0, 0}, {245, 245, 245}, X_WRISTBAND, true},
    // 4 Theo Park: the school blog writer, wolf cut, black hoodie
    {SK[2], {24, 20, 22}, {60, 40, 30}, {176, 120, 104}, 24, T_HOODIE, B_TRACKPANT, S_COMBAT, {36, 34, 40}, {70, 66, 76}, {50, 50, 56}, {0, 0, 0}, {30, 30, 30}, X_STUDS | X_HEADPH, true},
    // 5 Bree Thompson: runs @bayhightea, curls, polka-dot tube top
    {SK[3], {60, 34, 26}, {78, 50, 34}, {214, 100, 130}, 39, T_POLKATUBE, B_DENIMMINI, S_PLATFORM, {200, 120, 170}, {255, 240, 250}, {90, 110, 160}, {0, 0, 0}, {240, 230, 230}, X_BIGHOOPS | X_PHONE | X_NAILS, false},
    // 6 Dante Wells: student council president, polo, ambitious
    {SK[5], {20, 16, 16}, {60, 38, 26}, {150, 96, 86}, 46, T_POLO, B_WIDELEG, S_SNEAK, {200, 190, 160}, {240, 236, 224}, {80, 70, 60}, {0, 0, 0}, {245, 245, 245}, X_GLASSES | X_CROSS, true},
};
}   // namespace
void ocSbhPaint(int i, uint8_t* rgba) { ocBuildBig(SBH[((i % 7) + 7) % 7], rgba); }
void ocOtomePaint(int i, uint8_t* rgba) { ocBuildBig(OTOME[((i % 9) + 9) % 9], rgba); }
#ifndef OC_HOST
// ============================================================== 3DS texture + drawing
#include "draw.hpp"
static C3D_Tex g_tex[OC_COUNT];
static bool g_ready[OC_COUNT];
static bool ocTexture(int i) {
    i = ((i % OC_COUNT) + OC_COUNT) % OC_COUNT;
    if (g_ready[i]) return true;
    if (!C3D_TexInit(&g_tex[i], 128, 256, GPU_RGBA8)) return false;
    C3D_TexSetFilter(&g_tex[i], GPU_NEAREST, GPU_NEAREST);
    static uint8_t buf[CW * CH * 4];
    int w, h;
    ocPaint(i, buf, w, h);
    u32* px = (u32*)g_tex[i].data;
    memset(px, 0, 128 * 256 * 4);
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
        const uint8_t* s = buf + (y * w + x) * 4;
        int X = x, Y = y;   // 8x8 morton tiles, row 0 = top (as the 3DS reads them)
        u32 off = ((Y >> 3) * (128 >> 3) + (X >> 3)) * 64;
        u32 m = (X & 1) | ((Y & 1) << 1) | ((X & 2) << 1) | ((Y & 2) << 2) | ((X & 4) << 2) | ((Y & 4) << 3);
        px[off + m] = (u32)s[0] << 24 | (u32)s[1] << 16 | (u32)s[2] << 8 | s[3];
    }
    GSPGPU_FlushDataCache(g_tex[i].data, g_tex[i].size);
    g_ready[i] = true;
    return true;
}
// your own designs on a cast model (story mode runway shows)
static C3D_Tex g_dtex[4];
static bool g_dready[4];
static void uploadTo(C3D_Tex& tx, const uint8_t* buf) {
    u32* px = (u32*)tx.data;
    memset(px, 0, 128 * 256 * 4);
    for (int y = 0; y < CH; y++) for (int x = 0; x < CW; x++) {
        const uint8_t* s = buf + (y * CW + x) * 4;
        int X = x, Y = y;
        u32 off = ((Y >> 3) * (128 >> 3) + (X >> 3)) * 64;
        u32 m = (X & 1) | ((Y & 1) << 1) | ((X & 2) << 1) | ((Y & 2) << 2) | ((X & 4) << 2) | ((Y & 4) << 3);
        px[off + m] = (u32)s[0] << 24 | (u32)s[1] << 16 | (u32)s[2] << 8 | s[3];
    }
    GSPGPU_FlushDataCache(tx.data, tx.size);
}
void ocDesignSet(int slot, int model, int top, int bottom, int shoes, RGB c1, RGB c2, RGB c3) {
    if (slot < 0 || slot >= 4) return;
    if (!g_dready[slot]) { if (!C3D_TexInit(&g_dtex[slot], 128, 256, GPU_RGBA8)) return; C3D_TexSetFilter(&g_dtex[slot], GPU_NEAREST, GPU_NEAREST); g_dready[slot] = true; }
    static uint8_t buf[CW * CH * 4];
    ocPaintDesign(model, top, bottom, shoes, c1, c2, c3, buf);
    uploadTo(g_dtex[slot], buf);
}
void ocDesignDraw(int slot, float x, float y, float scale) {
    if (slot < 0 || slot >= 4 || !g_dready[slot]) return;
    static Tex3DS_SubTexture sub = {(u16)CW, (u16)CH, 0.f, 1.f, CW / 128.f, 1.f - CH / 256.f};
    C2D_Image img = {&g_dtex[slot], &sub};
    C2D_DrawImageAt(img, std::floor(x), std::floor(y), 0.5f, nullptr, scale, scale);
}
void ocDraw(int i, float x, float y, float scale) {
    if (!ocTexture(i)) return;
    i = ((i % OC_COUNT) + OC_COUNT) % OC_COUNT;
    static Tex3DS_SubTexture sub = {(u16)CW, (u16)CH, 0.f, 1.f, CW / 128.f, 1.f - CH / 256.f};
    C2D_Image img = {&g_tex[i], &sub};
    C2D_DrawImageAt(img, std::floor(x), std::floor(y), 0.5f, nullptr, scale, scale);
}



// ---- doll cache: in-world frames for anyone on screen, plus a few full-size ones for close-ups
#include "sim.hpp"
namespace {
struct Rig { uint8_t ok = 0, top = 0, headEnd = 14, hip = 46, cx = 24, aL0 = 12, aL1 = 15, aR0 = 31, aR1 = 34; };
struct Slot { uint32_t key = 0, used = 0; C3D_Tex tex; bool init = false; Rig rig; };
// body sheet: cut the idle frame into head / arms / torso / legs from its own silhouette
static Rig autoRig(const uint8_t* buf, int w, int h) {
    Rig r; int top = -1, bot = 0;
    auto op = [&](int x, int y) { return buf[((size_t)y * w + x) * 4 + 3] > 40; };
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) if (op(x, y)) { if (top < 0) top = y; bot = y; break; }
    if (top < 0 || bot - top < 30) return r;
    const int hh = bot - top; r.top = (uint8_t)top; r.headEnd = (uint8_t)(top + hh * 16 / 100); r.hip = (uint8_t)(top + hh / 2);
    int L = w, R = 0; for (int y = r.headEnd + 2; y < r.hip; y++) for (int x = 0; x < w; x++) if (op(x, y)) { L = std::min(L, x); R = std::max(R, x); }
    int cl = w, cr = 0; for (int y = r.hip - 4; y < r.hip; y++) for (int x = 0; x < w; x++) if (op(x, y)) { cl = std::min(cl, x); cr = std::max(cr, x); }
    if (L >= R) return r;
    const int aw = std::max(3, ((R - L + 1) - (cr - cl + 1)) / 2 + 2);
    r.cx = (uint8_t)((L + R) / 2); r.aL0 = (uint8_t)L; r.aL1 = (uint8_t)std::min(L + aw, (int)r.cx - 2); r.aR1 = (uint8_t)(R + 1); r.aR0 = (uint8_t)std::max(R + 1 - aw, (int)r.cx + 2); r.ok = 1;
    return r;
}
Slot g_ws[36], g_bs[4];
uint32_t g_frameNo = 1; int g_budget = 1;
uint32_t simKey(const Sim& s) {
    int oc = ocIndexOfSim(s.name, s.bio);
    uint32_t h = 2166136261u;
    auto mix = [&](uint32_t v) { h = (h ^ v) * 16777619u; };
    mix(spriteKey(s)); mix(ccWearKey(s)); mix(g_dollV2 ? 0xD011u : 0u);
    { const int idx = (int)(&s - G.sims.data()); if (idx >= 0 && idx < (int)G.sims.size()) mix(0x7A700u + lxTattoos(idx)); }
    if (oc >= 0 && s.outfit == 0) mix(0xC457u + oc);
    else {
        const PersonDef& d = s.look;
        mix(d.guy); mix(d.skin); mix(d.hairCol); mix(d.hairStyle); mix(d.top.r | d.top.g << 8 | d.top.b << 16); mix(d.bottom.r | d.bottom.g << 8 | d.bottom.b << 16);
        mix(d.bt); mix(d.topPattern); if (d.topPattern >= 100) mix(dsVersion(d.topPattern - 100)); mix(d.shoes.r | d.shoes.g << 8 | d.shoes.b << 16); mix(d.lips.r | d.lips.g << 8 | d.lips.b << 16); mix(d.topStyle); mix(d.shoeStyle); mix(d.acc);
        mix(d.f.eyeCol); mix(d.f.eyes); mix(d.f.brows); mix(d.f.nose); mix(d.f.lipShape); mix(d.f.shadow); mix(d.f.glasses); mix(d.f.marks); mix(d.f.pierce); mix(d.f.beard); mix(d.f.pad); mix(d.f.shape); mix(s.outfit);
    }
    return h ? h : 1;
}
template <int N> Slot* findSlot(Slot (&v)[N], uint32_t key, bool& fresh) {
    for (Slot& s : v) if (s.key == key) { s.used = g_frameNo; fresh = false; return &s; }
    fresh = true;
    if (g_budget <= 0) return nullptr;
    Slot* best = nullptr;
    for (Slot& s : v) { if (s.used == g_frameNo) continue; if (!best || s.used < best->used) best = &s; }
    if (!best) return nullptr;
    g_budget--;
    best->key = key; best->used = g_frameNo;
    return best;
}
void upload(C3D_Tex& tx, int TW, int TH, const uint8_t* buf, int w, int h, int cells) {
    u32* px = (u32*)tx.data;
    memset(px, 0, TW * TH * 4);
    for (int c = 0; c < cells; c++) for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
        const uint8_t* s = buf + ((size_t)c * w * h + y * w + x) * 4;
        int X = c * w + x, Y = y;   // the 3DS reads these tiles top-down: row 0 is the top of the doll (v = 1)
        u32 off = ((Y >> 3) * (TW >> 3) + (X >> 3)) * 64;
        u32 m = (X & 1) | ((Y & 1) << 1) | ((X & 2) << 1) | ((Y & 2) << 2) | ((X & 4) << 2) | ((Y & 4) << 3);
        px[off + m] = (u32)s[0] << 24 | (u32)s[1] << 16 | (u32)s[2] << 8 | s[3];
    }
    GSPGPU_FlushDataCache(tx.data, tx.size);
}
Slot* worldSlot(const Sim& sm) {
    bool fresh; Slot* s = findSlot(g_ws, simKey(sm), fresh);
    if (!s || !fresh) return s;
    if (!s->init) { if (!C3D_TexInit(&s->tex, 256, 128, GPU_RGBA8)) { s->key = 0; return nullptr; } C3D_TexSetFilter(&s->tex, GPU_NEAREST, GPU_NEAREST); s->init = true; }
    static uint8_t buf[3 * WCW * WCH * 4];
    if (!spriteWorld(sm, buf)) { ocBuildWorldSim(sm, buf); ccBakeWear(sm, buf, WCW, WCH, 3, 0.5f); }
    s->rig = autoRig(buf, WCW, WCH);
    upload(s->tex, 256, 128, buf, WCW, WCH, 3);
    return s;
}
Slot* bigSlot(const Sim& sm) {
    bool fresh; Slot* s = findSlot(g_bs, simKey(sm), fresh);
    if (!s || !fresh) return s;
    if (!s->init) { if (!C3D_TexInit(&s->tex, 128, 256, GPU_RGBA8)) { s->key = 0; return nullptr; } C3D_TexSetFilter(&s->tex, GPU_NEAREST, GPU_NEAREST); s->init = true; }
    static uint8_t buf[CW * CH * 4];
    if (!spriteBig(sm, buf)) { ocBuildBig(ocLookForSim(sm), buf); ccBakeWear(sm, buf, CW, CH, 1, 1.f); }
    upload(s->tex, 128, 256, buf, CW, CH, 1);
    return s;
}
float stageMul(const Sim& s) { return s.stage == ST_INFANT ? 0.42f : s.stage == ST_TODDLER ? 0.56f : s.stage == ST_CHILD ? 0.74f : s.stage == ST_PRETEEN ? 0.84f : s.stage == ST_TEEN ? 0.94f : 1.f; }
}   // namespace
void ocFrameTick() { g_frameNo++; g_budget = 2; }
float ocDollHeight(const Sim& s) { return 86.f * stageMul(s); }
bool g_rigOn = false; float g_rig[5] = {};
int ocDrawSim(const Sim& sm, float x, float footY, float scale, int pose, bool flip, float t, float walkPhase) {
    if (sm.stage < ST_CHILD && !spriteKey(sm)) return 0;   // babies/toddlers use sprites when there are any
    if (sm.stage < ST_CHILD && (pose == 6 || pose == 7 || pose == 8)) pose = 0;
    float sc = scale * stageMul(sm);
    if (sc > 1.2f) {   // close-ups (Create-a-Sim, wardrobe): the full-detail doll
        Slot* s = bigSlot(sm);
        if (!s) return 2;
        float k = std::max(1.f, std::floor(sc * 0.5f + 0.25f));   // whole-pixel scale: the anti-aliased doll stays crisp, never blurry
        static Tex3DS_SubTexture sub = {(u16)CW, (u16)CH, 0.f, 1.f, CW / 128.f, 1.f - CH / 256.f};
        C2D_Image img = {&s->tex, &sub};
        float x0 = x - 48 * k, y0 = footY - 186 * k;
        if (flip) C2D_DrawImageAt(img, std::floor(x0 + CW * k), std::floor(y0), 0.5f, nullptr, -k, k);
        else C2D_DrawImageAt(img, std::floor(x0), std::floor(y0), 0.5f, nullptr, k, k);
        return 1;
    }
    Slot* s = worldSlot(sm);
    if (!s) return 2;
    // pose ids (people.hpp): 0 idle, 1 phone, 2 dance, 3 sleep, 4 wave, 5 walk, 6 sit, 7 lie, 8 sit-sleep
    int fr = pose == 5 ? (walkPhase < 0.5f ? 1 : 2) : 0;
    float bob = 0, sway = 0;
    if (pose == 5) bob = -std::floor(std::fabs(std::sin(walkPhase * 6.2832f)) * 1.6f + 0.5f);
    if (pose == 2) { bob = -std::floor(std::fabs(std::sin(t * 7.f)) * 3.f); sway = std::floor(std::sin(t * 3.5f) * 3.f); flip = std::sin(t * 1.75f) > 0; }
    bool sit = pose == 6 || pose == 8;
    if (sit && s->rig.ok && g_rigOn) {   // rigged sitting: hips on the seat, thighs forward, shins hanging down
        const Rig& R = s->rig;
        const float kneeY = R.hip + (93 - R.hip) * 0.48f;
        C3D_Mtx m; C2D_ViewSave(&m);
        C2D_ViewTranslate(std::floor(x), std::floor(footY - R.hip * sc)); C2D_ViewScale(flip ? -sc : sc, sc); C2D_ViewTranslate(-24, 0);
        auto partT = [&](int x0, int y0, int x1, int y1, float px, float py, float ang, float tx, float ty) {
            if (x1 <= x0 || y1 <= y0) return;
            Tex3DS_SubTexture sb = {(u16)(x1 - x0), (u16)(y1 - y0), x0 / 256.f, 1.f - y0 / 128.f, x1 / 256.f, 1.f - y1 / 128.f};
            C2D_Image im = {&s->tex, &sb};
            C3D_Mtx pm; C2D_ViewSave(&pm); C2D_ViewTranslate(tx, ty); C2D_ViewRotate(ang);
            C2D_DrawImageAt(im, x0 - px, y0 - py, 0.5f, nullptr, 1, 1);
            C2D_ViewRestore(&pm);
        };
        const float a = -1.45f, L = kneeY - R.hip;   // thighs point forward (to the right before flipping)
        const float kx = -std::sin(a) * L, ky = std::cos(a) * L;
        for (int side = 0; side < 2; side++) {
            const int x0 = side ? R.cx : 0, x1 = side ? WCW : R.cx; const float px = side ? R.cx + 4.f : R.cx - 4.f;
            partT(x0, (int)kneeY, x1, WCH, px, kneeY, 0, px + kx, R.hip + ky);   // shin hangs straight down from the knee
            partT(x0, R.hip - 1, x1, (int)kneeY + 1, px, R.hip, a, px, R.hip); // thigh
        }
        partT(0, 0, WCW, R.hip + 2, 0, 0, 0, 0, 0);   // whole upper body
        C2D_ViewRestore(&m);
        return 1;
    }
    if (!sit && s->rig.ok && g_rigOn) {   // cut-out rig: draw the 6 body-sheet parts, each rotated about its joint
        const Rig& R = s->rig;
        float aL = g_rig[0], aR = g_rig[1], lL = g_rig[2], lR = g_rig[3], hd = g_rig[4];
        if (pose == 5) { const float k = std::sin(walkPhase * 6.2832f); lL += 0.32f * k; lR -= 0.32f * k; aL -= 0.22f * k; aR += 0.22f * k; hd += 0.03f * k; }
        if (pose == 2) { const float k = std::sin(t * 7.f); aL -= 0.35f + 0.25f * k; aR += 0.35f - 0.25f * k; lL += 0.12f * k; lR += 0.12f * k; hd += 0.08f * std::sin(t * 3.5f); }
        aL = std::max(-0.25f, std::min(0.25f, aL)); aR = std::max(-0.25f, std::min(0.25f, aR));   // arms share pixels with hair: keep their swing small
        const float bodyBob = pose == 5 ? -std::fabs(std::sin(walkPhase * 6.2832f)) * 1.2f : pose == 2 ? -std::fabs(std::sin(t * 7.f)) * 2.5f : 0;
        C3D_Mtx m; C2D_ViewSave(&m);
        C2D_ViewTranslate(std::floor(x), std::floor(footY - 93 * sc + bodyBob)); C2D_ViewScale(flip ? -sc : sc, sc); C2D_ViewTranslate(-24, 0);
        auto part = [&](int x0, int y0, int x1, int y1, float px, float py, float ang) {
            if (x1 <= x0 || y1 <= y0) return;
            Tex3DS_SubTexture sb = {(u16)(x1 - x0), (u16)(y1 - y0), x0 / 256.f, 1.f - y0 / 128.f, x1 / 256.f, 1.f - y1 / 128.f};
            C2D_Image im = {&s->tex, &sb};
            C3D_Mtx pm; C2D_ViewSave(&pm); C2D_ViewTranslate(px, py); C2D_ViewRotate(ang);
            C2D_DrawImageAt(im, x0 - px, y0 - py, 0.5f, nullptr, 1, 1);
            C2D_ViewRestore(&pm);
        };
        const int hipO = R.hip - 1;   // 1px overlaps hide the seams when parts rotate
        part(0, hipO, R.cx, WCH, R.cx - 4, R.hip, lL);                                  // left leg (pivot: hip)
        part(R.cx, hipO, WCW, WCH, R.cx + 4, R.hip, lR);                                // right leg
        part(R.aL1 - 1, R.headEnd - 1, R.aR0 + 1, R.hip + 2, 0, 0, 0);                   // torso
        part(0, 0, WCW, R.headEnd + 1, R.cx, R.headEnd, hd);                              // head (pivot: neck)
        part(0, R.headEnd, R.aL1, R.hip + 6, R.aL0 + 2, R.headEnd + 3, aL);               // left arm (pivot: shoulder)
        part(R.aR0, R.headEnd, WCW, R.hip + 6, R.aR1 - 2, R.headEnd + 3, aR);             // right arm
        C2D_ViewRestore(&m);
        return 1;
    }
    int rows = sit ? 61 : WCH;
    Tex3DS_SubTexture sub = {(u16)WCW, (u16)rows, fr * WCW / 256.f, 1.f, (fr * WCW + WCW) / 256.f, 1.f - rows / 128.f};
    C2D_Image img = {&s->tex, &sub};
    float x0 = std::floor(x - 24 * sc + sway), y0 = std::floor(sit ? footY - 61 * sc + 2 * sc : footY - 93 * sc + bob);
    if (flip) C2D_DrawImageAt(img, x0 + std::floor(WCW * sc), y0, 0.5f, nullptr, -sc, sc);
    else C2D_DrawImageAt(img, x0, y0, 0.5f, nullptr, sc, sc);
    return 1;
}
bool ocDrawSimBust(const Sim& sm, float cx, float cy, float r) {
    if (sm.stage < ST_CHILD && !spriteKey(sm)) return false;
    if (r > 22) {
        Slot* s = bigSlot(sm);
        if (!s) return true;
        const int bx = 22, by = 4, bw = 52, bh = 56;
        float k = (2 * r) / bw;
        Tex3DS_SubTexture sub = {(u16)bw, (u16)bh, bx / 128.f, 1.f - by / 256.f, (bx + bw) / 128.f, 1.f - (by + bh) / 256.f};
        C2D_Image img = {&s->tex, &sub};
        C2D_DrawImageAt(img, std::floor(cx - r), std::floor(cy - r * 1.05f), 0.5f, nullptr, k, k);
        return true;
    }
    Slot* s = worldSlot(sm);
    if (!s) return true;
    const int bx = 10, by = 1, bw = 28, bh = 29;
    float k = std::max(1.f, std::floor((2 * r) / bw + 0.25f));   // whole-pixel scaling stays crisp
    if ((2 * r) / bw < 0.9f) k = (2 * r) / bw;
    Tex3DS_SubTexture sub = {(u16)bw, (u16)bh, bx / 256.f, 1.f - by / 128.f, (bx + bw) / 256.f, 1.f - (by + bh) / 128.f};
    C2D_Image img = {&s->tex, &sub};
    C2D_DrawImageAt(img, std::floor(cx - bw * k / 2), std::floor(cy - bh * k * 0.55f), 0.5f, nullptr, k, k);
    return true;
}

// ============================================================== the cast in town
#include "sim.hpp"
#include "audio.hpp"
static int nearestSkin(RGB c) { int b = 0; long bd = 1L << 30; for (int i = 0; i < NUM_SKINS; i++) { RGB s = skinRGB(i); long d = (long)(s.r - c.r) * (s.r - c.r) + (long)(s.g - c.g) * (s.g - c.g) + (long)(s.b - c.b) * (s.b - c.b); if (d < bd) { bd = d; b = i; } } return b; }
static int nearestHair(RGB c) { int b = 0; long bd = 1L << 30; for (int i = 0; i < NUM_HAIRCOLS; i++) { RGB s = hairRGB(i); long d = (long)(s.r - c.r) * (s.r - c.r) + (long)(s.g - c.g) * (s.g - c.g) + (long)(s.b - c.b) * (s.b - c.b); if (d < bd) { bd = d; b = i; } } return b; }
int ocSim(int i) {
    const OcDef& d = ocDef(i);
    for (size_t k = 0; k < G.sims.size(); k++) if (G.sims[k].name == d.name && G.sims[k].bio.rfind("[Cast]", 0) == 0) return (int)k;   // townie, or you if you're playing as them
    return -1;
}
void ocEnsureInTown() {
    if (G.sims.empty()) return;
    for (int i = 0; i < OC_COUNT; i++) {
        if (ocSim(i) >= 0) continue;
        G.sims.push_back(ocMakeSim(i));
    }
}
Sim ocMakeSim(int i) {
    i = ((i % OC_COUNT) + OC_COUNT) % OC_COUNT;
    {
    static const uint8_t HS[OC_COUNT] = {1, 4, 7, 2, 3, 22, 16, 27,   0, 0, 19, 7, 5,   0, 1, 0, 0, 8, 3,
                                         0, 0, 17, 3, 18, 6, 0, 18, 6, 2, 0, 0, 0, 12, 2, 21,   3, 4, 16,
                                         1, 0, 13, 6, 0, 0, 4, 8, 0, 2, 6, 1, 0, 21};
    static const uint8_t TOPS[OC_COUNT] = {TS_CROP, TS_JERSEY, TS_TANK, TS_HOODIE, TS_DRESS, TS_CROP, TS_TEE, TS_JERSEY,
                                           TS_JACKET, TS_TANK, TS_TEE, TS_CARDIGAN, TS_POLO,   TS_TANK, TS_CROP, TS_TUBE, TS_TUBE, TS_CROP, TS_CROP,
                                           TS_TUBE, TS_CROP, TS_HOODIE, TS_TANK, TS_POLO, TS_DRESS, TS_TANK, TS_POLO, TS_DRESS, TS_CROP, TS_TANK, TS_JACKET, TS_CROP, TS_HOODIE, TS_TUBE, TS_TANK,   TS_JACKET, TS_JACKET, TS_TEE,
                                           TS_JACKET, TS_TANK, TS_CROP, TS_TANK, TS_CROP, TS_TUBE, TS_TANK, TS_CARDIGAN, TS_TANK, TS_TANK, TS_SWEATER, TS_CROP, TS_TUBE, TS_CARDIGAN};
    static const uint8_t BOTS[OC_COUNT] = {BT_PLEATED, BT_SHORTS, BT_JEANS, BT_SWEATS, BT_MINI, BT_MINI, BT_JEANS, BT_SHORTS,
                                           BT_JEANS, BT_MAXI, BT_JEANS, BT_JEANS, BT_JEANS,   BT_PANTS, BT_JEANS, BT_SHORTS, BT_MINI, BT_MAXI, BT_MAXI,
                                           BT_PANTS, BT_PANTS, BT_JEANS, BT_JEANS, BT_SHORTS, BT_GOWN, BT_MAXI, BT_JEANS, BT_MINI, BT_JEANS, BT_MAXI, BT_JEANS, BT_SHORTS, BT_MINI, BT_MINI, BT_PANTS,   BT_PANTS, BT_PANTS, BT_JEANS,
                                           BT_JEANS, BT_JEANS, BT_SWEATS, BT_MINI, BT_MINI, BT_SHORTS, BT_PANTS, BT_SHORTS, BT_MAXI, BT_JEANS, BT_PLEATED, BT_MINI, BT_MINI, BT_MINI};
    static const uint8_t SHOES[OC_COUNT] = {SS_HEELS, SS_SNEAKERS, SS_PLATFORMS, SS_SNEAKERS, SS_BOOTS, SS_BOOTS, SS_BOOTS, SS_SNEAKERS,
                                            SS_BOOTS, SS_SANDALS, SS_SNEAKERS, SS_SNEAKERS, SS_SNEAKERS,   SS_FLIPFLOPS, SS_PLATFORMS, SS_HEELS, SS_SANDALS, SS_PLATFORMS, SS_FLIPFLOPS,
                                            SS_SNEAKERS, SS_SNEAKERS, SS_SNEAKERS, SS_PLATFORMS, SS_SNEAKERS, SS_HEELS, SS_FLIPFLOPS, SS_SNEAKERS, SS_BOOTS, SS_SNEAKERS, SS_SANDALS, SS_BOOTS, SS_SNEAKERS, SS_SNEAKERS, SS_PLATFORMS, SS_HEELS,   SS_HEELS, SS_BOOTS, SS_BOOTS,
                                            SS_PLATFORMS, SS_SNEAKERS, SS_SNEAKERS, SS_HEELS, SS_BOOTS, SS_SANDALS, SS_HEELS, SS_BOOTS, SS_HEELS, SS_SNEAKERS, SS_HEELS, SS_SLIPPERS, SS_PLATFORMS, SS_BOOTS};
        const Entry& e = CAST[i];
        Sim s = randomSim(e.def.stage, e.def.guy ? 1 : 0);
        s.name = e.def.name;
        s.bio = std::string("[Cast] ") + e.def.title;
        s.household = false;
        s.look.skin = (uint8_t)nearestSkin(e.look.skin);
        s.look.hairCol = (uint8_t)nearestHair(e.look.hair);
        s.look.hairStyle = HS[i];
        s.look.top = e.look.topC; s.look.bottom = e.look.botC; s.look.shoes = e.look.shoeC; s.look.lips = e.look.lips;
        s.look.topStyle = TOPS[i] % NUM_TOPS; s.look.bt = (Bottom)(BOTS[i] % NUM_BOTTOMS); s.look.shoeStyle = SHOES[i]; s.look.topPattern = 0; s.outfit = 0;
        s.look.acc = (e.look.extras & (X_BAG | X_TOTE) ? ACC_BAG : 0) | (e.look.extras & X_NECK ? ACC_NECKLACE : 0) | (e.look.extras & X_HOOPS ? ACC_HOOPS : 0) | (e.look.extras & X_BANGLES ? ACC_BANGLES : 0);
        s.look.f.blush = 0;
        if (e.look.shadow.r | e.look.shadow.g | e.look.shadow.b) { s.look.f.shadow = 5; s.look.f.liner = 1; }
        if (e.look.extras & X_LIPRING) s.look.f.pierce = 7;
        if (e.look.extras & X_STUBBLE) s.look.f.beard = 1;
        return s;
    }
}

// ============================================================== Cast phone app
static int g_sel = 0, g_page = 0;
constexpr int PER_PAGE = 12;
static Btn castB(int i) { return Btn{6 + (i % 3) * 104.f, 28 + (i / 3) * 31.f, 100, 27}; }   // slot on the current page
static const Btn bMore{214, 210, 100, 26};
static const Btn bText{6, 176, 100, 28}, bInvite{110, 176, 100, 28}, bDate{214, 176, 100, 28};
void ocAppUpdate(const Input& in) {
    if (in.down & KEY_DRIGHT) { g_sel = (g_sel + 1) % OC_COUNT; g_page = g_sel / PER_PAGE; sfxTap(); }
    if (in.down & KEY_DLEFT) { g_sel = (g_sel + OC_COUNT - 1) % OC_COUNT; g_page = g_sel / PER_PAGE; sfxTap(); }
    if (!in.touch) return;
    if (bMore.in(in.tp)) { g_page = (g_page + 1) % ((OC_COUNT + PER_PAGE - 1) / PER_PAGE); sfxTap(); return; }
    for (int k = 0; k < PER_PAGE; k++) { int i = g_page * PER_PAGE + k; if (i < OC_COUNT && castB(k).in(in.tp)) { g_sel = i; sfxTap(); return; } }
    int sim = ocSim(g_sel);
    if (sim < 0) { ocEnsureInTown(); sim = ocSim(g_sel); }
    if (sim < 0) return;
    if (bText.in(in.tp)) { rel(G.mc, sim).met = true; rel(G.mc, sim).matched = true; sendText(sim, 0); toast("You texted " + G.sims[sim].name + "!"); sfxTap(); }
    else if (bInvite.in(in.tp)) { rel(G.mc, sim).met = true; rel(G.mc, sim).matched = true; sendText(sim, 2); sfxTap(); }
    else if (bDate.in(in.tp) && MC().stage >= ST_TEEN && canRomance(G.mc, sim)) { rel(G.mc, sim).met = true; rel(G.mc, sim).matched = true; sendText(sim, 3); sfxTap(); }
}
void ocAppDrawTop(float t) {
    (void)t;
    grad(0, 0, TOP_W, TOP_H, C(250, 236, 244), C(232, 222, 246));
    for (int i = 0; i < 12; i++) sparkle(std::fmod(i * 73.f, 400.f), 12 + (i * 41 % 210), 2 + (i % 3), C(255, 255, 255, 200));
    ellipse(70, 224, 90, 14, C(200, 170, 210, 120));
    ocDraw(g_sel, 22, 16, 1.f);
    const OcDef& d = ocDef(g_sel);
    flashPanel(146, 14, 244, 30, RGB{255, 46, 140}, 10);
    textOutline(d.name, 268, 17, 0.7f, C(255, 255, 255), C(UI_INK), 1, 1.1f);
    textOutline(d.title, 268, 50, 0.48f, C(150, 60, 160), C(255, 255, 255), 1, 0.6f);
    flashPanel(146, 74, 244, 96, RGB{255, 255, 255}, 8, 235);
    auto ls = wrapText(d.bio, 0.42f, 226);
    for (size_t k = 0; k < ls.size() && k < 5; k++) text(ls[k].c_str(), 156, 80 + k * 17.f, 0.42f, C(UI_INK));
    int sim = ocSim(g_sel);
    std::string r = sim == G.mc ? std::string("That's you!") : sim >= 0 ? std::string("You two: ") + relLabel(G.mc, sim) : "Not in town yet";
    textOutline(r.c_str(), 268, 180, 0.44f, C(255, 255, 255), C(UI_INK), 1, 0.8f);
    if (sim >= 0) { const Sim& s = G.sims[sim]; std::string a = s.stage >= ST_YA ? "Young Adult" : s.stage == ST_TEEN ? "Teen" : "Kid"; textOutline((a + "  -  Starlight Bay").c_str(), 268, 200, 0.4f, C(255, 236, 140), C(UI_INK), 1, 0.8f); }
}
void ocAppDrawBottom(float t) {
    bottomBackdrop(t);
    textOutline("STARLIGHT CAST", 160, 6, 0.55f, C(255, 255, 255), C(UI_INK), 1);
    for (int k = 0; k < PER_PAGE; k++) { int i = g_page * PER_PAGE + k; if (i < OC_COUNT) button(castB(k), ocDef(i).name, i == g_sel, i % 4, 0.42f); }
    { char pg[24]; snprintf(pg, sizeof pg, "More (%d/%d)", g_page + 1, (OC_COUNT + PER_PAGE - 1) / PER_PAGE); button(bMore, pg, false, 3, 0.4f); }
    int sim = ocSim(g_sel);
    if (sim >= 0) {
        button(bText, "Text", false, 1, 0.44f);
        button(bInvite, "Invite Over", false, 2, 0.44f);
        if (MC().stage >= ST_TEEN && canRomance(G.mc, sim)) button(bDate, "Ask on Date", false, 0, 0.44f);
    }
}
static C3D_Tex g_otTex[9];
static bool g_otReady[9];
void ocOtomeDraw(int i, float x, float y, float scale, int rows, int col0, int cols) {
    i = ((i % 9) + 9) % 9;
    if (!g_otReady[i]) {
        if (!C3D_TexInit(&g_otTex[i], 128, 256, GPU_RGBA8)) return;
        C3D_TexSetFilter(&g_otTex[i], GPU_NEAREST, GPU_NEAREST);
        static uint8_t buf[CW * CH * 4];
        ocOtomePaint(i, buf);
        uploadTo(g_otTex[i], buf);
        g_otReady[i] = true;
    }
    rows = std::max(8, std::min(rows, CH));
    col0 = std::max(0, std::min(col0, CW - 8)); cols = std::max(8, std::min(cols, CW - col0));
    Tex3DS_SubTexture sub = {(u16)cols, (u16)rows, col0 / 128.f, 1.f, (col0 + cols) / 128.f, 1.f - rows / 256.f};
    C2D_Image img = {&g_otTex[i], &sub};
    C2D_DrawImageAt(img, std::floor(x), std::floor(y), 0.5f, nullptr, scale, scale);
}
static void stampBig(C3D_Tex& tx, float x, float y, float scale, int rows, int col0, int cols) {
    rows = std::max(8, std::min(rows, CH));
    col0 = std::max(0, std::min(col0, CW - 8)); cols = std::max(8, std::min(cols, CW - col0));
    Tex3DS_SubTexture sub = {(u16)cols, (u16)rows, col0 / 128.f, 1.f, (col0 + cols) / 128.f, 1.f - rows / 256.f};
    C2D_Image img = {&tx, &sub};
    C2D_DrawImageAt(img, std::floor(x), std::floor(y), 0.5f, nullptr, scale, scale);
}
static C3D_Tex g_sbTex[7];
static bool g_sbReady[7];
void ocSbhDraw(int i, float x, float y, float scale, int rows, int col0, int cols) {
    i = ((i % 7) + 7) % 7;
    if (!g_sbReady[i]) {
        if (!C3D_TexInit(&g_sbTex[i], 128, 256, GPU_RGBA8)) return;
        C3D_TexSetFilter(&g_sbTex[i], GPU_NEAREST, GPU_NEAREST);
        static uint8_t buf[CW * CH * 4];
        ocSbhPaint(i, buf); uploadTo(g_sbTex[i], buf); g_sbReady[i] = true;
    }
    stampBig(g_sbTex[i], x, y, scale, rows, col0, cols);
}
static C3D_Tex g_meTex;
static uint32_t g_meKey = 0;
void ocSimPortraitDraw(const Sim& s, float x, float y, float scale, int rows, int col0, int cols) {
    uint32_t k = simKey(s);
    if (k != g_meKey) {
        if (!g_meKey && !C3D_TexInit(&g_meTex, 128, 256, GPU_RGBA8)) return;
        C3D_TexSetFilter(&g_meTex, GPU_NEAREST, GPU_NEAREST);
        static uint8_t buf[CW * CH * 4];
        ocPaintSim(s, buf); uploadTo(g_meTex, buf); g_meKey = k;
    }
    stampBig(g_meTex, x, y, scale, rows, col0, cols);
}
#endif
int ocHeadCount() { return headCountI(); }
std::string ocHeadName(int id) { return headNameI(id); }
