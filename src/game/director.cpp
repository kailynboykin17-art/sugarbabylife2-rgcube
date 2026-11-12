// Starlight Studios (see director.hpp). All scripts, stars and critics are original.
#include "director.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

using namespace col;
void toast(const std::string& m);
void fxBurst(float x, float y, int n);

// ---------------------------------------------------------------- data
enum Tag : uint8_t { TG_COMEDY = 1, TG_DRAMA = 2, TG_ROMANCE = 4, TG_VILLAIN = 8, TG_ACTION = 16, TG_TEEN = 32, TG_DIVA = 64 };
static const char* tagName(int t) { return t == TG_COMEDY ? "Comedy" : t == TG_DRAMA ? "Drama" : t == TG_ROMANCE ? "Romance" : t == TG_VILLAIN ? "Villain" : t == TG_ACTION ? "Action" : t == TG_TEEN ? "Teen Lead" : "Diva"; }
static std::string tagList(int m) { std::string s; for (int b = 1; b <= 64; b <<= 1) if (m & b) { if (!s.empty()) s += "/"; s += std::string(tagName(b)).substr(0, 4); } return s; }

struct Star { const char* name; uint8_t tags; bool guy; uint8_t skin, hair, hairCol, outfit; };
static const Star STARS[] = {
    {"Brielle Monroe", TG_ROMANCE | TG_DRAMA, false, 1, 50, 3, 13}, {"Jaxon Steele", TG_ACTION | TG_ROMANCE, true, 4, 47, 1, 18},
    {"Kiki Valentine", TG_COMEDY | TG_TEEN, false, 6, 41, 6, 9}, {"Desiree Vaughn", TG_VILLAIN | TG_DRAMA | TG_DIVA, false, 3, 52, 0, 3},
    {"Tyler Brooks", TG_TEEN | TG_COMEDY, true, 2, 44, 2, 20}, {"Marisol Reyes", TG_DRAMA | TG_ACTION, false, 5, 58, 1, 12},
    {"Coco St. James", TG_DIVA | TG_COMEDY, false, 8, 37, 0, 10}, {"Devon Hart", TG_VILLAIN | TG_ROMANCE, true, 7, 45, 0, 27},
    {"Skye Lennox", TG_TEEN | TG_ROMANCE, false, 0, 61, 3, 11}, {"Aaliyah Sinclair", TG_ACTION | TG_DIVA, false, 9, 66, 0, 16},
};
static const int NSTARS = sizeof(STARS) / sizeof(STARS[0]);   // index NSTARS = you

enum Loc { L_HALLWAY, L_PROM, L_CULDESAC, L_BEACH, L_MANSION, L_GARAGE, L_ROOFTOP, L_MALL, L_COUNT };
static const char* LOC_NAMES[L_COUNT] = {"School Hallway", "Prom Ballroom", "Suburban Cul-de-sac", "Sunset Beach", "Mansion Gala", "Parking Garage", "City Rooftop", "Mall Food Court"};

struct Theme { const char* name; const char* looks[8]; };
static const Theme THEMES[] = {
    {"Y2K Glam", {"Y2K Butterfly", "McBling Princess", "Pink Mini", "Velour Tracksuit", "Camo Motel Era", "Baddie", "Vaporwave", nullptr}},
    {"Preppy Queen Bee", {"Preppy", "Old Money", "Coquette", "Cheer Squad", "Light Academia", "Clean Girl", "Soft Girl", nullptr}},
    {"Dark & Dangerous", {"Scene Goth", "Punk", "Witchy", "Rockstar GF", "Midnight Stealth", "Leopard Glam", "Emo Kid", "Dark Academia"}},
    {"Beach Babe", {"Beach Babe", "Beach Day", "Coconut Girl", "Costa Girl", "VSCO Girl", "Boho", nullptr, nullptr}},
    {"Suburban Perfect", {"Office Boss", "Cozy Knit", "Sunday Best", "Minimalist", "Old Money", "Stockholm Style", "Clean Girl", nullptr}},
    {"Red Carpet", {"Red Carpet Gown", "Prom Night", "Royalcore", "Princess", "Siren", "Zebra Night Out", nullptr, nullptr}},
    {"Street Style", {"Streetwear", "Fresh Kicks", "Tracksuit Drip", "Skater Kid", "Cargo Cap Cutie", "Sporty", "Geek of the Week", "Skater Teen '03"}},
    {"Cozy Romance", {"Cozy Knit", "Cottagecore", "Shabby Chic", "Soft Girl", "Boho Sundress", "Cocoa Peacoat", "Coquette", nullptr}},
};
enum { TH_Y2K, TH_PREP, TH_DARK, TH_BEACH, TH_SUBURB, TH_CARPET, TH_STREET, TH_COZY };

struct Choice { const char* prompt; const char* opt[3]; uint8_t best, ok; };
struct Script {
    const char* title; const char* era; const char* logline;
    const char* role[3]; uint8_t need[3]; uint8_t nroles;
    uint8_t theme, loc, okLoc;
    Choice ch[2];
};
static const Script SCRIPTS[] = {
    {"Queen Bees of Westbrook", "2000s Teen Comedy", "A new girl sneaks into Westbrook High's most exclusive clique... and starts to become just like them.",
     {"The New Girl", "The Queen Bee", "The Funny Best Friend"}, {TG_TEEN, TG_VILLAIN, TG_COMEDY}, 3, TH_PREP, L_HALLWAY, L_MALL,
     {{"At the big party, the New Girl...", {"Spills the Queen Bee's secret", "Stays loyal to her real friends", "Leaves early"}, 1, 0},
      {"The finale at the Spring Fling:", {"She shares the crown with everyone", "She keeps the crown", "A food fight breaks out"}, 0, 2}}},
    {"My Secret Pop Star Life", "2000s Teen Musical", "By day she's just a regular sophomore. By night, she's the biggest pop star in the country. Nobody can know.",
     {"The Pop Star", "The Best Friend", "The Crush"}, {TG_DIVA, TG_COMEDY, TG_ROMANCE}, 3, TH_Y2K, L_ROOFTOP, L_HALLWAY,
     {{"The crush almost sees her without the wig! She...", {"Hides in a locker", "Tells him the truth", "Blames her twin"}, 1, 2},
      {"Big concert ending:", {"She sings a song about her real life", "She retires from music", "She cancels the show"}, 0, 1}}},
    {"Switched at Sweet Sixteen", "2000s Body-Swap Comedy", "A mom and her teen daughter wake up in each other's bodies - the day before the Sweet Sixteen.",
     {"The Daughter", "The Mom"}, {TG_TEEN, TG_COMEDY}, 2, TH_STREET, L_CULDESAC, L_MALL,
     {{"Mom (in her daughter's body) goes to school and...", {"Joins the band and rocks out", "Hides in the library", "Fails the pop quiz"}, 0, 1},
      {"How do they switch back?", {"They finally say 'I'm sorry'", "A magic fortune cookie", "They never switch back"}, 0, 1}}},
    {"The Nanny Who Knew Too Much", "Made-for-TV Thriller", "The perfect nanny shows up out of nowhere. She's charming, she's helpful... and she's hiding something.",
     {"The Busy Mom", "The Nanny", "The Detective"}, {TG_DRAMA, TG_VILLAIN, TG_ACTION}, 3, TH_SUBURB, L_CULDESAC, L_MANSION,
     {{"Mom finds an old photo in the nanny's bag. She...", {"Calls the detective", "Confronts the nanny alone", "Ignores it"}, 0, 1},
      {"The twist ending:", {"The nanny was protecting the family", "The nanny gets away", "It was all a dream"}, 0, 1}}},
    {"My Husband's Secret Wife", "Made-for-TV Thriller", "Her husband travels for work every other week. Turns out he's got a whole other life two towns over.",
     {"The Wife", "The Husband", "The Bestie"}, {TG_DRAMA, TG_VILLAIN, TG_COMEDY}, 3, TH_SUBURB, L_MANSION, L_CULDESAC,
     {{"She finds a second wedding ring. She...", {"Follows him on his 'work trip'", "Asks him straight up", "Throws it in the pool"}, 0, 1},
      {"The confrontation at the gala:", {"Both wives team up against him", "She forgives him", "She faints dramatically"}, 0, 2}}},
    {"My Prom Date Is Watching", "Made-for-TV Teen Thriller", "The new boy asked her to prom with a dozen roses. Now the roses keep showing up... everywhere.",
     {"The Prom Queen", "The Prom Date"}, {TG_TEEN, TG_VILLAIN}, 2, TH_CARPET, L_PROM, L_HALLWAY,
     {{"A note in her locker: 'Save me a dance.' She...", {"Tells her parents", "Goes to prom anyway, with friends", "Stays home"}, 1, 0},
      {"At prom, the lights go out...", {"Her friends save the night", "She chases him alone", "It's a surprise party"}, 0, 2}}},
    {"The Perfect Stepmother", "Made-for-TV Thriller", "Dad's new wife is perfect - too perfect. And only his daughter can see through her.",
     {"The Daughter", "The Stepmother", "The Dad"}, {TG_TEEN, TG_VILLAIN, TG_ROMANCE}, 3, TH_SUBURB, L_MANSION, L_CULDESAC,
     {{"The daughter finds the stepmom's fake ID. She...", {"Records her and shows Dad", "Runs away", "Posts it online"}, 0, 2},
      {"The final showdown:", {"Dad finally believes her", "The stepmom moves away", "They become besties"}, 0, 1}}},
    {"Step It Up: Crew Battle", "2010s Dance Movie", "A ballet girl from uptown and a street crew from downtown have one shot to win the city's biggest dance battle.",
     {"The Dancer", "The Rival", "The Love Interest"}, {TG_ACTION, TG_VILLAIN, TG_ROMANCE}, 3, TH_STREET, L_GARAGE, L_ROOFTOP,
     {{"The rival crew trashes their practice space. They...", {"Dance in the rain anyway", "Quit", "Call the cops"}, 0, 2},
      {"The final battle:", {"Ballet + hip-hop mashup routine", "Copy the rival's moves", "Skip the battle"}, 0, 1}}},
    {"Summer at Sunset Beach", "2010s Summer Romance", "She came to her aunt's beach town for a boring summer. Then she met the surfer who works at the snack shack.",
     {"The Girl", "The Surfer", "The Bestie"}, {TG_ROMANCE, TG_ROMANCE, TG_COMEDY}, 3, TH_BEACH, L_BEACH, L_ROOFTOP,
     {{"Their first date:", {"A bonfire under the stars", "Mini golf", "Watching TV"}, 0, 1},
      {"End of summer:", {"A big goodbye kiss on the pier", "They promise to text", "They never see each other again"}, 0, 1}}},
    {"The Kissing Contest", "2010s Teen Rom-Com", "Two best friends run the carnival kissing booth for charity... but she's secretly in love with his older brother.",
     {"The Lead", "The Best Friend", "The Crush"}, {TG_TEEN, TG_COMEDY, TG_ROMANCE}, 3, TH_PREP, L_HALLWAY, L_MALL,
     {{"The crush shows up at the booth! She...", {"Gets super flustered", "Plays it cool", "Runs away"}, 0, 1},
      {"The ending:", {"She picks herself first", "She picks the crush", "She picks the best friend"}, 0, 1}}},
    {"Main Character Energy", "2020s Teen Comedy", "An influencer loses all her followers overnight and has to survive high school as a nobody.",
     {"The Influencer", "The Real Friend", "The Hater"}, {TG_DIVA, TG_COMEDY, TG_VILLAIN}, 3, TH_Y2K, L_MALL, L_HALLWAY,
     {{"Her phone breaks. She...", {"Hangs out with real people for once", "Borrows her mom's flip phone", "Cries"}, 0, 1},
      {"Going viral again:", {"A video of her being genuinely kind", "A big drama callout", "She deletes social media"}, 0, 2}}},
    {"Left on Read", "2020s Teen Romance", "She writes love notes she never sends. One day, her little sister sends ALL of them.",
     {"The Lead", "The Crush", "The Little Sister"}, {TG_ROMANCE, TG_ROMANCE, TG_COMEDY}, 3, TH_COZY, L_ROOFTOP, L_HALLWAY,
     {{"The crush replies 'is this real??' She...", {"Says yes, it's real", "Says it was a prank", "Leaves him on read"}, 0, 2},
      {"Big romantic moment:", {"A sunset on the rooftop", "A text that says 'hey'", "Nothing happens"}, 0, 1}}},
    {"Dark Academy", "2020s Teen Mystery", "Everyone at Ravenwood Prep has a secret. When the class president vanishes, the new scholarship girl starts digging.",
     {"The Detective", "The Secret Society Leader", "The Sidekick"}, {TG_DRAMA, TG_VILLAIN, TG_COMEDY}, 3, TH_DARK, L_MANSION, L_GARAGE,
     {{"She finds a hidden door in the library. She...", {"Brings her sidekick and goes in", "Goes in alone at midnight", "Tells the principal"}, 0, 1},
      {"The secret society's big reveal:", {"The president faked it to expose them", "Ghosts did it", "Nobody ever finds out"}, 0, 2}}},
};
static const int NSCRIPTS = sizeof(SCRIPTS) / sizeof(SCRIPTS[0]);
static const char* CRITICS[3] = {"Roxanne Reels", "The Popcorn Princess", "@TeenScreenTea"};

// ---------------------------------------------------------------- state
enum Phase { PH_LOBBY, PH_SCRIPTS, PH_CAST, PH_STYLE, PH_LOC, PH_SCENE, PH_POSTER, PH_REVIEW, PH_JOBS, PH_JOB, PH_SHOP, PH_SHOPOPEN };
static int phase = PH_LOBBY, pick = 0, role = 0, scene = 0, jobKind = 0, shopPick = -1, shopWin = -1;
static int cast[3], look[3], loc = -1, choice[2];
static float pt = 0, meter = 0, posterBonus = 0;
static int score = 0, result = 0, payMoney = 0, payFans = 0;
static std::string critic[3];
static std::vector<int> owned;

static bool unlockedScript(int i) { return i < 3 + G.movie.made; }
static int totalStars() { int n = 0; for (int i = 0; i < 16; i++) n += G.movie.stars[i]; return n; }
static int outfitIdx(const char* name) { if (!name) return -1; for (int k = 0; k < NUM_OUTFITS; k++) if (!strcmp(OUTFITS[k].name, name)) return k; return -1; }
static bool onTheme(int th, int outfit) { for (int i = 0; i < 8; i++) if (THEMES[th].looks[i] && outfitIdx(THEMES[th].looks[i]) == outfit) return true; return false; }
static int youTags() {
    const Sim& me = MC(); int m = 0;
    if (me.hasTrait(TR_GOOFBALL)) m |= TG_COMEDY; if (me.hasTrait(TR_ROMANTIC)) m |= TG_ROMANCE; if (me.hasTrait(TR_MEAN) || me.hasTrait(TR_HOTHEAD) || me.hasTrait(TR_SNEAKY)) m |= TG_VILLAIN;
    if (me.hasTrait(TR_ACTIVE)) m |= TG_ACTION; if (me.hasTrait(TR_GLAMOROUS) || me.hasTrait(TR_FASHIONISTA)) m |= TG_DIVA; if (me.hasTrait(TR_CREATIVE) || me.hasTrait(TR_BOOKWORM)) m |= TG_DRAMA;
    if (me.stage <= ST_TEEN) m |= TG_TEEN;
    return m ? m : TG_DRAMA;
}
static int starTags(int s) { return s == NSTARS ? youTags() : STARS[s].tags; }
static const std::string starName(int s) { return s == NSTARS ? MC().name + " (You!)" : STARS[s].name; }
static Sim starSim(int s, int outfit) {
    Sim x = MC();
    if (s < NSTARS) {
        const Star& st = STARS[s];
        x.look.guy = st.guy; x.look.skin = st.skin; x.look.hairStyle = (uint8_t)(st.hair % NUM_HAIRSTYLES); x.look.hairCol = st.hairCol;
        x.look.f = FaceDef(); x.look.f.shadow = st.guy ? 0 : 1 + s % 5; x.look.f.liner = st.guy ? 0 : 1; x.look.f.eyeCol = (uint8_t)(s % 6); x.look.lips = st.guy ? RGB{190, 120, 110} : RGB{220, 80, 130};
        x.look.hat = HT_NONE; x.look.acc = 0; x.stage = ST_YA;
    }
    x.outfit = (uint8_t)(outfit >= 0 ? outfit : s < NSTARS ? STARS[s].outfit : MC().outfit);
    return x;
}
static void refreshOwned() { owned.clear(); for (int k = 1; k < NUM_OUTFITS; k++) if (ownsOutfit(k)) owned.push_back(k); if (owned.empty()) owned.push_back(0); }

void directorOpen() { phase = PH_LOBBY; pt = 0; refreshOwned(); }

// ---------------------------------------------------------------- scoring
static void scoreMovie() {
    const Script& S = SCRIPTS[pick];
    float castPts = 0, stylePts = 0, locPts = 0, chPts = 0;
    for (int r = 0; r < S.nroles; r++) { if (starTags(cast[r]) & S.need[r]) castPts += 1; if (onTheme(S.theme, look[r])) stylePts += 1; }
    castPts = castPts / S.nroles * 36; stylePts = stylePts / S.nroles * 24;
    locPts = loc == S.loc ? 16 : loc == S.okLoc ? 8 : 0;
    for (int c = 0; c < 2; c++) chPts += choice[c] == S.ch[c].best ? 12 : choice[c] == S.ch[c].ok ? 6 : 0;
    score = (int)std::min(100.f, castPts + stylePts + locPts + chPts + posterBonus);
    result = score >= 85 ? 3 : score >= 60 ? 2 : score >= 35 ? 1 : 0;
    // the critics focus on your weakest area
    const char* castQ = castPts >= 30 ? "The casting? Perfection. Every role fits like a glove." : castPts >= 18 ? "A couple of the actors felt miscast." : "Who cast these people?! Nobody fit their role.";
    const char* styleQ = stylePts >= 20 ? "The costumes are SO on theme - iconic looks!" : stylePts >= 10 ? "Some outfits worked, some were way off-theme." : "The wardrobe had nothing to do with the movie.";
    const char* storyQ = chPts + locPts >= 36 ? "The story and setting? Chef's kiss. I gasped at the ending!" : chPts + locPts >= 20 ? "The plot was fine, but that ending..." : "Wrong place, weird choices. I walked out.";
    critic[0] = castQ; critic[1] = styleQ; critic[2] = storyQ;
    payMoney = 100 + result * 220 + G.movie.made * 10; payFans = 150 + result * 400;
    G.money += payMoney; G.followers += payFans;
    if (result > G.movie.stars[pick]) G.movie.stars[pick] = (uint8_t)result;
    G.movie.made++;
    Sim& me = MC();
    addMoodlet(me, result >= 3 ? "Box Office Smash" : result >= 2 ? "Hit Movie" : result ? "Mixed Reviews" : "Total Flop", result ? EM_CONFIDENT : EM_SAD, result >= 2 ? 2 : 1, 480);
    skillGain(me, SK_CREATIVITY, 3 + result * 2.f); skillGain(me, SK_STYLE, 2.f);
    if (result >= 3) fxBurst(200, 100, 30);
}

// ---------------------------------------------------------------- input
static Btn listB(int i) { return Btn{6, 24 + i * 25.f, 308, 23}; }
static Btn gridB(int i) { return Btn{4 + (i % 2) * 158.f, 24 + (i / 2) * 30.f, 154, 27}; }
static const Btn bBack{236, 210, 80, 26}, bNext{120, 210, 110, 26}, bPrev{4, 210, 40, 26}, bFwd{48, 210, 40, 26};
static const Btn bLeft{20, 100, 60, 50}, bRight{240, 100, 60, 50};
static int listScroll = 0;
static const char* JOB_NAMES[4] = {"Shoot a Commercial", "Fan Convention", "Dinner with a Celeb", "Walk the Red Carpet"};
static const int JOB_PAY[4] = {90, 60, 40, 30}, JOB_FANS[4] = {80, 250, 120, 450};

bool directorUpdate(const Input& in) {
    pt += in.dt;
    const touchPosition& tp = in.tp;
    const bool tap = in.touch;
    if (phase == PH_POSTER || phase == PH_JOB) meter = 0.5f + 0.5f * std::sin(pt * 2.6f);
    if (in.down & KEY_B) {
        if (phase == PH_LOBBY) return false;
        phase = PH_LOBBY; sfxTap(); return true;
    }
    if (!tap) return true;
    switch (phase) {
    case PH_LOBBY:
        for (int i = 0; i < 4; i++) if (gridB(i + 2).in(tp)) {
            sfxTap(); pt = 0;
            if (i == 0) { phase = PH_SCRIPTS; listScroll = 0; }
            else if (i == 1) phase = PH_JOBS;
            else if (i == 2) { phase = PH_SHOP; shopPick = -1; }
            else return false;
            return true;
        }
        break;
    case PH_SCRIPTS:
        for (int i = 0; i < 7; i++) { int s = listScroll + i; if (s < NSCRIPTS && listB(i).in(tp)) { if (!unlockedScript(s)) { toast("Make more movies to unlock this script!"); sfxNo(); } else { pick = s; sfxTap(); } return true; } }
        if (bPrev.in(tp)) { listScroll = std::max(0, listScroll - 7); sfxTap(); }
        if (bFwd.in(tp)) { listScroll = std::min(NSCRIPTS - 1, listScroll + 7); sfxTap(); }
        if (bNext.in(tp) && unlockedScript(pick)) { role = 0; for (int r = 0; r < 3; r++) { cast[r] = -1; look[r] = owned[0]; } loc = -1; scene = 0; posterBonus = 0; phase = PH_CAST; listScroll = 0; sfxYay(); }
        if (bBack.in(tp)) { phase = PH_LOBBY; sfxTap(); }
        break;
    case PH_CAST:
        for (int i = 0; i < 7; i++) {
            int s = listScroll + i; if (s > NSTARS || !listB(i).in(tp)) continue;
            bool taken = false; for (int r = 0; r < role; r++) if (cast[r] == s) taken = true;
            if (taken) { toast("Already cast in this movie!"); sfxNo(); return true; }
            cast[role] = s; sfxTap();
            if (++role >= SCRIPTS[pick].nroles) { role = 0; phase = PH_STYLE; refreshOwned(); for (int r = 0; r < 3; r++) look[r] = cast[r] >= 0 && cast[r] < NSTARS ? STARS[cast[r]].outfit : MC().outfit; }
            return true;
        }
        if (bPrev.in(tp)) { listScroll = 0; sfxTap(); }
        if (bFwd.in(tp)) { listScroll = 7; sfxTap(); }
        if (bBack.in(tp)) { phase = PH_SCRIPTS; sfxTap(); }
        break;
    case PH_STYLE: {
        auto cyc = [&](int d) { int cur = 0; for (size_t i = 0; i < owned.size(); i++) if (owned[i] == look[role]) cur = (int)i; cur = (cur + d + (int)owned.size()) % (int)owned.size(); look[role] = owned[cur]; sfxTap(); };
        if (bLeft.in(tp)) cyc(-1);
        if (bRight.in(tp)) cyc(1);
        if (bNext.in(tp)) { sfxTap(); if (++role >= SCRIPTS[pick].nroles) { phase = PH_LOC; role = 0; } }
        if (bBack.in(tp)) { if (role > 0) role--; else phase = PH_CAST; sfxTap(); }
        break;
    }
    case PH_LOC:
        for (int i = 0; i < L_COUNT; i++) if (gridB(i).in(tp)) { loc = i; sfxTap(); return true; }
        if (bNext.in(tp) && loc >= 0) { phase = PH_SCENE; scene = 0; pt = 0; sfxYay(); }
        if (bBack.in(tp)) { phase = PH_STYLE; role = SCRIPTS[pick].nroles - 1; sfxTap(); }
        break;
    case PH_SCENE:
        for (int i = 0; i < 3; i++) if (gridB(i * 2).in(tp) || Btn{6, 60 + i * 44.f, 308, 40}.in(tp)) {
            choice[scene] = i; sfx(600, 900, 60);
            if (++scene >= 2) { phase = PH_POSTER; pt = 0; }
            return true;
        }
        break;
    case PH_POSTER:
        if (Btn{80, 100, 160, 60}.in(tp)) {
            float d = std::fabs(meter - 0.5f);
            posterBonus = d < 0.08f ? 6 : d < 0.2f ? 3 : 0;
            toast(posterBonus >= 6 ? "PERFECT poster shot!" : posterBonus ? "Nice poster!" : "Blurry... but it'll do.");
            scoreMovie(); phase = PH_REVIEW; pt = 0; result ? sfxYay() : sfxNo();
        }
        break;
    case PH_REVIEW:
        if (pt > 0.6f && bNext.in(tp)) { phase = PH_LOBBY; sfxTap(); }
        break;
    case PH_JOBS:
        for (int i = 0; i < 4; i++) if (listB(i + 1).in(tp)) {
            if (G.movie.jobDay[i] == G.day) { toast("Already did that today! Come back tomorrow."); sfxNo(); return true; }
            jobKind = i; phase = PH_JOB; pt = 0; sfxTap(); return true;
        }
        if (bBack.in(tp)) { phase = PH_LOBBY; sfxTap(); }
        break;
    case PH_JOB:
        if (Btn{80, 100, 160, 60}.in(tp)) {
            float d = std::fabs(meter - 0.5f); float k = d < 0.08f ? 2.f : d < 0.2f ? 1.4f : 1.f;
            int m = (int)(JOB_PAY[jobKind] * k), f = (int)(JOB_FANS[jobKind] * k);
            G.money += m; G.followers += f; G.movie.jobDay[jobKind] = G.day;
            char b[96]; snprintf(b, sizeof b, "%s: +$%d, +%d fans!%s", JOB_NAMES[jobKind], m, f, k >= 2 ? " Nailed it!" : "");
            toast(b); sfxYay(); addMoodlet(MC(), "Celeb Life", EM_CONFIDENT, 1, 240);
            phase = PH_JOBS;
        }
        break;
    case PH_SHOP:
        for (int i = 0; i < 3; i++) if (Btn{20 + i * 100.f, 60, 80, 90}.in(tp)) {
            if (G.money < 40) { toast("Each box costs $40!"); sfxNo(); return true; }
            G.money -= 40; shopPick = i;
            int jackpot = (G.day + G.movie.made + (int)(pt * 10)) % 3;
            int got = 0; shopWin = -1;
            for (int tries = 0; tries < 60 && got < (i == jackpot ? 2 : 1); tries++) { int k = 9 + irand(FIRST_KID_OUTFIT - 9); if (!ownsOutfit(k)) { giveOutfit(k); got++; shopWin = k; } }
            if (shopWin < 0) { G.money += 40; toast("You own every look already! Refunded."); shopPick = -1; return true; }
            toast(i == jackpot ? std::string("JACKPOT! 2 new looks, including ") + OUTFITS[shopWin].name + "!" : std::string("You won: ") + OUTFITS[shopWin].name + "!");
            phase = PH_SHOPOPEN; pt = 0; refreshOwned(); sfxYay();
            return true;
        }
        if (bBack.in(tp)) { phase = PH_LOBBY; sfxTap(); }
        break;
    case PH_SHOPOPEN:
        if (pt > 0.5f) { phase = PH_SHOP; shopPick = -1; }
        break;
    }
    return true;
}

// ---------------------------------------------------------------- drawing
static void drawLoc(int l, float t) {
    switch (l) {
    case L_HALLWAY: grad(0, 0, 400, 240, C(250, 236, 214), C(230, 210, 190)); for (int i = 0; i < 9; i++) { rrect(10 + i * 44.f, 60, 38, 110, 3, C(i % 2 ? RGB{120, 160, 220} : RGB{230, 90, 120})); rect(34 + i * 44.f, 100, 4, 14, C(200, 200, 210)); } rect(0, 170, 400, 70, C(214, 200, 180)); break;
    case L_PROM: grad(0, 0, 400, 240, C(60, 30, 90), C(120, 50, 140)); for (int i = 0; i < 20; i++) sparkle(std::fmod(i * 47.f + t * 8, 400.f), 20 + (i * 37) % 120, 3, C(255, 240, 200)); circle(200, 30, 18, C(220, 220, 240)); rect(0, 170, 400, 70, C(40, 20, 60)); break;
    case L_CULDESAC: grad(0, 0, 400, 170, C(140, 210, 250), C(220, 240, 255)); for (int i = 0; i < 3; i++) { float x = 30 + i * 130.f; tri(x, 90, x + 50, 50, x + 100, 90, C(170, 90, 90)); rect(x + 8, 90, 84, 70, C(i == 1 ? RGB{250, 240, 220} : RGB{230, 220, 240})); rect(x + 40, 120, 18, 40, C(120, 80, 60)); } rect(0, 160, 400, 80, C(120, 190, 100)); rect(0, 200, 400, 40, C(150, 150, 160)); break;
    case L_BEACH: grad(0, 0, 400, 140, C(255, 180, 140), C(255, 220, 180)); circle(320, 110, 30, C(255, 240, 160)); rect(0, 120, 400, 50, C(90, 160, 220)); rect(0, 165, 400, 75, C(246, 222, 170)); break;
    case L_MANSION: grad(0, 0, 400, 240, C(40, 36, 60), C(90, 70, 110)); for (int i = 0; i < 5; i++) { rect(30 + i * 80.f, 30, 16, 140, C(230, 220, 200)); } for (int i = 0; i < 9; i++) circle(200 - 80 + i * 20.f, 30, 3, C(255, 240, 180)); rect(0, 170, 400, 70, C(160, 30, 50)); break;
    case L_GARAGE: grad(0, 0, 400, 240, C(40, 44, 54), C(70, 74, 84)); for (int i = 0; i < 4; i++) rect(20 + i * 100.f, 0, 30, 170, C(90, 94, 104)); for (int i = 0; i < 6; i++) rect(i * 70.f, 200, 40, 4, C(240, 220, 80)); rect(0, 170, 400, 70, C(55, 58, 66)); break;
    case L_ROOFTOP: grad(0, 0, 400, 170, C(40, 30, 90), C(250, 120, 140)); for (int i = 0; i < 12; i++) { float h = 40 + (i * 37) % 70; rect(i * 34.f, 170 - h, 30, h, C(30, 24, 50)); for (int w = 0; w < 4; w++) rect(i * 34.f + 6 + (w % 2) * 12, 176 - h + (w / 2) * 14, 5, 6, C(255, 220, 120)); } rect(0, 170, 400, 70, C(90, 80, 100)); break;
    case L_MALL: grad(0, 0, 400, 240, C(255, 236, 244), C(240, 214, 230)); for (int i = 0; i < 4; i++) { rrect(14 + i * 98.f, 40, 86, 60, 6, C(255, 255, 255)); rect(14 + i * 98.f, 40, 86, 12, C(i % 2 ? RGB{255, 120, 170} : RGB{120, 200, 220})); } rect(0, 170, 400, 70, C(230, 220, 230)); break;
    }
}
static void drawCast(float t, float sc, int highlight = -1) {
    const Script& S = SCRIPTS[pick];
    bool lay = artLayerBegin(1, g_artStyle);
    for (int r = 0; r < S.nroles; r++) {
        if (cast[r] < 0) continue;
        float x = S.nroles == 2 ? 140 + r * 120.f : 100 + r * 100.f;
        Sim s = starSim(cast[r], look[r]);
        drawSim(s, x, 228, sc * 2.1f, t, 0, r == highlight ? PO_WAVE : PO_IDLE, r == 2);
    }
    if (lay) artLayerEnd(0, 0, 400, 240);
}
static void stars3(float x, float y, int n, float sz) { for (int i = 0; i < 3; i++) sparkle(x + (i - 1) * sz * 2.4f, y, sz, i < n ? C(GOLD) : C(UI1, 70)); }
static void header(const char* s) { rect(0, 0, 400, 22, C(UI1, 220)); text(s, 200, 3, 0.5f, C(CREAM), 1); }

void directorDrawTop(float t) {
    const Script& S = SCRIPTS[pick];
    switch (phase) {
    case PH_LOBBY: case PH_JOBS: case PH_JOB: case PH_SHOP: case PH_SHOPOPEN: {
        grad(0, 0, 400, 240, C(30, 20, 50), C(90, 40, 100));
        for (int i = 0; i < 6; i++) { float a = std::sin(t * 0.8f + i) * 0.4f; line(60 + i * 56.f, 240, 60 + i * 56.f + std::sin(a) * 200, 0, 6, C(255, 240, 180, 40)); }
        rrect(70, 24, 260, 56, 10, C(UI1)); rrect(74, 28, 252, 48, 8, C(255, 220, 120));
        textOutline("STARLIGHT STUDIOS", 200, 34, 0.75f, C(255, 255, 255), C(200, 40, 100), 1, 1.5f);
        char b[96]; snprintf(b, sizeof b, "Movies made: %d    Total stars: %d    Fans: %u", G.movie.made, totalStars(), (unsigned)G.followers);
        text(b, 200, 90, 0.42f, C(255, 230, 240), 1);
        if (phase == PH_JOB) { text(JOB_NAMES[jobKind], 200, 120, 0.6f, C(255, 255, 255), 1); rrect(80, 150, 240, 16, 6, C(UI1)); rect(82 + 236 * 0.42f, 152, 236 * 0.16f, 12, C(GOLD)); rect(82 + 236 * meter - 2, 146, 4, 24, C(HOT)); text("Strike a pose in the GOLD zone!", 200, 176, 0.42f, C(CREAM), 1); }
        else if (phase == PH_SHOP || phase == PH_SHOPOPEN) { text("Wardrobe Shop: pick a gift box ($40)", 200, 120, 0.5f, C(255, 255, 255), 1); text("One of the three is a JACKPOT with 2 looks!", 200, 140, 0.42f, C(255, 220, 240), 1);
            if (phase == PH_SHOPOPEN && shopWin >= 0) { bool lay = artLayerBegin(1, g_artStyle); Sim s = MC(); s.outfit = (uint8_t)shopWin; drawSim(s, 200, 236, 1.3f, t, 0, PO_WAVE, false); if (lay) artLayerEnd(0, 0, 400, 240); } }
        else { bool lay = artLayerBegin(1, g_artStyle); for (int i = 0; i < 3; i++) { Sim s = starSim(i * 3, -1); drawSim(s, 110 + i * 90.f, 236, 1.3f, t, 0, i == 1 ? PO_WAVE : PO_IDLE, i == 2); } if (lay) artLayerEnd(0, 0, 400, 240); }
        break;
    }
    case PH_SCRIPTS: {
        grad(0, 0, 400, 240, C(250, 240, 230), C(240, 220, 210));
        rrect(20, 14, 360, 212, 8, C(UI1)); rrect(24, 18, 352, 204, 6, C(255, 252, 246));
        text(S.era, 200, 24, 0.42f, C(200, 40, 100), 1);
        textOutline(S.title, 200, 40, 0.65f, C(255, 255, 255), C(UI1), 1, 1.2f);
        std::vector<std::string> ln = wrapText(S.logline, 0.4f, 330);
        for (size_t i = 0; i < ln.size() && i < 3; i++) text(ln[i].c_str(), 200, 66 + i * 14.f, 0.4f, C(UI1), 1);
        for (int r = 0; r < S.nroles; r++) { char b[80]; snprintf(b, sizeof b, "%s  -  needs %s", S.role[r], tagName(S.need[r])); text(b, 40, 116 + r * 16.f, 0.42f, C(UI1)); }
        char b[64]; snprintf(b, sizeof b, "Style goal: %s", THEMES[S.theme].name); text(b, 40, 170, 0.45f, C(200, 40, 100));
        text("Best stars:", 40, 194, 0.42f, C(UI1)); stars3(140, 200, G.movie.stars[pick], 6);
        if (!unlockedScript(pick)) textOutline("LOCKED", 300, 190, 0.6f, C(255, 255, 255), C(200, 40, 60), 1);
        break;
    }
    case PH_CAST: {
        drawLoc(L_MALL, t); header("Casting Call");
        char b[96]; snprintf(b, sizeof b, "Role %d/%d: %s", role + 1, S.nroles, S.role[role]); textOutline(b, 200, 30, 0.6f, C(255, 255, 255), C(UI1), 1);
        snprintf(b, sizeof b, "This role needs a %s actor", tagName(S.need[role])); text(b, 200, 54, 0.45f, C(UI1), 1);
        drawCast(t, 0.5f, role - 1);
        break;
    }
    case PH_STYLE: {
        drawLoc(L_MALL, t); header("Wardrobe & Styling");
        char b[96]; snprintf(b, sizeof b, "Style goal: %s", THEMES[S.theme].name); textOutline(b, 200, 28, 0.55f, C(255, 255, 255), C(UI1), 1);
        snprintf(b, sizeof b, "%s as %s", starName(cast[role]).c_str(), S.role[role]); text(b, 200, 50, 0.42f, C(UI1), 1);
        bool lay = artLayerBegin(1, g_artStyle); Sim s = starSim(cast[role], look[role]); drawSim(s, 200, 236, 1.9f, t, 0, PO_IDLE, false); if (lay) artLayerEnd(0, 0, 400, 240);
        if (onTheme(S.theme, look[role])) { textOutline("ON THEME!", 320, 120, 0.55f, C(255, 240, 120), C(UI1), 1); sparkle(320 + std::sin(t * 5) * 30, 110, 5, C(255, 255, 255)); }
        break;
    }
    case PH_LOC: { drawLoc(loc >= 0 ? loc : L_HALLWAY, t); header("Pick the Filming Location"); drawCast(t, 0.5f); if (loc >= 0) textOutline(LOC_NAMES[loc], 200, 30, 0.6f, C(255, 255, 255), C(UI1), 1); break; }
    case PH_SCENE: {
        drawLoc(loc, t); drawCast(t, 0.5f, scene);
        rect(0, 0, 400, 24, C(0, 0, 0, 200)); rect(0, 216, 400, 24, C(0, 0, 0, 200));   // letterbox
        char b[64]; snprintf(b, sizeof b, "SCENE %d  -  ACTION!", scene + 1); text(b, 200, 4, 0.5f, C(255, 240, 180), 1);
        rrect(40, 30, 320, 34, 8, C(255, 255, 255, 230)); text(S.ch[scene].prompt, 200, 40, 0.45f, C(UI1), 1);
        break;
    }
    case PH_POSTER: case PH_REVIEW: {
        drawLoc(loc, t); drawCast(t, 0.5f);
        rect(0, 0, 400, 240, C(0, 0, 0, phase == PH_POSTER ? 40 : 90));
        textOutline(S.title, 200, 16, 0.7f, C(255, 240, 180), C(UI1), 1, 1.5f);
        text("A Starlight Studios Production", 200, 42, 0.38f, C(255, 255, 255), 1);
        if (phase == PH_POSTER) { float x = 40 + meter * 320; circle(x, 140, 70, C(255, 255, 220, 50)); text("Snap the poster when the spotlight is in the middle!", 200, 222, 0.38f, C(255, 255, 255), 1); }
        else { stars3(200, 75, result, 14); char b[48]; snprintf(b, sizeof b, "Score: %d/100", score); textOutline(b, 200, 96, 0.55f, C(255, 255, 255), C(UI1), 1); }
        break;
    }
    }
}

void directorDrawBottom(float t) {
    bottomBackdrop(t);
    const Script& S = SCRIPTS[pick];
    switch (phase) {
    case PH_LOBBY:
        text("Director Mode", 160, 4, 0.55f, C(CHAMP), 1);
        text("Make movies, earn stars & fans!", 160, 26, 0.42f, C(CREAM), 1);
        button(gridB(2), "Make a Movie", false, 0, 0.45f); button(gridB(3), "Side Jobs", false, 2, 0.45f);
        button(gridB(4), "Wardrobe Shop", false, 3, 0.45f); button(gridB(5), "Back", false, 1, 0.45f);
        text("Match genre, style & story for 3 stars!", 160, 200, 0.4f, C(CREAM), 1);
        break;
    case PH_SCRIPTS:
        text("Scripts", 160, 4, 0.5f, C(CHAMP), 1);
        for (int i = 0; i < 7; i++) {
            int s = listScroll + i; if (s >= NSCRIPTS) break;
            Btn b = listB(i);
            if (!unlockedScript(s)) { rrect(b.x, b.y, b.w, b.h, 6, C(UI2, 200)); text("??? (make more movies)", b.x + 8, b.y + 5, 0.4f, C(CREAM, 160)); continue; }
            button(b, SCRIPTS[s].title, s == pick, s == pick ? 0 : 2, 0.4f);
            stars3(b.x + b.w - 26, b.y + 12, G.movie.stars[s], 4);
        }
        button(bPrev, "<", false, 3, 0.5f); button(bFwd, ">", false, 3, 0.5f);
        button(bNext, "Make It!", unlockedScript(pick), 0, 0.45f); button(bBack, "Back", false, 1, 0.45f);
        break;
    case PH_CAST:
        text("Hire an actor", 160, 4, 0.5f, C(CHAMP), 1);
        for (int i = 0; i < 7; i++) {
            int s = listScroll + i; if (s > NSTARS) break;
            Btn b = listB(i);
            bool fit = (starTags(s) & S.need[role]) != 0;
            std::string lab = starName(s) + "  [" + tagList(starTags(s)) + "]";
            button(b, lab.c_str(), fit, s == NSTARS ? 3 : fit ? 0 : 2, 0.38f);
        }
        button(bPrev, "<", false, 3, 0.5f); button(bFwd, ">", false, 3, 0.5f); button(bBack, "Back", false, 1, 0.45f);
        break;
    case PH_STYLE: {
        text("Dress your star", 160, 4, 0.5f, C(CHAMP), 1);
        button(bLeft, "<", false, 3, 0.8f); button(bRight, ">", false, 3, 0.8f);
        rrect(86, 104, 148, 42, 8, C(255, 255, 255, 220)); text(OUTFITS[look[role]].name, 160, 118, 0.45f, C(UI1), 1);
        char b[48]; snprintf(b, sizeof b, "Your closet: %d looks", (int)owned.size()); text(b, 160, 170, 0.4f, C(CREAM), 1);
        button(bNext, role + 1 < S.nroles ? "Next Star" : "To Location", false, 0, 0.42f); button(bBack, "Back", false, 1, 0.45f);
        break;
    }
    case PH_LOC:
        text("Where do we film?", 160, 4, 0.5f, C(CHAMP), 1);
        for (int i = 0; i < L_COUNT; i++) button(gridB(i), LOC_NAMES[i], loc == i, loc == i ? 0 : 2, 0.4f);
        button(bNext, "Action!", loc >= 0, 0, 0.45f); button(bBack, "Back", false, 1, 0.45f);
        break;
    case PH_SCENE:
        text("How does the scene play out?", 160, 4, 0.5f, C(CHAMP), 1);
        for (int i = 0; i < 3; i++) button(Btn{6, 60 + i * 44.f, 308, 40}, S.ch[scene].opt[i], false, i == 0 ? 0 : i == 1 ? 2 : 3, 0.42f);
        break;
    case PH_POSTER:
        text("Poster photo shoot!", 160, 20, 0.55f, C(CHAMP), 1);
        button(Btn{80, 100, 160, 60}, "SNAP!", std::fabs(meter - 0.5f) < 0.08f, 0, 0.8f);
        break;
    case PH_REVIEW:
        text("The Critics Are In", 160, 4, 0.5f, C(CHAMP), 1);
        for (int i = 0; i < 3; i++) {
            rrect(6, 26 + i * 52.f, 308, 48, 8, C(255, 255, 255, 230));
            text(CRITICS[i], 14, 30 + i * 52.f, 0.4f, C(200, 40, 100));
            std::vector<std::string> ln = wrapText(critic[i], 0.38f, 290);
            for (size_t k = 0; k < ln.size() && k < 2; k++) text(ln[k].c_str(), 14, 44 + i * 52.f + k * 13, 0.38f, C(UI1));
        }
        { char b[80]; snprintf(b, sizeof b, "+$%d   +%d fans", payMoney, payFans); text(b, 160, 186, 0.5f, C(255, 240, 160), 1); }
        button(bNext, "That's a Wrap!", true, 0, 0.42f);
        break;
    case PH_JOBS:
        text("Side Jobs (once a day each)", 160, 4, 0.5f, C(CHAMP), 1);
        for (int i = 0; i < 4; i++) { char b[64]; snprintf(b, sizeof b, "%s  ($%d, %d fans)", JOB_NAMES[i], JOB_PAY[i], JOB_FANS[i]); button(listB(i + 1), b, false, G.movie.jobDay[i] == G.day ? 1 : 0, 0.4f); }
        button(bBack, "Back", false, 1, 0.45f);
        break;
    case PH_JOB: button(Btn{80, 100, 160, 60}, "POSE!", std::fabs(meter - 0.5f) < 0.08f, 0, 0.8f); break;
    case PH_SHOP: case PH_SHOPOPEN:
        text("Wardrobe Shop", 160, 4, 0.5f, C(CHAMP), 1);
        for (int i = 0; i < 3; i++) { float x = 20 + i * 100.f, bob = std::sin(t * 3 + i) * 3; rrect(x, 60 + bob, 80, 90, 8, C(UI1)); rrect(x + 3, 63 + bob, 74, 84, 6, C(i == 0 ? RGB{255, 120, 170} : i == 1 ? RGB{120, 200, 230} : RGB{200, 150, 240})); rect(x + 36, 63 + bob, 8, 84, C(255, 240, 160)); rect(x + 3, 98 + bob, 74, 8, C(255, 240, 160)); heart(x + 40, 60 + bob, 7, C(255, 240, 160)); if (shopPick == i) sparkle(x + 40, 100, 20, C(255, 255, 255)); }
        { char b[32]; snprintf(b, sizeof b, "Money: $%d", G.money); text(b, 160, 170, 0.45f, C(CREAM), 1); }
        button(bBack, "Back", false, 1, 0.45f);
        break;
    }
}
