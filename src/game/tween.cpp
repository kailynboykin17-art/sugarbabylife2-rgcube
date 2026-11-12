#include "tween.hpp"
#include <cmath>
#include <algorithm>
#include <map>

float easeInOutSine(float x) { return -(std::cos(3.14159265f * x) - 1) / 2; }
float easeOutBack(float x) { const float c1 = 1.70158f, c3 = c1 + 1; return 1 + c3 * std::pow(x - 1, 3) + c1 * std::pow(x - 1, 2); }
float easeOutElastic(float x) { if (x <= 0) return 0; if (x >= 1) return 1; return std::pow(2.f, -10 * x) * std::sin((x * 10 - 0.75f) * 2.0944f) + 1; }
float easeOutBounce(float x) {
    const float n1 = 7.5625f, d1 = 2.75f;
    if (x < 1 / d1) return n1 * x * x;
    if (x < 2 / d1) { x -= 1.5f / d1; return n1 * x * x + 0.75f; }
    if (x < 2.5f / d1) { x -= 2.25f / d1; return n1 * x * x + 0.9375f; }
    x -= 2.625f / d1; return n1 * x * x + 0.984375f;
}

struct Tw { int kind; float t, dur; int loops; bool hold; };
static std::map<int, Tw> tw;
struct Seen { float age = 0, since = 0; };
static std::map<int, Seen> seen;   // how long a sim has been on screen (pop-in) and since last drawn

void tweenPlay(int sim, int kind, float dur, int loops) { if (sim < 0) return; tw[sim] = Tw{kind, 0, std::max(0.1f, dur), std::max(1, loops), false}; }
void tweenLoop(int sim, int kind) { if (sim < 0) return; auto it = tw.find(sim); if (it == tw.end() || it->second.kind != kind) tw[sim] = Tw{kind, 0, kind == TW_SPIN ? 1.6f : 1.2f, 1, true}; else it->second.hold = true; }
void tweenTick(float dt) {
    for (auto it = tw.begin(); it != tw.end();) {
        Tw& w = it->second; w.t += dt;
        if (w.t >= w.dur) { if (w.hold || --w.loops > 0) w.t = std::fmod(w.t, w.dur); else { it = tw.erase(it); continue; } }
        w.hold = false; ++it;
    }
    for (auto it = seen.begin(); it != seen.end();) { it->second.age += dt; it->second.since += dt; if (it->second.since > 5) it = seen.erase(it); else ++it; }
}

TweenX tweenGet(int sim, float t) {
    TweenX o;
    // pop-in when someone first shows up (easeOutBack scale)
    auto s = seen.find(sim);
    if (s == seen.end()) { seen[sim] = Seen{}; s = seen.find(sim); }
    if (s->second.age < 0.35f) { const float k = easeOutBack(s->second.age / 0.35f); o.sx = o.sy = std::max(0.6f, k); o.any = true; }
    s->second.since = 0;   // still on screen
    // gentle breathing so idle dolls never look frozen
    o.sy *= 1.f + 0.012f * std::sin(t * 2.2f + sim);
    o.armL = -0.04f * std::sin(t * 1.3f + sim); o.armR = 0.04f * std::sin(t * 1.1f + sim * 2); o.head = 0.03f * std::sin(t * 0.7f + sim);   // idle sway
    auto it = tw.find(sim);
    if (it == tw.end()) return o;
    const Tw& w = it->second; const float p = w.t / w.dur;
    o.any = true;
    switch (w.kind) {
    case TW_HOP: o.dy = -std::sin(p * 3.14159f) * 14; o.sy *= 1 + 0.08f * std::sin(p * 6.283f); o.sx *= 1 - 0.05f * std::sin(p * 6.283f); break;
    case TW_CHEER: o.dy = -std::fabs(std::sin(p * 9.42f)) * 10 * (1 - p * 0.5f); o.armL = -0.5f; o.armR = 0.5f; break;
    case TW_SPIN: o.sx *= std::cos(p * 6.2832f); if (std::fabs(o.sx) < 0.08f) o.sx = o.sx < 0 ? -0.08f : 0.08f; o.dy = -std::sin(p * 3.14159f) * 4; break;
    case TW_KNEEL: { const float k = p < 0.25f ? easeInOutSine(p / 0.25f) : p > 0.85f ? 1 - easeInOutSine((p - 0.85f) / 0.15f) : 1; o.sy *= 1 - 0.28f * k; o.dx = 3 * k; break; }
    case TW_BOW: { const float k = std::sin(p * 3.14159f); o.rot = 0.35f * k; o.dx = 4 * k; break; }
    case TW_LEANL: case TW_LEANR: { o.armL = o.armR = (w.kind == TW_LEANL ? -0.3f : 0.3f); const float k = easeInOutSine(std::min(1.f, p * 2)) * (1 - std::max(0.f, p - 0.7f) / 0.3f); o.rot = (w.kind == TW_LEANL ? -0.18f : 0.18f) * k; o.dx = (w.kind == TW_LEANL ? -5 : 5) * k; break; }
    case TW_SHAKE: o.dx = std::sin(p * 50) * 3 * (1 - p); o.rot = std::sin(p * 40) * 0.05f * (1 - p); break;
    case TW_SWING: o.rot = std::sin(p * 12.566f) * 0.3f; o.dx = std::sin(p * 12.566f) * 4; break;
    case TW_SWAY: o.rot = std::sin(p * 6.2832f) * 0.08f; o.dx = std::sin(p * 6.2832f) * 3; break;
    case TW_WAVE: o.armR = 0.5f + 0.25f * std::sin(p * 25.f) * (1 - p * 0.3f); o.head = 0.06f; break;
    case TW_CLAP: { const float k = 0.5f + 0.5f * std::sin(p * 31.f); o.armL = 0.15f + 0.2f * k; o.armR = -0.15f - 0.2f * k; break; }
    case TW_ARMSUP: { const float k = easeOutBack(std::min(1.f, p * 3)) * (p > 0.8f ? (1 - p) / 0.2f : 1); o.armL = -0.6f * k; o.armR = 0.6f * k; o.dy -= 4 * k; break; }
    case TW_STOMP: o.legL = 0.2f * std::max(0.f, std::sin(p * 18.85f)); o.legR = -0.2f * std::max(0.f, -std::sin(p * 18.85f)); o.rot = 0.03f * std::sin(p * 18.85f); break;
    case TW_POP: { const float k = easeOutElastic(p); o.sx *= 0.6f + 0.4f * k; o.sy *= 0.6f + 0.4f * k; break; }
    }
    return o;
}
