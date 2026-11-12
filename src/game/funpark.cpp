#include "lifeext.hpp"
#include "funpark.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "district.hpp"
#include "extras.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>
using namespace col;

static bool pay(int amt) { if (G.money < amt) { toast("Not enough money!"); sfxNo(); return false; } G.money -= amt; return true; }
static const char* CH[8] = {"Wash the Dishes", "Do the Laundry", "Vacuum", "Take Out the Trash", "Mop the Floors", "Make the Bed", "Water the Plants", "Scrub the Bathroom"};

std::vector<std::pair<std::string, int>> funparkSpotOptions(int spot) {
    std::vector<std::pair<std::string, int>> v;
    const bool small = MC().stage <= ST_CHILD;
    switch (spot) {
    case SP_TICKETS: v.push_back({"Buy a Day Pass ($25)", 9800}); break;
    case SP_CAROUSEL: v.push_back({"Ride the Carousel", 9801}); break;
    case SP_CANDY: v.push_back({"Cotton Candy ($4)", 9802}); v.push_back({"Funnel Cake ($6)", 9803}); break;
    case SP_COASTER: v.push_back({small ? "Too short for this ride!" : "Ride the Sparkle Coaster", small ? 9899 : 9804}); break;
    case SP_FERRIS: v.push_back({"Ride the Ferris Wheel", 9805}); if (G.date.active) v.push_back({"Kiss at the Top (date)", 9806}); break;
    case SP_BUMPER: v.push_back({"Bumper Cars", 9807}); break;
    case SP_GAMEBOOTH: v.push_back({"Ring Toss ($3) - win a plush", 9808}); break;
    case SP_TURNSTILE: v.push_back({"Ride the train (Map > Subway)", 9899}); v.push_back({"People-Watch on the Platform", 9820}); break;
    case SP_BUSKER: v.push_back({"Listen to the Busker", 9821}); v.push_back({"Tip $5", 9822}); v.push_back({"Busk for Tips Yourself", 9823}); break;
    case SP_CABINETS: break;
    case SP_DDR: break;
    case SP_PRIZES: v.push_back({"Trade Tickets for a Prize ($10)", 9824}); break;
    case SP_WASHER: v.push_back({"Wash & Fold Laundry ($6)", 9825}); break;
    case SP_TANK: v.push_back({"Watch the Jellyfish", 9826}); break;
    case SP_TOUCHPOOL: v.push_back({"Touch a Sea Star", 9827}); break;
    case SP_EXHIBIT: v.push_back({"Tour the Fashion Exhibit ($12)", 9828}); break;
    case SP_SCULPT: v.push_back({"Sketch the Sculpture", 9829}); break;
    case SP_ROSES: v.push_back({"Stroll the Roses", 9830}); v.push_back({"Photo Shoot in the Garden", 9831}); break;
    case SP_GREENHOUSE: v.push_back({"Buy a Houseplant ($15)", 9832}); break;
    case SP_ROOFBAR: v.push_back({"Sunset Mocktail ($10)", 9833}); if (G.date.active) v.push_back({"Dance Under the Stars (date)", 9834}); break;
    case SP_SKYVIEW: v.push_back({"Take in the City Lights", 9835}); break;
    }
    return v;
}
std::vector<std::pair<std::string, int>> choresOptions() {
    std::vector<std::pair<std::string, int>> v;
    if (!roomIsHome(G.room)) { v.push_back({"Go home to do chores.", 9899}); return v; }
    for (int k = 0; k < 8; k++) v.push_back({CH[k], 9900 + k});
    v.push_back({"Do ALL Chores (3 hrs)", 9910});
    return v;
}

static bool hasPass = false; static uint16_t passDay = 0xFFFF;
static bool ride() { if (hasPass && passDay == G.day) return true; return pay(6); }

void funparkOption(int id) {
    Sim& me = MC(); Activity a;
    if (id >= 9900 && id <= 9910) {
        const bool all = id == 9910;
        a = makeAct(all ? "Doing All the Chores" : CH[id - 9900], all ? 180 : 40, 8);
        a.rate[N_FUN] = all ? -6 : -4; a.rate[N_HYGIENE] = -4; a.rate[N_ENERGY] = -5; startActivity(a);
        for (Sim& s : G.sims) if (s.household) addMoodlet(s, all ? "Spotless House" : "Tidy Home", EM_HAPPY, all ? 2 : 1, all ? 1440 : 480);
        lxClean(all ? 100 : 15);
        if (all) { addMoodlet(me, "Productive!", EM_CONFIDENT, 2, 600); lxRepAdd(CI_FAMILY, 2, "helps at home"); }
        toast(all ? "Every chore done. The house sparkles!" : std::string(CH[id - 9900]) + ": done!");
        return;
    }
    switch (id) {
    case 9820: a = makeAct("People-Watching", 30, 8); a.rate[N_FUN] = 8; startActivity(a); return;
    case 9821: a = makeAct("Listening to the Busker", 20, 8); a.rate[N_FUN] = 12; startActivity(a); return;
    case 9822: if (!pay(5)) return; addMoodlet(me, "Supported the Arts", EM_HAPPY, 1, 180); return;
    case 9823: { a = makeAct("Busking", 60, 8); a.rate[N_SOCIAL] = 8; startActivity(a); lxSkill(XS_SING, 15); const int tip = 5 + irand(20) + (int)me.skills[SK_CHARISMA] * 4; G.money += tip; toast("Busking earned $" + std::to_string(tip) + "!"); return; }
    case 9824: if (!pay(10)) return; addMoodlet(me, "Arcade Prize!", EM_HAPPY, 1, 300); toast("You got a glitter alien plush!"); lxSkill(XS_GAMING, 6); return;
    case 9825: if (!pay(6)) return; a = makeAct("Doing Laundry", 60, 8); startActivity(a); lxClean(20); addMoodlet(me, "Fresh Laundry", EM_HAPPY, 1, 600); return;
    case 9826: a = makeAct("Watching Jellyfish", 30, 8); a.rate[N_FUN] = 12; startActivity(a); addMoodlet(me, "Calm & Glowy", EM_FINE, 1, 240); return;
    case 9827: a = makeAct("Touch Pool", 15, 8); a.rate[N_FUN] = 15; startActivity(a); return;
    case 9828: if (!pay(12)) return; a = makeAct("Fashion Exhibit", 60, 8); a.skill = SK_STYLE; a.skillRate = 10; startActivity(a); lxRepAdd(CI_FASHION, 1, "museum regular"); return;
    case 9829: a = makeAct("Sketching", 45, 8); a.skill = SK_CREATIVITY; a.skillRate = 10; startActivity(a); lxSkill(XS_PAINT, 10); return;
    case 9830: a = makeAct("Strolling the Roses", 30, 8); a.rate[N_FUN] = 10; startActivity(a); addMoodlet(me, "Smelled the Roses", EM_HAPPY, 1, 240); return;
    case 9831: a = makeAct("Garden Photo Shoot", 40, 8); startActivity(a); lxSkill(XS_PHOTO, 14); G.followers += 5 + irand(20); return;
    case 9832: if (!pay(15)) return; lxClean(5); addMoodlet(me, "Plant Parent", EM_HAPPY, 1, 600); toast("A new houseplant to brighten the place."); return;
    case 9833: if (!pay(10)) return; addMoodlet(me, "Sunset Sipping", EM_HAPPY, 1, 180); return;
    case 9834: if (G.date.partner < 0) return; changeRel(G.mc, G.date.partner, 3, 10); G.date.score += 8; addMoodlet(me, "Starlit Dance", EM_FLIRTY, 2, 300); toast("Dancing under the city stars."); return;
    case 9835: a = makeAct("Taking in the View", 20, 8); a.rate[N_FUN] = 8; startActivity(a); addMoodlet(me, "On Top of the City", EM_INSPIRED, 1, 240); return;
    case 9800: if (!pay(25)) return; hasPass = true; passDay = (uint16_t)G.day; toast("Day pass! Unlimited rides today."); return;
    case 9801: if (!ride()) return; a = makeAct("Riding the Carousel", 20, 8); a.rate[N_FUN] = 25; startActivity(a); return;
    case 9802: if (!pay(4)) return; me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + 10); addMoodlet(me, "Sugar Rush", EM_PLAYFUL, 1, 120); return;
    case 9803: if (!pay(6)) return; me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + 25); toast("Powdered sugar everywhere."); return;
    case 9804: if (!ride()) return; a = makeAct("Riding the Coaster", 15, 8); a.rate[N_FUN] = 50; startActivity(a);
        if (irand(100) < 25) { addMoodlet(me, "Queasy", EM_UNCOMFORTABLE, 1, 90); toast("WHEEE... ugh, that loop."); } else { addMoodlet(me, "Adrenaline!", EM_ENERGIZED, 2, 180); toast("AAAAAH! Again!"); } return;
    case 9805: if (!ride()) return; a = makeAct("Ferris Wheel", 25, 8); a.rate[N_FUN] = 20; startActivity(a); addMoodlet(me, "On Top of the World", EM_HAPPY, 1, 180); return;
    case 9806: if (G.date.partner < 0) return; changeRel(G.mc, G.date.partner, 3, 12); G.date.score += 10; addMoodlet(me, "Kiss at the Top", EM_FLIRTY, 2, 360); toast("A kiss at the very top of the wheel."); sfxYay(); return;
    case 9807: if (!ride()) return; a = makeAct("Bumper Cars", 15, 8); a.rate[N_FUN] = 35; a.rate[N_SOCIAL] = 10; startActivity(a); return;
    case 9808: if (!pay(3)) return;
        if (irand(100) < 30 + (int)me.skills[SK_FITNESS] * 5) { addMoodlet(me, "Won a Plush!", EM_HAPPY, 2, 600); toast("Ringer! You won a giant plush bunny!"); sfxYay(); }
        else toast("So close! Try again?"); return;
    }
}

void funparkDrawRoom(int room, float t, bool night) {
    grad(0, 0, TOP_W, 158, night ? C(30, 20, 70) : C(140, 210, 250), night ? C(80, 40, 110) : C(220, 240, 255));
    if (room == R_FUNPARK) {
        rect(20, 70, 70, 86, C(255, 240, 200)); rect(16, 62, 78, 10, C(255, 90, 150)); text("TICKETS", 55, 90, 0.35f, C(UI_INK), 1);
        const float cx = 200; rect(cx - 70, 60, 140, 6, C(255, 120, 180)); for (int k = 0; k < 7; k++) { const float x = cx - 60 + k * 20, y = 100 + std::sin(t * 2 + k) * 8; rect(x, 66, 2, y - 66, C(220, 200, 120)); circle(x, y, 7, C(k % 2 ? RGB{255, 255, 255} : RGB{150, 220, 255})); }
        rect(cx - 72, 140, 144, 16, C(255, 180, 210));
        rect(300, 90, 70, 66, C(255, 200, 230)); circle(335, 80, 18, C(255, 170, 220)); text("CANDY", 335, 110, 0.35f, C(UI_INK), 1);
        textOutline("SPARKLE LAND", 200, 10, 0.7f, C(255, 255, 255), C(255, 46, 140), 1);
    } else if (room != R_FUNRIDES) {
        static const RGB WALL[7] = {{180, 186, 196}, {40, 20, 70}, {200, 230, 240}, {20, 60, 110}, {240, 236, 230}, {170, 220, 160}, {40, 40, 80}};
        static const char* TITLE[7] = {"METRO CENTRAL", "PIXEL PALACE", "SUDS LAUNDROMAT", "MAPLE AQUARIUM", "MUSEUM OF GLAM", "BOTANICAL GARDENS", "SKYLINE ROOFTOP"};
        const int k = room - R_SUBWAY;
        grad(0, 0, TOP_W, 158, C(WALL[k]), C(WALL[k].r * 4 / 5, WALL[k].g * 4 / 5, WALL[k].b * 4 / 5));
        switch (room) {
        case R_SUBWAY: rect(0, 120, TOP_W, 36, C(70, 70, 80)); rect(0, 118, TOP_W, 4, C(250, 210, 40)); for (int i = 0; i < 10; i++) rect(i * 40.f + 6, 20, 30, 18, C(255, 255, 255)); rect(std::fmod(t * 80, 800.f) - 400, 70, 360, 48, C(200, 40, 60)); break;
        case R_PIXELARCADE: for (int i = 0; i < 6; i++) { rect(20 + i * 62.f, 70, 44, 86, C(30, 20, 50)); rect(26 + i * 62.f, 80, 32, 24, C(i % 2 ? RGB{80, 255, 200} : RGB{255, 80, 200})); } break;
        case R_LAUNDRO: for (int i = 0; i < 6; i++) { rect(20 + i * 62.f, 100, 52, 56, C(240, 240, 245)); circle(46 + i * 62.f, 126, 16, C(120, 180, 220)); circle(46 + i * 62.f, 126, 10 + std::sin(t * 6 + i) * 2, C(220, 240, 255)); } break;
        case R_AQUARIUM: rect(30, 30, 340, 120, C(20, 100, 170)); for (int i = 0; i < 7; i++) { const float x = 60 + i * 45.f, y = 70 + std::sin(t + i) * 20; circle(x, y, 9, C(255, 180, 230, 180)); for (int j = 0; j < 3; j++) rect(x - 5 + j * 4, y + 6, 1, 14, C(255, 200, 240, 140)); } break;
        case R_MUSEUM: for (int i = 0; i < 4; i++) { rect(30 + i * 95.f, 40, 60, 70, C(200, 170, 110)); rect(34 + i * 95.f, 44, 52, 62, C(i % 2 ? RGB{230, 120, 160} : RGB{120, 160, 230})); } break;
        case R_GARDENS: for (int i = 0; i < 16; i++) { circle(20 + i * 24.f, 130 + (i % 2) * 8, 10, C(70, 150, 80)); circle(20 + i * 24.f, 124 + (i % 2) * 8, 4, C(i % 3 ? RGB{240, 80, 120} : RGB{255, 220, 80})); } break;
        case R_ROOFTOP: for (int i = 0; i < 12; i++) { const float h = 40 + (i * 37) % 70; rect(i * 34.f, 150 - h, 30, h, C(30, 30, 60)); for (int w = 0; w < 4; w++) rect(i * 34.f + 4 + (w % 2) * 12, 156 - h + (w / 2) * 12, 6, 6, C(255, 220, 120)); } rect(0, 140, TOP_W, 18, C(90, 70, 60)); break;
        }
        textOutline(TITLE[k], 200, 6, 0.55f, C(255, 255, 255), C(UI_INK), 1);
    } else {
        for (int k = 0; k < 9; k++) { const float x = 10 + k * 15.f; rect(x, 150 - std::fabs(std::sin(k * 0.7f)) * 90, 3, 158, C(200, 60, 80)); }
        rect(10, 60, 140, 3, C(200, 60, 80)); circle(30 + std::fmod(t * 60, 120), 58, 5, C(255, 220, 0));
        const float fx = 220; circle(fx, 70, 52, C(255, 255, 255, 60)); for (int k = 0; k < 8; k++) { const float a = t * 0.5f + k * 0.785f; circle(fx + std::cos(a) * 50, 70 + std::sin(a) * 50, 6, C(k % 2 ? RGB{255, 90, 150} : RGB{90, 190, 255})); }
        rect(fx - 3, 70, 6, 88, C(120, 120, 140));
        rect(300, 120, 90, 36, C(80, 80, 120)); circle(320, 136, 7, C(255, 80, 80)); circle(350, 140, 7, C(80, 200, 120));
        textOutline("RIDES & GAMES", 200, 4, 0.55f, C(255, 255, 255), C(255, 46, 140), 1);
    }
    rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(200, 190, 170));
    if (night) for (int k = 0; k < 20; k++) circle(10 + k * 20.f, 150, 2, C(255, 230, 120, 200));
}
