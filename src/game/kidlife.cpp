#include "kidlife.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "people.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include "tween.hpp"
#include <string>
#include <algorithm>

static bool kidAge(int st) { return st == ST_CHILD || st == ST_PRETEEN; }
bool outfitFits(int k, int st, bool guy) { if (k >= FIRST_GIRL_BASIC && k < NUM_OUTFITS) return !guy && st >= ST_TEEN; if (k >= FIRST_MALE_OUTFIT && k < NUM_OUTFITS) return guy; return kidOutfitOk(k, st); }
bool kidOutfitOk(int k, int st) {
    if (k >= FIRST_MALE_OUTFIT) return true; return k < FIRST_KID_OUTFIT || k >= NUM_OUTFITS ? true : (st >= ST_TODDLER && st <= ST_PRETEEN); }
const char* schoolName(int st) { return st == ST_CHILD ? "Elementary School" : st == ST_PRETEEN ? "Middle School" : "High School"; }
static std::string first(int s) { const std::string& n = G.sims[s].name; return n.substr(0, n.find(' ')); }

struct Toy { const char* name; int price; uint8_t skill; const char* line; };
static const Toy TOYS[] = {
    {"Fashion Doll", 15, SK_STYLE, "You give your doll a full glow-up."}, {"Building Blocks", 12, SK_LOGIC, "You build a tower taller than you!"},
    {"Toy Flip Phone", 10, SK_CHARISMA, "\"Hello? Yes, this is the president.\""}, {"Giant Plushie", 18, SK_CHARISMA, "You hug your plushie and tell it everything."},
    {"Jump Rope", 6, SK_FITNESS, "Cinderella dressed in yella..."}, {"Coloring Set", 8, SK_CREATIVITY, "You color OUTSIDE the lines. On purpose."},
    {"Play Kitchen", 30, SK_COOKING, "You serve a 5-star plastic soup."}, {"RC Car", 25, SK_LOGIC, "VROOOM - straight into the couch."},
    {"Dollhouse", 40, SK_CREATIVITY, "The dolls are having family drama."}, {"Scooter", 28, SK_FITNESS, "You zoom down the sidewalk!"},
    {"Karaoke Mic", 20, SK_CHARISMA, "You give a sold-out living room concert."}, {"Slime Kit", 9, SK_CREATIVITY, "Glitter slime. Everywhere."},
};
constexpr int NTOY = sizeof TOYS / sizeof TOYS[0];

static int grade(const Sim& s) {   // rough grade level from age-in-stage
    const int base = s.stage == ST_CHILD ? 1 : s.stage == ST_PRETEEN ? 6 : 9, span = s.stage == ST_CHILD ? 5 : s.stage == ST_PRETEEN ? 3 : 4;
    return base + std::min(span - 1, (int)(s.stageDays * span / std::max(1, (int)STAGE_DAYS[s.stage])));
}
static std::string gradeName(int g) { static const char* O[] = {"K", "1st", "2nd", "3rd", "4th", "5th", "6th", "7th", "8th", "9th", "10th", "11th", "12th"}; return std::string(O[std::max(0, std::min(12, g))]) + " grade"; }

static std::vector<int> classmates() {   // same-age, not family
    std::vector<int> v; const Sim& me = MC();
    for (int i = 0; i < (int)G.sims.size(); i++) if (i != G.mc && G.sims[i].stage == me.stage && !isFamily(G.mc, i)) v.push_back(i);
    return v;
}

LxMenu kidApp() {
    LxMenu m; const Sim& me = MC();
    m.push_back({std::string("Purse/Bag: ") + (G.hidePurse ? "HIDDEN (tap to show)" : "SHOWN (tap to hide)"), 16001});
    if (me.stage >= ST_CHILD && me.stage <= ST_TEEN) m.push_back({std::string(schoolName(me.stage)) + " - " + gradeName(grade(me)) + " (" + letterGrade(me.gradePts) + ")", 16999});
    if (me.stage >= ST_TODDLER && me.stage <= ST_PRETEEN) { m.push_back({"Toy Box", 16002}); m.push_back({"Toy Shop", 16003}); }
    if (kidAge(me.stage)) {
        if (G.crush >= 0 && G.crush < (int)G.sims.size() && G.sims[G.crush].stage == me.stage) {
            m.push_back({"My Crush: " + first(G.crush) + (G.crushBack ? " (likes you back!)" : ""), 16999});
            m.push_back({"Wave & Blush", 16010}); m.push_back({"Share Your Snack", 16011}); m.push_back({"Pass a Note", 16012});
            m.push_back({"\"Do you like me? [ ] yes [ ] no\"", 16013}); m.push_back({"Ask Your BFF If They Like You", 16014}); m.push_back({"Get Over It", 16015});
        } else m.push_back({"Pick My First Crush", 16004});
    }
    if (m.size() == 1) m.push_back({"(Toys, crushes and school are for kids)", 16999});
    return m;
}

LxMenu kidOption(int id) {
    Sim& me = MC(); LxMenu m;
    if (id == 16001) { G.hidePurse ^= 1; toast(G.hidePurse ? "Purses & bags hidden on everyone." : "Purses & bags are back."); return kidApp(); }
    if (id == 16002) { for (int i = 0; i < NTOY; i++) if (G.toys >> i & 1) m.push_back({std::string("Play: ") + TOYS[i].name, 16100 + i});
                       if (m.empty()) m.push_back({"(Empty! Visit the Toy Shop)", 16999}); return m; }
    if (id == 16003) { for (int i = 0; i < NTOY; i++) if (!(G.toys >> i & 1)) m.push_back({std::string(TOYS[i].name) + " $" + std::to_string(TOYS[i].price), 16200 + i});
                       if (m.empty()) m.push_back({"(You have every toy!)", 16999}); return m; }
    if (id == 16004) { for (int i : classmates()) m.push_back({first(i) + " (" + G.sims[i].name + ")", 16300 + i});
                       if (m.empty()) m.push_back({"(No one your age around yet)", 16999}); return m; }
    if (id >= 16100 && id < 16100 + NTOY) { const Toy& t = TOYS[id - 16100]; me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 30); me.skills[t.skill] = std::min(10.f, me.skills[t.skill] + 0.15f);
        tweenPlay(G.mc, TW_HOP); toast(t.line); addMoodlet(me, "Playtime!", EM_HAPPY, 1, 180);
        for (int i = 0; i < (int)G.sims.size(); i++) if (i != G.mc && G.sims[i].household && kidAge(G.sims[i].stage) && irand(2)) { changeRel(G.mc, i, 4, 0); toast(first(i) + " plays too!"); break; }
        return {}; }
    if (id >= 16200 && id < 16200 + NTOY) { const Toy& t = TOYS[id - 16200]; if (G.money < t.price) { toast("Not enough money - ask a parent!"); return {}; }
        G.money -= t.price; G.toys |= 1u << (id - 16200); toast(std::string("New toy: ") + t.name + "!"); tweenPlay(G.mc, TW_CHEER); return {}; }
    if (id >= 16300 && id < 16300 + (int)G.sims.size()) { G.crush = (int16_t)(id - 16300); G.crushBack = 0; addMoodlet(me, "Butterflies", EM_FLIRTY, 1, 720); toast("You think " + first(G.crush) + " is SO cool. Don't tell anyone!"); return {}; }
    const int c = G.crush; if (c < 0 || c >= (int)G.sims.size()) return {};
    const std::string n = first(c);
    switch (id) {
    case 16010: changeRel(G.mc, c, 2, 0); toast(irand(2) ? n + " waves back! Your face turns tomato red." : "You waved... at the person behind " + n + ". Mortifying."); break;
    case 16011: changeRel(G.mc, c, 5, 0); toast(n + " takes half your cookie. Best. Day. Ever."); break;
    case 16012: changeRel(G.mc, c, 3, 0); toast(irand(3) ? n + " folds your note into their pocket and smiles." : "The teacher intercepts the note! The whole class goes \"OOOOH\"."); break;
    case 16013: if (irand(100) < 35 + rel(G.mc, c).fr / 3) { G.crushBack = 1; addMoodlet(me, "They Checked YES!", EM_HAPPY, 2, 1440); tweenPlay(G.mc, TW_HOP); toast(n + " checked YES! You can't stop smiling."); }
                else { addMoodlet(me, "They Checked No", EM_SAD, 1, 720); toast(n + " checked \"maybe\"... and drew a frowny face. Ouch."); } break;
    case 16014: toast(irand(2) ? "Your BFF says " + n + " talks about you ALL the time." : "Your BFF says " + n + " thinks you're \"just a friend.\""); break;
    case 16015: G.crush = -1; G.crushBack = 0; toast("Crushes are dumb anyway. (They're not.)"); break;
    }
    return {};
}

void kidHourly(int hour) {
    Sim& me = MC();
    if (G.room != R_SCHOOL || !me.schoolToday || hour < 9 || hour > 14 || irand(100) >= 35) return;
    static const char* E[] = {"Show & Tell: you bring your best toy!", "Recess! You win four square.", "Spelling bee - you nail \"necessary\"!", "Class pet hamster escaped again!",
                              "Field trip permission slip came home!", "Picture day - you blinked."};
    static const char* M[] = {"You can't get your locker open... again.", "The bell rings and everyone floods the hallway.", "Someone's mom is chaperoning the school dance.",
                              "Group project: you do ALL the work.", "Gym class: running the mile. Ugh.", "Lunch table drama: who sits where?"};
    static const char* H[] = {"Pop quiz in chemistry!", "Pep rally in the gym!", "Someone pulled the fire alarm.", "Yearbook photos today."};
    const char* line = me.stage == ST_CHILD ? E[irand(6)] : me.stage == ST_PRETEEN ? M[irand(6)] : H[irand(4)];
    toast(line); me.needs[N_SOCIAL] = std::min(100.f, me.needs[N_SOCIAL] + 5);
    if (kidAge(me.stage) && G.crush >= 0 && irand(4) == 0) toast(first(G.crush) + " sat next to you in class!!");
}
void kidDaily() {
    const Sim& me = MC();
    if (G.crush >= 0 && (G.crush >= (int)G.sims.size() || !kidAge(me.stage) || G.sims[G.crush].stage != me.stage)) { G.crush = -1; G.crushBack = 0; }   // outgrown
    if (kidAge(me.stage) && G.crush < 0 && irand(100) < 4) { auto v = classmates(); if (!v.empty()) toast("Is it just you, or is " + first(v[irand((int)v.size())]) + " kinda cool? (Kid Life > Pick My First Crush)"); }
}
