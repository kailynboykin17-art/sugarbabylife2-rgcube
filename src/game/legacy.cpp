// Legacy Challenges (see legacy.hpp). Original challenge cards inspired by the Sims community's
// legacy-challenge format: every card has a story, an aspiration, traits, a career and goals.
#include "legacy.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "goals.hpp"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
using namespace gm;

void toast(const std::string& m);

enum LKind : uint8_t { LK_MANUAL, LK_METRIC, LK_CAREER, LK_PET, LK_HOUSE, LK_ELIM, LK_VISIT, LK_LOVER, LK_LOVERWED,
                       LK_RECIPES, LK_GIFTS, LK_GIFTFOOD, LK_TV, LK_NEWHOME, LK_TRIP };   // content-pack & gallery goals (save v41)
struct LGoal { const char* text; uint8_t kind; int16_t a, b; };
struct LCard {
    const char* title; const char* series; const char* blurb;
    int8_t asp; uint8_t traits[3]; uint8_t career;
    RGB bg, dark, accent; const char* colors;
    LGoal goals[8]; uint8_t ngoals;
};

static const LCard CARDS[] = {
    {"Spooky Sweetheart", "Ghoul Glam Legacy - Gen 2",
     "The sweetest ghoul in town: pink, black and a little bit dramatic. She sings, she acts, she lights up every stage - and "
     "even though she's searching for The One, she won't give up her spotlight for anybody.",
     4, {TR_CHEERFUL, TR_ROMANTIC, TR_CREATIVE}, CR_ACTOR, {255, 226, 238}, {214, 60, 140}, {40, 30, 40}, "Pink & black",
     {{"Join a school club, then a squad", LK_METRIC, M_SQUAD, 1}, {"Creativity skill 6 (songs & scripts)", LK_METRIC, M_CREATIVE, 6},
      {"Charisma skill 6", LK_METRIC, M_CHARISMA, 6}, {"Reach the top of the Actor career", LK_CAREER, CR_ACTOR, 5},
      {"Marry your high-school sweetheart", LK_METRIC, M_MARRIED, 1}, {"Adopt a pet and dress it in pink", LK_PET, 0, 0},
      {"Have 3 kids", LK_METRIC, M_KIDS, 3}, {"Only wear pink & black (honor rule)", LK_MANUAL, 0, 0}}, 8},
    {"Second Chapter", "Struck by Love - Gen 1",
     "Twenty years with your high-school love... until you catch them cheating. You pack up the kids, move into the only "
     "place you can afford and finally finish the book you've been writing for years. You swore off love. Then the much "
     "younger new neighbor knocks on your door.",
     3, {TR_BOOKWORM, TR_LONER, TR_FAMILY}, CR_WRITER, {250, 236, 232}, {150, 80, 60}, {232, 120, 130}, "Brown & red",
     {{"Get married", LK_METRIC, M_MARRIED, 1}, {"Have 2 kids", LK_METRIC, M_KIDS, 2},
      {"Catch your spouse cheating, then move out", LK_MANUAL, 0, 0}, {"Become a Bestselling Author (Writer 4)", LK_CAREER, CR_WRITER, 4},
      {"Creativity skill 8", LK_METRIC, M_CREATIVE, 8}, {"Charisma skill 6", LK_METRIC, M_CHARISMA, 6},
      {"Fall for the young-adult next-door neighbor", LK_MANUAL, 0, 0}, {"Save up $10,000", LK_METRIC, M_MONEY, 10000}}, 8},
    {"The Outcast", "Old Money Lineage - Gen 1",
     "Your family cast you out with nothing. You're sleeping in a tent behind town, living on scraps and spite - until one night "
     "you meet Damien Vale: brooding, brilliant, and stuck in a cold marriage to the richest name in Starlight Bay. One look "
     "at you and he forgets every promise he ever made.",
     4, {TR_MEAN, TR_ROMANTIC, TR_NONE}, CR_BUSINESS, {236, 236, 240}, {30, 30, 36}, {200, 30, 50}, "Black & white",
     {{"Begin broke (starting this sets you to $0!)", LK_MANUAL, 0, 0}, {"Live rough: only basics at home (honor rule)", LK_MANUAL, 0, 0},
      {"Charisma skill 10", LK_METRIC, M_CHARISMA, 10}, {"Business career level 5... then quit", LK_CAREER, CR_BUSINESS, 5},
      {"Start dating Damien Vale", LK_LOVER, 0, 0}, {"Damien makes the first move (honor rule)", LK_MANUAL, 0, 0},
      {"Marry Damien", LK_LOVERWED, 0, 0}, {"Choose your path: does it last?", LK_MANUAL, 0, 0}}, 8},
    {"The Other Woman", "Hot Tea Legacy - Gen 2",
     "You never understood your mom's ever-changing love life - until you followed the same path. Now you're a doctor living "
     "your dream, flirting with someone at work who feels like The One... until the hospital gala, when their real spouse walks in.",
     1, {TR_AMBITIOUS, TR_OUTGOING, TR_ROMANTIC}, CR_DOCTOR, {228, 240, 252}, {60, 120, 200}, {120, 190, 240}, "Blue & white",
     {{"Get an A in school as a kid or teen", LK_METRIC, M_DEGREE, 1}, {"Host a sleepover (honor rule)", LK_MANUAL, 0, 0},
      {"Logic 6 and Charisma 6", LK_METRIC, M_LOGIC, 6}, {"Doctor career level 4 (Surgeon)", LK_CAREER, CR_DOCTOR, 4},
      {"Fall for a coworker... who's married", LK_MANUAL, 0, 0}, {"Have at least 2 kids", LK_METRIC, M_KIDS, 2},
      {"Host a family dinner or festival", LK_MANUAL, 0, 0}, {"Make 10 friends (the town confidant)", LK_METRIC, M_FRIENDS, 10}}, 8},
    {"The Player", "Sims in Love - Gen 4",
     "You grew up rich and comfy, having fun without ever getting tied down. Then you meet someone who isn't impressed by your "
     "status or your money - and that's exactly what catches your eye.",
     5, {TR_OUTGOING, TR_ROMANTIC, TR_SNEAKY}, CR_INFLUENCER, {255, 232, 244}, {220, 70, 130}, {255, 150, 200}, "Hot pink",
     {{"Charisma 6 and Creativity 4", LK_METRIC, M_CHARISMA, 6}, {"Go on 5 dates", LK_METRIC, M_DATES, 5},
      {"Hit 5,000 followers", LK_METRIC, M_FOLLOWERS, 5000}, {"Hit the club (party animal!)", LK_VISIT, R_CLUB, 0},
      {"Leave someone at the altar", LK_MANUAL, 0, 0}, {"Get caught two-timing once", LK_MANUAL, 0, 0},
      {"Fall for someone broke - prove you changed", LK_MANUAL, 0, 0}, {"Become famous (3 fame stars)", LK_METRIC, M_FAME, 3}}, 8},
    {"The Perfect Spouse", "Sims in Love - Gen 2",
     "You always wanted what your parents had. You marry right after high school, buy a house, start a family... and then it all "
     "crumbles when you find out your spouse has been cheating.",
     4, {TR_FAMILY, TR_FOODIE, TR_PERFECTIONIST}, CR_HOSTESS, {232, 242, 252}, {70, 130, 210}, {250, 200, 120}, "Sky blue",
     {{"Cooking skill 6", LK_METRIC, M_COOKING, 6}, {"Logic skill 4 (handy around the house)", LK_METRIC, M_LOGIC, 4},
      {"Get married soon after becoming a YA", LK_METRIC, M_MARRIED, 1}, {"As an adult, find out you were cheated on", LK_MANUAL, 0, 0},
      {"Enemies-to-lovers with 'the other one'", LK_MANUAL, 0, 0}, {"After the divorce, own a restaurant", LK_CAREER, CR_HOSTESS, 5},
      {"Eat out every birthday", LK_VISIT, R_RESTAURANT, 0}, {"Have a kid", LK_METRIC, M_KIDS, 1}}, 8},
    {"The Single Parent", "Sims in Love - Gen 6",
     "You made the mistake of falling for the celebrity of the moment. Once you realize a baby is on the way, you decide the "
     "best way to protect your child is to raise them alone.",
     4, {TR_FAMILY, TR_KIND, TR_FOODIE}, CR_DOCTOR, {246, 238, 228}, {140, 96, 60}, {214, 170, 110}, "Warm brown",
     {{"Cooking 6 (baking!) and Logic 6", LK_METRIC, M_COOKING, 6}, {"Fall in love with a celebrity", LK_MANUAL, 0, 0},
      {"Break up and raise your child alone", LK_METRIC, M_KIDS, 1}, {"Doctor career level 3", LK_CAREER, CR_DOCTOR, 3},
      {"No dates until your kid moves out", LK_MANUAL, 0, 0}, {"Eat something sweet every day", LK_VISIT, R_BAKERY, 0},
      {"Never tell your kid who their other parent is", LK_MANUAL, 0, 0}, {"Logic skill 6", LK_METRIC, M_LOGIC, 6}}, 8},
    {"Petal, the Garden Fairy", "Fairy Tale Legacy",
     "Petal's heart is as gentle as the flowers she tends. She finds magic in every leaf and every bloom, and believes even "
     "the smallest kind act can make the world more beautiful. Grow your own fairy tale.",
     6, {TR_CHEERFUL, TR_ACTIVE, TR_KIND}, CR_GARDENER, {238, 248, 232}, {110, 170, 90}, {246, 170, 200}, "Pink & green",
     {{"Creativity skill 6 (flower arranging)", LK_METRIC, M_CREATIVE, 6}, {"Fitness skill 4 (natural living)", LK_METRIC, M_FITNESS, 4},
      {"Become a Garden Guru (Gardener 5)", LK_CAREER, CR_GARDENER, 5}, {"Find 12 magic treasures (collectibles)", LK_METRIC, M_COLLECT, 12},
      {"Marry someone who loves nature", LK_METRIC, M_MARRIED, 1}, {"Have at least 2 children", LK_METRIC, M_KIDS, 2},
      {"Plant a new seed every week (honor rule)", LK_MANUAL, 0, 0}, {"Spend a day in Rosebud Park", LK_VISIT, R_PARK, 0}}, 8},
    {"Bad Girls House", "Reality Show Challenge",
     "Seven girls, one house, zero chill. Move them in, let them do their thing and stir up the drama. Spa days, club nights, "
     "dinners out... and whoever fights too much or takes it too far gets sent home.",
     1, {TR_HOTHEAD, TR_GLAMOROUS, TR_OUTGOING}, CR_NONE, {252, 230, 244}, {150, 30, 100}, {255, 120, 200}, "Devil pink",
     {{"7 girls living in one house", LK_HOUSE, 7, 0}, {"Spa day", LK_VISIT, R_SPA, 0}, {"Club night", LK_VISIT, R_CLUB, 0},
      {"Girls' dinner out", LK_VISIT, R_RESTAURANT, 0}, {"Beach trip", LK_VISIT, R_BEACH, 0},
      {"Send the first girl home", LK_ELIM, 1, 0}, {"Send 3 girls home", LK_ELIM, 3, 0}, {"Crown the last girl standing", LK_ELIM, 6, 0}}, 8},
    // multi-generation legacies: Sunday Supper (8), Colorful Chaos (10), Era by Era (8), Pretty Little Problems (10)
#include "legacy_series.inc"
};
static const int NCARDS = sizeof(CARDS) / sizeof(CARDS[0]);

static bool elimMode = false;
static int confirmStart = -1;
static int confirmQuit = 0;

const char* legacyMenuLabel() {
    static char b[48];
    if (G.leg.active >= 0 && G.leg.active < NCARDS) snprintf(b, sizeof b, "Legacy: Gen %d", G.leg.gen);
    else snprintf(b, sizeof b, "Legacy Challenges");
    return b;
}

static int legHouseCount() { int n = 0; for (const Sim& s : G.sims) if (s.household) n++; return n; }
static bool loverOk() { return G.leg.lover >= 0 && G.leg.lover < (int)G.sims.size() && G.leg.lover != G.mc; }

static bool goalMet(const LGoal& g, int i) {
    if (G.leg.done & (1u << i)) return true;
    const Sim& me = MC();
    switch (g.kind) {
    case LK_METRIC:
        if (g.a == M_CHARISMA && g.b == 6 && strstr(g.text, "Creativity")) return goalMetric(M_CHARISMA) >= 6 && goalMetric(M_CREATIVE) >= 4;
        if (g.a == M_LOGIC && strstr(g.text, "Charisma")) return goalMetric(M_LOGIC) >= g.b && goalMetric(M_CHARISMA) >= g.b;
        if (g.a == M_COOKING && strstr(g.text, "Logic")) return goalMetric(M_COOKING) >= g.b && goalMetric(M_LOGIC) >= g.b;
        return goalMetric(g.a) >= g.b;
    case LK_CAREER: return me.career == g.a && me.careerLevel + 1 >= g.b;
    case LK_PET: return !G.pets.empty();
    case LK_HOUSE: return legHouseCount() >= g.a;
    case LK_ELIM: return G.leg.elim >= g.a;
    case LK_LOVER: return loverOk() && rel(G.mc, G.leg.lover).status >= RS_DATING && rel(G.mc, G.leg.lover).status != RS_FAMILY;
    case LK_LOVERWED: return loverOk() && rel(G.mc, G.leg.lover).status == RS_MARRIED;
    case LK_RECIPES: return __builtin_popcountll(G.recipesMade) + __builtin_popcountll(G.recipesMade2) >= g.a;
    case LK_GIFTS: { int n = 0; for (auto& x : G.giftLog) if (x.item < 100) n++; return n >= g.a; }
    case LK_GIFTFOOD: { int n = 0; for (auto& x : G.giftLog) if (x.item >= 100) n++; return n >= g.a; }
    case LK_TV: return __builtin_popcountll(G.tvWatched) >= g.a;
    case LK_NEWHOME: return G.homeId >= 2 || G.leg.homeAtStart != G.homeId;
    case LK_TRIP: return G.trips > G.leg.tripsAtStart;
    default: return false;   // manual & visits are latched by taps / trips
    }
}
static int doneCount(int c) { int n = 0; for (int i = 0; i < CARDS[c].ngoals; i++) if (G.leg.done & (1u << i)) n++; return n; }

void legacyHourly() {
    int c = G.leg.active;
    if (c < 0 || c >= NCARDS || G.sims.empty()) return;
    for (int i = 0; i < CARDS[c].ngoals; i++) {
        const LGoal& g = CARDS[c].goals[i];
        if (G.leg.done & (1u << i) || g.kind == LK_MANUAL || g.kind == LK_VISIT) continue;
        if (goalMet(g, i)) { G.leg.done |= 1u << i; toast(std::string("Legacy goal done: ") + g.text + "!"); sfxYay(); }
    }
}
void legacyNotify(int ev, int arg) {
    int c = G.leg.active;
    if (c < 0 || c >= NCARDS || ev != GE_TRAVEL) return;
    for (int i = 0; i < CARDS[c].ngoals; i++) {
        const LGoal& g = CARDS[c].goals[i];
        bool hit = g.kind == LK_VISIT && (g.a == arg || (g.a == R_RESTAURANT && (arg == R_DINER || arg == R_SUSHI)) || (g.a == R_BAKERY && arg == R_CAFE));
        if (hit && !(G.leg.done & (1u << i))) { G.leg.done |= 1u << i; toast(std::string("Legacy goal done: ") + g.text + "!"); sfxYay(); }
    }
}

static void addToLiving(int idx, int k) {
    Actor a; a.kind = 0; a.id = (uint16_t)idx; a.x = 80.f + (k % 6) * 46; a.tx = a.x; a.y = 210.f + (k % 2) * 10;
    G.rooms[G.house][R_LIVING].actors.push_back(a);
}
static void removeFromHome(int idx) {
    for (int h = 0; h < 2; h++) for (int r = 0; r < R_COUNT; r++) {
        auto& v = G.rooms[h][r].actors;
        v.erase(std::remove_if(v.begin(), v.end(), [&](const Actor& a) { return a.kind == 0 && a.id == idx; }), v.end());
    }
}

static void startCard(int c) {
    const LCard& k = CARDS[c];
    LegacyState& L = G.leg;
    L.active = (int8_t)c; L.done = 0; L.elim = 0; L.startDay = G.day; L.lover = -1; L.homeAtStart = G.homeId; L.tripsAtStart = G.trips;
    Sim& me = MC();
    if (me.stage >= ST_CHILD) for (int t = 0; t < 3; t++) if (k.traits[t] != TR_NONE) me.traits[t] = k.traits[t];
    if (k.asp >= 0 && aspAllowed(k.asp)) aspChoose(k.asp);
    if (c == 2) {   // The Outcast: broke, plus the Vales arrive in town
        G.money = 0;
        if (G.sims.size() < 118) {
            Sim d = randomSim(ST_ADULT, 1); d.name = "Damien Vale"; d.bio = "old money, older secrets"; d.household = false;
            d.look.top = {30, 30, 36}; d.look.bottom = {30, 30, 36}; d.look.topStyle = TS_BLAZER; d.look.bt = BT_PANTS; d.look.shoeStyle = SS_LOAFERS; d.outfit = 0;
            d.traits[0] = TR_ROMANTIC; d.traits[1] = TR_SNOB; d.traits[2] = TR_AMBITIOUS; d.career = CR_BUSINESS; d.careerLevel = 4;
            Sim w = randomSim(ST_ADULT, 0); w.name = "Celeste Vale"; w.bio = "chair of every charity gala"; w.household = false;
            w.traits[0] = TR_SNOB; w.traits[1] = TR_GLAMOROUS; w.traits[2] = TR_JEALOUS;
            G.sims.push_back(d); int di = (int)G.sims.size() - 1;
            G.sims.push_back(w); int wi = (int)G.sims.size() - 1;
            Rel& r = rel(di, wi); r.status = RS_MARRIED; r.fr = 10; r.ro = 20; r.met = true;
            L.lover = (int16_t)di;
        }
        toast("You're broke and on your own. Find Damien Vale around town...");
    } else if (c == 8) {   // Bad Girls: fill the house up to 7
        int k2 = 0;
        static const char* GN[] = {"Destiny", "Mercedes", "Tiara", "Jazmine", "Chanel", "Bambi", "Aaliyah", "Krystal"};
        while (legHouseCount() < 7 && G.sims.size() < 118) {
            Sim g = randomSim(ST_YA, 0); g.household = true; g.name = GN[(k2 + G.day) % 8]; g.bio = "here for the drama";
            G.sims.push_back(g); int gi = (int)G.sims.size() - 1;
            Rel& r = rel(G.mc, gi); r.met = true; r.fr = 10;
            addToLiving(gi, k2++);
        }
        toast("The girls have moved in. Let the drama begin!");
    } else toast(std::string("Legacy started: ") + k.title + "!");
    sfxYay();
}

static void passTorch() {
    LegacyState& L = G.leg;
    int c = L.active;
    if (c < 32) L.cleared |= 1u << c; else L.cleared2 |= 1ull << (c - 32);
    G.money += 1000 * L.gen; G.satisfaction += 1500;
    char b[160]; snprintf(b, sizeof b, "Generation %d complete! +$%d and 1500 points. The legacy lives on...", L.gen, 1000 * L.gen);
    toast(b);
    // the heir: the oldest kid still at home takes over
    int heir = -1;
    for (size_t i = 0; i < G.sims.size(); i++)
        if ((int)i != G.mc && G.sims[i].household && G.sims[i].stage >= ST_CHILD && G.sims[i].stage <= ST_YA && (heir < 0 || G.sims[i].stage > G.sims[heir].stage)) heir = (int)i;
    if (heir >= 0 && c != 8) {
        G.mc = heir;
        for (int r = 0; r < R_COUNT; r++) for (const Actor& a : G.rooms[G.house][r].actors) if (a.kind == 0 && a.id == heir) G.room = r;
    }
    L.gen++; L.active = -1; L.done = 0; L.elim = 0; L.lover = -1;
    L.view = (uint8_t)((c + 1) % NCARDS);
    sfxYay();
}

void legacyOpen() {
    elimMode = false; confirmStart = -1; confirmQuit = 0;
    if (G.leg.active >= 0 && G.leg.active < NCARDS) G.leg.view = (uint8_t)G.leg.active;
    if (G.leg.view >= NCARDS) G.leg.view = 0;
}

static Btn goalB(int i) { return Btn{6, 26 + i * 22.f, 308, 21}; }
static const Btn B_PREV{4, 210, 36, 26}, B_NEXT{42, 210, 36, 26}, B_MAIN{80, 210, 120, 26}, B_EXTRA{202, 210, 58, 26}, B_BACK{262, 210, 54, 26};

static std::vector<int> elimList() {
    std::vector<int> v;
    for (size_t i = 0; i < G.sims.size(); i++) if ((int)i != G.mc && G.sims[i].household && G.sims[i].isAdult()) v.push_back((int)i);
    return v;
}

bool legacyUpdate(const Input& in) {
    LegacyState& L = G.leg;
    int v = L.view % NCARDS;
    const bool mine = L.active == v;
    if (elimMode) {
        if (in.down & KEY_B) { elimMode = false; return true; }
        if (!in.touch) return true;
        std::vector<int> e = elimList();
        for (int i = 0; i < (int)e.size() && i < 8; i++) if (goalB(i).in(in.tp)) {
            Sim& s = G.sims[e[i]];
            s.household = false; removeFromHome(e[i]); L.elim++;
            toast(s.name + " has been sent home. Pack your bags!"); sfxNo();
            elimMode = false; return true;
        }
        if (B_BACK.in(in.tp)) { elimMode = false; sfxTap(); }
        return true;
    }
    if (in.down & (KEY_B | KEY_START)) return false;
    if (in.down & KEY_LEFT) { L.view = (uint8_t)((v + NCARDS - 1) % NCARDS); confirmStart = -1; sfxTap(); }
    if (in.down & KEY_RIGHT) { L.view = (uint8_t)((v + 1) % NCARDS); confirmStart = -1; sfxTap(); }
    if (!in.touch) return true;
    if (B_BACK.in(in.tp)) { sfxTap(); return false; }
    if (B_PREV.in(in.tp)) { L.view = (uint8_t)((v + NCARDS - 1) % NCARDS); confirmStart = -1; confirmQuit = 0; sfxTap(); return true; }
    if (B_NEXT.in(in.tp)) { L.view = (uint8_t)((v + 1) % NCARDS); confirmStart = -1; confirmQuit = 0; sfxTap(); return true; }
    const LCard& k = CARDS[v];
    if (mine) {
        for (int i = 0; i < k.ngoals; i++) if (goalB(i).in(in.tp)) {
            if (k.goals[i].kind == LK_MANUAL || k.goals[i].kind == LK_VISIT) { L.done ^= 1u << i; sfxTap(); }
            else { toast("This one checks itself off when you do it!"); sfxNo(); }
            return true;
        }
        if (B_MAIN.in(in.tp)) {
            if (doneCount(v) >= k.ngoals) passTorch();
            else if (v == 8) { if (elimList().empty()) { toast("Nobody left to send home!"); sfxNo(); } else { elimMode = true; sfxTap(); } }
            else { Sim& me = MC(); me.look.top = k.bg; me.look.bottom = k.dark; me.look.shoes = k.accent; me.outfit = 0; toast(std::string("Dressed in the legacy colors: ") + k.colors + "!"); sfxYay(); }
            return true;
        }
        if (B_EXTRA.in(in.tp)) {
            if (++confirmQuit >= 2) { L.active = -1; L.done = 0; confirmQuit = 0; toast("You quit the challenge. No shame!"); sfxNo(); }
            else { toast("Tap Quit again to give up this challenge."); sfxTap(); }
            return true;
        }
        return true;
    }
    if (B_MAIN.in(in.tp)) {
        if (confirmStart != v) { confirmStart = v; toast(v == 2 ? "Heads up: this one starts you at $0! Tap again to start." : "Your traits & life goal will change. Tap again to start!"); sfxTap(); }
        else { confirmStart = -1; startCard(v); }
    }
    return true;
}

// ---------------------------------------------------------------- drawing
static u32 CC(RGB c, int a = 255) { return C2D_Color32(c.r, c.g, c.b, a); }

void legacyDrawTop(float t) {
    const LegacyState& L = G.leg;
    const LCard& k = CARDS[L.view % NCARDS];
    rect(0, 0, 400, 240, CC(k.bg));
    for (int i = 0; i < 9; i++) heart(20 + i * 46, 12 + (i % 2) * 220, 5, CC(k.accent, 60));
    outline(4, 4, 392, 232, 3, CC(k.dark));
    rect(4, 4, 392, 20, CC(k.dark));
    char top[96]; snprintf(top, sizeof top, "%s   |   Card %d/%d", k.series, L.view % NCARDS + 1, NCARDS);
    text(top, 200, 7, 0.4f, C2D_Color32(255, 255, 255, 255), 1);
    textOutline(k.title, 200, 28, 0.85f, CC(k.dark), C2D_Color32(255, 255, 255, 255), 1, 1.5f);
    // the two colour swatches
    circle(372, 44, 8, CC(k.dark)); circle(386, 50, 8, CC(k.accent));
    std::vector<std::string> lines = wrapText(k.blurb, 0.38f, 372);
    for (size_t i = 0; i < lines.size() && i < 5; i++) text(lines[i].c_str(), 14, 62 + i * 13.f, 0.38f, C2D_Color32(60, 44, 52, 255));
    // aspiration / traits / career boxes
    auto box = [&](float x, float w, const char* head, const std::vector<std::string>& body) {
        rrect(x, 134, w, 74, 6, CC(k.dark)); rrect(x + 3, 152, w - 6, 53, 4, C2D_Color32(255, 255, 255, 240));
        text(head, x + w / 2, 137, 0.42f, C2D_Color32(255, 255, 255, 255), 1);
        for (size_t i = 0; i < body.size(); i++) text(body[i].c_str(), x + w / 2, 157 + i * 15.f, 0.38f, CC(k.dark), 1);
    };
    box(10, 112, "Aspiration", {k.asp >= 0 ? ASP_NAMES[k.asp] : "Any"});
    std::vector<std::string> tr; for (int i = 0; i < 3; i++) if (k.traits[i] != TR_NONE) tr.push_back(TRAIT_NAMES[k.traits[i]]); if (tr.size() < 3) tr.push_back("+ Your Choice");
    box(126, 146, "Traits", tr);
    box(276, 114, "Career", {k.career != CR_NONE ? CAREERS[k.career].name : "None (drama!)", std::string("Colors: ") + k.colors});
    // status strip
    char st[96];
    if (L.active == L.view % NCARDS) snprintf(st, sizeof st, "Generation %d - %d/%d goals done", L.gen, doneCount(L.active), k.ngoals);
    else if ((L.view % NCARDS) < 32 ? (L.cleared & (1u << (L.view % NCARDS))) : (L.cleared2 >> (L.view % NCARDS - 32) & 1)) snprintf(st, sizeof st, "Completed in an earlier generation!");
    else if (L.active >= 0) snprintf(st, sizeof st, "You're playing \"%s\" right now", CARDS[L.active].title);
    else snprintf(st, sizeof st, "Tap Start to play this generation");
    rrect(60, 213, 280, 18, 8, CC(k.dark, 220));
    text(st, 200, 215, 0.4f, C2D_Color32(255, 255, 255, 255), 1);
    sparkle(30 + std::sin(t * 2) * 4, 220, 5, CC(k.accent));
}

void legacyDrawBottom(float t) {
    bottomBackdrop(t);
    const LegacyState& L = G.leg;
    int v = L.view % NCARDS;
    const LCard& k = CARDS[v];
    const bool mine = L.active == v;
    if (elimMode) {
        text("Who's going home?", 160, 4, 0.55f, C2D_Color32(255, 200, 230, 255), 1);
        std::vector<int> e = elimList();
        for (int i = 0; i < (int)e.size() && i < 8; i++) {
            const Sim& s = G.sims[e[i]];
            std::string lab = s.name + "  (" + TRAIT_NAMES[s.traits[0] < TR_COUNT ? s.traits[0] : 0] + ")";
            button(goalB(i), lab.c_str(), false, 3, 0.4f);
        }
        button(B_BACK, "Back", false, 1, 0.4f);
        return;
    }
    text(mine ? "Your Goals (tap to tick)" : "Goals", 160, 4, 0.5f, C2D_Color32(255, 220, 236, 255), 1);
    for (int i = 0; i < k.ngoals; i++) {
        Btn b = goalB(i);
        bool d = mine && (L.done & (1u << i));
        rrect(b.x, b.y, b.w, b.h, 5, d ? C2D_Color32(255, 236, 246, 240) : C2D_Color32(255, 255, 255, 200));
        rrect(b.x + 4, b.y + 4, 13, 13, 3, CC(k.dark));
        rrect(b.x + 6, b.y + 6, 9, 9, 2, d ? CC(k.accent) : C2D_Color32(255, 255, 255, 255));
        if (d) heart(b.x + 10.5f, b.y + 10.5f, 3.2f, C2D_Color32(255, 255, 255, 255));
        bool manual = k.goals[i].kind == LK_MANUAL || k.goals[i].kind == LK_VISIT;
        text(k.goals[i].text, b.x + 22, b.y + 4, 0.38f, d ? C2D_Color32(150, 120, 140, 255) : C2D_Color32(70, 40, 60, 255));
        if (!manual) text("auto", b.x + b.w - 4, b.y + 5, 0.3f, C2D_Color32(170, 140, 160, 255), 2);
    }
    button(B_PREV, "<", false, 3, 0.5f);
    button(B_NEXT, ">", false, 3, 0.5f);
    const char* main;
    if (mine) main = doneCount(v) >= k.ngoals ? "Pass the Torch!" : v == 8 ? "Send Home..." : "Wear Colors";
    else main = confirmStart == v ? "Sure? Start!" : L.active >= 0 ? "Switch to This" : "Start This";
    button(B_MAIN, main, mine && doneCount(v) >= k.ngoals, 0, 0.42f);
    if (mine) button(B_EXTRA, confirmQuit ? "Sure?" : "Quit", false, 2, 0.4f);
    button(B_BACK, "Back", false, 1, 0.4f);
}
