#include "oddjobs.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include "lifeext.hpp"
#include "levels.hpp"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <algorithm>

using namespace col;

enum { J_NANNY, J_GROCERY, J_BARISTA, J_SERVER, J_FLORIST, J_GROOMER, J_TEACHER, J_COUNT };
static const char* JN[J_COUNT] = {"Nanny", "Grocery Scanner", "Barista", "Restaurant Server", "Florist", "Pet Groomer", "Substitute Teacher"};
static const char* JD[J_COUNT] = {"Keep 3 kids happy: feed, play, nap & hug", "Scan items as they hit the scanner", "Memorize the order, then make it", "Take orders & serve the right dish",
    "Build the bouquet the customer asked for", "Groom pets in the right order", "Handle the class & answer questions"};
static const int PAY[J_COUNT] = {70, 55, 60, 65, 60, 65, 80};

// ------------------------------------------------ 24 more gigs, run on 4 shared engines (data-driven)
//   'M' match  : a request (or clues) for 2-4 picks - get every pick right.  memory = the request hides after a few seconds
//   'W' juggle : 3-4 people/things develop problems; tap the slot's right fix before its timer runs out
//   'T' timing : tap when the marker is in the zone; perItem good taps finish one item
//   'Q' quiz   : quick tutoring questions
enum { AGE_KID = 0, AGE_TEEN = 1, AGE_ADULT = 2 };
struct GJob {
    const char* name; const char* desc; int pay; char kind; int age; int skill; int circle;
    int nSteps; bool memory; const char* stepN[4]; const char* opt[4][4]; const char* clue[4][4];   // M
    int nSlots; const char* slotN; const char* prob[4]; const char* fix[4];                          // W (prob k is fixed by fix k)
    const char* verb; int perItem; float zone;                                                       // T
};
#define NOCLUE {{nullptr}}
static const GJob GJ[] = {
    // ---- glam & beauty
    {"Nail Tech", "Match the client's nail order", 70, 'M', AGE_TEEN, XS_MAKEUP, CI_FASHION, 3, false, {"Color", "Design", "Shape"},
        {{"Hot Pink", "Nude", "Black", "Chrome"}, {"French", "Hearts", "Gems", "Plain"}, {"Almond", "Square", "Coffin", "Stiletto"}}, NOCLUE},
    {"Lash Tech", "Read the vibe, pick the right set", 75, 'M', AGE_TEEN, XS_MAKEUP, CI_FASHION, 2, false, {"Style", "Length"},
        {{"Natural", "Wispy", "Mink", "Doll"}, {"Short", "Medium", "Long", "Dramatic"}},
        {{"\"just my lashes but better\"", "\"soft & fluttery\"", "\"fluffy & full\"", "\"wide-eyed & cute\""}, {"\"I play sports\"", "\"everyday\"", "\"date night\"", "\"I'm going viral\""}}},
    {"Braider", "Tap in rhythm to braid", 85, 'T', AGE_TEEN, XS_MAKEUP, CI_FASHION, 0, false, {}, {}, NOCLUE, 0, "", {}, {}, "Braid!", 8, 0.20f},
    {"Makeup Artist", "Match the prom look", 80, 'M', AGE_TEEN, XS_MAKEUP, CI_FASHION, 3, false, {"Eyes", "Lips", "Cheeks"},
        {{"Smoky", "Glitter", "Winged", "Soft Brown"}, {"Red", "Gloss", "Nude", "Berry"}, {"Blush", "Bronzer", "Highlight", "Contour"}}, NOCLUE},
    // ---- fashion & retail
    {"Boutique Stylist", "Dress the client for her vibe", 70, 'M', AGE_TEEN, XS_MAKEUP, CI_FASHION, 3, false, {"Top", "Bottom", "Shoes"},
        {{"Baby Tee", "Corset", "Hoodie", "Blazer"}, {"Mini Skirt", "Leather Pants", "Sweats", "Trousers"}, {"Platforms", "Heels", "Sneakers", "Loafers"}},
        {{"cute & casual", "date night", "comfy day", "boss girl"}, {"cute & casual", "date night", "comfy day", "boss girl"}, {"cute & casual", "date night", "comfy day", "boss girl"}}},
    {"Fitting Room", "Put returns on the right rack", 55, 'W', AGE_TEEN, XS_HOME, CI_FASHION, 0, false, {}, {}, NOCLUE, 4, "Pile",
        {"Jeans", "Dress", "Top", "Heels"}, {"Denim", "Dresses", "Tops", "Shoes"}},
    {"Mall Kiosk", "Pitch the right thing to each shopper", 60, 'W', AGE_TEEN, XS_COMEDY, CI_FASHION, 0, false, {}, {}, NOCLUE, 3, "Shopper",
        {"Teen girl", "Busy mom", "Tourist", "Gamer"}, {"Lip Gloss", "Phone Case", "Keychain", "Earbuds"}},
    // ---- food
    {"Boba Shop", "Memorize the order, then make it", 60, 'M', AGE_TEEN, XS_MIXOLOGY, CI_PARTY, 4, true, {"Tea", "Flavor", "Topping", "Ice"},
        {{"Milk Tea", "Green Tea", "Thai Tea", "Fruit Tea"}, {"Taro", "Strawberry", "Brown Sugar", "Mango"}, {"Boba", "Jelly", "Popping", "Pudding"}, {"No Ice", "Less Ice", "Regular", "Extra"}}, NOCLUE},
    {"Food Truck", "Lunch rush - remember every order", 65, 'M', AGE_TEEN, XS_HOME, CI_PARTY, 3, true, {"Main", "Side", "Drink"},
        {{"Tacos", "Burger", "Wings", "Gyro"}, {"Fries", "Elote", "Mac", "Salad"}, {"Lemonade", "Soda", "Horchata", "Water"}}, NOCLUE},
    {"Bakery Decorator", "Pipe the icing on the beat", 65, 'T', AGE_TEEN, XS_PAINT, CI_ARTISTS, 0, false, {}, {}, NOCLUE, 0, "", {}, {}, "Pipe!", 6, 0.22f},
    // ---- social & online
    {"Influencer Deal", "Shoot the brand's post their way", 95, 'M', AGE_TEEN, XS_PHOTO, CI_FASHION, 3, false, {"Pose", "Lighting", "Caption"},
        {{"Mirror Selfie", "Candid", "Hair Flip", "Pointing"}, {"Ring Light", "Golden Hour", "Flash", "Neon"}, {"\"obsessed\"", "\"link in bio\"", "\"run don't walk\"", "\"not sponsored jk\""}},
        {{"Glow Skin Co.", "Cozy Knits", "Hair Oil Brand", "Phone Case Brand"}, {"Glow Skin Co.", "Cozy Knits", "Hair Oil Brand", "Phone Case Brand"}, {"Glow Skin Co.", "Cozy Knits", "Hair Oil Brand", "Phone Case Brand"}}},
    {"Party Promoter", "Text the right people - avoid drama", 70, 'W', AGE_TEEN, XS_COMEDY, CI_PARTY, 0, false, {}, {}, NOCLUE, 4, "Contact",
        {"Party girl", "The ex", "DJ", "Snitch"}, {"Invite", "Leave on Read", "Book Them", "Block"}},
    {"Party Planner", "Decorate for the client's theme", 85, 'M', AGE_TEEN, XS_HOME, CI_PARTY, 3, false, {"Decor", "Cake", "Music"},
        {{"Pink Balloons", "Candles", "Neon Signs", "Flowers"}, {"Glitter Cake", "Tiered White", "Cupcake Tower", "Naked Cake"}, {"Pop", "Love Songs", "Hip-Hop", "Acoustic"}},
        {{"Sweet 16", "Wedding", "Club Night", "Garden Brunch"}, {"Sweet 16", "Wedding", "Club Night", "Garden Brunch"}, {"Sweet 16", "Wedding", "Club Night", "Garden Brunch"}}},
    // ---- out & about
    {"Dog Walker", "Keep every pup in line", 55, 'W', AGE_KID, XS_HOME, CI_FAMILY, 0, false, {}, {}, NOCLUE, 3, "Dog",
        {"Squirrel!", "Tangled", "Potty", "Sitting down"}, {"Tug Leash", "Untangle", "Poop Bag", "Treat"}},
    {"Lifeguard", "Watch the pool - save who needs it", 75, 'W', AGE_TEEN, XS_DANCE, CI_JOCKS, 0, false, {}, {}, NOCLUE, 4, "Swimmer",
        {"Struggling", "Running", "Waving hi", "Diving shallow"}, {"Rescue!", "Whistle", "Wave Back", "Warn"}},
    {"Valet", "Brake in the parking spot", 65, 'T', AGE_ADULT, XS_GAMING, CI_PARTY, 0, false, {}, {}, NOCLUE, 0, "", {}, {}, "Brake!", 1, 0.12f},
    {"Delivery Driver", "Pick the fastest route", 60, 'M', AGE_ADULT, XS_GAMING, CI_FAMILY, 2, false, {"Route", "Drop-off"},
        {{"Main St", "Highway", "Back Roads", "Bridge"}, {"Front Door", "Lobby", "Side Gate", "Hand It Over"}},
        {{"school traffic on the highway", "no traffic anywhere", "main st is closed", "back roads flooded"}, {"\"leave it at the door\"", "\"apartment building\"", "\"beware of dog\"", "\"I'll be outside\""}}},
    {"Rideshare Driver", "Read your passengers for tips", 70, 'W', AGE_ADULT, XS_COMEDY, CI_PARTY, 0, false, {}, {}, NOCLUE, 3, "Rider",
        {"Chatty", "Tired", "Messy", "On a call"}, {"Chat", "Quiet Ride", "Hand Water", "Turn Music Down"}},
    // ---- kid & teen friendly
    {"Twin Babysitting", "Two kids, twice the chaos", 80, 'W', AGE_TEEN, XS_HOME, CI_FAMILY, 0, false, {}, {}, NOCLUE, 2, "Twin",
        {"Hungry", "Fighting", "Wandering off", "Crying"}, {"Snack", "Separate", "Chase", "Hug"}},
    {"Lemonade Stand", "Price it for the weather", 35, 'M', AGE_KID, XS_HOME, CI_FAMILY, 2, false, {"Price", "Recipe"},
        {{"$1", "$2", "$3", "$5"}, {"Hot Cocoa", "Classic", "Pink Lemonade", "Extra Ice"}},
        {{"rainy & cold", "cloudy", "sunny", "heat wave"}, {"rainy & cold", "cloudy", "sunny", "heat wave"}}},
    {"Car Wash", "Fundraiser! Scrub every spot", 35, 'W', AGE_KID, XS_HOME, CI_FAMILY, 0, false, {}, {}, NOCLUE, 4, "Car",
        {"Mud", "Bird mess", "Dull paint", "Dusty wheels"}, {"Scrub", "Rinse", "Wax", "Brush"}},
    {"Tutor", "Help the kid with homework", 50, 'Q', AGE_TEEN, XS_WRITE, CI_SCHOLARS},
    // ---- adults only
    {"Bottle Service", "Keep VIP tables happy (18+)", 110, 'W', AGE_ADULT, XS_MIXOLOGY, CI_PARTY, 0, false, {}, {}, NOCLUE, 4, "Table",
        {"Wants a bottle", "Birthday!", "Wants the check", "Drama"}, {"Bring Bottle", "Sparklers", "Bring Check", "Security"}},
    {"Bartender", "Memorize the drink, then mix it (18+)", 90, 'M', AGE_ADULT, XS_MIXOLOGY, CI_PARTY, 3, true, {"Base", "Mixer", "Garnish"},
        {{"Vodka", "Tequila", "Rum", "Mocktail"}, {"Cranberry", "Lime", "Cola", "Pineapple"}, {"Lime Wheel", "Cherry", "Salt Rim", "Mint"}}, NOCLUE},
    // ---- batch 3
    {"Hair Salon Assistant", "Shampoo, cut & style to the ticket", 70, 'M', AGE_TEEN, XS_MAKEUP, CI_FASHION, 3, false, {"Wash", "Cut", "Style"},
        {{"Moisture", "Clarify", "Purple", "Scalp Scrub"}, {"Trim", "Layers", "Bob", "Bangs"}, {"Silk Press", "Curls", "Updo", "Beach Waves"}}, NOCLUE},
    {"Tattoo Apprentice", "Trace the stencil steady (18+)", 90, 'T', AGE_ADULT, XS_PAINT, CI_ARTISTS, 0, false, {}, {}, NOCLUE, 0, "", {}, {}, "Ink!", 7, 0.16f},
    {"Photo Booth Attendant", "Pick props for each group", 55, 'M', AGE_TEEN, XS_PHOTO, CI_PARTY, 2, false, {"Prop", "Backdrop"},
        {{"Tiaras", "Big Glasses", "Feather Boas", "Mustaches"}, {"Pink Tinsel", "Gold Balloons", "Neon Hearts", "Disco"}},
        {{"birthday girls", "nerdy friends", "bachelorette", "silly dads"}, {"\"make it cute\"", "\"classy please\"", "\"so Y2K\"", "\"party vibes\""}}},
    {"Dance Instructor", "Call the moves on the beat", 80, 'T', AGE_TEEN, XS_DANCE, CI_JOCKS, 0, false, {}, {}, NOCLUE, 0, "", {}, {}, "5-6-7-8!", 8, 0.22f},
    {"Cheer Coach", "Keep the squad on point", 70, 'W', AGE_TEEN, XS_DANCE, CI_JOCKS, 0, false, {}, {}, NOCLUE, 4, "Flyer",
        {"Wobbling", "Off count", "Forgot the cheer", "Bored"}, {"Spot Her", "Count It", "Teach It", "Hype Up"}},
    {"Thrift Flipper", "Price the finds right", 65, 'M', AGE_TEEN, XS_HOME, CI_FASHION, 2, false, {"Price", "Where to Sell"},
        {{"$5", "$15", "$40", "$120"}, {"Yard Sale", "Thrift App", "Pop-Up", "Luxury Resale"}},
        {{"plain tee", "vintage jeans", "Y2K baby tee", "designer bag"}, {"plain tee", "vintage jeans", "Y2K baby tee", "designer bag"}}},
    {"Ice Cream Scooper", "Build the cone from memory", 45, 'M', AGE_KID, XS_HOME, CI_FAMILY, 3, true, {"Cone", "Scoop", "Topping"},
        {{"Waffle", "Sugar", "Cup", "Dipped"}, {"Vanilla", "Strawberry", "Cookie Dough", "Mint Chip"}, {"Sprinkles", "Fudge", "Cherry", "Gummy Bears"}}, NOCLUE},
    {"Cake Pop Stand", "Dip on the beat", 40, 'T', AGE_KID, XS_HOME, CI_FAMILY, 0, false, {}, {}, NOCLUE, 0, "", {}, {}, "Dip!", 4, 0.24f},
    {"House Sitter", "Keep the house in shape", 60, 'W', AGE_TEEN, XS_HOME, CI_FAMILY, 0, false, {}, {}, NOCLUE, 4, "Room",
        {"Plants dry", "Cat on counter", "Mail piling", "Door open"}, {"Water", "Shoo", "Bring In", "Lock Up"}},
    {"Concert Merch", "Sell the right merch to each fan", 75, 'W', AGE_TEEN, XS_SING, CI_PARTY, 0, false, {}, {}, NOCLUE, 4, "Fan",
        {"Wants a tee", "Wants a poster", "Wants a lightstick", "Cash only"}, {"Tee", "Poster", "Lightstick", "Make Change"}},
    {"Front Desk", "Hotel check-ins - help every guest", 70, 'W', AGE_ADULT, XS_COMEDY, CI_FAMILY, 0, false, {}, {}, NOCLUE, 3, "Guest",
        {"Checking in", "Lost key", "Noise complaint", "Wants towels"}, {"Hand Key", "New Key", "Call Room", "Send Towels"}},
    {"Model Casting", "Walk it on the beat", 100, 'T', AGE_TEEN, XS_DANCE, CI_FASHION, 0, false, {}, {}, NOCLUE, 0, "", {}, {}, "Strut!", 6, 0.18f},
    {"Wedding DJ", "Read the room, play the right song", 95, 'M', AGE_ADULT, XS_SING, CI_PARTY, 2, false, {"Song", "Volume"},
        {{"Slow Jam", "Line Dance", "Throwback", "Club Hit"}, {"Soft", "Medium", "Loud", "Blasting"}},
        {{"first dance", "aunties on the floor", "parents' table", "after midnight"}, {"first dance", "aunties on the floor", "parents' table", "after midnight"}}},
    {"Pet Sitter", "Feed, walk & cuddle every pet", 55, 'W', AGE_KID, XS_HOME, CI_FAMILY, 0, false, {}, {}, NOCLUE, 3, "Pet",
        {"Hungry", "Needs out", "Lonely", "Chewing shoes"}, {"Feed", "Walk", "Cuddle", "Chew Toy"}},
    {"Spelling Tutor", "Quick quiz for the kid", 50, 'Q', AGE_TEEN, XS_WRITE, CI_SCHOLARS},
    {"Club Host", "Run the door & VIP list (18+)", 105, 'W', AGE_ADULT, XS_COMEDY, CI_PARTY, 0, false, {}, {}, NOCLUE, 4, "Guest",
        {"On the list", "Not on the list", "Fake ID", "Celebrity!"}, {"Let In", "Turn Away", "Call Bouncer", "VIP Rope"}},
};
static const int GJ_N = (int)(sizeof GJ / sizeof GJ[0]);
static const int NPAGES = (J_COUNT + GJ_N + 11) / 12;
static const int ALLJ = J_COUNT + GJ_N;
static const char* jobName(int j) { return j < J_COUNT ? JN[j] : GJ[j - J_COUNT].name; }
static const char* jobDesc(int j) { return j < J_COUNT ? JD[j] : GJ[j - J_COUNT].desc; }
static int jobPay(int j) { return j < J_COUNT ? PAY[j] : GJ[j - J_COUNT].pay; }
static int jobAge(int j) { return j < J_COUNT ? AGE_TEEN : GJ[j - J_COUNT].age; }
static int page = 0;

static int job = -1, score = 0, misses = 0, phase = 0;   // phase 0 pick, 1 playing, 2 results
static const GJob* gj() { return job >= J_COUNT ? &GJ[job - J_COUNT] : nullptr; }
static float T = 0, left = 60, cool = 0;
static char msg[64] = "";
static float msgT = 0;
static void say(const char* m) { snprintf(msg, sizeof msg, "%s", m); msgT = 1.6f; }
static void good(int pts, const char* m) { score += pts; say(m); sfxYay(); }
static void bad(const char* m) { misses++; say(m); sfxNo(); }
static Btn grid(int i, int cols, float y0, float h) { const float w = (312.f - (cols - 1) * 4) / cols; return Btn{4 + (i % cols) * (w + 4), y0 + (i / cols) * (h + 4), w, h}; }

// ------------------------------------------------ per-job state
static int need[3]; static float needT[3];                       // nanny: kid needs (0 none,1 hungry,2 bored,3 sleepy,4 crying)
static const char* NEEDS[5] = {"", "Hungry", "Bored", "Sleepy", "Crying"};
static const char* CARE[4] = {"Bottle", "Toy", "Nap", "Hug"};
static float itemX = -40; static int item = 0; static bool scanned = false, produce = false;   // grocery
static const char* GROC[10] = {"Milk", "Cereal", "Bananas", "Eggs", "Bread", "Apples", "Soda", "Chips", "Avocados", "Yogurt"};
static int ord[4], made[4], step = 0; static float showT = 0;    // barista & florist & server
static const char* SIZE[3] = {"Small", "Medium", "Large"};
static const char* DRINK[4] = {"Latte", "Mocha", "Cold Brew", "Matcha"};
static const char* MILK[4] = {"Whole", "Oat", "Almond", "No Milk"};
static const char* EXTRA[4] = {"Caramel", "Vanilla", "Whip", "Plain"};
static int tbl[3]; static float tblT[3]; static int carry = -1;  // server: wanted dish per table (-1 none), carry dish
static const char* DISH[5] = {"Pasta", "Burger", "Salad", "Sushi", "Pancakes"};
static int flw[3] = {}; static const char* FLOWERS[4] = {"Roses", "Tulips", "Daisies", "Lilies"}; static const char* RIB[3] = {"Pink", "White", "Gold"};
static int pet = 0, groomStep = 0, dirt = 0; static const char* PETS[3] = {"Poodle", "Kitty", "Bunny"};
static const char* TOOLS[4] = {"Shampoo", "Rinse", "Brush", "Bow"};
static int kidAct[4]; static float kidT[4]; static int qa = 0, qb = 0, qOpts[3], qRight = 0;   // teacher
static const char* MISB[4] = {"", "Talking", "Paper Airplane", "Napping"};
static const char* FIX[3] = {"Shh!", "Confiscate", "Wake Up"};
static int gProb[4]; static float gT[4]; static int gSel = -1;   // W engine
static float tPos = 0, tDir = 1, tZone = 0.5f; static int tHits = 0;   // T engine
static int qOp = 0, qWord = 0, qBlank = 0;                             // Q engine (qOp 3 = spelling)
static const char* WORDS[12] = {"friend", "school", "because", "pretty", "bunny", "glitter", "science", "library", "beautiful", "dessert", "weird", "sparkle"};
static bool spelling() { return job >= J_COUNT && GJ[job - J_COUNT].name[0] == 'S' && GJ[job - J_COUNT].kind == 'Q'; }

static void newOrder() {
    if (job == J_BARISTA) { ord[0] = irand(3); ord[1] = irand(4); ord[2] = irand(4); ord[3] = irand(4); step = 0; showT = 3.2f; }
    if (job == J_FLORIST) { ord[0] = 1 + irand(4); ord[1] = irand(4); ord[2] = 1 + irand(3); ord[3] = irand(3); flw[0] = flw[1] = flw[2] = 0; step = 0; showT = 0; }   // n of A, flower A, n of B, ribbon
    if (job == J_GROOMER) { pet = irand(3); groomStep = 0; dirt = 3 + irand(3); }
    if (const GJob* g = gj()) {
        if (g->kind == 'M') {
            const bool vibe = g->clue[0][0] && g->clue[1][0] && g->clue[0][1] && std::string(g->clue[0][0]) == (g->clue[1][0] ? g->clue[1][0] : "");
            const int v = irand(4);   // clue jobs where every step shares one vibe (Stylist, Planner, Lemonade, Influencer)
            for (int k = 0; k < g->nSteps; k++) ord[k] = vibe ? v : irand(4);
            step = 0; showT = g->memory ? 3.6f : 0;
        }
        if (g->kind == 'T') { tPos = 0; tDir = 1; tHits = 0; tZone = 0.2f + frand() * (0.8f - g->zone - 0.2f); }
        if (g->kind == 'Q' && spelling()) { qOp = 3; qWord = irand(12); const int L = (int)strlen(WORDS[qWord]); qBlank = irand(L); qRight = irand(3);
            const char c = WORDS[qWord][qBlank]; for (int k = 0; k < 3; k++) { int o; do o = 'a' + irand(26); while (o == c || (k && o == qOpts[k - 1])); qOpts[k] = k == qRight ? c : o; } }
        else if (g->kind == 'Q') { qOp = irand(3); qa = qOp == 2 ? 2 + irand(8) : 5 + irand(20); qb = qOp == 2 ? 2 + irand(8) : 1 + irand(qa - 1);
            const int ans = qOp == 0 ? qa + qb : qOp == 1 ? qa - qb : qa * qb; qRight = irand(3);
            for (int k = 0; k < 3; k++) qOpts[k] = k == qRight ? ans : ans + (k < qRight ? -1 - irand(4) : 1 + irand(4)); }
        return;
    }
    if (job == J_TEACHER) { qa = 2 + irand(9); qb = 1 + irand(9); qRight = irand(3); for (int k = 0; k < 3; k++) qOpts[k] = k == qRight ? qa + qb : qa + qb + (k < qRight ? -1 - irand(3) : 1 + irand(3)); }
}
static void startJob(int j) {
    job = j; score = misses = 0; phase = 1; left = 60; T = 0; cool = 0; msg[0] = 0;
    for (int k = 0; k < 3; k++) { need[k] = 0; needT[k] = 2 + k * 2.f; tbl[k] = -1; tblT[k] = 1 + k * 2.5f; }
    for (int k = 0; k < 4; k++) { kidAct[k] = 0; kidT[k] = 2 + k * 1.7f; gProb[k] = -1; gT[k] = 1.5f + k * 1.6f; }
    gSel = -1;
    itemX = -40; item = irand(10); scanned = false; produce = irand(4) == 0; carry = -1;
    newOrder();
}
static void finish() {
    phase = 2;
    const int pay = std::max(10, jobPay(job) + score * 4 - misses * 3);
    G.money += pay;
    Sim& me = MC(); me.needs[N_ENERGY] = std::max(0.f, me.needs[N_ENERGY] - 15);
    if (score >= 15) { addMoodlet(me, "Employee of the Day", EM_CONFIDENT, 2, 600); lxRepAdd(CI_FAMILY, 1, "hard worker"); }
    else if (misses > score) addMoodlet(me, "Rough Shift", EM_TENSE, 1, 360);
    snprintf(msg, sizeof msg, "Shift over! Score %d, misses %d. Paid $%d", score, misses, pay);
    lvlEvent(LEV_ODDJOB); lvlPrestige(score >= 12 ? 4 : 2, "odd job shift");
    if (const GJob* g = gj()) { lxSkill(g->skill, 6); if (score >= 12) lxRepAdd(g->circle, 1, g->name); }
    else lxSkill(job == J_BARISTA ? XS_MIXOLOGY : job == J_FLORIST ? XS_GARDEN : job == J_GROOMER ? XS_HOME : XS_COMEDY, 6);
}

void oddJobsOpen() { phase = 0; job = -1; page = 0; }
static Btn pickBtn(int i) { return grid(i, 2, 24, 26); }   // 12 gigs per page

bool oddJobsUpdate(const Input& in) {
    const float dt = in.dt; const touchPosition& tp = in.tp; const bool tap = in.touch;
    if (msgT > 0) msgT -= dt;
    if (phase == 0) {
        if (in.down & KEY_B) return false;
        if (tap) {
            for (int i = 0; i < 12; i++) { const int k = page * 12 + i; if (k >= ALLJ || !pickBtn(i).in(tp)) continue;
                const int st = MC().stage, a = jobAge(k);
                if (!lvlAllows(LF_ODDJOBS)) { say("Odd Jobs unlock at Level 5!"); return true; }
                if (a == AGE_ADULT && !lvlAllows(LF_ADULTGIGS)) { say("18+ gigs unlock at Level 29!"); return true; }
                { const char* nm = jobName(k); if ((!strcmp(nm, "Model Casting") || !strcmp(nm, "Influencer Deal")) && !lvlAllows(LF_MODELING)) { say("Modeling gigs unlock at Level 17!"); return true; }
                  (void)nm; }
                if (a == AGE_KID && st < ST_CHILD) { say("Too little for a job!"); return true; }
                if (a == AGE_TEEN && st < ST_TEEN) { say("Teens and up only!"); return true; }
                if (a == AGE_ADULT && st < ST_YA) { say("Adults only (18+)!"); return true; }
                startJob(k); sfxTap(); return true; }
            if (Btn{4, 206, 70, 28}.in(tp)) { page = (page + NPAGES - 1) % NPAGES; sfxTap(); }
            if (Btn{80, 206, 70, 28}.in(tp)) { page = (page + 1) % NPAGES; sfxTap(); }
            if (Btn{236, 206, 80, 28}.in(tp)) return false; }
        if (in.down & (KEY_R | KEY_DRIGHT)) page = (page + 1) % NPAGES;
        if (in.down & (KEY_L | KEY_DLEFT)) page = (page + NPAGES - 1) % NPAGES;
        return true;
    }
    if (phase == 2) { if ((in.down & (KEY_A | KEY_B)) || tap) { phase = 0; } return true; }
    T += dt; left -= dt; if (cool > 0) cool -= dt;
    if (left <= 0 || (in.down & KEY_START)) { finish(); return true; }
    if (const GJob* g = gj()) {
        switch (g->kind) {
        case 'M':
            if (showT > 0) { showT -= dt; break; }
            if (tap) for (int c = 0; c < 4; c++) if (grid(c, 2, 64, 40).in(tp)) {
                made[step] = c; step++; sfxTap();
                if (step == g->nSteps) { bool ok = true; for (int k = 0; k < g->nSteps; k++) ok &= made[k] == ord[k];
                    if (ok) good(2 + g->nSteps * 2, "Perfect! Big tip!"); else bad("Not what they wanted...");
                    newOrder(); }
            }
            if (tap && Btn{4, 206, 100, 28}.in(tp)) { step = 0; say("Started over."); }
            break;
        case 'W':
            for (int k = 0; k < g->nSlots; k++) {
                if (gProb[k] < 0) { if ((gT[k] -= dt) <= 0) { gProb[k] = irand(4); gT[k] = std::max(3.5f, 7.f - T * 0.04f); } }
                else if ((gT[k] -= dt) <= 0) { bad("Too slow!"); gProb[k] = -1; gT[k] = 2 + irand(3); }
            }
            if (tap) {
                for (int k = 0; k < g->nSlots; k++) if (grid(k, g->nSlots, 30, 40).in(tp)) { gSel = k; sfxTap(); }
                for (int f = 0; f < 4; f++) if (grid(f, 2, 100, 34).in(tp)) {
                    if (gSel < 0 || gProb[gSel] < 0) { bad("Pick who needs help first."); break; }
                    if (f == gProb[gSel]) { gProb[gSel] = -1; gT[gSel] = 2 + irand(4); good(2, "Handled!"); } else bad("That didn't help...");
                    gSel = -1;
                }
            }
            break;
        case 'T':
            tPos += tDir * dt * (0.55f + T * 0.008f); if (tPos > 1) { tPos = 1; tDir = -1; } if (tPos < 0) { tPos = 0; tDir = 1; }
            if (tap && Btn{60, 120, 200, 70}.in(tp)) {
                if (tPos >= tZone && tPos <= tZone + g->zone) { tHits++; sfxTap(); if (tHits >= g->perItem) { good(2 + g->perItem / 2, "Done! Looks amazing."); newOrder(); } else say("Nice!"); }
                else { bad("Off beat!"); if (g->perItem == 1) newOrder(); }
            }
            break;
        case 'Q':
            if (tap) for (int q = 0; q < 3; q++) if (grid(q, 3, 120, 44).in(tp)) { if (q == qRight) good(3, "Got it! The kid gets it now."); else bad("Hmm, that's not it."); newOrder(); }
            break;
        }
        return true;
    }
    switch (job) {
    case J_NANNY:
        for (int k = 0; k < 3; k++) { if (need[k]) { needT[k] -= dt; if (needT[k] <= 0) { bad("A kid had a meltdown!"); need[k] = 0; needT[k] = 3; } } else if ((needT[k] -= dt) <= 0) { need[k] = 1 + irand(4); needT[k] = 6; } }
        if (tap) for (int k = 0; k < 3; k++) for (int c = 0; c < 4; c++) if (grid(k * 4 + c, 4, 40, 34).in(tp)) {
            if (!need[k]) bad("They didn't need that.");
            else if (need[k] - 1 == c) { need[k] = 0; needT[k] = 2 + irand(3); good(2, "Happy kid!"); }
            else bad("Wrong care!");
        }
        break;
    case J_GROCERY:
        itemX += dt * (90 + T * 1.5f);
        if (itemX > 420) { if (!scanned) bad("Missed an item!"); itemX = -40; item = irand(10); scanned = false; produce = irand(4) == 0; }
        if (tap && Btn{20, 150, 130, 50}.in(tp)) {   // SCAN
            if (scanned) bad("Double scanned!");
            else if (itemX > 170 && itemX < 250) { if (produce) bad("Produce goes on the scale!"); else { scanned = true; good(2, "BEEP!"); } }
            else bad("Not in the scanner yet!");
        }
        if (tap && Btn{170, 150, 130, 50}.in(tp)) {  // WEIGH
            if (scanned) bad("Already rang that up!");
            else if (itemX > 170 && itemX < 250 && produce) { scanned = true; good(3, "Weighed & rung up!"); }
            else bad("That's not produce!");
        }
        break;
    case J_BARISTA:
        if (showT > 0) { showT -= dt; break; }   // memorize!
        if (tap) for (int c = 0; c < 4; c++) if (grid(c, 4, 60, 40).in(tp)) {
            if (step == 0 && c > 2) break;
            made[step] = c; step++;
            if (step == 4) {
                const bool ok = made[0] == ord[0] && made[1] == ord[1] && made[2] == ord[2] && made[3] == ord[3];
                if (ok) good(5, "Perfect order! Big tip!"); else bad("That's... not what I ordered.");
                newOrder();
            }
        }
        if (tap && Btn{4, 206, 100, 28}.in(tp)) { step = 0; say("Started over."); }
        break;
    case J_SERVER:
        for (int k = 0; k < 3; k++) {
            if (tbl[k] < 0) { if ((tblT[k] -= dt) <= 0) { tbl[k] = irand(5); tblT[k] = 14; } }
            else if ((tblT[k] -= dt) <= 0) { bad("A table walked out!"); tbl[k] = -1; tblT[k] = 3; }
        }
        if (tap) {
            for (int d = 0; d < 5; d++) if (grid(d, 5, 40, 30).in(tp)) { carry = d; say((std::string("Carrying ") + DISH[d]).c_str()); }
            for (int k = 0; k < 3; k++) if (grid(k, 3, 110, 60).in(tp)) {
                if (tbl[k] < 0) { bad("Nobody's sitting there."); break; }
                if (carry < 0) { bad("Grab a dish from the pass first."); break; }
                if (carry == tbl[k]) { good(3 + (tblT[k] > 8 ? 2 : 0), "Served with a smile!"); tbl[k] = -1; tblT[k] = 2 + irand(3); }
                else bad("Wrong dish!");
                carry = -1;
            }
        }
        break;
    case J_FLORIST:
        if (tap) {
            for (int f = 0; f < 4; f++) if (grid(f, 4, 60, 36).in(tp)) { if (f == ord[1]) flw[0]++; else if (f == (ord[1] + 1) % 4) flw[1]++; else flw[2]++; sfxTap(); }
            for (int r = 0; r < 3; r++) if (grid(r, 3, 104, 30).in(tp)) { step = r + 1; sfxTap(); }
            if (Btn{110, 150, 100, 40}.in(tp)) {
                const bool ok = flw[0] == ord[0] && flw[1] == ord[2] && flw[2] == 0 && step == ord[3] + 1;
                if (ok) good(6, "Gorgeous! The customer loves it."); else bad("Not quite what they asked for...");
                newOrder();
            }
            if (Btn{4, 206, 100, 28}.in(tp)) { flw[0] = flw[1] = flw[2] = 0; step = 0; }
        }
        break;
    case J_GROOMER:
        if (tap) {
            if (groomStep == 1 && dirt > 0 && Btn{120, 30, 80, 80}.in(tp)) { dirt--; sfxTap(); if (!dirt) say("Squeaky clean! Now rinse."); break; }
            for (int t = 0; t < 4; t++) if (grid(t, 4, 150, 40).in(tp)) {
                const int want = groomStep == 0 ? 0 : groomStep == 1 ? (dirt ? -1 : 1) : groomStep == 2 ? 2 : 3;
                if (want == -1) { bad("Scrub the dirt spots first!"); break; }
                if (t == want) { groomStep++; if (groomStep == 1) say("Lather up! Tap the pet to scrub."); else if (groomStep == 4) { good(6, "Fluffy & fabulous!"); newOrder(); } else sfxTap(); }
                else bad("Wrong order!");
            }
        }
        break;
    case J_TEACHER:
        for (int k = 0; k < 4; k++) {
            if (!kidAct[k]) { if ((kidT[k] -= dt) <= 0) { kidAct[k] = 1 + irand(3); kidT[k] = 6; } }
            else if ((kidT[k] -= dt) <= 0) { bad("The class got out of hand!"); kidAct[k] = 0; kidT[k] = 3; }
        }
        if (tap) {
            static int sel = -1;
            for (int k = 0; k < 4; k++) if (grid(k, 4, 30, 40).in(tp)) sel = k;
            for (int f = 0; f < 3; f++) if (grid(f, 3, 78, 28).in(tp)) {
                if (sel < 0 || !kidAct[sel]) { bad("Tap a misbehaving kid first."); break; }
                if (f == kidAct[sel] - 1) { kidAct[sel] = 0; kidT[sel] = 3 + irand(4); good(2, "Class is back on track."); } else bad("That didn't work...");
                sel = -1;
            }
            for (int q = 0; q < 3; q++) if (grid(q, 3, 150, 34).in(tp)) { if (q == qRight) good(3, "Correct! The class claps."); else bad("Oops, wrong answer!"); newOrder(); }
        }
        break;
    }
    return true;
}

void oddJobsDrawTop(float t) {
    char b[96];
    grad(0, 0, TOP_W, TOP_H, C(255, 228, 240), C(250, 205, 226));
    for (int k = 0; k < 10; k++) heart(20 + k * 40.f, 14 + std::sin(t + k) * 3, 4, C(255, 160, 200, 120));
    if (phase == 0) { textOutline("ODD JOBS", 200, 70, 1.f, C(255, 255, 255), C(255, 46, 140), 1); text("Pick a gig on the bottom screen", 200, 120, 0.55f, C(UI_INK), 1); return; }
    textOutline(jobName(job), 200, 6, 0.7f, C(255, 255, 255), C(255, 46, 140), 1);
    snprintf(b, sizeof b, "Score %d   Misses %d   %ds", score, misses, std::max(0, (int)left)); text(b, 200, 32, 0.45f, C(UI_INK), 1);
    if (phase == 2) { rrect(40, 90, 320, 60, 12, C(255, 255, 255, 230)); text(msg, 200, 108, 0.5f, C(UI_INK), 1); text("Tap to continue", 200, 128, 0.4f, C(180, 100, 140), 1); return; }
    if (const GJob* g = gj()) {
        switch (g->kind) {
        case 'M':
            rrect(30, 56, 340, 130, 14, C(255, 255, 255, 235));
            if (showT > 0 || !g->memory) {
                text(g->memory ? "Remember this order:" : g->clue[0][0] ? "The client says:" : "The order:", 200, 62, 0.42f, C(180, 90, 130), 1);
                bool vibe = g->clue[0][0] && g->clue[1][0] && std::string(g->clue[0][0]) == g->clue[1][0];
                if (vibe) { snprintf(b, sizeof b, "\"%s\"", g->clue[0][ord[0]]); text(b, 200, 100, 0.65f, C(UI_INK), 1); }
                else for (int k = 0; k < g->nSteps; k++) { snprintf(b, sizeof b, "%s: %s", g->stepN[k], g->clue[k][0] ? g->clue[k][ord[k]] : g->opt[k][ord[k]]); text(b, 200, 84 + k * 22.f, 0.5f, C(UI_INK), 1); }
                if (g->memory && showT > 0) rect(60, 176, 280 * showT / 3.6f, 5, C(255, 80, 120));
            } else text("Make it from memory!", 200, 90, 0.6f, C(150, 90, 60), 1);
            if (showT <= 0) { snprintf(b, sizeof b, "Step %d of %d: %s", step + 1, g->nSteps, g->stepN[std::min(g->nSteps - 1, step)]); text(b, 200, 192, 0.45f, C(UI_INK), 1);
                for (int k = 0; k < step; k++) { snprintf(b, sizeof b, "%s", g->opt[k][made[k]]); text(b, 60 + k * 94.f, 168, 0.36f, C(255, 46, 140), 1); } }
            break;
        case 'W':
            for (int k = 0; k < g->nSlots; k++) {
                const float x = 200 + (k - (g->nSlots - 1) / 2.f) * (360.f / g->nSlots); const bool trouble = gProb[k] >= 0;
                circle(x, 140 + (trouble ? std::sin(t * 12 + k) * 2 : 0), 20, C(trouble ? RGB{255, 200, 210} : RGB{230, 240, 255}));
                snprintf(b, sizeof b, "%s %d", g->slotN, k + 1); text(b, x, 166, 0.36f, C(UI_INK), 1);
                if (trouble) { const float bw = std::min(88.f, 360.f / g->nSlots - 10); rrect(x - bw / 2, 84, bw, 22, 8, C(255, 255, 255)); text(g->prob[gProb[k]], x, 89, 0.33f, C(200, 40, 90), 1);
                    rect(x - 36, 110, 72 * std::max(0.f, gT[k] / 7.f), 4, C(255, 80, 120)); }
                if (gSel == k) rrect(x - 24, 186, 48, 4, 2, C(255, 46, 140));
            }
            break;
        case 'T': {
            rrect(40, 110, 320, 30, 10, C(255, 255, 255, 230));
            rrect(40 + 320 * tZone, 110, 320 * g->zone, 30, 8, C(120, 220, 160));
            circle(40 + 320 * tPos, 125, 10, C(255, 46, 140));
            snprintf(b, sizeof b, "%s  %d / %d", g->verb, tHits, g->perItem); text(b, 200, 160, 0.55f, C(UI_INK), 1);
            break; }
        case 'Q': {
            rect(60, 60, 280, 70, C(40, 80, 60)); static const char* OP[3] = {"+", "-", "x"};
            if (qOp == 3) { std::string w = WORDS[qWord]; w[qBlank] = '_'; std::string sp; for (char c : w) { sp += c; sp += ' '; } text(sp.c_str(), 200, 80, 0.85f, C(255, 255, 255), 1); }
            else { snprintf(b, sizeof b, "%d %s %d = ?", qa, OP[qOp], qb); text(b, 200, 80, 0.85f, C(255, 255, 255), 1); }
            circle(200, 168, 16, C(230, 190, 160)); text("\"Ohhh... wait, how?\"", 200, 190, 0.4f, C(UI_INK), 1);
            break; }
        }
        if (msgT > 0) { rrect(110, 214, 180, 22, 8, C(255, 255, 255, 230)); text(msg, 200, 218, 0.4f, C(UI_INK), 1); }
        return;
    }
    switch (job) {
    case J_NANNY:
        for (int k = 0; k < 3; k++) {
            const float x = 80 + k * 120.f; const bool upset = need[k] != 0;
            circle(x, 130 + (upset ? std::sin(t * 12) * 2 : 0), 22, C(240, 200, 170)); circle(x - 7, 125, 3, C(40, 30, 30)); circle(x + 7, 125, 3, C(40, 30, 30));
            rect(x - 6, 140, 12, 3, C(upset ? RGB{180, 60, 60} : RGB{230, 100, 130})); rect(x - 18, 152, 36, 40, C(k == 0 ? RGB{150, 200, 255} : k == 1 ? RGB{255, 180, 210} : RGB{180, 230, 170}));
            if (upset) { rrect(x - 38, 70, 76, 22, 8, C(255, 255, 255)); text(NEEDS[need[k]], x, 74, 0.42f, C(200, 40, 90), 1); rect(x - 30, 96, 60 * std::max(0.f, needT[k] / 6), 4, C(255, 80, 120)); }
        }
        break;
    case J_GROCERY:
        rect(0, 150, TOP_W, 30, C(60, 60, 70)); for (int k = 0; k < 20; k++) rect(std::fmod(k * 24 + t * 120, 480.f) - 40, 160, 10, 4, C(90, 90, 100));
        rect(170, 120, 80, 60, C(255, 80, 80, 60)); text("SCANNER", 210, 102, 0.4f, C(200, 30, 60), 1);
        if (!scanned) { rrect(itemX - 26, 120, 52, 30, 6, C(produce ? RGB{140, 210, 120} : RGB{255, 255, 255})); text(GROC[item], itemX, 128, 0.36f, C(UI_INK), 1); if (produce) text("(produce)", itemX, 112, 0.3f, C(60, 140, 60), 1); }
        break;
    case J_BARISTA:
        rrect(60, 60, 280, 110, 14, C(255, 255, 255, 230));
        if (showT > 0) { text("Customer orders:", 200, 70, 0.45f, C(150, 90, 60), 1); snprintf(b, sizeof b, "%s %s", SIZE[ord[0]], DRINK[ord[1]]); text(b, 200, 96, 0.6f, C(UI_INK), 1); snprintf(b, sizeof b, "%s milk, %s", MILK[ord[2]], EXTRA[ord[3]]); text(b, 200, 124, 0.5f, C(UI_INK), 1); rect(80, 156, 240 * showT / 3.2f, 5, C(255, 80, 120)); }
        else { text("Make it from memory!", 200, 80, 0.55f, C(150, 90, 60), 1); static const char* ST[4] = {"Size", "Drink", "Milk", "Extra"}; snprintf(b, sizeof b, "Step %d of 4: %s", step + 1, ST[std::min(3, step)]); text(b, 200, 110, 0.5f, C(UI_INK), 1);
            rrect(180, 130, 40, 34, 6, C(250, 240, 230)); for (int k = 0; k < step; k++) rect(184, 158 - k * 7.f, 32, 6, C(k == 0 ? RGB{200, 160, 120} : k == 1 ? RGB{120, 80, 50} : k == 2 ? RGB{240, 236, 225} : RGB{230, 170, 90})); }
        break;
    case J_SERVER:
        for (int k = 0; k < 3; k++) { const float x = 80 + k * 120.f; ellipse(x, 160, 44, 12, C(250, 245, 235)); rect(x - 3, 160, 6, 30, C(150, 110, 80));
            if (tbl[k] >= 0) { circle(x, 122, 14, C(230, 190, 160)); rrect(x - 36, 76, 72, 22, 8, C(255, 255, 255)); text(DISH[tbl[k]], x, 80, 0.42f, C(UI_INK), 1); rect(x - 30, 102, 60 * std::max(0.f, tblT[k] / 14), 4, C(255, 80, 120)); } }
        if (carry >= 0) { snprintf(b, sizeof b, "Tray: %s", DISH[carry]); text(b, 200, 210, 0.5f, C(UI_INK), 1); }
        break;
    case J_FLORIST:
        rrect(50, 56, 300, 64, 12, C(255, 255, 255, 230));
        snprintf(b, sizeof b, "\"%d %s + %d %s,", ord[0], FLOWERS[ord[1]], ord[2], FLOWERS[(ord[1] + 1) % 4]); text(b, 200, 64, 0.5f, C(UI_INK), 1);
        snprintf(b, sizeof b, "with a %s ribbon please!\"", RIB[ord[3]]); text(b, 200, 88, 0.5f, C(UI_INK), 1);
        for (int i = 0; i < flw[0] + flw[1] + flw[2] && i < 14; i++) { const float x = 150 + (i % 7) * 16.f, y = 150 + (i / 7) * 14.f; circle(x, y, 6, C(i < flw[0] ? RGB{240, 70, 110} : i < flw[0] + flw[1] ? RGB{255, 200, 60} : RGB{200, 200, 255})); }
        if (step) { rect(180, 184, 40, 8, C(step == 1 ? RGB{255, 150, 200} : step == 2 ? RGB{255, 255, 255} : RGB{230, 190, 80})); }
        break;
    case J_GROOMER: {
        const float x = 200, y = 130; const RGB fur = pet == 0 ? RGB{245, 240, 235} : pet == 1 ? RGB{230, 170, 110} : RGB{200, 190, 200};
        circle(x, y, 40, C(fur)); circle(x - 28, y - 30, 14, C(fur)); circle(x + 28, y - 30, 14, C(fur)); circle(x - 12, y - 6, 4, C(30, 30, 30)); circle(x + 12, y - 6, 4, C(30, 30, 30)); circle(x, y + 8, 4, C(240, 120, 140));
        if (groomStep <= 1) for (int k = 0; k < dirt; k++) circle(x - 24 + k * 12.f, y + 22, 5, C(120, 90, 60));
        if (groomStep == 1) for (int k = 0; k < 8; k++) circle(x - 36 + k * 10.f, y - 40 + std::sin(t * 4 + k) * 4, 4, C(255, 255, 255, 200));
        if (groomStep >= 4 - 1 && groomStep == 3) { }
        static const char* NEXT[4] = {"Step 1: Shampoo", "Scrub the spots, then Rinse", "Step 3: Brush", "Step 4: Bow"};
        snprintf(b, sizeof b, "%s - %s", PETS[pet], NEXT[std::min(3, groomStep)]); text(b, 200, 200, 0.45f, C(UI_INK), 1);
        break;
    }
    case J_TEACHER:
        rect(60, 50, 280, 60, C(40, 80, 60)); snprintf(b, sizeof b, "%d + %d = ?", qa, qb); text(b, 200, 66, 0.8f, C(255, 255, 255), 1);
        for (int k = 0; k < 4; k++) { const float x = 60 + k * 94.f; rect(x - 24, 160, 48, 16, C(180, 140, 100)); circle(x, 140, 13, C(220, 180, 150));
            if (kidAct[k]) { text(MISB[kidAct[k]], x, 118, 0.34f, C(200, 40, 80), 1); if (kidAct[k] == 2) circle(x + 18 + std::sin(t * 5) * 6, 120, 3, C(255, 255, 255)); } }
        break;
    }
    if (msgT > 0) { rrect(110, 214, 180, 22, 8, C(255, 255, 255, 230)); text(msg, 200, 218, 0.4f, C(UI_INK), 1); }
}

void oddJobsDrawBottom(float t) {
    (void)t; char b[64];
    rect(0, 0, BOT_W, BOT_H, C(255, 236, 246));
    if (phase == 0) {
        snprintf(b, sizeof b, "Odd Jobs - pick a gig (page %d/%d, L/R):", page + 1, NPAGES); text(b, 160, 6, 0.42f, C(UI_INK), 1);
        for (int i = 0; i < 12; i++) { const int k = page * 12 + i; if (k >= ALLJ) break; Btn g = pickBtn(i);
            const int a = jobAge(k); rrect(g.x, g.y, g.w, g.h, 8, C(a == AGE_ADULT ? RGB{255, 228, 236} : a == AGE_KID ? RGB{232, 248, 236} : RGB{255, 255, 255}));
            text(jobName(k), g.x + 6, g.y + 2, 0.4f, C(255, 46, 140)); snprintf(b, sizeof b, "~$%d+%s", jobPay(k), a == AGE_ADULT ? " 18+" : a == AGE_KID ? " kids ok" : "");
            text(b, g.x + 6, g.y + 14, 0.32f, C(140, 90, 120)); }
        button(Btn{4, 206, 70, 28}, "< Prev", false, 0, 0.42f); button(Btn{80, 206, 70, 28}, "Next >", false, 0, 0.42f);
        button(Btn{236, 206, 80, 28}, "Back", false, 1, 0.45f);
        if (msgT > 0) text(msg, 193, 214, 0.32f, C(200, 40, 90), 1);
        return;
    }
    if (phase == 2) { text("Nice work!", 160, 100, 0.7f, C(255, 46, 140), 1); return; }
    text(jobDesc(job), 160, 6, 0.38f, C(140, 90, 120), 1);
    if (const GJob* g = gj()) {
        switch (g->kind) {
        case 'M':
            if (showT > 0) { text("Memorize it...", 160, 100, 0.55f, C(UI_INK), 1); break; }
            snprintf(b, sizeof b, "Pick the %s:", g->stepN[std::min(g->nSteps - 1, step)]); text(b, 160, 40, 0.45f, C(UI_INK), 1);
            for (int c = 0; c < 4; c++) button(grid(c, 2, 64, 40), g->opt[std::min(g->nSteps - 1, step)][c], false, 0, 0.42f);
            button(Btn{4, 206, 100, 28}, "Start Over", false, 1, 0.4f); break;
        case 'W':
            for (int k = 0; k < g->nSlots; k++) { snprintf(b, sizeof b, "%s %d%s", g->slotN, k + 1, gProb[k] >= 0 ? "!" : ""); button(grid(k, g->nSlots, 30, 40), b, gSel == k, gProb[k] >= 0 ? 2 : 0, 0.4f); }
            text("then tap the fix:", 160, 80, 0.38f, C(UI_INK), 1);
            for (int f = 0; f < 4; f++) button(grid(f, 2, 100, 34), g->fix[f], false, 0, 0.42f); break;
        case 'T': button(Btn{60, 120, 200, 70}, g->verb, false, 2, 0.8f); text("Tap when the dot is in the green zone", 160, 90, 0.4f, C(UI_INK), 1); break;
        case 'Q': text("Pick the right answer:", 160, 90, 0.45f, C(UI_INK), 1); for (int q = 0; q < 3; q++) { if (qOp == 3) snprintf(b, sizeof b, "%c", (char)qOpts[q]); else snprintf(b, sizeof b, "%d", qOpts[q]); button(grid(q, 3, 120, 44), b, false, 1, 0.6f); } break;
        }
        text("START = clock out early", 230, 224, 0.3f, C(160, 120, 140), 1);
        return;
    }
    switch (job) {
    case J_NANNY: for (int k = 0; k < 3; k++) for (int c = 0; c < 4; c++) { snprintf(b, sizeof b, "%d: %s", k + 1, CARE[c]); button(grid(k * 4 + c, 4, 40, 34), b, need[k] != 0, 0, 0.4f); } break;
    case J_GROCERY: button(Btn{20, 150, 130, 50}, "SCAN", false, 2, 0.7f); button(Btn{170, 150, 130, 50}, "WEIGH", false, 0, 0.7f); text("Scan items in the red zone. Produce = WEIGH.", 160, 100, 0.38f, C(UI_INK), 1); break;
    case J_BARISTA:
        if (showT > 0) { text("Memorize the order...", 160, 100, 0.55f, C(UI_INK), 1); break; }
        for (int c = 0; c < 4; c++) { const char* L = step == 0 ? (c < 3 ? SIZE[c] : "-") : step == 1 ? DRINK[c] : step == 2 ? MILK[c] : EXTRA[c]; button(grid(c, 4, 60, 40), L, false, 0, 0.42f); }
        button(Btn{4, 206, 100, 28}, "Start Over", false, 1, 0.4f); break;
    case J_SERVER: text("Kitchen pass:", 6, 26, 0.36f, C(UI_INK)); for (int d = 0; d < 5; d++) button(grid(d, 5, 40, 30), DISH[d], carry == d, 0, 0.38f); text("Tables:", 6, 96, 0.36f, C(UI_INK)); for (int k = 0; k < 3; k++) { snprintf(b, sizeof b, "Table %d", k + 1); button(grid(k, 3, 110, 60), b, false, 2, 0.45f); } break;
    case J_FLORIST:
        text("Flowers:", 6, 46, 0.36f, C(UI_INK)); for (int f = 0; f < 4; f++) button(grid(f, 4, 60, 36), FLOWERS[f], false, 0, 0.42f);
        for (int r = 0; r < 3; r++) { snprintf(b, sizeof b, "%s Ribbon", RIB[r]); button(grid(r, 3, 104, 30), b, step == r + 1, 0, 0.38f); }
        button(Btn{110, 150, 100, 40}, "Wrap It!", false, 2, 0.55f); button(Btn{4, 206, 100, 28}, "Start Over", false, 1, 0.4f); break;
    case J_GROOMER: if (groomStep == 1 && dirt) { rrect(120, 30, 80, 80, 12, C(200, 230, 255)); text("Scrub!", 160, 60, 0.5f, C(UI_INK), 1); } for (int k = 0; k < 4; k++) button(grid(k, 4, 150, 40), TOOLS[k], false, 0, 0.45f); break;
    case J_TEACHER:
        for (int k = 0; k < 4; k++) { snprintf(b, sizeof b, "Kid %d%s", k + 1, kidAct[k] ? "!" : ""); button(grid(k, 4, 30, 40), b, false, kidAct[k] ? 2 : 0, 0.42f); }
        for (int f = 0; f < 3; f++) button(grid(f, 3, 78, 28), FIX[f], false, 0, 0.42f);
        text("Answer the board:", 6, 134, 0.36f, C(UI_INK)); for (int q = 0; q < 3; q++) { snprintf(b, sizeof b, "%d", qOpts[q]); button(grid(q, 3, 150, 34), b, false, 1, 0.55f); }
        break;
    }
    snprintf(b, sizeof b, "START = clock out early"); text(b, 230, 224, 0.3f, C(160, 120, 140), 1);
}
