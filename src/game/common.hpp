// Dream Dollhouse - a 3DS homebrew digital dollhouse
// Art direction: early-2000s girly "diva" vector style. Everything is drawn
// procedurally with citro2d shapes, so there are no image assets to convert.
#pragma once
#include <3ds.h>
#include <citro2d.h>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

// Maple Valley: the same game with painted sprite art (build with -DMAPLE_VALLEY), its own saves
#ifdef MAPLE_VALLEY
#define APP_DIR    "sdmc:/3ds/MapleValley"
#define GAME_TITLE "Maple Valley"
#define SPRITES_DEFAULT true
#else
#define GAME_TITLE "Sugar Bunny Life 2"
#define SPRITES_DEFAULT false
#endif
// One codebase, two games: build with -DDIVA_LIFE for Dream Dollhouse: Diva Life
#ifdef DIVA_LIFE
#define DEFAULT_ART 4      // HD Glam
#define FIRST_ART_CHOICE 0 // every art style is available
#ifndef APP_DIR
#define APP_DIR   "sdmc:/3ds/DreamDollhouse"
#endif
#else
#define DEFAULT_ART 7      // Flash Stick (the original look)
#define FIRST_ART_CHOICE 7 // the Stick Figure Ver. styles
#ifndef APP_DIR
#define APP_DIR   "sdmc:/3ds/StickFigureVer"
#endif
#endif
#define CFG_DIR   APP_DIR "/config"
#define SAVE_PATH CFG_DIR "/save.dat"

constexpr float TOP_W = 400.f, TOP_H = 240.f, BOT_W = 320.f, BOT_H = 240.f;

struct RGB { uint8_t r, g, b; };

inline u32 C(const RGB& c, uint8_t a = 255) { return C2D_Color32(c.r, c.g, c.b, a); }
inline u32 C(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) { return C2D_Color32(r, g, b, a); }
RGB shade(const RGB& c, float f);           // f < 1 darker, f > 1 lighter
RGB mix(const RGB& a, const RGB& b, float t);

inline float frand() { return (float)rand() / (float)RAND_MAX; }
inline int irand(int n) { return n <= 0 ? 0 : rand() % n; }
template <class T> inline T clampv(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

struct Input {
    u32 down = 0, held = 0;
    touchPosition tp{};
    bool touch = false;          // stylus touched down this frame
    circlePosition cp{};
    float dt = 0.f;
};

// The diva palette: chocolate, mocha, cream, blush and hot pink with gold.
namespace col {
const RGB WHITE{255, 255, 255}, BLACK{20, 16, 18};
const RGB CHOC{52, 32, 27}, CHOC2{70, 44, 36}, MOCHA{120, 80, 62}, LATTE{186, 140, 110};
// interface theme: handheld turquoise
const RGB UI1{20, 86, 108}, UI2{30, 118, 142}, UIM{52, 164, 186}, UIL{150, 226, 236};
const RGB CREAM{250, 236, 216}, CHAMP{240, 214, 170}, GOLD{224, 184, 104};
const RGB BLUSH{255, 200, 218}, PINK{244, 120, 168}, HOT{255, 58, 140}, BERRY{176, 30, 90};
const RGB LILAC{200, 166, 230}, BABYBLUE{168, 210, 242}, MINT{168, 226, 200};
const RGB GRAY{168, 160, 166}, LIGHTGRAY{222, 214, 216}, GREEN{98, 170, 104};
const RGB WOOD{150, 100, 68}, RED{220, 50, 70}, ORANGE{245, 140, 40}, PURPLE{120, 70, 170};
const RGB TEXT{60, 36, 30};
}
