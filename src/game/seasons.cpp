#include "seasons.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "goals.hpp"
#include "draw.hpp"
#include "extras.hpp"
#include "audio.hpp"
#include "life.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>

using namespace col;

namespace {
struct FestDef { const char* name; int dayInMonth; int room; int h0, h1; const char* blurb; const char* acts[3]; };
// the in-game "month" is 28 days: 7 per season (Spring 0-6, Summer 7-13, Fall 14-20, Winter 21-27)
const FestDef FEST[] = {
    {"Sweetheart Day", 1, R_CAFE, 10, 22, "Pink everything! Valentines, candy hearts and a couples photo booth at Cafe Latte.",
     {"Send Valentines to Friends", "Couples Photo Booth", "Candy Heart Tasting"}},
    {"Blossom Festival", 4, R_PARK, 10, 18, "Cherry blossoms are out! Flower crowns, an egg hunt and picnics in Rosebud Park.",
     {"Make a Flower Crown", "Spring Egg Hunt", "Blossom Picnic"}},
    {"Summer Splash", 9, R_BEACH, 11, 23, "Sandcastle contest, ice cream and fireworks over Sunset Beach!",
     {"Sandcastle Contest", "Ice Cream Stand ($5)", "Watch the Fireworks"}},
    {"Summer Street Fair", 12, R_STREET, 12, 22, "Ferris wheel, cotton candy and carnival games on Maple Lane.",
     {"Ride the Ferris Wheel", "Cotton Candy ($4)", "Ring Toss (win a plush!)"}},
    {"Harvest Festival", 16, R_PARK, 10, 19, "Pumpkins, apple cider and hayrides in Rosebud Park.",
     {"Pumpkin Carving", "Bob for Apples", "Hayride"}},
    {"Spooky Night", 18, R_STREET, 16, 23, "Costumes on! Trick-or-treat down Maple Lane and brave the haunted house.",
     {"Trick-or-Treat", "Haunted House", "Costume Contest"}},
    {"Winterfest", 25, R_PARK, 10, 22, "Twinkle lights, hot cocoa, ice skating and a gift exchange.",
     {"Gift Exchange", "Hot Cocoa Stand", "Build a Snow Bunny"}},
    {"New Year's Eve", 27, R_STREET, 19, 24, "The countdown party on Maple Lane! Fireworks at midnight.",
     {"Countdown Party", "Make a Resolution", "Midnight Fireworks"}},
};
constexpr int NUM_FEST = sizeof(FEST) / sizeof(FEST[0]);
int hourNow() { return (int)(G.minute / 60); }

std::vector<int> townHere() { std::vector<int> v; for (const Actor& a : curRoom().actors) if (a.kind == 0 && a.id != G.mc && a.id < G.sims.size() && !G.sims[a.id].household) v.push_back(a.id); return v; }
void friendlyCrowd(int dfr) { for (int s : townHere()) changeRel(G.mc, s, dfr, 0); }
void memo(const std::string& t) { addMemory(MEM_FUN, G.mc, -1, t); }

// morning recap
std::string morningText() {
    char b[96];
    std::string o;
    snprintf(b, sizeof b, "%s. ", dateStr(G.day).c_str()); o += b;
    static const char* const W[WX_COUNT] = {"Sunny skies", "Cloudy", "Rainy - bring an umbrella", "Stormy! Stay cozy", "Snow day!", "Heat wave - beach weather"};
    o += W[G.weather % WX_COUNT]; o += ".";
    int f = festToday();
    if (f >= 0) { snprintf(b, sizeof b, "\nTODAY: %s at %s (%d-%d)!", FEST[f].name, ROOM_NAMES[FEST[f].room], FEST[f].h0 > 12 ? FEST[f].h0 - 12 : FEST[f].h0, FEST[f].h1 > 12 ? FEST[f].h1 - 12 : FEST[f].h1); o += b; }
    else { std::string h = lifeTodayHeadline(); if (!h.empty()) o += "\nIn town: " + h; }
    const Sim& me = MC();
    if (me.career != CR_NONE && isWeekday(G.day)) { snprintf(b, sizeof b, "\nWork at %d:00 (%s).", CAREERS[me.career].startHour, ROOM_NAMES[CAREERS[me.career].room]); o += b; }
    if (me.stage >= ST_CHILD && me.stage <= ST_TEEN && isWeekday(G.day)) o += "\nSchool at 8 AM!";
    int fussy = 0; for (const Sim& s : G.sims) if (s.household && s.isLittle()) fussy++;
    if (fussy) o += "\nThe little ones will want breakfast!";
    return o;
}
std::string mornFn(int, int) { return ""; }
}   // namespace

int festOn(int day) { int d = day % 28; for (int i = 0; i < NUM_FEST; i++) if (FEST[i].dayInMonth == d) return i; return -1; }
int festToday() { return festOn(G.day); }
const char* festName(int f) { return f >= 0 && f < NUM_FEST ? FEST[f].name : ""; }
int festRoom(int f) { return f >= 0 && f < NUM_FEST ? FEST[f].room : -1; }
int festStart(int f) { return f >= 0 && f < NUM_FEST ? FEST[f].h0 : 0; }
int festEnd(int f) { return f >= 0 && f < NUM_FEST ? FEST[f].h1 : 0; }
bool festOpenNow() { int f = festToday(); int h = hourNow(); return f >= 0 && h >= FEST[f].h0 && h < FEST[f].h1; }

int festActs(std::string out[3], int ids[3]) {
    int f = festToday();
    if (f < 0 || !festOpenNow() || G.room != FEST[f].room) return 0;
    int n = 0;
    for (int i = 0; i < 3; i++) if (!(G.festDone >> (f * 3 + i) & 1)) { out[n] = std::string(FEST[f].name) + ": " + FEST[f].acts[i]; ids[n] = i; n++; }
    return n;
}

void festDo(int i) {
    int f = festToday();
    if (f < 0 || i < 0 || i > 2 || (G.festDone >> (f * 3 + i) & 1)) return;
    G.festDone |= 1u << (f * 3 + i);
    Sim& me = MC();
    auto fun = [&](int v) { me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + v); };
    auto soc = [&](int v) { me.needs[N_SOCIAL] = std::min(100.f, me.needs[N_SOCIAL] + v); };
    std::string res;
    int p = partnerOf(G.mc);
    switch (f * 3 + i) {
    case 0: { int n = 0; for (const Rel& r : G.rels) if ((r.a == G.mc || r.b == G.mc) && r.met && r.fr >= 10 && n < 8) { changeRel(r.a == G.mc ? r.b : r.a, G.mc, 6, 0); n++; } res = "You handed out " + std::to_string(n) + " valentines. Your friends LOVED them."; soc(25); break; }
    case 1: if (p >= 0) { changeRel(G.mc, p, 4, 12); res = "A strip of four photos with " + G.sims[p].name + ". Instant keepsake."; addMoodlet(me, "Photo Booth Cuties", EM_FLIRTY, 2, 360); } else { res = "Solo photo booth: four iconic poses. Self-love day!"; addMoodlet(me, "Main Character", EM_CONFIDENT, 2, 360); } fun(25); break;
    case 2: res = "\"BE MINE\", \"CUTIE\", \"TEXT ME\"... you ate the whole box."; me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + 15); fun(15); addMoodlet(me, "Sugar Rush", EM_PLAYFUL, 1, 180); break;
    case 3: skillGain(me, SK_CREATIVITY, 4); res = "A crown of pink blossoms. You're the spring princess now."; addMoodlet(me, "Flower Crown", EM_HAPPY, 2, 480); break;
    case 4: { int m = 20 + irand(60); G.money += m; res = "You found " + std::to_string(3 + irand(6)) + " eggs - one had $" + std::to_string(m) + " inside!"; fun(30); collectSearch(collectSetFor(R_PARK) >= 0 ? collectSetFor(R_PARK) : 0); break; }
    case 5: friendlyCrowd(5); soc(35); me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + 30); res = "A blanket under the blossoms with everyone. Petals in your hair."; break;
    case 6: { int sc = skillLevel(me, SK_CREATIVITY) * 8 + irand(40); bool win = sc >= 50; skillGain(me, SK_CREATIVITY, 3); if (win) { G.money += 150; G.trophies++; } res = win ? "Your sand palace WON! +$150 and a trophy." : "A cute castle... until a wave took it. Next year!"; addMoodlet(me, win ? "Sandcastle Champ" : "Wave Wrecked", win ? EM_CONFIDENT : EM_PLAYFUL, 2, 300); break; }
    case 7: if (G.money < 5) { res = "No cash for ice cream. Tragic."; break; } G.money -= 5; me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + 20); fun(20); res = "Strawberry swirl with sprinkles. Perfection."; break;
    case 8: res = "Pink and gold fireworks over the water. Everyone goes quiet."; if (p >= 0) { changeRel(G.mc, p, 3, 10); res += " " + G.sims[p].name + " holds your hand."; } addMoodlet(me, "Fireworks Magic", EM_HAPPY, 3, 480); memo("Fireworks at Summer Splash"); break;
    case 9: fun(30); if (p >= 0) { changeRel(G.mc, p, 2, 10); res = "Stuck at the very top with " + G.sims[p].name + ". Best view in town."; } else res = "The whole town looks tiny and glittery from up here."; addMoodlet(me, "Top of the World", EM_HAPPY, 2, 300); break;
    case 10: if (G.money < 4) { res = "No cash. You just smell it longingly."; break; } G.money -= 4; me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + 10); fun(20); res = "Pink cotton candy as big as your head."; break;
    case 11: { bool w = irand(100) < 30 + skillLevel(me, SK_FITNESS) * 6; res = w ? "RINGER! You won a giant pink bunny plush!" : "So close! The ring bounced off. Rigged, probably."; if (w) { addMoodlet(me, "Won a Plush!", EM_HAPPY, 2, 480); memo("Won a giant bunny plush at the fair"); } fun(15); break; }
    case 12: skillGain(me, SK_CREATIVITY, 4); res = irand(2) ? "Your pumpkin has a cat face and a bow. Adorable." : "Your pumpkin looks... terrifying. Perfect."; fun(20); break;
    case 13: { bool w = irand(100) < 45 + skillLevel(me, SK_FITNESS) * 4; me.needs[N_HYGIENE] = std::max(0.f, me.needs[N_HYGIENE] - 15); res = w ? "Got one on the first try! The crowd cheers." : "Soaking wet. No apple. Iconic anyway."; fun(25); friendlyCrowd(2); break; }
    case 14: soc(30); friendlyCrowd(4); res = "Hay everywhere, cider in hand, everyone singing off-key."; addMoodlet(me, "Cozy Fall Vibes", EM_HAPPY, 2, 360); break;
    case 15: { int c = 5 + irand(20); fun(30); me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + 15); res = "Your bag has " + std::to_string(c) + " pieces of candy. Trading begins NOW."; break; }
    case 16: fun(35); addMoodlet(me, irand(2) ? "Scared Silly" : "Fearless", irand(2) ? EM_TENSE : EM_CONFIDENT, 1, 180); res = "Something grabbed your ankle. You screamed. Everyone heard."; break;
    case 17: { int sc = skillLevel(me, SK_STYLE) * 9 + irand(40); bool w = sc >= 55; if (w) { G.money += 100; G.followers += 60; } res = w ? "Best Costume! +$100 and the whole street wants a selfie." : "Honorable mention. The judges were biased."; addMoodlet(me, w ? "Costume Queen" : "Still Cute", EM_CONFIDENT, 1, 240); break; }
    case 18: { auto v = townHere(); int to = v.empty() ? -1 : v[irand((int)v.size())]; if (to >= 0) { changeRel(G.mc, to, 12, 0); res = "You got " + G.sims[to].name + " in the gift exchange. They LOVE their gift."; } else res = "You got fuzzy socks. You love fuzzy socks."; soc(25); break; }
    case 19: me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + 15); fun(20); res = "Hot cocoa with tiny marshmallows. Warm hands, warm heart."; addMoodlet(me, "Cozy Cocoa", EM_HAPPY, 2, 300); break;
    case 20: skillGain(me, SK_CREATIVITY, 3); fun(25); res = "A snow bunny with button eyes and a pink scarf. Perfect."; memo("Built a snow bunny at Winterfest"); break;
    case 21: soc(40); fun(30); friendlyCrowd(4); res = "Music, glitter, the whole town counting down..."; if (p >= 0) { changeRel(G.mc, p, 4, 15); res += " 3... 2... 1... and a midnight kiss with " + G.sims[p].name + "."; } addMoodlet(me, "New Year Energy", EM_ENERGIZED, 2, 600); break;
    case 22: lifeBoostGoal(G.mc, 10); addMoodlet(me, "Fresh Start", EM_INSPIRED, 3, 1440); res = "New year, new you. You wrote it down and everything."; break;
    case 23: addMoodlet(me, "Midnight Fireworks", EM_HAPPY, 3, 600); fun(30); res = "The sky explodes in pink and gold. Happy New Year, Starlight Bay!"; memo("New Year's fireworks"); break;
    }
    if (!res.empty()) rewardPopup(FEST[f].acts[i], res);
    sfxYay();
}

void seasonsHourly(int hour) {
    int f = festToday();
    if (f < 0) return;
    if (hour == 8) toast(std::string("Today: ") + FEST[f].name + " at " + ROOM_NAMES[FEST[f].room] + "! " + FEST[f].blurb);
    if (hour == FEST[f].h0) toast(std::string(FEST[f].name) + " is starting now at " + ROOM_NAMES[FEST[f].room] + "!");
}
void seasonsDaily() { G.festDone = 0; }

void seasonsDrawOverlay(float t) {
    const bool out = roomOutdoors(G.room);
    const int season = seasonOf(G.day);
    if (out && G.weather != WX_RAIN && G.weather != WX_STORM) {
        if (season == 0) for (int i = 0; i < 26; i++) {   // blossom petals
            float x = std::fmod(i * 61.3f + t * (14 + i % 5 * 3) + std::sin(t + i) * 12, 420.f) - 10, y = std::fmod(i * 37.7f + t * (18 + i % 4 * 4), 250.f) - 10;
            ellipse(x, y, 2.6f, 1.6f, C(255, 182, 210, 220)); ellipse(x + 1, y - 0.5f, 1.2f, 0.8f, C(255, 230, 240, 200));
        }
        if (season == 1) for (int i = 0; i < 12; i++) { float a = std::fabs(std::sin(t * 1.5f + i * 1.7f)); sparkle(std::fmod(i * 83.f, 400.f), 30 + std::fmod(i * 47.f, 160.f), 1.5f + a * 2.5f, C(255, 250, 200, (uint8_t)(120 + a * 120))); }
        if (season == 2) for (int i = 0; i < 20; i++) {   // falling leaves
            static const RGB L[4] = {{230, 120, 40}, {200, 70, 40}, {240, 180, 60}, {170, 90, 50}};
            float x = std::fmod(i * 71.f + t * 10 + std::sin(t * 1.3f + i) * 18, 420.f) - 10, y = std::fmod(i * 29.f + t * (16 + i % 3 * 5), 250.f) - 10;
            ellipse(x, y, 3.2f, 1.8f, C(L[i % 4])); line(x - 3, y, x + 3, y, 1, C(L[i % 4].r / 2, L[i % 4].g / 2, L[i % 4].b / 2));
        }
    }
    int f = festToday();
    if (f >= 0 && festOpenNow() && G.room == FEST[f].room) {   // bunting & lanterns
        static const RGB B[5] = {{255, 110, 180}, {255, 220, 90}, {120, 210, 255}, {180, 140, 255}, {120, 230, 170}};
        for (int i = 0; i < 20; i++) {
            float x0 = i * 21.f, sag = std::sin(i / 19.f * 3.14159f) * 10;
            RGB c = f == 5 ? (i % 2 ? RGB{255, 140, 40} : RGB{90, 50, 120}) : f == 6 ? (i % 2 ? RGB{255, 255, 255} : RGB{120, 200, 255}) : B[i % 5];
            tri(x0, 46 + sag, x0 + 18, 46 + sag, x0 + 9, 60 + sag, C(c));
        }
        line(0, 46, 400, 46, 1, C(255, 255, 255, 180));
        for (int i = 0; i < 6; i++) { float x = 30 + i * 68.f, gl = 0.5f + 0.5f * std::sin(t * 3 + i); circle(x, 70 + (i % 2) * 8, 6, C(255, 220, 140, (uint8_t)(140 + gl * 100))); circle(x, 70 + (i % 2) * 8, 10, C(255, 220, 140, (uint8_t)(30 + gl * 40))); }
        if ((f == 2 || f == 7) && hourNow() >= 20) for (int k = 0; k < 3; k++) {   // fireworks
            float ph = std::fmod(t * 0.6f + k * 0.33f, 1.f), cx = 80 + k * 120, cy = 50 + k * 12;
            for (int r = 0; r < 12; r++) { float a = r * 0.5236f; circle(cx + std::cos(a) * ph * 34, cy + std::sin(a) * ph * 34, 2, C(B[(r + k) % 5].r, B[(r + k) % 5].g, B[(r + k) % 5].b, (uint8_t)(255 * (1 - ph)))); }
        }
        textOutline(FEST[f].name, 200, 62, 0.45f, C(255, 255, 255), C(UI_INK), 1, 0.9f);
    }
}

bool seasonsNightSleep() {
    if (!ACT.active() || ACT.kind != AK_SLEEP || ACT.total < 300) return false;
    int h = hourNow();
    return h >= 19 || h < 7;
}
void seasonsDrawSleep(float t) {
    if (!seasonsNightSleep()) return;
    rect(0, 0, TOP_W, TOP_H, C(20, 14, 50, 225));
    for (int i = 0; i < 30; i++) sparkle(std::fmod(i * 97.f, 400.f), std::fmod(i * 53.f, 200.f), 1 + std::fabs(std::sin(t * 2 + i)) * 2, C(255, 255, 255, 200));
    circle(320, 60, 22, C(255, 246, 210)); circle(330, 54, 20, C(20, 14, 50));
    for (int k = 0; k < 3; k++) { float ph = std::fmod(t * 0.7f + k * 0.33f, 1.f); textOutline("Z", 180 + k * 18 + ph * 20, 140 - k * 16 - ph * 30, 0.6f + k * 0.15f, C(255, 255, 255, (uint8_t)(255 * (1 - ph))), C(UI_INK), 1, 0.8f); }
    textOutline("Sweet dreams...", 200, 190, 0.6f, C(255, 220, 240), C(UI_INK), 1, 1.f);
    text(clockStr(G.minute).c_str(), 200, 212, 0.45f, C(200, 190, 240), 1);
}
void seasonsGoodMorning() {
    momentCustom("Good Morning!", morningText(), "Let's go!", nullptr, nullptr, mornFn, 0, G.mc);
}
