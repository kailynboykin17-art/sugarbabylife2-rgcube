#pragma once
#include "common.hpp"

void drawInit();
void drawExit();
void drawFrameBegin();   // resets the per-frame text buffer

extern bool g_pix;
extern bool g_layer;   // any offscreen art layer (pixel or smooth) is active
bool artLayerBegin(int slot, int style);   // pixel layer for pixel styles, 2x anti-aliased layer for HD Glam
void artLayerEnd(float x0 = 0, float y0 = 0, float x1 = 400, float y1 = 240);   // true while drawing into the pixel-art pass
void drawSetScreen(C3D_RenderTarget* t);   // the screen being drawn this scene
bool pixBegin(int slot = 0);   // start a half-res pixel-art layer (call pixEnd to stamp it on screen)
void pixEnd(float x0 = 0, float y0 = 0, float x1 = 400, float y1 = 240);   // stamps that screen region
extern u32 g_ink;   // non-zero: draw opaque shapes in this flat colour (outline pass)
void rect(float x, float y, float w, float h, u32 c);
void grad(float x, float y, float w, float h, u32 top, u32 bottom);
void circle(float cx, float cy, float r, u32 c);
void ellipse(float cx, float cy, float w, float h, u32 c);
void tri(float x0, float y0, float x1, float y1, float x2, float y2, u32 c);
void quad(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, u32 c);
void line(float x0, float y0, float x1, float y1, float th, u32 c);
void rrect(float x, float y, float w, float h, float r, u32 c);
void outline(float x, float y, float w, float h, float th, u32 c);

// Y2K girly flourishes
void sparkle(float x, float y, float size, u32 c);                  // 4-point twinkle star
void swirl(float cx, float cy, float r, float turns, float th, u32 c, bool flip = false);
void heart(float cx, float cy, float size, u32 c);
void zebraRect(float x, float y, float w, float h, u32 base, u32 stripe, int seed = 0);
void leopardRect(float x, float y, float w, float h, int seed = 0);

// Text (system font). align: 0 left, 1 center, 2 right
void text(const char* s, float x, float y, float sc, u32 c, int align = 0);
void textShadow(const char* s, float x, float y, float sc, u32 c, u32 shadow, int align = 0);
void textOutline(const char* s, float x, float y, float sc, u32 c, u32 ol, int align = 0, float th = 1.0f);   // chunky outlined lettering
float textW(const char* s, float sc);
std::vector<std::string> wrapText(const std::string& s, float sc, float maxW);

// Local-coordinate pen: draw a shape designed in a W x H box whose bottom
// center sits at (x, footY), scaled by s and optionally mirrored.
struct Pen {
    float ox, oy, s, W;
    bool flip;
    Pen(float x, float footY, float W_, float H_, float s_, bool f = false)
        : ox(x - W_ * s_ / 2), oy(footY - H_ * s_), s(s_), W(W_), flip(f) {}
    float X(float lx) const { return flip ? ox + (W - lx) * s : ox + lx * s; }
    float Y(float ly) const { return oy + ly * s; }
    void R(float x, float y, float w, float h, u32 c) const {
        float px = flip ? ox + (W - x - w) * s : ox + x * s;
        rect(px, Y(y), w * s, h * s, c);
    }
    void E(float cx, float cy, float w, float h, u32 c) const { ellipse(X(cx), Y(cy), w * s, h * s, c); }
    void Ci(float cx, float cy, float r, u32 c) const { circle(X(cx), Y(cy), r * s, c); }
    void T(float x0, float y0, float x1, float y1, float x2, float y2, u32 c) const {
        tri(X(x0), Y(y0), X(x1), Y(y1), X(x2), Y(y2), c);
    }
    void Q(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, u32 c) const {
        quad(X(x0), Y(y0), X(x1), Y(y1), X(x2), Y(y2), X(x3), Y(y3), c);
    }
    void L(float x0, float y0, float x1, float y1, float th, u32 c) const {
        line(X(x0), Y(y0), X(x1), Y(y1), th * s, c);
    }
    void Sp(float x, float y, float size, u32 c) const { sparkle(X(x), Y(y), size * s, c); }
    void H(float x, float y, float size, u32 c) const { heart(X(x), Y(y), size * s, c); }
    // Patterned fabric rect: palette index 10 = zebra, 11 = leopard, else solid
    void F(float x, float y, float w, float h, int palIdx, u32 solid, int seed = 0) const;
};

struct Btn {
    float x, y, w, h;
    bool in(const touchPosition& t) const { return t.px >= x && t.px < x + w && t.py >= y && t.py < y + h; }
};

// Flash-style glossy button. style: 0 hot pink, 1 turquoise, 2 orange, 3 purple
extern const RGB UI_INK;
void flashPanel(float x, float y, float w, float h, RGB fill, float r = 8, uint8_t a = 255);
void button(const Btn& b, const char* label, bool on = false, int style = 0, float sc = 0.5f);
void panelBg(float x, float y, float w, float h);   // chocolate panel with gold trim
void bottomBackdrop(float t);                       // mocha + script-swirl wallpaper
void flashSun(float x, float y, float r, float t);
void flashCloud(float x, float y, float s);
void teenBadge(float x, float y, float s = 1.f);
