// Photo filters: Moonivy's Planet-inspired color grades as cheap 2D overlays.
#include "filters.hpp"
#include "game.hpp"
#include "draw.hpp"
#include <cmath>
#include <algorithm>
using namespace col;

namespace {
struct Grade { const char* name; RGB wash; uint8_t washA; RGB glow; uint8_t glowA; RGB vig; uint8_t vigA; int sparkles; bool grain; };
const Grade GR[NUM_FILTERS] = {
    {"Off", {0, 0, 0}, 0, {0, 0, 0}, 0, {0, 0, 0}, 0, 0, false},
    // muted cream wash + warm peach bloom up top + soft taupe edges + a few twinkles (the Planet V2 look)
    {"Planet (Moonivy)", {236, 222, 210}, 70, {255, 214, 196}, 46, {150, 128, 120}, 70, 10, false},
    // brighter, cooler pale version (the Lottie V1 side of the reference)
    {"Lottie Pale", {240, 236, 236}, 82, {255, 240, 236}, 50, {170, 160, 166}, 60, 8, false},
    {"Peach Glow", {255, 210, 190}, 50, {255, 190, 170}, 60, {200, 120, 120}, 50, 6, false},
    {"Y2K Pink Bloom", {255, 180, 220}, 55, {255, 200, 240}, 70, {200, 80, 160}, 60, 14, false},
    {"Vintage Film", {226, 196, 150}, 80, {255, 230, 180}, 30, {80, 60, 40}, 110, 0, true},
    {"Moody Violet", {150, 120, 200}, 60, {200, 160, 255}, 30, {40, 20, 70}, 120, 4, false},
    {"Clean Girl", {250, 248, 244}, 45, {255, 255, 255}, 50, {200, 196, 190}, 30, 3, false},
};
}
const char* filterName(int f) { return GR[((f % NUM_FILTERS) + NUM_FILTERS) % NUM_FILTERS].name; }

void filterDraw(float t) {
    const int f = G.filter % NUM_FILTERS; if (!f) return;
    const Grade& g = GR[f];
    const float k = 0.5f + 0.25f * std::clamp<int>(G.filterStr, 1, 3);   // strength: 0.75 / 1 / 1.25
    auto A = [&](uint8_t a) { return (uint8_t)std::min(255.f, a * k); };
    rect(0, 0, TOP_W, TOP_H, C(g.wash.r, g.wash.g, g.wash.b, A(g.washA)));                          // muted color wash
    grad(0, 0, TOP_W, 90, C(g.glow.r, g.glow.g, g.glow.b, A(g.glowA)), C(g.glow.r, g.glow.g, g.glow.b, 0));   // soft bloom from above
    for (int i = 0; i < 6; i++) {                                                                    // vignette, built from stacked bands
        const uint8_t a = (uint8_t)(A(g.vigA) * (6 - i) / 12);
        const float w = 6 + i * 6.f;
        rect(0, 0, w, TOP_H, C(g.vig.r, g.vig.g, g.vig.b, a)); rect(TOP_W - w, 0, w, TOP_H, C(g.vig.r, g.vig.g, g.vig.b, a));
        rect(0, TOP_H - w * 0.6f, TOP_W, w * 0.6f, C(g.vig.r, g.vig.g, g.vig.b, a));
    }
    for (int i = 0; i < g.sparkles; i++) {                                                           // dreamy twinkles
        const float x = std::fmod(i * 71.f + 13, 400.f), y = std::fmod(i * 53.f + 29, 230.f);
        const float tw = 0.5f + 0.5f * std::sin(t * 2.f + i * 1.7f);
        sparkle(x, y, 1.5f + 2.f * tw, C(255, 255, 255, (uint8_t)(120 * tw)));
    }
    if (g.grain) for (int i = 0; i < 60; i++) { uint32_t h = (uint32_t)(i * 2654435761u + (uint32_t)(t * 24) * 40503u); rect((h >> 8) % 400, (h >> 18) % 240, 1, 1, C(40, 30, 20, 60)); }
}
