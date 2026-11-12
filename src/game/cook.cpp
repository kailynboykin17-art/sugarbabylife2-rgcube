#include "content.hpp"
// Kitchen mode, Cooking-Mama style: pick a recipe and Chef Bea walks you through quick touch
// steps (chop, crack, stir, pour, flip, sprinkle, roll, bake). Every step is graded, then Bea
// awards a bronze, silver or gold medal. Serve the dish, add a drink, and eat together -
// some dishes + drinks make secret recipe combos.
#include "cook.hpp"
#include "draw.hpp"
#include "sim.hpp"
#include "audio.hpp"
#include "goals.hpp"
#include "lifeplus.hpp"
#include "economy.hpp"
#include <cmath>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>

using namespace col;

const char* FOOD_NAMES[F_COUNT] = {"Chicken", "Pizza", "Cookies", "Eggs", "Pancakes", "Cupcakes", "Burger", "Pasta", "Tacos", "Mac & Cheese", "Lasagna", "Brownies", "Fried Rice", "Ramen", "Grilled Cheese", "Steak", "Cinnamon Rolls", "Smoothie Bowl"};
static const int UNLOCK[F_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 4, 3, 2, 3, 0, 5, 4, 1};   // cooking skill level needed
const char* DRINK_NAMES[NUM_DRINKS] = {"Water", "Pink Lemonade", "Milk", "Smoothie", "Choc Shake", "Iced Latte"};
static const RGB DRINK_COL[NUM_DRINKS] = {{190, 225, 250}, {255, 160, 190}, {250, 250, 245}, {240, 90, 140}, {120, 70, 50}, {200, 160, 120}};

// ---------------------------------------------------------------- recipes = a list of steps
enum StepType { KS_CHOP, KS_CRACK, KS_STIR, KS_POUR, KS_FLIP, KS_SPRINKLE, KS_ROLL, KS_OVEN };
struct Step { uint8_t type; const char* what; RGB col; };
struct RecipeSteps { Step s[5]; uint8_t n; };
static const RGB EGG{250, 246, 236}, BATTER{246, 222, 160}, CHOCO{110, 64, 40}, GRN{110, 190, 90}, TOM{230, 60, 60}, DOUGH{246, 226, 186}, CHEESE{255, 206, 70}, SAUCE{200, 50, 40}, MILK{250, 250, 250}, WATER{150, 200, 250}, MEAT{170, 90, 60}, ONION{240, 230, 250}, SPRK{255, 120, 190};
static const RecipeSteps STEPS[F_COUNT] = {
    {{{KS_CHOP, "the herbs", GRN}, {KS_SPRINKLE, "the seasoning", {200, 80, 40}}, {KS_OVEN, "the chicken", MEAT}}, 3},
    {{{KS_ROLL, "the dough", DOUGH}, {KS_POUR, "the sauce", SAUCE}, {KS_SPRINKLE, "the toppings", TOM}, {KS_OVEN, "the pizza", DOUGH}}, 4},
    {{{KS_CRACK, "the egg", EGG}, {KS_STIR, "the cookie dough", BATTER}, {KS_SPRINKLE, "the choc chips", CHOCO}, {KS_OVEN, "the cookies", BATTER}}, 4},
    {{{KS_CRACK, "the eggs", EGG}, {KS_STIR, "the eggs", {255, 220, 90}}, {KS_FLIP, "the omelette", {255, 214, 80}}}, 3},
    {{{KS_CRACK, "the eggs", EGG}, {KS_STIR, "the batter", BATTER}, {KS_POUR, "the batter", BATTER}, {KS_FLIP, "the pancakes", {226, 170, 90}}}, 4},
    {{{KS_CRACK, "the eggs", EGG}, {KS_STIR, "the batter", BATTER}, {KS_POUR, "the batter", BATTER}, {KS_OVEN, "the cupcakes", BATTER}, {KS_SPRINKLE, "the sprinkles", SPRK}}, 5},
    {{{KS_CHOP, "the lettuce", GRN}, {KS_ROLL, "the patty", MEAT}, {KS_FLIP, "the burger", MEAT}}, 3},
    {{{KS_CHOP, "the tomatoes", TOM}, {KS_POUR, "the water", WATER}, {KS_STIR, "the noodles", {245, 226, 160}}}, 3},
    {{{KS_CHOP, "the onions", ONION}, {KS_FLIP, "the taco meat", MEAT}, {KS_SPRINKLE, "the cheese", CHEESE}}, 3},
    {{{KS_POUR, "the milk", MILK}, {KS_STIR, "the cheese sauce", CHEESE}, {KS_SPRINKLE, "the breadcrumbs", {214, 170, 100}}}, 3},
    {{{KS_CHOP, "the veggies", GRN}, {KS_POUR, "the sauce", SAUCE}, {KS_SPRINKLE, "the cheese", CHEESE}, {KS_OVEN, "the lasagna", SAUCE}}, 4},
    {{{KS_CRACK, "the eggs", EGG}, {KS_STIR, "the cocoa batter", CHOCO}, {KS_POUR, "the batter", CHOCO}, {KS_OVEN, "the brownies", CHOCO}}, 4},
    {{{KS_CHOP, "the green onions", GRN}, {KS_CRACK, "the egg", EGG}, {KS_STIR, "the rice", {250, 236, 200}}, {KS_SPRINKLE, "the soy sauce", {120, 70, 40}}}, 4},
    {{{KS_POUR, "the broth", {230, 190, 120}}, {KS_CHOP, "the scallions", GRN}, {KS_CRACK, "the soft egg", EGG}, {KS_STIR, "the noodles", {250, 230, 160}}}, 4},
    {{{KS_SPRINKLE, "the cheese", CHEESE}, {KS_FLIP, "the sandwich", {240, 200, 120}}}, 2},
    {{{KS_SPRINKLE, "the seasoning", {200, 80, 40}}, {KS_FLIP, "the steak", MEAT}, {KS_CHOP, "the steak", MEAT}}, 3},
    {{{KS_ROLL, "the dough", DOUGH}, {KS_SPRINKLE, "the cinnamon sugar", {190, 120, 60}}, {KS_OVEN, "the rolls", DOUGH}, {KS_POUR, "the icing", MILK}}, 4},
    {{{KS_CHOP, "the strawberries", TOM}, {KS_POUR, "the smoothie", {240, 90, 140}}, {KS_SPRINKLE, "the granola", {214, 170, 100}}}, 3},
};
static const char* STEP_VERB[8] = {"Chop", "Crack", "Stir", "Pour", "Flip", "Add", "Roll", "Bake"};
static const char* STEP_HOW[8] = {"Tap each dotted line to slice!", "Tap when the meter hits GOLD - 3 times!", "Draw circles in the bowl - fast!",
                                  "Hold to pour. Let go at the line!", "Tap when the heat hits GOLD - flip both sides!", "Tap the sparkles before they vanish!",
                                  "Swipe up and down to roll it out!", "Tap TAKE OUT when the heat is GOLD!"};
static const float STEP_TIME[8] = {6, 8, 7, 6, 9, 11, 6, 8};

// recipe combos: dishes (+ drink) on the table together
struct Recipe { const char* name; int8_t a, b, c, drink; };
static const Recipe RECIPES[] = {
    {"Big Breakfast", F_PANCAKES, F_EGGS, -1, 2}, {"Date Night Dinner", F_PASTA, F_CHICKEN, -1, 1}, {"Diner Combo", F_BURGER, -1, -1, 4},
    {"Bake Sale", F_COOKIES, F_CUPCAKES, F_BROWNIES, 2}, {"Pizza Party", F_PIZZA, F_COOKIES, -1, 1}, {"Taco Tuesday", F_TACOS, -1, -1, 3},
    {"Comfort Food Night", F_MAC, F_CHICKEN, -1, 0}, {"Sunday Supper", F_LASAGNA, F_PASTA, -1, 1}, {"Brunch Babe", F_PANCAKES, F_CUPCAKES, -1, 5},
};
static const int NUM_RECIPES = sizeof(RECIPES) / sizeof(RECIPES[0]);

// ---------------------------------------------------------------- state
struct Held { int food = -1, st = FS_RAW, stars = 0; };
static Held held;
static int plates[4] = {-1, -1, -1, -1}, plateSt[4], plateStars[4];
static int cupDrink = -1;
static float cupFill = 0;
static std::string msg;
static float msgT = 0;
static int tut = -1;   // cooking class step, -1 = off
enum { T_PICK, T_PLAY, T_SERVE, T_EAT, T_DONE };
static const char* TUT_TEXT[T_DONE] = {
    "Hi, I'm Chef Bea! Let's make PANCAKES. Tap Pancakes in the Recipe Book.",
    "Follow my steps on the touch screen. Every step is graded - go for GOLD!",
    "Look at that! Now tap SERVE to put it on the table.",
    "Pour a drink if you like, then tap FAMILY DINNER! Everyone at home eats. Some dishes + drinks make secret combos.",
};
// cooking session
enum { PH_HUB, PH_INTRO, PH_STEP, PH_FEEDBACK, PH_RESULT };
static int phase = PH_HUB, dish = -1, stepI = 0;
static float pt = 0, limit = 6;            // phase timer, step time limit
static int scores[5];
static int curScore = 0;
// per-step scratch
static bool cut[6]; static int misses = 0, count = 0, lastMedal = 0;
static float meter = 0, acc = 0, prevA = 0, level = 0, lastY = 0, travel = 0; static bool hadPrev = false, pouring = false; static int dir = 0;
static float tx[8], ty[8], tborn = 0; static bool thit[8];

static void say2(const std::string& s) { msg = s; msgT = 3; }
static int cookLvl() { return skillLevel(MC(), SK_COOKING); }
static bool unlocked(int f) { return cookLvl() >= UNLOCK[f]; }
static const Step& curStep() { return STEPS[dish].s[stepI]; }

void cookStart() {
    held = Held(); phase = PH_HUB;
    if (!G.cookTutDone) { tut = T_PICK; msg = "Cooking class with Chef Bea!"; msgT = 4; }
    else { tut = -1; msg = G.groceries ? "Fresh groceries in stock: +1 star on every dish!" : "Pick a recipe from the Recipe Book!"; msgT = 4; }
}

void drawFood(int f, int st, float x, float y, float s, float t) {
    auto pick = [st](RGB raw, RGB done) { return st == FS_RAW ? raw : st == FS_DONE ? done : RGB{60, 40, 30}; };
    Pen p(x, y, 30, 20, s);
    switch (f) {
    case F_CHICKEN: { RGB c = pick({250, 190, 180}, {214, 140, 60}); p.E(12, 11, 22, 16, C(c)); p.R(20, 9, 8, 3, C(CREAM)); p.Ci(28, 8, 2.2f, C(CREAM)); p.Ci(28, 12, 2.2f, C(CREAM)); break; }
    case F_PIZZA: { RGB c = pick({250, 230, 190}, {240, 196, 100}); p.Ci(15, 10, 11, C(shade(c, 0.85f))); p.Ci(15, 10, 9, C(c));
        for (int i = 0; i < 5; i++) p.Ci(15 + std::cos(i * 1.3f) * 5, 10 + std::sin(i * 1.3f) * 5, 1.8f, C(st == FS_BURNT ? RGB{40, 20, 20} : RGB{200, 50, 50})); break; }
    case F_COOKIES: { RGB c = pick({240, 220, 180}, {200, 140, 80}); for (int i = 0; i < 3; i++) { p.Ci(6 + i * 9, 12, 4.5f, C(c)); p.Ci(5 + i * 9, 11, 0.9f, C(CHOC)); p.Ci(8 + i * 9, 13, 0.9f, C(CHOC)); } break; }
    case F_EGGS: p.E(15, 11, 22, 14, C(st == FS_BURNT ? RGB{120, 100, 80} : RGB{255, 255, 250})); p.Ci(15, 11, 4, C(st == FS_RAW ? RGB{255, 220, 80} : RGB{255, 190, 50})); break;
    case F_PANCAKES: { RGB c = pick({245, 230, 200}, {220, 160, 90}); for (int i = 0; i < 3; i++) p.E(15, 16 - i * 4, 24, 6, C(shade(c, 1.f - i * 0.05f))); if (st == FS_DONE) { p.E(15, 7, 14, 4, C(150, 80, 30)); p.R(12, 3, 6, 3, C(255, 240, 160)); } break; }
    case F_CUPCAKES: { RGB c = pick({245, 230, 200}, {220, 170, 110}); for (int i = 0; i < 2; i++) { p.Q(4 + i * 12, 12, 14 + i * 12, 12, 13 + i * 12, 19, 5 + i * 12, 19, C(HOT)); p.E(9 + i * 12, 10, 12, 8, C(c)); if (st == FS_DONE) { p.Ci(9 + i * 12, 7, 4, C(BLUSH)); p.Sp(9 + i * 12, 3, 2, C(GOLD)); } } break; }
    case F_BURGER: { RGB c = pick({230, 120, 120}, {120, 70, 40}); p.E(15, 15, 22, 7, C(220, 170, 90)); p.E(15, 12, 22, 5, C(c)); p.E(15, 10, 22, 3, C(GREEN)); p.E(15, 7, 22, 9, C(230, 176, 96)); break; }
    case F_PASTA: { RGB c = pick({245, 236, 190}, {245, 210, 120}); p.E(15, 13, 26, 10, C(CREAM)); p.E(15, 11, 20, 7, C(c)); if (st == FS_DONE) { p.Ci(12, 10, 2, C(200, 50, 50)); p.Ci(18, 11, 2, C(200, 50, 50)); } break; }
    case F_TACOS: { RGB c = pick({250, 236, 180}, {236, 190, 90}); for (int i = 0; i < 2; i++) { p.E(9 + i * 12, 12, 12, 12, C(c)); p.E(9 + i * 12, 9, 10, 4, C(st == FS_DONE ? RGB{120, 70, 40} : RGB{230, 120, 120})); p.Ci(7 + i * 12, 7, 1.6f, C(GREEN)); p.Ci(11 + i * 12, 7, 1.4f, C(255, 210, 60)); } break; }
    case F_MAC: { RGB c = pick({252, 240, 170}, {255, 196, 60}); p.E(15, 14, 26, 10, C(CREAM)); p.E(15, 11, 21, 8, C(c)); for (int i = 0; i < 5; i++) p.Ci(9 + i * 3, 10 + (i % 2), 1.4f, C(shade(c, 0.85f))); break; }
    case F_LASAGNA: { RGB c = pick({250, 220, 200}, {230, 150, 80}); p.R(3, 6, 24, 12, C(200, 200, 210)); for (int i = 0; i < 3; i++) { p.R(5, 8 + i * 3, 20, 2, C(c)); p.R(5, 10 + i * 3, 20, 1, C(200, 50, 50)); } break; }
    case F_FRIEDRICE: { RGB c = pick({250, 240, 210}, {240, 200, 120}); p.E(15, 14, 26, 10, C(CREAM)); p.E(15, 11, 21, 8, C(c)); for (int i = 0; i < 6; i++) p.Ci(8 + i * 3, 10 + (i % 2) * 2, 1.2f, C(i % 2 ? GREEN : RGB{240, 120, 60})); break; }
    case F_RAMEN: { RGB c = pick({250, 236, 180}, {240, 200, 110}); p.E(15, 13, 26, 12, C(200, 60, 60)); p.E(15, 10, 22, 7, C(230, 190, 120)); for (int i = 0; i < 4; i++) p.R(7 + i * 4, 8, 2, 5, C(c)); p.Ci(20, 9, 3, C(255, 255, 250)); p.Ci(20, 9, 1.5f, C(255, 190, 40)); break; }
    case F_GRILLCHEESE: { RGB c = pick({250, 236, 200}, {230, 160, 70}); p.R(5, 7, 20, 12, C(c)); p.R(5, 12, 20, 2, C(255, 200, 40)); p.R(5, 7, 20, 2, C(shade(c, 1.15f))); break; }
    case F_STEAK: { RGB c = pick({220, 110, 110}, {130, 70, 50}); p.E(15, 14, 26, 9, C(CREAM)); p.E(14, 12, 18, 8, C(c)); for (int i = 0; i < 3; i++) p.R(8 + i * 5, 9, 1, 7, C(shade(c, 0.6f))); p.Ci(23, 12, 2, C(GREEN)); break; }
    case F_CINNAROLL: { RGB c = pick({250, 230, 200}, {210, 150, 90}); for (int i = 0; i < 2; i++) { p.Ci(9 + i * 12, 12, 6, C(c)); p.Ci(9 + i * 12, 12, 3.5f, C(shade(c, 0.75f))); p.Ci(9 + i * 12, 12, 1.5f, C(c)); } p.R(4, 9, 22, 2, C(255, 255, 255)); break; }
    case F_SMOOTHIEBOWL: { RGB c = pick({250, 170, 200}, {240, 90, 140}); p.E(15, 14, 26, 10, C(255, 255, 255)); p.E(15, 11, 22, 7, C(c)); p.Ci(10, 10, 2, C(230, 40, 60)); p.Ci(15, 9, 2, C(255, 230, 120)); p.R(18, 9, 6, 2, C(214, 170, 100)); break; }
    case F_BROWNIES: { RGB c = pick({170, 120, 90}, {90, 50, 34}); for (int i = 0; i < 3; i++) { p.R(3 + i * 8.5f, 8, 7.5f, 9, C(c)); p.R(3 + i * 8.5f, 8, 7.5f, 2, C(shade(c, 1.25f))); } break; }
    }
    if (st == FS_DONE) for (int i = 0; i < 2; i++) { float k = std::fmod(t + i * 0.5f, 1.f); circle(x - 4 + i * 8, y - 20 * s - k * 10, 2 + k * 2, C(255, 255, 255, (uint8_t)(120 * (1 - k)))); }
    if (st == FS_BURNT) { circle(x, y - 24 * s - std::fmod(t, 1.f) * 10, 5, C(60, 60, 60, 120)); }
}

void drawDrink(int d, float x, float y, float s, float fill) {
    quad(x - 8 * s, y - 24 * s, x + 8 * s, y - 24 * s, x + 6 * s, y, x - 6 * s, y, C(255, 255, 255, 110));
    if (d >= 0) {
        float h = 22 * s * fill;
        float k = h / (24 * s);
        quad(x - 6 * s - 2 * s * k, y - h, x + 6 * s + 2 * s * k, y - h, x + 6 * s, y, x - 6 * s, y, C(DRINK_COL[d], 220));
        if (fill > 0.9f) { line(x + 3 * s, y - 30 * s, x + 1 * s, y - 14 * s, 1.5f * s, C(HOT)); circle(x - 4 * s, y - 24 * s, 2.5f * s, C(255, 220, 80)); }
    }
    outline(x - 8 * s, y - 24 * s, 16 * s, 1, 1, C(255, 255, 255, 200));
}

// ---------------------------------------------------------------- the session
static float cookTimeScale() { return (1.f + 0.05f * cookLvl()) * (tut >= 0 ? 1.4f : 1.f) * lotCookBonus(); }
static void beginStep() {
    const Step& st = curStep();
    phase = PH_STEP; pt = 0; limit = STEP_TIME[st.type] * cookTimeScale(); curScore = 0;
    for (bool& c : cut) c = false;
    misses = count = 0; meter = acc = level = travel = 0; hadPrev = pouring = false; dir = 0; tborn = 0;
    for (int i = 0; i < 8; i++) { tx[i] = 70 + (i * 97) % 180; ty[i] = 92 + (i * 53) % 90; thit[i] = false; }
}
static void startDish(int f) {
    dish = f; stepI = 0; for (int& x : scores) x = 0;
    phase = PH_INTRO; pt = 0;
    if (tut == T_PICK) tut = T_PLAY;
    sfxTap();
}
static void finishStep(int score) {
    score = std::max(0, std::min(100, score));
    scores[stepI] = score; curScore = score;
    phase = PH_FEEDBACK; pt = 0;
    if (score >= 85) sfxYay(); else if (score < 40) sfxNo(); else sfxTap();
}
static int dishAvg() { int n = STEPS[dish].n, t = 0; for (int i = 0; i < n; i++) t += scores[i]; return t / n; }
static int medalFor(int avg) { return avg >= 85 ? 3 : avg >= 65 ? 2 : avg >= 40 ? 1 : 0; }

// one frame of the current step; returns true when finished
static void stepUpdate(const Input& in) {
    const Step& st = curStep();
    const bool held2 = (in.held & KEY_TOUCH) != 0;
    const float px = in.tp.px, py = in.tp.py;
    pt += in.dt;
    bool timeUp = pt >= limit;
    switch (st.type) {
    case KS_CHOP: {
        const int N = 5;
        if (in.touch) {
            int best = -1; float bd = 16;
            if (py > 70 && py < 170) for (int i = 0; i < N; i++) { float lx = 40 + (i + 1) * 240.f / (N + 1); if (!cut[i] && std::fabs(px - lx) < bd) { bd = std::fabs(px - lx); best = i; } }
            if (best >= 0) { cut[best] = true; count++; sfx(900, 500, 40); } else { misses++; sfx(200, 150, 40); }
        }
        if (count >= N || timeUp) finishStep(count * 100 / N - misses * 10 + (count >= N ? (int)((1 - pt / limit) * 15) : 0));
        break;
    }
    case KS_CRACK:
        meter = 0.5f + 0.5f * std::sin(pt * 3.4f);
        if (in.touch && py > 30) {
            int add = meter >= 0.62f && meter <= 0.86f ? 34 : meter >= 0.45f && meter <= 0.95f ? 22 : 8;
            curScore += add; count++; sfx(add >= 34 ? 1200 : 500, add >= 34 ? 900 : 300, 60);
        }
        if (count >= 3 || timeUp) finishStep(curScore);
        break;
    case KS_STIR:
        if (held2) {
            float dx = px - 160, dy = py - 135, d = std::sqrt(dx * dx + dy * dy);
            if (d > 15 && d < 110) {
                float a = std::atan2(dy, dx);
                if (hadPrev) { float da = a - prevA; while (da > 3.14159f) da -= 6.28318f; while (da < -3.14159f) da += 6.28318f; if (std::fabs(da) < 1.2f) acc += std::fabs(da); }
                prevA = a; hadPrev = true;
            } else hadPrev = false;
        } else hadPrev = false;
        if (acc >= 4 * 6.28318f) finishStep(100 - (int)(std::max(0.f, pt / limit - 0.6f) * 40));
        else if (timeUp) finishStep((int)(acc / (4 * 6.28318f) * 90));
        break;
    case KS_POUR:
        if (held2 && py > 30) { pouring = true; level += in.dt * 0.32f; }
        if ((pouring && !held2) || level >= 1.f || timeUp) {
            if (level >= 1.f) finishStep(10);
            else finishStep(100 - (int)(std::fabs(level - 0.75f) * 380));
        }
        break;
    case KS_FLIP:
        meter += in.dt * 0.3f;
        if (in.touch && py > 30) {
            int add = meter >= 0.55f && meter <= 0.75f ? 50 : meter >= 0.4f && meter <= 0.88f ? 32 : 10;
            curScore += add; count++; meter = 0; sfx(700, 1100, 70);
        } else if (meter > 1.05f) { curScore += 4; count++; meter = 0; sfx(150, 100, 120); }
        if (count >= 2 || timeUp) finishStep(curScore);
        break;
    case KS_SPRINKLE: {
        int idx = (int)(pt / 1.3f);
        if (idx < 8 && in.touch && !thit[idx]) {
            float dx = px - tx[idx], dy = py - ty[idx];
            if (dx * dx + dy * dy < 22 * 22) { thit[idx] = true; count++; sfx(1300, 1600, 40); } else misses++;
        }
        if (idx >= 8 || timeUp) finishStep(count * 100 / 8 - misses * 3);
        break;
    }
    case KS_ROLL:
        if (held2) {
            if (hadPrev) {
                float dy = py - lastY; int nd = dy > 2 ? 1 : dy < -2 ? -1 : 0;
                if (nd != 0) { if (nd != dir && travel > 45) { count++; travel = 0; sfx(400, 600, 30); } if (nd != dir) travel = 0; dir = nd; travel += std::fabs(dy); }
            }
            lastY = py; hadPrev = true;
        } else { if (travel > 45) { count++; } travel = 0; hadPrev = false; }
        if (count >= 6) finishStep(100 - (int)(std::max(0.f, pt / limit - 0.6f) * 40));
        else if (timeUp) finishStep(count * 100 / 6 - 5);
        break;
    case KS_OVEN:
        meter += in.dt * 0.2f;
        if ((in.touch && py > 150 && py < 200 && px > 100 && px < 220) || meter > 1.05f) {
            finishStep(meter > 1.05f ? 5 : meter >= 0.6f && meter <= 0.78f ? 100 : meter >= 0.45f && meter <= 0.9f ? 70 : 25);
        } else if (timeUp) finishStep(20);
        break;
    }
}

static void eatAll() {
    int n = 0, good = 0, stars = 0, burnt = 0;
    bool has[F_COUNT] = {};
    for (int i = 0; i < 4; i++) if (plates[i] >= 0) { n++; has[plates[i]] = true; if (plateSt[i] == FS_DONE) { good++; stars += plateStars[i]; } else burnt++; }
    if (contentCooking() && n) {   // cooking a content-pack recipe: the plates become one real dish in the fridge
        contentCookDone(n, good, stars, burnt);
        for (int i = 0; i < 4; i++) plates[i] = -1;
        say2("Dish packed into the fridge! Find it in your Bag.");
        return;
    }
    if (!n && cupDrink < 0) { say2("Nothing on the table."); return; }
    float per = 22.f + stars * 2.f;
    // family dinner: everyone who lives here (and anyone visiting the kitchen) eats
    // call everyone who's home (and awake) to the table
    if (roomIsHome(G.room))
        for (int rm = 0; rm < R_COUNT; rm++) {
            if (rm == G.room || !roomIsHome(rm)) continue;
            auto& v = G.rooms[G.house][rm].actors;
            for (size_t k = 0; k < v.size();) {
                if (v[k].kind == 0 && v[k].id < G.sims.size() && G.sims[v[k].id].household && !v[k].sleeping && G.sims[v[k].id].stage >= ST_TODDLER) {
                    Actor a = v[k]; v.erase(v.begin() + k);
                    int seat = 0; for (const Actor& o : curRoom().actors) if (o.kind == 0) seat++;
                    a.x = 50 + (seat % 7) * 48.f; a.tx = a.x; a.y = 214; a.wait = 2;
                    curRoom().actors.push_back(a);
                } else k++;
            }
        }
    std::vector<int> eaters;
    for (Actor& a : curRoom().actors) if (a.kind == 0 && a.id < G.sims.size() && G.sims[a.id].stage >= ST_TODDLER && std::find(eaters.begin(), eaters.end(), (int)a.id) == eaters.end()) eaters.push_back(a.id);
    if (std::find(eaters.begin(), eaters.end(), G.mc) == eaters.end()) eaters.push_back(G.mc);
    static const char* YUM[] = {"This is SO good!", "Can I have seconds?", "You should open a restaurant!", "Best dinner ever!", "Mmm, yummy!"};
    static const char* MEH[] = {"It's... crunchy.", "Did something burn?", "I'll just have the drink.", "Interesting flavor..."};
    for (int id : eaters) {
        Sim& s = G.sims[id];
        s.needs[N_HUNGER] = std::fmin(100.f, s.needs[N_HUNGER] + per * n + (cupDrink >= 0 ? 8 : 0));
        s.needs[N_SOCIAL] = std::fmin(100.f, s.needs[N_SOCIAL] + 10);
        if (s.favFood < F_COUNT && has[s.favFood] && good) { addMoodlet(s, "My Favorite Food!", EM_HAPPY, 2, 300); if (id != G.mc) changeRel(G.mc, id, 3, 0); }
        if (id == G.mc) continue;
        if (good) { addMoodlet(s, stars >= good * 3 ? "Gold Medal Dinner" : "Family Dinner", EM_HAPPY, stars >= good * 2 ? 2 : 1, 240); changeRel(G.mc, id, stars >= good * 2 ? 4 : 2, 0); if (stars >= good * 3) addSentiment(id, SN_GRATEFUL, 3); }
        else addMoodlet(s, "Kitchen Disaster", EM_TENSE, 1, 90);
        for (Actor& a : curRoom().actors) if (a.kind == 0 && a.id == id) { a.say = good && !burnt ? YUM[(id + G.day) % 5] : MEH[(id + G.day) % 4]; a.sayT = 3.f; }
    }
    Sim& me = MC();
    me.needs[N_HUNGER] = std::fmin(100.f, me.needs[N_HUNGER] + 10);
    std::string combo;
    for (int r = 0; r < NUM_RECIPES && burnt == 0; r++) {
        const Recipe& R = RECIPES[r];
        if ((R.a < 0 || has[R.a]) && (R.b < 0 || has[R.b]) && (R.c < 0 || has[R.c]) && (R.drink < 0 || R.drink == cupDrink)) {
            combo = R.name;
            bool fresh = !(G.recipes & (1u << r));
            G.recipes |= 1u << r;
            skillGain(me, SK_COOKING, fresh ? 8.f : 3.f);
            addMoodlet(me, fresh ? "New Recipe!" : "Signature Dish", EM_INSPIRED, 2, 240);
            if (fresh) { G.satisfaction += 150; fxBurst(200, 120, 20); }
            break;
        }
    }
    int avg = good ? (stars + good / 2) / good : 0;
    if (good > 0) addMoodlet(me, avg >= 3 ? "Gold Medal Meal" : good >= 2 ? "Feast!" : "Yummy Meal", EM_HAPPY, avg >= 3 || good >= 2 ? 2 : 1, 180);
    if (burnt) addMoodlet(me, "Ate a Kitchen Disaster", EM_TENSE, 1, 90);
    if (G.date.active) G.date.score += 10 * good + 5 * stars + (combo.empty() ? 0 : 15);
    for (int i = 0; i < 4; i++) plates[i] = -1;
    cupDrink = -1;
    if (!combo.empty()) say2("Recipe combo: " + combo + "! Everyone's impressed.");
    else if (eaters.size() > 1) { char b[80]; snprintf(b, sizeof b, avg >= 3 ? "Gold medal family dinner! %d people ate." : "Family dinner! %d people ate together.", (int)eaters.size()); say2(b); }
    else say2(avg >= 3 ? "Gold medal meal! Chef Bea is SO proud." : "Yum! Everything's eaten.");
    if (eaters.size() > 1 && good) addMoodlet(me, "Fed the Family", EM_HAPPY, 1, 240);
    sfxYay();
}

// ---------------------------------------------------------------- hub input
static Btn recipeB(int i) { return Btn{4 + (i % 6) * 52.5f, 18 + (i / 6) * 26.f, 50, 24}; }
static Btn drinkB(int i) { return Btn{4 + (i % 3) * 105.f, 112 + (i / 3) * 22.f, 101, 20}; }
static const Btn bServe{4, 160, 76, 26}, bEat{82, 160, 76, 26}, bTrash{160, 160, 76, 26}, bBack{238, 160, 78, 26};
static const Btn bTip{220, 210, 96, 26}, bTake{110, 160, 100, 34};

bool cookUpdate(const Input& in) {
    if (msgT > 0) msgT -= in.dt;
    if (cupDrink >= 0 && cupFill < 1) cupFill = std::fmin(1.f, cupFill + in.dt * 0.8f);
    if (phase != PH_HUB) {
        if (in.down & KEY_B) { phase = PH_HUB; say2("You stopped cooking. Chef Bea saved the leftovers... in the trash."); if (tut == T_PLAY) tut = T_PICK; return true; }
        switch (phase) {
        case PH_INTRO: pt += in.dt; if (pt > 1.6f || (in.touch && pt > 0.3f)) beginStep(); break;
        case PH_STEP: stepUpdate(in); break;
        case PH_FEEDBACK:
            pt += in.dt;
            if (pt > 1.1f) { if (++stepI >= STEPS[dish].n) { phase = PH_RESULT; pt = 0; lastMedal = medalFor(dishAvg()); if (lastMedal >= 2) fxBurst(200, 100, lastMedal * 12); lastMedal ? sfxYay() : sfxNo(); } else beginStep(); }
            break;
        case PH_RESULT:
            pt += in.dt;
            if (pt > 0.8f && (in.touch || (in.down & KEY_A))) {
                held = Held(); held.food = dish; held.st = lastMedal ? FS_DONE : FS_BURNT; held.stars = lastMedal;
                skillGain(MC(), SK_COOKING, 1.f + lastMedal * 1.5f);
                phase = PH_HUB;
                if (tut == T_PLAY) tut = T_SERVE;
                say2(lastMedal ? "Your dish is ready! Serve it." : "Hmm... trash it and try again?");
            }
            break;
        }
        return true;
    }
    if (in.down & KEY_B) return false;
    if (!in.touch) return true;
    const touchPosition& tp = in.tp;
    if (tut >= 0 && bTip.in(tp)) { tut = -1; G.cookTutDone = true; say2("Class skipped! Replay it with Cooking Class."); sfxTap(); return true; }
    if (tut < 0 && bTip.in(tp)) { tut = T_PICK; say2("Cooking class with Chef Bea!"); sfxTap(); return true; }
    for (int i = 0; i < F_COUNT; i++)
        if (recipeB(i).in(tp)) {
            if (!unlocked(i)) { char b[64]; snprintf(b, sizeof b, "%s unlocks at Cooking skill %d!", FOOD_NAMES[i], UNLOCK[i]); say2(b); sfxNo(); }
            else if (held.food >= 0) { say2("Serve or trash what you're holding first!"); sfxNo(); }
            else if (tut == T_PICK && i != F_PANCAKES) { say2("Chef Bea says: let's start with Pancakes!"); sfxNo(); }
            else startDish(i);
            return true;
        }
    for (int i = 0; i < NUM_DRINKS; i++)
        if (drinkB(i).in(tp)) { cupDrink = i; cupFill = 0; say2(std::string("Pouring ") + DRINK_NAMES[i] + "!"); sfxTap(); return true; }
    if (bServe.in(tp)) {
        if (held.food < 0) { say2("Cook something first!"); return true; }
        for (int i = 0; i < 4; i++)
            if (plates[i] < 0) {
                int stars = held.stars;
                if (held.st == FS_DONE && G.groceries > 0) { stars = std::min(3, stars + economyGroceryStars()); G.groceries--; }
                plates[i] = held.food; plateSt[i] = held.st; plateStars[i] = held.st == FS_DONE ? std::max(1, stars) : 0; held = Held();
                G.dishes++;
                goalNotify(GE_COOK);
                skillGain(MC(), SK_COOKING, plateSt[i] == FS_DONE ? 2.f + plateStars[i] : 0.5f);
                char b[64]; snprintf(b, sizeof b, plateSt[i] == FS_DONE ? "Served! %d star%s. Tap Eat when ready." : "Served... it's a disaster.", plateStars[i], plateStars[i] == 1 ? "" : "s");
                say2(b); if (tut == T_SERVE) tut = T_EAT;
                sfxYay();
                return true;
            }
        say2("Table's full! Eat first.");
        return true;
    }
    if (bEat.in(tp)) {
        bool any = cupDrink >= 0; for (int i = 0; i < 4; i++) if (plates[i] >= 0) any = true;
        eatAll();
        if (any && tut == T_EAT) { tut = -1; G.cookTutDone = true; G.money += 50; addMoodlet(MC(), "Cooking Class Grad", EM_CONFIDENT, 2, 300); say2("Class complete! +$50. Find all 9 recipe combos!"); }
        return true;
    }
    if (bTrash.in(tp)) { held = Held(); say2("Tossed it."); return true; }
    if (bBack.in(tp)) return false;
    return true;
}

// ---------------------------------------------------------------- Chef Bea (original guide character)
static void drawBea(float x, float y, float s, int mood, float t) {   // mood 0 talk, 1 cheer, 2 worried; (x,y) = bottom centre
    auto X = [&](float v) { return x + v * s; }; auto Y = [&](float v) { return y - v * s; };
    const u32 skin = C(255, 224, 200), hair = C(250, 150, 190), white = C(255, 255, 255), ink = C(70, 40, 60), pink = C(255, 120, 170);
    float bob = std::sin(t * (mood == 1 ? 9 : 3)) * (mood == 1 ? 3 : 1.2f) * s;
    // body: chef coat + apron
    rrect(X(-26), Y(56) + bob, 52 * s, 56 * s, 10 * s, white);
    rrect(X(-18), Y(44) + bob, 36 * s, 44 * s, 6 * s, pink);
    heart(X(0), Y(30) + bob, 5 * s, white);
    for (int i = 0; i < 2; i++) circle(X(-6 + i * 12), Y(50) + bob, 1.6f * s, C(230, 200, 210));
    // arms (up when cheering)
    for (int sd = -1; sd <= 1; sd += 2) {
        if (mood == 1) { line(X(sd * 24), Y(50) + bob, X(sd * 36), Y(76) + bob, 7 * s, white); circle(X(sd * 37), Y(79) + bob, 4.5f * s, skin); }
        else { line(X(sd * 24), Y(50) + bob, X(sd * 30), Y(24) + bob, 7 * s, white); circle(X(sd * 30), Y(21) + bob, 4.5f * s, skin); }
    }
    float hy = 82;
    // bunny ears + chef hat
    for (int sd = -1; sd <= 1; sd += 2) { ellipse(X(sd * 12), Y(hy + 40) + bob, 13 * s, 34 * s, white); ellipse(X(sd * 12), Y(hy + 40) + bob, 6 * s, 24 * s, C(255, 190, 210)); }
    rrect(X(-17), Y(hy + 30) + bob, 34 * s, 16 * s, 4 * s, white);
    for (int i = -1; i <= 1; i++) circle(X(i * 11), Y(hy + 32) + bob, 9 * s, white);
    rect(X(-17), Y(hy + 17) + bob, 34 * s, 4 * s, pink);
    // head
    circle(X(0), Y(hy) + bob, 22 * s, hair);
    circle(X(0), Y(hy - 2) + bob, 18 * s, skin);
    for (int i = -2; i <= 2; i++) circle(X(i * 7), Y(hy + 13) + bob, 6 * s, hair);   // bangs
    // face
    if (mood == 1) { for (int sd = -1; sd <= 1; sd += 2) { line(X(sd * 7 - 3), Y(hy - 3) + bob, X(sd * 7), Y(hy) + bob, 1.8f * s, ink); line(X(sd * 7), Y(hy) + bob, X(sd * 7 + 3), Y(hy - 3) + bob, 1.8f * s, ink); } }
    else for (int sd = -1; sd <= 1; sd += 2) { ellipse(X(sd * 7), Y(hy - 2) + bob, 5 * s, 7 * s, ink); circle(X(sd * 7 + 1), Y(hy) + bob, 1.2f * s, white); }
    if (mood == 2) { line(X(-11), Y(hy + 6) + bob, X(-4), Y(hy + 4) + bob, 1.4f * s, ink); line(X(11), Y(hy + 6) + bob, X(4), Y(hy + 4) + bob, 1.4f * s, ink); ellipse(X(0), Y(hy - 11) + bob, 6 * s, 4 * s, ink); }
    else { ellipse(X(0), Y(hy - 10) + bob, (mood == 1 ? 9 : 6) * s, (mood == 1 ? 6 : 3.5f) * s, C(200, 60, 100)); }
    for (int sd = -1; sd <= 1; sd += 2) ellipse(X(sd * 12), Y(hy - 7) + bob, 6 * s, 3 * s, C(255, 150, 170, 170));
}
static void bubble(float x, float y, float w, const std::string& s, u32 col = C(UI1)) {
    std::vector<std::string> ln = wrapText(s, 0.42f, w - 16);
    float h = 10 + ln.size() * 15.f;
    rrect(x - 2, y - 2, w + 4, h + 4, 10, C(UI1)); rrect(x, y, w, h, 9, C(255, 250, 252));
    tri(x - 12, y + h - 14, x + 2, y + h - 22, x + 2, y + h - 8, C(255, 250, 252));
    for (size_t i = 0; i < ln.size(); i++) text(ln[i].c_str(), x + 8, y + 6 + i * 15.f, 0.42f, col);
}
static void medalIcon(float x, float y, float r, int m) {
    static const RGB MC_[4] = {{150, 150, 160}, {205, 127, 50}, {200, 205, 215}, {255, 205, 60}};
    line(x - r * 0.6f, y - r * 2.2f, x, y - r * 0.6f, r * 0.5f, C(255, 90, 150)); line(x + r * 0.6f, y - r * 2.2f, x, y - r * 0.6f, r * 0.5f, C(120, 180, 255));
    circle(x, y, r + 2, C(UI1)); circle(x, y, r, C(MC_[m])); sparkle(x, y, r * 0.5f, C(255, 255, 255, 220));
}
static void starRow(float x, float y, int n, float sz) { for (int i = 0; i < 3; i++) sparkle(x + (i - 1) * sz * 2.2f, y, sz, i < n ? C(GOLD) : C(255, 255, 255, 70)); }
static const char* MEDAL_NAME[4] = {"Oops!", "Bronze", "Silver", "GOLD"};

// ---------------------------------------------------------------- drawing: top
void cookDrawTop(float t) {
    grad(0, 0, TOP_W, 170, C(255, 236, 244), C(255, 214, 230));
    for (int r = 0; r < 6; r++) for (int k = 0; k < 25; k++) rrect(k * 16.f + (r % 2) * 8, 60 + r * 12.f, 14, 10, 2, C(255, 255, 255, 70));
    rect(0, 170, TOP_W, 70, C(120, 80, 60));
    if (phase != PH_HUB && dish >= 0) {   // cooking with Chef Bea
        const Step& st = STEPS[dish].s[std::min<int>(stepI, STEPS[dish].n - 1)];
        int mood = phase == PH_RESULT ? (lastMedal >= 2 ? 1 : lastMedal == 0 ? 2 : 0) : phase == PH_FEEDBACK ? (curScore >= 85 ? 1 : curScore < 40 ? 2 : 0) : 0;
        drawBea(80, 236, 1.25f, mood, t);
        char hd[64]; snprintf(hd, sizeof hd, "%s  -  Step %d/%d", FOOD_NAMES[dish], std::min(stepI + 1, (int)STEPS[dish].n), STEPS[dish].n);
        textOutline(hd, 260, 6, 0.55f, C(255, 255, 255), C(UI1), 1, 1.2f);
        std::string line;
        if (phase == PH_INTRO) line = std::string("Let's make ") + FOOD_NAMES[dish] + "! Ready? Here we go!";
        else if (phase == PH_STEP) line = std::string(STEP_VERB[st.type]) + " " + st.what + "! " + STEP_HOW[st.type];
        else if (phase == PH_FEEDBACK) line = curScore >= 85 ? "PERFECT! You're amazing!" : curScore >= 65 ? "Great job! Keep going!" : curScore >= 40 ? "Not bad! Next step!" : "Oh no! It's okay - keep trying!";
        else line = lastMedal == 3 ? "WOW! Even better than Chef Bea!! GOLD!" : lastMedal == 2 ? "So yummy! That's a Silver medal!" : lastMedal == 1 ? "Good effort! Bronze for you!" : "Don't cry! We'll fix it together next time!";
        bubble(160, 34, 228, line);
        for (int i = 0; i < STEPS[dish].n; i++) {   // step pips
            float px2 = 190 + i * 30.f;
            rrect(px2, 130, 26, 14, 6, C(UI1));
            u32 c = i < stepI || (i == stepI && phase >= PH_FEEDBACK) ? (scores[i] >= 85 ? C(GOLD) : scores[i] >= 65 ? C(200, 205, 215) : scores[i] >= 40 ? C(205, 127, 50) : C(150, 150, 160)) : i == stepI ? C(HOT) : C(255, 255, 255, 90);
            rrect(px2 + 2, 132, 22, 10, 5, c);
            text(STEP_VERB[STEPS[dish].s[i].type], px2 + 13, 146, 0.3f, C(UI1), 1);
        }
        if (phase == PH_STEP) { float k = 1 - pt / limit; rrect(190, 166, 190, 12, 5, C(UI1)); rrect(192, 168, 186 * std::max(0.f, k), 8, 4, C(k > 0.3f ? RGB{120, 220, 150} : RGB{255, 90, 90})); }
        if (phase == PH_RESULT) { medalIcon(300, 200, 22, lastMedal); textOutline(MEDAL_NAME[lastMedal], 300, 222, 0.55f, C(255, 255, 255), C(UI1), 1); drawFood(dish, lastMedal ? FS_DONE : FS_BURNT, 220, 214, 1.4f, t); }
        return;
    }
    // the kitchen hub: Chef Bea at her counter
    drawBea(60, 206, 0.95f, 0, t);
    rect(112, 120, 280, 60, C(CREAM)); rect(112, 116, 280, 6, C(232, 224, 222));
    rect(150, 110, 100, 8, C(BLACK)); circle(176, 114, 9, C(70, 70, 70)); circle(224, 114, 9, C(70, 70, 70));
    rect(150, 124, 100, 54, C(BLACK)); rect(158, 132, 84, 36, C(60, 30, 26)); rect(160, 126, 80, 3, C(GOLD));
    rect(300, 112, 60, 8, C(200, 210, 220)); rect(326, 92, 4, 20, C(GOLD)); rect(326, 92, 14, 4, C(GOLD));
    drawDrink(cupDrink, 280, 116, 1.f, cupFill);
    rect(0, 212, TOP_W, 28, C(WOOD)); rect(0, 210, TOP_W, 4, C(shade(WOOD, 1.2f)));
    for (int i = 0; i < 4; i++) {
        float px = 60 + i * 90;
        ellipse(px, 220, 54, 12, C(WHITE)); ellipse(px, 220, 40, 8, C(BLUSH));
        if (plates[i] >= 0) { drawFood(plates[i], plateSt[i], px, 222, 1.f, t); if (plateSt[i] == FS_DONE) starRow(px, 234, plateStars[i], 3.f); }
    }
    if (held.food >= 0) {
        rrect(300, 8, 94, 62, 8, C(UI1, 220));
        text("Ready:", 347, 10, 0.4f, C(CREAM), 1);
        drawFood(held.food, held.st, 347, 52, 0.9f, t);
        if (held.st == FS_DONE) starRow(347, 62, held.stars, 3.5f);
    }
    textShadow("Dishes:", 6, 4, 0.42f, C(UI1), C(255, 255, 255, 160));
    textShadow(std::to_string(G.dishes).c_str(), 56, 4, 0.42f, C(HOT), C(255, 255, 255, 160));
    { int n = 0; for (int r = 0; r < NUM_RECIPES; r++) if (G.recipes & (1u << r)) n++; char b[40]; snprintf(b, sizeof b, "Combos: %d/%d   Fresh: %d", n, NUM_RECIPES, G.groceries); textShadow(b, 6, 18, 0.38f, C(UI1), C(255, 255, 255, 160)); }
    if (tut >= 0 && tut < T_DONE) bubble(110, 40, 180, TUT_TEXT[tut], C(HOT));
    else if (msgT > 0) bubble(110, 40, 180, msg);
}

// ---------------------------------------------------------------- drawing: bottom (the minigames)
static void stepDraw(float t) {
    const Step& st = curStep();
    u32 ink = C(UI1), col = C(st.col), dark = C(shade(st.col, 0.75f));
    char tl[48]; snprintf(tl, sizeof tl, "%s %s!", STEP_VERB[st.type], st.what);
    textOutline(tl, 160, 4, 0.55f, C(255, 255, 255), ink, 1, 1.2f);
    switch (st.type) {
    case KS_CHOP: {
        rrect(30, 200, 260, 26, 6, C(214, 170, 120));   // board
        rrect(40, 90, 240, 70, 26, col); rrect(40, 90, 240, 18, 12, C(255, 255, 255, 60));
        for (int i = 0; i < 5; i++) { float lx = 40 + (i + 1) * 240.f / 6; if (cut[i]) rect(lx - 1.5f, 84, 3, 82, C(255, 255, 255)); else for (int k = 0; k < 7; k++) rect(lx - 1, 86 + k * 11.f, 2, 6, ink); }
        char b[32]; snprintf(b, sizeof b, "Slices: %d/5", count); text(b, 160, 176, 0.45f, C(CREAM), 1);
        break;
    }
    case KS_CRACK: {
        ellipse(160, 180, 150, 40, C(240, 240, 250)); ellipse(160, 176, 130, 28, C(255, 255, 255));
        for (int i = 0; i < 3; i++) { float ex = 90 + i * 70.f; if (i < count) { ellipse(ex, 176, 22, 14, C(255, 255, 250)); circle(ex, 176, 6, C(255, 200, 60)); } else { ellipse(ex, 110, 26, 32, C(250, 240, 220)); ellipse(ex - 4, 102, 8, 10, C(255, 255, 255, 140)); } }
        rrect(40, 50, 240, 16, 6, ink); rect(42 + 236 * 0.62f, 52, 236 * 0.24f, 12, C(GOLD)); rect(42 + 236 * 0.45f, 52, 236 * 0.17f, 12, C(255, 230, 150, 120)); rect(42 + 236 * 0.86f, 52, 236 * 0.09f, 12, C(255, 230, 150, 120));
        rect(42 + 236 * meter - 2, 46, 4, 24, C(HOT));
        break;
    }
    case KS_STIR: {
        circle(160, 135, 96, C(UI1)); circle(160, 135, 92, C(250, 250, 255)); circle(160, 135, 78, col);
        float sw = acc * 0.6f; for (int i = 0; i < 3; i++) swirl(160, 135, 20 + i * 18, 1.2f, 2, dark, false);
        (void)sw;
        float k = std::min(1.f, acc / (4 * 6.28318f)); rrect(20, 222, 280, 10, 4, ink); rrect(22, 224, 276 * k, 6, 3, C(GOLD));
        text("Stir power", 160, 206, 0.4f, C(CREAM), 1);
        break;
    }
    case KS_POUR: {
        rrect(110, 60, 100, 150, 10, C(UI1)); rrect(114, 64, 92, 142, 8, C(240, 246, 255));
        float h = 138 * std::min(1.f, level); rect(116, 204 - h, 88, h, col);
        float ly = 204 - 138 * 0.75f; rect(100, ly - 1, 120, 3, C(HOT)); text("FILL LINE", 226, ly - 7, 0.38f, C(HOT));
        if (pouring && level < 1) { rect(156, 30, 8, 204 - h - 30, col); }
        break;
    }
    case KS_FLIP: {
        ellipse(160, 150, 200, 60, C(60, 60, 66)); ellipse(160, 146, 180, 48, C(90, 90, 96)); rect(250, 142, 60, 10, C(60, 60, 66));
        float jump = 0; drawFood(dish, meter > 0.88f ? FS_BURNT : meter > 0.4f ? FS_DONE : FS_RAW, 160, 150 - jump, 2.0f, t);
        rrect(40, 40, 240, 16, 6, ink); rect(42 + 236 * 0.55f, 42, 236 * 0.2f, 12, C(GOLD)); rect(42 + 236 * 0.88f, 42, 236 * 0.12f, 12, C(255, 80, 80));
        rect(42 + 236 * std::min(1.f, meter) - 2, 36, 4, 24, C(HOT));
        char b[24]; snprintf(b, sizeof b, "Flips: %d/2", count); text(b, 160, 210, 0.45f, C(CREAM), 1);
        break;
    }
    case KS_SPRINKLE: {
        ellipse(160, 140, 240, 140, C(255, 255, 255)); ellipse(160, 140, 200, 110, C(shade(st.col, 1.15f)));
        for (int i = 0; i < 8; i++) if (thit[i]) for (int k = 0; k < 4; k++) circle(tx[i] + (k - 1.5f) * 4, ty[i] + (k % 2) * 3, 2.5f, col);
        int idx = (int)(pt / 1.3f);
        if (idx < 8 && !thit[idx]) { float life = std::fmod(pt, 1.3f) / 1.3f; circle(tx[idx], ty[idx], 18 - life * 6, C(255, 240, 120, 200)); sparkle(tx[idx], ty[idx], 10, C(255, 255, 255)); }
        char b[24]; snprintf(b, sizeof b, "%d/8", count); text(b, 290, 214, 0.45f, C(CREAM), 1);
        break;
    }
    case KS_ROLL: {
        rrect(20, 196, 280, 30, 6, C(214, 170, 120));
        float w = 90 + count * 25.f; ellipse(160, 130, w, 100 - count * 5.f, col);
        float ry = 80 + (dir > 0 ? 30 : 0) + std::sin(t * 4) * 4; rrect(60, ry, 200, 16, 8, C(200, 150, 100)); rect(40, ry + 5, 20, 6, C(150, 100, 70)); rect(260, ry + 5, 20, 6, C(150, 100, 70));
        char b[24]; snprintf(b, sizeof b, "Rolls: %d/6", count); text(b, 160, 214, 0.45f, C(CREAM), 1);
        break;
    }
    case KS_OVEN: {
        rrect(60, 34, 200, 116, 10, C(UI1)); rrect(70, 44, 180, 80, 6, C(60, 30, 26)); rrect(70, 44, 180, 80, 6, C(255, 140, 60, (uint8_t)(60 + 120 * meter)));
        drawFood(dish, meter > 0.9f ? FS_BURNT : meter > 0.45f ? FS_DONE : FS_RAW, 160, 116, 1.6f, t);
        rrect(40, 132, 240, 14, 6, ink); rect(42 + 236 * 0.6f, 134, 236 * 0.18f, 10, C(GOLD)); rect(42 + 236 * 0.9f, 134, 236 * 0.1f, 10, C(255, 80, 80));
        rect(42 + 236 * std::min(1.f, meter) - 2, 128, 4, 22, C(HOT));
        button(bTake, "TAKE OUT!", meter >= 0.6f && meter <= 0.78f, 2, 0.55f);
        break;
    }
    }
    text("B: give up", 6, 226, 0.34f, C(CREAM, 180));
}

void cookDrawBottom(float t) {
    bottomBackdrop(t);
    if (phase == PH_STEP) { stepDraw(t); return; }
    if (phase != PH_HUB) {
        if (phase == PH_INTRO) { drawFood(dish, FS_DONE, 160, 150, 3.f, t); textOutline("Tap to start!", 160, 190, 0.6f, C(255, 255, 255), C(UI1), 1); }
        else if (phase == PH_FEEDBACK) { stepDraw(t); rrect(70, 90, 180, 50, 12, C(UI1, 230)); textOutline(curScore >= 85 ? "PERFECT!" : curScore >= 65 ? "GREAT!" : curScore >= 40 ? "OKAY!" : "OOPS!", 160, 98, 0.8f, C(curScore >= 85 ? GOLD : CREAM), C(UI1), 1); }
        else { drawFood(dish, lastMedal ? FS_DONE : FS_BURNT, 160, 140, 3.f, t); medalIcon(260, 70, 26, lastMedal); textOutline(MEDAL_NAME[lastMedal], 160, 170, 0.8f, C(255, 255, 255), C(UI1), 1); text("Tap to continue", 160, 206, 0.45f, C(CREAM), 1); }
        return;
    }
    text(tut == T_PICK ? "Recipe Book - pick Pancakes!" : "Recipe Book", 160, 3, 0.45f, C(CHAMP), 1);
    for (int i = 0; i < F_COUNT; i++) {
        Btn b = recipeB(i);
        if (!unlocked(i)) { rrect(b.x, b.y, b.w, b.h, 6, C(UI2, 200)); char lb[24]; snprintf(lb, sizeof lb, "Lv %d", UNLOCK[i]); text(lb, b.x + b.w / 2, b.y + 6, 0.36f, C(CREAM, 180), 1); }
        else button(b, FOOD_NAMES[i], held.food == i, 2, 0.26f);
    }
    text("Drinks", 6, 100, 0.36f, C(CHAMP));
    for (int i = 0; i < NUM_DRINKS; i++) button(drinkB(i), DRINK_NAMES[i], cupDrink == i, 0, 0.36f);
    button(bServe, "Serve", false, 0, 0.45f);
    button(bEat, "Family Dinner!", false, 2, 0.34f);
    button(bTrash, "Trash", false, 1, 0.45f);
    button(bBack, "Done", false, 1, 0.45f);
    auto glow = [&](const Btn& b) { float a = 0.5f + 0.5f * std::sin(t * 8); outline(b.x - 3, b.y - 3, b.w + 6, b.h + 6, 3, C(255, 240, 120, (uint8_t)(120 + 120 * a))); };
    if (tut == T_PICK) glow(recipeB(F_PANCAKES)); else if (tut == T_SERVE) glow(bServe); else if (tut == T_EAT) glow(bEat);
    button(bTip, tut >= 0 ? "Skip Class" : "Cooking Class", false, 3, 0.38f);
    text(tut >= 0 ? "Follow Chef Bea!" : "Gold medal + fresh groceries = 3 stars", 6, 196, 0.36f, C(CREAM), 0);
}
