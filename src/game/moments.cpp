// Life Moments: little story events that pop up during the day. Each one asks
// you to pick how to react (2-3 choices) and the choice has real consequences.
#include "drama.hpp"
#include "goals.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "squads.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>

using namespace col;

struct MomentDef { const char* title; const char* text; Stage minSt, maxSt; uint8_t where; const char* opt[3]; };
// where: 0 anywhere, 1 home only, 2 out in town only
static const MomentDef MOMENTS[] = {
    {"Lost Puppy", "A tiny puppy with no collar is wandering around, looking up at you with huge eyes.", ST_CHILD, ST_ELDER, 2,
     {"Help find its owner", "Take it home!", "Keep walking"}},
    {"Found Wallet", "There's a sparkly wallet on the ground. Inside: $200 and a library card.", ST_CHILD, ST_ELDER, 2,
     {"Turn it in", "Keep the cash", "Post about it"}},
    {"Group Chat Drama", "{A} and {B} are fighting in the group chat, and BOTH want you on their side.", ST_PRETEEN, ST_ELDER, 0,
     {"Side with {A}", "Side with {B}", "Mute the chat"}},
    {"Fashion Emergency", "SNAP. The strap on your favorite shoe just broke, right before you head out.", ST_PRETEEN, ST_ELDER, 0,
     {"DIY fix it", "Buy new shoes ($60)", "Go barefoot & own it"}},
    {"Surprise Party", "{A} is throwing a surprise party tonight and wants you there!", ST_TEEN, ST_ELDER, 0,
     {"Go party!", "Help plan it", "Stay in"}},
    {"Paparazzi!", "A photographer just spotted you and is snapping away!", ST_TEEN, ST_ELDER, 2,
     {"Strike a pose", "Hide behind shades", "Tell them off"}},
    {"Bake Sale", "The school bake sale is tomorrow and you totally forgot to make something.", ST_CHILD, ST_TEEN, 1,
     {"Bake cookies", "Buy store cookies ($20)", "Skip it"}},
    {"Going Viral", "An old video of you dancing is suddenly going viral!", ST_TEEN, ST_ELDER, 0,
     {"Lean into it!", "Make a reply video", "Ask them to delete it"}},
    {"Secret Admirer", "A note in pink ink: \"I think you're cute. Meet me at Cafe Latte?\"", ST_YA, ST_ELDER, 0,
     {"Go meet them", "Show your friends", "Toss it"}},
    {"Free Samples", "A sales rep is handing out fancy perfume samples.", ST_TEEN, ST_ELDER, 2,
     {"Try one", "Grab a handful", "No thanks"}},
    {"Neighbor Needs Help", "{A} is struggling to move a giant couch up the stairs.", ST_TEEN, ST_ELDER, 0,
     {"Help lift it", "Cheer them on", "Too busy"}},
    {"Talent Scout", "A talent scout says you have \"the look\" and offers a quick modeling gig.", ST_TEEN, ST_ELDER, 0,
     {"Take the gig ($250)", "Negotiate for more", "Not interested"}},
    {"Midnight Snack", "Your stomach is growling and the fridge is calling your name...", ST_CHILD, ST_ELDER, 1,
     {"Make a fancy snack", "Raid the fridge", "Back to bed"}},
    {"Scratch Card", "You found a lucky scratch card in your purse!", ST_YA, ST_ELDER, 0,
     {"Scratch it!", "Give it to {A}"}},
    {"Bestie in Tears", "{A} just got dumped and is calling you, crying.", ST_TEEN, ST_ELDER, 0,
     {"Rush over with ice cream", "Talk on the phone", "Say you're busy"}},
    {"Rainy Day", "It's pouring outside. What's the plan?", ST_CHILD, ST_ELDER, 1,
     {"Movie marathon", "Dance in the rain", "Cozy nap"}},
    {"Pop Quiz Panic", "Rumor says there's a pop quiz tomorrow!", ST_CHILD, ST_TEEN, 0,
     {"Study all night", "Make flash cards with {A}", "Wing it"}},
    {"Mystery Package", "A pink box with a bow showed up at your door. No name on it.", ST_CHILD, ST_ELDER, 1,
     {"Open it!", "Leave it for later"}},
};
static const int NUM_MOMENTS = sizeof(MOMENTS) / sizeof(MOMENTS[0]);

static int cur = -1, a1 = -1, a2 = -1, phase = 0;   // phase 0: choose, 1: result
static std::string resultText;
static float shownT = 0;
// custom story popups (crime, blackmail, burglars...) share the same card
static const int CUSTOM = 1000;
static std::string cTitle, cText, cOpt[3];
static MomentFn cFn = nullptr;
static int cCtx = 0, cSim = -1;
void momentCustom(const std::string& title, const std::string& text, const char* o0, const char* o1, const char* o2, MomentFn fn, int ctx, int portraitSim) {
    cTitle = title; cText = text; cOpt[0] = o0 ? o0 : ""; cOpt[1] = o1 ? o1 : ""; cOpt[2] = o2 ? o2 : "";
    cFn = fn; cCtx = ctx; cSim = portraitSim;
    cur = CUSTOM; phase = 0; shownT = 0; a1 = portraitSim; a2 = -1;
    sfx(520, 880, 140);
}
static uint16_t lastDay = 0xFFFF;
static int todayCount = 0;

bool momentActive() { return cur >= 0; }

static std::string fill(const char* s) {
    std::string out = s;
    auto rep = [&](const char* key, int sim) {
        size_t p;
        while ((p = out.find(key)) != std::string::npos) out.replace(p, 3, sim >= 0 ? G.sims[sim].name : std::string("a friend"));
    };
    rep("{A}", a1); rep("{B}", a2);
    return out;
}
static int optCount(int m) { int n = 0; if (m == CUSTOM) { for (int i = 0; i < 3; i++) if (!cOpt[i].empty()) n++; return n; } for (int i = 0; i < 3; i++) if (MOMENTS[m].opt[i]) n++; return n; }

static void pickPeople() {
    std::vector<int> known, any;
    for (size_t i = 0; i < G.sims.size(); i++) {
        if ((int)i == G.mc || G.sims[i].household || G.sims[i].stage < ST_CHILD) continue;
        any.push_back((int)i);
        const Rel* r = findRel(G.mc, (int)i);
        if (r && r->met && r->fr >= 0) known.push_back((int)i);
    }
    const std::vector<int>& pool = known.size() >= 2 ? known : any;
    a1 = pool.empty() ? -1 : pool[irand((int)pool.size())];
    a2 = -1;
    if (pool.size() >= 2) for (int tries = 0; tries < 12 && (tries == 0 || a2 == a1); tries++) a2 = pool[irand((int)pool.size())];
    if (a2 == a1) a2 = -1;
}

static bool fits(int m) {
    const MomentDef& d = MOMENTS[m];
    const Sim& me = MC();
    if (me.stage < d.minSt || me.stage > d.maxSt) return false;
    bool home = roomIsHome(G.room);
    if (d.where == 1 && !home) return false;
    if (d.where == 2 && home) return false;
    if (m == 5 && fameStars() < 1) return false;               // paparazzi need a little fame
    if (m == 8 && partnerOf(G.mc) >= 0) return false;          // secret admirer: singles only
    if (m == 15 && G.weather != WX_RAIN && G.weather != WX_STORM) return false;
    if (m == 12 && (G.minute < 21 * 60 && G.minute > 5 * 60)) return false;   // midnight snack at night
    return true;
}

void momentMaybeStart() {
    if (cur >= 0 || G.sims.empty()) return;
    if (crimeMomentMaybe()) return;   // blackmailers & burglars get first dibs
    if (dramaMomentMaybe()) return;   // friendship & romance drama
    if (lastDay != G.day) { lastDay = G.day; todayCount = 0; }
    int h = (int)(G.minute / 60);
    bool night = h >= 21 || h < 2;
    if (todayCount >= 2 || (!night && (h < 9 || h > 21)) || irand(100) >= 9) return;
    std::vector<int> ok;
    for (int m = 0; m < NUM_MOMENTS; m++) if (fits(m)) ok.push_back(m);
    if (ok.empty()) return;
    cur = ok[irand((int)ok.size())];
    pickPeople();
    phase = 0; shownT = 0;
    todayCount++;
    G.momentsSeen++;
    sfx(660, 990, 120);
}

static void need(int n, float d) { Sim& me = MC(); me.needs[n] = clampv(me.needs[n] + d, 0.f, 100.f); }
static void resolve(int m, int c) {
    Sim& me = MC();
    char b[200];
    auto res = [&](const std::string& s) { resultText = s; };
    switch (m) {
    case 0:   // lost puppy
        if (c == 0) { G.rep = (int16_t)std::min(100, G.rep + 5); G.satisfaction += 80; addMoodlet(me, "Good Deed", EM_HAPPY, 2, 240);
                      if (irand(2)) { G.money += 50; res("You found the owner! They hugged you and insisted you take $50. (+Reputation)"); }
                      else res("After an hour of searching, the owner came running. Happy tears all around! (+Reputation)"); }
        else if (c == 1) {
            if (G.pets.size() < (size_t)NUM_PETS) {
                int p = (int)G.pets.size();
                G.pets.push_back((uint8_t)p);
                Actor pa; pa.kind = 1; pa.id = (uint16_t)p; pa.x = 120; pa.tx = 120; pa.y = 228;
                G.rooms[G.house][R_LIVING].actors.push_back(pa);
                res(std::string("Welcome home! You named your new ") + PET_KINDS[p] + " " + PET_NAMES[p] + ". They're waiting in the living room.");
                addMoodlet(me, "New Best Friend", EM_HAPPY, 2, 720);
            } else { res("Your home is already full of pets, so you take it to the shelter, where it gets adopted right away."); G.rep = (int16_t)std::min(100, G.rep + 2); }
        } else { addMoodlet(me, "Guilty Conscience", EM_TENSE, 1, 180); res("You keep walking... but those puppy eyes are going to haunt you."); }
        break;
    case 1:   // wallet
        if (c == 0) { G.rep = (int16_t)std::min(100, G.rep + 6); G.satisfaction += 100; addMoodlet(me, "Honest Diva", EM_CONFIDENT, 1, 240); res("The owner was SO grateful. Word gets around that you're a good one. (+Reputation)"); }
        else if (c == 1) { G.money += 200; G.rep = (int16_t)std::max(-100, G.rep - 8); addMoodlet(me, "Guilty", EM_TENSE, 1, 240); res("+$200... but someone saw you, and now people are whispering. (-Reputation)"); }
        else { uint32_t f = 50 + G.followers / 50; G.followers += f; snprintf(b, sizeof b, "Your post found the owner in an hour! +%u followers for being a hero.", (unsigned)f); res(b); G.rep = (int16_t)std::min(100, G.rep + 3); }
        break;
    case 2:   // group chat
        if (c == 0 && a1 >= 0) { changeRel(G.mc, a1, 10, 0); if (a2 >= 0) changeRel(G.mc, a2, -12, 0); res(fill("{A} sends you ten heart emojis. {B} leaves the chat. Yikes.")); }
        else if (c == 1 && a2 >= 0) { changeRel(G.mc, a2, 10, 0); if (a1 >= 0) changeRel(G.mc, a1, -12, 0); res(fill("{B} is thrilled. {A} is NOT. The drama continues.")); }
        else { addMoodlet(me, "Peace & Quiet", EM_FINE, 1, 120); need(N_FUN, 5); res("Notifications: off. Inner peace: on."); }
        break;
    case 3:   // fashion emergency
        if (c == 0) {
            skillGain(me, SK_STYLE, 3);
            if (frand() < 0.4f + skillLevel(me, SK_STYLE) * 0.08f) { addMoodlet(me, "Fixed It - Even Cuter", EM_CONFIDENT, 2, 240); res("A bow, some glue and a little glitter... honestly, they're cuter now."); }
            else { addMoodlet(me, "Wobbly Shoe", EM_TENSE, 1, 120); res("It's... holding. Mostly. Don't run."); }
        } else if (c == 1) { if (G.money >= 60) { G.money -= 60; addMoodlet(me, "New Shoes!", EM_HAPPY, 1, 240); res("New shoes = new you. Worth every penny."); } else res("Not enough money... you'll have to wing it."); }
        else {
            if (skillLevel(me, SK_CHARISMA) >= 3 || me.hasTrait(TR_GLAMOROUS)) { addMoodlet(me, "Trendsetter", EM_CONFIDENT, 2, 240); G.followers += 80; res("You strut barefoot like it's planned. People think it's a new trend!"); }
            else { addMoodlet(me, "Embarrassed", EM_TENSE, 1, 120); res("Ouch, ouch, hot pavement. Maybe not your best idea."); }
        }
        break;
    case 4:   // surprise party
        if (c == 0) { need(N_SOCIAL, 40); need(N_FUN, 30); need(N_ENERGY, -15); if (a1 >= 0) changeRel(G.mc, a1, 8, 0); addMoodlet(me, "Party Girl", EM_PLAYFUL, 2, 300); res("Best. Party. Ever. You danced until your feet hurt."); }
        else if (c == 1) { skillGain(me, SK_CREATIVITY, 3); if (a1 >= 0) changeRel(G.mc, a1, 12, 0); need(N_SOCIAL, 20); res(fill("Your decorations stole the show. {A} says you're the best friend ever.")); }
        else { need(N_ENERGY, 15); if (a1 >= 0) changeRel(G.mc, a1, -4, 0); res("Face mask, PJs and a rom-com. Self care!"); }
        break;
    case 5:   // paparazzi
        if (c == 0) { uint32_t f = 300 + G.followers / 20; G.followers += f; addMoodlet(me, "Camera Ready", EM_CONFIDENT, 2, 240); snprintf(b, sizeof b, "Your pose is everywhere! +%u followers.", (unsigned)f); res(b); }
        else if (c == 1) { addMoodlet(me, "Mysterious", EM_FINE, 1, 180); res("No comment. The mystery only makes them more curious."); }
        else { G.followers += 150; G.rep = (int16_t)std::max(-100, G.rep - 5); res("\"DIVA SNAPS AT PHOTOGRAPHER\" - drama sells. (+followers, -Reputation)"); }
        break;
    case 6:   // bake sale
        if (c == 0) { skillGain(me, SK_COOKING, 4); need(N_ENERGY, -10); addMoodlet(me, "Proud Baker", EM_HAPPY, 1, 240); G.satisfaction += 60; res("Your cookies sold out first! Everyone wants the recipe."); }
        else if (c == 1) { if (G.money >= 20) { G.money -= 20; res("Nobody can tell they're store-bought. Probably."); } else res("No money for cookies... you show up empty-handed."); }
        else { addMoodlet(me, "Forgot Again", EM_TENSE, 1, 180); res("Everyone else brought something. Awkward."); }
        break;
    case 7:   // viral
        if (c == 0) { uint32_t f = 800 + G.followers / 10; G.followers = (uint32_t)(G.followers + f * perkFollowerMult()); addMoodlet(me, "Internet Famous", EM_CONFIDENT, 2, 480); snprintf(b, sizeof b, "You're trending! +%u followers.", (unsigned)f); res(b); }
        else if (c == 1) { skillGain(me, SK_CREATIVITY, 3); uint32_t f = 500 + G.followers / 15; G.followers += f; snprintf(b, sizeof b, "Your reply video is even funnier. +%u followers!", (unsigned)f); res(b); }
        else { G.rep = (int16_t)std::min(100, G.rep + 2); res("They took it down. Your dignity remains intact."); }
        break;
    case 8: {  // secret admirer
        if (c == 0) {
            int best = -1;
            for (size_t i = 0; i < G.sims.size(); i++) if ((int)i != G.mc && !G.sims[i].household && canRomance(G.mc, (int)i) && partnerOf((int)i) < 0) { best = (int)i; if (irand(3) == 0) break; }
            if (best >= 0) { changeRel(G.mc, best, 10, 25); addMoodlet(me, "Butterflies", EM_FLIRTY, 2, 360); res("It was " + G.sims[best].name + "! You talk for hours over lattes. Sparks are flying."); }
            else res("Nobody showed up... but the latte was great.");
        } else if (c == 1) { need(N_SOCIAL, 25); need(N_FUN, 15); res("Your friends scream. Theories are being made. This is the best day."); }
        else res("Into the trash it goes. You're focused on YOU.");
        break;
    }
    case 9:   // samples
        if (c == 0) { addMoodlet(me, "Smelling Divine", EM_FLIRTY, 1, 240); res("You smell like a vanilla dream."); }
        else if (c == 1) { need(N_FUN, 10); G.rep = (int16_t)std::max(-100, G.rep - 2); res("The sales rep is giving you a LOOK. Worth it."); }
        else res("You glide past. Untouchable.");
        break;
    case 10:  // neighbor couch
        if (c == 0) { skillGain(me, SK_FITNESS, 4); need(N_ENERGY, -15); if (a1 >= 0) changeRel(G.mc, a1, 12, 0); G.satisfaction += 60; res(fill("Teamwork! {A} owes you big time.")); }
        else if (c == 1) { if (a1 >= 0) changeRel(G.mc, a1, 3, 0); res(fill("\"You got this!\" {A} gives you a tired thumbs-up.")); }
        else { if (a1 >= 0) changeRel(G.mc, a1, -3, 0); res("You slip away. The couch remains stuck."); }
        break;
    case 11:  // talent scout
        if (c == 0) { G.money += 250; skillGain(me, SK_STYLE, 4); need(N_ENERGY, -20); addMoodlet(me, "Model Moment", EM_CONFIDENT, 2, 300); res("+$250 and a photo shoot that went great!"); }
        else if (c == 1) { if (frand() < 0.45f + skillLevel(me, SK_CHARISMA) * 0.05f) { G.money += 500; res("You talked them up to $500. Business queen!"); } else res("They walked. Maybe next time ask a little nicer."); }
        else res("You're not here to be discovered. You're here to discover.");
        break;
    case 12:  // midnight snack
        if (c == 0) { skillGain(me, SK_COOKING, 2); need(N_HUNGER, 35); addMoodlet(me, "Midnight Chef", EM_HAPPY, 1, 180); res("A gourmet grilled cheese at 1 AM. Iconic."); }
        else if (c == 1) { need(N_HUNGER, 30); need(N_HYGIENE, -5); res("Cold pizza, straight from the box. No regrets."); }
        else { need(N_ENERGY, 10); res("Snack denied. Beauty sleep accepted."); }
        break;
    case 13:  // scratch card
        if (c == 0) { int r = irand(100); if (r < 12) { G.money += 500; addMoodlet(me, "Jackpot!", EM_HAPPY, 2, 480); res("$500!!! Shopping spree incoming!"); } else if (r < 50) { G.money += 20; res("You won $20. Hey, that's a latte!"); } else res("Nothing. Not even a free latte."); }
        else { if (a1 >= 0) changeRel(G.mc, a1, 6, 0); res(fill("{A} won $50 with it and bought you lunch!")); }
        break;
    case 14:  // bestie in tears
        if (c == 0) { if (a1 >= 0) changeRel(G.mc, a1, 15, 0); need(N_SOCIAL, 20); need(N_ENERGY, -10); addMoodlet(me, "Ride or Die", EM_HAPPY, 1, 240); res(fill("Two spoons, one pint, zero exes. {A} feels so much better.")); }
        else if (c == 1) { if (a1 >= 0) changeRel(G.mc, a1, 8, 0); need(N_SOCIAL, 10); res(fill("You stay on the phone until {A} is laughing again.")); }
        else { if (a1 >= 0) changeRel(G.mc, a1, -10, 0); res(fill("{A} hangs up. That stung.")); }
        break;
    case 15:  // rainy day
        if (c == 0) { need(N_FUN, 40); need(N_ENERGY, 10); res("Three rom-coms and a bowl of popcorn later, the rain doesn't matter."); }
        else if (c == 1) { need(N_FUN, 35); need(N_HYGIENE, -20); addMoodlet(me, "Singing in the Rain", EM_PLAYFUL, 2, 240); res("Soaked and SO happy."); }
        else { need(N_ENERGY, 40); res("The rain on the window is the best lullaby."); }
        break;
    case 16:  // pop quiz
        if (c == 0) { skillGain(me, SK_LOGIC, 4); need(N_ENERGY, -20); me.gradePts = std::min(100.f, me.gradePts + 4); res("You aced it! Totally worth the yawns."); }
        else if (c == 1) { skillGain(me, SK_LOGIC, 3); if (a1 >= 0) changeRel(G.mc, a1, 8, 0); me.gradePts = std::min(100.f, me.gradePts + 3); res(fill("Studying with {A} was actually fun. B+!")); }
        else { me.gradePts = std::max(0.f, me.gradePts - 4); addMoodlet(me, "Quiz Panic", EM_TENSE, 1, 180); res("...What's a hypotenuse?"); }
        break;
    case 17:  // mystery package
        if (c == 0) {
            int r = irand(4);
            if (r == 0) { int k = FIRST_AESTHETIC + irand(NUM_OUTFITS - FIRST_AESTHETIC); giveOutfit(k); res(std::string("A brand new outfit: ") + OUTFITS[k].name + "! It's in your closet."); }
            else if (r == 1) { G.money += 100; res("A card with $100 and a note: \"From your biggest fan.\""); }
            else if (r == 2) { G.satisfaction += 150; addMoodlet(me, "Spoiled", EM_HAPPY, 1, 240); res("Chocolates and a tiny tiara. Someone loves you! (+150 points)"); }
            else { addMoodlet(me, "Confetti Bomb", EM_PLAYFUL, 1, 120); need(N_HYGIENE, -10); res("A glitter bomb! You'll be finding sparkles for weeks."); }
        } else res("You set it by the door. Still curious though...");
        break;
    }
}

bool momentUpdate(const Input& in) {
    if (cur < 0) return false;
    shownT += in.dt;
    if (phase == 1) {
        if (shownT > 0.3f && ((in.down & KEY_A) || in.touch)) { cur = -1; sfxTap(); }
        return true;
    }
    int n = optCount(cur);
    for (int i = 0; i < n; i++) {
        Btn b{20, 60 + i * 50.f, 280, 42};
        static const u32 K[3] = {KEY_A, KEY_B, KEY_X};
        if ((in.touch && b.in(in.tp)) || (shownT > 0.3f && (in.down & K[i]))) {
            if (cur == CUSTOM) { resultText = cFn ? cFn(i, cCtx) : std::string(); if (resultText.empty()) { cur = -1; return true; } }
            else resolve(cur, i);
            phase = 1; shownT = 0; sfxYay();
            return true;
        }
    }
    return true;
}

void momentDrawTop(float t) {
    if (cur < 0) return;
    const bool custom = cur == CUSTOM;
    const char* title = custom ? cTitle.c_str() : MOMENTS[cur].title;
    rect(0, 0, TOP_W, TOP_H, C(0, 0, 0, 100));
    // Flash-game card with a rainbow frame
    static const RGB RB[6] = {{255, 46, 140}, {255, 168, 36}, {255, 220, 60}, {96, 206, 96}, {40, 192, 210}, {150, 96, 230}};
    rrect(22, 20, 356, 202, 20, C(UI_INK));
    for (int i = 0; i < 6; i++) rrect(24 + i * 0.6f, 22 + i * 0.6f, 352 - i * 1.2f, 198 - i * 1.2f, 18, C(RB[(i + (int)(t * 4)) % 6]));
    rrect(31, 29, 338, 184, 14, C(UI_INK));
    rrect(32.5f, 30.5f, 335, 181, 13, C(255, 252, 254));
    float tw = std::fmax(160.f, textW(phase == 0 ? title : "What happened...", 0.55f) + 40);
    flashPanel(200 - tw / 2, 9, tw, 26, RGB{255, 46, 140}, 11);
    textOutline(phase == 0 ? title : "What happened...", 200, 11, 0.55f, C(WHITE), C(UI_INK), 1, 1.0f);
    heart(54, 52, 7, C(HOT)); sparkle(346, 52, 6 + std::sin(t * 4) * 2, C(GOLD));
    std::string body = phase == 0 ? (custom ? cText : fill(MOMENTS[cur].text)) : resultText;
    auto lines = wrapText(body, 0.55f, 300);
    for (size_t i = 0; i < lines.size() && i < 6; i++) text(lines[i].c_str(), 200, 64 + i * 22.f, 0.55f, C(60, 40, 56), 1);
    if (a1 >= 0 && a1 < (int)G.sims.size() && phase == 0 && (custom || std::string(MOMENTS[cur].text).find("{A}") != std::string::npos)) { circle(62, 182, 28, C(UI_INK)); circle(62, 182, 26, C(255, 230, 240)); drawSimPortrait(G.sims[a1], 62, 182, 26, t); }
    if (phase == 1) { float p = 0.5f + 0.5f * std::sin(t * 6); circle(346, 194, 10 + p, C(245, 140, 40)); text("A", 346, 184, 0.55f, C(WHITE), 1); }
}

void momentDrawBottom(float t) {
    if (cur < 0) return;
    rect(0, 0, BOT_W, BOT_H, C(10, 40, 50, 170));
    if (phase == 1) { textOutline("Tap or press A to continue", 160, 110, 0.55f, C(WHITE), C(UI_INK), 1); return; }
    flashPanel(60, 12, 200, 28, RGB{255, 46, 140}, 12);
    textOutline("CHOOSE WHAT TO DO:", 160, 15, 0.55f, C(WHITE), C(UI_INK), 1);
    static const char* KEYS[3] = {"A", "B", "X"};
    static const int STY[3] = {0, 1, 2};
    for (int i = 0; i < optCount(cur); i++) {
        Btn b{20, 60 + i * 50.f, 280, 42};
        std::string lb = cur == CUSTOM ? cOpt[i] : fill(MOMENTS[cur].opt[i]);
        button(b, lb.c_str(), false, STY[i], 0.5f);
        circle(b.x + 18, b.y + b.h / 2, 12, C(UI_INK)); circle(b.x + 18, b.y + b.h / 2, 10.5f, C(255, 255, 255));
        textOutline(KEYS[i], b.x + 18, b.y + b.h / 2 - 9, 0.5f, C(255, 46, 140), C(UI_INK), 1, 0.6f);
    }
    (void)t;
}
