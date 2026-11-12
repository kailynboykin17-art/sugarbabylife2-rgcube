#include "deeplife.hpp"
#include "lifeext.hpp"
#include "hotdate.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "district.hpp"
#include "people.hpp"
#include "goals.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include "vfs.hpp"
#define fopen vfopen   // sidecar saves go into the one-file save pack

using namespace col;

enum { DS_NONE, DS_SEATED, DS_ORDERED, DS_SERVED, DS_CHECK };
struct HState {
    uint32_t magic = 0x48445431;
    uint8_t dine = DS_NONE, tier = 0, cartN = 0, tryingOn = 0;
    int16_t cart[6] = {-1, -1, -1, -1, -1, -1};   // >=0 outfit id, -2 magazine, -3 pet goods
    int16_t prevOutfit = -1;
    int32_t bill = 0;
};
static HState H;

enum {
    A_TAXIHOME, A_HOTDOG, A_ICECREAM, A_MAGAZINE,
    A_BROWSE0, A_BROWSE1, A_BROWSE2, A_PETGOODS, A_TRYON, A_TAKEOFF,
    A_RINGUP, A_PUTBACK,
    A_TABLE, A_ORDER0, A_ORDER1, A_ORDER2, A_WAIT, A_EAT, A_PAY, A_GAZE, A_HANDS, A_FOOTSIE, A_WATCHCHEF, A_TIP, A_LEAVE
};
static const char* TIER_NAME[3] = {"Cheap Eats", "House Special", "Chef's Tasting Menu"};
static const int TIER_COST[3] = {15, 35, 70};

static bool downtown(int r) { return r == R_DOWNTOWN || r == R_DTBOUTIQUE || r == R_DTBISTRO; }
static bool pay(int amt) { if (G.money < amt) { toast("Not enough money!"); sfxNo(); return false; } G.money -= amt; return true; }
static int itemPrice(int c) { return c == -2 ? 8 : c == -3 ? 20 : 40 + (c % 5) * 10; }
static int cartTotal() { int t = 0; for (int i = 0; i < H.cartN; i++) t += itemPrice(H.cart[i]); return t; }
static bool nameHas(int k, const char* const* words) { std::string n = OUTFITS[k].name; for (auto& ch : n) ch = (char)tolower(ch); for (int i = 0; words[i]; i++) if (n.find(words[i]) != std::string::npos) return true; return false; }
static int pickOutfit(int rack) {
    static const char* FORMAL[] = {"gown", "formal", "prom", "gala", "suit", "tux", "cocktail", "evening", "dress", nullptr};
    static const char* SWIM[] = {"swim", "bikini", "beach", "pool", "surf", nullptr};
    for (int tries = 0; tries < 120; tries++) {
        int k = 1 + irand(NUM_OUTFITS - 1);
        if (rack == 1 && !nameHas(k, FORMAL) && tries < 100) continue;
        if (rack == 2 && !nameHas(k, SWIM) && tries < 100) continue;
        bool inCart = false; for (int i = 0; i < H.cartN; i++) inCart |= H.cart[i] == k;
        if (tries < 60 && !lxTrendy(k)) continue;   // racks are stocked with this week's trend first
        if (!inCart && !ownsOutfit(k) && k < FIRST_KID_OUTFIT) return k;
    }
    return -1;
}
static void endTryOn() { if (H.tryingOn && H.prevOutfit >= 0) MC().outfit = (uint8_t)H.prevOutfit; H.tryingOn = 0; H.prevOutfit = -1; }
static bool partnerHere() { return G.date.active && G.date.partner >= 0 && G.date.partner < (int)G.sims.size(); }

std::vector<std::pair<std::string, int>> hotdateSpotOptions(int kind) {
    std::vector<std::pair<std::string, int>> v; char b[80];
    auto o = [&](const std::string& n, int id) { v.push_back({n, id}); };
    switch (kind) {
    case SP_TAXI: o("Taxi Home ($15)", A_TAXIHOME); break;
    case SP_HOTDOG: o("Hot Dog ($5)", A_HOTDOG); break;
    case SP_ICECART: o("Ice Cream Cone ($4)", A_ICECREAM); break;
    case SP_NEWSSTAND: o("Add Magazine to Cart ($8)", A_MAGAZINE); break;
    case SP_DRACK0: o("Browse Fashion Outfits", A_BROWSE0); break;
    case SP_DRACK1: o("Browse Formal Wear", A_BROWSE1); break;
    case SP_DRACK2: o("Browse Swimsuits", A_BROWSE2); o("Add Pet Goods ($20)", A_PETGOODS); break;
    case SP_DRESSBOOTH:
        if (H.tryingOn) o("Change Back", A_TAKEOFF);
        else for (int i = 0; i < H.cartN; i++) if (H.cart[i] >= 0) { snprintf(b, sizeof b, "Try On: %s", OUTFITS[H.cart[i]].name); o(b, A_TRYON); break; }
        if (v.empty()) o("Try On (cart is empty)", A_TRYON);
        break;
    case SP_DREGISTER:
        if (H.cartN) { snprintf(b, sizeof b, "Ring Up %d Item%s ($%d)", H.cartN, H.cartN > 1 ? "s" : "", cartTotal()); o(b, A_RINGUP); o("Put Items Back", A_PUTBACK); }
        else o("Clerk: \"Let me know when you're ready!\"", A_PUTBACK);
        break;
    case SP_PODIUM:
        if (H.dine == DS_NONE) o(partnerHere() ? "Request a Table for Two" : "Request a Table", A_TABLE);
        else if (H.dine == DS_CHECK) { snprintf(b, sizeof b, "Pay the Check ($%d)", (int)H.bill); o(b, A_PAY); }
        else o("Host: \"Your booth is ready!\"", A_LEAVE);
        break;
    case SP_DTABLE:
        if (H.dine == DS_NONE) { o("Ask the Host for a Table first", A_LEAVE); break; }
        if (H.dine == DS_SEATED) for (int k = 0; k < 3; k++) { snprintf(b, sizeof b, "Order %s ($%d)", TIER_NAME[k], TIER_COST[k]); o(b, A_ORDER0 + k); }
        if (H.dine == DS_ORDERED) o("Wait for the Waiter", A_WAIT);
        if (H.dine == DS_SERVED) o("Eat Your Meal", A_EAT);
        if (H.dine == DS_CHECK) { snprintf(b, sizeof b, "Pay the Check ($%d)", (int)H.bill); o(b, A_PAY); }
        if (partnerHere()) { o("Gaze", A_GAZE); o("Hold Hands", A_HANDS); o("Footsie", A_FOOTSIE); }
        break;
    case SP_STOVE: o("Watch the Chef", A_WATCHCHEF); o("Tip the Chef ($10)", A_TIP); break;
    }
    return v;
}

static void romance(int fr, int ro, const char* mood, const char* line, float score) {
    if (!partnerHere()) return;
    Sim& p = G.sims[G.date.partner];
    changeRel(G.mc, G.date.partner, fr, ro); G.date.score += score;
    addMoodlet(MC(), mood, EM_FLIRTY, 1, 120); addMoodlet(p, mood, EM_FLIRTY, 1, 120);
    toast(p.name.substr(0, p.name.find(' ')) + line); sfxYay();
}

void hotdateAction(int a) {
    Sim& me = MC(); Activity act; char b[120];
    switch (a) {
    case A_TAXIHOME:
        if (H.cartN) { toast("Ring up or put back your items first."); return; }
        if (H.dine == DS_CHECK || H.dine == DS_SERVED) { toast("Pay the check before you leave!"); return; }
        if (!pay(15)) return;
        H.dine = DS_NONE; endTryOn(); toast("The taxi drops you at home."); travelTo(R_LIVING); return;
    case A_HOTDOG: if (!pay(5)) return; me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + 30); toast("Hot dog from the stand. Classic."); return;
    case A_ICECREAM: if (!pay(4)) return; me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + 12); addMoodlet(me, "Ice Cream!", EM_HAPPY, 1, 90); return;
    case A_MAGAZINE: case A_PETGOODS:
        if (H.cartN >= 6) { toast("Your arms are full! Ring up first."); return; }
        H.cart[H.cartN++] = (int16_t)(a == A_MAGAZINE ? -2 : -3); toast(a == A_MAGAZINE ? "Magazine added to your cart." : "Pet goods added to your cart."); return;
    case A_BROWSE0: case A_BROWSE1: case A_BROWSE2: {
        if (H.cartN >= 6) { toast("Your arms are full! Ring up first."); return; }
        int k = pickOutfit(a - A_BROWSE0);
        if (k < 0) { toast("Nothing new on this rack for you."); return; }
        H.cart[H.cartN++] = (int16_t)k; snprintf(b, sizeof b, "You pick out %s ($%d). Try it on in the booth or ring it up.", OUTFITS[k].name, itemPrice(k)); toast(b); sfxTap(); return;
    }
    case A_TRYON:
        for (int i = 0; i < H.cartN; i++) if (H.cart[i] >= 0) {
            if (!H.tryingOn) H.prevOutfit = me.outfit;
            H.tryingOn = 1; me.outfit = (uint8_t)H.cart[i];
            act = makeAct("Trying On Clothes", 15, 8); startActivity(act);
            toast(std::string("Trying on ") + OUTFITS[H.cart[i]].name + ". Cute!"); return;
        }
        toast("Grab something off a rack first."); return;
    case A_TAKEOFF: endTryOn(); toast("Back in your own clothes."); return;
    case A_PUTBACK: endTryOn(); H.cartN = 0; toast("You put everything back."); return;
    case A_RINGUP: {
        const int tot = cartTotal(); if (!pay(tot)) return;
        int wear = -1; std::string got;
        for (int i = 0; i < H.cartN; i++) {
            int c = H.cart[i];
            if (c >= 0) { giveOutfit(c); wear = c; goalNotify(GE_BUYOUTFIT); }
            else if (c == -2) { skillGain(me, SK_STYLE, 0.3f); addMoodlet(me, "Read a Fashion Mag", EM_INSPIRED, 1, 240); }
            else { addMoodlet(me, "Spoiled My Pet", EM_HAPPY, 1, 240); }
        }
        H.tryingOn = 0; H.prevOutfit = -1; if (wear >= 0) me.outfit = (uint8_t)wear;
        snprintf(b, sizeof b, "The clerk rings you up: $%d. Bagged and in your closet!", tot); toast(b);
        dlHabit(5, 6 + H.cartN * 2);
        H.cartN = 0; lxRepAdd(CI_FASHION, 2, "fresh fits"); sfxYay(); return;
    }
    case A_TABLE:
        if (H.dine != DS_NONE) return;
        H.dine = DS_SEATED; H.bill = 0;
        toast(partnerHere() ? "The host walks you both to a cozy booth, sitting across from each other." : "The host walks you to an open booth and hands you a menu.");
        return;
    case A_ORDER0: case A_ORDER1: case A_ORDER2:
        H.tier = (uint8_t)(a - A_ORDER0); H.bill = TIER_COST[H.tier] * (partnerHere() ? 2 : 1); H.dine = DS_ORDERED;
        snprintf(b, sizeof b, "The waiter takes your order (%s) back to the chef.", TIER_NAME[H.tier]); toast(b); return;
    case A_WAIT:
        act = makeAct("Waiting for Food", 30, 8); act.rate[N_SOCIAL] = partnerHere() ? 10 : 2; startActivity(act);
        H.dine = DS_SERVED; toast("The chef plates it on the stove... the waiter carries out a platter!"); return;
    case A_EAT:
        act = makeAct("Eating at the Bistro", 45, 8); act.rate[N_HUNGER] = 70 + H.tier * 15; act.rate[N_FUN] = 4 + H.tier * 4; startActivity(act);
        if (partnerHere()) G.date.score += 4 + H.tier * 6;
        addMoodlet(me, H.tier == 2 ? "Five-Star Meal" : H.tier == 1 ? "Tasty Dinner" : "Cheap Eats", H.tier ? EM_HAPPY : EM_FINE, 1 + (H.tier == 2), 240);
        H.dine = DS_CHECK; toast("Delicious. The waiter brings the check."); return;
    case A_PAY:
        if (!pay(H.bill)) return;
        snprintf(b, sizeof b, "You paid $%d. The host thanks you!", (int)H.bill); toast(b); H.dine = DS_NONE; H.bill = 0; return;
    case A_GAZE: romance(2, 4, "Lost in Their Eyes", " gazes right back at you...", 3); return;
    case A_HANDS: romance(3, 6, "Holding Hands", " laces fingers with yours.", 4); return;
    case A_FOOTSIE: romance(2, 8, "Footsie!", " giggles and plays footsie back.", 5); return;
    case A_WATCHCHEF: act = makeAct("Watching the Chef", 20, 8); act.skill = SK_COOKING; act.skillRate = 10; startActivity(act); return;
    case A_TIP: if (!pay(10)) return; toast("The chef salutes you with a spatula."); return;
    case A_LEAVE: return;
    }
}

bool hotdateBlocks(int room, std::string* why) {
    if (!downtown(G.room) || G.room == room) return false;
    if (H.cartN && G.room == R_DTBOUTIQUE) { if (why) *why = "Ring up your items at the register (or put them back) before you leave!"; return true; }
    if ((H.dine == DS_SERVED || H.dine == DS_CHECK || H.dine == DS_ORDERED) && G.room == R_DTBISTRO) { if (why) *why = "You have to pay the check before leaving the bistro!"; return true; }
    return false;
}
bool hotdateTaxi(int target) {
    if (!downtown(target) || downtown(G.room)) return true;
    if (G.money < 15) { toast("Taxi fare is $15. You can't afford it!"); sfxNo(); return false; }
    G.money -= 15; toast("You call a taxi downtown. (-$15)"); return true;
}
void hotdateHourly() {
    if (!downtown(G.room)) { if (H.dine == DS_SEATED) H.dine = DS_NONE; endTryOn(); }
}

void hotdateSaveTo(const std::string& path) { FILE* f = fopen(path.c_str(), "wb"); if (!f) return; fwrite(&H, sizeof H, 1, f); fclose(f); }
void hotdateLoadFrom(const std::string& path) {
    H = HState{}; FILE* f = fopen(path.c_str(), "rb"); if (!f) return;
    HState h; size_t n = fread(&h, sizeof h, 1, f); fclose(f);
    if (n != 1 || h.magic != 0x48445431) return;
    h.dine %= 5; h.tier %= 3; if (h.cartN > 6) h.cartN = 0;
    for (auto& c : h.cart) if (c >= NUM_OUTFITS || c < -3) c = -2;
    if (h.prevOutfit >= NUM_OUTFITS) h.prevOutfit = -1;
    H = h;
}

void hotdateDrawRoom(int room, float t, bool night) {
    (void)t;
    switch (room) {
    case R_DOWNTOWN:
        grad(0, 0, TOP_W, 158, C(150, 200, 240), C(210, 230, 250));
        for (int k = 0; k < 6; k++) { const float x = k * 68.f, h = 80 + (k * 37) % 50; rect(x, 158 - h - 20, 64, h, C(k % 2 ? RGB{200, 170, 160} : RGB{170, 160, 190})); for (int w = 0; w < 6; w++) rect(x + 8 + (w % 3) * 18, 158 - h - 10 + (w / 3) * 22, 10, 12, C(night ? RGB{255, 230, 140} : RGB{220, 240, 255})); }
        rect(20, 120, 60, 26, C(250, 210, 40)); circle(34, 146, 6, C(30, 30, 30)); circle(66, 146, 6, C(30, 30, 30)); text("TAXI", 50, 124, 0.35f, C(30, 30, 30), 1);
        textOutline("DOWNTOWN", 200, 8, 0.6f, C(255, 255, 255), C(UI_INK), 1);
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(150, 150, 156)); for (int k = 0; k < 10; k++) rect(k * 40.f + 10, 196, 20, 3, C(240, 240, 240));
        break;
    case R_DTBOUTIQUE:
        grad(0, 0, TOP_W, 158, C(252, 236, 240), C(244, 222, 230));
        for (int r = 0; r < 3; r++) { const float x = 22 + r * 75.f; rect(x, 70, 56, 3, C(180, 150, 90)); for (int k = 0; k < 6; k++) rect(x + 2 + k * 9.f, 73, 7, 40, C(k % 3 == 0 ? RGB{255, 90, 160} : k % 3 == 1 ? RGB{90, 160, 230} : RGB{250, 220, 120})); }
        rect(258, 40, 50, 116, C(200, 120, 160)); rect(262, 44, 42, 112, C(255, 200, 220)); text("FITTING", 283, 30, 0.3f, C(UI_INK), 1);
        rect(330, 110, 64, 46, C(160, 110, 80)); rect(345, 98, 30, 14, C(60, 60, 70)); text("$", 360, 99, 0.4f, C(120, 255, 140), 1);
        textOutline("CHIC BOUTIQUE", 200, 8, 0.6f, C(255, 255, 255), C(255, 46, 140), 1);
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(236, 220, 210));
        break;
    case R_DTBISTRO:
        grad(0, 0, TOP_W, 158, C(110, 40, 40), C(150, 70, 60));
        for (int k = 0; k < 6; k++) circle(30 + k * 68.f, 30, 6, C(255, 220, 140));
        rect(20, 100, 40, 56, C(80, 40, 20)); text("HOST", 40, 92, 0.3f, C(255, 240, 220), 1);
        rect(150, 110, 100, 10, C(250, 245, 235)); rect(140, 100, 14, 56, C(140, 20, 40)); rect(246, 100, 14, 56, C(140, 20, 40)); circle(200, 106, 4, C(255, 200, 90));
        rect(310, 96, 80, 60, C(180, 180, 190)); for (int k = 0; k < 4; k++) circle(325 + k * 17.f, 100, 5, C(40, 40, 40));
        textOutline("LE BISTRO", 200, 46, 0.6f, C(255, 230, 200), C(UI_INK), 1);
        rect(0, 156, TOP_W, 2, C(UI_INK)); for (int k = 0; k < 20; k++) rect(k * 20.f, 158, 20, 82, C(k % 2 ? RGB{60, 30, 30} : RGB{240, 230, 220}));
        break;
    }
    if (night) rect(0, 0, TOP_W, TOP_H, C(10, 10, 30, 70));
}
