#include "shopgames.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
using namespace col;

namespace {
int kind = 0, round_ = 0, rounds = 5, score = 0, phase = 0, answer = 0, cash = 0;
float timer = 0, rivalT = 0, fb = 0; bool fbGood = false; std::string fbText;
std::vector<std::string> opts;
std::string prompt, sub;
// grocery
struct GItem { const char* name; int price; bool onList; };
std::vector<GItem> shelf; std::vector<bool> inCart; int budget = 40;

struct Book { const char* genre; const char* desc; const char* title; };
const Book BOOKS[] = {
    {"Enemies to Lovers", "\"They HATE each other. Then they share one bed at a fae court.\"", "A Court of Thorns & Side-Eye"},
    {"Dark Romance", "\"Morally gray guy, spooky mansion, she's way too into it.\"", "Haunting Adeline's Group Chat"},
    {"Sad Contemporary", "\"I want to sob in public. Toxic love, tiny tattoo shop.\"", "It Ends With Us Crying"},
    {"Rom-Com", "\"Fake dating a grumpy scientist. Light, funny, beachy.\"", "The Love Hypothesis Test"},
    {"Romantasy", "\"Dragons, war college, and a bad boy wingleader.\"", "Fourth Wing Night"},
    {"Thriller", "\"A silent patient, a twist I won't see coming.\"", "The Quiet Patient Next Door"},
    {"Historical Drama", "\"Old Hollywood star, seven husbands, juicy secrets.\"", "Seven Exes of Evelyn Glow"},
    {"Cozy Fantasy", "\"An orc retires to open a coffee shop. Low stakes, warm vibes.\"", "Lattes & Legends"},
    {"Sports Romance", "\"Hockey captain falls for the figure skater. Spicy.\"", "Icebreaker Kiss"},
    {"YA Mystery", "\"Small-town girl solves a cold case for a school project.\"", "A Good Girl's Group Project"},
};
const int NB = sizeof(BOOKS) / sizeof(BOOKS[0]);

struct Thrift { const char* name; RGB c; };
const Thrift TH[] = {{"Y2K Baby Tee", {255, 150, 200}}, {"Vintage Levi's", {80, 120, 190}}, {"Leather Jacket", {50, 40, 40}}, {"Slip Dress", {240, 220, 180}},
                     {"Band Tee", {30, 30, 30}}, {"Cardigan", {200, 120, 80}}, {"Mini Skirt", {220, 60, 90}}, {"Platform Boots", {120, 70, 160}}, {"Corset Top", {250, 250, 250}}};
int grid[6]; int target = 0;

const char* SNACKS[] = {"Hot Chips", "Soda", "Gum", "Energy Drink", "Candy Bar", "Ramen Cup", "Iced Tea", "Scratch-Off"};
const int SNP[] = {3, 2, 1, 4, 2, 2, 3, 5};
int custTotal = 0, custPaid = 0;

Btn optB(int i) { return Btn{10.f + (i % 2) * 152, 70.f + (i / 2) * 64, 148, 58}; }
Btn cellB(int i) { return Btn{8.f + (i % 3) * 102, 40.f + (i / 3) * 90, 98, 84}; }
Btn shelfB(int i) { return Btn{6.f + (i % 2) * 156, 30.f + (i / 2) * 34, 152, 30}; }
const Btn DONE{214, 204, 100, 30};

void newRound() {
    opts.clear(); fb = 0;
    if (kind == 1) {
        int b = irand(NB); answer = irand(4);
        std::vector<int> pick{b};
        while ((int)pick.size() < 4) { int x = irand(NB); if (std::find(pick.begin(), pick.end(), x) == pick.end()) pick.push_back(x); }
        std::swap(pick[0], pick[answer]);
        for (int x : pick) opts.push_back(BOOKS[x].title);
        prompt = std::string("Patron wants: ") + BOOKS[b].genre; sub = BOOKS[b].desc;
    } else if (kind == 2) {
        for (int i = 0; i < 6; i++) grid[i] = irand(9);
        target = grid[irand(6)];
        rivalT = 2.2f - round_ * 0.2f; timer = 0;
        prompt = std::string("GRAB IT: ") + TH[target].name; sub = "Before that other shopper does!";
    } else if (kind == 3) {
        int n = 1 + irand(3); custTotal = 0; sub = "";
        for (int i = 0; i < n; i++) { int k = irand(8); custTotal += SNP[k]; sub += std::string(i ? ", " : "") + SNACKS[k]; }
        custPaid = custTotal <= 5 ? 5 : custTotal <= 10 ? 10 : 20;
        if (custPaid == custTotal) custPaid += 5;
        int ch = custPaid - custTotal; answer = irand(4);
        int wrong[3] = {ch + 1, std::max(0, ch - 1) == ch ? ch + 2 : ch - 1, ch + 3}; int w = 0;
        for (int i = 0; i < 4; i++) { char b[16]; snprintf(b, sizeof b, "$%d", i == answer ? ch : wrong[w++]); opts.push_back(b); }
        char b[64]; snprintf(b, sizeof b, "Total $%d. They hand you $%d.", custTotal, custPaid); prompt = b;
    }
}
void startGrocery() {
    static const GItem POOL[] = {{"Eggs", 4, true}, {"Milk", 4, true}, {"Bread", 3, true}, {"Rice", 5, true}, {"Chicken", 9, true}, {"Apples", 4, true},
                                 {"Fancy Eggs", 9, true}, {"Oat Milk", 7, true}, {"Name-Brand Cereal", 8, false}, {"Gummy Bears", 3, false}, {"Lobster", 25, false}, {"Cake", 12, false}};
    shelf.assign(POOL, POOL + 12);
    for (int i = (int)shelf.size() - 1; i > 0; i--) std::swap(shelf[i], shelf[irand(i + 1)]);   // shuffle
    inCart.assign(12, false); budget = 30;
    prompt = "List: Eggs, Milk, Bread, Rice, Chicken, Apples"; sub = "Budget $30. Fancy versions count but cost more!";
}
int cartTotal() { int t = 0; for (size_t i = 0; i < shelf.size(); i++) if (inCart[i]) t += shelf[i].price; return t; }
bool listDone() {
    const char* need[6] = {"Eggs", "Milk", "Bread", "Rice", "Chicken", "Apples"};
    for (auto n : need) { bool ok = false; for (size_t i = 0; i < shelf.size(); i++) if (inCart[i] && std::string(shelf[i].name).find(n) != std::string::npos) ok = true;
        if (!ok && std::string(n) == "Milk") for (size_t i = 0; i < shelf.size(); i++) if (inCart[i] && std::string(shelf[i].name) == "Oat Milk") ok = true;
        if (!ok) return false; }
    return true;
}
void finish() {
    phase = 2; Sim& me = G.sims[G.mc]; char b[128];
    if (kind == 0) {
        int tot = cartTotal(); bool ok = listDone() && tot <= budget;
        if (ok) { int saved = budget - tot; G.money += saved; addMoodlet(me, "Budget Queen", EM_CONFIDENT, 2, 240); skillGain(me, SK_LOGIC, 3);
            snprintf(b, sizeof b, "Got the whole list for $%d! Kept $%d change.", tot, saved); sfxYay(); }
        else { snprintf(b, sizeof b, listDone() ? "Over budget ($%d of $%d)! Card declined vibes." : "You forgot something on the list!", tot, budget); sfxNo(); }
        score = ok;
    } else if (kind == 1) { G.money += score * 6; skillGain(me, SK_LOGIC, score); snprintf(b, sizeof b, "%d/%d patrons matched! Paid $%d.", score, rounds, score * 6); if (score >= 3) addMoodlet(me, "Bookish Bliss", EM_HAPPY, 1, 200); }
    else if (kind == 2) { int got = 0; for (int i = 0; i < score; i++) { int k = 32 + irand(FIRST_KID_OUTFIT - 32); if (!ownsOutfit(k)) { giveOutfit(k); got++; } }
        snprintf(b, sizeof b, "You snagged %d of %d finds! %d new looks in your closet.", score, rounds, got); if (score) addMoodlet(me, "Thrift Score", EM_HAPPY, 2, 240); }
    else { G.money += score * 8; skillGain(me, SK_CHARISMA, score); snprintf(b, sizeof b, "%d/%d customers rung up right. Earned $%d.", score, rounds, score * 8); }
    fbText = b;
}
void answerPick(bool good) {
    if (good) { score++; sfxYay(); fbText = kind == 2 ? "Got it first!" : "Perfect!"; } else { sfxNo(); fbText = kind == 2 ? "She grabbed it first..." : "Oops, wrong one!"; if (kind != 2 && irand(3) == 0) addMoodlet(G.sims[G.mc], "Awkward Mistake", EM_EMBARRASSED, 1, 90); }
    fbGood = good; fb = 0.9f;
}
}

void shopStart(int k) { kind = k; round_ = 0; score = 0; phase = 0; rounds = k == 2 ? 6 : 5; if (k == 0) startGrocery(); else newRound(); }

bool shopUpdate(const Input& in) {
    bool tap = in.touch; const touchPosition& tp = in.tp;
    if (phase == 0) { if (tap || (in.down & KEY_A)) { phase = 1; sfxTap(); } return true; }
    if (phase == 2) { if (tap || (in.down & KEY_A)) return false; return true; }
    if (in.down & KEY_B) { finish(); return true; }
    if (fb > 0) { fb -= in.dt; if (fb <= 0) { if (++round_ >= rounds) finish(); else newRound(); } return true; }
    if (kind == 0) {
        if (tap) { for (int i = 0; i < 12; i++) if (shelfB(i).in(tp)) { inCart[i] = !inCart[i]; sfxTap(); }
            if (DONE.in(tp)) finish(); }
    } else if (kind == 2) {
        timer += in.dt;
        if (timer > rivalT) { answerPick(false); return true; }
        if (tap) for (int i = 0; i < 6; i++) if (cellB(i).in(tp)) { if (grid[i] == target) answerPick(true); else { sfxNo(); timer += 0.4f; } }
    } else if (tap) {
        for (int i = 0; i < 4; i++) if (optB(i).in(tp)) { answerPick(i == answer); break; }
    }
    return true;
}

static void textWrap(const char* s, float x, float y, float sc, u32 c, float w, int al) {
    auto L = wrapText(s, sc, w); for (size_t i = 0; i < L.size(); i++) text(L[i].c_str(), x, y + i * 30 * sc, sc, c, al);
}
static const char* TITLES[] = {"Budget Grocery Run", "Book Match", "Thrift Grab!", "Quik Stop Register"};
void shopDrawTop(float t) {
    grad(0, 0, TOP_W, TOP_H, C(255, 236, 246), C(236, 220, 255));
    textOutline(TITLES[kind], 200, 14, 0.75f, C(255, 255, 255), C(255, 70, 150), 1);
    if (phase == 0) {
        static const char* HOW[] = {"Tap items to put them in your cart. Get everything on the list without going over budget.",
                                    "Patrons describe what they're in the mood for. Hand them the title that fits the genre!",
                                    "The item to grab shows up here. Find it on the rack and tap it before the rival does!",
                                    "Ring up customers and give back the right change. Every correct one pays $8."};
        textWrap(HOW[kind], 200, 90, 0.5f, C(UI_INK), 340, 1);
        text("Tap or press A to start", 200, 200, 0.45f, C(255, 70, 150), 1);
        return;
    }
    if (phase == 2) { textWrap(fbText.c_str(), 200, 100, 0.55f, C(UI_INK), 340, 1); text("Tap to leave", 200, 200, 0.45f, C(255, 70, 150), 1); return; }
    textWrap(prompt.c_str(), 200, 64, 0.55f, C(UI_INK), 360, 1);
    textWrap(sub.c_str(), 200, 110, 0.45f, C(120, 80, 120), 360, 1);
    char b[48];
    if (kind == 0) snprintf(b, sizeof b, "Cart: $%d / $%d", cartTotal(), budget);
    else snprintf(b, sizeof b, "Round %d/%d   Score %d", round_ + 1, rounds, score);
    text(b, 200, 170, 0.5f, C(kind == 0 && cartTotal() > budget ? RGB{220, 40, 60} : UI_INK), 1);
    if (kind == 2) { float f = std::max(0.f, 1 - timer / rivalT); rrect(100, 200, 200, 10, 4, C(200, 200, 200)); rrect(100, 200, 200 * f, 10, 4, C(255, 80, 140)); }
    if (fb > 0) textOutline(fbText.c_str(), 200, 140, 0.6f, C(fbGood ? RGB{60, 200, 110} : RGB{240, 70, 90}), C(UI_INK), 1);
    (void)t;
}
void shopDrawBottom(float t) {
    rect(0, 0, BOT_W, BOT_H, C(255, 248, 252));
    if (phase != 1) { text(phase == 0 ? "Tap to start!" : "Tap to finish", 160, 110, 0.6f, C(UI_INK), 1); return; }
    if (kind == 0) {
        for (int i = 0; i < 12; i++) { char b[48]; snprintf(b, sizeof b, "%s $%d", shelf[i].name, shelf[i].price); button(shelfB(i), b, inCart[i], 0, 0.38f); }
        button(DONE, "Checkout", false, 2, 0.45f);
    } else if (kind == 2) {
        for (int i = 0; i < 6; i++) { Btn c = cellB(i); rrect(c.x, c.y, c.w, c.h, 6, C(UI_INK)); rrect(c.x + 2, c.y + 2, c.w - 4, c.h - 4, 5, C(255, 255, 255));
            rrect(c.x + 30, c.y + 10, 38, 44, 6, C(TH[grid[i]].c)); text(TH[grid[i]].name, c.x + c.w / 2, c.y + 62, 0.34f, C(UI_INK), 1); }
        float rx = 20 + std::min(1.f, timer / rivalT) * 260; circle(rx, 228, 8, C(200, 60, 120)); text("rival", rx, 214, 0.3f, C(UI_INK), 1);
    } else {
        text(kind == 1 ? "Pick the best title:" : "Their change:", 160, 40, 0.5f, C(UI_INK), 1);
        for (int i = 0; i < 4 && i < (int)opts.size(); i++) button(optB(i), opts[i].c_str(), false, 0, kind == 1 ? 0.36f : 0.6f);
    }
    text("B: quit", 300, 6, 0.32f, C(150, 120, 150), 2);
    (void)t;
}
