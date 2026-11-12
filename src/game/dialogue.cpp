// Story dialogue: a big speech bubble on the top screen with the speaker's portrait.
// While a line is showing the game waits; press A (or tap) to continue.
#include "life3.hpp"
#include "game.hpp"
#include "draw.hpp"
#include "sim.hpp"
#include <cmath>
#include <cstdio>
#include <deque>

using namespace col;

namespace {
struct Line { int speaker; std::string text; };
std::deque<Line> q;
float shownT = 0;
}

void dlgSay(int speaker, const std::string& text) {
    if (q.size() > 8) return;
    q.push_back({speaker, text});
    if (q.size() == 1) shownT = 0;
}
bool dlgActive() { return !q.empty(); }
void dlgAdvance() {
    if (q.empty()) return;
    if (shownT < 0.25f) return;   // don't skip a line by accident
    q.pop_front();
    shownT = 0;
}

void dlgDrawTop(float t) {
    if (q.empty()) return;
    shownT += 1.f / 60.f;
    const Line& l = q.front();
    rect(0, 0, TOP_W, TOP_H, C(0, 0, 0, 70));
    // portrait bubble
    const float pcx = 44, pcy = 168, pr = 38;
    circle(pcx, pcy, pr + 4, C(80, 190, 235));
    circle(pcx, pcy, pr, C(255, 255, 255));
    if (l.speaker >= 0 && l.speaker < (int)G.sims.size())
        drawSimPortrait(G.sims[l.speaker], pcx, pcy, pr, t);
    else { heart(pcx, pcy, 16, C(HOT)); sparkle(pcx + 18, pcy - 18, 6, C(GOLD)); }
    // speech bubble
    const float bx = 84, by = 46, bw = 308, bh = 112;
    rrect(bx - 3, by - 3, bw + 6, bh + 6, 18, C(80, 190, 235));
    rrect(bx, by, bw, bh, 16, C(255, 255, 255));
    tri(bx + 4, by + bh - 34, bx + 4, by + bh - 14, bx - 26, by + bh + 6, C(255, 255, 255));
    // name tag
    std::string name = l.speaker >= 0 && l.speaker < (int)G.sims.size() ? G.sims[l.speaker].name : std::string(GAME_TITLE);
    float nw = textW(name.c_str(), 0.5f) + 22;
    rrect(bx + 14, by - 16, nw, 22, 9, C(HOT));
    text(name.c_str(), bx + 14 + nw / 2, by - 14, 0.5f, C(WHITE), 1);
    // text with a typewriter reveal
    int reveal = (int)(shownT * 60.f * l3TextMult());
    std::string shown = l.text.substr(0, std::min((size_t)reveal, l.text.size()));
    auto lines = wrapText(shown, 0.55f, bw - 34);
    for (size_t i = 0; i < lines.size() && i < 5; i++) text(lines[i].c_str(), bx + bw / 2, by + 12 + i * 19.f, 0.55f, C(40, 30, 40), 1);
    // A prompt
    float pulse = 0.5f + 0.5f * std::sin(t * 6);
    circle(bx + bw - 18, by + bh - 16, 10 + pulse, C(245, 140, 40));
    text("A", bx + bw - 18, by + bh - 26, 0.55f, C(WHITE), 1);
    if (q.size() > 1) { char b[16]; snprintf(b, sizeof b, "%d more", (int)q.size() - 1); text(b, bx + bw - 34, by + bh - 20, 0.38f, C(150, 140, 150), 2); }
}
