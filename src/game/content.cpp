#include "goals.hpp"
// City content pack: gifts & reactions, recipes -> real food, and original offline TV.
#include "content.hpp"
#include "district.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <algorithm>
using namespace col;
void toast(const std::string& m);
void say(int simIdx, const std::string& line);

namespace {
struct GiftDef { const char* code; const char* name; int cat; int price; const char* likes; const char* dislikes; const char* love; const char* hate; };
struct RecipeDef { const char* code; const char* name; int course; int cost; const char* ingredients; const char* steps; const char* tags; };
struct ShowDef { const char* title; const char* genre; const char* air; int rating; const char* ep[5]; const char* sum[5]; const char* view[2]; };
struct DlgLine { int ctx; const char* who; const char* text; };
#include "content_data.inc"
const int NGIFT = sizeof(GIFTS) / sizeof(GIFTS[0]), NREC = sizeof(RECIPES) / sizeof(RECIPES[0]), NSHOW = sizeof(SHOWS) / sizeof(SHOWS[0]), NDLG = sizeof(DLG) / sizeof(DLG[0]);
const int NREC_ALL = NREC;
const int NCAT = sizeof(GIFT_CATS) / sizeof(GIFT_CATS[0]);
struct StoreItem { const char* name; int da, price; const char* tags; };
const StoreItem STORE[] = {
    {"Bubble Tea", DA_BUBBLETEA, 6, "boba,tea,cute,drinks"}, {"Cupcake", DA_CUPCAKE, 6, "sweets,baking,cute"}, {"Party Cake", DA_CAKE, 30, "sweets,baking,sharing"},
    {"Ice Cream Cone", DA_ICECREAM, 4, "sweets,cute"}, {"Milkshake", DA_SHAKE, 8, "sweets,diner,cute"}, {"Lemonade", DA_LEMONADE, 2, "drinks,practical"},
    {"Fruit Cup", DA_FRUIT, 3, "fitness,practical"}, {"Tropical Mocktail", DA_MOCKTAIL, 8, "drinks,nightlife"}, {"Hot Cocoa", DA_COCOA, 5, "cozy,chocolate,sweets"},
    {"Crepe", DA_CREPE, 9, "sweets,foodie,aesthetic"}, {"Chips & Slushie", DA_CSNACK, 4, "snack,savory,gaming"},
};
const int NSTORE = sizeof(STORE) / sizeof(STORE[0]);
const char* foodName(int r) { return r >= 100 ? STORE[(r - 100) % NSTORE].name : RECIPES[r % NREC_ALL].name; }
const char* foodTags(int r) { return r >= 100 ? STORE[(r - 100) % NSTORE].tags : RECIPES[r % NREC_ALL].tags; }
const char* COURSE[6] = {"Breakfast", "Lunch", "Dinner", "Dessert", "Drink", "Snack"};
const char* QUAL[4] = {"Ruined", "Basic", "Good", "Excellent"};

// ---------------------------------------------------------------- taste profiles
// Every NPC gets their own stable likes/dislikes (from their traits + name), and the eight named
// characters from the design pack get their exact favorites when they exist in the save.
const char* TAGPOOL[] = {"romance", "luxury", "fashion", "sweets", "chocolate", "coffee", "tea", "boba", "cozy", "plants", "music", "vintage", "art", "creative",
                         "streetwear", "sneakers", "tech", "gaming", "fitness", "practical", "spicy", "foodie", "savory", "reading", "photography", "nightlife",
                         "crowds", "homebody", "cute", "aesthetic", "breakfast", "cheese", "sleep", "fragrance", "decor", "cats", "travel", "jazz", "minimalist", "baking"};
const int NTAG = sizeof(TAGPOOL) / sizeof(TAGPOOL[0]);
struct TasteProfile { const char* name; const char* love; const char* hate; const char* likeTags; const char* hateTags; };
const TasteProfile NAMED[] = {
    {"Alana", "G008,G014,G065", "G043,G063", "boba,fragrance,decor,hangouts,fashion", "spicy"},
    {"Rio", "G058,G033,G026", "G008,G069", "tech,art,quiet,ramen,coffee", "fragrance-sensitive,loud"},
    {"Suki", "G003,G006,G038", "G040,G034", "fashion,luxury,pastry,elegant", "cheap"},
    {"Jayden", "G031,G070,G067", "G004,G064", "sneakers,streetwear,gaming,savory", "formal"},
    {"Cassie", "G026,G029,G063", "G030,G050", "reading,art,tea,cozy", "crowds"},
    {"Noah", "G036,G057,G066", "G007", "fitness,practical,plants", "fragile"},
    {"Duncan", "G032,G065,G004", "G053", "practical,minimalist,coffee", "clutter"},
    {"Dora", "G021,G027,G051", "G058", "vintage,art,plants,colorful,tea", "nontech"},
};
const TasteProfile* named(const Sim& s) { for (const TasteProfile& p : NAMED) if (s.name.rfind(std::string(p.name) + " ", 0) == 0 || s.name == p.name) return &p; return nullptr; }
uint32_t h32(const std::string& s) { uint32_t h = 2166136261u; for (char c : s) h = (h ^ (uint8_t)c) * 16777619u; return h; }
bool hasTag(const char* list, const std::string& tag) {
    std::string L = std::string(",") + list + ","; return L.find("," + tag + ",") != std::string::npos;
}
std::vector<std::string> tastes(const Sim& s, bool likes) {
    std::vector<std::string> v;
    if (const TasteProfile* p = named(s)) { std::string L = likes ? p->likeTags : p->hateTags; size_t a = 0; while (a < L.size()) { size_t b = L.find(',', a); if (b == std::string::npos) b = L.size(); v.push_back(L.substr(a, b - a)); a = b + 1; } }
    struct TT { int tr; const char* tag; };
    static const TT LIKE[] = {{TR_FASHIONISTA, "fashion"}, {TR_ROMANTIC, "romance"}, {TR_BOOKWORM, "reading"}, {TR_FOODIE, "foodie"}, {TR_CREATIVE, "art"}, {TR_OUTGOING, "nightlife"},
                              {TR_LONER, "homebody"}, {TR_ACTIVE, "fitness"}, {TR_GLAMOROUS, "luxury"}, {TR_MUSIC, "music"}, {TR_PETLOVER, "cats"}, {TR_GAMER, "gaming"}, {TR_FRUGAL, "practical"}, {TR_NIGHTOWL, "nightlife"}, {TR_SNOB, "luxury"}};
    static const TT HATE[] = {{TR_LONER, "crowds"}, {TR_FRUGAL, "luxury"}, {TR_NEAT, "clutter"}, {TR_ACTIVE, "homebody"}, {TR_BOOKWORM, "nightlife"}, {TR_LAZY, "fitness"}};
    if (likes) { for (const TT& t : LIKE) if (s.hasTrait((Trait)t.tr)) v.push_back(t.tag); }
    else { for (const TT& t : HATE) if (s.hasTrait((Trait)t.tr)) v.push_back(t.tag); }
    uint32_t h = h32(s.name);
    for (int k = 0; k < (likes ? 4 : 2); k++) { std::string tg = TAGPOOL[(h >> (k * 5)) % NTAG]; if (likes ? true : std::find(v.begin(), v.end(), tg) == v.end()) v.push_back(tg); h = h * 2654435761u + 7; }
    if (!likes) { auto L = tastes(s, true); v.erase(std::remove_if(v.begin(), v.end(), [&](const std::string& x) { return std::find(L.begin(), L.end(), x) != L.end(); }), v.end()); }
    return v;
}
int scoreTags(const Sim& s, const char* likeList, const char* hateList) {
    int sc = 0;
    for (auto& t : tastes(s, true)) { if (hasTag(likeList, t)) sc += 2; if (hateList && hasTag(hateList, t)) sc -= 1; }
    for (auto& t : tastes(s, false)) { if (hasTag(likeList, t)) sc -= 3; if (hateList && hasTag(hateList, t)) sc -= 2; }
    return sc;
}

// ---------------------------------------------------------------- UI state
int tab = CT_SHOP, cat = 0, page = 0, sel = -1, giveTo = -1, showSel = -1, epSel = -1;
enum Sub { SB_LIST, SB_EPISODES, SB_PLAY, SB_DONE };
int sub = SB_LIST;
int pendingRecipe = -1;
// TV playback
int scene = 0; float sceneT = 0; bool autoplay = true; int rooting = -1; std::string actorA, actorB, tvResult;
int actorSimA = -1, actorSimB = -1;

std::string fmt(const char* f, ...) { char b[400]; va_list a; va_start(a, f); vsnprintf(b, sizeof b, f, a); va_end(a); return b; }
int gcount(int g) { return g < (int)G.giftInv.size() ? G.giftInv[g] : 0; }
void gadd(int g, int n) { if (G.giftInv.size() < (size_t)NGIFT) G.giftInv.resize(NGIFT); G.giftInv[g] = (uint8_t)std::clamp(G.giftInv[g] + n, 0, 99); }
int ageDays(const FoodItem& f) { return (int)G.day - (int)f.day; }
bool spoiled(const FoodItem& f) { return ageDays(f) > 6; }
int mcRating() { const Sim& me = MC(); return me.stage >= ST_YA ? 2 : me.stage >= ST_TEEN ? 1 : 0; }

std::vector<int> listGifts() { std::vector<int> v; for (int i = 0; i < NGIFT; i++) if (GIFTS[i].cat == cat) v.push_back(i); return v; }
struct BagRow { bool food; int idx; };
std::vector<BagRow> bagRows() { std::vector<BagRow> v; for (int i = 0; i < NGIFT; i++) if (gcount(i)) v.push_back({false, i}); for (size_t i = 0; i < G.foodInv.size(); i++) v.push_back({true, (int)i}); return v; }

// scene text for an episode: built from the show's own episode synopsis
std::string sceneText(int s, int e, int k) {
    const ShowDef& S = SHOWS[s];
    std::string sum = S.sum[e];
    switch (k) {
    case 0: return fmt("%s - Season 1, Episode %d: \"%s\"", S.title, e + 1, S.ep[e]);
    case 1: return fmt("Previously... %s gave us drama we are STILL recovering from. Tonight: %s.", S.title, sum.c_str());
    case 2: return fmt("%s is the first to notice something is off. %s won't admit it, but everyone can see it.", actorA.c_str(), actorB.c_str());
    case 3: return fmt("The big moment: %s. %s and %s face each other while the whole room goes quiet.", sum.c_str(), actorA.c_str(), actorB.c_str());
    case 4: return fmt("End of episode %d. Next time on %s: \"%s\".", e + 1, S.title, e < 4 ? S.ep[e + 1] : "Season Finale Reunion");
    }
    return "";
}
std::vector<int> coViewers() { std::vector<int> v; for (const Actor& a : G.rooms[G.house][G.room].actors) if (a.kind == 0 && a.id != G.mc && a.id < G.sims.size() && G.sims[a.id].stage >= ST_CHILD && !a.sleeping) v.push_back(a.id); return v; }
void finishEpisode() {
    const ShowDef& S = SHOWS[showSel];
    const int id = showSel * 5 + epSel;
    const bool first = !(G.tvWatched >> id & 1);
    G.tvWatched |= (1ull << id);
    Sim& me = MC();
    me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 25);
    tvResult = first ? "New episode watched!" : "Rerun watched.";
    auto vs = coViewers();
    for (size_t k = 0; k < vs.size() && k < 3; k++) {
        int o = vs[k];
        const Sim& s = G.sims[o];
        if (S.rating > 0 && s.stage < (S.rating == 2 ? ST_YA : ST_TEEN)) continue;
        int fav = scoreTags(s, S.rating == 2 ? "romance,nightlife,crowds" : S.rating == 1 ? "fashion,music,vintage,cute" : "cozy,foodie,art,fashion", "");
        std::string line = fav >= 0 ? S.view[(h32(s.name) + epSel) & 1] : contentLine("TV_SHOWS_WATCHING_AND_REACTIONS", h32(s.name) + epSel);
        say(o, line);
        changeRel(G.mc, o, fav >= 0 ? 3 : 1, 0);
        addMemoryOnce(MEM_FRIEND, G.mc, o, std::string("Watched ") + S.title + " together.");
        tvResult += "\n" + s.name.substr(0, s.name.find(' ')) + ": \"" + line + "\"";
    }
    if (vs.empty()) tvResult += "\nYou watched alone. Maybe invite someone next time!";
    sub = SB_DONE;
}

// ---------------------------------------------------------------- layout
Btn tabB(int k) { return Btn{2 + k * 79.5f, 2, 78, 20}; }
Btn rowB(int k) { return Btn{6, 50 + k * 25.f, 308, 23}; }
const Btn bBack{4, 208, 64, 28}, bPrev{72, 208, 44, 28}, bNext{120, 208, 44, 28}, bAct{168, 208, 148, 28}, bCatL{6, 26, 30, 20}, bCatR{284, 26, 30, 20};
}

// ---------------------------------------------------------------- public
std::string contentLine(const char* ctx, uint32_t seed) {
    int c = -1; for (size_t k = 0; k < sizeof(DLG_CTX) / sizeof(DLG_CTX[0]); k++) if (!strcmp(DLG_CTX[k], ctx)) c = (int)k;
    std::vector<int> pick; for (int i = 0; i < NDLG; i++) if (DLG[i].ctx == c && !strcmp(DLG[i].who, "ANY")) pick.push_back(i);
    if (pick.empty()) return "...";
    return DLG[pick[seed % pick.size()]].text;
}
void contentOpen(int t, int target) { tab = t; giveTo = target; page = 0; sel = -1; sub = SB_LIST; if (target >= 0) tab = CT_BAG; }
bool contentCooking() { return pendingRecipe >= 0; }
bool contentHasGiftables() { for (int i = 0; i < NGIFT; i++) if (gcount(i)) return true; for (auto& f : G.foodInv) if (!spoiled(f) && f.quality) return true; return false; }
void contentGiftOptions(std::vector<std::pair<std::string, int>>& out) {
    for (int i = 0; i < NGIFT; i++) if (gcount(i)) out.push_back({std::string(GIFTS[i].name) + (gcount(i) > 1 ? " (x" + std::to_string(gcount(i)) + ")" : ""), 1700 + i});
    for (size_t i = 0; i < G.foodInv.size() && i < 90; i++) { const FoodItem& f = G.foodInv[i]; if (!spoiled(f) && f.quality) out.push_back({std::string(foodName(f.recipe)) + " (" + QUAL[f.quality & 3] + ")", 1800 + (int)i}); }
}
void contentGive(int optId, int t) {
    if (t < 0 || t >= (int)G.sims.size() || t == G.mc) return;
    Sim& s = G.sims[t];
    const Rel* r = findRel(G.mc, t);
    const int fr = r ? r->fr : 0;
    std::string line; int result;   // 0 hate, 1 neutral, 2 like, 3 love
    std::string what;
    if (optId >= 1700 && optId < 1700 + NGIFT) {
        const int g = optId - 1700; if (!gcount(g)) return;
        const GiftDef& d = GIFTS[g]; what = d.name;
        int sc = scoreTags(s, d.likes, d.dislikes);
        if (const TasteProfile* p = named(s)) { if (std::string(p->love).find(d.code) != std::string::npos) sc += 6; if (std::string(p->hate).find(d.code) != std::string::npos) sc -= 6; }
        int repeats = 0; for (auto& l : G.giftLog) if (l.sim == t && l.item == g && G.day - l.day < 7) repeats++;
        sc -= repeats * 2;
        if (s.stage < ST_TEEN && (hasTag(d.likes, "nightlife") || hasTag(d.likes, "romance"))) sc = -9;   // age-appropriate gifts only
        result = sc >= 4 ? 3 : sc >= 1 ? 2 : sc >= -1 ? 1 : 0;
        line = result == 3 ? d.love : result == 0 ? d.hate : contentLine("GIFTS_AND_GIFT_REACTIONS", h32(s.name) + g);
        if (repeats) line = "Another one? " + line;
        // a very bad match from a near stranger gets politely handed back: it never vanishes
        if (result == 0 && fr < 10) { line += " (They hand it back to you.)"; }
        else gadd(g, -1);
        G.giftLog.push_back({(int16_t)t, (uint8_t)g, (uint8_t)result, G.day});
    } else if (optId >= 1800 && optId < 1900) {
        const int i = optId - 1800; if (i >= (int)G.foodInv.size()) return;
        FoodItem f = G.foodInv[i];
        if (spoiled(f) || !f.quality) { toast("That food isn't fit to give anyone."); sfxNo(); return; }
        what = foodName(f.recipe);
        int sc = scoreTags(s, foodTags(f.recipe), nullptr) + (f.quality - 1);
        result = sc >= 4 ? 3 : sc >= 1 ? 2 : sc >= -1 ? 1 : 0;
        line = result >= 2 ? contentLine("FOOD_COOKING_AND_NEEDS", h32(s.name) + f.recipe) : result == 1 ? "Thanks! I'll have it later." : "Oh... I don't really eat this. But thank you.";
        G.foodInv.erase(G.foodInv.begin() + i);
        if (result >= 1) s.needs[N_HUNGER] = std::min(100.f, s.needs[N_HUNGER] + 15.f * f.quality);   // they actually eat it
        G.giftLog.push_back({(int16_t)t, (uint8_t)(100 + f.recipe % 120), (uint8_t)result, G.day});
    } else return;
    if (G.giftLog.size() > 120) G.giftLog.erase(G.giftLog.begin());
    static const int DFR[4] = {-4, 2, 6, 12};
    changeRel(G.mc, t, DFR[result], 0);
    goalNotify(GE_SOCIAL, t);   // gifts build friendship; they never force romance
    say(t, line);
    if (result == 3) { addMemoryOnce(MEM_FRIEND, t, G.mc, "Loved the " + what + " I got."); sfxYay(); }
    static const char* VERB[4] = {"disliked", "thought the", "liked the", "LOVED the"};
    toast(s.name.substr(0, s.name.find(' ')) + " " + VERB[result] + (result == 0 ? " the " : " ") + what + (result == 1 ? " was okay." : "!"));
}
bool contentCookDone(int plates, int good, int stars, int burnt) {
    if (pendingRecipe < 0) return false;
    const int r = pendingRecipe; pendingRecipe = -1;
    const RecipeDef& R = RECIPES[r];
    if (!plates) { toast("Nothing was cooked, so no ingredients were used."); return true; }
    if (G.money < R.cost) { toast("You can't afford the ingredients anymore."); sfxNo(); return true; }
    G.money -= R.cost;   // ingredients are spent exactly once, when the dish is actually made
    int q = good == 0 ? 0 : (stars / std::max(1, good)) >= 3 && !burnt ? 3 : (stars / std::max(1, good)) >= 2 ? 2 : 1;
    if (r < 64) G.recipesMade |= 1ull << r; else G.recipesMade2 |= 1ull << (r - 64);
    FoodItem f; f.recipe = (uint8_t)r; f.quality = (uint8_t)q; f.day = G.day; f.servings = (uint8_t)std::max(1, good);
    if (G.foodInv.size() < 40) G.foodInv.push_back(f);
    toast(fmt("You made %s (%s, %d serving%s)! It's in the fridge: eat it or gift it from your Bag.", R.name, QUAL[q], f.servings, f.servings == 1 ? "" : "s"));
    if (q >= 2) addMoodlet(MC(), "Home Chef", EM_CONFIDENT, 1, 240);
    sfxYay();
    return true;
}
void contentDaily() {
    for (size_t i = 0; i < G.foodInv.size();) { if (ageDays(G.foodInv[i]) > 12) G.foodInv.erase(G.foodInv.begin() + i); else i++; }   // long-spoiled food gets tossed
}

int contentUpdate(const Input& in) {
    if (sub == SB_PLAY) {
        sceneT += in.dt;
        const bool next = (in.down & KEY_A) || (in.touch && bAct.in(in.tp)) || (autoplay && sceneT > 5.5f);
        if (in.down & KEY_START || (in.touch && bNext.in(in.tp))) { autoplay = !autoplay; sfxTap(); }   // pause / play
        if ((in.down & KEY_Y) || (in.touch && bPrev.in(in.tp))) { scene = 4; sceneT = 0; sfxTap(); }   // skip to the end
        if ((in.down & KEY_L) && scene > 0) { scene--; sceneT = 0; }
        if (in.down & KEY_B || (in.touch && bBack.in(in.tp))) { sub = SB_EPISODES; return 0; }
        if (next) { sceneT = 0; if (scene < 4) scene++; else { rooting = -1; finishEpisode(); } sfxTap(); }
        return 0;
    }
    if (sub == SB_DONE) {
        if (!in.touch && !(in.down & (KEY_A | KEY_B))) return 0;
        if (in.touch && rowB(4).in(in.tp)) { rooting = 0; } else if (in.touch && rowB(5).in(in.tp)) { rooting = 1; }
        else if (in.touch && bNext.in(in.tp)) { scene = 0; sceneT = 0; sub = SB_PLAY; return 0; }   // replay
        else if ((in.down & (KEY_A | KEY_B)) || (in.touch && bBack.in(in.tp))) { sub = SB_EPISODES; return 0; }
        return 0;
    }
    if (in.down & KEY_B) { if (sub == SB_EPISODES) { sub = SB_LIST; return 0; } return 1; }
    if (in.down & KEY_R) { tab = (tab + 1) % 4; page = 0; sel = -1; sub = SB_LIST; }
    if (in.down & KEY_L) { tab = (tab + 3) % 4; page = 0; sel = -1; sub = SB_LIST; }
    if (!in.touch) return 0;
    for (int k = 0; k < 4; k++) if (tabB(k).in(in.tp)) { tab = k; page = 0; sel = -1; sub = SB_LIST; sfxTap(); return 0; }
    if (bBack.in(in.tp)) { if (sub == SB_EPISODES) { sub = SB_LIST; return 0; } return 1; }
    if (bPrev.in(in.tp)) { page = std::max(0, page - 1); sel = -1; sfxTap(); return 0; }
    if (bNext.in(in.tp)) { page++; sel = -1; sfxTap(); return 0; }
    switch (tab) {
    case CT_SHOP: {
        if (bCatL.in(in.tp)) { cat = (cat + NCAT - 1) % NCAT; page = 0; sel = -1; sfxTap(); return 0; }
        if (bCatR.in(in.tp)) { cat = (cat + 1) % NCAT; page = 0; sel = -1; sfxTap(); return 0; }
        auto v = listGifts(); page = std::min(page, std::max(0, ((int)v.size() - 1) / 6));
        for (int k = 0; k < 6; k++) { int i = page * 6 + k; if (i < (int)v.size() && rowB(k).in(in.tp)) { sel = v[i]; sfxTap(); return 0; } }
        if (sel >= 0 && bAct.in(in.tp)) {
            const GiftDef& d = GIFTS[sel];
            if (G.money < d.price) { toast("Not enough money!"); sfxNo(); return 0; }
            G.money -= d.price; gadd(sel, 1); toast(std::string("Bought ") + d.name + "! It's in your Bag."); sfxYay();
        }
        return 0; }
    case CT_BAG: {
        auto v = bagRows(); page = std::min(page, std::max(0, ((int)v.size() - 1) / 6));
        for (int k = 0; k < 6; k++) { int i = page * 6 + k; if (i < (int)v.size() && rowB(k).in(in.tp)) { sel = i; sfxTap(); return 0; } }
        if (sel >= 0 && sel < (int)v.size() && bAct.in(in.tp)) {
            const BagRow& b = v[sel];
            if (giveTo >= 0) { contentGive(b.food ? 1800 + b.idx : 1700 + b.idx, giveTo); sel = -1; return 1; }
            if (b.food) {
                FoodItem& f = G.foodInv[b.idx];
                if (spoiled(f)) { G.foodInv.erase(G.foodInv.begin() + b.idx); toast("Gross - it spoiled. You tossed it in the trash."); sel = -1; return 0; }
                Sim& me = MC(); me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + 20.f + 12.f * f.quality);
                if (f.quality == 0) addMoodlet(me, "Ate Something Burnt", EM_UNCOMFORTABLE, 1, 120);
                if (f.quality == 3) addMoodlet(me, "Delicious Meal", EM_HAPPY, 1, 180);
                toast(std::string("You ate the ") + foodName(f.recipe) + ".");
                if (--f.servings == 0) G.foodInv.erase(G.foodInv.begin() + b.idx);
                sel = -1;
            } else toast("Gifts are given from a person's menu: tap someone, then \"Give a Gift >\".");
        }
        return 0; }
    case CT_RECIPES: {
        page = std::min(page, (NREC - 1) / 6);
        for (int k = 0; k < 6; k++) { int i = page * 6 + k; if (i < NREC && rowB(k).in(in.tp)) { sel = i; sfxTap(); return 0; } }
        if (sel >= 0 && bAct.in(in.tp)) {
            if (G.money < RECIPES[sel].cost) { toast("You can't afford the ingredients."); sfxNo(); return 0; }
            pendingRecipe = sel; toast(std::string("Let's make ") + RECIPES[sel].name + "! Cook and plate it, then tap EAT to finish the dish.");
            return 2;
        }
        return 0; }
    case CT_TV: {
        if (sub == SB_LIST) {
            page = std::min(page, (NSHOW - 1) / 6);
            for (int k = 0; k < 6; k++) { int i = page * 6 + k; if (i < NSHOW && rowB(k).in(in.tp)) {
                if (SHOWS[i].rating > mcRating()) { toast(SHOWS[i].rating == 2 ? "That show is for adults only." : "That show is for teens and up."); sfxNo(); return 0; }
                showSel = i; sub = SB_EPISODES; sfxTap(); return 0; } }
        } else if (sub == SB_EPISODES) {
            for (int k = 0; k < 5; k++) if (rowB(k).in(in.tp)) {
                epSel = k; scene = 0; sceneT = 0; autoplay = true; sub = SB_PLAY;
                // two townies "star" in the episode stills
                std::vector<int> pool; for (size_t j = 0; j < G.sims.size(); j++) if (!G.sims[j].household && G.sims[j].stage >= (SHOWS[showSel].rating ? ST_TEEN : ST_CHILD)) pool.push_back((int)j);
                if (pool.size() >= 2) { uint32_t hh = h32(SHOWS[showSel].title); actorSimA = pool[hh % pool.size()]; actorSimB = pool[(hh / 7 + 1) % pool.size()]; if (actorSimB == actorSimA) actorSimB = pool[(hh / 7 + 2) % pool.size()]; }
                else { actorSimA = actorSimB = -1; }
                actorA = actorSimA >= 0 ? G.sims[actorSimA].name.substr(0, G.sims[actorSimA].name.find(' ')) : "Jules";
                actorB = actorSimB >= 0 ? G.sims[actorSimB].name.substr(0, G.sims[actorSimB].name.find(' ')) : "Marisol";
                sfxTap(); return 0;
            }
        }
        return 0; }
    }
    return 0;
}

void contentDrawTop(float t) {
    if (sub == SB_PLAY || sub == SB_DONE) {   // the TV set
        const ShowDef& S = SHOWS[showSel];
        rect(0, 0, TOP_W, TOP_H, C(40, 30, 46));
        rrect(14, 8, 372, 226, 14, C(20, 18, 24)); rrect(22, 14, 356, 186, 8, C(0, 0, 0));
        static const RGB TINT[12] = {{255, 170, 120}, {200, 120, 255}, {120, 200, 255}, {120, 255, 200}, {255, 220, 120}, {150, 160, 255}, {255, 160, 200}, {255, 200, 160}, {240, 140, 200}, {200, 230, 140}, {120, 120, 160}, {255, 210, 170}};
        const RGB c = TINT[showSel % 12];
        grad(24, 16, 352, 182, C(c.r / 2, c.g / 2, c.b / 2), C(c));
        for (int k = 0; k < 6; k++) circle(60 + k * 60.f + std::sin(t + k) * 8, 40 + (k % 3) * 18.f, 4, C(255, 255, 255, 60));
        if (actorSimA >= 0) { circle(130, 110, 46, C(255, 255, 255, 60)); drawSimPortrait(G.sims[actorSimA], 130, 110, 42, t); }
        if (actorSimB >= 0) { circle(270, 110, 46, C(255, 255, 255, 60)); drawSimPortrait(G.sims[actorSimB], 270, 110, 42, t); }
        rect(24, 166, 352, 32, C(0, 0, 0, 170));
        if (sub == SB_PLAY) {
            auto lines = wrapText(sceneText(showSel, epSel, scene), 0.4f, 340);
            for (size_t k = 0; k < lines.size() && k < 2; k++) text(lines[k].c_str(), 30, 168 + k * 14.f, 0.4f, C(255, 255, 255));
            if (!autoplay) textOutline("PAUSED", 200, 24, 0.5f, C(255, 255, 255), C(UI_INK), 1);
        } else text("THE END - thanks for watching!", 200, 174, 0.45f, C(255, 255, 255), 1);
        text(S.title, 200, 206, 0.45f, C(255, 230, 240), 1);
        char b[64]; snprintf(b, sizeof b, "Scene %d/5", scene + 1); text(b, 370, 220, 0.34f, C(200, 190, 210), 2);
        return;
    }
    grad(0, 0, TOP_W, TOP_H, C(255, 240, 246), C(236, 228, 255));
    static const char* TT[4] = {"GIFT SHOP", "MY BAG", "RECIPES", "TV GUIDE"};
    textOutline(giveTo >= 0 ? "PICK A GIFT" : TT[tab], 200, 4, 0.6f, C(255, 255, 255), C(BERRY), 1);
    char b[220];
    snprintf(b, sizeof b, "$%d", G.money); text(b, 390, 6, 0.42f, C(BERRY), 2);
    float y = 36;
    auto para = [&](const std::string& s, float sc, RGB col) { for (auto& l : wrapText(s, sc, 370)) { if (y > 228) break; text(l.c_str(), 14, y, sc, C(col)); y += sc * 32; } };
    if (tab == CT_SHOP && sel >= 0) {
        const GiftDef& d = GIFTS[sel];
        para(d.name, 0.55f, BERRY); para(GIFT_CATS[d.cat], 0.4f, RGB{110, 90, 120});
        para(fmt("Price: $%d     You own: %d", d.price, gcount(sel)), 0.42f, UI_INK);
        para(std::string("People who like: ") + d.likes, 0.4f, UI_INK);
        para(std::string("Might not suit: ") + d.dislikes, 0.4f, UI_INK);
        para(std::string("\"") + d.love + "\"", 0.4f, RGB{60, 140, 90});
    } else if (tab == CT_RECIPES && sel >= 0) {
        const RecipeDef& R = RECIPES[sel];
        para(R.name, 0.55f, BERRY); para(COURSE[R.course], 0.4f, RGB{110, 90, 120});
        para(std::string("Ingredients: ") + R.ingredients, 0.42f, UI_INK);
        para(std::string("Steps: ") + R.steps, 0.42f, UI_INK);
        para(fmt("Ingredient cost: $%d (paid only when you finish the dish)", R.cost), 0.4f, UI_INK);
        para("Better cooking = better quality: Ruined, Basic, Good or Excellent.", 0.38f, RGB{110, 90, 120});
    } else if (tab == CT_TV) {
        const int s = sub == SB_EPISODES ? showSel : sel >= 0 ? sel : 0;
        const ShowDef& S = SHOWS[s];
        para(S.title, 0.55f, BERRY); para(S.genre, 0.4f, UI_INK);
        para(fmt("Airs %s   Rated: %s", S.air, S.rating == 2 ? "Adult" : S.rating == 1 ? "Teen" : "All Ages"), 0.4f, RGB{110, 90, 120});
        int w = 0; for (int e = 0; e < 5; e++) if (G.tvWatched >> (s * 5 + e) & 1) w++;
        para(fmt("You've watched %d of 5 episodes.", w), 0.4f, UI_INK);
        para("Watch with family or friends in the room and they'll react!", 0.38f, RGB{110, 90, 120});
    } else if (tab == CT_BAG) {
        auto v = bagRows();
        if (sel >= 0 && sel < (int)v.size()) {
            const BagRow& r = v[sel];
            if (r.food) { const FoodItem& f = G.foodInv[r.idx]; para(foodName(f.recipe), 0.55f, BERRY);
                para(fmt("Quality: %s   Servings: %d", QUAL[f.quality & 3], f.servings), 0.42f, UI_INK);
                para(spoiled(f) ? "SPOILED - toss it." : fmt("Made %d day%s ago (stays fresh about 6 days in the fridge).", ageDays(f), ageDays(f) == 1 ? "" : "s"), 0.4f, UI_INK); }
            else { const GiftDef& d = GIFTS[r.idx]; para(d.name, 0.55f, BERRY); para(fmt("You have %d.", gcount(r.idx)), 0.42f, UI_INK); para(std::string("Likes: ") + d.likes, 0.4f, UI_INK); }
        } else para(giveTo >= 0 ? "Choose something to give to " + G.sims[giveTo].name + "." : "Everything you've bought or cooked lives here.", 0.42f, UI_INK);
    } else para(tab == CT_SHOP ? "70 city gifts in 9 categories. Every person has their own tastes - pay attention to what they love!" : "Pick something to see the details.", 0.42f, UI_INK);
}

void contentDrawBottom(float t) {
    (void)t;
    grad(0, 0, BOT_W, BOT_H, C(255, 246, 250), C(244, 236, 255));
    if (sub == SB_PLAY) {
        text("A / tap = next scene", 160, 40, 0.42f, C(UI_INK), 1);
        text("START = pause    Y = skip    L = back a scene", 160, 62, 0.36f, C(110, 100, 120), 1);
        button(bBack, "Stop", false, 1, 0.4f); button(bPrev, "Skip", false, 0, 0.36f); button(bNext, autoplay ? "||" : ">", false, 0, 0.45f);
        button(bAct, "Next Scene", true, 2, 0.4f);
        return;
    }
    if (sub == SB_DONE) {
        float y = 8;
        for (auto& l : wrapText(tvResult, 0.38f, 300)) { if (y > 140) break; text(l.c_str(), 10, y, 0.38f, C(UI_INK)); y += 13; }
        if (rooting < 0) { text("Who are you rooting for?", 160, 136, 0.4f, C(BERRY), 1); button(rowB(4), actorA.c_str(), false, 0, 0.4f); button(rowB(5), actorB.c_str(), false, 3, 0.4f); }
        else text(fmt("Team %s! You'll be talking about this all week.", rooting ? actorB.c_str() : actorA.c_str()).c_str(), 160, 160, 0.4f, C(BERRY), 1);
        button(bBack, "Done", false, 1, 0.4f); button(bNext, "Re", false, 0, 0.4f);
        return;
    }
    static const char* TN[4] = {"Shop", "Bag", "Recipes", "TV"};
    for (int k = 0; k < 4; k++) button(tabB(k), TN[k], tab == k, k == 3 ? 3 : 0, 0.4f);
    char b[96];
    int total = 0;
    auto row = [&](int k, const std::string& label, bool on, int style) { button(rowB(k), label.c_str(), on, style, 0.36f); };
    switch (tab) {
    case CT_SHOP: {
        button(bCatL, "<", false, 0, 0.42f); button(bCatR, ">", false, 0, 0.42f);
        text(GIFT_CATS[cat], 160, 28, 0.36f, C(BERRY), 1);
        auto v = listGifts(); total = (int)v.size();
        for (int k = 0; k < 6; k++) { int i = page * 6 + k; if (i >= total) break; snprintf(b, sizeof b, "%s  $%d", GIFTS[v[i]].name, GIFTS[v[i]].price); row(k, b, sel == v[i], 0); }
        if (sel >= 0) button(bAct, "Buy", true, 2, 0.42f);
        break; }
    case CT_BAG: {
        auto v = bagRows(); total = (int)v.size();
        text(giveTo >= 0 ? ("Give to " + G.sims[giveTo].name).c_str() : "Gifts & food", 160, 28, 0.38f, C(BERRY), 1);
        if (v.empty()) text("Your bag is empty. Visit the Shop or cook something!", 160, 110, 0.38f, C(UI_INK), 1);
        for (int k = 0; k < 6; k++) {
            int i = page * 6 + k; if (i >= total) break;
            const BagRow& r = v[i];
            if (r.food) { const FoodItem& f = G.foodInv[r.idx]; snprintf(b, sizeof b, "%s (%s)%s", foodName(f.recipe), QUAL[f.quality & 3], spoiled(f) ? " - spoiled" : ""); row(k, b, sel == i, spoiled(f) ? 1 : 2); }
            else { snprintf(b, sizeof b, "%s x%d", GIFTS[r.idx].name, gcount(r.idx)); row(k, b, sel == i, 0); }
        }
        if (sel >= 0 && sel < total) button(bAct, giveTo >= 0 ? "Give It!" : v[sel].food ? (spoiled(G.foodInv[v[sel].idx]) ? "Toss It" : "Eat") : "How to Give", true, 2, 0.4f);
        break; }
    case CT_RECIPES: {
        total = NREC; text("75 recipes - cooked in your kitchen", 160, 28, 0.36f, C(BERRY), 1);
        for (int k = 0; k < 6; k++) { int i = page * 6 + k; if (i >= total) break; snprintf(b, sizeof b, "%s  ($%d)  %s", RECIPES[i].name, RECIPES[i].cost, COURSE[RECIPES[i].course]); row(k, b, sel == i, 0); }
        if (sel >= 0) button(bAct, "Cook It!", true, 2, 0.42f);
        break; }
    case CT_TV: {
        if (sub == SB_EPISODES) {
            text(SHOWS[showSel].title, 160, 28, 0.4f, C(BERRY), 1);
            for (int k = 0; k < 5; k++) { snprintf(b, sizeof b, "Ep %d: %s%s", k + 1, SHOWS[showSel].ep[k], G.tvWatched >> (showSel * 5 + k) & 1 ? "  (seen)" : ""); row(k, b, false, G.tvWatched >> (showSel * 5 + k) & 1 ? 0 : 2); }
            break;
        }
        total = NSHOW; text("12 original shows - all offline", 160, 28, 0.36f, C(BERRY), 1);
        for (int k = 0; k < 6; k++) {
            int i = page * 6 + k; if (i >= total) break;
            const bool locked = SHOWS[i].rating > mcRating();
            snprintf(b, sizeof b, "%s%s", SHOWS[i].title, locked ? (SHOWS[i].rating == 2 ? "  (18+)" : "  (13+)") : "");
            row(k, b, sel == i, locked ? 1 : 3);
        }
        break; }
    }
    button(bBack, giveTo >= 0 ? "Cancel" : "Back", false, 1, 0.4f);
    if (tab != CT_TV || sub == SB_LIST) {
        const int pages = std::max(1, (total + 5) / 6);
        if (page >= pages) page = pages - 1;
        button(bPrev, "<", false, 0, 0.45f); button(bNext, ">", false, 0, 0.45f);
        if (!(sel >= 0)) { snprintf(b, sizeof b, "page %d/%d", page + 1, pages); text(b, 240, 216, 0.36f, C(110, 100, 120), 1); }
    }
}

int contentToGoFor(int da) { for (int k = 0; k < NSTORE; k++) if (STORE[k].da == da) return k; return -1; }
const char* contentToGoName(int k) { return STORE[k % NSTORE].name; }
int contentToGoPrice(int k) { return STORE[k % NSTORE].price; }
void contentBuyToGo(int k) {
    k %= NSTORE;
    if (G.money < STORE[k].price) { toast("Not enough money!"); sfxNo(); return; }
    if (G.foodInv.size() >= 40) { toast("Your backpack is full!"); sfxNo(); return; }
    G.money -= STORE[k].price;
    FoodItem f; f.recipe = (uint8_t)(100 + k); f.quality = 2; f.servings = 1; f.day = G.day; G.foodInv.push_back(f);
    toast(std::string(STORE[k].name) + " packed in your Backpack! Eat it later or give it to someone.");
    sfxYay();
}
