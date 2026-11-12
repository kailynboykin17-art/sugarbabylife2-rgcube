#include "settings2.hpp"
#include "emote.hpp"
#include "game.hpp"
#include "draw.hpp"
#include "tween.hpp"
#include <vector>
#include <cmath>
#include <cctype>
#include <algorithm>
using namespace col;

namespace {
struct E { int sim, kind; std::string word; float t; };
std::vector<E> live;
float lastAuto = -10, clockT = 0;
const char* WORDS[EMO_COUNT][6] = {
    {"Swoon!", "<3 <3", "Obsessed.", "Cutie!", "Be still my heart", "Ugh, I love"},
    {"Yasss!", "Slay!", "Periodt.", "LET'S GO!", "Iconic.", "Main character!"},
    {"Ugh...", "Not today.", "Crying rn", "Why me", "Sigh.", "I'm fine. (not)"},
    {"Excuse me?!", "Bruh.", "The AUDACITY", "Grrr", "Oh it's ON.", "Try me."},
    {"Wait WHAT", "OMG", "No way!", "I'm shaking", "Plot twist!", "STOP."},
    {"Cha-ching!", "Rich girl!", "$$$", "Bag secured", "Money!", "Ka-ching"},
    {"La la la~", "Bop!", "Vibing", "Encore!", "Hit it!", "Jam!"},
    {"Awkward...", "Oops.", "Yikes.", "Mortified", "Um...", "Haha... ha."},
    {"Ooh!", "Idea!", "Genius.", "Big brain", "Wait - yes!", "Aha!"},
    {"Blegh.", "Achoo!", "I need soup", "Ugh, sick", "Cough cough", "Tissues pls"},
};
const int REACT[EMO_COUNT] = {TW_SWAY, TW_HOP, TW_BOW, TW_STOMP, TW_SHAKE, TW_CHEER, TW_SWAY, TW_SHAKE, TW_ARMSUP, TW_BOW};
bool has(const std::string& s, const char* k) { return s.find(k) != std::string::npos; }
}

void emotePlay(int sim, int kind, const std::string& word) {
    if (!gsOn(GS_EMOTES)) return;
    if (sim < 0 || kind < 0 || kind >= EMO_COUNT) return;
    live.erase(std::remove_if(live.begin(), live.end(), [&](const E& e) { return e.sim == sim; }), live.end());
    if (live.size() >= 14) live.erase(live.begin());
    live.push_back({sim, kind, word.empty() ? WORDS[kind][irand(6)] : word, 0});
    tweenPlay(sim, REACT[kind]);
}

void emoteFromText(int sim, const std::string& raw) {
    if (clockT - lastAuto < 1.2f) return;   // don't spam: one reaction per moment
    std::string m; m.reserve(raw.size()); for (char c : raw) m += (char)std::tolower((unsigned char)c);
    int k = -1;
    if (has(m, "won") || has(m, "+$") || has(m, "paid") || has(m, "bonus") || has(m, "lottery") || has(m, "profit") || has(m, "promot")) k = EMO_MONEY;
    else if (has(m, "love") || has(m, "kiss") || has(m, "crush") || has(m, "date") || has(m, "swoon") || has(m, "married") || has(m, "yes!") || has(m, "bff")) k = EMO_LOVE;
    else if (has(m, "fire") || has(m, "busted") || has(m, "surprise") || has(m, "!!") || has(m, "what?") || has(m, "found out") || has(m, "caught")) k = EMO_SHOCK;
    else if (has(m, "grounded") || has(m, "fight") || has(m, "furious") || has(m, "rude") || has(m, "betray") || has(m, "drama") || has(m, "tattle") || has(m, "hoa")) k = EMO_ANGRY;
    else if (has(m, "cold") || has(m, "sick") || has(m, "flu") || has(m, "doctor")) k = EMO_SICK;
    else if (has(m, "sad") || has(m, "lost") || has(m, "broke up") || has(m, "heartbroken") || has(m, "rejected") || has(m, "missed") || has(m, "denied") || has(m, "ouch") || has(m, "failed")) k = EMO_SAD;
    else if (has(m, "sing") || has(m, "danc") || has(m, "music") || has(m, "concert") || has(m, "karaoke")) k = EMO_MUSIC;
    else if (has(m, "oops") || has(m, "awkward") || has(m, "mortif") || has(m, "embarrass") || has(m, "teacher reads")) k = EMO_SWEAT;
    else if (has(m, "learned") || has(m, "skill") || has(m, "idea") || has(m, "solve") || has(m, "correct")) k = EMO_IDEA;
    else if (has(m, "!") || has(m, "new ") || has(m, "yay") || has(m, "best") || has(m, "cute") || has(m, "iconic") || has(m, "slay")) k = EMO_HAPPY;
    if (k < 0) return;
    lastAuto = clockT; emotePlay(sim, k);
}

static void icon(int k, float x, float y, float s, float t) {
    switch (k) {
    case EMO_LOVE: circle(x - 3 * s, y - 1 * s, 3.4f * s, C(255, 60, 120)); circle(x + 3 * s, y - 1 * s, 3.4f * s, C(255, 60, 120)); tri(x - 6.3f * s, y, x + 6.3f * s, y, x, y + 7 * s, C(255, 60, 120)); break;
    case EMO_HAPPY: for (int i = 0; i < 4; i++) { const float a = i * 1.5708f + t * 2; rect(x + std::cos(a) * 3 * s - 0.8f, y + std::sin(a) * 3 * s - 0.8f, 1.6f, 1.6f, C(255, 200, 40)); }
                    rect(x - 0.8f * s, y - 6 * s, 1.6f * s, 12 * s, C(255, 200, 40)); rect(x - 6 * s, y - 0.8f * s, 12 * s, 1.6f * s, C(255, 200, 40)); break;
    case EMO_SAD: circle(x - 3 * s, y - 1 * s, 3.5f * s, C(140, 150, 175)); circle(x + 2 * s, y - 2 * s, 4 * s, C(140, 150, 175)); rect(x - 6 * s, y - 1 * s, 11 * s, 3.5f * s, C(140, 150, 175));
                  for (int i = 0; i < 3; i++) rect(x - 4 * s + i * 3.5f * s, y + 3.5f * s + std::fmod(t * 9 + i * 2.f, 4.f), 1.2f * s, 2.4f * s, C(80, 150, 255)); break;
    case EMO_ANGRY: for (int i = 0; i < 4; i++) { const float a = i * 1.5708f + 0.785f; tri(x, y, x + std::cos(a - 0.3f) * 7 * s, y + std::sin(a - 0.3f) * 7 * s, x + std::cos(a + 0.3f) * 7 * s, y + std::sin(a + 0.3f) * 7 * s, C(235, 40, 50)); } circle(x, y, 2 * s, C(255, 255, 255)); break;
    case EMO_SHOCK: text("!?", x, y - 8 * s, 0.6f * s, C(255, 120, 30), 1); break;
    case EMO_MONEY: text("$", x, y - 8 * s, 0.7f * s, C(40, 170, 70), 1); break;
    case EMO_MUSIC: circle(x - 2 * s, y + 3 * s, 2.5f * s, C(150, 80, 220)); rect(x + 0.2f * s, y - 6 * s, 1.4f * s, 9 * s, C(150, 80, 220)); rect(x + 0.2f * s, y - 6 * s, 5 * s, 1.6f * s, C(150, 80, 220)); break;
    case EMO_SWEAT: tri(x, y - 6 * s, x - 3.5f * s, y + 1 * s, x + 3.5f * s, y + 1 * s, C(100, 180, 255)); circle(x, y + 1.5f * s, 3.5f * s, C(100, 180, 255)); break;
    case EMO_IDEA: circle(x, y - 1 * s, 4.5f * s, C(255, 230, 80)); rect(x - 2 * s, y + 3 * s, 4 * s, 3 * s, C(160, 160, 170)); break;
    case EMO_SICK: for (int i = 0; i < 10; i++) { const float a = i * 0.7f + t * 3, r = i * 0.6f * s; rect(x + std::cos(a) * r - 0.8f, y + std::sin(a) * r - 0.8f, 1.8f, 1.8f, C(110, 200, 90)); } break;
    }
}

void emoteDraw(int sim, float x, float headY, float t) {
    static float lastT = 0; const float dt = std::max(0.f, std::min(0.1f, t - lastT)); if (t != lastT) { clockT += dt; for (auto& e : live) e.t += dt; lastT = t; }
    live.erase(std::remove_if(live.begin(), live.end(), [](const E& e) { return e.t > 2.6f; }), live.end());
    for (const E& e : live) {
        if (e.sim != sim) continue;
        const float pop = e.t < 0.18f ? e.t / 0.18f * 1.15f : e.t < 0.28f ? 1.15f - (e.t - 0.18f) * 1.5f : 1.f;
        const float a = std::min(1.f, (2.6f - e.t) * 3.f);
        const float by = headY - 16 - std::sin(t * 3) * 1.5f - std::min(e.t, 0.3f) * 10, s = pop;
        const uint8_t al = (uint8_t)(255 * std::max(0.f, a));
        circle(x + 9, headY - 4, 1.8f, C(255, 255, 255, al)); circle(x + 6, headY - 8, 2.6f, C(255, 255, 255, al));
        circle(x, by, 11.5f * s, C(UI_INK, al)); circle(x, by, 10 * s, C(255, 255, 255, al));
        if (a > 0.3f) icon(e.kind, x, by, s, t);
        if (e.t > 0.15f) textOutline(e.word.c_str(), x, by - 26, 0.36f, C(255, 255, 255, al), C(UI_INK, al), 1, 0.8f);
    }
}
