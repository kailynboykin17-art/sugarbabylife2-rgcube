// See economy.hpp. All store, app and restaurant names are original.
#include "economy.hpp"
#include "lifeplus.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "goals.hpp"
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>

using namespace col;
void toast(const std::string& m);
static Economy& E() { return G.eco; }
static uint32_t nowMin() { return (uint32_t)G.day * 1440u + (uint32_t)G.minute; }
static int householdN() { int n = 0; for (const Sim& s : G.sims) if (s.household) n++; return n; }
static bool atHome() { return roomIsHome(G.room); }

void economySpend(int cat, int amount) { if (cat >= 0 && cat < EC_COUNT) E().spend[cat] += amount; }

// ================================================================ bills & utilities
static const char* BILL_NAMES[BI_COUNT] = {"Rent & Property Tax", "Electricity", "Water", "Internet", "Phone Plans", "Trash Pickup", "Subscriptions", "Home Insurance", "Late Fees"};
static const char* SUB_NAMES[4] = {"StreamFlix", "Beatbox Music", "GameZone Pass", "Home Insurance"};
static const int SUB_COST[4] = {12, 8, 10, 15};
static int billTotal(const Bill& b) { int t = 0; for (int i = 0; i < BI_COUNT; i++) t += b.item[i]; return t; }
static int phones() { int n = 0; for (const Sim& s : G.sims) if (s.household && s.stage >= ST_TEEN) n++; return n; }

void economyIssueBills(int homeItems, int people) {
    Economy& e = E();
    // last week's unpaid bill rolls over as a late fee
    int carried = 0;
    if (!e.bill.paid) carried = billTotal(e.bill) + 25;
    Bill b; b.issued = G.day; b.due = (uint16_t)(G.day + 3); b.paid = false;
    b.item[BI_RENT] = (int16_t)(std::max(20, 45 + homeItems * 3 + (G.house ? 40 : 0) + (G.houseType[G.house % 2] == 0 ? -25 : G.houseType[G.house % 2] == 2 ? 45 : 0)));
    b.item[BI_ELEC] = (int16_t)(10 + std::min<uint32_t>(e.elecUse, 400) * 0.5f);
    b.item[BI_WATER] = (int16_t)(8 + std::min<uint32_t>(e.waterUse, 400) * 0.45f);
    b.item[BI_NET] = 15;
    b.item[BI_PHONE] = (int16_t)(8 * std::max(1, phones()));
    b.item[BI_TRASH] = (int16_t)(4 + people);
    for (int i = 0; i < 3; i++) if (e.subs & (1 << i)) b.item[BI_SUBS] += SUB_COST[i];
    if (e.subs & 8) b.item[BI_INSURE] = SUB_COST[3];
    b.item[BI_LATE] = (int16_t)carried;
    e.bill = b;
    for (int i = 3; i > 0; i--) e.history[i] = e.history[i - 1];
    e.history[0] = (int16_t)billTotal(b);
    e.elecUse = e.waterUse = 0;
    char m[120];
    if (e.autopay && G.money >= billTotal(b)) {
        G.money -= billTotal(b); economySpend(EC_BILLS, billTotal(b)); e.bill.paid = true;
        snprintf(m, sizeof m, "Autopay: this week's bills ($%d) are paid. Check Wallet for the breakdown.", billTotal(b));
    } else snprintf(m, sizeof m, "Bills are in the mailbox: $%d due Wednesday. Pay them in the Wallet app!", billTotal(b));
    toast(m);
}
static void payBill() {
    Economy& e = E();
    int t = billTotal(e.bill) + ((e.powerOff || e.waterOff || e.netOff) ? 20 : 0);
    if (e.bill.paid) { toast("You're all paid up!"); return; }
    if (G.money < t) { toast("Not enough money to pay the bills!"); sfxNo(); return; }
    G.money -= t; economySpend(EC_BILLS, t); e.bill.paid = true;
    bool was = e.powerOff || e.waterOff || e.netOff;
    e.powerOff = e.waterOff = e.netOff = false;
    toast(was ? "Bills paid (+$20 reconnection fee). The lights, water & wifi are back on!" : "Bills paid! Responsible queen.");
    addMoodlet(MC(), "Bills Paid", EM_FINE, 1, 300); sfxYay();
}
bool economyUtility(bool power, bool water) {
    Economy& e = E();
    if (!atHome()) return true;
    if (power) e.elecUse++;
    if (water) e.waterUse++;
    if (power && e.powerOff) { toast("Nothing happens... the power's been cut off. Pay your bills in the Wallet app!"); sfxNo(); return false; }
    if (water && e.waterOff) { toast("No water! The water's been shut off. Pay your bills in the Wallet app!"); sfxNo(); return false; }
    return true;
}

// ================================================================ groceries
void economyGroceriesStocked(int qty, int quality) {
    Economy& e = E();
    int cap = 20 + householdN() * 6;   // the fridge only holds so much
    int before = G.groceries;
    G.groceries = (uint8_t)std::min(cap, std::min(60, G.groceries + qty));
    if (G.groceries - before < qty) toast("The fridge is FULL. Some groceries didn't fit.");
    e.groceryQ = (uint8_t)(before ? (e.groceryQ + quality + 1) / 2 : quality);
    e.groceryDay = G.day;
}
int economyGroceryStars() { return G.groceries > 0 && E().groceryQ >= 1 ? 1 : 0; }

// ================================================================ Yum Run (food delivery)
struct Dish { const char* name; int price; };
struct Rest { const char* name; const char* cat; float rating; int fee; int eta; Dish menu[3]; uint8_t quality; };
static const Rest RESTS[10] = {
    {"Burger Barn", "Burgers & Fast Food", 4.3f, 3, 30, {{"Double Smash Burger", 11}, {"Chili Cheese Fries", 6}, {"Strawberry Shake", 5}}, 1},
    {"Pizza Palace", "Pizza", 4.5f, 2, 35, {{"Large Pepperoni", 16}, {"Garlic Knots", 6}, {"Cinnamon Sticks", 6}}, 1},
    {"Taco Fiesta", "Mexican", 4.6f, 3, 30, {{"Street Tacos (3)", 10}, {"Loaded Nachos", 9}, {"Horchata", 4}}, 2},
    {"Golden Dragon", "Chinese", 4.4f, 4, 40, {{"Orange Chicken", 13}, {"Lo Mein", 11}, {"Egg Rolls (2)", 5}}, 1},
    {"Nonna's Kitchen", "Italian", 4.7f, 5, 45, {{"Chicken Alfredo", 17}, {"Lasagna", 16}, {"Tiramisu", 7}}, 2},
    {"Sunday Soul Kitchen", "Soul Food", 4.9f, 5, 45, {{"Fried Chicken Plate", 15}, {"Mac & Collards", 9}, {"Peach Cobbler", 6}}, 3},
    {"Sugar Rush", "Desserts", 4.8f, 3, 25, {{"Cupcake Box", 14}, {"Cookie Dough Cup", 7}, {"Boba Tea", 6}}, 2},
    {"Rise & Shine", "Breakfast", 4.5f, 3, 30, {{"Pancake Stack", 10}, {"Breakfast Burrito", 9}, {"Iced Latte", 5}}, 2},
    {"Cafe Latte Express", "Cafe", 4.2f, 2, 20, {{"Avocado Toast", 9}, {"Croissant", 4}, {"Caramel Frappe", 6}}, 1},
    {"Le Petit Bijou", "Fine Dining", 4.9f, 9, 60, {{"Filet & Truffle Fries", 42}, {"Lobster Mac", 34}, {"Creme Brulee", 12}}, 3},
};
static int yumRest = -1;     // menu being viewed
static uint8_t cart[3] = {0, 0, 0};
static int tipSel = 2;
static const int TIP_PCT[4] = {0, 10, 15, 20};
static int promoRest() { return (G.day * 7 + 3) % 10; }   // free delivery here today
static int subtotal() { int s = 0; for (int i = 0; i < 3; i++) s += cart[i] * RESTS[yumRest].menu[i].price; return s; }
static int feeFor(int r) { return r == promoRest() ? 0 : RESTS[r].fee; }
static int orderTotal() { int s = subtotal(); float t = s * 1.08f + s * 0.1f + feeFor(yumRest) + s * TIP_PCT[tipSel] / 100.f; if (weekday(G.day) == 4) t -= s * 0.2f; return (int)std::ceil(t); }
static const char* STATUS[8] = {"Order placed", "Restaurant confirmed", "Preparing your food", "Driver assigned", "Driver picking up food", "On the way!", "Arriving now...", "Delivered"};
static int orderStage() {
    const FoodOrder& o = E().order; if (o.rest < 0) return -1;
    float k = (nowMin() - o.placed) / (float)o.eta;
    return k < 0.05f ? 0 : k < 0.12f ? 1 : k < 0.35f ? 2 : k < 0.45f ? 3 : k < 0.6f ? 4 : k < 0.92f ? 5 : k < 1.f ? 6 : 7;
}
static void deliverFood() {
    Economy& e = E(); FoodOrder o = e.order; e.order = FoodOrder();
    const Rest& R = RESTS[o.rest];
    int portions = o.qty[0] + o.qty[1] + o.qty[2];
    bool cold = o.tip == 0 && irand(100) < 30;
    if (!atHome()) { toast(std::string("Your ") + R.name + " order was left at the door. It'll be in the fridge."); G.groceries = (uint8_t)std::min(60, G.groceries + portions); return; }
    int fed = 0;
    for (Actor& a : curRoom().actors) if (a.kind == 0 && a.id < G.sims.size() && fed < portions * 2) {
        Sim& s = G.sims[a.id]; if (s.stage < ST_TODDLER) continue;
        s.needs[N_HUNGER] = std::min(100.f, s.needs[N_HUNGER] + 60);
        addMoodlet(s, cold ? "Cold Takeout" : R.quality >= 3 ? "Restaurant-Quality Feast" : "Takeout Treat", cold ? EM_TENSE : EM_HAPPY, cold ? 1 : R.quality >= 3 ? 2 : 1, 180);
        if (a.id != G.mc) { a.say = cold ? "Ugh, it's cold..." : "Yesss, " + std::string(R.name) + "!"; a.sayT = 3; }
        fed++;
    }
    int left = std::max(0, portions - (fed + 1) / 2);
    if (left) G.groceries = (uint8_t)std::min(60, G.groceries + left);
    char b[140]; snprintf(b, sizeof b, "Ding dong! %s is here.%s%s", R.name, cold ? " ...It's cold. Should've tipped." : " Still hot!", left ? " Leftovers went in the fridge." : "");
    toast(b); sfxYay();
}

// ================================================================ ShopNow (online shopping)
enum Eff : uint8_t { FX_GADGET, FX_OUTFIT, FX_GROC, FX_MAKEUP, FX_TOYS, FX_FLOWERS, FX_SELFCARE, FX_MYSTERY, FX_FUNALL, FX_SKILL, FX_PET, FX_SCHOOL, FX_MOOD, FX_DECOR };
struct Item { const char* name; int price; float rating; uint8_t fx, arg; const char* desc; };
struct Cat { const char* name; Item items[4]; };
static const Cat CATS[11] = {
    {"Groceries", {{"Budget Box", 35, 3.6f, FX_GROC, 0, "14 meals. Cheap... and it shows."}, {"Standard Box", 55, 4.3f, FX_GROC, 1, "14 meals. Fresh = +1 cooking star."},
                   {"Premium Box", 80, 4.7f, FX_GROC, 2, "14 meals, top quality, lasts longer."}, {"Organic Box", 95, 4.8f, FX_GROC, 3, "14 meals, farm-fresh & organic."}}},
    {"Fashion", {{"Cute Fit (random)", 35, 4.4f, FX_OUTFIT, 0, "A brand-new look for your closet"}, {"Designer-Inspired Fit", 70, 4.6f, FX_OUTFIT, 1, "Two new looks in one box"}, {"Statement Handbag", 45, 4.5f, FX_MOOD, 0, "Instant confidence"}, {"Platform Sneakers", 40, 4.2f, FX_MOOD, 0, "Walk taller"}}},
    {"Beauty", {{"Makeup Haul", 22, 4.5f, FX_MAKEUP, 0, "New lip, blush & shadow"}, {"Skincare Set", 28, 4.7f, FX_SELFCARE, 0, "Glowing skin, clean & fresh"}, {"Signature Perfume", 38, 4.6f, FX_MOOD, 1, "Smells like main character"}, {"Self-Care Kit", 18, 4.4f, FX_SELFCARE, 0, "Bath bomb + face mask"}}},
    {"Electronics", {{"SugarPod Music Player", 70, 4.7f, FX_GADGET, 1, "Y2K vibes, your whole playlist"}, {"Laptop", 450, 4.6f, FX_GADGET, 2, "Blog, binge, homework"}, {"Big-Screen TV", 340, 4.4f, FX_FUNALL, 1, "Movie nights at home hit different"}, {"E-Reader", 90, 4.8f, FX_GADGET, 0, "Read anywhere you sit or lie down"}}},
    {"Home", {{"Cozy Throw Blanket", 25, 4.8f, FX_DECOR, 0, "Extra-comfy evenings"}, {"Fairy String Lights", 18, 4.6f, FX_DECOR, 1, "Aesthetic bedroom unlocked"}, {"Air Fryer", 75, 4.7f, FX_SKILL, SK_COOKING, "Cooking gets easier"}, {"Robot Vacuum", 150, 4.3f, FX_DECOR, 2, "Keeps the house clean"}}},
    {"Gaming", {{"Gaming Console", 120, 4.7f, FX_FUNALL, 0, "Fun for the whole house"}, {"New Game", 40, 4.5f, FX_FUNALL, 2, "The one everyone's talking about"}, {"Pro Controller", 35, 4.4f, FX_MOOD, 3, "Gamers love it"}, {"Gaming Chair", 90, 4.2f, FX_MOOD, 4, "Back support & RGB"}}},
    {"Books", {{"Romance Novel", 12, 4.6f, FX_SKILL, SK_CREATIVITY, "Swoon-worthy & inspiring"}, {"Mystery Thriller", 14, 4.5f, FX_SKILL, SK_LOGIC, "Can't put it down"}, {"Cookbook", 20, 4.7f, FX_SKILL, SK_COOKING, "Level up in the kitchen"}, {"Fashion Magazine", 6, 4.1f, FX_SKILL, SK_STYLE, "This month's trends"}}},
    {"Toys", {{"Toy Bundle", 25, 4.6f, FX_TOYS, 0, "Fun for every kid at home"}, {"Fashion Doll Set", 30, 4.7f, FX_TOYS, 0, "Comes with 3 outfits"}, {"Plush Bunny", 15, 4.9f, FX_TOYS, 0, "The softest thing ever"}, {"Mystery Collectible", 10, 4.0f, FX_MYSTERY, 0, "Could be rare!"}}},
    {"Pets", {{"Gourmet Pet Food", 20, 4.6f, FX_PET, 0, "Happy, healthy pets"}, {"Pet Toy Pack", 15, 4.5f, FX_PET, 1, "Hours of play"}, {"Cozy Pet Bed", 35, 4.8f, FX_PET, 2, "Naptime perfection"}, {"Pet Outfit", 18, 4.3f, FX_PET, 3, "Too cute"}}},
    {"School", {{"Cute Backpack", 30, 4.6f, FX_SCHOOL, 0, "Ready for the semester"}, {"Notebook & Gel Pens", 10, 4.7f, FX_SCHOOL, 1, "Homework is fun now?"}, {"Graphing Calculator", 45, 4.4f, FX_SKILL, SK_LOGIC, "Math, but easier"}, {"Planner", 12, 4.5f, FX_SCHOOL, 2, "Organized era"}}},
    {"Lifestyle", {{"Scented Candles", 16, 4.7f, FX_MOOD, 5, "Vanilla cupcake scent"}, {"Flowers for Bae", 20, 4.8f, FX_FLOWERS, 0, "Sent to your partner"}, {"Desk Organizer", 14, 4.3f, FX_DECOR, 3, "Clean desk, clear mind"}, {"Mystery Box", 30, 4.1f, FX_MYSTERY, 0, "Could be anything..."}}},
};
static int shopCat = 0, shopItem = 0;
static uint8_t pack(int c, int i) { return (uint8_t)(c * 4 + i); }
static void openPackage(uint8_t code) {
    int c = code / 4, i = code % 4; if (c >= 11) return;
    const Item& it = CATS[c].items[i];
    Sim& me = MC();
    int fx = it.fx, arg = it.arg;
    if (fx == FX_MYSTERY) { static const uint8_t M[] = {FX_OUTFIT, FX_MAKEUP, FX_SELFCARE, FX_TOYS, FX_MOOD}; fx = M[irand(5)]; arg = 0; }
    std::string msg = std::string("Unboxed: ") + it.name + "! ";
    switch (fx) {
    case FX_GADGET: G.gadgets |= (uint8_t)(1 << arg); msg += arg == 0 ? "Read on it from any bed, couch or chair!" : arg == 1 ? "Listen to music from any bed, couch or chair!" : "Use it from any bed, couch, chair or desk!"; break;
    case FX_OUTFIT: { int got = 0, last = -1; for (int k = 0; k < 60 && got < (arg ? 2 : 1); k++) { int o = 32 + irand(FIRST_KID_OUTFIT - 32); if (!ownsOutfit(o)) { giveOutfit(o); got++; last = o; } } msg += last >= 0 ? std::string("New look: ") + OUTFITS[last].name + "." : "(You already own it.)"; break; }
    case FX_GROC: economyGroceriesStocked(14, arg); msg += "Groceries unpacked into the fridge."; if (arg == 3) addMoodlet(me, "Organic Ingredients", EM_FINE, 1, 600); break;
    case FX_MAKEUP: { static const RGB L[] = {{255, 150, 190}, {220, 70, 120}, {230, 160, 150}}; me.look.lips = L[irand(3)]; me.look.f.blush = (uint8_t)(1 + irand(3)); me.look.f.shadow = (uint8_t)(1 + irand(6)); msg += "New makeup look applied."; goalNotify(GE_MAKEUP); break; }
    case FX_TOYS: { int n = 0; for (Sim& s : G.sims) if (s.household && s.stage <= ST_CHILD) { s.needs[N_FUN] = 100; addMoodlet(s, "New Toys!", EM_HAPPY, 2, 600); n++; } msg += n ? "The kids are THRILLED." : "For you? Sure!"; break; }
    case FX_FLOWERS: { int p = partnerOf(G.mc); if (p >= 0) { changeRel(G.mc, p, 3, 10); addSentiment(p, SN_GRATEFUL, 6); msg += G.sims[p].name + " LOVES them."; } else { addMoodlet(me, "Flowers for Me", EM_HAPPY, 1, 480); msg += "Self-love!"; } break; }
    case FX_SELFCARE: addMoodlet(me, "Self-Care Day", EM_FINE, 2, 600); me.needs[N_HYGIENE] = 100; msg += "Glowing!"; break;
    case FX_FUNALL: for (Sim& s : G.sims) if (s.household) { s.needs[N_FUN] = std::min(100.f, s.needs[N_FUN] + 35); addMoodlet(s, arg == 1 ? "Big-Screen Movie Night" : "New Game Hype", EM_PLAYFUL, 2, 720); } msg += "Fun for the whole house."; break;
    case FX_SKILL: skillGain(me, arg, 6); addMoodlet(me, "Learning Something New", EM_INSPIRED, 1, 300); msg += std::string(SKILL_NAMES[arg]) + " went up!"; break;
    case FX_PET: if (G.pets.empty()) msg += "...but you don't have a pet yet. Cute though!"; else { addMoodlet(me, "Spoiled My Pet", EM_HAPPY, 1, 600); msg += "Your pets are SO happy."; } break;
    case FX_SCHOOL: { int n = 0; for (Sim& s : G.sims) if (s.household && s.stage >= ST_CHILD && s.stage <= ST_TEEN) { addMoodlet(s, "School Supplies Glow-Up", EM_CONFIDENT, 1, 1440); n++; } msg += n ? "Back-to-school ready!" : "Very organized of you."; break; }
    case FX_DECOR: for (Sim& s : G.sims) if (s.household) addMoodlet(s, arg == 1 ? "Aesthetic Room" : arg == 2 ? "Clean Floors" : "Cozy Home", EM_FINE, 1, 1440); msg += "The house feels nicer."; break;
    case FX_MOOD: default: addMoodlet(me, "New Purchase High", EM_CONFIDENT, 2, 480); msg += "Obsessed."; break;
    }
    toast(msg);
}
static std::string mDoorbell(int ch, int) {
    Economy& e = E();
    if (ch != 1) { int n = (int)e.porch.size(); for (uint8_t p : e.porch) openPackage(p); e.porch.clear(); sfxYay(); return n > 1 ? "You tore open every box. Unboxing ASMR." : "You ripped it open right there on the porch."; }
    return "You left it on the porch for later. (Open it in ShopNow.)";
}

// ================================================================ hourly / daily
void economyHourly(int hour) {
    if (G.sims.empty()) return;
    Economy& e = E();
    // packages in transit arrive on the porch
    for (size_t i = 0; i < G.packages.size();) {
        const Package& p = G.packages[i];
        if (G.day > p.day || (G.day == p.day && hour >= p.hour)) {
            uint8_t code = p.kind; G.packages.erase(G.packages.begin() + i);
            if (hasLot(LT_PORCH) && irand(100) < 30) { toast("Your package got stolen off the porch! Ugh, porch pirates."); addMoodlet(MC(), "Package Stolen", EM_ANGRY, 2, 480); continue; }
            e.porch.push_back(code);
            continue;
        }
        i++;
    }
    if (!e.porch.empty() && atHome() && !momentActive() && hour >= 8 && hour <= 21) {
        char b[96]; snprintf(b, sizeof b, "Ding dong! %d package%s on the porch.", (int)e.porch.size(), e.porch.size() > 1 ? "s are" : " is");
        momentCustom("Package Delivered!", b, "Open it now!", "Leave it for later", "Open them all!", mDoorbell, 0, -1);
    }
    if (e.porch.size() >= 5 && hour == 18) { toast("The porch is buried in boxes. Your family is NOT happy about it."); for (Sim& s : G.sims) if (s.household && &s != &MC()) addMoodlet(s, "Cluttered Porch", EM_TENSE, 1, 300); }
    // food delivery
    if (e.order.rest >= 0 && orderStage() >= 7) deliverFood();
    else if (e.order.rest >= 0 && orderStage() == 5 && hour % 1 == 0) {}
    // shopping habits: impulse buyers in the household
    for (size_t i = 0; i < G.sims.size(); i++) {
        Sim& s = G.sims[i]; if (!s.household || (int)i == G.mc || s.stage < ST_TEEN || irand(60)) continue;
        if (s.hasTrait(TR_FASHIONISTA) || s.hasTrait(TR_GLAMOROUS) || s.hasTrait(TR_SHOPAHOLIC)) { int c = 25 + irand(50); G.money = std::max<int32_t>(0, G.money - c); economySpend(EC_SHOPPING, c); toast(s.name + " ordered ANOTHER handbag online... -$" + std::to_string(c)); if (partnerOf((int)i) == G.mc) changeRel(G.mc, (int)i, -1, 0); }
        else if (s.hasTrait(TR_GAMER)) { int c = 40; G.money = std::max<int32_t>(0, G.money - c); economySpend(EC_SHOPPING, c); toast(s.name + " bought a new game. -$40"); }
        else if (s.hasTrait(TR_LAZY) && s.needs[N_HUNGER] < 45 && G.groceries == 0) { int c = 18; G.money = std::max<int32_t>(0, G.money - c); economySpend(EC_TAKEOUT, c); s.needs[N_HUNGER] = 90; toast(s.name + " didn't feel like cooking and ordered takeout. -$18"); }
    }
    if (MC().hasTrait(TR_SHOPAHOLIC) && hour >= 10 && hour <= 22 && irand(30) == 0 && G.money > 60 && G.packages.size() < 12) {
        int c = 1 + irand(10); int i = irand(4); const Item& it = CATS[c].items[i];
        G.money -= it.price; economySpend(EC_SHOPPING, it.price);
        Package p; p.kind = pack(c, i); p.day = (uint16_t)(G.day + 1); p.hour = 10; G.packages.push_back(p);
        addMoodlet(MC(), "Add-to-Cart Rush", EM_PLAYFUL, 1, 180);
        toast(std::string("Oops... you impulse-bought ") + it.name + " (-$" + std::to_string(it.price) + "). Shopaholic things.");
    }
    if (hour == 9 && G.groceries > 0 && G.groceries <= 3) toast("Running low on groceries! Restock at Sugar Market or ShopNow.");
}
void economyDaily() {
    Economy& e = E();
    if (e.weekStartMoney < 0) e.weekStartMoney = G.money;
    // late bills: fees, then services get cut
    if (!e.bill.paid && G.day > e.bill.due) {
        int late = G.day - e.bill.due;
        e.bill.item[BI_LATE] = (int16_t)(e.bill.item[BI_LATE] + 10);
        if (late == 1) toast("Your bills are overdue! A $10 late fee was added. Pay in the Wallet app.");
        if (late >= 2 && !e.powerOff) { e.powerOff = true; toast("The POWER has been shut off for unpaid bills!"); }
        if (late >= 3 && !e.waterOff) { e.waterOff = true; toast("The WATER has been shut off too!"); }
        if (late >= 4 && !e.netOff) { e.netOff = true; toast("And now the wifi is gone. No internet!"); }
        for (Sim& s : G.sims) if (s.household) addMoodlet(s, e.powerOff ? "No Power" : "Bill Stress", EM_TENSE, e.powerOff ? 2 : 1, 1440);
    }
    // groceries go bad (premium lasts longer)
    int life = e.groceryQ >= 2 ? 10 : 7;
    if (G.groceries > 0 && G.day - e.groceryDay >= life) { int lost = (G.groceries + 1) / 2; G.groceries = (uint8_t)(G.groceries - lost); e.groceryDay = G.day; toast("Ew, " + std::to_string(lost) + " meals' worth of groceries went bad in the fridge."); }
    // weekly budget roll-over (Sunday) + recurring grocery order (Saturday)
    if (weekday(G.day) == 6) {
        e.spend[EC_INCOME] = std::max<int32_t>(0, G.money - e.weekStartMoney + e.spend[EC_BILLS] + e.spend[EC_GROCERY] + e.spend[EC_TAKEOUT] + e.spend[EC_SHOPPING] + e.spend[EC_FUN]);
        for (int i = 0; i < EC_COUNT; i++) { e.lastSpend[i] = e.spend[i]; e.spend[i] = 0; }
        e.weekStartMoney = G.money; e.ordersWeek = 0;
    }
    if (weekday(G.day) == 5 && e.autoGrocery) {
        if (G.money >= 55 && G.packages.size() < 12) { G.money -= 55; economySpend(EC_GROCERY, 55); Package p; p.kind = pack(0, 1); p.day = G.day; p.hour = 11; G.packages.push_back(p); toast("Your weekly grocery order is on the way (-$55)."); }
        else toast("Your weekly grocery order failed: not enough money!");
    }
}

// ================================================================ Wallet app
static int walletTab = 0;
static Btn tabB(int i) { return Btn{4 + i * 105.f, 24, 102, 22}; }
void walletAppUpdate(const Input& in) {
    if (!in.touch) return;
    for (int i = 0; i < 3; i++) if (tabB(i).in(in.tp)) { walletTab = i; sfxTap(); return; }
    Economy& e = E();
    if (walletTab == 0) {
        if (Btn{84, 210, 112, 26}.in(in.tp)) payBill();
        if (Btn{200, 210, 116, 26}.in(in.tp)) { e.autopay = !e.autopay; toast(e.autopay ? "Autopay ON: bills pay themselves on Sunday if you can afford it." : "Autopay off."); sfxTap(); }
    } else if (walletTab == 2) {
        for (int i = 0; i < 4; i++) if (Btn{6, 56 + i * 32.f, 308, 28}.in(in.tp)) { e.subs ^= (uint8_t)(1 << i); toast(std::string(SUB_NAMES[i]) + ((e.subs >> i) & 1 ? " subscribed! It'll be on next week's bill." : " canceled.")); sfxTap(); }
        if (Btn{6, 180, 308, 24}.in(in.tp)) { e.autoGrocery = !e.autoGrocery; toast(e.autoGrocery ? "Weekly groceries ON: a Standard Box arrives every Saturday ($55)." : "Weekly groceries off."); sfxTap(); }
    }
}
void walletAppDrawTop(float) {
    Economy& e = E();
    grad(0, 0, 400, 240, C(236, 250, 240), C(214, 236, 226));
    textOutline("Wallet", 200, 8, 0.7f, C(255, 255, 255), C(40, 140, 100), 1, 1.2f);
    char b[96]; snprintf(b, sizeof b, "Balance: $%d", G.money); text(b, 200, 36, 0.6f, C(30, 100, 70), 1);
    int status = !e.bill.paid ? (G.day > e.bill.due ? 2 : 1) : 0;
    snprintf(b, sizeof b, status == 0 ? "All bills paid" : status == 1 ? "Bill due in %d day(s)" : "OVERDUE by %d day(s)!", status == 2 ? G.day - e.bill.due : e.bill.due - G.day);
    text(b, 200, 58, 0.5f, status == 2 ? C(220, 40, 40) : status == 1 ? C(220, 140, 30) : C(40, 140, 100), 1);
    std::string svc = std::string("Power: ") + (e.powerOff ? "OFF" : "on") + "   Water: " + (e.waterOff ? "OFF" : "on") + "   Wifi: " + (e.netOff ? "OFF" : "on");
    text(svc.c_str(), 200, 78, 0.42f, e.powerOff || e.waterOff || e.netOff ? C(220, 40, 40) : C(UI1), 1);
    snprintf(b, sizeof b, "This week's usage: %u power, %u water", (unsigned)e.elecUse, (unsigned)e.waterUse); text(b, 200, 96, 0.4f, C(UI1), 1);
    text("Past bills:", 60, 120, 0.42f, C(UI1));
    for (int i = 0; i < 4; i++) { float h = std::min(80.f, e.history[i] / 6.f); rect(150 + i * 50.f, 200 - h, 30, h, C(i ? RGB{150, 200, 180} : RGB{40, 140, 100})); snprintf(b, sizeof b, "$%d", e.history[i]); text(b, 165 + i * 50.f, 204, 0.36f, C(UI1), 1); }
    text("this week", 165, 218, 0.32f, C(UI1), 1);
}
void walletAppDrawBottom(float) {
    Economy& e = E();
    static const char* T[3] = {"Bills", "Budget", "Subscriptions"};
    for (int i = 0; i < 3; i++) button(tabB(i), T[i], walletTab == i, walletTab == i ? 0 : 2, 0.42f);
    char b[80];
    if (walletTab == 0) {
        for (int i = 0; i < BI_COUNT; i++) { snprintf(b, sizeof b, "%-20s $%d", BILL_NAMES[i], e.bill.item[i]); text(BILL_NAMES[i], 14, 52 + i * 16.f, 0.4f, C(CREAM)); snprintf(b, sizeof b, "$%d", e.bill.item[i]); text(b, 300, 52 + i * 16.f, 0.4f, C(CREAM), 2); }
        snprintf(b, sizeof b, "TOTAL  $%d%s", billTotal(e.bill), e.bill.paid ? "  (paid)" : ""); text(b, 160, 196, 0.5f, C(GOLD), 1);
        button(Btn{84, 210, 112, 26}, e.bill.paid ? "Paid!" : "Pay Bills", !e.bill.paid, 0, 0.45f);
        button(Btn{200, 210, 116, 26}, e.autopay ? "Autopay: ON" : "Autopay: OFF", e.autopay, 3, 0.42f);
    } else if (walletTab == 1) {
        static const char* N[EC_COUNT] = {"Bills", "Groceries", "Takeout", "Shopping", "Fun", "Income"};
        text("This week              Last week", 160, 52, 0.4f, C(CHAMP), 1);
        for (int i = 0; i < EC_COUNT; i++) {
            float y = 70 + i * 22.f; text(N[i], 14, y, 0.42f, C(CREAM));
            float w1 = std::min(90.f, (i == EC_INCOME ? e.lastSpend[i] : e.spend[i]) / 5.f), w2 = std::min(90.f, e.lastSpend[i] / 5.f);
            if (i != EC_INCOME) { rect(90, y + 2, w1, 10, C(i == EC_TAKEOUT || i == EC_SHOPPING ? RGB{255, 140, 160} : RGB{140, 220, 180})); snprintf(b, sizeof b, "$%d", e.spend[i]); text(b, 186, y, 0.38f, C(CREAM)); }
            rect(220, y + 2, w2 * 0.8f, 10, C(200, 200, 220)); snprintf(b, sizeof b, "$%d", e.lastSpend[i]); text(b, 312, y, 0.38f, C(CREAM), 2);
        }
        int out = e.spend[EC_BILLS] + e.spend[EC_GROCERY] + e.spend[EC_TAKEOUT] + e.spend[EC_SHOPPING] + e.spend[EC_FUN];
        snprintf(b, sizeof b, "Spent this week: $%d", out); text(b, 160, 206, 0.45f, C(GOLD), 1);
        if (e.spend[EC_TAKEOUT] > e.spend[EC_GROCERY] + 40) text("Tip: groceries are way cheaper than takeout!", 160, 222, 0.36f, C(255, 200, 200), 1);
    } else {
        for (int i = 0; i < 4; i++) { snprintf(b, sizeof b, "%s  $%d/wk  %s", SUB_NAMES[i], SUB_COST[i], (e.subs >> i) & 1 ? "(on)" : ""); button(Btn{6, 56 + i * 32.f, 308, 28}, b, (e.subs >> i) & 1, (e.subs >> i) & 1 ? 0 : 2, 0.42f); }
        text("Insurance pays for gremlin repairs.", 160, 186 - 22, 0.36f, C(CREAM), 1);
        button(Btn{6, 180, 308, 24}, e.autoGrocery ? "Weekly Groceries: ON ($55, Sat)" : "Weekly Groceries: OFF", e.autoGrocery, 3, 0.42f);
    }
}

// ================================================================ Yum Run app
void yumAppUpdate(const Input& in) {
    if (!in.touch) return;
    Economy& e = E();
    const touchPosition& tp = in.tp;
    if (yumRest < 0) {
        for (int i = 0; i < 10; i++) if (Btn{4 + (i % 2) * 158.f, 26 + (i / 2) * 36.f, 154, 33}.in(tp)) {
            if (e.netOff) { toast("No wifi! Pay your internet bill first."); sfxNo(); return; }
            yumRest = i; cart[0] = cart[1] = cart[2] = 0; sfxTap(); return;
        }
        return;
    }
    for (int i = 0; i < 3; i++) {
        if (Btn{228, 30 + i * 34.f, 40, 28}.in(tp)) { if (cart[i]) cart[i]--; sfxTap(); return; }
        if (Btn{272, 30 + i * 34.f, 40, 28}.in(tp)) { if (cart[i] < 9) cart[i]++; sfxTap(); return; }
    }
    for (int i = 0; i < 4; i++) if (Btn{60 + i * 64.f, 140, 60, 24}.in(tp)) { tipSel = i; sfxTap(); return; }
    if (Btn{84, 210, 100, 26}.in(tp)) { yumRest = -1; sfxTap(); return; }
    if (Btn{188, 210, 128, 26}.in(tp)) {
        if (e.order.rest >= 0) { toast("You already have an order on the way!"); sfxNo(); return; }
        if (subtotal() < 10) { toast("Minimum order is $10!"); sfxNo(); return; }
        int t = orderTotal(); if (MC().hasTrait(TR_FRUGAL)) t = t * 85 / 100;
        if (G.money < t) { toast("Not enough money!"); sfxNo(); return; }
        G.money -= t; economySpend(EC_TAKEOUT, t);
        FoodOrder o; o.rest = (int8_t)yumRest; for (int i = 0; i < 3; i++) o.qty[i] = cart[i]; o.placed = nowMin(); o.eta = (uint8_t)(RESTS[yumRest].eta + irand(10) - (TIP_PCT[tipSel] >= 15 ? 5 : 0)); o.tip = (uint8_t)TIP_PCT[tipSel]; o.total = (int16_t)t;
        e.order = o; e.ordersWeek++;
        if (e.ordersWeek == 4) toast("4 takeout orders this week... your wallet is crying.");
        char b[96]; snprintf(b, sizeof b, "Order placed at %s! Arriving in about %d min.", RESTS[yumRest].name, o.eta); toast(b); sfxYay();
        yumRest = -1;
    }
}
void yumAppDrawTop(float t) {
    Economy& e = E();
    grad(0, 0, 400, 240, C(255, 244, 236), C(255, 226, 214));
    rrect(120, 8, 160, 36, 12, C(240, 60, 90)); textOutline("Yum Run", 200, 14, 0.7f, C(255, 255, 255), C(150, 20, 50), 1, 1.2f);
    if (e.order.rest >= 0) {
        int st = orderStage(); const Rest& R = RESTS[e.order.rest];
        char b[96]; snprintf(b, sizeof b, "Your order from %s", R.name); text(b, 200, 54, 0.5f, C(UI1), 1);
        for (int i = 0; i < 7; i++) { float y = 76 + i * 18.f; circle(60, y + 6, 6, i <= st ? C(240, 60, 90) : C(220, 200, 200)); if (i < 6) rect(59, y + 12, 2, 12, C(220, 200, 200)); text(STATUS[i], 76, y, 0.42f, i == st ? C(240, 60, 90) : i < st ? C(UI1) : C(170, 150, 150)); }
        int left = std::max(0, (int)e.order.eta - (int)(nowMin() - e.order.placed)); snprintf(b, sizeof b, "ETA: %d min", left); text(b, 320, 80, 0.55f, C(UI1), 1);
        float k = std::min(1.f, (nowMin() - e.order.placed) / (float)e.order.eta);
        float sx = 240 + k * 130; rrect(sx - 12, 150, 24, 14, 4, C(240, 60, 90)); circle(sx - 7, 166, 4, C(UI1)); circle(sx + 7, 166, 4, C(UI1)); (void)t;
        rect(240, 170, 140, 2, C(200, 180, 180)); tri(378, 140, 392, 150, 378, 160, C(255, 200, 120));
    } else if (yumRest >= 0) {
        const Rest& R = RESTS[yumRest];
        text(R.name, 200, 54, 0.6f, C(UI1), 1);
        char b[120]; snprintf(b, sizeof b, "%s  -  %.1f stars  -  %d-%d min", R.cat, R.rating, R.eta, R.eta + 10); text(b, 200, 76, 0.42f, C(UI1), 1);
        int s = subtotal(); float tip = s * TIP_PCT[tipSel] / 100.f;
        snprintf(b, sizeof b, "Subtotal $%d   Tax $%.0f   Service $%.0f", s, s * 0.08f, s * 0.1f); text(b, 200, 110, 0.42f, C(UI1), 1);
        snprintf(b, sizeof b, "Delivery $%d%s   Tip $%.0f%s", feeFor(yumRest), yumRest == promoRest() ? " (FREE today!)" : "", tip, weekday(G.day) == 4 ? "   Friday: 20% off!" : ""); text(b, 200, 126, 0.42f, C(UI1), 1);
        snprintf(b, sizeof b, "TOTAL: $%d", s ? orderTotal() : 0); text(b, 200, 150, 0.6f, C(240, 60, 90), 1);
        if (TIP_PCT[tipSel] == 0) text("No tip... your food might show up cold.", 200, 176, 0.4f, C(200, 80, 80), 1);
    } else {
        text("Hungry? Pick a restaurant!", 200, 56, 0.5f, C(UI1), 1);
        char b[96]; snprintf(b, sizeof b, "Today's deal: FREE delivery from %s", RESTS[promoRest()].name); text(b, 200, 80, 0.42f, C(240, 60, 90), 1);
        if (weekday(G.day) == 4) text("It's Friday: 20% off everything!", 200, 98, 0.42f, C(240, 60, 90), 1);
        text("Cooking at home is cheaper - but sometimes you just need fries.", 200, 210, 0.38f, C(UI1), 1);
    }
}
void yumAppDrawBottom(float) {
    char b[80];
    if (yumRest < 0) {
        for (int i = 0; i < 10; i++) {
            Btn bb{4 + (i % 2) * 158.f, 26 + (i / 2) * 36.f, 154, 33};
            button(bb, RESTS[i].name, i == promoRest(), i == promoRest() ? 0 : 2, 0.4f);
            snprintf(b, sizeof b, "%.1f* %s $%d", RESTS[i].rating, RESTS[i].cat, feeFor(i)); text(b, bb.x + bb.w / 2, bb.y + 20, 0.3f, C(255, 255, 255, 220), 1);
        }
        return;
    }
    const Rest& R = RESTS[yumRest];
    for (int i = 0; i < 3; i++) {
        float y = 30 + i * 34.f;
        rrect(6, y, 216, 28, 6, C(255, 255, 255, 220)); snprintf(b, sizeof b, "%s  $%d", R.menu[i].name, R.menu[i].price); text(b, 12, y + 7, 0.4f, C(UI1));
        button(Btn{228, y, 40, 28}, "-", false, 1, 0.55f); button(Btn{272, y, 40, 28}, "+", false, 0, 0.55f);
        snprintf(b, sizeof b, "x%d", cart[i]); text(b, 206, y + 7, 0.42f, C(240, 60, 90), 2);
    }
    text("Tip:", 30, 144, 0.42f, C(CREAM));
    for (int i = 0; i < 4; i++) { snprintf(b, sizeof b, "%d%%", TIP_PCT[i]); button(Btn{60 + i * 64.f, 140, 60, 24}, b, tipSel == i, tipSel == i ? 0 : 2, 0.42f); }
    button(Btn{84, 210, 100, 26}, "< Restaurants", false, 1, 0.38f);
    snprintf(b, sizeof b, "Order ($%d)", subtotal() ? orderTotal() : 0); button(Btn{188, 210, 128, 26}, b, subtotal() >= 10, 0, 0.42f);
}

// ================================================================ ShopNow app
static Btn catB(int i) { return Btn{4 + (i % 6) * 52.5f, 24 + (i / 6) * 22.f, 50, 20}; }
static Btn itemB(int i) { return Btn{6, 72 + i * 30.f, 308, 27}; }
static const Btn bBuy{84, 210, 92, 26}, bExpress{180, 210, 88, 26}, bPorch{272, 210, 44, 26};
void shopAppUpdate(const Input& in) {
    if (!in.touch) return;
    Economy& e = E();
    for (int i = 0; i < 11; i++) if (catB(i).in(in.tp)) { shopCat = i; shopItem = 0; sfxTap(); return; }
    for (int i = 0; i < 4; i++) if (itemB(i).in(in.tp)) { shopItem = i; sfxTap(); return; }
    if (bPorch.in(in.tp)) { if (e.porch.empty()) { toast("Nothing on the porch."); return; } for (uint8_t p : e.porch) openPackage(p); e.porch.clear(); sfxYay(); return; }
    bool exp = bExpress.in(in.tp);
    if (bBuy.in(in.tp) || exp) {
        if (e.netOff) { toast("No wifi! Pay your internet bill first."); sfxNo(); return; }
        const Item& it = CATS[shopCat].items[shopItem];
        int price = it.price + (exp ? 10 : 0);
        if (MC().hasTrait(TR_FRUGAL)) price = price * 85 / 100;
        if (G.money < price) { toast("Not enough money!"); sfxNo(); return; }
        if (G.packages.size() >= 12) { toast("Too many packages on the way!"); sfxNo(); return; }
        G.money -= price; economySpend(shopCat == 0 ? EC_GROCERY : EC_SHOPPING, price);
        Package p; p.kind = pack(shopCat, shopItem);
        int nowH = (int)(G.minute / 60);
        if (exp) { int h = nowH + 2; p.day = (uint16_t)(G.day + (h >= 24 ? 1 : 0)); p.hour = (uint8_t)(h % 24); } else { p.day = (uint16_t)(G.day + 1); p.hour = 10; }
        G.packages.push_back(p);
        toast(std::string("Ordered ") + it.name + (exp ? "! Same-day: 2 hours." : "! Arrives tomorrow, 10 AM."));
        int recent = (int)G.packages.size() + (int)e.porch.size();
        if (recent >= 5) { addMoodlet(MC(), "Shopaholic Era", EM_PLAYFUL, 1, 600); toast("That's a LOT of packages... shopaholic era?"); }
        sfxYay();
    }
}
void shopAppDrawTop(float t) {
    Economy& e = E();
    grad(0, 0, 400, 240, C(255, 240, 230), C(255, 214, 200));
    rrect(110, 8, 180, 36, 12, C(255, 120, 60)); textOutline("ShopNow", 200, 14, 0.7f, C(255, 255, 255), C(150, 50, 20), 1, 1.2f);
    const Item& it = CATS[shopCat].items[shopItem];
    text(it.name, 200, 54, 0.6f, C(200, 60, 40), 1);
    char b[96]; snprintf(b, sizeof b, "$%d   -   %.1f stars   -   %s", it.price, it.rating, CATS[shopCat].name); text(b, 200, 76, 0.42f, C(UI1), 1);
    text(it.desc, 200, 94, 0.45f, C(UI1), 1);
    for (int i = 0; i < 5; i++) sparkle(170 + i * 15.f, 116, 5, i < (int)(it.rating + 0.5f) ? C(255, 190, 40) : C(220, 200, 190));
    float bob = std::sin(t * 3) * 3; rrect(176, 128 + bob, 48, 34, 4, C(200, 150, 90)); rect(176, 140 + bob, 48, 5, C(230, 190, 120)); rect(197, 128 + bob, 6, 34, C(230, 190, 120));
    text("Standard: tomorrow 10 AM   Same-day: +$10, 2 hrs", 200, 170, 0.38f, C(UI1), 1);
    snprintf(b, sizeof b, "In transit: %d     On your porch: %d", (int)G.packages.size(), (int)e.porch.size()); text(b, 200, 188, 0.45f, C(UI1), 1);
    if (shopCat == 0) { snprintf(b, sizeof b, "Fridge: %d meals  (holds %d)", G.groceries, 20 + householdN() * 6); text(b, 200, 206, 0.4f, C(40, 140, 100), 1); }
    if (hasLot(LT_PORCH)) text("Warning: porch pirates in your area!", 200, 222, 0.38f, C(220, 40, 40), 1);
}
void shopAppDrawBottom(float) {
    for (int i = 0; i < 11; i++) button(catB(i), CATS[i].name, shopCat == i, shopCat == i ? 0 : 3, 0.28f);
    for (int i = 0; i < 4; i++) { const Item& it = CATS[shopCat].items[i]; char b[64]; snprintf(b, sizeof b, "%s   $%d", it.name, it.price); button(itemB(i), b, shopItem == i, shopItem == i ? 0 : 2, 0.4f); }
    char b[32]; snprintf(b, sizeof b, "Order $%d", CATS[shopCat].items[shopItem].price); button(bBuy, b, false, 0, 0.4f);
    button(bExpress, "Same-Day", false, 3, 0.4f);
    snprintf(b, sizeof b, "%d", (int)E().porch.size()); button(bPorch, b, !E().porch.empty(), 1, 0.45f);
}
