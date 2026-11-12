#include "designer.hpp"
#include "lifeplus2.hpp"
#include "outings.hpp"
#include "farm.hpp"
#include "levels.hpp"
#include "life3.hpp"
#include "settings2.hpp"
#include "famdrama.hpp"
#include "houseview.hpp"
#include "choices.hpp"
#include "emote.hpp"
#include "dynasty.hpp"
#include "photomode.hpp"
#include "rivals.hpp"
#include "dailylife.hpp"
#include "town2.hpp"
#include "growup.hpp"
#include "kidlife.hpp"
#include "oddjobs.hpp"
#include "tween.hpp"
#include "sevents.hpp"
#include "staff.hpp"
#include "deeplife.hpp"
#include "lifeext.hpp"
#include "funpark.hpp"
#include "spicy.hpp"
#include "hotdate.hpp"
#include "nightlife.hpp"
#include "villa.hpp"
#include "clubs.hpp"
#include "filters.hpp"
int g_phoneJump = 0; int g_seCalDay = 0; static bool clubsFromPhone = false, galleryFromPhone = false;
#include "challenge.hpp"
#include "content.hpp"
#include "gallery.hpp"
#include "kpop.hpp"
#include "lockup.hpp"
// Dream Dollhouse: Diva Life - a Y2K girly life sim for Nintendo 3DS homebrew.
#include "bgs.hpp"
#include "worldsim.hpp"
#include "cc.hpp"
#include "sprites.hpp"
#include "travelcourt.hpp"
#include "health.hpp"
#include "lifestory.hpp"
#include "common.hpp"
#include "draw.hpp"
#include "catalog.hpp"
#include "people.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "social.hpp"
#include "phone.hpp"
#include "cook.hpp"
#include "minigames.hpp"
#include "styling.hpp"
#include "jobs.hpp"
#include "squads.hpp"
#include "goals.hpp"
#include "extras.hpp"
#include "crime.hpp"
#include "cutscene.hpp"
#include "heist.hpp"
#include "chatter.hpp"
#include "district.hpp"
#include "oc.hpp"
#include "serve.hpp"
#include "shopgames.hpp"
#include "moviemaker.hpp"
#include "story.hpp"
#include "lovebug.hpp"
#include "life.hpp"
#include "seasons.hpp"
#include "otome.hpp"
#include "legacy.hpp"
#include "director.hpp"
#include "economy.hpp"
#include "chat.hpp"
#include "drama.hpp"
#include "cas.hpp"
#include "audio.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <deque>
#include <cstring>
#include <cctype>

using namespace col;

Game G;

static int menuPage = 0;   // 0 = settings & life, 1 = game modes
enum Mode { M_TITLE, M_CAS, M_PLAY, M_BUILD, M_PHONE, M_MENU, M_COOK, M_MG, M_STYLE, M_DIARY, M_CONTROLS, M_SHIFT, M_GLAM, M_CUTSCENE, M_HEIST, M_SERVE, M_STORY, M_OTOME, M_LEGACY, M_DIRECTOR, M_SHOPGAME, M_MOVIEMAKER, M_LIFEBOOK, M_CC, M_WORLD, M_SLOTS, M_KPOP, M_GALLERY, M_CONTENT, M_CHALLENGE, M_CLUBS, M_VILLA, M_SPICY, M_ODDJOB, M_PHOTO, M_RIVALS, M_HOUSE, M_MINI, M_DESIGN };
static void slotsOpen(int m);
static Mode mode = M_TITLE, controlsReturn = M_TITLE;
static float T = 0;
static std::string upperStr(const std::string& s) { std::string o = s; for (char& c : o) if (c >= 'a' && c <= 'z') c = (char)(c - 32); return o; }
static int speedMul = 1;
static bool quitApp = false;

// ===================================================================== services
static std::deque<std::string> toasts, toastLog;
static float toastT = 0;
void toast(const std::string& m) {
    if (m.empty()) return;
    if (!toasts.empty() && toasts.back() == m) return;   // no double pop-ups
    toastLog.push_back(m); if (toastLog.size() > 40) toastLog.pop_front();
    emoteFromText(G.mc, m);
    if (toasts.size() >= 12) { toasts.pop_back(); toasts.push_back("+ more news - see Daily Life > Notifications"); return; }
    toasts.push_back(m);
}
std::vector<std::string> toastHistory() { return std::vector<std::string>(toastLog.rbegin(), toastLog.rend()); }

struct Particle { float x, y, vx, vy, life; RGB c; int kind; };
static std::vector<Particle> parts;
void fxBurst(float x, float y, int n) {
    static const RGB cs[] = {HOT, GOLD, BLUSH, CREAM, LILAC};
    for (int i = 0; i < n && parts.size() < 200; i++)
        parts.push_back({x, y, (frand() - 0.5f) * 220, -frand() * 200 - 40, 1.2f + frand(), cs[irand(5)], irand(3)});
}

static Actor* actorOf(int simIdx) {
    for (Actor& a : curRoom().actors) if (a.kind == 0 && a.id == simIdx) return &a;
    return nullptr;
}
void say(int simIdx, const std::string& line) {
    Actor* a = actorOf(simIdx);
    if (a) { a->say = line; a->sayT = 2.5f; }
}

static Actor* mcActor() {
    Actor* a = actorOf(G.mc);
    if (a) return a;
    for (int r = 0; r < R_COUNT; r++)
        for (Actor& x : G.rooms[G.house][r].actors)
            if (x.kind == 0 && x.id == G.mc) { G.room = r; return &x; }
    Actor n; n.kind = 0; n.id = (uint16_t)G.mc; n.x = 200; n.tx = 200; n.y = 222;
    curRoom().actors.push_back(n);
    return &curRoom().actors.back();
}

// ===================================================================== tutorial
struct TutStep { const char* title; const char* body; const char* key; };
static const TutStep TUT[] = {
    {"Welcome, Diva!", "This is you! Let's learn the 3DS controls. Press A to begin.", "A"},
    {"Walk Around", "Use the Circle Pad or the +Control Pad to walk around the room.", "+"},
    {"Change Rooms", "Press L or R to visit the other rooms in your house.", "L R"},
    {"Touch Screen", "The bottom screen is a touch screen! Tap it with your stylus.", "Tap"},
    {"Interact", "Walk up to furniture or a person and press A. (Or tap them in the Nearby list!)", "A"},
    {"Go Back", "Press B to close menus or go back. B always means back!", "B"},
    {"Your Phone", "Press X to open your phone: Texts, Glam Date, InstaGlam, the Map & more.", "X"},
    {"Put It Away", "Press B (or X again) to close the phone.", "B"},
    {"Build Mode", "Press Y to open Build mode and decorate your dream house.", "Y"},
    {"Building", "+Pad moves items, A places, Y recolors, X sells, L/R pick items. Press B to leave.", "B"},
    {"Fast Forward", "Press SELECT to speed up time (1x / 3x). Great for sleeping!", "SEL"},
    {"Menu", "Press START for the menu: Save, Controls, Lights, Music & more. Press START to close it.", "STA"},
    {"You're Ready!", "Keep your needs up, make friends, find love, get famous. Press A to start living!", "A"},
};
static const int TUT_N = sizeof(TUT) / sizeof(TUT[0]);
static int tut = -1;
static float tutMove = 0;
static const Btn bTutSkip{232, 0, 88, 22};

static void tutAdvance() { tut++; sfxYay(); if (tut >= TUT_N) { tut = -1; toast("Tutorial complete! Have fun!"); } }

// returns true if the tutorial consumed this input
static bool tutorialInput(const Input& in) {
    if (tut < 0) return false;
    if (in.touch && bTutSkip.in(in.tp) && (mode == M_PLAY)) { tut = -1; toast("Tutorial skipped (replay it from the Menu)"); return true; }
    switch (tut) {
    case 0: case 12: if (in.down & KEY_A) { tutAdvance(); return true; } return mode == M_PLAY && (in.down & (KEY_X | KEY_Y | KEY_START));
    case 1:
        if (in.held & (KEY_DUP | KEY_DDOWN | KEY_DLEFT | KEY_DRIGHT | KEY_CPAD_LEFT | KEY_CPAD_RIGHT | KEY_CPAD_UP | KEY_CPAD_DOWN)) tutMove += in.dt;
        if (tutMove > 0.8f) tutAdvance();
        return false;
    case 2: if (in.down & (KEY_L | KEY_R)) tutAdvance(); return false;
    case 3: if (in.touch) tutAdvance(); return false;
    case 4: if (mode == M_PLAY && (in.down & KEY_A)) tutAdvance(); return false;
    case 5: if (in.down & KEY_B) tutAdvance(); return false;
    case 6: if (in.down & KEY_X) tutAdvance(); return false;
    case 7: if (mode == M_PHONE && (in.down & (KEY_B | KEY_X))) tutAdvance(); return false;
    case 8: if (in.down & KEY_Y) tutAdvance(); return false;
    case 9: if (mode == M_BUILD && (in.down & KEY_B)) tutAdvance(); return false;
    case 10: if (in.down & KEY_SELECT) tutAdvance(); return false;
    case 11: if (mode == M_MENU && (in.down & KEY_START)) tutAdvance(); return false;
    }
    return false;
}

static void drawKeyIcon(const char* k, float x, float y) {
    circle(x, y, 14, C(GOLD)); circle(x, y, 12, C(UI1));
    text(k, x, y - 8, std::strlen(k) > 2 ? 0.36f : 0.5f, C(CREAM), 1);
}

static void drawTutorialTop() {
    if (tut < 0) return;
    const TutStep& s = TUT[tut];
    const float Y0 = 32;
    rrect(6, Y0, 388, 66, 10, C(GOLD));
    rrect(8, Y0 + 2, 384, 62, 9, C(UI1));
    drawKeyIcon(s.key, 32, Y0 + 32);
    char b[32];
    snprintf(b, sizeof b, "Tutorial %d/%d", tut + 1, TUT_N);
    text(b, 384, Y0 + 5, 0.36f, C(CHAMP), 2);
    text(s.title, 56, Y0 + 6, 0.55f, C(BLUSH));
    auto lines = wrapText(s.body, 0.42f, 324);
    for (size_t i = 0; i < lines.size() && i < 3; i++) text(lines[i].c_str(), 56, Y0 + 26 + i * 13.f, 0.42f, C(CREAM));
    sparkle(380, Y0 + 52, 3 + std::fabs(std::sin(T * 4)) * 3, C(GOLD));
}

// ===================================================================== 3DS controls diagram
static void drawConsole(float t) {
    grad(0, 0, TOP_W, TOP_H, C(UIM), C(UI1));
    textShadow("3DS Controls", 200, 4, 0.65f, C(BLUSH), C(BLACK), 1);
    // lower half of a pink 3DS
    rrect(100, 40, 200, 150, 14, C(HOT));
    rrect(104, 44, 192, 142, 12, C(255, 120, 170));
    rect(140, 52, 120, 90, C(UI1)); rect(144, 56, 112, 82, C(BLUSH));
    text("Touch", 200, 84, 0.45f, C(UI1), 1);
    circle(122, 66, 12, C(80, 80, 90)); circle(122, 66, 9, C(150, 150, 160));                 // circle pad
    rect(115, 98, 14, 4, C(BLACK)); rect(120, 93, 4, 14, C(BLACK));                         // d-pad
    float bx = 278, by = 80;
    static const char* L[4] = {"X", "A", "B", "Y"};
    static const float off[4][2] = {{0, -10}, {10, 0}, {0, 10}, {-10, 0}};
    for (int i = 0; i < 4; i++) { circle(bx + off[i][0], by + off[i][1], 5.5f, C(CREAM)); text(L[i], bx + off[i][0], by + off[i][1] - 6, 0.36f, C(UI1), 1); }
    rrect(176, 150, 20, 6, 3, C(CREAM)); rrect(204, 150, 20, 6, 3, C(CREAM));
    rrect(100, 32, 50, 10, 4, C(BERRY)); rrect(250, 32, 50, 10, 4, C(BERRY));
    // labels
    u32 lc = C(CREAM), hc = C(GOLD);
    auto lab = [&](float x0, float y0, float x1, float y1, const char* k, const char* d, int al) {
        line(x0, y0, x1, y1, 1, C(CHAMP, 160));
        text(k, x1 + (al == 2 ? -2 : 2), y1 - 14, 0.4f, hc, al);
        text(d, x1 + (al == 2 ? -2 : 2), y1 - 2, 0.36f, lc, al);
    };
    lab(112, 62, 90, 60, "Circle Pad", "walk / move", 2);
    lab(118, 100, 90, 110, "+Control Pad", "walk / move items", 2);
    lab(125, 37, 90, 30, "L / R", "change rooms", 2);
    lab(275, 37, 310, 30, "L / R", "switch tabs", 0);
    lab(288, 80, 310, 66, "A", "interact / confirm", 0);
    lab(278, 90, 310, 104, "B", "back / cancel", 0);
    lab(278, 70, 310, 140, "X / Y", "Phone / Build", 0);
    lab(186, 156, 120, 214, "SELECT", "time speed 1x/3x", 2);
    lab(214, 156, 280, 214, "START", "menu & save", 0);
    sparkle(200, 196, 4 + std::sin(t * 3) * 2, C(GOLD));
}

static void drawControlsBottom(float t) {
    bottomBackdrop(t);
    text("Quick Guide", 160, 4, 0.6f, C(BLUSH), 1);
    static const char* lines[] = {
        "LIVING:  Pad = walk   A = interact nearby",
        "L/R = next room   X = Phone   Y = Build",
        "SELECT = speed   START = menu",
        "BUILD:  tap to buy   Pad = move item",
        "A = place   Y = recolor   X = sell   L/R = pick",
        "PHONE:  tap apps   Up/Down = scroll",
        "Glam Date: L = nope   R = like",
        "COOKING: tap fridge, oven, stove & serve",
        "Watch your needs! A happy sim does better.",
    };
    for (int i = 0; i < 9; i++) text(lines[i], 10, 30 + i * 18.f, 0.42f, C(i % 3 == 0 ? GOLD : CREAM));
    button(Btn{120, 204, 80, 30}, "Back", false, 1, 0.5f);
}

// ===================================================================== interactions
enum ItemAct {
    IA_SLEEP, IA_NAP, IA_RELAX, IA_TV, IA_READ, IA_NOVEL, IA_HOMEWORK, IA_WEB, IA_MAKEUP, IA_SPEECH, IA_OUTFIT,
    IA_COOK, IA_SNACK, IA_BATH, IA_TOYS, IA_DANCE, IA_PARTY, IA_MUSIC, IA_ARCADE, IA_WATER, IA_ADMIRE, IA_LIGHTS,
    IA_DIARY, IA_ROCK, IA_SUNBATHE, IA_PLANOUTFIT, IA_BBQ,
    IA_SIT, IA_GETUP, IA_SHOWER, IA_TOILET, IA_BRUSH, IA_PILLOW, IA_MAGIC, IA_CHECKLOOK, IA_CONTENT, IA_SOCIALAPP,
    IA_JOBS, IA_STUDY, IA_QUICKMEAL, IA_RELAXBED,
    IA_PRETEND, IA_TEAPARTY, IA_FORT, IA_CRAYONS, IA_STUDYCALL, IA_TUTORIAL, IA_SLEEPOVER, IA_LEMONADE, IA_VIRALDANCE, IA_GLAMSTATION, IA_HACK, IA_STREAMGAME, IA_EREAD, IA_MUSICPOD, IA_LAPBLOG, IA_LAPBINGE, IA_LAPHOMEWORK
};
static void standingBoost(int c, int d) { if (c >= 0 && c < CQ_COUNT) G.cliqueStand[c] = (uint8_t)clampv((int)G.cliqueStand[c] + d, 0, 100); }
static Actor* mcActor();
static int partnerHere() {   // an eligible household partner in this room, or -1
    int p = partnerOf(G.mc);
    if (p < 0 || !G.sims[p].household) return -1;
    for (const Actor& a : curRoom().actors) if (a.kind == 0 && a.id == p) return p;
    return -1;
}
enum PetAct { PA_PET, PA_FEED, PA_PLAY };
enum VenueAct {
    VA_DINNER, VA_WORK, VA_SHOP, VA_STYLETIPS, VA_CLASS, VA_LATTE, VA_MASSAGE, VA_MAKEOVER, VA_DANCE, VA_MOCKTAIL,
    VA_SWIM, VA_TAN, VA_SCHOOL, VA_CHEER, VA_PET, VA_MUFFIN, VA_GOHOME,
    VA_SNACK, VA_PHOTOBOOTH, VA_JOG, VA_PICNIC, VA_DUCKS, VA_PAINT, VA_MOVIE_ROM, VA_MOVIE_HOR, VA_MOVIE_COM, VA_POPCORN,
    VA_SKATE, VA_COCOA, VA_MG_KARAOKE, VA_MG_VOLLEY, VA_MG_NAILS, VA_MG_SMOOTHIE, VA_CONTEST, VA_CROWN, VA_WORKAUTO,
    VA_PLAYGROUND, VA_SKATEPARK, VA_SANDCASTLE, VA_HANGOUT, VA_TOKENS, VA_LIBRARY, VA_CLUBMEET, VA_CLUBQUIT, VA_COLLECT,
    VA_PLANTMENU, VA_GWATER, VA_GHARVEST, VA_GSELL, VA_GSNACK, VA_CHOCO, VA_GLAMSALON, VA_CLUB0, VA_PLANT0 = VA_CLUB0 + 10, VA_CRIME0 = VA_PLANT0 + 10,
    VA_FEST0 = VA_CRIME0 + 101
};
static bool venueHere() { return !roomIsHome(G.room) || G.room == R_YARD; }
static int outfitPrice() { const TownEvent* e = eventHere(R_MALL); return (G.room == R_MALL && e && e->game == -1 && std::string(e->name) == "Mall Sale Day") ? 40 : 80; }

struct Opt { std::string name; int id; };
struct Target { int kind; int idx; float d; };   // kind 0 item, 1 actor
static bool menuOpen = false;
static Target menuT{-1, -1, 0};
static int menuRoom = -1;   // the room the menu was opened in
static std::vector<Opt> opts;
static int optPage = 0;
static float partyMins = 0;

static std::vector<Opt> itemOptions(int shape, int idx = -1) {
    const Sim& me = MC();
    std::vector<Opt> v;
    auto a = [&](const char* n, int id) { v.push_back({n, 2000 + id}); };
    bool little = me.isLittle();
    Actor* ma = idx >= 0 ? mcActor() : nullptr;
    bool onIt = ma && ma->seat == idx && ma->useKind;
    const int orig = shape;
    shape = baseShape(shape);
    if (orig == SH_GAMINGPC && !little) { a("Stream a Game", IA_STREAMGAME); a("Play PC Games", IA_ARCADE); }
    if ((G.gadgets & 1) && (shape == SH_BED || shape == SH_SOFA || shape == SH_BEANBAG || shape == SH_CHAIR || shape == SH_LOUNGER)) a("Read on E-Reader", IA_EREAD);
    if ((G.gadgets & 2) && (shape == SH_BED || shape == SH_SOFA || shape == SH_BEANBAG || shape == SH_CHAIR || shape == SH_LOUNGER)) a("Listen to SugarPod", IA_MUSICPOD);
    if ((G.gadgets & 4) && (shape == SH_BED || shape == SH_SOFA || shape == SH_BEANBAG || shape == SH_CHAIR || shape == SH_LOUNGER || shape == SH_DESK)) { a("Laptop: Write a Blog Post", IA_LAPBLOG); a("Laptop: Binge a Show", IA_LAPBINGE); if (me.stage >= ST_CHILD && me.stage <= ST_YA) a("Laptop: Do Homework", IA_LAPHOMEWORK); }
    if (orig == SH_TEATABLE) { a("Have a Tea Party", IA_TEAPARTY); a("Grab a Snack", IA_SNACK); }
    if (orig == SH_GAMERCHAIR) a("Play Games", IA_ARCADE);
    switch (shape) {
    case SH_BED:
        if (!little) { a("Sleep", IA_SLEEP); a("Nap", IA_NAP); }
        a("Relax in Bed", IA_RELAXBED);
        if (me.stage >= ST_CHILD && me.stage <= ST_TEEN) a("Have a Sleepover", IA_SLEEPOVER);
        if (me.isAdult() && partnerHere() >= 0) {
            a("Pillow Talk", IA_PILLOW);
            if (me.stage <= ST_MIDDLE && G.sims[partnerHere()].stage <= ST_MIDDLE) a("Make a Little Magic", IA_MAGIC);
        }
        break;
    case SH_CRIB: if (little) a("Sleep", IA_SLEEP); else a("Rock Baby", IA_ROCK); break;
    case SH_SOFA: case SH_CHAIR: case SH_BEANBAG: case SH_BENCH:
        a("Sit", IA_SIT); a("Relax", IA_RELAX);
        if (shape == SH_SOFA) {
            bool tv = false; for (const Placed& p : curRoom().items) if (itemShape(p.item) == SH_TV) tv = true;
            if (tv) a("Watch TV", IA_TV);
            if (!little) a("Nap", IA_NAP);
            if (me.stage >= ST_CHILD && me.stage <= ST_PRETEEN) a("Build a Blanket Fort", IA_FORT);
        }
        if (shape == SH_CHAIR || shape == SH_BENCH) a("Read a Book", IA_READ);
        break;
    case SH_LOUNGER: a("Sunbathe", IA_SUNBATHE); a("Relax", IA_RELAX); break;
    case SH_TV: a("Watch TV", IA_TV); v.push_back({"TV Guide (Shows)", 8703}); break;
    case SH_BOOKSHELF: a("Read a Book", IA_READ); a("Study", IA_STUDY); if (me.stage >= ST_TEEN) a("Write Novel", IA_NOVEL); break;
    case SH_DESK:
        if (me.stage >= ST_CHILD && me.stage <= ST_TEEN) a("Do Homework", IA_HOMEWORK);
        if (me.stage >= ST_CHILD && me.stage <= ST_PRETEEN) a("Draw with Crayons", IA_CRAYONS);
        if (me.stage >= ST_PRETEEN && me.stage <= ST_TEEN) a("Study Group Call", IA_STUDYCALL);
        a("Browse the Web", IA_WEB); a("Play Games", IA_ARCADE); a("Check Social Media", IA_SOCIALAPP);
        if (me.stage >= ST_TEEN) { a("Create Content", IA_CONTENT); a("Look for Work", IA_JOBS); a("Write Novel", IA_NOVEL); a("Hack an Account (shady)", IA_HACK); }
        a("Write in Diary", IA_DIARY);
        break;
    case SH_VANITY: case SH_MIRROR:
        a("Check Appearance", IA_CHECKLOOK);
        if (me.stage >= ST_CHILD) a("Glam Station (restyle)", IA_GLAMSTATION);
        if (!little) { a("Do Makeup", IA_MAKEUP); a("Practice Charisma", IA_SPEECH); }
        if (me.stage >= ST_PRETEEN) a("Film a Makeup Tutorial", IA_TUTORIAL);
        break;
    case SH_DRESSER: case SH_SHOERACK: a("Change Outfit", IA_OUTFIT); if (!little) a("Plan Outfits", IA_PLANOUTFIT); break;
    case SH_FRIDGE: case SH_OVEN: case SH_COUNTER:
        if (!little) { a("Cook a Meal", IA_COOK); a("Quick Meal", IA_QUICKMEAL); v.push_back({"Cook a Recipe (60)", 8702}); }
        a("Grab a Snack", IA_SNACK);
        if (shape == SH_FRIDGE) v.push_back({"Open Fridge / Bag", 8701});
        break;
    case SH_GRILL: if (!little) a("Backyard BBQ", IA_BBQ); break;
    case SH_SINK: a("Wash Hands", IA_BATH); if (!little) a("Brush Teeth", IA_BRUSH); break;
    case SH_BATHTUB: a("Bubble Bath", IA_BATH); break;
    case SH_SHOWER: if (!little) a("Take a Shower", IA_SHOWER); break;
    case SH_TOILET: if (!little) a("Use Toilet", IA_TOILET); break;
    case SH_TOYBOX:
        a("Play with Toys", IA_TOYS);
        if (me.stage >= ST_TODDLER && me.stage <= ST_PRETEEN) { a("Play Pretend", IA_PRETEND); a("Tea Party", IA_TEAPARTY); }
        break;
    case SH_TABLE: if (me.stage >= ST_CHILD && me.stage <= ST_PRETEEN) a("Run a Lemonade Stand", IA_LEMONADE); break;
    case SH_SPEAKER: case SH_DISCOBALL:
        a("Dance", IA_DANCE); a("Change Music", IA_MUSIC);
        if (me.stage >= ST_TEEN) a("Throw a Party!", IA_PARTY);
        if (me.stage >= ST_PRETEEN && me.stage <= ST_YA) a("Learn a Viral Dance", IA_VIRALDANCE);
        break;
    case SH_ARCADE: a("Play Games", IA_ARCADE); break;
    case SH_PLANT: case SH_FLOWERS: if (!little) a("Water", IA_WATER); break;
    case SH_PAINTING: a("Admire Art", IA_ADMIRE); break;
    case SH_LAMP: a(G.lightsOn ? "Lights Off" : "Lights On", IA_LIGHTS); break;
    case SH_UMBRELLA: a("Relax in Shade", IA_RELAX); break;
    default: break;
    }
    if (onIt) v.insert(v.begin(), Opt{"Get Up", 2000 + IA_GETUP});
    return v;
}

static std::vector<Opt> venueOptions() {
    std::vector<Opt> v;
    const Sim& me = MC();
    auto a = [&](const std::string& n, int id) { v.push_back({n, id < 0 ? -1 : 3000 + id}); };
    if (me.career && CAREERS[me.career].room == G.room) {
        if (atWorkTime(me)) {
            if (shiftHasGame(me.career)) { a("Work Shift (play it!)", VA_WORK); a("Work Quietly (skip)", VA_WORKAUTO); }
            else a("Go to Work!", VA_WORK);
        }
        else if (me.workedToday) a("(Worked today)", -1);
    }
    if (const TownEvent* ev = eventHere(G.room)) {
        if (ev->game >= 0 && !eventEntered(ev)) a(std::string("Enter the ") + ev->name + " Contest!", VA_CONTEST);
        if (ev->game == -2 && !eventEntered(ev)) a("Crowning Ceremony!", VA_CROWN);
    }
    { std::string fo[3]; int fi[3]; int fn = festActs(fo, fi); for (int k = 0; k < fn; k++) a(fo[k], VA_FEST0 + fi[k]); }
    bool movieNight = eventHere(R_CINEMA) && G.room == R_CINEMA;
    bool spaMonday = eventHere(R_SPA) && G.room == R_SPA;
    switch (G.room) {
    case R_MALL:
        if (me.stage >= ST_PRETEEN && me.stage <= ST_YA) a("Hang Out & Window Shop", VA_HANGOUT);
        if (me.stage >= ST_CHILD && me.stage <= ST_TEEN) a("Arcade Tokens ($5)", VA_TOKENS);
        a("Shop Outfits", VA_SHOP); a("Food Court ($12)", VA_SNACK); a("Photo Booth ($5)", VA_PHOTOBOOTH); a("Smoothie Rush", VA_MG_SMOOTHIE);
        if (me.stage >= ST_CHILD) a("Buy Chocolates ($15)", VA_CHOCO);
        break;
    case R_PARK:
        if (me.stage >= ST_TODDLER && me.stage <= ST_PRETEEN) a("Playground", VA_PLAYGROUND);
        if (me.stage >= ST_PRETEEN && me.stage <= ST_YA) a("Skate Park", VA_SKATEPARK);
        a("Go for a Jog", VA_JOG); a("Picnic ($10)", VA_PICNIC); a("Feed the Ducks", VA_DUCKS); a("Paint Outdoors", VA_PAINT); break;
    case R_CINEMA:
        a(movieNight ? "Romance Movie (FREE)" : "Romance Movie ($15)", VA_MOVIE_ROM); a(movieNight ? "Horror Movie (FREE)" : "Horror Movie ($15)", VA_MOVIE_HOR);
        a(movieNight ? "Comedy Movie (FREE)" : "Comedy Movie ($15)", VA_MOVIE_COM); a("Popcorn ($6)", VA_POPCORN);
        break;
    case R_RINK: a("Ice Skate", VA_SKATE); a("Hot Cocoa ($6)", VA_COCOA); break;
    case R_RESTAURANT: a("Order Dinner ($30)", VA_DINNER); break;
    case R_BOUTIQUE: a("Shop Outfits", VA_SHOP); a("Style Clients (tips)", VA_STYLETIPS); a("Adopt a Pet ($200)", VA_PET); break;
    case R_COLLEGE: if (me.enrolled && !me.degree) a("Attend Class", VA_CLASS); else if (!me.enrolled) a("(Enroll in Jobs app)", -1); break;
    case R_CAFE: a("Iced Latte ($8)", VA_LATTE); a("Muffin ($5)", VA_MUFFIN); a("Sing Karaoke", VA_MG_KARAOKE); break;
    case R_SPA: a(spaMonday ? "Massage ($20 today!)" : "Massage ($40)", VA_MASSAGE); a("Makeover ($25)", VA_MAKEOVER); if (me.stage >= ST_CHILD) a("Glam Session ($40)", VA_GLAMSALON); a("Paint Nails", VA_MG_NAILS); break;
    case R_CLUB: a("Dance", VA_DANCE); a("Fancy Mocktail ($10)", VA_MOCKTAIL); break;
    case R_BEACH: if (me.stage >= ST_TODDLER && me.stage <= ST_PRETEEN) a("Build a Sandcastle", VA_SANDCASTLE); a("Swim", VA_SWIM); a("Tan", VA_TAN); a("Beach Volleyball", VA_MG_VOLLEY); break;
    case R_YARD:
        if (me.stage < ST_CHILD) { a("(Too little to garden)", -1); break; }
        if (gardenPlanted() < NUM_PLOTS) a("Plant Seeds >", VA_PLANTMENU);
        if (gardenNeedsWater()) a("Water the Garden", VA_GWATER);
        if (gardenReady()) a("Harvest (" + std::to_string(gardenReady()) + " ready)", VA_GHARVEST);
        if (basketSellValue()) a("Sell Harvest ($" + std::to_string(basketSellValue()) + ")", VA_GSELL);
        if (basketCount(0)) a("Snack on Strawberries", VA_GSNACK);
        if (v.empty()) a("(Garden is growing...)", -1);
        break;
    case R_SCHOOL:
        if (me.stage >= ST_CHILD && me.stage <= ST_TEEN && isWeekday(G.day) && !me.schoolToday && G.minute < 10 * 60) a(me.stage == ST_CHILD ? "Go to Elementary Class" : me.stage == ST_PRETEEN ? "Go to Middle School Class" : "Go to Class", VA_SCHOOL);
        if (me.stage >= ST_CHILD && me.stage <= ST_TEEN) {
            if (G.club < 0) for (int k = 0; k < CQ_COUNT; k++) a(std::string("Join ") + CLUB_NAMES[k], VA_CLUB0 + k);
            else { a(std::string("Club Meeting: ") + CLUB_NAMES[G.club], VA_CLUBMEET); a("Switch Clubs", VA_CLUBQUIT); }
            a("Study in the Library", VA_LIBRARY);
        }
        break;
    }
    if (int cs = collectSetFor(G.room); cs >= 0 && me.stage >= ST_CHILD) {
        static const char* LBL[NUM_SETS] = {"Look for Seashells", "Catch Butterflies", "Charm Gacha ($5)", "Hunt for Ice Crystals"};
        a(LBL[cs], VA_COLLECT);
    }
    for (const CrimeOpt& co : crimeVenueOptions(G.room)) a(co.label, co.crime < 0 ? -1 : VA_CRIME0 + co.crime);
    if (G.room != R_YARD) a("Go Home", VA_GOHOME);
    return v;
}

// interactive objects in town districts (kind 3 targets)
static std::vector<Opt> spotOptions(int kind) {
    const Sim& me = MC();
    std::vector<Opt> v;
    auto va = [&](const char* n, int id) { v.push_back({n, 3000 + id}); };
    auto da = [&](const char* n, int id) { v.push_back({n, 8000 + id}); };
    auto mg = [&](const char* n, int id) { v.push_back({n, 5000 + id}); };
    const bool kid = me.stage < ST_TEEN;
    switch (kind) {
    case SP_RACK: va("Shop Outfits", VA_SHOP); break;
    case SP_SHOES: da("Buy New Shoes ($45)", DA_SHOES); break;
    case SP_ACCESS: da("Buy Accessories ($35)", DA_ACCESS); break;
    case SP_MIRROR: v.push_back({"Restyle in the Fitting Room", 8500}); break;
    case SP_MAKEUP: da("Try a Lip Gloss Sample", DA_LIPSAMPLE); if (!kid) da("Makeup Makeover ($20)", DA_MAKEOVER); break;
    case SP_NAILS: mg("Paint Nails", MG_NAILS); break;
    case SP_FOODSTALL: va("Food Court Meal ($12)", VA_SNACK); da("Bubble Tea ($6)", DA_BUBBLETEA); va("Smoothie Rush", VA_MG_SMOOTHIE); va("Buy Chocolates ($15)", VA_CHOCO); break;
    case SP_PHOTOBOOTH: va("Photo Booth ($5)", VA_PHOTOBOOTH); break;
    case SP_ARCADE: va("Play Arcade ($5)", VA_TOKENS); da("Challenge Someone", DA_CHALLENGE); break;
    case SP_CLAW: da("Claw Machine ($3)", DA_CLAW); break;
    case SP_FURNITURE: if (me.isAdult()) da("Order a Decor Piece ($60)", DA_FURNITURE); else v.push_back({"(Ask an adult to buy furniture)", -1}); break;
    case SP_ESPRESSO: va("Iced Latte ($8)", VA_LATTE); va("Muffin ($5)", VA_MUFFIN); break;
    case SP_STAGE: va("Sing Karaoke", VA_MG_KARAOKE); break;
    case SP_DISPLAY: da("Cupcake ($6)", DA_CUPCAKE); da("Whole Party Cake ($30)", DA_CAKE); break;
    case SP_DECORATE: da("Decorate Cupcakes", DA_DECORATE); break;
    case SP_OVEN: da("Baking Class ($15)", DA_BAKECLASS); break;
    case SP_PICNIC: va("Picnic ($10)", VA_PICNIC); da("Sit & People-Watch", DA_PEOPLEWATCH); break;
    case SP_POND: va("Feed the Ducks", VA_DUCKS); break;
    case SP_SWINGS: da("Swing", DA_SWING); if (me.stage >= ST_TODDLER && me.stage <= ST_PRETEEN) va("Playground Time", VA_PLAYGROUND); break;
    case SP_SANDBOX: da("Play in the Sand", DA_SANDBOX); break;
    case SP_HOOP: da("Shoot Hoops", DA_HOOPS); break;
    case SP_ICECREAM: da("Ice Cream Cone ($4)", DA_ICECREAM); break;
    case SP_DINNERTABLE: va(G.date.active ? "Romantic Dinner ($60)" : "Order Dinner ($30)", VA_DINNER); break;
    case SP_BOOTH: da(G.date.active ? "Burger Date ($26)" : "Burger & Shake ($14)", DA_BURGER); break;
    case SP_SHAKE: da("Milkshake ($8)", DA_SHAKE); if (me.stage >= ST_TEEN) v.push_back({"Work a Shift: Dinner Rush", 8502}); break;
    case SP_JUKEBOX: da("Play a Song & Dance", DA_JUKEBOX); break;
    case SP_CONVEYOR: da("Grab Sushi Plates ($18)", DA_SUSHI); break;
    case SP_SUSHICLASS: if (!kid) da("Sushi Class ($25)", DA_SUSHICLASS); else v.push_back({"(Teens and up)", -1}); break;
    case SP_HOMEDOOR: va("Go Inside", VA_GOHOME); break;
    case SP_NDOOR0: case SP_NDOOR1: case SP_NDOOR2: {
        int n = neighborFor(kind - SP_NDOOR0);
        static std::string lbl; lbl = n >= 0 ? "Knock (" + G.sims[n].name + "'s place)" : std::string("Knock");
        v.push_back({lbl, 8000 + DA_KNOCK0 + (kind - SP_NDOOR0)});
        break;
    }
    case SP_MAILBOX: da("Check the Mail", DA_MAIL); break;
    case SP_REALTY: if (me.isAdult()) v.push_back({G.house == 0 ? "Move to the Pink Penthouse" : "Move to Cocoa Cottage", 8501}); else v.push_back({"(Adults only)", -1}); break;
    case SP_LEMONADE: da("Buy Lemonade ($2)", DA_LEMONADE); break;
    case SP_NCOUCH: da("Hang Out", DA_NHANG); break;
    case SP_NFRIDGE: da("Raid Their Fridge", DA_NFRIDGE); break;
    case SP_NEXIT: da("Say Goodbye", DA_NEXIT); break;
    case SP_RECEPTION: da("Checkup ($30)", DA_CHECKUP); da("Flu Shot ($10)", DA_FLUSHOT); if (me.stage >= ST_TEEN) da("Volunteer", DA_VOLUNTEER); break;
    case SP_EXAMBED: da("Checkup ($30)", DA_CHECKUP); if (isSick(G.mc)) v.push_back({"See a Doctor & Get Meds ($60)", 8611}); for (auto& p : town2SpotOptions(SP_HOSPBED)) v.push_back({p.first, p.second}); break;
    case SP_NURSERYWIN: da("Peek at the Babies", DA_BABIES); break;
    case SP_PETPEN: va("Adopt a Pet ($200)", VA_PET); break;
    case SP_PETSUPPLY: da("Pet Treats ($12)", DA_PETTREATS); break;
    case SP_GROOM: da("Groom Your Pet ($25)", DA_GROOM); da("Vet Checkup ($40)", DA_VETCHECK); break;
    case SP_BOOKSHELF: da("Read a Book", DA_READ); da("Buy a Bestseller ($15)", DA_BUYBOOK); v.push_back({"Help Patrons: Book Match (minigame)", 8601}); break;
    case SP_STUDYTABLE: da("Study", DA_STUDY); break;
    case SP_STORYRUG: da("Story Time", DA_STORYTIME); if (!kid) da("Poetry Slam", DA_POETRY); break;
    case SP_TREADMILL: da("Run", DA_TREADMILL); break;
    case SP_WEIGHTS: if (!kid) da("Lift Weights", DA_WEIGHTS); else v.push_back({"(Teens and up)", -1}); break;
    case SP_YOGAMAT: da("Yoga Class ($10)", DA_YOGA); break;
    case SP_LAPPOOL: da("Swim Laps", DA_LAPS); break;
    case SP_FRONTDESK: if (!kid) da("Book a Night ($150)", DA_HOTELNIGHT); da("Room Service ($35)", DA_ROOMSERVICE); break;
    case SP_HOTELPOOL: da("Lounge Poolside", DA_HOTELPOOL); break;
    case SP_LOUNGE: da("Piano Lounge", DA_LOUNGE); break;
    case SP_ALTAR: if (pendingMemorial() >= 0) v.push_back({"Hold a Memorial for " + G.sims[pendingMemorial()].name, 8610}); if (me.isAdult()) { da("Dream Wedding ($500)", DA_WEDDING); da("Renew Vows ($100)", DA_VOWRENEW); } else v.push_back({"(Adults only)", -1}); break;
    case SP_BANQUET: da("Banquet Plate ($20)", DA_BANQUET); if (me.isAdult()) da("Host a Party ($200)", DA_PARTY); break;
    case SP_DANCEFLOOR: da("Slow Dance", DA_SLOWDANCE); break;
    case SP_PRODUCE: da("Fruit Cup ($3)", DA_FRUIT); da("Garden Seeds ($8)", DA_SEEDS); da("Grab Flowers ($12)", DA_FLOWERS); break;
    case SP_AISLES: if (me.stage >= ST_TEEN) da("Weekly Groceries ($60)", DA_GROCERIES); da("Snack Haul ($15)", DA_SNACKHAUL); v.push_back({"Budget Grocery Run (minigame)", 8600}); break;
    case SP_BEAUTYAISLE: da("Lip Gloss ($8)", DA_LIPGLOSS); da("Nail Polish ($6)", DA_NAILPOLISH); if (!kid) { da("Blush ($14)", DA_BLUSH); da("Lash Kit ($12)", DA_LASHES); da("Eyeshadow Palette ($22)", DA_PALETTE); } break;
    case SP_FITRACK: da("Cute Fit ($30)", DA_CUTEFIT); da("Fit Haul: 3 Looks ($75)", DA_FITHAUL); break;
    case SP_CREAKY: da("Ghost Hunt", DA_GHOSTHUNT); if (me.stage >= ST_TEEN) da("Stay the Night (Dare)", DA_DARE); break;
    case SP_SEANCE: da("Hold a Seance", DA_SEANCE); break;
    case SP_PARTYFLOOR: if (me.stage >= ST_YA) da("Join the Party", DA_FRATPARTY); else v.push_back({"(College age and up)", -1}); break;
    case SP_LOUNGER: da("Soak Up the Sun", DA_TAN); break;
    case SP_LAGOON: da("Snorkel", DA_SNORKEL); da("Collect Shells", DA_SHELLS); break;
    case SP_TIKIBAR: da("Tropical Mocktail ($8)", DA_MOCKTAIL); da("Hula Lesson", DA_HULA); break;
    case SP_SOUVENIR: da("Buy a Souvenir ($15)", DA_SOUVENIR); da("Fly Home (end trip)", DA_FLYHOME); break;
    case SP_SKILIFT: da("Ski the Slopes", DA_SKI); da("Snowboard Lesson ($30)", DA_SNOWBOARD); break;
    case SP_SNOWFIELD: da("Build a Snowman", DA_SNOWMAN); da("Snowball Fight", DA_SNOWBALL); break;
    case SP_FIREPLACE: da("Hot Cocoa ($5)", DA_COCOA); da("Cozy Up by the Fire", DA_COZY); da("Fly Home (end trip)", DA_FLYHOME); break;
    case SP_HOTTUB: da("Soak in the Hot Tub", DA_HOTTUB); break;
    case SP_CREPECART: da("Crepe ($9)", DA_CREPE); break;
    case SP_COUTURE: da("Couture Look ($120)", DA_COUTURE); break;
    case SP_LOOKOUT: da("Take in the View", DA_VIEW); da("Selfie at the Top", DA_TOPSELFIE); break;
    case SP_ARTIST: da("Portrait Painted ($25)", DA_PORTRAIT); da("Fly Home (end trip)", DA_FLYHOME); break;
    case SP_HFRONTDESK: da("Ask the Concierge", DA_CONCIERGE); da("Check Out & Fly Home", DA_SUITEFLY); break;
    case SP_HLOUNGE: da("Lobby Coffee ($6)", DA_LOUNGECOFFEE); break;
    case SP_BELLHOP: da("Order a Ride to the Sights", DA_LUGGAGE); break;
    case SP_RAINSHOWER: da("Rain Shower", DA_RAINSHOWER); break;
    case SP_SOAKTUB2: da("Bubble Soak", DA_HSOAK); break;
    case SP_HVANITY: da("Glam Up at the Vanity", DA_HGLAM); break;
    case SP_INFINITY: da("Swim in the Infinity Pool", DA_INFINITY); break;
    case SP_MASSAGE: da("Spa Massage ($40)", DA_MASSAGE); break;
    case SP_POOLBAR: da("Poolside Mocktail ($8)", DA_POOLDRINK); break;
    case SP_HOTELBED: da("Sleep", DA_HOTELSLEEP); da("Check Out & Fly Home", DA_SUITEFLY); break;
    case SP_ROOMSVC: da("Order Room Service ($25)", DA_SUITESVC); break;
    case SP_MINIBAR: da("Raid the Minibar ($12)", DA_MINIBAR); break;
    case SP_BALCONY: da("Enjoy the View", DA_BALCONY); break;
    case SP_LANDMARK: da("Take Tourist Photos", DA_LANDMARK); break;
    case SP_TOURDESK: da("Guided Tour ($30)", DA_TOUR); break;
    case SP_STREETFOOD: da("Try Local Food ($12)", DA_STREETFOOD); break;
    case SP_GIFTSHOP: da("Buy a Souvenir ($15)", DA_SOUVENIR); break;
    case SP_BUSKER: case SP_TURNSTILE: case SP_CABINETS: case SP_DDR: case SP_PRIZES: case SP_WASHER: case SP_TANK: case SP_TOUCHPOOL: case SP_EXHIBIT: case SP_SCULPT: case SP_ROSES: case SP_GREENHOUSE: case SP_ROOFBAR: case SP_SKYVIEW:
        if (kind == SP_CABINETS) for (int k = 0; k < NUM_MINIGAMES; k++) mg(MG_NAMES[k], k);
        if (kind == SP_DDR) mg("Dance Machine (Color Dance)", 3);
        for (auto& p : funparkSpotOptions(kind)) v.push_back({p.first, p.second});
        break;
    case SP_FCFOOD: case SP_FCTABLE: case SP_T2PHOTOBOOTH: case SP_PIERCE: case SP_LOCKER: case SP_BULLETIN: case SP_DESK: case SP_CHALK: case SP_LUNCHLINE: case SP_LUNCHTABLE: case SP_T2BLEACHERS: case SP_T2COURT:
    case SP_CRIB: case SP_PLAYMAT: case SP_PEW: case SP_BAKESALE: case SP_CCDESK: case SP_T2STORYRUG: case SP_POOLWATER: case SP_SNACKBAR: case SP_GRILL: case SP_BOUNCE: case SP_DJ: case SP_DORMDESK: case SP_DORMBUNK: case SP_GREEK:
    case SP_SALONCHAIR: case SP_NAILBAR: case SP_HOSPDESK: case SP_HOSPBED: case SP_RINKFLOOR: case SP_SKATERENT: case SP_MKSTALL: case SP_MKGAMES: case SP_CLERK: case SP_JUDGE:
        for (auto& p : town2SpotOptions(kind)) v.push_back({p.first, p.second});
        break;
    case SP_AVSTORE: case SP_AVERRAND: case SP_AVFAIR: case SP_AVPUB: case SP_AVGARDEN: case SP_AVMEADOW: case SP_AVCOOP: case SP_AVNECTAR: case SP_AVSTALL: case SP_AVPEN: case SP_AVPIGS: case SP_AVLOOM:
        for (auto& p : farmSpotOptions(kind)) v.push_back({p.first, p.second});
        break;
    case SP_TICKETS: case SP_CAROUSEL: case SP_CANDY: case SP_COASTER: case SP_FERRIS: case SP_BUMPER: case SP_GAMEBOOTH:
        for (auto& p : funparkSpotOptions(kind)) v.push_back({p.first, p.second});
        if (kind == SP_GAMEBOOTH) { mg("Balloon Pop (Pinata)", 1); mg("Duck Catch (Treat Catch)", 2); }
        break;
    case SP_VSTAGE: case SP_VVIP: case SP_VBAR: case SP_VMGR:
        for (auto& p : velvetSpotOptions(kind == SP_VSTAGE ? 0 : kind == SP_VVIP ? 1 : kind == SP_VBAR ? 2 : 3)) v.push_back({p.first, p.second});
        break;
    case SP_TAXI: case SP_HOTDOG: case SP_ICECART: case SP_NEWSSTAND: case SP_DRACK0: case SP_DRACK1: case SP_DRACK2: case SP_DRESSBOOTH: case SP_DREGISTER: case SP_PODIUM: case SP_DTABLE: case SP_STOVE:
        for (auto& p : hotdateSpotOptions(kind)) v.push_back({p.first, 8000 + DA_HD0 + p.second});
        break;
    case SP_LANES: case SP_KMIC: case SP_EASEL: case SP_DFLOOR: case SP_NFOOD:
    case SP_BOWLSNACK: case SP_KBOOTH: case SP_SIPBAR: case SP_BARRE: case SP_VENDOR: case SP_REHABDESK: case SP_CIRCLE:
        if (kind == SP_LANES) { mg("Bowl a Frame (Treat Catch)", 2); mg("Strike Smash (Pinata)", 1); }
        if (kind == SP_KMIC) mg("Sing! (Karaoke Star)", 7);
        if (kind == SP_EASEL) mg("Paint Match", 0);
        if (kind == SP_DFLOOR) { mg("Dance-Off (Color Dance)", 3); mg("Runway Strut", 5); }
        if (kind == SP_NFOOD) mg("Smoothie Rush", 9);
        for (auto& p : nightSpotOptions(kind)) v.push_back({p.first, 8000 + p.second});
        break;
    case SP_BUNK: da("Sleep on the Bunk", DA_JAILSLEEP); da("Do Push-ups", DA_PUSHUPS); break;
    case SP_PAYPHONE: da("Call Home", DA_CALLHOME); da("Call a Lawyer ($150)", DA_LAWYER); break;
    case SP_JAILYARD: da("Shoot Hoops", DA_JAILHOOPS); da("Make a Friend", DA_JAILFRIEND); da("Start a Fight (+time!)", DA_JAILFIGHT); break;
    case SP_GUARD: { char bb[48]; snprintf(bb, sizeof bb, "Post Bail"); da(bb, DA_BAIL); da("When Do I Get Out?", DA_TIMELEFT); break; }
    case SP_DORMBED: da("Sleep", DA_DORMSLEEP); da("Video-Call Home", DA_HOMESICK); break;
    case SP_STUDYDESK: da("Study for Exams", DA_PREP); break;
    case SP_COMMONS: da("Hang with Roommates", DA_ROOMIES); da("Pillow Fight!", DA_PILLOW); da("Sneak Out After Lights-Out", DA_SNEAKOUT); break;
    case SP_GATE: da(boarding() && isWeekday(G.day) ? "Gate (weekends only)" : "Go Home", DA_GATE); break;
    case SP_FRATCOUCH: if (me.stage >= ST_TEEN) da("Hang Out", DA_FRATHANG); else v.push_back({"(Too young for this lawn)", -1}); break;
    case SP_SNACKS: da("Snacks & Slushie ($4)", DA_CSNACK); da("Scratch-Off ($5)", DA_SCRATCH); break;
    case SP_REGISTER: if (me.stage >= ST_TEEN) v.push_back({"Work the Register (minigame)", 8603}); else v.push_back({"(Teens and up)", -1}); break;
    case SP_RACKS: v.push_back({"Thrift Grab! (minigame)", 8602}); da("Browse a Rack ($10)", DA_BROWSE); break;
    case SP_DONATE: da("Donate Clothes", DA_DONATE); break;
    case SP_COURT: da("Play a Match", DA_TENNIS); da("Take a Lesson ($25)", DA_LESSON); break;
    case SP_BLEACHERS: da("Watch a Match", DA_WATCH); break;
    case SP_CHECKOUT: if (me.stage >= ST_TEEN) da("Work a Cashier Shift (+$28)", DA_CASHIER); else v.push_back({"(Teens and up can work here)", -1}); break;
    }
    // snacks & drinks can also be bought to go: they land in the Backpack to eat later or gift
    for (size_t i = 0, n = v.size(); i < n; i++) if (v[i].id >= 8000 && v[i].id < 8500) { int k = contentToGoFor(v[i].id - 8000); if (k >= 0) {
        char b[64]; snprintf(b, sizeof b, "%s To Go ($%d)", contentToGoName(k), contentToGoPrice(k)); v.push_back({b, 8800 + k}); } }
    return v;
}
static std::vector<Target> nearby() {
    std::vector<Target> v;
    Actor* me = mcActor();
    RoomState& rs = curRoom();
    for (size_t i = 0; i < rs.items.size(); i++)
        if (!itemOptions(itemShape(rs.items[i].item), -1).empty())
            v.push_back({0, (int)i, std::fabs(rs.items[i].x - me->x)});
    {
        int ns; const Spot* sp = spotsIn(G.room, ns);
        for (int i = 0; i < ns; i++) v.push_back({3, i, std::fabs(sp[i].x - me->x) - 6});
    }
    for (size_t i = 0; i < rs.actors.size(); i++) {
        const Actor& a = rs.actors[i];
        if (a.kind == 0 && a.id == G.mc) continue;
        v.push_back({1, (int)i, std::fabs(a.x - me->x) + std::fabs(a.y - me->y) * 0.5f - 10});
    }
    std::sort(v.begin(), v.end(), [](const Target& a, const Target& b) { return a.d < b.d; });
    return v;
}

static std::string targetName(const Target& t) {
    RoomState& rs = curRoom();
    if (t.kind == 0) return t.idx < (int)rs.items.size() ? itemName(rs.items[t.idx].item) : "?";
    if (t.kind == 3) { int ns; const Spot* sp = spotsIn(G.room, ns); return t.idx < ns ? sp[t.idx].name : "?"; }
    if (t.idx >= (int)rs.actors.size()) return "?";
    const Actor& a = rs.actors[t.idx];
    if (a.kind == 1) return std::string(PET_NAMES[a.id]) + " the " + PET_KINDS[a.id];
    return G.sims[a.id].name;
}

static void openMenu(const Target& t) {
    RoomState& rs = curRoom();
    menuT = t; optPage = 0; opts.clear(); menuRoom = G.room;
    if (t.kind == 0) opts = itemOptions(itemShape(rs.items[t.idx].item), t.idx);
    else if (t.kind == 3) { int ns; const Spot* sp = spotsIn(G.room, ns); if (t.idx < ns) opts = spotOptions(sp[t.idx].kind); }
    else if (t.kind == 1) {
        const Actor& a = rs.actors[t.idx];
        if (a.kind == 1) { opts = {{"Pet", 2100 + PA_PET}, {"Feed", 2100 + PA_FEED}, {"Play Fetch", 2100 + PA_PLAY}}; }
        else {
            auto all = socialOptions(a.id);
            bool simple = G.sims[a.id].isLittle() || MC().isLittle();
            if (simple) for (const SocialOpt& s : all) opts.push_back({s.name, 1000 + s.id});
            else {
                static const int order[SC_COUNT] = {SC_FRIENDLY, SC_FUNNY, SC_ROMANCE, SC_MEAN, SC_FAMILY, SC_SPECIAL};
                for (int k = 0; k < SC_COUNT; k++) {
                    int cat = order[k];
                    for (const SocialOpt& s : all) if (s.cat == cat) { opts.push_back({std::string(SOCIAL_CAT_NAMES[cat]) + " >", 1900 + cat}); break; }
                }
            }
        }
    } else opts = venueOptions();
    menuOpen = !opts.empty();
}

static Activity mk(const char* name, float minutes, float speed, uint8_t pose = PO_IDLE) {
    Activity a = makeAct(name, minutes, speed);
    a.pose = pose;
    return a;
}

static void startParty() {
    RoomState& rs = curRoom();
    std::vector<int> pool;
    for (const Rel& r : G.rels) {
        int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1;
        if (o >= 0 && !G.sims[o].household && r.fr >= 5 && G.sims[o].stage >= ST_TEEN) pool.push_back(o);
    }
    for (size_t i = 0; pool.size() < 5 && i < G.sims.size(); i++)
        if (!G.sims[i].household && G.sims[i].stage == MC().stage && irand(3) == 0) pool.push_back((int)i);
    int n = 0;
    for (int idx : pool) {
        if (n >= 5) break;
        if (actorOf(idx)) continue;
        Actor a; a.kind = 0; a.id = (uint16_t)idx; a.x = 40 + irand(320); a.tx = a.x; a.y = 190 + irand(44); a.stay = 180;
        a.say = n == 0 ? "PARTY!!" : ""; a.sayT = 2;
        rs.actors.push_back(a);
        changeRel(G.mc, idx, 4, 0);
        n++;
    }
    partyMins = 150;
    musicPlay(1);
    G.track = songCount() ? 100 : 1;
    toast("Party time! " + std::to_string(n) + " guests came!");
    uint32_t nf = 20 + n * 15 + G.followers / 50;
    G.followers += nf;
    toast("Your party went viral: +" + std::to_string(nf) + " followers");
    addMoodlet(MC(), "Epic Party", EM_HAPPY, 2, 360);
    fxBurst(200, 60, 40);
}

static float fadeT = 0;
static bool pendingMagicNews = false;
static void putPartnerInBed(int p);
static void standUp(Actor& a, const RoomState& rs);
static void moveHouse();
static void clearQueue();
static void enterPlay(bool fresh);
static void doOption(const Opt& o) {
    Sim& me = MC();
    Actor* ma = mcActor();
    menuOpen = false;
    if (o.id < 0) return;
    sfxTap();
    if (o.id >= 29000 && o.id < 30000) {   // Life Extras
        if (o.id == 29999) { menuOpen = true; return; }
        if (o.id >= 29960 && o.id < 29963) {   // restore a daily backup
            cancelActivity(); clearQueue();
            if (restoreBackup(o.id - 29960, G)) { setArtStyle(G.artStyle); enterPlay(false); toast("Backup restored! Welcome back to Day " + std::to_string(G.day + 1) + "."); sfxYay(); }
            else toast("Couldn't restore that backup.");
            return;
        }
        LxMenu nx = l3Option(o.id);
        if (!nx.empty()) { opts.clear(); for (auto& p : nx) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
        return;
    }
    if (o.id >= 28000 && o.id < 29000) {   // Game Mode & Levels
        if (o.id == 28999) { menuOpen = true; return; }
        LxMenu nx = lvlOption(o.id);
        if (!nx.empty()) { opts.clear(); for (auto& p : nx) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
        return;
    }
    if (o.id >= 27000 && o.id < 28000) {   // Apple Valley farm
        if (o.id == 27999) { menuOpen = true; return; }
        LxMenu nx = farmOption(o.id);
        if (!nx.empty()) { opts.clear(); for (auto& p : nx) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
        return;
    }
    if (o.id >= 26000 && o.id < 27000) {   // Family Outings & Hangouts
        if (o.id == 26999) { menuOpen = true; return; }
        LxMenu nx = outOption(o.id);
        if (!nx.empty()) { opts.clear(); for (auto& p : nx) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
        return;
    }
    if (o.id >= 25000 && o.id < 26000) {   // Design Studio
        LxMenu nx = dsOption(o.id);
        if (dsEditing()) { menuOpen = false; mode = M_DESIGN; return; }
        if (!nx.empty()) { opts.clear(); for (auto& p : nx) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
        return;
    }
    if (o.id >= 24000 && o.id < 25000) {   // Life+
        if (o.id == 24999) { menuOpen = true; return; }
        LxMenu nx = lpOption(o.id);
        if (lpMiniActive()) { menuOpen = false; mode = M_MINI; return; }
        if (!nx.empty()) { opts.clear(); for (auto& p : nx) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
        return;
    }
    if (o.id >= 23000 && o.id < 24000) {   // game settings
        if (o.id == 23999) { menuOpen = true; return; }
        if (o.id == 23021) { opts.clear(); for (auto& p : spicySettingsOptions()) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; return; }
        LxMenu nx = gsOption(o.id);
        if (!nx.empty()) { opts.clear(); for (auto& p : nx) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
        return;
    }
    if (o.id >= 22000 && o.id < 23000) {   // family dynamics
        if (o.id == 22999) { menuOpen = true; return; }
        LxMenu nx = famDynMenu(o.id);
        if (!nx.empty()) { opts.clear(); for (auto& p : nx) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
        return;
    }
    if (o.id >= 21000 && o.id < 22000) {   // choice events
        if (o.id == 21999) { menuOpen = true; return; }   // the prompt line: keep the choices up
        choiceOption(o.id); return;
    }
    if (o.id >= 20000 && o.id < 21000) {   // Dynasty
        if (o.id == 20999) { menuOpen = !opts.empty(); return; }
        LxMenu nx = dyOption(o.id);
        if (!nx.empty()) { opts.clear(); for (auto& p : nx) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
        return;
    }
    if (o.id >= 19000 && o.id < 20000) {   // Daily Life
        if (o.id == 19999) { menuOpen = !opts.empty(); return; }
        LxMenu nx = dlyOption(o.id);
        if (!nx.empty()) { opts.clear(); for (auto& p : nx) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
        return;
    }
    if (o.id >= 18000 && o.id < 18900) {   // Town 2 venues
        if (o.id == 18899) { menuOpen = !opts.empty(); return; }
        LxMenu nx = town2Option(o.id);
        if (!nx.empty()) { opts.clear(); for (auto& p : nx) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
        return;
    }
    if (o.id >= 17000 && o.id < 18000) {   // Grow Up
        if (o.id == 17999) { menuOpen = !opts.empty(); return; }
        LxMenu nx = guOption(o.id);
        if (!nx.empty()) { opts.clear(); for (auto& p : nx) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
        return;
    }
    if (o.id >= 16000 && o.id < 17000) {   // Kid Life
        if (o.id == 16999) { menuOpen = !opts.empty(); return; }
        LxMenu nx = kidOption(o.id);
        if (!nx.empty()) { opts.clear(); for (auto& p : nx) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
        return;
    }
    if (o.id >= 14000 && o.id < 15000) {   // Social Events
        if (o.id == 14999) { menuOpen = !opts.empty(); return; }
        SeMenu nx = seOption(o.id);
        if (!nx.empty()) { opts.clear(); for (auto& p : nx) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
        return;
    }
    if (o.id >= 11000 && o.id < 12600) {   // Deep Life menus
        if (o.id == 11999) { menuOpen = !opts.empty(); return; }
        DlMenu nx = dlOption(o.id);
        if (!nx.empty()) { opts.clear(); for (auto& p : nx) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
        return;
    }
    if ((o.id >= 10000 && o.id < 11000) || (o.id >= 13000 && o.id < 14000) || (o.id >= 15000 && o.id < 16000)) {   // Life Extras menus
        if (o.id == 10999) { menuOpen = !opts.empty(); return; }
        LxMenu nx = lxOption(o.id);
        if (nx.size() == 1 && nx[0].first == "__SE__") nx = sePlanMenu(g_seCalDay);
        if (!nx.empty()) { opts.clear(); for (auto& p : nx) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
        if (lxWantsParty()) { startParty(); lxAfterParty(); }
        return;
    }
    if (o.id >= 9800 && o.id < 10000) { if (o.id != 9899) funparkOption(o.id); return; }
    if (o.id >= 9500 && o.id < 9800) {   // Spicy settings / FanVault / Velvet Room
        if (o.id == 9599 || o.id == 9699) return;
        spicyOption(o.id);
        if (o.id >= 9500 && o.id < 9600) { opts.clear(); for (auto& p : spicySettingsOptions()) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
        return;
    }
    if (o.id >= 9200 && o.id < 9300) {   // Music app
        int t = o.id == 9299 ? -1 : o.id == 9298 ? (songCount() ? 100 + irand(songCount()) : irand(NUM_TRACKS)) : o.id >= 9280 ? o.id - 9280 : 100 + (o.id - 9200);
        musicPlay(t); G.track = t;
        toast(t < 0 ? "Music off." : std::string("Now playing: ") + (t >= 100 ? songName(t - 100) : TRACK_NAMES[t])); return;
    }
    if (o.id >= 9100 && o.id < 9200) {   // Cheats app
        const int c = o.id - 9100;
        auto fill = [&](int n) { for (Sim& s : G.sims) if (s.household) { if (n < 0) for (float& x : s.needs) x = 100; else s.needs[n] = 100; } };
        if (c == 0) { fill(-1); G.money += 50000; for (Sim& s : G.sims) if (s.household) addMoodlet(s, "Cheat Code Glow", EM_HAPPY, 2, 600); toast("ALL cheats: needs maxed, +$50,000, everyone happy!"); }
        else if (c == 1) { fill(-1); toast("All needs maxed for the household!"); }
        else if (c >= 2 && c < 2 + N_COUNT) { fill(c - 2); toast("Need maxed!"); }
        else if (c == 10) { G.money += 1000; toast("+$1,000"); }
        else if (c == 11) { G.money += 50000; toast("+$50,000 (motherlode!)"); }
        else if (c == 12) { G.money += 1000000; toast("+$1,000,000!"); }
        else if (c == 13) { G.money = 0; toast("Money set to $0."); }
        else if (c == 14) { for (float& k : me.skills) k = 10; toast("All skills maxed!"); }
        else if (c == 15) { addMoodlet(me, "Cheat: Very Happy", EM_HAPPY, 3, 1440); toast("Mood set to Very Happy!"); }
        else if (c == 16) { extern bool g_dollV2; g_dollV2 = !g_dollV2; toast(g_dollV2 ? "Art style: Doll 2.0" : "Art style: Classic"); }
        sfxYay(); return;
    }
    if (o.id >= 8000) {   // district objects
        if (o.id == 8500) { casStartEdit(G.mc, 0); mode = M_GLAM; return; }
        if (o.id == 8501) { moveHouse(); return; }
        if (o.id >= 8800 && o.id < 8820) { contentBuyToGo(o.id - 8800); return; }
        if (o.id >= 8700 && o.id <= 8703) { contentOpen(o.id - 8700); mode = M_CONTENT; return; }
        if (o.id == 8610) { holdMemorial(); return; }
        if (o.id == 8611) { if (G.money < 60) { toast("The doctor visit costs $60."); return; } G.money -= 60; G.diag[G.mc] = 1; healthTreat(G.mc, 0); toast("The doctor says rest up. Medicine taken: you'll feel better soon!"); return; }
        if (o.id >= 8600 && o.id <= 8603) { cancelActivity(); shopStart(o.id - 8600); mode = M_SHOPGAME; return; }
        if (o.id == 8502) { cancelActivity(); serveStart(); mode = M_SERVE; return; }
        districtAction(o.id - 8000);
        return;
    }
    if (o.id >= 7000) {   // buy outfit
        int k = o.id - 7000;
        int price = outfitPrice();
        if (G.money < price) { toast("Not enough money!"); sfxNo(); return; }
        G.money -= price; giveOutfit(k); me.outfit = (uint8_t)k;
        goalNotify(GE_BUYOUTFIT, k);
        toast(std::string("Bought & wearing: ") + OUTFITS[k].name + "!");
        addMoodlet(me, "New Outfit!", EM_CONFIDENT, me.hasTrait(TR_FASHIONISTA) ? 3 : 1, 240);
        return;
    }
    if (o.id >= 6000) { int tr = o.id == 6100 ? -1 : o.id - 6000; G.track = tr; musicPlay(tr); if (tr >= 0 && MC().hasTrait(TR_MUSIC)) addMoodlet(MC(), "Music Lover", EM_HAPPY, 2, 240); toast(tr < 0 ? "Music off" : std::string("Now playing: ") + TRACK_NAMES[tr]); return; }
    if (o.id >= 5000) { mgStart(o.id - 5000); mode = M_MG; return; }
    if (o.id >= 4000) { me.outfit = (uint8_t)(o.id - 4000); toast(std::string("Changed into: ") + OUTFITS[me.outfit].name); if (me.hasTrait(TR_FASHIONISTA)) addMoodlet(me, "Fresh Fit", EM_CONFIDENT, 1, 120); return; }
    if (o.id >= 3000) {
        int v = o.id - 3000;
        Activity a;
        if (v == VA_DANCE || v == VA_CHEER) goalNotify(GE_DANCE);
        if (v == VA_CLUBMEET) goalNotify(GE_CLUB);
        if (v == VA_COLLECT) {
            int cs = collectSetFor(G.room);
            if (cs == 2) { if (G.money < 5) { toast("You need $5 for the gacha!"); sfxNo(); return; } G.money -= 5; }
            collectSearch(cs);
            return;
        }
        if (v >= VA_FEST0 && v < VA_FEST0 + 3) { festDo(v - VA_FEST0); return; }
        if (v >= VA_CRIME0 && v <= VA_CRIME0 + 100) {
            int c = v - VA_CRIME0;
            if (c == CRIME_HEIST) { cancelActivity(); heistStart(); mode = M_HEIST; return; }
            crimeAttempt(c);
            return;
        }
        if (v >= VA_PLANT0 && v < VA_PLANT0 + NUM_CROPS) { gardenPlant(v - VA_PLANT0); return; }
        if (v >= VA_CLUB0 && v < VA_CLUB0 + CQ_COUNT) {
            G.club = (int8_t)(v - VA_CLUB0);
            static const char* PITCH[CQ_COUNT] = {"Welcome to Fashion Club! We judge outfits. Lovingly.", "Welcome to Cheer Squad! Smile and spirit fingers!",
                                                  "Welcome to Drama Club! Everything is a SCENE.", "Welcome to Robotics! Mind the soldering iron.", "Welcome to the Garage Band! Can you play anything? Doesn't matter!"};
            dlgSay(-1, PITCH[G.club]);
            standingBoost(G.club, 10);
            addMoodlet(me, "New Club!", EM_HAPPY, 1, 240);
            return;
        }
        switch (v) {
        case VA_GOHOME: travelTo(R_LIVING); return;
        case VA_PLANTMENU:
            opts.clear();
            for (int k = 0; k < NUM_CROPS; k++) opts.push_back({std::string(CROP_NAMES[k]) + " $" + std::to_string(cropSeedPrice(k)) + " (" + std::to_string(cropDays(k)) + "d)", 3000 + VA_PLANT0 + k});
            menuOpen = true; optPage = 0; menuT = {2, -1, 0};
            return;
        case VA_GWATER: gardenWater(); say(G.mc, "Drink up, little plants!"); return;
        case VA_GHARVEST: gardenHarvest(); return;
        case VA_GSELL: sellHarvest(); return;
        case VA_GSNACK: if (G.basket[0]) { G.basket[0]--; me.needs[N_HUNGER] = std::fmin(100.f, me.needs[N_HUNGER] + 18); say(G.mc, "Sweet and juicy!"); addMoodlet(me, "Homegrown Snack", EM_HAPPY, 1, 120); } return;
        case VA_CHOCO:
            if (G.money < 15) { toast("Not enough money!"); sfxNo(); return; }
            if (G.basket[6] >= 99) { toast("You have plenty of chocolates!"); return; }
            G.money -= 15; G.basket[6]++; toast("Bought a box of chocolates - give it to someone special!"); sfxTap(); return;
        case VA_GLAMSALON:
            if (G.money < 40) { toast("Not enough money!"); sfxNo(); return; }
            casStartEdit(G.mc, 40); mode = M_GLAM; return;
        case VA_PLAYGROUND: a = mk("Playground: swings & slides", 60, 8, PO_DANCE); a.rate[N_FUN] = 55; a.rate[N_SOCIAL] = 15; a.rate[N_ENERGY] = -8; a.skill = SK_FITNESS; a.skillRate = 4; startActivity(a); say(G.mc, "Wheee!"); addMoodlet(me, "Swing Set Champion", EM_PLAYFUL, 1, 180); return;
        case VA_SKATEPARK: a = mk("Skate Park", 60, 8, PO_DANCE); a.rate[N_FUN] = 45; a.rate[N_ENERGY] = -10; a.skill = SK_FITNESS; a.skillRate = 6; startActivity(a); standingBoost(CQ_ALT, 4); addMoodlet(me, "Landed a Kickflip", EM_CONFIDENT, 1, 180); return;
        case VA_SANDCASTLE: a = mk("Building a Sandcastle", 60, 8); a.rate[N_FUN] = 45; a.skill = SK_CREATIVITY; a.skillRate = 5; startActivity(a); addMoodlet(me, "Sandcastle Queen", EM_HAPPY, 1, 180); return;
        case VA_HANGOUT: a = mk("Hanging Out at the Mall", 60, 8, PO_PHONE); a.rate[N_FUN] = 30; a.rate[N_SOCIAL] = 40; startActivity(a); standingBoost(CQ_IT, 4); say(G.mc, "Okay but that bag though..."); return;
        case VA_TOKENS: if (G.money < 5) { toast("Not enough money!"); return; } G.money -= 5; mgStart(irand(2) ? MG_CATCH : MG_PINATA); mode = M_MG; return;
        case VA_LIBRARY: a = mk("Studying in the Library", 60, 10); a.kind = AK_HOMEWORK; a.rate[N_FUN] = -4; a.skill = SK_LOGIC; a.skillRate = 6; startActivity(a); me.gradePts = std::fmin(100.f, me.gradePts + 2); standingBoost(CQ_BRAIN, 2); addMoodlet(me, "In the Zone", EM_FOCUSED, 2, 120); return;
        case VA_CLUBQUIT: if (G.club >= 0) toast(std::string("You left ") + CLUB_NAMES[G.club] + "."); G.club = -1; return;
        case VA_CLUBMEET: {
            if (G.club < 0) return;
            static const uint8_t SKL[CQ_COUNT] = {SK_STYLE, SK_FITNESS, SK_CHARISMA, SK_LOGIC, SK_CREATIVITY};
            static const char* LINE[CQ_COUNT] = {"Mood board time!", "Ready? OK!", "To be or not to be...", "The robot is ALIVE!", "One, two, three, four!"};
            static std::string nm; nm = std::string("Club: ") + CLUB_NAMES[G.club];
            a = mk(nm.c_str(), 90, 10, (G.club == 1 || G.club == 4) ? PO_DANCE : PO_IDLE);
            a.rate[N_FUN] = 30; a.rate[N_SOCIAL] = 30; a.skill = SKL[G.club]; a.skillRate = 7; startActivity(a);
            say(G.mc, LINE[G.club]); standingBoost(G.club, 6); addMoodlet(me, "Club Spirit", EM_INSPIRED, 1, 240);
            return;
        }
        case VA_CONTEST: { const TownEvent* ev = eventHere(G.room); if (!ev) return; eventMarkEntered(ev); g_mgContest = ev;
            dlgSay(-1, std::string("Good luck in the ") + ev->name + " contest! Do your best!"); mgStart(ev->game); mode = M_MG; return; }
        case VA_CROWN: { const TownEvent* ev = eventHere(G.room); if (!ev) return; eventMarkEntered(ev); eventCrowning(ev); return; }
        case VA_MG_KARAOKE: mgStart(MG_KARAOKE); mode = M_MG; return;
        case VA_MG_VOLLEY: mgStart(MG_VOLLEY); mode = M_MG; return;
        case VA_MG_NAILS: mgStart(MG_NAILS); mode = M_MG; return;
        case VA_MG_SMOOTHIE: mgStart(MG_SMOOTHIE); mode = M_MG; return;
        case VA_SNACK: if (G.money < 12) return; G.money -= 12; me.needs[N_HUNGER] = std::fmin(100.f, me.needs[N_HUNGER] + 45); addMoodlet(me, "Food Court Feast", EM_HAPPY, 1, 120); say(G.mc, "Mmm, fries!"); return;
        case VA_PHOTOBOOTH: { if (G.money < 5) return; G.money -= 5; uint32_t f = 10 + G.followers / 100; G.followers += f; me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 15);
            toast("Photo strip posted! +" + std::to_string(f) + " followers"); if (G.date.active) G.date.score += 12; return; }
        case VA_JOG: a = mk("Jogging", 60, 8, PO_WAVE); a.rate[N_FUN] = 10; a.rate[N_ENERGY] = -12; a.rate[N_HYGIENE] = -10; a.skill = SK_FITNESS; a.skillRate = 7; startActivity(a); addMoodlet(me, "Runner's High", EM_ENERGIZED, 1, 180); return;
        case VA_PICNIC: if (G.money < 10) return; G.money -= 10; a = mk("Picnic", 60, 8); a.rate[N_HUNGER] = 50; a.rate[N_SOCIAL] = 20; a.rate[N_FUN] = 20; startActivity(a); if (G.date.active) G.date.score += 20; return;
        case VA_DUCKS: me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 12); say(G.mc, "Quack quack!"); addMoodlet(me, "Duck Friends", EM_HAPPY, 1, 90); return;
        case VA_PAINT: a = mk("Painting", 90, 10); a.rate[N_FUN] = 15; a.skill = SK_CREATIVITY; a.skillRate = 6; startActivity(a); addMoodlet(me, "Inspired by Nature", EM_INSPIRED, 1, 180); return;
        case VA_MOVIE_ROM: case VA_MOVIE_HOR: case VA_MOVIE_COM: {
            bool free = eventHere(R_CINEMA) != nullptr;
            if (!free) { if (G.money < 15) { toast("Not enough money!"); return; } G.money -= 15; }
            a = mk("Watching a Movie", 120, 20); a.rate[N_FUN] = 30; a.rate[N_SOCIAL] = 6; startActivity(a);
            if (v == VA_MOVIE_ROM) addMoodlet(me, "Swoon-Worthy Movie", EM_FLIRTY, 2, 240);
            else if (v == VA_MOVIE_HOR) addMoodlet(me, "Scared Silly", EM_TENSE, 1, 120);
            else addMoodlet(me, "Laughed So Hard", EM_PLAYFUL, 2, 240);
            if (G.date.active) G.date.score += v == VA_MOVIE_ROM ? 30 : 18;
            return;
        }
        case VA_POPCORN: if (G.money < 6) return; G.money -= 6; me.needs[N_HUNGER] = std::fmin(100.f, me.needs[N_HUNGER] + 20); say(G.mc, "*crunch*"); return;
        case VA_SKATE: a = mk("Ice Skating", 60, 8, PO_DANCE); a.rate[N_FUN] = 35; a.rate[N_ENERGY] = -8; a.skill = SK_FITNESS; a.skillRate = 5; startActivity(a); if (G.date.active) G.date.score += 20; addMoodlet(me, "Twirling!", EM_PLAYFUL, 1, 120); return;
        case VA_COCOA: if (G.money < 6) return; G.money -= 6; me.needs[N_ENERGY] = std::fmin(100.f, me.needs[N_ENERGY] + 12); addMoodlet(me, "Cozy Cocoa", EM_HAPPY, 1, 120); if (G.date.active) G.date.score += 8; return;
        case VA_WORK:
            if (me.career == CR_STYLIST) { styleStart(3, true); mode = M_STYLE; return; }
            if (shiftHasGame(me.career)) { shiftStart(me.career); mode = M_SHIFT; return; }
            [[fallthrough]];
        case VA_WORKAUTO:
            a = mk("Working", CAREERS[me.career].hours * 60.f, 40, PO_IDLE); a.kind = AK_WORK;
            a.rate[N_FUN] = -6; a.rate[N_SOCIAL] = 6; a.skill = CAREERS[me.career].skill; a.skillRate = 3;
            startActivity(a); toast("Off to work! (Time flies...)"); return;
        case VA_DINNER: {
            int cost = G.date.active ? 60 : 30;
            if (G.money < cost) { toast("Not enough money!"); sfxNo(); return; }
            G.money -= cost;
            a = mk("Eating Dinner", 60, 8); a.kind = AK_DINNER; a.rate[N_HUNGER] = 80; a.rate[N_FUN] = 15; a.rate[N_SOCIAL] = 15;
            startActivity(a); addMoodlet(me, "Fancy Dinner", EM_HAPPY, 1, 240); return;
        }
        case VA_SHOP:
            opts.clear();
            for (int k = 1; k < NUM_OUTFITS; k++) if (!ownsOutfit(k) && outfitFits(k, MC().stage, MC().look.guy)) opts.push_back({std::string(OUTFITS[k].name) + " $" + std::to_string(outfitPrice()), 7000 + k});
            if (opts.empty()) { toast("You own every outfit, diva!"); return; }
            menuOpen = true; optPage = 0; menuT = {2, -1, 0};
            return;
        case VA_STYLETIPS: styleStart(2, false); mode = M_STYLE; return;
        case VA_PET: {
            if (G.money < 200) { toast("Not enough money!"); sfxNo(); return; }
            if (G.pets.size() >= (size_t)NUM_PETS) { toast("Your home is full of pets!"); return; }
            int p = (int)G.pets.size();
            G.pets.push_back((uint8_t)p);
            G.money -= 200;
            Actor pa; pa.kind = 1; pa.id = (uint16_t)p; pa.x = 120; pa.tx = 120; pa.y = 228;
            G.rooms[G.house][R_LIVING].actors.push_back(pa);
            toast(std::string("You adopted ") + PET_NAMES[p] + " the " + PET_KINDS[p] + "! (at home)"); sfxYay();
            addMemory(MEM_FAMILY, G.mc, -1, std::string("Adopted ") + PET_NAMES[p] + " the " + PET_KINDS[p] + "!"); return;
        }
        case VA_CLASS: a = mk("In Class", 180, 30); a.kind = AK_CLASS; a.rate[N_FUN] = -5; a.rate[N_SOCIAL] = 5; a.skill = SK_LOGIC; a.skillRate = 4; startActivity(a); return;
        case VA_LATTE: if (G.money < 8) return; G.money -= 8; me.needs[N_ENERGY] = std::fmin(100.f, me.needs[N_ENERGY] + 25); addMoodlet(me, "Caffeinated", EM_ENERGIZED, 1, 120); if (G.date.active) G.date.score += 8; return;
        case VA_MUFFIN: if (G.money < 5) return; G.money -= 5; me.needs[N_HUNGER] = std::fmin(100.f, me.needs[N_HUNGER] + 25); return;
        case VA_MASSAGE: { int cost = eventHere(R_SPA) && G.room == R_SPA ? 20 : 40; if (G.money < cost) { toast("Not enough money!"); return; } G.money -= cost; } a = mk("Spa Massage", 60, 8); a.rate[N_HYGIENE] = 40; a.rate[N_ENERGY] = 25; a.rate[N_FUN] = 25; startActivity(a); addMoodlet(me, "Pampered", EM_HAPPY, 2, 300); return;
        case VA_MAKEOVER:
            if (G.money < 25) { toast("Not enough money!"); return; }
            G.money -= 25; if (!me.look.guy) me.look.hairStyle = (uint8_t)((me.look.hairStyle + 1) % NUM_HAIRSTYLES); me.look.hairCol = (uint8_t)irand(7);
            addMoodlet(me, "New Look!", EM_CONFIDENT, 2, 300); toast("Makeover done! Gorgeous!"); fxBurst(200, 80, 15); return;
        case VA_DANCE: case VA_CHEER:
            a = mk(v == VA_DANCE ? "Dancing" : "Cheer Practice", 60, 8, PO_DANCE); a.kind = AK_DANCE; a.rate[N_FUN] = 35; a.rate[N_ENERGY] = -10; a.rate[N_SOCIAL] = 15; a.skill = SK_FITNESS; a.skillRate = 4; startActivity(a); return;
        case VA_MOCKTAIL: if (G.money < 10) return; G.money -= 10; me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 15); addMoodlet(me, "Fancy Sips", EM_PLAYFUL, 1, 120); if (G.date.active) G.date.score += 8; return;
        case VA_SWIM: a = mk("Swimming", 60, 8, PO_WAVE); a.kind = AK_SWIM; a.rate[N_FUN] = 30; a.rate[N_HYGIENE] = 10; a.rate[N_ENERGY] = -8; a.skill = SK_FITNESS; a.skillRate = 5; startActivity(a); if (G.date.active) G.date.score += 12; return;
        case VA_TAN: a = mk("Tanning", 60, 8); a.rate[N_FUN] = 20; a.rate[N_ENERGY] = 10; startActivity(a); addMoodlet(me, "Sun-Kissed", EM_HAPPY, 1, 240); return;
        case VA_SCHOOL: a = mk(schoolName(me.stage), 7 * 60, 50); a.kind = AK_SCHOOL; a.rate[N_SOCIAL] = 8; a.rate[N_FUN] = -3; a.skill = SK_LOGIC; a.skillRate = 2; startActivity(a); toast(std::string("Off to ") + schoolName(me.stage) + "!"); return;
        }
        return;
    }
    if (o.id >= 2100) {   // pets
        if (menuT.kind != 1 || menuT.idx < 0 || menuT.idx >= (int)curRoom().actors.size()) return;
        Actor& pa = curRoom().actors[menuT.idx];
        me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 12);
        me.needs[N_SOCIAL] = std::fmin(100.f, me.needs[N_SOCIAL] + 6);
        static const char* lines[3] = {"Who's a good baby?", "Dinner time!", "Go get it!"};
        say(G.mc, lines[o.id - 2100]);
        pa.say = pa.id == 4 ? "*blub*" : pa.id == 1 ? "Purrr..." : "<3"; pa.sayT = 2;
        if (me.hasTrait(TR_PETLOVER)) { addMoodlet(me, "Best Friend Fur-ever", EM_HAPPY, 2, 240); me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 15); }
        addMoodlet(me, "Pet Love", EM_HAPPY, 1, 120);
        return;
    }
    if (o.id >= 2000) {
        int act = o.id - 2000;
        {   // utilities: unpaid bills cut the power / water
            const bool pw = act == IA_TV || act == IA_WEB || act == IA_COOK || act == IA_ARCADE || act == IA_MUSIC || act == IA_CONTENT || act == IA_LIGHTS || act == IA_STREAMGAME || act == IA_LAPBLOG || act == IA_LAPBINGE || act == IA_LAPHOMEWORK;
            const bool wt = act == IA_SHOWER || act == IA_BATH || act == IA_TOILET || act == IA_BRUSH || act == IA_WATER;
            if ((pw || wt) && !economyUtility(pw, wt)) return;
        }
        Activity a;
        switch (act) {   // let wants know what you're up to
        case IA_SLEEP: goalNotify(GE_SLEEP); break;
        case IA_DANCE: goalNotify(GE_DANCE); break;
        case IA_BATH: case IA_SHOWER: goalNotify(GE_BATH); break;
        case IA_TV: goalNotify(GE_TV); break;
        case IA_READ: goalNotify(GE_READ); break;
        case IA_MAKEUP: goalNotify(GE_MAKEUP); break;
        case IA_TOYS: case IA_PRETEND: case IA_TEAPARTY: goalNotify(GE_TOYS); break;
        case IA_HOMEWORK: goalNotify(GE_HOMEWORK); break;
        default: break;
        }
        switch (act) {
        case IA_SLEEP: { int hh = (int)(G.minute / 60); a = mk("Sleeping", 8 * 60, (hh >= 19 || hh < 5) ? 1800 : 60, PO_SLEEP); } a.kind = AK_SLEEP; a.rate[N_ENERGY] = 14; startActivity(a); if (ma) ma->sleeping = true; return;
        case IA_NAP: a = mk("Napping", 90, 25, PO_SLEEP); a.kind = AK_SLEEP; a.rate[N_ENERGY] = 14; startActivity(a); return;
        case IA_RELAX: a = mk("Relaxing", 60, 8); a.rate[N_FUN] = 15; a.rate[N_ENERGY] = 10; startActivity(a); return;
        case IA_SUNBATHE: a = mk("Sunbathing", 60, 8); a.rate[N_FUN] = 20; a.rate[N_ENERGY] = 10; startActivity(a); addMoodlet(me, "Sun-Kissed", EM_HAPPY, 1, 180); return;
        case IA_TV: a = mk("Watching TV", 60, 8); a.rate[N_FUN] = 35; startActivity(a); return;
        case IA_READ: a = mk("Reading", 60, 8); a.rate[N_FUN] = 15; a.skill = SK_LOGIC; a.skillRate = 5; startActivity(a); return;
        case IA_NOVEL: a = mk("Writing a Novel", 120, 12); a.kind = AK_NOVEL; a.rate[N_FUN] = 5; a.skill = SK_CREATIVITY; a.skillRate = 5; startActivity(a); return;
        case IA_HOMEWORK: a = mk("Homework", 60, 10); a.kind = AK_HOMEWORK; a.rate[N_FUN] = -10; a.skill = SK_LOGIC; a.skillRate = 5; startActivity(a); return;
        case IA_WEB: a = mk("Browsing the Web", 60, 8); a.rate[N_FUN] = 25; a.rate[N_SOCIAL] = 10; startActivity(a); return;
        case IA_DIARY: {
            std::string s;
            if (askText("Dear Diary...", s, 240)) { G.diary.push_back({dateStr(G.day) + " " + clockStr(G.minute), s}); toast("Saved in your secret diary!"); me.needs[N_FUN] += 8; addMoodlet(me, "Got It Off My Chest", EM_HAPPY, 1, 120); }
            return;
        }
        case IA_MAKEUP: a = mk("Doing Makeup", 30, 6); a.rate[N_HYGIENE] = 15; a.rate[N_FUN] = 10; a.skill = SK_STYLE; a.skillRate = 8; startActivity(a); addMoodlet(me, "Flawless Face", EM_CONFIDENT, 1, 240); return;
        case IA_SPEECH: a = mk("Practicing Speech", 60, 8); a.skill = SK_CHARISMA; a.skillRate = 6; startActivity(a); return;
        case IA_PLANOUTFIT: a = mk("Planning Outfits", 60, 8); a.rate[N_FUN] = 12; a.skill = SK_STYLE; a.skillRate = 6; startActivity(a); return;
        case IA_OUTFIT:
            opts.clear();
            for (int k = 0; k < NUM_OUTFITS; k++) if ((ownsOutfit(k) || (k >= FIRST_KID_OUTFIT && k < FIRST_MALE_OUTFIT && MC().stage <= ST_PRETEEN) || (k >= FIRST_MALE_OUTFIT && MC().stage >= ST_TEEN)) && outfitFits(k, MC().stage, MC().look.guy)) opts.push_back({OUTFITS[k].name, 4000 + k});
            menuOpen = true; optPage = 0;
            return;
        case IA_COOK: case IA_BBQ: cookStart(); mode = M_COOK; return;
        case IA_SNACK: me.needs[N_HUNGER] = std::fmin(100.f, me.needs[N_HUNGER] + 20); say(G.mc, "*munch*"); return;
        case IA_BATH: a = mk(me.isLittle() ? "Bath Time" : "Bubble Bath", 30, 6); a.rate[N_HYGIENE] = 160; a.rate[N_FUN] = 10; startActivity(a); return;
        case IA_PRETEND: {
            static const char* P[] = {"Pretend: Princess Castle", "Pretend: Space Explorer", "Pretend: Fashion Show", "Pretend: Pet Hospital"};
            a = mk(P[irand(4)], 45, 8, PO_WAVE); a.rate[N_FUN] = 45; a.skill = SK_CREATIVITY; a.skillRate = 5; startActivity(a);
            addMoodlet(me, "Imagination Station", EM_PLAYFUL, 1, 120); return;
        }
        case IA_TEAPARTY: a = mk("Tea Party", 40, 8); a.rate[N_FUN] = 35; a.rate[N_SOCIAL] = 25; a.skill = SK_CHARISMA; a.skillRate = 4; startActivity(a); say(G.mc, "More tea, Mr. Teddy?"); return;
        case IA_FORT: a = mk("Building a Blanket Fort", 50, 8); a.rate[N_FUN] = 50; a.rate[N_ENERGY] = -4; a.skill = SK_CREATIVITY; a.skillRate = 4; startActivity(a); addMoodlet(me, "Best Fort Ever", EM_HAPPY, 2, 240); return;
        case IA_CRAYONS: a = mk("Drawing with Crayons", 40, 8); a.rate[N_FUN] = 30; a.skill = SK_CREATIVITY; a.skillRate = 6; startActivity(a); addMoodlet(me, "Little Artist", EM_INSPIRED, 1, 120); return;
        case IA_STUDYCALL: a = mk("Study Group Call", 60, 10, PO_PHONE); a.kind = AK_HOMEWORK; a.rate[N_SOCIAL] = 30; a.rate[N_FUN] = 5; a.skill = SK_LOGIC; a.skillRate = 5; startActivity(a); return;
        case IA_TUTORIAL: {
            a = mk("Filming a Makeup Tutorial", 60, 10); a.rate[N_FUN] = 20; a.skill = SK_STYLE; a.skillRate = 6; startActivity(a);
            uint32_t f = 20 + skillLevel(me, SK_STYLE) * 15 + irand(40);
            G.followers += f;
            Post p; p.sim = (int16_t)G.mc; p.text = irand(2) ? "new GRWM tutorial is UP!! link in bio" : "soft glam tutorial, the blush tho <3"; p.likes = f * 3; G.posts.insert(G.posts.begin(), p);
            if (G.posts.size() > 30) G.posts.pop_back();
            toast("Tutorial posted! +" + std::to_string(f) + " followers"); return;
        }
        case IA_VIRALDANCE: {
            a = mk("Learning a Viral Dance", 45, 8, PO_DANCE); a.rate[N_FUN] = 35; a.rate[N_ENERGY] = -10; a.skill = SK_FITNESS; a.skillRate = 5; startActivity(a);
            uint32_t f = 10 + skillLevel(me, SK_FITNESS) * 10 + irand(30); G.followers += f;
            toast("Nailed it! Posted the dance: +" + std::to_string(f) + " followers"); if (!audioIsPlaying()) { musicPlay(1); G.track = 1; } return;
        }
        case IA_LEMONADE: {
            int cups = 3 + irand(6) + skillLevel(me, SK_CHARISMA);
            G.money += cups * 3;
            a = mk("Lemonade Stand", 60, 10, PO_WAVE); a.rate[N_SOCIAL] = 20; a.rate[N_FUN] = 15; a.skill = SK_CHARISMA; a.skillRate = 5; startActivity(a);
            toast("Lemonade, 50 cents! You sold " + std::to_string(cups) + " cups: +$" + std::to_string(cups * 3)); return;
        }
        case IA_SLEEPOVER: { milestone(G.mc, MI_SLEEPOVER);
            int best = -1, bestFr = -999;
            const bool teen = me.stage == ST_TEEN;
            for (const Rel& r : G.rels) {
                int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1;
                if (o < 0 || G.sims[o].household) continue;
                const Sim& os = G.sims[o];
                bool sameAge = os.stage >= ST_CHILD && os.stage <= ST_TEEN && ((os.stage == ST_TEEN) == teen);
                if (sameAge && r.fr > bestFr) { bestFr = r.fr; best = o; }
            }
            if (best < 0) {
                std::vector<int> pool;
                for (size_t i = 0; i < G.sims.size(); i++) if (!G.sims[i].household && G.sims[i].stage >= ST_CHILD && G.sims[i].stage <= ST_TEEN && ((G.sims[i].stage == ST_TEEN) == teen)) pool.push_back((int)i);
                if (pool.empty()) { toast("No friends your age to invite yet!"); return; }
                best = pool[irand((int)pool.size())];
            }
            bool here = false;
            for (const Actor& ac : curRoom().actors) if (ac.kind == 0 && ac.id == best) here = true;
            if (!here) { Actor f; f.kind = 0; f.id = (uint16_t)best; Actor* mca = mcActor(); f.x = mca ? clampv(mca->x + 40, 20.f, 380.f) : 240; f.tx = f.x; f.y = 226; curRoom().actors.push_back(f); }
            dlgSay(best, teen ? "Sleepover!! I brought face masks, snacks AND the scary movie." : "Sleepover!! Can we stay up REALLY late?");
            dlgSay(G.mc, "Pillow fort, then truth or dare. Let's gooo!");
            changeRel(G.mc, best, 12, 0);
            a = mk("Sleepover!", 120, 12, PO_DANCE); a.rate[N_FUN] = 40; a.rate[N_SOCIAL] = 45; a.rate[N_ENERGY] = -6; startActivity(a);
            addMoodlet(me, "Best Sleepover Ever", EM_HAPPY, 2, 600);
            return;
        }
        case IA_TOYS: a = mk("Playing", 45, 8); a.rate[N_FUN] = 40; a.skill = SK_CREATIVITY; a.skillRate = 3; startActivity(a); return;
        case IA_DANCE: a = mk("Dancing", 60, 8, PO_DANCE); a.rate[N_FUN] = 30; a.rate[N_ENERGY] = -8; a.skill = SK_FITNESS; a.skillRate = 4; startActivity(a); if (!audioIsPlaying()) { musicPlay(1); G.track = 1; } return;
        case IA_MUSIC:
            opts.clear();
            for (int k = 0; k < NUM_TRACKS; k++) opts.push_back({TRACK_NAMES[k], 6000 + k});
            opts.push_back({"Music Off", 6100});
            menuOpen = true; optPage = 0;
            return;
        case IA_PARTY: if (partyMins > 0) { toast("The party's already on!"); return; } startParty(); return;
        case IA_ARCADE: opts.clear(); for (int k = 0; k < NUM_MINIGAMES; k++) opts.push_back({MG_NAMES[k], 5000 + k}); menuOpen = true; optPage = 0; return;
        case IA_WATER: say(G.mc, "Grow, babies!"); me.needs[N_FUN] += 3; return;
        case IA_ADMIRE: addMoodlet(me, "Inspired by Art", EM_INSPIRED, 2, 180); say(G.mc, "So chic..."); return;
        case IA_LIGHTS: G.lightsOn = !G.lightsOn; return;
        case IA_ROCK: for (Actor& x : curRoom().actors) if (x.kind == 0 && G.sims[x.id].stage == ST_INFANT) { G.sims[x.id].needs[N_ENERGY] = 100; x.sleeping = true; } say(G.mc, "Shhh..."); return;
        case IA_SIT: return;   // sitting happens when we arrive at the seat
        case IA_GETUP: if (ma) { cancelActivity(); standUp(*ma, curRoom()); } return;
        case IA_STREAMGAME: a = mk("Streaming a Game", 90, 8); a.rate[N_FUN] = 30; a.rate[N_SOCIAL] = 8; a.skill = SK_LOGIC; a.skillRate = 3; startActivity(a); G.followers += 20 + skillLevel(me, SK_LOGIC) * 10; say(G.mc, "Hey chat! Let's GO!"); return;
        case IA_MUSICPOD: a = mk("Listening to SugarPod", 45, 6); a.rate[N_FUN] = 20; startActivity(a); addMoodlet(me, "Playlist Mood", EM_HAPPY, 1, 150); return;
        case IA_LAPBLOG: a = mk("Writing a Blog Post", 60, 8); a.rate[N_FUN] = 10; a.skill = SK_CREATIVITY; a.skillRate = 5; startActivity(a); G.followers += 15 + irand(30); return;
        case IA_LAPBINGE: a = mk("Binging a Show", 90, 6); a.rate[N_FUN] = 28; startActivity(a); return;
        case IA_LAPHOMEWORK: a = mk("Homework on Laptop", 60, 8); a.kind = AK_HOMEWORK; a.rate[N_FUN] = -2; a.skill = SK_LOGIC; a.skillRate = 6; startActivity(a); me.gradePts = std::fmin(100.f, me.gradePts + 3); addMoodlet(me, "In the Zone", EM_FOCUSED, 2, 120); return;
        case IA_EREAD: a = mk("Reading on E-Reader", 60, 8); a.rate[N_FUN] = 22; a.skill = me.hasTrait(TR_BOOKWORM) ? SK_LOGIC : SK_CREATIVITY; a.skillRate = 3; startActivity(a); addMoodlet(me, "Lost in a Book", EM_INSPIRED, 1, 180); return;
        case IA_RELAXBED: a = mk("Relaxing in Bed", 60, 8); a.rate[N_ENERGY] = 12; a.rate[N_FUN] = 12; startActivity(a); return;
        case IA_SHOWER: a = mk("Showering", 20, 6); a.rate[N_HYGIENE] = 260; startActivity(a); say(G.mc, "La la la~"); return;
        case IA_TOILET: a = mk("Using the Toilet", 6, 3); a.rate[N_BLADDER] = 1000; startActivity(a); return;
        case IA_BRUSH: me.needs[N_HYGIENE] = std::fmin(100.f, me.needs[N_HYGIENE] + 12); addMoodlet(me, "Minty Fresh", EM_HAPPY, 1, 90); say(G.mc, "*brush brush*"); return;
        case IA_QUICKMEAL:
            if (G.groceries > 0) { G.groceries--; a = mk("Eating a Home-Cooked Meal", 30, 6); a.rate[N_HUNGER] = 150; startActivity(a); addMoodlet(MC(), "Fresh Groceries", EM_HAPPY, 1, 120); say(G.mc, G.groceries ? "Fresh from Sugar Market. Yum!" : "That was the last of the groceries..."); return; }
            a = mk("Eating a Quick Meal", 30, 6); a.rate[N_HUNGER] = 110; startActivity(a); say(G.mc, "Cereal for dinner! (Stock up at Sugar Market)"); return;
        case IA_STUDY: a = mk("Studying", 60, 8); a.rate[N_FUN] = -5; a.skill = SK_LOGIC; a.skillRate = 7; startActivity(a); if (me.enrolled && !me.degree) me.credits += 0.25f; return;
        case IA_GLAMSTATION: casStartEdit(G.mc, 0); mode = M_GLAM; return;
        case IA_HACK: crimeAttempt(CRIME_HACK); return;
        case IA_CHECKLOOK: {
            int st = skillLevel(me, SK_STYLE);
            static const char* lines[] = {"Hmm... could be better.", "Looking good!", "Wow. I look AMAZING.", "Serving looks today."};
            say(G.mc, lines[std::min(3, st / 3 + (me.outfit ? 1 : 0))]);
            addMoodlet(me, st >= 5 ? "Feeling Myself" : "Checked My Look", EM_CONFIDENT, st >= 5 ? 2 : 1, 120);
            return;
        }
        case IA_CONTENT: {
            a = mk("Creating Content", 90, 10); a.rate[N_FUN] = 15; a.rate[N_SOCIAL] = 8; a.skill = SK_CREATIVITY; a.skillRate = 4; startActivity(a);
            uint32_t f = 15 + skillLevel(me, SK_CREATIVITY) * 6 + skillLevel(me, SK_STYLE) * 4 + G.followers / 60;
            G.followers += f;
            toast("Your new video went up! +" + std::to_string(f) + " followers");
            return;
        }
        case IA_SOCIALAPP: phoneOpen(AP_SOCIAL); mode = M_PHONE; return;
        case IA_JOBS: phoneOpen(AP_JOBS); mode = M_PHONE; return;
        case IA_PILLOW: {
            int p = partnerHere();
            if (p < 0) return;
            a = mk("Pillow Talk", 40, 8); a.rate[N_SOCIAL] = 60; a.rate[N_FUN] = 20; startActivity(a);
            static const char* talk[] = {
                "Today was so long... but this part is my favorite.", "Remember our first date? You were SO nervous. It was cute.",
                "Okay, real talk. Where do you see us in five years?", "You know you're my favorite person, right?",
                "I've been thinking... we make a really good team."};
            dlgSay(p, talk[irand(5)]);
            changeRel(G.mc, p, 3, 6);
            addMoodlet(me, "Sweet Pillow Talk", EM_FLIRTY, 2, 240);
            putPartnerInBed(p);
            return;
        }
        case IA_MAGIC: {
            int p = partnerHere();
            if (p < 0) return;
            const Rel* r = findRel(G.mc, p);
            if (!r || r->ro < 35 || irand(100) < 15) {
                dlgSay(p, "Not tonight, babe... I'm just really tired. Cuddle instead?");
                changeRel(G.mc, p, 1, 1);
                return;
            }
            putPartnerInBed(p);
            fadeT = 3.f;
            changeRel(G.mc, p, 4, 10);
            addMoodlet(me, "Made a Little Magic", EM_HAPPY, 3, 480);
            me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 30);
            me.needs[N_ENERGY] = std::fmax(0.f, me.needs[N_ENERGY] - 10);
            if (G.pregnancyOn) {
                Sim& s = G.sims[p];
                Sim* mom = !me.look.guy ? &me : !s.look.guy ? &s : nullptr;
                if (mom && !mom->pregnant && !lxBirthControl((int)(mom - &G.sims[0])) && irand(100) < 45) { mom->pregnant = true; mom->pregDays = 3; pendingMagicNews = true; }
            }
            return;
        }
        }
        return;
    }
    if (o.id >= 1700 && o.id < 1900) {   // content-pack gift or cooked dish
        if (menuT.kind != 1 || menuT.idx >= (int)curRoom().actors.size()) return;
        const Actor& ta = curRoom().actors[menuT.idx];
        if (ma) ma->facingLeft = ta.x < ma->x;
        contentGive(o.id, ta.id);
        return;
    }
    if (o.id >= 1950 && o.id < 1950 + NUM_GIFTS) {   // hand over a gift
        if (menuT.kind != 1 || menuT.idx >= (int)curRoom().actors.size()) return;
        const Actor& ta = curRoom().actors[menuT.idx];
        if (ma) ma->facingLeft = ta.x < ma->x;
        giveGift(o.id - 1950, ta.id);
        return;
    }
    if (o.id >= 1900 && o.id < 2000) {   // open a social category
        if (menuT.kind != 1 || menuT.idx < 0 || menuT.idx >= (int)curRoom().actors.size()) return;
        int cat = o.id - 1900;
        const Actor& ta = curRoom().actors[menuT.idx];
        opts.clear();
        for (const SocialOpt& s : socialOptions(ta.id)) if (s.cat == cat) opts.push_back({s.name, 1000 + s.id});
        menuOpen = !opts.empty(); optPage = 0;
        return;
    }
    if (o.id >= 1000) {
        const Actor& ta = curRoom().actors[menuT.idx];
        int t = ta.id;
        if (ma) ma->facingLeft = ta.x < ma->x;
        if (o.id - 1000 == SO_GIFT) {   // pick what to give
            opts.clear();
            for (int k = 0; k < NUM_GIFTS; k++) if (basketCount(k)) opts.push_back({std::string(GIFT_NAMES[k]) + " (x" + std::to_string(basketCount(k)) + ")", 1950 + k});
            { std::vector<std::pair<std::string, int>> cg; contentGiftOptions(cg); for (auto& p : cg) opts.push_back({p.first, p.second}); }
            menuOpen = !opts.empty(); optPage = 0;
            return;
        }
        doSocial(o.id - 1000, t);
        return;
    }
}

// ===================================================================== actors
static const char* CHATTER[] = {"Love your shoes!", "Hiii!", "OMG same", "Brunch later?", "So cute!", "Totally!", "Did you hear?!", "Lol", "<3", "Whatever!"};

// ---------------------------------------------------------------- seats & walking
static bool seatTaken(const RoomState& rs, int idx, int slot, const Actor* except) {
    for (const Actor& o : rs.actors) {
        if (&o == except || o.kind != 0) continue;
        if (o.seat == idx && o.seatSlot == slot && o.useKind) return true;
        if (o.goSeat == idx && o.goSlot == slot) return true;
    }
    return false;
}
static int freeSlot(const RoomState& rs, int idx, const Actor* except) {
    UseSpot u = useSpot(itemShape(rs.items[idx].item));
    for (int s = 0; s < u.slots; s++) if (!seatTaken(rs, idx, s, except)) return s;
    return -1;
}
static void standUp(Actor& a, const RoomState& rs) {
    if (a.seat >= 0 && a.seat < (int)rs.items.size()) {
        const Placed& pl = rs.items[a.seat];
        UseSpot u = useSpot(itemShape(pl.item));
        a.x = pl.x + u.slotDX[a.seatSlot % 2];
        a.y = clampv((float)pl.y + 8.f, 168.f, 238.f);
    }
    a.seat = -1; a.useKind = 0; a.useT = 0;
    a.sleeping = false;
}
static void sitDown(Actor& a, const RoomState& rs, int idx, int slot) {
    if (idx < 0 || idx >= (int)rs.items.size()) return;
    const Placed& pl = rs.items[idx];
    UseSpot u = useSpot(itemShape(pl.item));
    a.seat = (int16_t)idx; a.seatSlot = (uint8_t)slot; a.useKind = u.kind;
    a.x = pl.x + u.slotDX[slot % 2];
    a.y = pl.y + 0.6f;
    a.walking = false;
}
static void walkTo(Actor& a, float x, float y) {
    a.tx = clampv(x, 16.f, 384.f);
    a.ty = clampv(y, 168.f, 238.f);
    a.walking = true;
}
// moves an actor toward (tx, ty); returns true when it arrives
static bool stepWalk(Actor& a, float speed, float dt) {
    if (!a.walking) return false;
    float dx = a.tx - a.x, dy = a.ty - a.y;
    float d = std::sqrt(dx * dx + dy * dy);
    if (d < 2.f) { a.x = a.tx; a.y = a.ty; a.walking = false; return true; }
    float stp = std::fmin(d, speed * dt);
    a.x += dx / d * stp; a.y += dy / d * stp;
    if (std::fabs(dx) > 1) a.facingLeft = dx < 0;
    a.walkPhase = std::fmod(a.walkPhase + dt * speed / 38.f, 1.f);
    return false;
}

static void putPartnerInBed(int p) {
    RoomState& rs = curRoom();
    Actor* me = mcActor();
    if (!me || me->seat < 0) return;
    for (Actor& a : rs.actors) {
        if (a.kind != 0 || a.id != p) continue;
        int s2 = freeSlot(rs, me->seat, &a);
        if (s2 < 0) return;
        standUp(a, rs);
        sitDown(a, rs, me->seat, s2);
        a.useT = 60.f;
        return;
    }
}

static void updateActors(float dt, float gameMins) {
    // Style Savvy / Sims-style foot traffic in town: people wander in from the edges,
    // browse for a while, stroll straight past, and head off again
    if (!roomIsHome(G.room) && G.room != R_NEIGHBOR && !G.sims.empty()) {
        static float spawnT = 3.f;
        spawnT -= dt;
        if (spawnT <= 0) {
            bool out = roomOutdoors(G.room);
            spawnT = (out ? 4.f : 7.f) + frand() * (out ? 6.f : 9.f);
            RoomState& rs0 = curRoom();
            int here = 0;
            for (const Actor& a : rs0.actors) if (a.kind == 0 && !G.sims[a.id].household) here++;
            int cap = 5 + (fameStars() >= 3 ? 1 : 0) + (out ? 1 : 0);
            int hr = (int)(G.minute / 60);
            if (hr >= 23 || hr < 6) cap = 2;
            for (int tries = 0; tries < 20 && here < cap; tries++) {
                // mostly people whose routine brings them here now; sometimes a random passer-by
                int id = tries < 4 && irand(100) < 70 ? lifePickTownie(G.room) : -1;
                if (id < 0) id = irand((int)G.sims.size());
                if (id == G.mc || id == G.cop || !townieFits(G.room, G.sims[id]) || G.sims[id].stage <= ST_TODDLER || lifeAway(id)) continue;
                bool dup = false; for (const Actor& a : rs0.actors) if (a.kind == 0 && a.id == id) dup = true;
                if (dup) continue;
                Actor a; a.kind = 0; a.id = (uint16_t)id;
                bool fromLeft = irand(2) == 0;
                a.x = fromLeft ? -24.f : 424.f; a.y = 186.f + irand(48);
                if (irand(100) < (out ? 55 : 30)) { a.passer = 1; a.tx = fromLeft ? 430.f : -30.f; a.ty = a.y; a.walking = true; }
                else { a.tx = 40.f + irand(320); a.ty = 186.f + irand(48); a.walking = true; a.stay = (float)(40 + irand(140)); }
                if (irand(5) == 0) { static const char* const HI[6] = {"Just passing through!", "Ooh, cute place.", "Has anyone seen my phone?", "Running late, sorry!", "Love this song!", "Window shopping is cardio."}; a.say = HI[irand(6)]; a.sayT = 2.5f; }
                rs0.actors.push_back(a);
                break;
            }
        }
    }
    chatterTick(dt * guAutonomy());
    RoomState& rs = curRoom();
    int hour = (int)(G.minute / 60);
    bool night = hour >= 23 || hour < 6;
    for (size_t i = 0; i < rs.actors.size();) {
        Actor& a = rs.actors[i];
        if (a.sayT > 0) a.sayT -= dt;
        if (a.seat >= (int)rs.items.size()) { a.seat = -1; a.useKind = 0; }
        if (a.goSeat >= (int)rs.items.size()) a.goSeat = -1;
        bool isMC = a.kind == 0 && a.id == G.mc;
        const bool town = !roomIsHome(G.room) && G.room != R_NEIGHBOR;
        // foot traffic: people strolling through, or heading off after a while
        if (a.passer && !isMC) {
            if (stepWalk(a, a.passer == 1 ? 32.f : 36.f, dt) || !a.walking) {
                if (!menuOpen) { rs.actors.erase(rs.actors.begin() + i); continue; }
                a.tx = a.x < 200 ? -30.f : 430.f; a.ty = a.y; a.walking = true;
            }
            i++;
            continue;
        }
        if (a.stay > 0 && !isMC) {
            a.stay -= gameMins;
            if (a.stay <= 0 && !(G.date.active && G.date.partner == a.id)) {
                if (town) {   // walk off-screen instead of vanishing
                    if (a.seat >= 0) standUp(a, rs);
                    a.goSeat = -1; a.passer = 2; a.sleeping = false;
                    a.tx = a.x < 200 ? -30.f : 430.f; a.ty = a.y; a.walking = true;
                    i++;
                    continue;
                }
                if (menuOpen) { a.stay = 5; i++; continue; }
                toast(G.sims[a.id].name + " went home.");
                rs.actors.erase(rs.actors.begin() + i);
                continue;
            }
        }
        if (isMC) {
            if (ACT.active()) { a.sleeping = ACT.pose == PO_SLEEP; a.pose = ACT.pose; a.poseT = 0.5f; }
            else { a.sleeping = false; if (a.poseT > 0) a.poseT -= dt; }
            stepWalk(a, MC().stage <= ST_TODDLER ? 40.f : 78.f, dt);
            i++;
            continue;
        }
        if (a.poseT > 0) a.poseT -= dt;
        if (a.kind == 1) {   // pets just roam
            a.sleeping = night;
            if (!a.sleeping) {
                a.wait -= dt;
                if (a.wait <= 0) { walkTo(a, 30 + frand() * 340, 200 + frand() * 36); a.wait = 3 + frand() * 5; }
                stepWalk(a, 40.f, dt);
            }
            i++;
            continue;
        }
        const Sim& sm = G.sims[a.id];
        if (sm.stage == ST_INFANT) { a.sleeping = night; i++; continue; }
        // using furniture
        if (a.seat >= 0) {
            a.useT -= dt;
            bool wake = a.sleeping && !night;
            if ((a.useT <= 0 && !a.sleeping) || wake) standUp(a, rs);
            i++;
            continue;
        }
        // heading to a seat / bed
        if (a.goSeat >= 0) {
            if (stepWalk(a, 30.f, dt) || !a.walking) {
                int idx = a.goSeat, slot = a.goSlot;
                a.goSeat = -1;
                if (!seatTaken(rs, idx, slot, &a)) {
                    sitDown(a, rs, idx, slot);
                    a.sleeping = a.goSleep;
                    a.useT = a.goSleep ? 9999.f : 8 + frand() * 18;
                }
                a.goSleep = false;
            }
            i++;
            continue;
        }
        bool householdNight = sm.household && roomIsHome(G.room) && night && !partyMins;
        if (G.date.active && a.id == G.date.partner) {   // dates stick close to you
            Actor* me = mcActor();
            if (me->seat >= 0) {   // sit near you if there's room
                int s2 = freeSlot(rs, me->seat, &a);
                if (s2 >= 0 && a.goSeat < 0) { const Placed& pl = rs.items[me->seat]; UseSpot u = useSpot(itemShape(pl.item)); a.goSeat = me->seat; a.goSlot = (uint8_t)s2; walkTo(a, pl.x + u.slotDX[s2 % 2], pl.y + 6); }
            } else if (std::fabs(a.x - me->x) > 50 || std::fabs(a.y - me->y) > 20) walkTo(a, me->x + (me->x > 200 ? -38 : 38), me->y);
            stepWalk(a, 40.f, dt);
            i++;
            continue;
        }
        const int alv = sm.household ? l3AutoOf(a.id) : 2;
        if (alv == 0 && !householdNight) { i++; continue; }   // autonomy off: they wait for you
        a.wait -= dt;
        if (a.wait <= 0 && !a.walking) {
            a.wait = alv == 1 ? 8 + frand() * 10 : 3 + frand() * 6;
            int choice = irand(100);
            bool wantSleep = householdNight;
            if (wantSleep || (choice < 35 && sm.stage >= ST_CHILD)) {
                // look for somewhere to sit (or a bed at night)
                std::vector<std::pair<int, int>> spots;
                for (size_t k = 0; k < rs.items.size(); k++) {
                    UseSpot u = useSpot(itemShape(rs.items[k].item));
                    bool fits = wantSleep ? (u.kind == 2 && baseShape(itemShape(rs.items[k].item)) == SH_BED) : (u.kind == 1 && itemShape(rs.items[k].item) != SH_TOILET);
                    if (!fits) continue;
                    int s = freeSlot(rs, (int)k, &a);
                    if (s >= 0) spots.push_back({(int)k, s});
                }
                if (!spots.empty()) {
                    auto sp = spots[irand((int)spots.size())];
                    const Placed& pl = rs.items[sp.first];
                    UseSpot u = useSpot(itemShape(pl.item));
                    a.goSeat = (int16_t)sp.first; a.goSlot = (uint8_t)sp.second; a.goSleep = wantSleep;
                    walkTo(a, pl.x + u.slotDX[sp.second % 2], pl.y + 6);
                } else if (wantSleep) a.sleeping = true;
            } else {
                a.sleeping = false;
                walkTo(a, 30 + frand() * 340, 172 + frand() * 64);
                if (sm.stage >= ST_CHILD && a.sayT <= 0 && irand(100) < 22) {   // NPCs act out their traits
                    struct TL { uint8_t tr; const char* l[3]; uint8_t pose; };
                    static const TL TLS[] = {
                        {TR_DANCER, {"*hits a spin*", "This floor is my stage.", "5, 6, 7, 8!"}, PO_DANCE},
                        {TR_MUSIC, {"*humming a tune*", "This song is stuck in my head!", "Who's playing music?"}, PO_DANCE},
                        {TR_FOODIE, {"I could eat.", "Have you tried that new sushi place?", "Snack break?"}, PO_IDLE},
                        {TR_BOOKWORM, {"Just one more chapter...", "Have you read it? It's SO good.", "*nose in a book*"}, PO_IDLE},
                        {TR_FASHIONISTA, {"That fit? Iconic.", "Is this color in season?", "Outfit check!"}, PO_WAVE},
                        {TR_GLAMOROUS, {"Is my lighting okay?", "Darling!", "*poses*"}, PO_WAVE},
                        {TR_DIVA, {"Where's my iced coffee?", "I don't DO mornings.", "Don't talk to me before noon."}, PO_IDLE},
                        {TR_GOOFBALL, {"Pull my finger!", "Knock knock!", "*makes a silly face*"}, PO_WAVE},
                        {TR_HOTHEAD, {"Ugh, EVERYTHING is annoying today.", "Who moved my stuff?!", "*grumbles*"}, PO_IDLE},
                        {TR_ACTIVE, {"Anyone want to go for a run?", "Leg day!", "*does lunges*"}, PO_DANCE},
                        {TR_LAZY, {"*yawns*", "Is it nap time yet?", "Five more minutes..."}, PO_IDLE},
                        {TR_ROMANTIC, {"Love is in the air~", "Have you ever been in love?", "*daydreams*"}, PO_IDLE},
                        {TR_FLIRTY, {"Hey there~", "*winks*", "You come here often?"}, PO_WAVE},
                        {TR_GAMER, {"Just hit a new high score!", "GG.", "One more round..."}, PO_IDLE},
                        {TR_NERD, {"Did you know octopuses have 3 hearts?", "Technically...", "*pushes up glasses*"}, PO_IDLE},
                        {TR_DRAMATIC, {"I'm literally DYING.", "This is the WORST day of my life!", "*gasps*"}, PO_IDLE},
                        {TR_SHY, {"Oh! Um... hi.", "*looks at shoes*", "..."}, PO_IDLE},
                        {TR_CHEERFUL, {"What a beautiful day!", "Hiii!", "*smiles big*"}, PO_WAVE},
                        {TR_SNOB, {"Is this place... public?", "How quaint.", "*sniffs*"}, PO_IDLE},
                        {TR_PETLOVER, {"Is that a DOG?!", "I miss my cat.", "Puppies!!"}, PO_WAVE},
                        {TR_FARMER, {"Gonna rain soon - good for the crops.", "Fresh eggs this morning!", "Howdy!"}, PO_WAVE},
                        {TR_FANGIRL, {"Did you see the new music video?!", "I'd die for them.", "*screams internally*"}, PO_DANCE},
                        {TR_HOOPS, {"Ball is life.", "Anyone wanna shoot around?", "*dribbles invisible ball*"}, PO_IDLE},
                        {TR_BOXER, {"*shadowboxes*", "Float like a butterfly...", "Gym later?"}, PO_IDLE},
                        {TR_NIGHTOWL, {"Mornings are fake.", "I'm way more fun after midnight.", "*sips energy drink*"}, PO_IDLE},
                        {TR_KIND, {"Need any help?", "You're doing amazing!", "Have a good day!"}, PO_WAVE},
                        {TR_MEAN, {"What are YOU looking at?", "Ew.", "Move."}, PO_IDLE},
                    };
                    for (uint8_t tr : sm.traits) { if (tr == TR_NONE) continue;
                        for (const TL& t : TLS) if (t.tr == tr) { a.say = t.l[irand(3)]; a.sayT = 2.6f; if (t.pose != PO_IDLE) { a.pose = t.pose; a.poseT = 1.8f; a.walking = false; a.tx = a.x; a.ty = a.y; } goto saidIt; } }
                    saidIt:;
                }
            }
        }
        stepWalk(a, sm.stage == ST_TODDLER ? 15.f : 30.f, dt);
        if (!sm.household && a.sayT <= 0 && irand(650) == 0) {
            const Rel* r = findRel(G.mc, a.id);
            static const char* SHADE[] = {"Ew, it's you.", "Nice outfit... NOT.", "Who invited HER?", "Bless your heart.", "Try harder, babe.", "*whispers about you*"};
            if (r && r->fr <= -10) {
                a.say = SHADE[irand(6)]; a.sayT = 2.5f;
                addMoodlet(MC(), "Got Shaded", EM_ANGRY, 1, 90);
            } else { a.say = ambientLine(a.id); a.sayT = 2.4f; }
        }
        i++;
    }
    tweenTick(dt);
    staffFrame(dt, partyMins > 0);
    lxGroupFrame();
    if (partyMins > 0) {
        partyMins -= gameMins;
        if (partyMins <= 0) { toast("The party wound down. Best night ever!"); MC().needs[N_FUN] = 100; }
    }
}

// ===================================================================== build mode
static int buildTab = 0, buildPage = 0, buildColor = 0, selItem = -1;
int itemPrice(int id) {
    const ShapeInfo& s = SHAPES[itemShape(id)];
    int p = 30 + (int)(s.w * s.h / 18.f);
    int c = itemColor(id);
    if (c >= 10) p += 40;
    if (c == 7) p += 25;
    return clampv(p, 25, 480);
}
static Btn shapeB(int i) { return Btn{4 + (i % 2) * 158.f, 62 + (i / 2) * 30.f, 154, 27}; }
static Btn swatchB(int i) { return Btn{4 + i * 26.5f, 32, 24, 24}; }
static const Btn btabs[4] = {{4, 2, 58, 26}, {64, 2, 58, 26}, {124, 2, 58, 26}, {184, 2, 62, 26}};
struct RoomTheme { const char* name; uint8_t wall, wallPat, floor, floorPat, c[3]; };
static const RoomTheme THEMES[16] = {
    {"Y2K", 5, 1, 10, 3, {2, 3, 1}},        {"McBling", 4, 3, 5, 0, {1, 7, 10}},
    {"Soft Girl", 0, 1, 4, 3, {0, 4, 2}},   {"Downtown Girl", 1, 0, 1, 1, {6, 4, 9}},
    {"Coquette", 10, 2, 3, 1, {4, 0, 7}},   {"Grunge", 3, 1, 1, 1, {9, 8, 5}},
    {"Scene Goth", 9, 3, 6, 2, {9, 1, 10}}, {"Baddie", 9, 2, 6, 0, {9, 7, 11}},
    {"Balcony Suite", 0, 4, 3, 4, {0, 7, 4}}, {"Fairy Garden", 1, 5, 3, 4, {4, 7, 6}},
    {"Starry Dream", 0, 6, 3, 4, {7, 5, 0}},  {"Lavender Moon", 5, 7, 4, 5, {2, 7, 0}},
    {"Cozy Loft", 1, 8, 1, 4, {0, 7, 5}},     {"Kawaii", 1, 9, 1, 1, {0, 7, 5}},
    {"Mint Bath", 1, 10, 4, 6, {0, 7, 4}},    {"Sage Living", 1, 11, 1, 1, {6, 5, 7}},
};
static constexpr int NUM_THEMES = 16;
// furnished room themes: walls, floor AND a whole matching furniture set in one tap
struct FurnItem { int sh, col, x, y; };
struct FurnTheme { const char* name; uint8_t wall, wallPat, floor, floorPat, n; FurnItem it[8]; };
static const FurnTheme FTHEMES[] = {
    {"Emo Red Room", 11, 0, 6, 2, 8, {{SH_FAIRYCURTAIN, 7, 110, 74}, {SH_POSTERWALL, 9, 210, 110}, {SH_FIGSHELF, 9, 320, 64}, {SH_GAMERDESK, 9, 80, 214}, {SH_EMOBED, 8, 270, 214}, {SH_LEORUG, 11, 250, 232}, {SH_GUITAR, 8, 365, 214}, {SH_LAVALAMP, 8, 150, 214}}},
    {"Pastel Gamer", 1, 9, 1, 1, 8, {{SH_FAIRYCURTAIN, 0, 100, 70}, {SH_POSTERWALL, 2, 230, 104}, {SH_PLUSHSHELF, 0, 330, 92}, {SH_GAMERDESK, 4, 90, 214}, {SH_GAMERCHAIR, 0, 120, 222}, {SH_GAMERBED, 2, 270, 214}, {SH_CHECKRUG, 2, 220, 234}, {SH_CLOTHESRACK, 4, 370, 214}}},
    {"Y2K Brat Bedroom", 4, 1, 10, 3, 8, {{SH_NEONHEART, 1, 200, 64}, {SH_POSTERWALL, 1, 300, 104}, {SH_HOLLYVANITY, 4, 75, 214}, {SH_BRATBED, 0, 250, 214}, {SH_HEARTRUG, 1, 150, 234}, {SH_LEORUG, 11, 280, 234}, {SH_CLOTHESRACK, 9, 365, 214}, {SH_FLOORMIRROR, 7, 145, 214}}},
    {"Cottagecore", 1, 5, 3, 4, 8, {{SH_FAIRYCURTAIN, 7, 200, 62}, {SH_VINES, 3, 320, 60}, {SH_PLANTSHELF, 5, 80, 82}, {SH_DAISYBED, 0, 220, 214}, {SH_BOOKCUBBY, 4, 70, 214}, {SH_CLOTHESRACK, 5, 350, 214}, {SH_ROUNDRUG, 0, 210, 234}, {SH_MONSTERA, 3, 390, 214}}},
    {"Horror Fan Den", 9, 0, 6, 0, 8, {{SH_POSTERWALL, 9, 90, 100}, {SH_FAIRYCURTAIN, 7, 220, 64}, {SH_FIGSHELF, 9, 330, 62}, {SH_GAMERDESK, 9, 75, 214}, {SH_GAMERCHAIR, 9, 110, 222}, {SH_IRONBED, 9, 260, 214}, {SH_SUNRUG, 7, 220, 236}, {SH_LAVALAMP, 3, 350, 214}}},
    {"NYC Penthouse", 0, 4, 3, 4, 8, {{SH_NEONHEART, 0, 330, 60}, {SH_OVALVANITY, 4, 80, 214}, {SH_FLOORMIRROR, 4, 30, 214}, {SH_RECORDCONSOLE, 0, 160, 214}, {SH_CLOUDBED, 0, 270, 214}, {SH_NIGHTSTAND, 4, 345, 214}, {SH_ROUNDRUG, 4, 250, 234}, {SH_MONSTERA, 3, 385, 214}}},
    {"Glam Lounge", 11, 2, 6, 0, 8, {{SH_NEONHEART, 1, 110, 62}, {SH_BEADCURTAIN, 1, 30, 214}, {SH_HOLLYVANITY, 4, 90, 214}, {SH_PLUSHSOFA, 1, 240, 218}, {SH_LIPSPILLOW, 8, 165, 224}, {SH_LEORUG, 11, 240, 236}, {SH_CRTTV, 9, 360, 214}, {SH_DISCOBALL, 7, 240, 40}}},
    {"2000s Girls Lounge", 5, 1, 1, 1, 8, {{SH_NEONHEART, 1, 330, 60}, {SH_BEADCURTAIN, 2, 40, 214}, {SH_CRTTV, 4, 105, 214}, {SH_LAVALAMP, 1, 150, 214}, {SH_PLUSHSOFA, 0, 260, 218}, {SH_MOONCHAIR, 2, 180, 224}, {SH_FLUFFYRUG, 1, 240, 236}, {SH_DISCOBALL, 7, 220, 36}}},
    {"Cozy Fireplace Living", 1, 0, 1, 4, 7, {{SH_FIREPLACE, 4, 200, 214}, {SH_FLOORLAMP, 7, 80, 214}, {SH_MONSTERA, 3, 40, 214}, {SH_SECTIONAL, 4, 200, 234}, {SH_FLUFFYRUG, 4, 200, 238}, {SH_WINDOWSEAT, 4, 340, 214}, {SH_FLOATSHELF, 5, 340, 90}}},
    {"Kawaii Gamer Living", 1, 9, 1, 1, 8, {{SH_FLOATSHELF, 4, 90, 86}, {SH_PLUSHSHELF, 0, 330, 92}, {SH_TV, 9, 110, 214}, {SH_GAMERDESK, 4, 250, 214}, {SH_GAMERCHAIR, 4, 280, 222}, {SH_PLUSHSOFA, 4, 160, 232}, {SH_CHECKRUG, 2, 190, 238}, {SH_MONSTERA, 3, 385, 214}}},
    {"Farmhouse Kitchen", 1, 11, 1, 1, 8, {{SH_CLOCK, 5, 200, 70}, {SH_FLOATSHELF, 4, 320, 90}, {SH_RETROFRIDGE, 4, 50, 196}, {SH_COUNTER, 4, 130, 196}, {SH_OVEN, 9, 190, 196}, {SH_SINK, 4, 250, 196}, {SH_ISLAND, 4, 250, 232}, {SH_LANTERN, 7, 370, 214}}},
    {"Modern Gray Kitchen", 10, 0, 6, 0, 8, {{SH_FLOATSHELF, 9, 140, 84}, {SH_DRUMLAMP, 9, 250, 40}, {SH_STEELFRIDGE, 9, 45, 196}, {SH_COUNTER, 9, 120, 196}, {SH_OVEN, 9, 180, 196}, {SH_SINK, 9, 240, 196}, {SH_COUNTER, 9, 300, 196}, {SH_ISLAND, 9, 220, 232}}},
};
static constexpr int NUM_FTHEMES = sizeof FTHEMES / sizeof FTHEMES[0];
static int themePage = 0, fthemeSel = -1;
static int fthemePrice(int k) { int p = 0; for (int i = 0; i < FTHEMES[k].n; i++) p += itemPrice(itemId(FTHEMES[k].it[i].sh, FTHEMES[k].it[i].col)); return p * 8 / 10; }   // bundle discount
static int buildShapeAt(int i) {   // build catalog order, skipping empty custom-content slots
    int n = 0;
    for (int sh = 0; sh < SH_COUNT; sh++) { if (sh >= SH_CC0 && sh <= SH_CC_LAST && !ccSlotUsed(sh - SH_CC0)) continue; if (n++ == i) return sh; }
    return SH_COUNT;
}
static int buildShapeCount() { int n = 0; for (int sh = 0; sh < SH_COUNT; sh++) if (!(sh >= SH_CC0 && sh <= SH_CC_LAST && !ccSlotUsed(sh - SH_CC0))) n++; return n; }
#define BUILD_PAGES ((buildShapeCount() + 7) / 8)
static Btn themeB(int i) { return Btn{4 + (i % 2) * 158.f, 30 + (i / 2) * 20.5f, 154, 19}; }
static const Btn bPrev{4, 184, 50, 22}, bNextP{266, 184, 50, 22}, bExit{244, 210, 72, 26}, bSell{4, 210, 70, 26}, bDoneB{82, 210, 70, 26};

static void buildUpdate(const Input& in) {
    RoomState& rs = curRoom();
    if (in.down & KEY_B) { selItem = -1; mode = M_PLAY; return; }
    if (in.touch && bExit.in(in.tp)) { selItem = -1; mode = M_PLAY; return; }
    if (selItem >= (int)rs.items.size()) selItem = -1;
    if (rs.items.size() && (in.down & (KEY_L | KEY_R))) {
        int n = (int)rs.items.size();
        selItem = selItem < 0 ? 0 : (selItem + ((in.down & KEY_R) ? 1 : n - 1)) % n;
    }
    if (selItem >= 0) {
        Placed& p = rs.items[selItem];
        bool wall = SHAPES[itemShape(p.item)].wall;
        float mx = 0, my = 0;
        if (in.held & KEY_LEFT) mx = -1; if (in.held & KEY_RIGHT) mx = 1;
        if (in.held & KEY_UP) my = -1; if (in.held & KEY_DOWN) my = 1;
        if (std::abs(in.cp.dx) > 20) mx = in.cp.dx / 156.f;
        if (std::abs(in.cp.dy) > 20) my = -in.cp.dy / 156.f;
        p.x = (int16_t)clampv(p.x + mx * 140 * in.dt + (mx > 0 ? 0.5f : mx < 0 ? -0.5f : 0), 10.f, 390.f);
        p.y = (int16_t)clampv(p.y + my * 100 * in.dt + (my > 0 ? 0.5f : my < 0 ? -0.5f : 0), wall ? 40.f : 168.f, wall ? 150.f : 238.f);
        if (in.down & KEY_A) { selItem = -1; sfxTap(); }
        else if (in.down & KEY_Y) { p.item = (uint16_t)itemId(itemShape(p.item), (itemColor(p.item) + 1) % NUM_COLORS); sfxTap(); }
        else if ((in.down & KEY_X) || (in.touch && bSell.in(in.tp))) {
            int refund = itemPrice(p.item) * 7 / 10;
            G.money += refund; rs.items.erase(rs.items.begin() + selItem); selItem = -1;
            toast("Sold for $" + std::to_string(refund)); sfxTap();
            return;
        }
        if (in.touch && bDoneB.in(in.tp)) { selItem = -1; return; }
    }
    if (!in.touch) return;
    for (int i = 0; i < 4; i++) if (btabs[i].in(in.tp)) { buildTab = i; sfxTap(); return; }
    if (buildTab == 3) {
        if (Btn{60, 194, 200, 22}.in(in.tp)) { themePage ^= 1; fthemeSel = -1; sfxTap(); return; }
        if (themePage == 1) {
            for (int i = 0; i < NUM_FTHEMES && i < 16; i++) if (themeB(i).in(in.tp)) {
                if (fthemeSel != i) { fthemeSel = i; toast(std::string(FTHEMES[i].name) + ": $" + std::to_string(fthemePrice(i)) + ". Tap again to redo this room! (Old furniture is sold.)"); sfxTap(); return; }
                const FurnTheme& th = FTHEMES[i]; const int price = fthemePrice(i);
                int refund = 0; for (const Placed& pl : rs.items) refund += itemPrice(pl.item) / 2;
                if (G.money + refund < price) { toast("Need $" + std::to_string(price - refund - G.money) + " more for this room."); sfxNo(); return; }
                G.money += refund - price; rs.items.clear(); selItem = -1;
                rs.wall = th.wall; rs.wallPat = th.wallPat; rs.floor = th.floor; rs.floorPat = th.floorPat;
                for (int k = 0; k < th.n; k++) rs.items.push_back({(uint16_t)itemId(th.it[k].sh, th.it[k].col), (int16_t)th.it[k].x, (int16_t)th.it[k].y});
                toast(std::string(th.name) + " is DONE! (Sold old furniture for $" + std::to_string(refund) + ")"); sfxYay(); fxBurst(200, 100, 30); fthemeSel = -1;
                return;
            }
            return;
        }
        for (int i = 0; i < NUM_THEMES; i++)
            if (themeB(i).in(in.tp)) {
                const RoomTheme& th = THEMES[i];
                rs.wall = th.wall; rs.wallPat = th.wallPat; rs.floor = th.floor; rs.floorPat = th.floorPat;
                for (size_t k = 0; k < rs.items.size(); k++) rs.items[k].item = (uint16_t)itemId(itemShape(rs.items[k].item), th.c[k % 3]);
                toast(std::string(th.name) + " makeover done!");
                sfxYay(); fxBurst(200, 100, 25);
                return;
            }
        return;
    }
    if (buildTab == 0) {
        for (int i = 0; i < NUM_COLORS; i++) if (swatchB(i).in(in.tp)) { buildColor = i; if (selItem >= 0) rs.items[selItem].item = (uint16_t)itemId(itemShape(rs.items[selItem].item), i); sfxTap(); return; }
        if (bPrev.in(in.tp)) { buildPage = (buildPage + BUILD_PAGES - 1) % BUILD_PAGES; return; }
        if (bNextP.in(in.tp)) { buildPage = (buildPage + 1) % BUILD_PAGES; return; }
        for (int i = 0; i < 8; i++) {
            int sh = buildShapeAt(buildPage * 8 + i);
            if (sh >= SH_COUNT || !shapeB(i).in(in.tp)) continue;
            int id = itemId(sh, buildColor);
            int price = itemPrice(id);
            if (rs.items.size() >= 40) { toast("This room is full!"); sfxNo(); return; }
            if (G.money < price) { toast("Not enough money! (Need $" + std::to_string(price) + ")"); sfxNo(); return; }
            G.money -= price;
            bool wall = SHAPES[sh].wall;
            rs.items.push_back({(uint16_t)id, (int16_t)200, (int16_t)(wall ? 90 : 214)});
            selItem = (int)rs.items.size() - 1;
            toast("Bought " + itemName(id) + "! Move it with the +Pad, A to place.");
            sfxYay();
            return;
        }
    } else {
        for (int i = 0; i < 12; i++) {
            Btn b{4 + (i % 6) * 52.f, 40 + (i / 6) * 40.f, 48, 34};
            if (b.in(in.tp)) { if (buildTab == 1) rs.wall = (uint8_t)i; else rs.floor = (uint8_t)i; sfxTap(); return; }
        }
        for (int i = 0; i < (buildTab == 1 ? NUM_WALLPATS : NUM_FLOORPATS); i++) {
            Btn b{4 + (i % 4) * 78.f, 118 + (i / 4) * 25.f, 74, 23};
            if (b.in(in.tp)) { if (buildTab == 1) rs.wallPat = (uint8_t)i; else rs.floorPat = (uint8_t)i; sfxTap(); return; }
        }
    }
}

static void buildDrawBottom(float t) {
    bottomBackdrop(t);
    static const char* tn[4] = {"Buy", "Walls", "Floors", "Themes"};
    for (int i = 0; i < 4; i++) button(btabs[i], tn[i], buildTab == i, 1, 0.42f);
    char b[48];
    snprintf(b, sizeof b, "$%d", (int)G.money); text(b, 314, 6, 0.5f, C(GOLD), 2);
    if (buildTab == 3 && themePage == 1) {
        for (int i = 0; i < NUM_FTHEMES && i < 16; i++) {
            Btn r = themeB(i); button(r, "", fthemeSel == i, fthemeSel == i ? 2 : 0, 0.4f);
            const FurnTheme& th = FTHEMES[i];
            for (int k = 0; k < 3; k++) { const int ci = th.it[k + 3].col; if (ci == 10) zebraRect(r.x + 6 + k * 13, r.y + 4, 11, 11, C(CREAM), C(BLACK), k); else if (ci == 11) leopardRect(r.x + 6 + k * 13, r.y + 4, 11, 11, k); else rect(r.x + 6 + k * 13, r.y + 4, 11, 11, C(PALETTE[ci])); }
            text(th.name, r.x + 48, r.y + 3, 0.32f, C(CREAM));
        }
        button(Btn{60, 194, 200, 22}, "< Recolor Themes", false, 3, 0.36f);
    } else if (buildTab == 3) {
        for (int i = 0; i < NUM_THEMES; i++) {
            Btn r = themeB(i);
            button(r, "", i >= 8, i >= 8 ? 0 : 1, 0.42f);
            for (int k = 0; k < 3; k++) {
                int ci = THEMES[i].c[k];
                if (ci == 10) zebraRect(r.x + 6 + k * 13, r.y + 4, 11, 11, C(CREAM), C(BLACK), k);
                else if (ci == 11) leopardRect(r.x + 6 + k * 13, r.y + 4, 11, 11, k);
                else rect(r.x + 6 + k * 13, r.y + 4, 11, 11, C(PALETTE[ci]));
            }
            text(THEMES[i].name, r.x + 48, r.y + 3, 0.38f, C(CREAM));
        }
        button(Btn{60, 194, 200, 22}, "Furnished Rooms >", false, 2, 0.36f);
    } else if (buildTab == 0) {
        for (int i = 0; i < NUM_COLORS; i++) {
            Btn s = swatchB(i);
            rrect(s.x - 1, s.y - 1, s.w + 2, s.h + 2, 5, C(buildColor == i ? GOLD : UI1));
            if (i == 10) zebraRect(s.x + 2, s.y + 2, s.w - 4, s.h - 4, C(CREAM), C(BLACK), 2);
            else if (i == 11) leopardRect(s.x + 2, s.y + 2, s.w - 4, s.h - 4, 3);
            else rrect(s.x + 2, s.y + 2, s.w - 4, s.h - 4, 4, C(PALETTE[i]));
        }
        for (int i = 0; i < 8; i++) {
            int sh = buildShapeAt(buildPage * 8 + i);
            if (sh >= SH_COUNT) break;
            Btn r = shapeB(i);
            button(r, "", false, 1, 0.4f);
            int id = itemId(sh, buildColor);
            text(SHAPES[sh].name, r.x + 6, r.y + 6, 0.4f, C(CREAM));
            snprintf(b, sizeof b, "$%d", itemPrice(id)); text(b, r.x + r.w - 6, r.y + 6, 0.4f, C(GOLD), 2);
        }
        button(bPrev, "<", false, 1, 0.45f); button(bNextP, ">", false, 1, 0.45f);
        snprintf(b, sizeof b, "Page %d/%d - %s", buildPage + 1, BUILD_PAGES, COLOR_NAMES[buildColor]); text(b, 160, 186, 0.4f, C(CHAMP), 1);
    } else {
        const RGB* pal = buildTab == 1 ? WALL_COLORS : FLOOR_COLORS;
        int curSel = buildTab == 1 ? curRoom().wall : curRoom().floor;
        int curPat = buildTab == 1 ? curRoom().wallPat : curRoom().floorPat;
        for (int i = 0; i < 12; i++) {
            Btn r{4 + (i % 6) * 52.f, 40 + (i / 6) * 40.f, 48, 34};
            rrect(r.x - 1, r.y - 1, r.w + 2, r.h + 2, 6, C(curSel == i ? GOLD : UI1));
            rrect(r.x + 2, r.y + 2, r.w - 4, r.h - 4, 5, C(pal[i]));
        }
        for (int i = 0; i < (buildTab == 1 ? NUM_WALLPATS : NUM_FLOORPATS); i++) button(Btn{4 + (i % 4) * 78.f, 118 + (i / 4) * 25.f, 74, 23}, buildTab == 1 ? WALL_PATTERNS[i] : FLOOR_PATTERNS[i], curPat == i, i >= 4 ? 3 : 0, 0.36f);
        text(buildTab == 1 ? "The purple ones are full room makeovers!" : "Free to change!", 160, 194, 0.36f, C(CHAMP), 1);
    }
    if (selItem >= 0) { button(bSell, "Sell (X)", false, 1, 0.4f); button(bDoneB, "Place (A)", false, 2, 0.4f); }
    else text("L/R: select a placed item", 80, 216, 0.36f, C(CREAM), 1);
    button(bExit, "Exit (B)", false, 1, 0.45f);
}

// ===================================================================== play screen
static const Btn bPhone{4, 210, 76, 26}, bBuild{82, 210, 76, 26}, bMap{160, 210, 76, 26}, bMenu{238, 210, 78, 26};
static const Btn bCancel{238, 86, 78, 22}, bVenue{6, 86, 140, 22};
static Btn nearB(int i) { return Btn{6, 112 + i * 24.f, 308, 22}; }
static Btn queueB(int i) { return Btn{6 + i * 78.f, 86, 74, 22}; }
// Pie menu: up to 6 slices in a ring around a center Back button (flick the Circle Pad to pick, A to choose)
// (long info lines fall back to the 2-column list so nothing overlaps)
static bool pieMode = false;
static Btn optB(int i) {
    if (!pieMode) return Btn{6 + (i % 2) * 156.f, 104 + (i / 2) * 26.f, 152, 23};
    const float a = -1.5708f + i * 1.0472f; return Btn{160 + std::cos(a) * 96 - 52, 160 + std::sin(a) * 48 - 11, 104, 22};
}
static Btn optBackB() { return pieMode ? Btn{128, 150, 64, 20} : Btn{4, 182, 70, 24}; }
static Btn optMoreB() { return pieMode ? Btn{258, 216, 60, 22} : Btn{246, 182, 70, 24}; }
#define bOptBack optBackB()
#define bOptMore optMoreB()
static int pieSel = -1;
static float pieOpenT = 1;   // pie slices fly out from the center with an overshoot when the menu opens
static Btn optBDraw(int i) {
    Btn b = optB(i); if (!pieMode || pieOpenT >= 0.3f) return b;
    const float k = easeOutBack(std::min(1.f, pieOpenT / 0.3f)); const float cx = 160 - b.w / 2, cy = 160 - b.h / 2;
    b.x = cx + (b.x - cx) * k; b.y = cy + (b.y - cy) * k; return b;
}


// ===================================================================== action queue
// Pick an action -> it joins the queue -> your sim walks over -> sits/lies/stands -> does it.
struct QAct { Opt opt; int kind = 0; int item = -1; int akind = 0, aid = -1; int seat = -1, slot = 0; int room = -1; };
static std::deque<QAct> gq;
static int qPhase = 0;

static bool needsWalk(const Opt& o) {
    if (o.id >= 1000 && o.id < 1900) return true;
    if (o.id >= 2100 && o.id < 2200) return true;
    if (o.id >= 2000 && o.id < 2100) {
        int a = o.id - 2000;
        return !(a == IA_OUTFIT || a == IA_MUSIC || a == IA_ARCADE || a == IA_GETUP);
    }
    return false;
}
static int resolveActor(int akind, int aid) {
    auto& v = curRoom().actors;
    for (size_t i = 0; i < v.size(); i++) if (v[i].kind == akind && v[i].id == aid) return (int)i;
    return -1;
}
static void clearQueue() { gq.clear(); qPhase = 0; }

static void chooseOption(const Opt& o) {
    if (menuT.kind == 3) {   // walk over to the object, then use it
        int ns; const Spot* sp = spotsIn(G.room, ns);
        Actor* ma = mcActor();
        if (ma && menuT.idx < ns && o.id >= 0) { if (ma->seat >= 0) standUp(*ma, curRoom()); walkTo(*ma, sp[menuT.idx].x + (ma->x < sp[menuT.idx].x ? -18 : 18), std::fmax(200.f, (float)sp[menuT.idx].y + 8)); }
    }
    if (needsWalk(o) && (menuT.kind == 0 || menuT.kind == 1)) {
        if (gq.size() >= 5) { toast("Your action queue is full!"); sfxNo(); return; }
        QAct q;
        q.opt = o; q.kind = menuT.kind;
        if (menuT.kind == 0) q.item = menuT.idx;
        else { const Actor& a = curRoom().actors[menuT.idx]; q.akind = a.kind; q.aid = a.id; }
        q.room = G.room;
        gq.push_back(q);
        menuOpen = false;
        sfxTap();
        return;
    }
    doOption(o);
}

static bool approachFor(QAct& q, Actor& me, float& x, float& y, std::string& why) {
    RoomState& rs = curRoom();
    q.seat = -1; q.slot = 0;
    if (q.kind == 0) {
        if (q.item < 0 || q.item >= (int)rs.items.size()) { why = "That's not there anymore."; return false; }
        const Placed& pl = rs.items[q.item];
        int sh = itemShape(pl.item), act = q.opt.id - 2000;
        UseSpot u = useSpot(sh);
        if (sh == SH_TV && act == IA_TV) {   // find somewhere to sit and watch
            int best = -1, bslot = 0; float bd = 1e9f;
            for (size_t k = 0; k < rs.items.size(); k++) {
                int s2 = itemShape(rs.items[k].item);
                if (s2 != SH_SOFA && s2 != SH_CHAIR && s2 != SH_BEANBAG) continue;
                int fs = me.seat == (int)k ? me.seatSlot : freeSlot(rs, (int)k, &me);
                if (fs < 0) continue;
                float d = std::fabs(rs.items[k].x - pl.x);
                if (d < bd) { bd = d; best = (int)k; bslot = fs; }
            }
            if (best >= 0) { q.seat = best; q.slot = bslot; }
        } else if (u.kind > 0) {
            if (me.seat == q.item) { q.seat = q.item; q.slot = me.seatSlot; }
            else {
                int s = freeSlot(rs, q.item, &me);
                if (s < 0) { why = baseShape(sh) == SH_BED ? "The bed is already taken." : sh == SH_TOILET || sh == SH_SHOWER || sh == SH_BATHTUB ? "Someone's in there!" : "That seat is taken!"; return false; }
                q.seat = q.item; q.slot = s;
            }
        }
        if (q.seat >= 0) {
            const Placed& sp = rs.items[q.seat];
            UseSpot su = useSpot(itemShape(sp.item));
            x = sp.x + su.slotDX[q.slot % 2]; y = sp.y + 6;
        } else if (SHAPES[sh].wall) { x = pl.x; y = 178; }
        else {
            float side = me.x < pl.x ? -1.f : 1.f;
            x = pl.x + side * (SHAPES[sh].w * 0.5f + 12);
            if (x < 16 || x > 384) x = pl.x - side * (SHAPES[sh].w * 0.5f + 12);
            y = pl.y + 2;
        }
        return true;
    }
    if (q.kind == 1) {
        int ai = resolveActor(q.akind, q.aid);
        if (ai < 0) { why = "They left."; return false; }
        const Actor& a = rs.actors[ai];
        float side = me.x < a.x ? -1.f : 1.f;
        x = a.x + side * 34; if (x < 16 || x > 384) x = a.x - side * 34;
        y = a.y;
        return true;
    }
    x = me.x; y = me.y;
    return true;
}

static void processQueue(Actor* ma) {
    if (!ma || gq.empty()) { qPhase = 0; return; }
    RoomState& rs = curRoom();
    QAct& q = gq.front();
    if (q.room != G.room) { gq.clear(); qPhase = 0; return; }   // we moved: old plans don't apply
    if (qPhase == 0) {
        float x, y; std::string why;
        if (!approachFor(q, *ma, x, y, why)) { toast(why); sfxNo(); gq.pop_front(); return; }
        bool there = q.seat >= 0 && ma->seat == q.seat;
        if (!there) {
            if (ma->seat >= 0) { cancelActivity(); standUp(*ma, rs); }
            float dx = x - ma->x, dy = y - ma->y;
            if (dx * dx + dy * dy > 9) { walkTo(*ma, x, y); qPhase = 1; return; }
        }
        qPhase = 2;
    }
    if (qPhase == 1) { if (ma->walking) return; qPhase = 2; }
    if (qPhase == 2) {
        if (q.seat >= (int)rs.items.size()) { gq.pop_front(); qPhase = 0; return; }
        if (q.seat >= 0 && ma->seat != q.seat) {
            if (seatTaken(rs, q.seat, q.slot, ma)) { toast("Someone took that spot!"); sfxNo(); gq.pop_front(); qPhase = 0; return; }
            sitDown(*ma, rs, q.seat, q.slot);
        }
        if (q.kind == 1) {
            int ai = resolveActor(q.akind, q.aid);
            if (ai < 0) { toast("They left."); gq.pop_front(); qPhase = 0; return; }
            menuT = Target{1, ai, 0};
            if (ma->seat < 0) ma->facingLeft = rs.actors[ai].x < ma->x;
        } else {
            menuT = Target{0, q.item, 0};
            if (ma->seat < 0 && q.item < (int)rs.items.size()) ma->facingLeft = rs.items[q.item].x < ma->x;
        }
        qPhase = 3;
        Opt o = q.opt;
        doOption(o);
        return;
    }
    if (qPhase == 3) {
        if (ACT.active()) return;
        if (ma->seat >= 0 && ma->seat < (int)rs.items.size()) {
            int sh = itemShape(rs.items[ma->seat].item);
            if (ma->useKind == 2 || ma->useKind == 3 || sh == SH_TOILET) standUp(*ma, rs);
        }
        if (!gq.empty()) gq.pop_front();
        qPhase = 0;
    }
}

static void cancelQueueItem(int i, Actor* ma) {
    if (i < 0 || i >= (int)gq.size()) return;
    if (i == 0 && qPhase >= 1) {
        cancelActivity();
        if (ma) {
            ma->walking = false;
            if (ma->useKind == 2 || ma->useKind == 3) standUp(*ma, curRoom());
        }
        qPhase = 0;
    }
    gq.erase(gq.begin() + i);
    sfxTap();
}

bool menuStillOpen() { return menuOpen; }
// ---- Free Will for your own Sim (Life Extras toggle): after a few idle seconds, they look after
// their lowest need using furniture in the room, through the same walk-then-use queue as a tap.
static float mcIdle = 0, autoTownT = 0;
static std::string first(int i) { const std::string& n = G.sims[i].name; return n.substr(0, n.find(' ')); }
static void autoMC(const Input& in) {
    if (in.down || in.held || in.touch) { mcIdle = 0; return; }
    const int lvl = l3MCAuto();   // 0 off (critical needs only), 1 medium, 2 full
    if (menuOpen || ACT.active() || !gq.empty() || G.date.active || fadeT > 0) { mcIdle = 0; return; }
    Actor* ma = mcActor(); if (!ma || ma->walking) return;
    mcIdle += in.dt;
    if (mcIdle < (lvl == 2 ? 6.f : lvl == 1 ? 10.f : 4.f)) return;
    mcIdle = 0;
    const Sim& me = MC(); RoomState& rs = curRoom();
    const int hour = (int)(G.minute / 60); const bool night = hour >= 22 || hour < 6;
    const float th = lvl == 2 ? 70.f : lvl == 1 ? 40.f : 10.f;   // off: only a critical need forces an action
    int need = -1; float lo = th;
    for (int n = 0; n < N_COUNT; n++) if (me.needs[n] < lo) { lo = me.needs[n]; need = n; }
    if (lvl == 2 && night && me.needs[N_ENERGY] < 80) need = N_ENERGY;
    if (lvl == 2 && !me.isLittle()) {   // Full autonomy leaves the house: work, school, town, then home again
        const bool home = roomIsHome(G.room);
        if (me.career && atWorkTime(me)) {
            if (G.room != CAREERS[me.career].room) { if (ma->seat >= 0) standUp(*ma, rs); toast(first(G.mc) + " is heading to work!"); travelTo(CAREERS[me.career].room); mcIdle = 4.f; return; }
            doOption(Opt{"", 3000 + VA_WORKAUTO}); return;
        }
        if (me.stage >= ST_CHILD && me.stage <= ST_TEEN && isWeekday(G.day) && !me.schoolToday && G.minute >= 7 * 60 && G.minute < 10 * 60) {
            if (G.room != R_SCHOOL) { if (ma->seat >= 0) standUp(*ma, rs); toast(first(G.mc) + " is off to school!"); travelTo(R_SCHOOL); mcIdle = 4.f; return; }
            doOption(Opt{"", 3000 + VA_SCHOOL}); return;
        }
        if (!home && G.room != R_NEIGHBOR) {   // out in town: head home when a need gets low or it's late
            autoTownT += 7.f;
            if (need >= 0 && need != N_SOCIAL && need != N_FUN) { toast(first(G.mc) + " is heading home."); travelTo(R_LIVING); return; }
            if (hour >= 21 || hour < 6 || autoTownT > 90.f) { toast(first(G.mc) + " is heading home."); travelTo(R_LIVING); return; }
        } else if (home && need < 0 && hour >= 10 && hour < 19 && irand(6) == 0) {   // bored at home: go out
            static const int FUN[6] = {R_PARK, R_MALL, R_CAFE, R_BEACH, R_ARCADE, R_BAKERY};
            for (int t = 0; t < 6; t++) { const int r = FUN[irand(6)]; std::string why; if (!lvlRoomOk(r, &why)) continue;
                if (me.stage < ST_TEEN && r != R_PARK && r != R_BAKERY) continue;
                if (ma->seat >= 0) standUp(*ma, rs); autoTownT = 0; toast(first(G.mc) + " is bored - off to the " + ROOM_NAMES[r] + "!"); travelTo(r); return; }
        }
        if (!home) {   // in town: mingle or wander
            std::vector<int> who; for (size_t i = 0; i < rs.actors.size(); i++) { const Actor& a = rs.actors[i]; if (a.kind == 0 && a.id != G.mc && a.id < G.sims.size() && !a.passer) who.push_back((int)i); }
            if (!who.empty() && irand(2)) { menuT = Target{1, who[irand((int)who.size())], 0}; menuRoom = G.room; chooseOption(Opt{"Chat", 1000 + SO_CHAT}); }
            else walkTo(*ma, 40 + frand() * 320, 186 + frand() * 48);
            return;
        }
    }
    if (need < 0 && lvl < 2) return;   // only Full picks random fun / chats / wanders when nothing is needed
    if (need == N_SOCIAL || (need < 0 && irand(3) == 0)) {   // chat with someone here
        std::vector<int> who;
        for (size_t i = 0; i < rs.actors.size(); i++) { const Actor& a = rs.actors[i]; if (a.kind == 0 && a.id != G.mc && a.id < G.sims.size() && !a.passer) who.push_back((int)i); }
        if (!who.empty()) { menuT = Target{1, who[irand((int)who.size())], 0}; menuRoom = G.room; chooseOption(Opt{"Chat", 1000 + SO_CHAT}); return; }
        if (need == N_SOCIAL) need = N_FUN;
    }
    std::vector<int> want;
    switch (need) {
    case N_HUNGER: want = {IA_QUICKMEAL, IA_SNACK, IA_COOK}; break;
    case N_ENERGY: if (night || lo < 25) want = {IA_SLEEP}; want.push_back(IA_NAP); want.push_back(IA_RELAXBED); break;
    case N_HYGIENE: want = {IA_SHOWER, IA_BATH}; break;
    case N_BLADDER: want = {IA_TOILET}; break;
    default: want = {IA_TV, IA_ARCADE, IA_MUSIC, IA_DANCE, IA_READ, IA_TOYS}; break;
    }
    for (int w : want) {
        std::vector<int> items;
        for (size_t k = 0; k < rs.items.size(); k++)
            for (const Opt& o : itemOptions(itemShape(rs.items[k].item), (int)k)) if (o.id == 2000 + w) { items.push_back((int)k); break; }
        if (items.empty()) continue;
        const int k = items[irand((int)items.size())];
        menuT = Target{0, k, 0}; menuRoom = G.room;
        chooseOption(Opt{"", 2000 + w});
        return;
    }
    if ((need >= 0 || lvl == 2) && roomIsHome(G.room)) {   // nothing for it here: head to the home room that has it
        for (int w : want) for (int r = 0; r < R_COUNT; r++) {
            if (r == G.room || !roomIsHome(r)) continue;
            for (const Placed& pl : G.rooms[G.house][r].items)
                for (const Opt& o : itemOptions(itemShape(pl.item), -1)) if (o.id == 2000 + w) { if (ma->seat >= 0) standUp(*ma, rs); travelTo(r); mcIdle = 5.f; return; }
        }
    }
    if (need < 0 && !ma->walking && ma->seat < 0) walkTo(*ma, 40 + frand() * 320, 186 + frand() * 48);   // just wander
}

static void playUpdate(const Input& in) {
    if (!menuOpen) autoMC(in);
    if (gsWantGuide() && !menuOpen) { LxMenu gm = gsGuide(); opts.clear(); opts.push_back({"WELCOME! How to Play:", 23999}); for (auto& p : gm) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
    if (choiceWanted() && !menuOpen) {   // a drama choice pops up: pick how to react
        LxMenu cm = choiceMenu();
        if (!cm.empty()) { opts.clear(); for (auto& p : cm) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; sfxTap(); }
    }
    if (menuOpen && menuRoom != G.room && menuT.kind != 2) menuOpen = false;   // never act on another room's people
    Sim& me = MC();
    Actor* ma = mcActor();
    float gameMins = in.dt * timeScale() * speedMul;
    worldTick(in.dt * speedMul);
    startDateIfDue();
    updateActors(in.dt, gameMins);
    ma = mcActor();
    if (fadeT > 0) {
        fadeT -= in.dt;
        if (fadeT <= 0) {
            dlgSay(-1, "Some time passes... You two cuddle and chat until you both drift off.");
            if (pendingMagicNews) { dlgSay(-1, "Something feels a little different... A test from the pharmacy might say more in a few days!"); pendingMagicNews = false; }
        }
        return;
    }
    if (dlgActive()) {
        if ((in.down & (KEY_A | KEY_B)) || in.touch) { dlgAdvance(); sfxTap(); }
        return;
    }
    if (momentActive()) { momentUpdate(in); return; }
    processQueue(ma);

    if (menuOpen) {
        pieMode = false;   // pie menu retired: the 2-column list is easier to read and tap
        if (in.down & KEY_B) { menuOpen = false; return; }
        if (in.touch) {
            if (bOptBack.in(in.tp)) { menuOpen = false; return; }
            if (bOptMore.in(in.tp) && opts.size() > 6) { optPage = (optPage + 1) % ((int)(opts.size() + 5) / 6); return; }
            for (int i = 0; i < 6; i++) {
                int k = optPage * 6 + i;
                if (k < (int)opts.size() && optB(i).in(in.tp)) { Opt o = opts[k]; chooseOption(o); return; }
            }
        }
        if (std::abs(in.cp.dx) > 60 || std::abs(in.cp.dy) > 60) {
            float a = std::atan2(-(float)in.cp.dy, (float)in.cp.dx) + 1.5708f; if (a < 0) a += 6.2832f;
            int s = (int)((a + 0.5236f) / 1.0472f) % 6; if (optPage * 6 + s < (int)opts.size()) pieSel = s;
        }
        if (in.down & KEY_R && opts.size() > 6) { optPage = (optPage + 1) % ((int)(opts.size() + 5) / 6); pieSel = -1; return; }
        if (in.down & KEY_A) { if (!opts.empty()) { int k = optPage * 6 + std::max(0, pieSel); if (k >= (int)opts.size()) k = optPage * 6; Opt o = opts[k]; pieSel = -1; chooseOption(o); } return; }
        return;
    }
    if (in.down & KEY_X) { phoneOpen(); mode = M_PHONE; sfxTap(); return; }
    if (in.down & KEY_Y) {
        if (roomIsHome(G.room)) { clearQueue(); cancelActivity(); for (Actor& a : curRoom().actors) { standUp(a, curRoom()); a.goSeat = -1; a.walking = false; } mode = M_BUILD; selItem = -1; sfxTap(); }
        else { toast("You can only build at home!"); sfxNo(); }
        return;
    }
    if (in.down & KEY_START) { mode = M_MENU; menuPage = 0; sfxTap(); return; }
    if (in.down & KEY_SELECT) { speedMul = speedMul == 1 ? 3 : 1; toast(speedMul == 1 ? "Speed: 1x" : "Speed: 3x >>"); }
    if (in.down & (KEY_L | KEY_R)) {
        if (roomIsHome(G.room)) {
            // which rooms this home has: apartment, house, or two-story home
            static const int APT[] = {R_LIVING, R_KITCHEN, R_BEDROOM, R_BATHROOM, R_CLOSET, R_NURSERY};
            static const int HOUSE[] = {R_LIVING, R_KITCHEN, R_BEDROOM, R_CLOSET, R_NURSERY, R_BASEMENT, R_YARD};
            static const int TWO[] = {R_LIVING, R_KITCHEN, R_BEDROOM, R_BEDROOM2, R_TEENROOM, R_BATHROOM, R_CLOSET, R_NURSERY, R_STUDY, R_BASEMENT, R_YARD};
            int ht = G.houseType[G.house % 2]; const int* L = ht == 0 ? APT : ht == 2 ? TWO : HOUSE; int n = ht == 0 ? 6 : ht == 2 ? 11 : 7;
            int cur = 0; for (int i = 0; i < n; i++) if (L[i] == G.room) cur = i;
            int r = L[(cur + ((in.down & KEY_R) ? 1 : n - 1)) % n];
            furnishExtraRooms(G.rooms[G.house]);
            clearQueue();
            travelTo(r);
            ma = mcActor();
        } else {
            int nr = districtNext(G.room, (in.down & KEY_R) ? 1 : -1);
            if (nr >= 0) { clearQueue(); travelTo(nr); ma = mcActor(); }
            else toast("Use the Map (X > Map) to travel!");
        }
    }
    // movement
    float mx = 0, my = 0;
    if (in.held & KEY_DLEFT) mx = -1; if (in.held & KEY_DRIGHT) mx = 1;
    if (in.held & KEY_DUP) my = -1; if (in.held & KEY_DDOWN) my = 1;
    if (std::abs(in.cp.dx) > 18) mx = in.cp.dx / 156.f;
    if (std::abs(in.cp.dy) > 18) my = -in.cp.dy / 156.f;
    if ((mx || my) && ma) {
        if (!gq.empty()) clearQueue();
        if (ACT.active()) cancelActivity();
        ma = mcActor(); if (!ma) return;   // the actor list may have changed (someone joined or left)
        if (ma->seat >= 0) standUp(*ma, curRoom());
        ma = mcActor(); if (!ma) return;
        ma->walking = false;
        float sp = me.stage <= ST_TODDLER ? 40 : 90;
        ma->x = clampv(ma->x + mx * sp * in.dt, 16.f, 384.f);
        ma->y = clampv(ma->y + my * 60 * in.dt, 168.f, 238.f);
        if (mx) ma->facingLeft = mx < 0;
        ma->walkPhase = std::fmod(ma->walkPhase + in.dt * 2.3f, 1.f);
        ma->poseT = 0.12f; ma->pose = PO_WALK;
    }
    if (in.down & KEY_A) {
        auto nb = nearby();
        if (!nb.empty() && nb[0].d < 120) openMenu(nb[0]);
        else if (venueHere()) openMenu(Target{2, -1, 0});
        else toast("Walk closer to something!");
        return;
    }
    if (!in.touch) return;
    const touchPosition& tp = in.tp;
    if (bPhone.in(tp)) { phoneOpen(); mode = M_PHONE; sfxTap(); return; }
    if (bBuild.in(tp)) { if (roomIsHome(G.room)) { mode = M_BUILD; selItem = -1; } else toast("You can only build at home!"); return; }
    if (bMap.in(tp)) { phoneOpen(AP_MAP); mode = M_PHONE; sfxTap(); return; }
    if (bMenu.in(tp)) { mode = M_MENU; menuPage = 0; sfxTap(); return; }
    if (!gq.empty()) { for (int i = 0; i < 4 && i < (int)gq.size(); i++) if (queueB(i).in(tp)) { cancelQueueItem(i, ma); return; } }
    else if (ACT.active() && bCancel.in(tp)) { cancelActivity(); if (ma && (ma->useKind == 2 || ma->useKind == 3)) standUp(*ma, curRoom()); return; }
    else if (ma && ma->seat >= 0 && queueB(0).in(tp)) { standUp(*ma, curRoom()); sfxTap(); return; }
    if (venueHere() && bVenue.in(tp)) { openMenu(Target{2, -1, 0}); return; }
    auto nb = nearby();
    for (int i = 0; i < 4 && i < (int)nb.size(); i++)
        if (nearB(i).in(tp)) {
            openMenu(nb[i]);
            sfxTap();
            return;
        }
}

// Flash-style need meters: a coloured icon badge + chunky outlined bar + percentage
static const RGB NEED_COLS[N_COUNT] = {{255, 140, 40}, {255, 210, 40}, {90, 186, 245}, {96, 206, 96}, {170, 104, 230}, {54, 204, 196}};
void needIcon(int n, float cx, float cy, float r) {
    circle(cx, cy, r + 1.4f, C(UI_INK));
    circle(cx, cy, r, C(NEED_COLS[n]));
    const u32 W = C(255, 255, 255), K = C(UI_INK);
    switch (n) {
    case N_HUNGER:   // burger
        ellipse(cx, cy - r * 0.2f, r * 1.3f, r * 0.8f, C(250, 200, 110)); rect(cx - r * 0.65f, cy - r * 0.15f, r * 1.3f, r * 0.25f, C(110, 60, 40));
        rect(cx - r * 0.6f, cy + r * 0.12f, r * 1.2f, r * 0.28f, C(250, 200, 110)); break;
    case N_ENERGY: tri(cx + r * 0.2f, cy - r * 0.8f, cx - r * 0.45f, cy + r * 0.12f, cx + r * 0.05f, cy + r * 0.12f, W); tri(cx - r * 0.2f, cy + r * 0.8f, cx + r * 0.45f, cy - r * 0.12f, cx - r * 0.05f, cy - r * 0.12f, W); break;
    case N_HYGIENE: circle(cx - r * 0.25f, cy + r * 0.1f, r * 0.45f, W); circle(cx + r * 0.35f, cy - r * 0.3f, r * 0.28f, W); circle(cx - r * 0.25f, cy + r * 0.1f, r * 0.28f, C(NEED_COLS[n])); break;
    case N_FUN: circle(cx - r * 0.3f, cy - r * 0.2f, r * 0.13f, K); circle(cx + r * 0.3f, cy - r * 0.2f, r * 0.13f, K); line(cx - r * 0.4f, cy + r * 0.2f, cx, cy + r * 0.45f, r * 0.18f, K); line(cx, cy + r * 0.45f, cx + r * 0.4f, cy + r * 0.2f, r * 0.18f, K); break;
    case N_SOCIAL: circle(cx - r * 0.3f, cy - r * 0.25f, r * 0.26f, W); circle(cx + r * 0.3f, cy - r * 0.25f, r * 0.26f, W); ellipse(cx - r * 0.3f, cy + r * 0.35f, r * 0.75f, r * 0.55f, W); ellipse(cx + r * 0.3f, cy + r * 0.35f, r * 0.75f, r * 0.55f, W); break;
    default: circle(cx, cy + r * 0.2f, r * 0.42f, W); tri(cx - r * 0.38f, cy + r * 0.05f, cx + r * 0.38f, cy + r * 0.05f, cx, cy - r * 0.6f, W); break;   // droplet
    }
}
static void needBar(float x, float y, int n, float v) {
    v = clampv(v, 0.f, 100.f);
    const float bw = 108;
    const bool low = v < 25;
    rrect(x + 10, y, bw + 3, 12, 5, C(UI_INK));
    rrect(x + 11.5f, y + 1.5f, bw, 9, 4, C(20, 60, 74));
    if (v > 1) rrect(x + 11.5f, y + 1.5f, bw * v / 100.f, 9, 4, C(low && std::fmod(T, 0.8f) < 0.4f ? RGB{255, 70, 90} : NEED_COLS[n]));
    if (v > 6) rrect(x + 13.f, y + 2.5f, (bw - 3) * v / 100.f, 2.5f, 1.2f, C(255, 255, 255, 90));
    needIcon(n, x + 6, y + 6, 6.5f);
    char b[24]; snprintf(b, sizeof b, "%s %d%%", NEED_NAMES[n], (int)v);
    textOutline(b, x + 18, y - 0.5f, 0.32f, C(255, 255, 255), C(UI_INK), 0, 0.8f);
}

static void playDrawBottom(float t) {
    bottomBackdrop(t);
    Sim& me = MC();
    char b[96];
    // top bar: clock pill + money pill (Flash style)
    rect(0, 0, BOT_W, 21, C(255, 46, 140));
    rect(0, 21, BOT_W, 2, C(UI_INK));
    rect(0, 0, BOT_W, 6, C(255, 255, 255, 50));
    snprintf(b, sizeof b, "%s  %s", clockStr(G.minute).c_str(), dateStr(G.day).c_str());
    textOutline(b, 6, 2, 0.4f, C(255, 255, 255), C(UI_INK), 0, 0.9f);
    drawWeatherIcon(textW(b, 0.4f) + 18, 11);
    snprintf(b, sizeof b, "$%d", (int)G.money);
    float mw = textW(b, 0.44f) + 12;
    flashPanel(314 - mw, 2.5f, mw, 16, RGB{40, 192, 210}, 6);
    textOutline(b, 314 - mw / 2, 2.5f, 0.44f, C(140, 255, 120), C(UI_INK), 1, 0.9f);
    if (speedMul > 1) textOutline(">>", 300 - mw - 6, 2, 0.45f, C(255, 230, 80), C(UI_INK), 2);
    // needs + mood
    for (int n = 0; n < N_COUNT; n++) needBar(4 + (n % 2) * 160.f, 26 + (n / 2) * 15.5f, n, me.needs[n]);
    int eStr = 0; Emotion e = emotionOf(me, eStr);
    {   // mood band: the backdrop takes the current emotion's color, "Very ..." when it's strong
        std::string em = EMOTION_NAMES[e]; if (eStr >= 4 && e != EM_FINE) { for (auto& ch : em) ch = (char)toupper(ch); em = "VERY " + em; }
        snprintf(b, sizeof b, "%s - %s", me.name.c_str(), em.c_str());
        float w = textW(b, 0.38f) + 24, gx = 160 - w / 2, gy = 70;
        rrect(gx - 1, gy - 1, w + 2, 15, 6, C(UI_INK)); rrect(gx, gy, w, 13, 5, C(EMOTION_COLORS[e]));
        rrect(gx + 2, gy + 1, w - 4, 4, 3, C(255, 255, 255, 70));
        textOutline(b, 160, gy - 0.5f, 0.38f, C(255, 255, 255), C(UI_INK), 1, 0.8f);
    }
    { static bool was = false; if (menuOpen && !was) pieOpenT = 0; was = menuOpen; pieOpenT += 1.f / 60.f; }
    if (menuOpen) {
        rect(0, 84, BOT_W, 1, C(GOLD));
        std::string title = menuT.kind == 2 ? ROOM_NAMES[G.room] : targetName(menuT);
        text(title.c_str(), 160, 86, 0.45f, C(GOLD), 1);
        for (int i = 0; i < 6; i++) {
            int k = optPage * 6 + i;
            if (k >= (int)opts.size()) break;
            if (i == pieSel) { Btn h = optBDraw(i); rrect(h.x - 3, h.y - 3, h.w + 6, h.h + 6, 9, C(GOLD)); }
            button(optBDraw(i), opts[k].name.c_str(), false, opts[k].id >= 1000 && opts[k].id < 2000 && (opts[k].name == "Flirt" || opts[k].name == "Kiss" || opts[k].name.find("Propose") != std::string::npos) ? 2 : 0, 0.4f);
        }
        if (pieMode) circle(160, 160, 30, C(UI_INK, 60));
        button(bOptBack, "Back", false, 1, 0.42f);
        if (opts.size() > 6) { char pg[24]; snprintf(pg, sizeof pg, "More %d/%d", optPage + 1, (int)(opts.size() + 5) / 6); button(bOptMore, pg, false, 1, 0.38f); }
    } else {
        Actor* mca = mcActor();
        if (!gq.empty()) {   // the action queue: tap an action to cancel it
            for (int i = 0; i < 4 && i < (int)gq.size(); i++) {
                Btn qb = queueB(i);
                rrect(qb.x, qb.y, qb.w, qb.h, 6, C(i == 0 ? UI2 : UI1, 230));
                if (i == 0 && ACT.active() && qPhase == 3) { float k = 1.f - ACT.left / std::fmax(1.f, ACT.total); rrect(qb.x + 1, qb.y + 1, (qb.w - 2) * clampv(k, 0.f, 1.f), qb.h - 2, 5, C(HOT, 170)); }
                else if (i == 0 && qPhase == 1) rrect(qb.x + 1, qb.y + 1, (qb.w - 2) * (0.5f + 0.5f * std::sin(t * 6)), qb.h - 2, 5, C(LILAC, 120));
                std::string lb = gq[i].opt.name;
                if (lb.size() > 11) lb = lb.substr(0, 10) + ".";
                text(lb.c_str(), qb.x + qb.w / 2 - 4, qb.y + 4, 0.36f, C(CREAM), 1);
                text("x", qb.x + qb.w - 7, qb.y + 2, 0.4f, C(BLUSH), 1);
            }
        } else if (ACT.active()) {
            rrect(6, 86, 228, 22, 6, C(UI2));
            float k = 1.f - ACT.left / std::fmax(1.f, ACT.total);
            rrect(7, 87, 226 * clampv(k, 0.f, 1.f), 20, 5, C(HOT, 180));
            text(ACT.name.c_str(), 120, 89, 0.42f, C(CREAM), 1);
            button(bCancel, "Stop", false, 1, 0.42f);
        } else if (mca && mca->seat >= 0) {
            button(queueB(0), "Get Up", false, 2, 0.4f);
            text(mca->useKind == 2 ? "Lying down" : mca->useKind == 3 ? "In the bath" : "Sitting", 200, 89, 0.42f, C(CHAMP), 1);
        } else if (venueHere()) {
            button(bVenue, G.room == R_YARD ? "Garden" : "Things to do here", false, 2, 0.4f);
            if (G.room == R_YARD) text("L/R: rooms", 314, 90, 0.38f, C(CREAM, 160), 2);
            else if (districtNext(G.room, 1) >= 0) textOutline("L/R: next spot", 314, 90, 0.36f, C(200, 250, 255), C(UI_INK), 2, 0.8f);
            if (G.date.active) { snprintf(b, sizeof b, "Date: %d pts", (int)G.date.score); text(b, 238, 89, 0.42f, C(HOT)); }
        } else {
            textOutline(ROOM_NAMES[G.room], 8, 89, 0.45f, C(255, 255, 255), C(UI_INK));
            textOutline("L/R: rooms", 314, 90, 0.36f, C(200, 250, 255), C(UI_INK), 2, 0.8f);
        }
        auto nb = nearby();
        for (int i = 0; i < 4 && i < (int)nb.size(); i++) {
            Btn r = nearB(i);
            flashPanel(r.x, r.y, r.w, r.h - 1, i == 0 ? RGB{255, 255, 255} : RGB{232, 250, 252}, 6, 240);
            const Target& tg = nb[i];
            if (tg.kind == 1) {
                const Actor& a = curRoom().actors[tg.idx];
                heart(r.x + 10, r.y + 11, 4, C(a.kind == 1 ? GOLD : HOT));
                text(targetName(tg).c_str(), r.x + 20, r.y + 3, 0.42f, C(UI_INK));
                if (a.kind == 0) text(relLabel(G.mc, a.id), r.x + r.w - 6, r.y + 4, 0.38f, C(220, 40, 130), 2);
            } else {
                sparkle(r.x + 10, r.y + 11, 5, C(255, 168, 36));
                text(targetName(tg).c_str(), r.x + 20, r.y + 3, 0.42f, C(UI_INK));
            }
        }
    }
    rect(0, 205, BOT_W, 35, C(255, 46, 140));
    rect(0, 204, BOT_W, 2, C(UI_INK));
    button(bPhone, phoneUnread() ? "PHONE (!)" : "PHONE", phoneUnread() > 0, 1, 0.4f);
    button(bBuild, "BUILD", false, 1, 0.4f);
    button(bMap, "MAP", false, 1, 0.4f);
    button(bMenu, "MENU", false, 2, 0.4f);
    if (tut >= 0) button(bTutSkip, "Skip Tutorial", false, 1, 0.36f);
}

// ===================================================================== menu
// menuPage declared above
static Btn menuB(int i) { return Btn{6 + (i % 2) * 156.f, 22 + (i / 2) * 26.f, 152, 23}; }               // page 1: 15 buttons, 8 rows
static Btn modeB(int i) { return Btn{6 + (i % 2) * 156.f, 21 + (i / 2) * 27.f, 152, 24}; }               // page 2: big mode tiles
static int diaryScroll = 0;
static bool diaryUnlocked = false;

static void moveHouse() {
    int from = G.house, to = 1 - G.house;
    for (int r = 0; r < R_COUNT; r++) {
        auto& v = G.rooms[from][r].actors;
        for (size_t i = 0; i < v.size();) {
            if (v[i].kind == 1 || G.sims[v[i].id].household) { Actor a = v[i]; a.x = 60 + irand(280); a.tx = a.x; a.y = 200 + irand(30); G.rooms[to][G.sims.size() && a.kind == 0 && G.sims[a.id].stage == ST_INFANT ? R_NURSERY : R_LIVING].actors.push_back(a); v.erase(v.begin() + i); }
            else i++;
        }
        if (!roomIsHome(r)) G.rooms[from][r].actors.clear();
    }
    G.house = to; G.room = R_LIVING; G.homeId = (uint8_t)to;
    toast(std::string("Moved to the ") + HOUSE_NAMES[to] + "!");
}

static void menuUpdate(const Input& in) {
    if (menuPage == 1 && (in.down & KEY_B)) { menuPage = 0; sfxTap(); return; }
    if (in.down & (KEY_B | KEY_START)) { menuPage = 0; mode = M_PLAY; return; }
    if (!in.touch) return;
    if (menuPage == 1) {   // game modes page
        static const int MAP[16] = {14, 15, 16, 17, 18, 19, 20, 21, 30, 32, 36, 33, 34, 35, 37, 38};
        for (int k = 0; k < 16; k++) {
            if (!modeB(k).in(in.tp)) continue;
            sfxTap();
            const int id = MAP[k];
            if (id == 31) { menuPage = 0; return; }
            if (id == 30) { kpopOpen(); mode = M_KPOP; return; }
            if (id == 32) { challengeOpen(); mode = M_CHALLENGE; return; }
            if (id == 36) { villaOpen(); mode = M_VILLA; return; }
            if (id == 33) { clubsOpen(0); clubsFromPhone = false; mode = M_CLUBS; return; }
            if (id == 34) { clubsOpen(2); clubsFromPhone = false; mode = M_CLUBS; return; }
            if (id == 37) { galleryOpen(); mode = M_GALLERY; return; }
            if (id == 38) { rivalsOpen(); mode = M_RIVALS; return; }
            if (id == 35) { G.filter = (uint8_t)((G.filter + 1) % NUM_FILTERS); toast(std::string("Photo filter: ") + filterName(G.filter)); return; }
            switch (id) {
            case 14: if (MC().stage < ST_TEEN) { toast("Story mode unlocks when you're a teen!"); sfxNo(); return; } storyStart(); mode = M_STORY; return;
            case 15: otomeOpen(); mode = M_OTOME; return;
            case 16: legacyOpen(); mode = M_LEGACY; return;
            case 17: directorOpen(); mode = M_DIRECTOR; return;
            case 18: movieOpen(); mode = M_MOVIEMAKER; return;
            case 19: lifeBookOpen(); mode = M_LIFEBOOK; return;
            case 20: ccOpen(); mode = M_CC; return;
            case 21: worldOpen(); mode = M_WORLD; return;
            }
        }
        return;
    }
    if (menuB(14).in(in.tp)) { sfxTap(); menuPage = 1; return; }
    for (int i = 0; i < 14; i++) {
        if (!menuB(i).in(in.tp)) continue;
        sfxTap();
        switch (i) {
        case 0: mode = M_PLAY; return;
        case 1: slotsOpen(1); mode = M_SLOTS; return;
        case 2: controlsReturn = M_MENU; mode = M_CONTROLS; break;
        case 3: tut = 0; tutMove = 0; mode = M_PLAY; break;
        case 4: G.holiday = (G.holiday + 1) % 4; break;
        case 5: setArtStyle(G.artStyle + 1); G.artStyle = (uint8_t)g_artStyle; toast(std::string("Art style: ") + artStyleName(G.artStyle)); break;
        case 6: { int tr = musicTrack() + 1; if (tr >= NUM_TRACKS) tr = -1; musicPlay(tr); G.track = tr; break; }
        case 7: galleryOpen(); mode = M_GALLERY; break;
        case 8: diaryScroll = 0; diaryUnlocked = G.diaryCode.empty(); mode = M_DIARY; break;
        case 9: G.aging = !G.aging; toast(G.aging ? "Aging ON" : "Aging OFF (sandbox)"); break;
        case 10: G.money += 5000; toast("Cheat: +$5,000!"); break;
        case 11: saveGame(G); mode = M_TITLE; break;
        case 13:   // needs cheat: top up everyone at home
            for (Sim& s : G.sims) if (s.household) for (float& n : s.needs) n = 100;
            addMoodlet(MC(), "Fully Refreshed", EM_HAPPY, 1, 120);
            toast("Cheat: all needs filled!"); sfxYay();
            break;
        case 14:
            if (MC().stage < ST_TEEN) { toast("Story mode unlocks when you're a teen!"); sfxNo(); break; }
            storyStart(); mode = M_STORY; return;
        case 15: otomeOpen(); mode = M_OTOME; return;
        case 16: legacyOpen(); mode = M_LEGACY; return;
        case 17: directorOpen(); mode = M_DIRECTOR; return;
        case 18: movieOpen(); mode = M_MOVIEMAKER; return;
        case 19: lifeBookOpen(); mode = M_LIFEBOOK; return;
        case 20: ccOpen(); mode = M_CC; return;
        case 21: worldOpen(); mode = M_WORLD; return;
        case 12: G.pregnancyOn = !G.pregnancyOn; toast(G.pregnancyOn ? "Babies ON: \"Make a Little Magic\" may lead to a baby" : "Babies OFF: no surprise pregnancies"); break;
        }
        return;
    }
}

static void menuDrawBottom(float t) {
    bottomBackdrop(t);
    text("Menu", 160, 1, 0.5f, C(BLUSH), 1);
    std::string mus = musicTrack() >= 100 ? songName(musicTrack() - 100) : musicTrack() >= 0 && musicTrack() < NUM_TRACKS ? TRACK_NAMES[musicTrack()] : "Off";
    std::string labels[12] = {"Resume", "Save Slots", "3DS Controls", "Replay Tutorial",
                              std::string("Decor: ") + HOLIDAY_NAMES[G.holiday], std::string("Art: ") + artStyleName(G.artStyle),
                              "Music: " + mus, "Home Gallery",
                              "Secret Diary", G.aging ? "Aging: On" : "Aging: Off", "Money Cheat", "Save & Quit"};
    if (menuPage == 1) {
        rect(0, 0, BOT_W, 18, C(250, 236, 246));
        text("Game Modes   (B = back)", 160, 1, 0.5f, C(BERRY), 1);
        std::string st = std::string("Story Mode: ") + (G.story.ch >= 8 ? "Complete!" : storyRankName());
        std::string filtLbl = std::string("Filter: ") + filterName(G.filter);
        const char* L[16] = {st.c_str(), "Otome Ver.", legacyMenuLabel(), "Director Mode", "Movie Maker", "Life Book & Family", "CC Manager (Mods)", "World Settings", "K-Pop Trainee", "Challenge Modes", "Love Villa", "Clubs", "Daily Goals", filtLbl.c_str(), "Home Gallery", "Sugar Rivals PvP"};
        static const int STY[16] = {3, 1, 0, 3, 2, 0, 3, 2, 1, 1, 1, 0, 2, 3, 0, 2};
        for (int k = 0; k < 16; k++) button(modeB(k), L[k], k >= 8 && k <= 11, STY[k], 0.38f);
        return;
    }
    for (int i = 0; i < 12; i++) button(menuB(i), labels[i].c_str(), false, i == 0 ? 2 : i == 11 ? 1 : 0, 0.4f);
    button(menuB(12), G.pregnancyOn ? "Babies: On" : "Babies: Off", false, 0, 0.4f);
    button(menuB(13), "Needs Cheat", false, 2, 0.4f);
    button(menuB(14), "Game Modes >", true, 3, 0.42f);
    return;
    { std::string st = std::string("Story Mode: ") + (G.story.ch >= 8 ? "Complete!" : storyRankName()); button(menuB(14), st.c_str(), false, 3, 0.36f); }
    button(menuB(15), "Otome Ver.", false, 1, 0.36f);
    button(menuB(16), legacyMenuLabel(), false, 0, 0.36f);
    button(menuB(17), "Director Mode", false, 3, 0.36f);
    button(menuB(18), "Movie Maker", false, 2, 0.36f);
    button(menuB(19), "Life Book & Family", false, 0, 0.36f);
    button(menuB(20), "CC Manager (Mods)", false, 3, 0.36f);
    button(menuB(21), "World Settings", false, 2, 0.36f);
}

static void diaryUpdate(const Input& in) {
    if (in.down & KEY_B) { mode = M_MENU; return; }
    if (in.down & KEY_UP) diaryScroll = std::max(0, diaryScroll - 1);
    if (in.down & KEY_DOWN) diaryScroll = std::min(std::max(0, (int)G.diary.size() - 1), diaryScroll + 1);
    if (!in.touch) return;
    if (!diaryUnlocked && Btn{100, 90, 120, 34}.in(in.tp)) {
        std::string code;
        if (askText("Secret code", code, 8, true) && code == G.diaryCode) { diaryUnlocked = true; sfxYay(); } else { toast("Wrong code!"); sfxNo(); }
        return;
    }
    if (diaryUnlocked && Btn{6, 170, 150, 30}.in(in.tp)) {
        std::string s;
        if (askText("Dear Diary...", s, 240)) G.diary.push_back({dateStr(G.day) + " " + clockStr(G.minute), s});
    }
    if (diaryUnlocked && Btn{164, 170, 150, 30}.in(in.tp)) {
        std::string code;
        if (askText("New code (blank = no lock)", code, 8, true)) { G.diaryCode = code; toast("Diary locked with your code!"); }
        else { G.diaryCode.clear(); toast("Diary lock removed."); }
    }
    if (Btn{120, 206, 80, 30}.in(in.tp)) mode = M_MENU;
}

static void diaryDrawTop(float t) {
    grad(0, 0, TOP_W, TOP_H, C(255, 236, 242), C(255, 214, 228));
    rrect(20, 10, 360, 222, 12, C(HOT)); rrect(26, 16, 348, 210, 10, C(255, 250, 246));
    for (int i = 0; i < 12; i++) line(40, 46 + i * 15.f, 360, 46 + i * 15.f, 1, C(190, 210, 240));
    line(56, 20, 56, 222, 1, C(255, 150, 170));
    textShadow("My Super-Secret Diary", 200, 20, 0.6f, C(BERRY), C(BLUSH), 1);
    heart(46, 28, 5, C(HOT)); heart(354, 28, 5, C(HOT));
    if (!diaryUnlocked) { text("LOCKED", 200, 110, 0.9f, C(BERRY), 1); heart(200, 160, 14, C(HOT)); return; }
    if (G.diary.empty()) { text("No entries yet. Write your first secret!", 210, 110, 0.48f, C(UIM), 1); return; }
    float y = 44;
    for (int i = (int)G.diary.size() - 1 - diaryScroll; i >= 0 && y < 214; i--) {
        const DiaryEntry& d = G.diary[i];
        text(d.date.c_str(), 62, y, 0.4f, C(HOT));
        y += 15;
        for (auto& l : wrapText(d.text, 0.42f, 290)) { if (y > 214) break; text(l.c_str(), 62, y, 0.42f, C(UI1)); y += 15; }
        y += 4;
    }
    (void)t;
}

static void diaryDrawBottom(float t) {
    bottomBackdrop(t);
    text("Secret Diary", 160, 6, 0.6f, C(BLUSH), 1);
    if (!diaryUnlocked) button(Btn{100, 90, 120, 34}, "Enter Code", false, 2, 0.5f);
    else {
        button(Btn{6, 170, 150, 30}, "Write Entry", false, 0, 0.45f);
        button(Btn{164, 170, 150, 30}, G.diaryCode.empty() ? "Set Lock Code" : "Change Code", false, 2, 0.45f);
        text("Up/Down: flip pages", 160, 60, 0.45f, C(CREAM), 1);
    }
    button(Btn{120, 206, 80, 30}, "Back", false, 1, 0.45f);
}

// ===================================================================== title
static const Btn bContinue{70, 70, 180, 36}, bNew{70, 114, 180, 36}, bCtl{70, 158, 180, 32};
static bool hasSave = false;

static void titleDrawTop(float t) {
    // sunny Flash-game beach
    grad(0, 0, TOP_W, 150, C(90, 200, 245), C(200, 240, 255));
    flashSun(330, 48, 22, t);
    flashCloud(70 + std::fmod(t * 4, 40.f), 40, 1.1f); flashCloud(220 - std::fmod(t * 3, 30.f), 64, 0.8f);
    grad(0, 128, TOP_W, 40, C(40, 180, 220), C(110, 220, 235));
    for (int i = 0; i < 8; i++) ellipse(i * 56.f + std::fmod(t * 12, 56.f), 140 + (i % 2) * 10, 30, 3, C(255, 255, 255, 140));
    grad(0, 162, TOP_W, 78, C(250, 226, 170), C(236, 200, 140));
    rect(0, 161, TOP_W, 2, C(UI_INK));
    // the cast: a Flash stick figure and a Fashion Doll strike a pose
    const int keep = g_artStyle;
    bool lay = artLayerBegin(1, 10);
    PersonDef a = PEOPLE[0];
    a.age = ADULT; a.guy = false; a.skin = 8; a.hairCol = 0; a.hairStyle = 3; a.top = HOT; a.bottom = {84, 132, 204}; a.bt = BT_SHORTS;
    a.topStyle = TS_TANK; a.shoeStyle = SS_SANDALS; a.hat = HT_NONE; a.acc = 0; a.f.blush = 0; a.f.liner = 1;
    PersonDef b = PEOPLE[1];
    b.age = ADULT; b.guy = false; b.skin = 4; b.hairCol = 3; b.hairStyle = 1; b.top = {150, 96, 230}; b.bottom = {84, 132, 204}; b.bt = BT_JEANS;
    b.topStyle = TS_CROP; b.shoeStyle = SS_PLATFORMS; b.acc = ACC_BAG | ACC_HOOPS | ACC_NECKLACE; b.accCol = {255, 90, 160};
    b.f.eyes = 0; b.f.shadow = 4; b.f.liner = 2; b.f.blush = 0; b.f.pierce = 2; b.f.eyeCol = 3;
    PersonDef c = PEOPLE[11];
    c.age = ADULT; c.guy = true; c.skin = 9; c.hairCol = 0; c.hairStyle = 15; c.top = WHITE; c.bottom = {70, 110, 190}; c.bt = BT_SHORTS; c.topStyle = TS_TANK; c.shoeStyle = SS_SANDALS; c.hat = HT_NONE;
#ifdef DIVA_LIFE
    setArtStyle(G.artStyle < NUM_ART_STYLES ? G.artStyle : 4);
    if (artIsPixel(g_artStyle) || g_artStyle < 3) setArtStyle(4);
#else
    setArtStyle(7);
#endif
    g_stickMood = 1; drawPerson(a, 0, 236, 226, 1.15f, t, 0, PO_WAVE, false);
    g_stickMood = 0; drawPerson(c, 0, 288, 228, 1.2f, t + 1.f, 0, PO_IDLE, true);
    setArtStyle(10);
    g_stickMood = 8; drawPerson(b, 0, 352, 232, 1.32f, t + 2.f, 0, PO_IDLE, true);
    g_stickMood = 0;
    if (lay) artLayerEnd(180, 0, 400, 240);
    setArtStyle(keep);
    // logo
    textOutline(GAME_TITLE, 104, 70, 0.82f, C(255, 255, 255), C(UI_INK), 1, 1.6f);
    textOutline(GAME_TITLE, 104, 70, 0.82f, C(255, 70, 160), C(255, 255, 255), 1, 0.7f);
    flashPanel(30, 106, 148, 24, RGB{40, 192, 210}, 10);
#ifdef DIVA_LIFE
    textOutline("DIVA LIFE 2.0", 104, 108, 0.5f, C(255, 255, 255), C(UI_INK), 1, 1.0f);
#else
    textOutline("STARLIGHT BAY", 104, 108, 0.5f, C(255, 255, 255), C(UI_INK), 1, 1.0f);
#endif
    for (int i = 0; i < 5; i++) sparkle(20 + i * 42, 58 + (i % 2) * 70, 2 + std::fabs(std::sin(t * 2 + i)) * 4, C(255, 255, 255, 230));
    teenBadge(26, 206, 1.0f);
    textOutline("Mild Crime - Comic Mischief - Romance", 50, 215, 0.32f, C(255, 255, 255), C(UI_INK), 0, 0.8f);
    { char vb[48]; snprintf(vb, sizeof vb, "%s  -  What's New: Game Settings app", GAME_VERSION); textOutline(vb, 396, 228, 0.3f, C(255, 255, 255), C(UI_INK), 2, 0.8f); }
}

static void titleDrawBottom(float t) {
    bottomBackdrop(t);
    textOutline("Welcome to Starlight Bay!", 160, 18, 0.58f, C(255, 255, 255), C(UI_INK), 1, 1.1f);
    textOutline("Art style can be changed in Create-a-Sim or the Menu", 160, 40, 0.34f, C(255, 236, 120), C(UI_INK), 1, 0.8f);
    if (hasSave) button(bContinue, "Load Game", false, 2, 0.6f);
    button(bNew, "New Life", false, 0, 0.6f);
    button(bCtl, "3DS Controls", false, 1, 0.5f);
    textOutline("START: quit to Homebrew Launcher", 160, 214, 0.36f, C(220, 250, 255), C(UI_INK), 1, 0.8f);
}

static void enterPlay(bool fresh) {
    ccAssignSlots();
    spriteEnsureCast();
#ifdef MAPLE_VALLEY
    if (spriteNames().empty()) spriteScan();
    if (spriteNames().empty()) toast("Sprite art not found! Unzip MapleValley_SD_card.zip so the SD card has LifeSim/Sprites/ at the top level.");
#endif
    mode = M_PLAY;
    menuOpen = false;
    ACT = Activity();
    partyMins = 0;
    if (G.track >= 0) musicPlay(G.track);
    if (fresh) { tut = 0; tutMove = 0; G.money = 2500; toast("Welcome to your new life, " + MC().name + "!"); cutscenePlay(CS_INTRO); }
}

// ===================================================================== save slots screen
static int slotMode = 0, slotSel = 0, slotConfirm = 0;   // mode 0 load, 1 save (in game), 2 new life; confirm 1 overwrite 2 delete 3 new-over
static SlotInfo slotInf[NUM_SLOTS];
static Mode slotReturn = M_TITLE;
static void slotsRefresh() { for (int k = 0; k < NUM_SLOTS; k++) readSlotInfo(k, slotInf[k]); hasSave = saveExists(); }
static void slotsOpen(int m) {
    slotMode = m; slotConfirm = 0; slotReturn = m == 1 ? M_MENU : M_TITLE; slotsRefresh();
    slotSel = g_saveSlot;
    if (m == 0 && !slotInf[slotSel].used) for (int k = 0; k < NUM_SLOTS; k++) if (slotInf[k].used) { slotSel = k; break; }
    if (m == 2 && slotInf[slotSel].used) for (int k = 0; k < NUM_SLOTS; k++) if (!slotInf[k].used) { slotSel = k; break; }
}
static Btn slotRow(int k) { return Btn{4, 22.f + k * 23, 312, 21}; }
static const Btn bsBack{4, 208, 62, 28}, bsMain{70, 208, 110, 28}, bsRename{184, 208, 64, 28}, bsDelete{252, 208, 64, 28}, bsYes{70, 150, 84, 30}, bsNo{166, 150, 84, 30};
static void slotDoMain() {
    const int k = slotSel; const bool used = slotInf[k].used;
    if (slotMode == 0) {
        if (!used) { toast("This slot is empty. Pick a saved life (or start a New Life)."); sfxNo(); return; }
        g_saveSlot = k;
        if (loadGame(G)) { setArtStyle(G.artStyle); enterPlay(false); toast("Welcome back, " + MC().name + "!"); sfxYay(); }
        else { toast("Couldn't load that save (and its backup)."); sfxNo(); }
        return;
    }
    if (slotMode == 1) {
        if (used && k != g_saveSlot && slotConfirm != 1) { slotConfirm = 1; return; }
        if (used && k == g_saveSlot && slotConfirm != 1) { slotConfirm = 1; return; }
        slotConfirm = 0;
        if (k != g_saveSlot) { g_slotName = slotInf[k].used ? slotInf[k].name : "My Life"; }
        g_saveSlot = k;
        toast(saveGame(G) ? "Saved to slot " + std::to_string(k + 1) + "!" : "Save failed! Check SD card."); sfxYay(); slotsRefresh(); return;
    }
    // new life
    if (used && slotConfirm != 3) { slotConfirm = 3; return; }
    slotConfirm = 0;
    if (used) deleteSlot(k);
    g_saveSlot = k; g_playSecs = 0; g_slotName = "My Life";
    casStart(); mode = M_CAS; sfxTap();
}
static void slotsUpdate(const Input& in) {
    if (in.down & KEY_B) { if (slotConfirm) slotConfirm = 0; else mode = slotReturn; return; }
    if (in.down & KEY_DUP) { slotSel = (slotSel + NUM_SLOTS - 1) % NUM_SLOTS; slotConfirm = 0; }
    if (in.down & KEY_DDOWN) { slotSel = (slotSel + 1) % NUM_SLOTS; slotConfirm = 0; }
    if (in.down & KEY_A) { if (slotConfirm == 2) { deleteSlot(slotSel); slotConfirm = 0; slotsRefresh(); toast("Save deleted."); } else slotDoMain(); return; }
    if (!in.touch) return;
    if (slotConfirm) {
        if (bsYes.in(in.tp)) { sfxTap(); if (slotConfirm == 2) { deleteSlot(slotSel); slotConfirm = 0; slotsRefresh(); toast("Save deleted."); } else slotDoMain(); }
        else if (bsNo.in(in.tp)) { slotConfirm = 0; sfxTap(); }
        return;
    }
    for (int k = 0; k < NUM_SLOTS; k++) if (slotRow(k).in(in.tp)) { if (slotSel == k) slotDoMain(); else { slotSel = k; sfxTap(); } return; }
    if (bsBack.in(in.tp)) { sfxTap(); mode = slotReturn; return; }
    if (bsMain.in(in.tp)) { sfxTap(); slotDoMain(); return; }
    if (bsRename.in(in.tp)) {
        if (!slotInf[slotSel].used) { toast("Save something here first!"); sfxNo(); return; }
        std::string nm = slotInf[slotSel].name;
        if (askText("Save name", nm, 24, false) && !nm.empty()) { if (renameSlot(slotSel, nm)) { if (slotSel == g_saveSlot) g_slotName = nm; toast("Renamed!"); } slotsRefresh(); }
        return;
    }
    if (bsDelete.in(in.tp)) {
        if (!slotInf[slotSel].used) { sfxNo(); return; }
        if (slotMode == 1 && slotSel == g_saveSlot) { toast("You can't delete the life you're playing right now."); sfxNo(); return; }
        slotConfirm = 2; sfxNo(); return;
    }
}
static std::string playStr(uint32_t s) { char b[32]; snprintf(b, sizeof b, "%uh %02um", s / 3600, (s / 60) % 60); return b; }
static std::string realStr(int64_t t) { if (t <= 0) return "unknown"; time_t tt = (time_t)t; struct tm* m = localtime(&tt); if (!m) return "unknown"; char b[40]; strftime(b, sizeof b, "%b %d %Y  %I:%M %p", m); return b; }
static void slotsDrawTop(float t) {
    grad(0, 0, TOP_W, TOP_H, C(255, 226, 240), C(226, 214, 255));
    textOutline(slotMode == 0 ? "LOAD A LIFE" : slotMode == 1 ? "SAVE YOUR LIFE" : "START A NEW LIFE", 200, 8, 0.7f, C(255, 255, 255), C(255, 70, 150), 1);
    const SlotInfo& s = slotInf[slotSel];
    char b[96];
    rrect(30, 44, 340, 170, 12, C(UI_INK)); rrect(32, 46, 336, 166, 11, C(255, 252, 254));
    snprintf(b, sizeof b, "Slot %d", slotSel + 1); text(b, 50, 54, 0.4f, C(255, 70, 150));
    if (!s.used) { text("Empty slot", 200, 112, 0.7f, C(170, 150, 170), 1); text(slotMode == 2 ? "A brand-new life starts here!" : "Nothing saved here yet.", 200, 146, 0.42f, C(150, 130, 150), 1); return; }
    if (s.hasInfo) { circle(96, 126, 44, C(UI_INK)); circle(96, 126, 41, C(255, 226, 240)); drawSimPortrait(s.portrait, 96, 126, 38, t); }
    textOutline(s.name.c_str(), 150, 64, 0.6f, C(255, 255, 255), C(150, 30, 120), 0);
    snprintf(b, sizeof b, "Playing as: %s", s.mc.c_str()); text(b, 150, 92, 0.42f, C(UI_INK));
    snprintf(b, sizeof b, "Household: the %s family (%d)", s.family.c_str(), s.household); text(b, 150, 110, 0.4f, C(UI_INK));
    snprintf(b, sizeof b, "%s, %s", dateStr(s.day).c_str(), clockStr(s.minute).c_str()); text(b, 150, 130, 0.4f, C(110, 90, 130));
    std::string pl = "Playtime: " + playStr(s.playSecs); text(pl.c_str(), 150, 150, 0.4f, C(110, 90, 130));
    std::string sv = "Last saved: " + realStr(s.savedAt); text(sv.c_str(), 150, 170, 0.36f, C(140, 120, 150));
    if (slotMode == 1 && slotSel == g_saveSlot) text("(the life you're playing now)", 200, 194, 0.36f, C(255, 70, 150), 1);
}
static void slotsDrawBottom(float t) {
    bottomBackdrop(t);
    for (int k = 0; k < NUM_SLOTS; k++) {
        const SlotInfo& s = slotInf[k]; Btn r = slotRow(k);
        rrect(r.x, r.y, r.w, r.h, 6, C(k == slotSel ? RGB{255, 70, 150} : RGB{60, 40, 70})); rrect(r.x + 2, r.y + 2, r.w - 4, r.h - 4, 5, C(k == slotSel ? RGB{255, 236, 246} : RGB{255, 255, 255}));
        char b[96];
        if (!s.used) { snprintf(b, sizeof b, "Slot %d  -  Empty", k + 1); text(b, 16, r.y + 4, 0.4f, C(160, 140, 160)); continue; }
        if (s.hasInfo) drawSimPortrait(s.portrait, r.x + 16, r.y + 11, 8, t);
        snprintf(b, sizeof b, "%d. %s", k + 1, s.name.c_str()); text(b, 32, r.y + 1, 0.36f, C(UI_INK));
        snprintf(b, sizeof b, "%s  -  Day %d  -  %s", s.mc.c_str(), s.day + 1, playStr(s.playSecs).c_str()); text(b, 32, r.y + 11, 0.27f, C(130, 110, 140));
        if (k == g_saveSlot && slotMode == 1) text("NOW", 306, r.y + 6, 0.3f, C(255, 70, 150), 2);
    }
    button(bsBack, "Back", false, 1, 0.38f);
    button(bsMain, slotMode == 0 ? "Load" : slotMode == 1 ? "Save Here" : "New Life Here", false, 2, 0.36f);
    button(bsRename, "Rename", false, 0, 0.32f);
    button(bsDelete, "Delete", false, 1, 0.32f);
    if (slotConfirm) {
        rect(0, 0, BOT_W, BOT_H, C(0, 0, 0, 140));
        rrect(30, 70, 260, 120, 10, C(UI_INK)); rrect(32, 72, 256, 116, 9, C(255, 250, 252));
        char b[96];
        if (slotConfirm == 1) snprintf(b, sizeof b, "Overwrite slot %d?", slotSel + 1);
        else if (slotConfirm == 2) snprintf(b, sizeof b, "Delete slot %d forever?", slotSel + 1);
        else snprintf(b, sizeof b, "Erase slot %d for a new life?", slotSel + 1);
        text(b, 160, 88, 0.48f, C(UI_INK), 1);
        text(slotConfirm == 1 ? "The old save is kept as a backup." : "This can't be undone.", 160, 114, 0.36f, C(150, 120, 150), 1);
        button(bsYes, "Yes", false, 2, 0.45f); button(bsNo, "No", false, 1, 0.45f);
    }
}

static void titleUpdate(const Input& in) {
    if (in.down & KEY_START) { quitApp = true; return; }
    if (!in.touch) { if ((in.down & KEY_A) && hasSave) { slotsOpen(0); mode = M_SLOTS; } return; }
    if (hasSave && bContinue.in(in.tp)) { slotsOpen(0); mode = M_SLOTS; sfxTap(); return; }
    if (bNew.in(in.tp)) { slotsOpen(2); mode = M_SLOTS; sfxTap(); return; }
    if (bCtl.in(in.tp)) { controlsReturn = M_TITLE; mode = M_CONTROLS; sfxTap(); return; }
}

// ===================================================================== overlays
static std::string rwTitle, rwLine;
static float rwT = 0;
void rewardPopup(const std::string& title, const std::string& line) { rwTitle = title; rwLine = line; rwT = 4.5f; }
static void drawReward(float dt) {
    if (rwT <= 0) return;
    rwT -= dt;
    float a = clampv(rwT * 3.f, 0.f, 1.f), slide = std::fmin(1.f, (4.5f - rwT) * 4.f);
    float x = 400 - 170 * slide, y = 64;
    static const RGB RB[6] = {{255, 46, 140}, {255, 168, 36}, {255, 220, 60}, {96, 206, 96}, {40, 192, 210}, {150, 96, 230}};
    rrect(x - 2, y - 2, 164, 62, 10, C(UI_INK, (uint8_t)(255 * a)));
    for (int i = 0; i < 3; i++) rrect(x + i, y + i, 160 - i * 2, 58 - i * 2, 9, C(RB[(i + (int)(T * 5)) % 6], (uint8_t)(255 * a)));
    rrect(x + 4, y + 18, 152, 37, 7, C(255, 255, 255, (uint8_t)(250 * a)));
    sparkle(x + 12, y + 9, 5, C(255, 220, 60, (uint8_t)(255 * a))); sparkle(x + 148, y + 9, 5, C(255, 220, 60, (uint8_t)(255 * a)));
    textOutline(rwTitle.c_str(), x + 80, y + 1, 0.4f, C(255, 255, 255, (uint8_t)(255 * a)), C(UI_INK, (uint8_t)(255 * a)), 1, 0.9f);
    auto ls = wrapText(rwLine, 0.38f, 146);
    for (size_t i = 0; i < ls.size() && i < 2; i++) textOutline(ls[i].c_str(), x + 80, y + 20 + i * 15.f, 0.38f, C(200, 150, 0, (uint8_t)(255 * a)), C(255, 255, 255, (uint8_t)(255 * a)), 1, 0.5f);
}
// Sims-style thought bubbles: "Amy thinks you're SO sweet."
struct Thought { int sim; std::string text; int mood; float t; };
static std::deque<Thought> thoughts;
void thoughtPopup(int sim, const std::string& text, int mood) {
    if (sim < 0 || sim >= (int)G.sims.size()) return;
    if (thoughts.size() >= 3) thoughts.pop_front();
    thoughts.push_back({sim, text, mood, 0.f});
}
static void drawThoughts(float dt) {
    if (thoughts.empty()) return;
    Thought& th = thoughts.front();
    th.t += dt;
    const float life = 3.4f;
    if (th.t > life || th.sim >= (int)G.sims.size()) { thoughts.pop_front(); return; }
    float a = clampv(std::fmin(th.t * 5.f, (life - th.t) * 4.f), 0.f, 1.f);
    float slide = std::fmin(1.f, th.t * 5.f);
    const float x = 6 - (1 - slide) * 30, y = 168;
    static const RGB MC_[4] = {{150, 150, 170}, {150, 150, 170}, {96, 206, 96}, {255, 90, 160}};
    RGB edge = th.mood < 0 ? RGB{230, 70, 80} : MC_[std::min(3, th.mood + 1)];
    auto ls = wrapText(th.text, 0.4f, 196);
    const float h = 26 + std::min<size_t>(ls.size(), 2) * 14.f;
    // the bubble
    rrect(x - 2, y - 2, 262 + 4, h + 4, 12, C(UI_INK, (uint8_t)(255 * a)));
    rrect(x, y, 262, h, 11, C(255, 255, 255, (uint8_t)(245 * a)));
    rrect(x, y, 6, h, 3, C(edge, (uint8_t)(255 * a)));
    for (int i = 0; i < 3; i++) circle(x + 40 + i * 7.f, y + h + 4 + i * 4.f, 3.5f - i, C(255, 255, 255, (uint8_t)(230 * a)));
    if (a > 0.6f) { circle(x + 26, y + h / 2, 18, C(UI_INK)); circle(x + 26, y + h / 2, 16.5f, C(255, 236, 244)); drawSimPortrait(G.sims[th.sim], x + 26, y + h / 2, 16, T); }
    std::string who = G.sims[th.sim].name + " thinks...";
    text(who.c_str(), x + 50, y + 3, 0.38f, C(edge.r / 2 + 40, edge.g / 2 + 20, edge.b / 2 + 30, (uint8_t)(255 * a)));
    for (size_t i = 0; i < ls.size() && i < 2; i++) text(ls[i].c_str(), x + 50, y + 18 + i * 14.f, 0.4f, C(UI_INK, (uint8_t)(255 * a)));
    const char* icon = th.mood < 0 ? "!" : th.mood == 2 ? "<3" : th.mood == 1 ? "+" : "?";
    textOutline(icon, x + 248, y + 4, 0.5f, C(edge, (uint8_t)(255 * a)), C(255, 255, 255, (uint8_t)(255 * a)), 1, 0.6f);
}
static void drawToastsAndFx(float dt) {
    if (mode == M_PLAY) brawlFrame(dt);
    drawReward(dt);
    if (mode == M_PLAY || mode == M_MENU) drawThoughts(dt);
    for (size_t i = 0; i < parts.size();) {
        Particle& p = parts[i];
        p.life -= dt; p.vy += 300 * dt; p.x += p.vx * dt; p.y += p.vy * dt;
        if (p.life <= 0 || p.y > 250) { parts.erase(parts.begin() + i); continue; }
        uint8_t al = (uint8_t)clampv(p.life * 255.f, 0.f, 255.f);
        if (p.kind == 0) sparkle(p.x, p.y, 4, C(p.c, al));
        else if (p.kind == 1) heart(p.x, p.y, 3, C(p.c, al));
        else rect(p.x, p.y, 4, 3, C(p.c, al));
        i++;
    }
    if (!toasts.empty()) {
        toastT += dt;
        const std::string& m = toasts.front();
        // wrap long messages onto up to 3 lines; show longer ones for longer; hurry when a queue builds up
        std::vector<std::string> lines; { std::string cur, word; const float maxW = 372; auto flush = [&]() { if (!cur.empty()) lines.push_back(cur); cur.clear(); };
            for (size_t i = 0; i <= m.size(); i++) { const char c = i < m.size() ? m[i] : ' '; if (c == ' ') { const std::string t2 = cur.empty() ? word : cur + " " + word; if (textW(t2.c_str(), 0.4f) > maxW && !cur.empty()) { flush(); cur = word; } else cur = t2; word.clear(); } else word += c; }
            flush(); if (lines.size() > 3) { lines.resize(3); lines[2] += "..."; } }
        const float sc = lines.size() >= 3 ? 0.36f : 0.4f;
        const float hold = std::min(5.5f, 2.2f + m.size() * 0.03f) * (toasts.size() > 4 ? 0.7f : 1.f) * l3ToastMult();
        const float fade = clampv((hold - toastT) * 4.f, 0.f, 1.f) * clampv(toastT * 8.f, 0.f, 1.f);
        float w = 0; for (auto& l : lines) w = std::fmax(w, textW(l.c_str(), sc)); w = std::fmin(392.f, w + 24);
        const float h = 6 + lines.size() * 26 * sc;
        const float ty = (mode == M_PLAY || mode == M_MENU) ? 38 : 6;
        if (fade > 0.05f) {
            flashPanel(200 - w / 2, ty, w, h, RGB{255, 46, 140}, 8, (uint8_t)(235 * fade));
            for (size_t i = 0; i < lines.size(); i++) textOutline(lines[i].c_str(), 200, ty + 1.5f + i * 26 * sc, sc, C(255, 255, 255, (uint8_t)(255 * fade)), C(UI_INK, (uint8_t)(255 * fade)), 1, 0.8f);
            if (toasts.size() > 1) { char q[8]; snprintf(q, sizeof q, "+%d", (int)toasts.size() - 1); text(q, 200 + w / 2 - 4, ty + h - 11, 0.3f, C(255, 255, 255, (uint8_t)(200 * fade)), 2); }
        }
        if (toastT > hold) { toasts.pop_front(); toastT = 0; }
    }
}

static void playDrawTop(float t) {
    bool savedLights = G.lightsOn;
    int hour = (int)(G.minute / 60);
    if (!roomIsHome(G.room) && G.room != R_CLUB && (hour >= 20 || hour < 6)) G.lightsOn = false;
    bool pix = artLayerBegin(0, g_artStyle);   // the whole room becomes pixel art
    drawRoom(G, t, mode == M_BUILD ? selItem : -1, menuOpen && menuT.kind == 1 ? menuT.idx : -1, partyMins > 0);
    styleGrade(t);
    if (pix) artLayerEnd();
    drawRoomOverlay(G, t);
    brawlDraw(t);
    lxDrawPests(t);
    lxDrawWork(t);
    filterDraw(t);
    G.lightsOn = savedLights;
    if (g_photoClean) return;
    if (mode == M_BUILD) {
        rect(0, 0, TOP_W, 16, C(UI1, 200));
        text("BUILD MODE", 6, 1, 0.42f, C(GOLD));
        if (selItem >= 0) text(itemName(curRoom().items[selItem].item).c_str(), 394, 1, 0.42f, C(BLUSH), 2);
        return;
    }
    // Flash HUD: player card (top left) + location badge (top right)
    {
        Sim& me = MC();
        int lv = 1; for (int k = 0; k < SK_COUNT; k++) lv += skillLevel(me, k);
        lv = 1 + lv / 3;
        std::string nm = upperStr(me.name);
        char b[64]; snprintf(b, sizeof b, "Lv. %d   $%d", lv, (int)G.money);
        float w = std::fmax(textW(nm.c_str(), 0.42f), textW(b, 0.36f)) + 42;
        flashPanel(3, 3, w, 29, RGB{40, 192, 210}, 7, 235);
        circle(18, 17.5f, 11.5f, C(UI_INK)); { Emotion pe = emotionOf(me); RGB pc = pe == EM_FINE ? RGB{255, 236, 244} : EMOTION_COLORS[pe]; circle(18, 17.5f, 10, C(pc)); }
        drawSimPortrait(me, 18, 17.5f, 10, t);
        textOutline(nm.c_str(), 33, 2.5f, 0.42f, C(255, 255, 255), C(UI_INK), 0, 0.9f);
        textOutline(b, 33, 16.5f, 0.36f, C(150, 255, 130), C(UI_INK), 0, 0.8f);
        if (artIsFlip(g_artStyle)) {   // little stat icons: flower = beauty, heart = health, smiley = humor, gem = culture
            static const int SK[4] = {SK_STYLE, SK_FITNESS, SK_CHARISMA, SK_CREATIVITY};
            flashPanel(3, 216, 168, 20, RGB{28, 44, 112}, 4, 235);
            for (int i = 0; i < 4; i++) {
                const float x = 8 + i * 41, y = 219;
                const u32 K = C(30, 20, 36);
                switch (i) {
                case 0: rect(x + 4, y + 2, 6, 10, K); rect(x + 2, y + 4, 10, 6, K); rect(x + 5, y + 3, 4, 8, C(255, 120, 190)); rect(x + 3, y + 5, 8, 4, C(255, 120, 190)); rect(x + 6, y + 6, 2, 2, C(255, 220, 80)); break;
                case 1: rect(x + 2, y + 3, 4, 6, K); rect(x + 8, y + 3, 4, 6, K); rect(x + 3, y + 5, 8, 5, K); rect(x + 5, y + 10, 4, 2, K); rect(x + 3, y + 4, 2, 4, C(240, 50, 70)); rect(x + 9, y + 4, 2, 4, C(240, 50, 70)); rect(x + 4, y + 6, 6, 3, C(240, 50, 70)); rect(x + 6, y + 9, 2, 1, C(240, 50, 70)); rect(x + 3, y + 4, 1, 1, C(255, 200, 200)); break;
                case 2: rect(x + 2, y + 2, 10, 10, K); rect(x + 3, y + 3, 8, 8, C(255, 214, 40)); rect(x + 5, y + 5, 1, 2, K); rect(x + 8, y + 5, 1, 2, K); rect(x + 5, y + 8, 4, 1, K); rect(x + 4, y + 7, 1, 1, K); rect(x + 9, y + 7, 1, 1, K); break;
                default: rect(x + 3, y + 2, 8, 3, K); rect(x + 1, y + 4, 12, 3, K); rect(x + 4, y + 7, 6, 3, K); rect(x + 6, y + 10, 2, 2, K); rect(x + 4, y + 3, 6, 2, C(120, 230, 240)); rect(x + 2, y + 5, 10, 1, C(60, 180, 220)); rect(x + 5, y + 7, 4, 2, C(40, 140, 200)); rect(x + 5, y + 3, 1, 1, C(255, 255, 255)); break;
                }
                char sb[8]; snprintf(sb, sizeof sb, "%d", skillLevel(me, SK[i]));
                text(sb, x + 16, y + 1, 0.4f, C(255, 255, 255));
            }
        }
        if (heatStars() > 0) {   // wanted level
            flashPanel(3, 34, 74, 14, RGB{40, 30, 60}, 5, 220);
            for (int i = 0; i < 5; i++) sparkle(12 + i * 14.f, 41, 5, C(i < heatStars() ? RGB{255, 200, 40} : RGB{90, 80, 110}));
        }
        std::string loc = upperStr(ROOM_NAMES[G.room]);
        float lw = std::fmax(textW(loc.c_str(), 0.42f), textW("LOCATION:", 0.32f)) + 30;
        flashPanel(397 - lw, 3, lw, 29, RGB{40, 192, 210}, 7, 235);
        float px = 397 - lw + 11;
        circle(px, 13, 5.5f, C(UI_INK)); tri(px - 4.6f, 15, px + 4.6f, 15, px, 25, C(UI_INK));
        circle(px, 13, 4.2f, C(255, 46, 140)); tri(px - 3.4f, 15, px + 3.4f, 15, px, 23, C(255, 46, 140)); circle(px, 13, 1.6f, C(UI_INK));
        textOutline("LOCATION:", px + 9, 3, 0.32f, C(255, 236, 120), C(UI_INK), 0, 0.8f);
        textOutline(loc.c_str(), px + 9, 14.5f, 0.42f, C(255, 255, 255), C(UI_INK), 0, 0.9f);
    }
    if (ACT.active() && speedMul == 1 && ACT.speed > 10) text(">>", 392, 222, 0.5f, C(HOT), 2);
    seasonsDrawSleep(t);
    if (fadeT > 0) {   // fade to black (the interaction happens off-screen)
        float a = fadeT > 2.4f ? (3.f - fadeT) / 0.6f : fadeT < 0.6f ? fadeT / 0.6f : 1.f;
        rect(0, 0, TOP_W, TOP_H, C(20, 8, 24, (uint8_t)(255 * clampv(a, 0.f, 1.f))));
        if (a > 0.8f) { heart(200, 110, 14, C(HOT)); sparkle(222, 96, 6, C(GOLD)); sparkle(176, 124, 5, C(BLUSH)); }
    }
    if (mode == M_PLAY) momentDrawTop(t);
    dlgDrawTop(t);
}

// ===================================================================== main
#ifdef __3DS__
extern "C" { u32 __stacksize__ = 256 * 1024; }   // main-thread stack: 256 KB (libctru default is 32 KB) - deep menus, saves & doll painting stay safe
#endif
int main() {
    romfsInit();
    gfxInitDefault();
    // big GPU command buffer: busy scenes switch between shape and circle modes thousands of times
    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE * 6)) C3D_Init(C3D_DEFAULT_CMDBUF_SIZE * 2);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS * 8);
    C2D_Prepare();
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    srand((unsigned)time(nullptr));
    drawInit();
    audioInit();
    musicPlay(0);
    hasSave = saveExists();
    ccLoadAll();
    spriteScan();
    bgScan();
    if (!audioOk()) toast("Tip: dump dspfirm.cdc for music & sounds");

    u64 last = osGetTime();
    float autosave = 0;
    while (aptMainLoop() && !quitApp) {
        u64 now = osGetTime();
        float dt = clampv((now - last) / 1000.f, 0.f, 0.1f);
        last = now;
        T += dt;
        hidScanInput();
        Input in;
        in.down = hidKeysDown(); in.held = hidKeysHeld(); in.dt = dt;
        hidTouchRead(&in.tp);
        hidCircleRead(&in.cp);
        in.touch = (in.down & KEY_TOUCH) != 0;
        audioUpdate();
        ocFrameTick();
        if (mode != M_TITLE && mode != M_CAS && !G.sims.empty()) chatTick(dt);
        if (mode != M_TITLE && mode != M_CAS && !G.sims.empty()) lovebugTick(dt);

        bool consumed = (mode == M_PLAY || mode == M_PHONE || mode == M_BUILD || mode == M_MENU) && tutorialInput(in);
        if (!consumed) {
            switch (mode) {
            case M_TITLE: titleUpdate(in); break;
            case M_CAS: { int r = casUpdate(in); if (r == 1) enterPlay(true); else if (r == -1) mode = M_TITLE; break; }
            case M_PLAY: playUpdate(in); break;
            case M_BUILD: buildUpdate(in); break;
            case M_PHONE: worldTick(dt * speedMul * 0.5f); if (!phoneUpdate(in)) {
                    mode = M_PLAY;
                    if (g_phoneJump == 1) { contentOpen(CT_BAG); mode = M_CONTENT; }
                    else if (g_phoneJump == 4) { galleryOpen(); galleryFromPhone = true; mode = M_GALLERY; }
                    else if (g_phoneJump == 30) { oddJobsOpen(); mode = M_ODDJOB; }
                    else if (g_phoneJump == 41) { opts.clear(); for (auto& p : dsApp()) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
                    else if (g_phoneJump == 39) { opts.clear(); for (auto& p : lpApp()) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
                    else if (g_phoneJump == 38) { opts.clear(); for (auto& p : gsApp()) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
                    else if (g_phoneJump == 37) { houseViewOpen(); mode = M_HOUSE; }
                    else if (g_phoneJump == 36) { opts.clear(); for (auto& p : dyApp()) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
                    else if (g_phoneJump == 35) { photoOpen(); mode = M_PHOTO; }
                    else if (g_phoneJump == 34) { opts.clear(); for (auto& p : dlyApp()) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
                    else if (g_phoneJump == 33) { opts.clear(); for (auto& p : guApp()) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
                    else if (g_phoneJump == 32) { opts.clear(); for (auto& p : kidApp()) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
                    else if (g_phoneJump == 31) { opts.clear(); for (auto& p : lxWardrobe()) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
                    else if (g_phoneJump == 29) { opts.clear(); for (auto& p : seApp()) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
                    else if (g_phoneJump == 40) { opts.clear(); for (auto& p : lxApp(14)) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
                    else if (g_phoneJump == 27 || g_phoneJump == 28) { opts.clear(); for (auto& p : lxApp(g_phoneJump - 15)) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
                    else if (g_phoneJump == 25 || g_phoneJump == 26) { opts.clear(); for (auto& p : lxApp(g_phoneJump - 15)) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
                    else if (g_phoneJump >= 20 && g_phoneJump <= 24) { opts.clear(); for (auto& p : dlApp(g_phoneJump - 20)) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
                    else if (g_phoneJump >= 10 && g_phoneJump <= 19) { opts.clear(); for (auto& p : lxApp(g_phoneJump - 10)) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
                    else if (g_phoneJump == 9) { opts.clear(); for (auto& p : choresOptions()) opts.push_back({p.first, p.second}); menuOpen = true; optPage = 0; menuRoom = G.room; }
                    else if (g_phoneJump == 7 || g_phoneJump == 8) {
                        opts.clear(); for (auto& p : (g_phoneJump == 7 ? spicySettingsOptions() : fanVaultOptions())) opts.push_back({p.first, p.second});
                        menuOpen = true; optPage = 0; menuRoom = G.room;
                    }
                    else if (g_phoneJump == 6) {
                        opts.clear();
                        for (int k = 0; k < songCount(); k++) opts.push_back({std::string(musicTrack() == 100 + k ? "> " : "") + songName(k), 9200 + k});
                        for (int k = 0; k < NUM_TRACKS; k++) opts.push_back({std::string(musicTrack() == k ? "> " : "") + TRACK_NAMES[k] + " (classic)", 9280 + k});
                        opts.push_back({"Shuffle a Song", 9298}); opts.push_back({"Music Off", 9299});
                        menuOpen = true; optPage = 0; menuRoom = G.room; menuRoom = G.room;
                    }
                    else if (g_phoneJump == 5) {
                        static const char* NN[N_COUNT] = {"Hunger", "Energy", "Hygiene", "Fun", "Social", "Bladder"};
                        opts.clear(); opts.push_back({"ALL CHEATS AT ONCE", 9100}); opts.push_back({"Max All Needs", 9101});
                        for (int n = 0; n < N_COUNT; n++) opts.push_back({std::string("Max ") + NN[n], 9102 + n});
                        opts.push_back({"+$1,000 (Rosebud)", 9110}); opts.push_back({"+$50,000 (Motherlode)", 9111}); opts.push_back({"+$1,000,000", 9112}); opts.push_back({"Set Money to $0", 9113});
                        opts.push_back({"Max All Skills", 9114}); opts.push_back({"Make Me Very Happy", 9115}); { extern bool g_dollV2; opts.push_back({g_dollV2 ? "Art Style: Doll 2.0 (tap: Classic)" : "Art Style: Classic (tap: Doll 2.0)", 9116}); }
                        menuOpen = true; optPage = 0; menuRoom = G.room; menuRoom = G.room; menuRoom = G.room;
                    }
                    else if (g_phoneJump == 2 || g_phoneJump == 3) { clubsOpen(g_phoneJump == 2 ? 0 : 2); clubsFromPhone = true; mode = M_CLUBS; }
                    g_phoneJump = 0;
                } break;
            case M_MENU: menuUpdate(in); break;
            case M_COOK: worldTick(dt); if (!cookUpdate(in)) { mode = M_PLAY; contentCookDone(0, 0, 0, 0); } break;
            case M_MG:
                if (!mgUpdate(in)) {
                    mode = M_PLAY;
                    if (g_mgContest) { eventContestResult((const TownEvent*)g_mgContest, g_mgLastScore); g_mgContest = nullptr; }
                }
                break;
            case M_STYLE: if (!styleUpdate(in)) mode = M_PLAY; break;
            case M_SHIFT:
                if (!shiftUpdate(in)) {   // the rest of the shift flies by
                    mode = M_PLAY;
                    Activity a = mk("Wrapping up work", std::fmax(30.f, g_shiftHours * 60.f - 60.f), 60, PO_IDLE);
                    startActivity(a);
                }
                break;
            case M_DIARY: diaryUpdate(in); break;
            case M_GLAM: { int r = casUpdate(in); if (r == 2 || r == 3) mode = M_PLAY; break; }
            case M_CUTSCENE: if (!cutsceneUpdate(in)) mode = M_PLAY; break;
            case M_HEIST: if (!heistUpdate(in)) mode = M_PLAY; break;
            case M_SERVE: if (!serveUpdate(in)) mode = M_PLAY; break;
            case M_SHOPGAME: if (!shopUpdate(in)) mode = M_PLAY; break;
            case M_STORY: if (!storyUpdate(in)) mode = M_PLAY; break;
            case M_OTOME: if (!otomeUpdate(in)) mode = M_PLAY; break;
            case M_LEGACY: if (!legacyUpdate(in)) mode = M_MENU; break;
            case M_DIRECTOR: if (!directorUpdate(in)) mode = M_MENU; break;
            case M_MOVIEMAKER: if (!movieUpdate(in)) mode = M_MENU; break;
            case M_LIFEBOOK: if (!lifeBookUpdate(in)) mode = M_MENU; break;
            case M_CC: if (!ccUpdate(in)) mode = M_MENU; break;
            case M_WORLD: if (!worldUpdate(in)) mode = M_MENU; break;
            case M_KPOP: if (!kpopUpdate(in)) mode = M_MENU; break;
            case M_GALLERY: if (!galleryUpdate(in)) { mode = galleryFromPhone ? M_PLAY : M_MENU; galleryFromPhone = false; } break;
            case M_CHALLENGE: if (!challengeUpdate(in)) mode = M_MENU; break;
            case M_VILLA: if (!villaUpdate(in)) mode = M_MENU; break;
            case M_SPICY: if (!spicyUpdate(in)) mode = M_PLAY; break;
            case M_ODDJOB: if (!oddJobsUpdate(in)) mode = M_PLAY; break;
            case M_PHOTO: if (!photoUpdate(in)) mode = M_PLAY; break;
            case M_RIVALS: if (!rivalsUpdate(in)) mode = M_PLAY; break;
            case M_HOUSE: if (!houseViewUpdate(in)) mode = M_PLAY; break;
            case M_MINI: if (!lpMiniUpdate(in)) mode = M_PLAY; break;
            case M_DESIGN: if (!dsUpdate(in)) mode = M_PLAY; break;
            case M_CLUBS: if (!clubsUpdate(in)) mode = clubsFromPhone ? M_PLAY : M_MENU; break;
            case M_CONTENT: { int r = contentUpdate(in); if (r == 1) mode = M_PLAY; else if (r == 2) { cookStart(); mode = M_COOK; } break; }
            case M_SLOTS: slotsUpdate(in); break;
            case M_CONTROLS: if ((in.down & KEY_B) || (in.touch && Btn{120, 204, 80, 30}.in(in.tp))) mode = controlsReturn; break;
            }
        }
        if (spicyPending() && mode == M_PLAY) { mode = M_SPICY; menuOpen = false; }
        if (cutscenePending() && (mode == M_PLAY || mode == M_PHONE)) { cutsceneBegin(); mode = M_CUTSCENE; menuOpen = false; }
        if (mode == M_PLAY && !G.sims.empty()) { static int ocTick = 0; if (ocTick-- <= 0) { ocEnsureInTown(); ocTick = 600; } }   // the Starlight Cast lives in town
        if (mode != M_TITLE && mode != M_CAS && mode != M_SLOTS && !G.sims.empty()) { static float ps = 0; ps += dt; if (ps >= 1) { g_playSecs += (uint32_t)ps; ps -= (int)ps; } }
        { extern bool platformWantsSave(); if (platformWantsSave() && (mode == M_PLAY || mode == M_BUILD)) autosave = 1e9f; }   // RG Cube: save when the app goes to the background
        if (mode == M_PLAY || mode == M_BUILD) {
            autosave += dt;
            if (autosave > 300) { autosave = 0; if (saveGame(G)) toast("Autosaved"); }
        }

        drawFrameBegin();
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, C(UI1));
        C2D_SceneBegin(top);
        drawSetScreen(top);
        switch (mode) {
        case M_TITLE: titleDrawTop(T); break;
        case M_CAS: casDrawTop(T); break;
        case M_PLAY: case M_BUILD: case M_MENU: playDrawTop(T); break;
        case M_PHONE: phoneDrawTop(T); break;
        case M_COOK: cookDrawTop(T); break;
        case M_MG: mgDrawTop(T); break;
        case M_STYLE: styleDrawTop(T); break;
        case M_SHIFT: shiftDrawTop(T); break;
        case M_DIARY: diaryDrawTop(T); break;
        case M_GLAM: casDrawTop(T); break;
        case M_CUTSCENE: cutsceneDrawTop(T); break;
        case M_HEIST: heistDrawTop(T); break;
        case M_SERVE: serveDrawTop(T); break;
        case M_SHOPGAME: shopDrawTop(T); break;
        case M_STORY: storyDrawTop(T); break;
        case M_OTOME: otomeDrawTop(T); break;
        case M_LEGACY: legacyDrawTop(T); break;
        case M_DIRECTOR: directorDrawTop(T); break;
        case M_MOVIEMAKER: movieDrawTop(T); break;
        case M_LIFEBOOK: lifeBookDrawTop(T); break;
        case M_CC: ccDrawTop(T); break;
        case M_WORLD: worldDrawTop(T); break;
        case M_KPOP: kpopDrawTop(T); break;
        case M_GALLERY: galleryDrawTop(T); break;
        case M_CHALLENGE: challengeDrawTop(T); break;
        case M_CLUBS: clubsDrawTop(T); break;
        case M_VILLA: villaDrawTop(T); break;
        case M_SPICY: spicyDrawTop(T); break;
        case M_ODDJOB: oddJobsDrawTop(T); break;
        case M_PHOTO: playDrawTop(T); photoDrawOverlay(T); break;
        case M_RIVALS: rivalsDrawTop(T); break;
        case M_HOUSE: houseViewDrawTop(T); break;
        case M_MINI: lpMiniDrawTop(T); break;
        case M_DESIGN: dsDrawTop(T); break;
        case M_CONTENT: contentDrawTop(T); break;
        case M_SLOTS: slotsDrawTop(T); break;
        case M_CONTROLS: drawConsole(T); break;
        }
        if (mode == M_MENU) { rect(0, 0, TOP_W, TOP_H, C(UI1, 120)); textShadow("Paused", 200, 100, 0.9f, C(BLUSH), C(BLACK), 1); }
        if (mode != M_CUTSCENE && mode != M_HEIST && mode != M_SERVE && mode != M_SHOPGAME && mode != M_STORY && mode != M_OTOME && mode != M_LEGACY && mode != M_DIRECTOR && mode != M_MOVIEMAKER && mode != M_LIFEBOOK && mode != M_SLOTS && mode != M_CC && mode != M_WORLD && mode != M_KPOP && mode != M_GALLERY && mode != M_CONTENT && mode != M_CHALLENGE && mode != M_CLUBS && mode != M_VILLA && mode != M_SPICY && mode != M_ODDJOB && mode != M_PHOTO && mode != M_RIVALS && mode != M_HOUSE && mode != M_MINI && mode != M_DESIGN) drawTutorialTop();
        if (mode != M_PHOTO) drawToastsAndFx(dt);
        C2D_TargetClear(bot, C(UI1));
        C2D_SceneBegin(bot);
        drawSetScreen(bot);
        switch (mode) {
        case M_TITLE: titleDrawBottom(T); break;
        case M_CAS: casDrawBottom(T); break;
        case M_PLAY: playDrawBottom(T); if (momentActive() && !dlgActive()) momentDrawBottom(T); break;
        case M_BUILD: buildDrawBottom(T); break;
        case M_PHONE: phoneDrawBottom(T); break;
        case M_MENU: menuDrawBottom(T); break;
        case M_COOK: cookDrawBottom(T); break;
        case M_MG: mgDrawBottom(T); break;
        case M_STYLE: styleDrawBottom(T); break;
        case M_SHIFT: shiftDrawBottom(T); break;
        case M_DIARY: diaryDrawBottom(T); break;
        case M_GLAM: casDrawBottom(T); break;
        case M_CUTSCENE: cutsceneDrawBottom(T); break;
        case M_HEIST: heistDrawBottom(T); break;
        case M_SERVE: serveDrawBottom(T); break;
        case M_SHOPGAME: shopDrawBottom(T); break;
        case M_STORY: storyDrawBottom(T); break;
        case M_OTOME: otomeDrawBottom(T); break;
        case M_LEGACY: legacyDrawBottom(T); break;
        case M_DIRECTOR: directorDrawBottom(T); break;
        case M_MOVIEMAKER: movieDrawBottom(T); break;
        case M_LIFEBOOK: lifeBookDrawBottom(T); break;
        case M_CC: ccDrawBottom(T); break;
        case M_WORLD: worldDrawBottom(T); break;
        case M_KPOP: kpopDrawBottom(T); break;
        case M_GALLERY: galleryDrawBottom(T); break;
        case M_CHALLENGE: challengeDrawBottom(T); break;
        case M_CLUBS: clubsDrawBottom(T); break;
        case M_VILLA: villaDrawBottom(T); break;
        case M_SPICY: spicyDrawBottom(T); break;
        case M_ODDJOB: oddJobsDrawBottom(T); break;
        case M_PHOTO: photoDrawBottom(T); break;
        case M_RIVALS: rivalsDrawBottom(T); break;
        case M_HOUSE: houseViewDrawBottom(T); break;
        case M_MINI: lpMiniDrawBottom(T); break;
        case M_DESIGN: dsDrawBottom(T); break;
        case M_CONTENT: contentDrawBottom(T); break;
        case M_SLOTS: slotsDrawBottom(T); break;
        case M_CONTROLS: drawControlsBottom(T); break;
        }
        C3D_FrameEnd(0);
        photoAfterFrame();
    }
    if (mode == M_PLAY || mode == M_BUILD || mode == M_MENU || mode == M_PHONE) saveGame(G);
    audioExit();
    drawExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
    return 0;
}
