#include "lockup.hpp"
// Crime & the police, scandals (The Tea) and blackmail. Teen 13+: mischief and drama,
// nothing graphic - getting caught means fines, embarrassment and headlines.
#include "travelcourt.hpp"
#include "crime.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "social.hpp"
#include "goals.hpp"
#include "extras.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "cutscene.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace col;

const char* SECRETS[NUM_SECRETS] = {
    "still sleeps with a teddy bear named Mr. Snuggles", "cheated on the big math test", "carries a FAKE designer bag",
    "can't actually swim", "secretly runs the anonymous gossip account", "ate the whole office birthday cake",
    "lies about their age online", "dyes their 'natural' hair", "is terrified of the ducks at the park",
    "still owes Cafe Latte $40", "stole the school mascot costume", "has a huge secret crush on someone in town",
    "faked being sick to skip work", "pays someone to write their posts", "failed the driving test four times",
    "blasts polka music when nobody's home", "got banned from the rink for 'excessive twirling'", "wears a wig"};
static const char* CRIME_VERB[CRIME_COUNT] = {"shoplifting at the mall", "tagging a wall", "sneaking into the movies", "dining and dashing",
                                              "picking pockets", "hacking accounts", "breaking into a house", "eavesdropping", "toilet-papering a house", "ding-dong-ditching", "cheating on a test", "freeing the pet shop hamsters", "skipping class", "spray-painting a car",
                                              "swapping price tags", "using a fake ID", "scalping concert tickets", "joyriding a golf cart", "faking sick", "stealing ALL the free samples", "copying homework", "running a catfish account"};
static const int FINES[CRIME_COUNT] = {120, 80, 25, 60, 100, 150, 400, 0, 50, 15, 0, 90, 0, 140, 90, 75, 110, 130, 0, 20, 0, 60};
static const float BASE_CATCH[CRIME_COUNT] = {0.28f, 0.18f, 0.22f, 0.25f, 0.3f, 0.2f, 0.f, 0.f, 0.2f, 0.15f, 0.3f, 0.25f, 0.2f, 0.25f, 0.24f, 0.32f, 0.26f, 0.3f, 0.2f, 0.12f, 0.25f, 0.18f};
static const uint8_t HEAT_GAIN[CRIME_COUNT] = {14, 8, 4, 10, 14, 10, 30, 0, 8, 2, 0, 10, 0, 14, 10, 8, 12, 16, 0, 2, 0, 6};
static const int SCANDAL_FOR[CRIME_COUNT] = {SC_SHOPLIFT, SC_VANDAL, SC_SHOPLIFT, SC_DINEDASH, SC_PICKPOCKET, SC_ARREST, SC_ARREST, SC_SHOPLIFT, SC_VANDAL, SC_VANDAL, SC_SECRET, SC_VANDAL, SC_SECRET, SC_VANDAL,
                                              SC_SHOPLIFT, SC_ARREST, SC_ARREST, SC_VANDAL, SC_SECRET, SC_SHOPLIFT, SC_SECRET, SC_SECRET};

int secretOf(int sim) { uint32_t h = (uint32_t)sim * 2654435761u + 0x5bd1e995u; return (int)(h % NUM_SECRETS); }
int mischiefLevel() { return clampv((int)(G.mischief / 10.f), 0, 10); }
int heatStars() { return clampv((G.heat + 19) / 20, 0, 5); }
int dirtIndex(int sim) { for (size_t i = 0; i < G.dirt.size(); i++) if (G.dirt[i].sim == sim) return (int)i; return -1; }
static void heatUp(int d) { G.heat = (uint8_t)clampv((int)G.heat + d, 0, 100); }
static void repDown(int d) { G.rep = (int16_t)std::max(-100, G.rep - d); }
static void mischiefUp(float d) { G.mischief = std::fmin(100.f, G.mischief + d); }

int copSim() {
    if (G.cop >= 0 && G.cop < (int)G.sims.size()) return G.cop;
    Sim s = randomSim(ST_ADULT, irand(2));
    s.name = "Officer Reyes";
    s.bio = "to protect & serve (& judge)";
    s.look.top = {44, 62, 120}; s.look.bottom = {36, 44, 84}; s.look.bt = BT_PANTS; s.look.topStyle = TS_BUTTON; s.look.topPattern = 0;
    s.look.shoes = {30, 26, 30}; s.look.shoeStyle = SS_BOOTS; s.look.hat = HT_CAP; s.look.acc = ACC_SHADES; s.outfit = 0;
    s.household = false;
    G.sims.push_back(s);
    G.cop = (int16_t)(G.sims.size() - 1);
    return G.cop;
}
bool copHere() {
    if (G.cop < 0) return false;
    for (const Actor& a : curRoom().actors) if (a.kind == 0 && a.id == G.cop) return true;
    return false;
}
static int witnessesHere() {
    int n = 0;
    for (const Actor& a : curRoom().actors) if (a.kind == 0 && a.id != G.mc && a.id < G.sims.size() && !G.sims[a.id].household && G.sims[a.id].stage >= ST_CHILD) n++;
    return n;
}
static int randomWitness() {
    std::vector<int> v;
    for (const Actor& a : curRoom().actors) if (a.kind == 0 && a.id != G.mc && a.id != G.cop && a.id < G.sims.size() && !G.sims[a.id].household && G.sims[a.id].stage >= ST_TEEN) v.push_back(a.id);
    return v.empty() ? -1 : v[irand((int)v.size())];
}

std::vector<CrimeOpt> crimeVenueOptions(int room) {
    std::vector<CrimeOpt> v;
    const Sim& me = MC();
    const int h = (int)(G.minute / 60);
    bool night = h >= 21 || h < 4;
#ifdef HEIST_TEST
    night = true;
#endif
    if (me.stage < ST_CHILD) return v;
    if ((room == R_MALL || room == R_BOUTIQUE) && me.stage >= ST_TEEN) v.push_back({"Shoplift (risky!)", CRIME_SHOPLIFT});
    if ((room == R_PARK || room == R_SCHOOL || room == R_MALL || room == R_BEACH) && me.stage >= ST_PRETEEN) v.push_back({"Tag a Wall (graffiti)", CRIME_GRAFFITI});
    if (room == R_CINEMA) v.push_back({"Sneak Into a Movie", CRIME_SNEAKIN});
    if (room == R_RESTAURANT && me.stage >= ST_TEEN) v.push_back({"Dine & Dash", CRIME_DINEDASH});
    if (room == R_CAFE || room == R_MALL || room == R_SCHOOL || room == R_SPA || room == R_CLUB) v.push_back({"Eavesdrop on Gossip", CRIME_EAVESDROP});
    if ((room == R_CLUB || room == R_PARK) && me.stage >= ST_YA && night) v.push_back({G.heistDay == G.day ? "(Heist done tonight)" : "Midnight Heist (crew job)", G.heistDay == G.day ? -1 : CRIME_HEIST});
    if (room == R_STREET && me.stage >= ST_CHILD) { v.push_back({"Ding-Dong Ditch", CRIME_DITCH}); if (me.stage >= ST_PRETEEN && night) v.push_back({"TP a House", CRIME_TP}); if (me.stage >= ST_TEEN && night) v.push_back({"Spray-Paint a Car", CRIME_SPRAYCAR}); }
    if (room == R_SCHOOL && me.stage >= ST_CHILD && me.stage <= ST_TEEN) { v.push_back({"Cheat on a Test", CRIME_CHEAT}); v.push_back({"Skip Class", CRIME_SKIPCLASS}); }
    if (room == R_PETSHOP) v.push_back({"Free the Hamsters!", CRIME_HAMSTERS});
    if (room == R_BOUTIQUE && me.stage >= ST_TEEN) v.push_back({"Swap Price Tags", CRIME_TAGSWAP});
    if (room == R_CLUB && me.stage == ST_TEEN) v.push_back({"Sneak In w/ a Fake ID", CRIME_FAKEID});
    if ((room == R_STREET || room == R_MALL) && me.stage >= ST_TEEN) v.push_back({"Scalp Concert Tickets", CRIME_SCALP});
    if ((room == R_PARK || room == R_HOTEL) && me.stage >= ST_TEEN) v.push_back({"Joyride a Golf Cart", CRIME_JOYRIDE});
    if (room == R_BEACH && me.career != CR_NONE && isWeekday(G.day) && !me.workedToday) v.push_back({"Fake Sick (call in to work)", CRIME_FAKESICK});
    if (room == R_BAKERY || room == R_MALL_BEAUTY) v.push_back({"Take ALL the Free Samples", CRIME_SAMPLES});
    if (room == R_SCHOOL && me.stage >= ST_CHILD && me.stage <= ST_TEEN) v.push_back({"Copy Someone's Homework", CRIME_COPYHW});
    if ((room == R_CAFE || room == R_LIBRARY) && me.stage >= ST_TEEN) v.push_back({"Run a Catfish Account", CRIME_CATFISH});
    if (room == R_MALL && !G.alarm && me.isAdult()) v.push_back({"Home Security System ($300)", 100});
    return v;
}

// ---------------------------------------------------------------- getting busted
static std::string bustResult(int choice, int crime) {
    Sim& me = MC();
    const int fine = FINES[crime % CRIME_COUNT];
    char b[220];
    std::string out;
    G.rapSheet++;
    if (choice == 0) {
        if (G.money >= fine) { G.money -= fine; snprintf(b, sizeof b, "You pay the $%d fine. Officer Reyes writes it up anyway. \"Don't let me see you again.\"", fine); }
        else { me.needs[N_ENERGY] = std::fmax(0.f, me.needs[N_ENERGY] - 35); me.needs[N_FUN] = std::fmax(0.f, me.needs[N_FUN] - 30); snprintf(b, sizeof b, "You can't cover the $%d fine, so it's community service: four hours of picking up litter in a bright orange vest.", fine); }
        out = b; heatUp(6); repDown(4);
        addMoodlet(me, "Busted", EM_TENSE, 2, 360);
    } else if (choice == 1) {
        float ch = 0.25f + skillLevel(me, SK_CHARISMA) * 0.06f + (me.hasTrait(TR_GLAMOROUS) ? 0.1f : 0.f);
        if (frand() < ch) { out = "\"...Fine. It's a warning. ONE warning.\" Your charm saved you this time!"; heatUp(4); addMoodlet(me, "Smooth Talker", EM_CONFIDENT, 1, 180); G.rapSheet--; }
        else { int f = fine * 3 / 2; G.money = std::max(0, (int)G.money - f); snprintf(b, sizeof b, "\"Nice try.\" The fine just went UP to $%d for the attitude.", f); out = b; heatUp(10); repDown(6); addMoodlet(me, "Embarrassed", EM_TENSE, 2, 240); }
    } else {
        float ch = 0.3f + skillLevel(me, SK_FITNESS) * 0.06f + mischiefLevel() * 0.02f;
        if (frand() < ch) { out = "You bolt around the corner and lose them in the crowd! Heart pounding. They'll be looking for you now..."; heatUp(25); G.rapSheet--; addMoodlet(me, "Adrenaline Rush", EM_ENERGIZED, 2, 180); travelTo(R_LIVING); }
        else { int f = fine * 2; G.money = std::max(0, (int)G.money - f); snprintf(b, sizeof b, "You trip over your own shoelace. Ouch. A night in the slammer and a $%d fine. Not your best look.", f); out = b; heatUp(18); repDown(10); me.needs[N_ENERGY] = 20; courtSchedule(crime); if (me.stage >= ST_TEEN) jailSentence(1, "resisting arrest"); }
    }
    if (G.rapSheet >= 3 && me.isAdult()) courtSchedule(crime);
    if (G.rapSheet > 0 && (fameStars() >= 1 || G.followers >= 300) && irand(100) < 70) scandalBreak(SCANDAL_FOR[crime % CRIME_COUNT]);
    return out;
}
static void busted(int crime) {
    sfxNo();
    const bool first = !(G.csSeen & (1u << CS_BUSTED));
    int cop = copSim();
    if (!copHere()) { Actor a; a.kind = 0; a.id = (uint16_t)cop; a.x = 360; a.tx = 300; a.y = 214; a.stay = 60; a.say = "FREEZE!"; a.sayT = 3; curRoom().actors.push_back(a); }
    if (first) cutscenePlay(CS_BUSTED);
    char b[200];
    snprintf(b, sizeof b, "Officer Reyes: \"Hold it right there! I saw you %s. That's a $%d fine.\"", CRIME_VERB[crime % CRIME_COUNT], FINES[crime % CRIME_COUNT]);
    momentCustom("BUSTED!", b, "Pay the fine", "Talk your way out", "Make a run for it!", bustResult, crime, cop);
}

// ---------------------------------------------------------------- crimes
static void maybeWitness(int crime) {
    if (irand(100) >= 35) return;
    int w = randomWitness();
    if (w < 0) return;
    G.witness = (int16_t)w; G.witnessCrime = (uint8_t)crime; G.witnessDay = G.day;
    say(w, irand(2) ? "...did I just see that?" : "Ohhh, I'm telling.");
}
void crimeAttempt(int crime, int target) {
    Sim& me = MC();
    if (crime == 100) {   // home security
        if (G.money < 300) { toast("Not enough money!"); sfxNo(); return; }
        G.money -= 300; G.alarm = true; toast("Home security installed! Burglars beware."); sfxYay(); return;
    }
    if (crime == CRIME_EAVESDROP) {
        std::vector<int> v;
        for (const Actor& a : curRoom().actors) if (a.kind == 0 && a.id != G.mc && a.id < G.sims.size() && !G.sims[a.id].household && dirtIndex(a.id) < 0) v.push_back(a.id);
        if (v.empty() || irand(100) < 35) { static const char* NOPE[] = {"...they're just talking about the weather. Boring.", "You lean in. They lean away. Awkward.", "Something about a coupon? Not juicy."}; toast(NOPE[irand(3)]); return; }
        int s = v[irand((int)v.size())];
        G.dirt.push_back({(uint16_t)s, (uint8_t)secretOf(s), 0, G.day});
        toast("Juicy! " + G.sims[s].name + " " + SECRETS[secretOf(s)] + "!");
        addMoodlet(me, "Know a Secret", EM_PLAYFUL, 1, 240); mischiefUp(1.f); sfxYay();
        return;
    }
    if (crime == CRIME_HEIST) return;   // handled by the heist minigame
    float catchP = BASE_CATCH[crime] + G.heat * 0.003f + (copHere() ? 0.35f : 0.f) + witnessesHere() * 0.02f - mischiefLevel() * 0.025f - (MC().hasTrait(TR_SNEAKY) ? 0.12f : 0.f);
    catchP = clampv(catchP, 0.05f, 0.9f);
#ifdef HEIST_TEST
    catchP = 1.f;
#endif
    G.crimes++;
    if (frand() < catchP) {
        if (crime == CRIME_FAKESICK) { me.performance -= 25; dlgSay(-1, "Your boss is ALSO at the beach. She waves. You wave back. Performance -25. Awkward."); addMoodlet(me, "Caught Skipping", EM_TENSE, 2, 360); sfxNo(); return; }
        if (crime == CRIME_SAMPLES) { dlgSay(-1, "\"Sweetie. That's the whole tray.\" The clerk takes it back. Everyone saw."); repDown(3); addMoodlet(me, "Mortified", EM_TENSE, 1, 180); sfxNo(); return; }
        if (crime == CRIME_COPYHW) { me.gradePts = std::max(0.f, me.gradePts - 10.f); dlgSay(-1, "Teacher: \"Interesting. You made the EXACT same spelling mistakes.\" Zero on the assignment."); addMoodlet(me, "Detention", EM_SAD, 2, 360); sfxNo(); return; }
        if (crime == CRIME_CATFISH) { scandalBreak(SC_SECRET); dlgSay(-1, "Someone reverse-searched the photos. The catfish account is traced back to YOU. It's all over STATUS."); addMoodlet(me, "Exposed", EM_TENSE, 2, 480); return; }
        if (crime == CRIME_CHEAT || crime == CRIME_SKIPCLASS) {   // the principal, not the police
            me.gradePts = std::max(0.f, me.gradePts - 12.f);
            dlgSay(-1, crime == CRIME_CHEAT ? "Principal: \"Are those answers written on your HAND?\" Detention, and your grade drops." : "Principal: \"Going somewhere?\" Back to class - and detention after school.");
            addMoodlet(me, "Detention", EM_SAD, 2, 360); sfxNo(); return;
        }
        if (crime == CRIME_DITCH) { int w = randomWitness(); if (w >= 0) { say(w, "HEY! I SEE you, you little gremlin!"); changeRel(G.mc, w, -6, 0); } else say(G.mc, "RUN!!"); addMoodlet(me, "Busted (Kinda)", EM_PLAYFUL, 1, 120); return; }
        busted(crime); return;
    }
    mischiefUp(3.f); heatUp(HEAT_GAIN[crime]);
    char b[160];
    switch (crime) {
    case CRIME_SHOPLIFT: {
        int k = -1;
        for (int tries = 0; tries < 20 && k < 0; tries++) { int c = FIRST_AESTHETIC + irand(FIRST_KID_OUTFIT - FIRST_AESTHETIC - 1); if (!ownsOutfit(c)) k = c; }
        if (k >= 0) { giveOutfit(k); snprintf(b, sizeof b, "You walk out with the %s look. Nobody noticed... right?", OUTFITS[k].name); }
        else { G.money += 40; snprintf(b, sizeof b, "You pocket a lip gloss set (worth $40) and stroll out."); }
        toast(b); addMoodlet(me, "Sticky Fingers", EM_TENSE, 1, 180);
        break;
    }
    case CRIME_GRAFFITI: {
        skillGain(me, SK_CREATIVITY, 4); uint32_t f = 25 + G.followers / 60; G.followers += f;
        snprintf(b, sizeof b, "Your tag is a masterpiece. Someone posted it: +%u followers.", (unsigned)f); toast(b);
        G.cliqueStand[4] = (uint8_t)std::min(100, G.cliqueStand[4] + 4); addMoodlet(me, "Street Artist", EM_INSPIRED, 1, 240);
        break;
    }
    case CRIME_SNEAKIN: {
        Activity a = makeAct("Watching a Movie (shh!)", 120, 20); a.rate[N_FUN] = 34; startActivity(a);
        toast("You slipped in through the exit door. Free movie!"); addMoodlet(me, "Rebel", EM_PLAYFUL, 1, 180);
        break;
    }
    case CRIME_DINEDASH:
        me.needs[N_HUNGER] = std::fmin(100.f, me.needs[N_HUNGER] + 70);
        toast("Fancy dinner... and a sprint out the door. $0 bill!"); addMoodlet(me, "Full & Guilty", EM_TENSE, 1, 180);
        break;
    case CRIME_PICKPOCKET:
        if (target >= 0) { int amt = 15 + irand(70); G.money += amt; snprintf(b, sizeof b, "You lifted $%d from %s without them noticing.", amt, G.sims[target].name.c_str()); toast(b); }
        break;
    case CRIME_TP: {
        int n = randomWitness();
        snprintf(b, sizeof b, "Rolls and rolls of toilet paper, all over the Peach House trees. Art."); toast(b);
        if (n >= 0) changeRel(G.mc, n, -4, 0);
        me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 25); addMoodlet(me, "Prank Legend", EM_PLAYFUL, 2, 300); G.followers += 30;
        break;
    }
    case CRIME_DITCH: me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 15); say(G.mc, "*DING DONG* ...RUN!!"); addMoodlet(me, "Giggle Fit", EM_PLAYFUL, 1, 120); break;
    case CRIME_CHEAT: me.gradePts = std::min(100.f, me.gradePts + 12.f); toast("You aced the test... with a little help from your sleeve."); addMoodlet(me, "Guilty A+", EM_TENSE, 1, 240); break;
    case CRIME_SKIPCLASS: me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 30); me.gradePts = std::max(0.f, me.gradePts - 3.f); toast("You skipped class and hit the vending machines. Freedom!"); travelTo(R_MALL); break;
    case CRIME_HAMSTERS: me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 30); dlgSay(-1, "You flip the latch. Forty hamsters scatter across the store like tiny furry fireworks. Chaos. Beautiful chaos."); addMoodlet(me, "Hamster Hero", EM_PLAYFUL, 2, 360); G.followers += 60; break;
    case CRIME_TAGSWAP: {
        int k = -1;
        for (int tries = 0; tries < 20 && k < 0; tries++) { int c = FIRST_AESTHETIC + irand(FIRST_KID_OUTFIT - FIRST_AESTHETIC - 1); if (!ownsOutfit(c)) k = c; }
        if (k >= 0 && G.money >= 15) { G.money -= 15; giveOutfit(k); snprintf(b, sizeof b, "The %s look rang up as $15. The cashier didn't blink.", OUTFITS[k].name); }
        else snprintf(b, sizeof b, "You swapped some tags... then got nervous and bought nothing.");
        toast(b); addMoodlet(me, "Bargain Bandit", EM_TENSE, 1, 180);
        break;
    }
    case CRIME_FAKEID:
        me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 35); me.needs[N_SOCIAL] = std::fmin(100.f, me.needs[N_SOCIAL] + 25);
        toast("The bouncer squints at \"Kaylee, age 23\"... and waves you in. You only drink sodas. It's about the vibe.");
        addMoodlet(me, "Underground", EM_PLAYFUL, 2, 240);
        break;
    case CRIME_SCALP: { int amt = 60 + irand(120); G.money += amt; snprintf(b, sizeof b, "Sold two front-row tickets for $%d profit. Hustle.", amt); toast(b); addMoodlet(me, "Hustler", EM_CONFIDENT, 1, 180); repDown(1); break; }
    case CRIME_JOYRIDE: me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 45); G.followers += 40; toast("You and a golf cart did DONUTS on the lawn. Someone filmed it. Legend."); addMoodlet(me, "Joyride!", EM_ENERGIZED, 2, 300); break;
    case CRIME_FAKESICK: me.workedToday = true; me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 40); me.needs[N_ENERGY] = std::fmin(100.f, me.needs[N_ENERGY] + 20); toast("\"*cough cough* I can't come in today...\" Beach day!! (No pay today.)"); addMoodlet(me, "Playing Hooky", EM_HAPPY, 2, 360); break;
    case CRIME_SAMPLES: me.needs[N_HUNGER] = std::fmin(100.f, me.needs[N_HUNGER] + 45); toast("You swept the ENTIRE sample tray into your bag. Lunch: handled."); addMoodlet(me, "Snack Goblin", EM_PLAYFUL, 1, 180); break;
    case CRIME_COPYHW: me.homework = true; me.gradePts = std::min(100.f, me.gradePts + 4.f); toast("Copied the homework in 4 minutes flat. Changed a few words. Genius."); addMoodlet(me, "Guilty A+", EM_TENSE, 1, 180); break;
    case CRIME_CATFISH: {
        std::vector<int> v;
        for (const Rel& r : G.rels) { int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1; if (o >= 0 && o < (int)G.sims.size() && dirtIndex(o) < 0 && !G.sims[o].household && G.sims[o].stage >= ST_TEEN) v.push_back(o); }
        if (v.empty()) { toast("Nobody's talking to your fake account yet."); break; }
        int s = v[irand((int)v.size())];
        G.dirt.push_back({(uint16_t)s, (uint8_t)secretOf(s), 0, G.day});
        snprintf(b, sizeof b, "\"Jade from Malibu\" got %s to spill: they %s!", G.sims[s].name.c_str(), SECRETS[secretOf(s)]); toast(b);
        addMoodlet(me, "Two-Faced", EM_TENSE, 1, 240);
        break;
    }
    case CRIME_SPRAYCAR: me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 20); skillGain(me, SK_CREATIVITY, 3); toast("Someone's car now has a giant pink heart on it. You're an artist."); addMoodlet(me, "Vandal Vibes", EM_TENSE, 1, 240); break;
    case CRIME_HACK: {
        std::vector<int> v;
        for (const Rel& r : G.rels) { int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1; if (o >= 0 && o < (int)G.sims.size() && dirtIndex(o) < 0 && !G.sims[o].household) v.push_back(o); }
        if (!v.empty() && irand(2)) { int s = v[irand((int)v.size())]; G.dirt.push_back({(uint16_t)s, (uint8_t)secretOf(s), 0, G.day}); toast("Hacked " + G.sims[s].name + "'s DMs: they " + SECRETS[secretOf(s)] + "!"); }
        else { uint32_t f = 60 + G.followers / 30; G.followers += f; snprintf(b, sizeof b, "You boosted your posts with a bot farm. +%u followers.", (unsigned)f); toast(b); }
        skillGain(me, SK_LOGIC, 2);
        break;
    }
    }
    if (crime != CRIME_HACK) maybeWitness(crime);
    sfxYay();
}

void crimeHeistResult(bool caught, int loot) {
    Sim& me = MC();
    G.heistDay = G.day;
    G.crimes++;
    if (caught) { busted(CRIME_HEIST); return; }
    int pay = loot + loot * mischiefLevel() / 20;
    G.money += pay; G.heists++;
    mischiefUp(6.f); heatUp(HEAT_GAIN[CRIME_HEIST] - (loot < 100 ? 10 : 0));
    char b[120]; snprintf(b, sizeof b, "Clean getaway! The crew fences the loot: +$%d.", pay);
    toast(b); addMoodlet(me, "Master Thief", EM_CONFIDENT, 2, 360);
    if (G.heists == 1) addMemoryOnce(MEM_FUN, G.mc, -1, "Pulled off my first midnight heist. Shh!");
}

// ---------------------------------------------------------------- scandals
static const char* SCANDAL_TXT[SC_KIND_COUNT] = {"CAUGHT SHOPLIFTING at the mall", "ARRESTED in the middle of the night", "caught TWO-TIMING",
                                                 "exposed as a BLACKMAILER", "", "caught TAGGING walls", "DINED & DASHED", "caught PICKPOCKETING",
                                                 "had a PUBLIC MELTDOWN", "carrying a FAKE designer bag"};
std::string scandalHeadline(const Scandal& s) {
    std::string n = s.sim >= 0 && s.sim < (int)G.sims.size() ? G.sims[s.sim].name : std::string("Someone");
    if (s.kind == SC_SECRET) return n + " " + SECRETS[s.extra % NUM_SECRETS] + "!";
    return n + " " + SCANDAL_TXT[s.kind % SC_KIND_COUNT] + "!";
}
static void teaPush(int sim, int kind, int extra) {
    Scandal s; s.sim = (int16_t)sim; s.kind = (uint8_t)kind; s.extra = (uint8_t)extra; s.day = G.day;
    G.tea.insert(G.tea.begin(), s);
    if (G.tea.size() > 24) G.tea.pop_back();
}
void scandalBreak(int kind) {
    Sim& me = MC();
    const bool first = !(G.csSeen & (1u << CS_SCANDAL));
    G.myScandal = (uint8_t)std::min(100, G.myScandal + 55 + irand(25));
    G.myScandalKind = (uint8_t)kind;
    teaPush(G.mc, kind, 0);
    uint32_t lost = G.followers / 6;
    G.followers -= lost;
    repDown(8);
    addMoodlet(me, "Scandalized", EM_SAD, 2, 720);
    char b[160]; snprintf(b, sizeof b, "SCANDAL! \"%s\" -%u followers. Check The Tea app!", scandalHeadline(G.tea.front()).c_str(), (unsigned)lost);
    toast(b);
    if (first) cutscenePlay(CS_SCANDAL);
}
void npcScandal(int sim, int kind, int extra) { teaPush(sim, kind, extra); }

// ---------------------------------------------------------------- the clock
void crimeOnTravel(int room) {
    if (roomIsHome(room) || G.heat < 30) return;
    if (irand(100) < G.heat / 2) {
        int c = copSim();
        Actor a; a.kind = 0; a.id = (uint16_t)c; a.x = 40 + irand(320); a.tx = a.x; a.y = 190 + irand(40); a.stay = 120;
        a.say = G.heat >= 70 ? "I've got my eye on YOU." : "Just patrolling. Move along."; a.sayT = 3;
        curRoom().actors.push_back(a);
    }
}
void crimeHourly(int hour) { (void)hour; }
void crimeDaily() {
    G.heat = (uint8_t)std::max(0, (int)G.heat - 9);
    if (G.myScandal) {
        G.myScandal = (uint8_t)std::max(0, (int)G.myScandal - 9);
        G.followers -= G.followers / 40;
        if (!G.myScandal) toast("The scandal finally blew over. Phew!");
    }
    if (G.witness >= 0 && G.day > G.witnessDay + 5) G.witness = -1;
    // townie drama for the tabloid
    int n = 1 + irand(2);
    for (int i = 0; i < n; i++) {
        int s = irand((int)G.sims.size());
        if (s == G.mc || s == G.cop || G.sims[s].household || G.sims[s].stage < ST_TEEN) continue;
        int k = irand(4) == 0 ? SC_SECRET : (int[]){SC_SHOPLIFT, SC_TWOTIMING, SC_MELTDOWN, SC_FAKEBAG, SC_VANDAL, SC_DINEDASH}[irand(6)];
        npcScandal(s, k, secretOf(s));
    }
}

// a townie who saw your crime wants hush money; burglars at night
static std::string payUp(int choice, int amt) {
    Sim& me = MC();
    int w = G.witness;
    G.witness = -1;
    if (w < 0 || w >= (int)G.sims.size()) return "";
    char b[200];
    if (choice == 0) {
        if (G.money < amt) { scandalBreak(SCANDAL_FOR[G.witnessCrime % CRIME_COUNT]); return "You can't afford it... so " + G.sims[w].name + " posts EVERYTHING."; }
        G.money -= amt; changeRel(G.mc, w, -15, 0); addMoodlet(me, "Paid Hush Money", EM_TENSE, 1, 240);
        snprintf(b, sizeof b, "You send $%d. %s replies with a zipped-lips emoji. For now.", amt, G.sims[w].name.c_str()); return b;
    }
    if (choice == 1) {
        if (irand(100) < 65) { scandalBreak(SCANDAL_FOR[G.witnessCrime % CRIME_COUNT]); changeRel(G.mc, w, -30, 0); return G.sims[w].name + " wasn't bluffing. The whole town knows by lunch."; }
        changeRel(G.mc, w, -10, 0); return "They never post it. Turns out they were all talk.";
    }
    int di = dirtIndex(w);
    if (di >= 0) { changeRel(G.mc, w, -20, 0); addMoodlet(me, "Checkmate", EM_CONFIDENT, 2, 240); return std::string("\"Funny... I know you ") + SECRETS[secretOf(w)] + ".\" Silence. They delete the photo. Checkmate."; }
    float ch = 0.3f + skillLevel(me, SK_CHARISMA) * 0.06f;
    if (frand() < ch) { changeRel(G.mc, w, -5, 0); return "\"Go ahead. Nobody will believe you.\" ...They back down!"; }
    scandalBreak(SCANDAL_FOR[G.witnessCrime % CRIME_COUNT]); return "They called YOUR bluff instead. It's everywhere now.";
}
static std::string burglar(int choice, int) {
    Sim& me = MC();
    G.burgleDay = G.day;
    char b[200];
    if (choice == 0) {
        float ch = 0.35f + skillLevel(me, SK_FITNESS) * 0.07f;
        if (frand() < ch) { G.money += 100; G.rep = (int16_t)std::min(100, G.rep + 6); addMoodlet(me, "Home Defender", EM_CONFIDENT, 3, 600); return "You flip on the lights and YELL. The burglar trips over the cat bed and runs! The police reward you $100."; }
        int lost = 100 + irand(200); G.money = std::max(0, (int)G.money - lost); addMoodlet(me, "Shaken Up", EM_TENSE, 2, 480);
        snprintf(b, sizeof b, "They shove past you and escape with $%d. You're okay, just rattled.", lost); return b;
    }
    if (choice == 1) { G.money += 50; addMoodlet(me, "Safe & Sound", EM_FINE, 1, 240); return "Officer Reyes arrives in four minutes flat and catches them climbing out the window. +$50 reward!"; }
    int lost = 150 + irand(250); G.money = std::max(0, (int)G.money - lost); addMoodlet(me, "Robbed", EM_SAD, 2, 720);
    snprintf(b, sizeof b, "When you peek out, they're gone... and so is $%d. Maybe it's time for a security system (Mall).", lost); return b;
}
bool crimeMomentMaybe() {
    const int h = (int)(G.minute / 60);
    if (G.witness >= 0 && G.witness < (int)G.sims.size() && G.day > G.witnessDay && h >= 10 && h <= 21 && irand(100) < 30) {
        int w = G.witness;
        int amt = 80 + irand(170);
        char b[220]; snprintf(b, sizeof b, "%s texts you: \"I saw you %s. $%d or EVERYONE finds out.\"", G.sims[w].name.c_str(), CRIME_VERB[G.witnessCrime % CRIME_COUNT], amt);
        ChatThread& th = threadFor(w); th.msgs.push_back({false, "i saw what u did... we need to talk"}); th.unread = true;
        momentCustom("Blackmail!", b, "Pay up", "Refuse", dirtIndex(w) >= 0 ? "Turn the tables" : "Call their bluff", payUp, amt, w);
        return true;
    }
    if (roomIsHome(G.room) && !G.alarm && (h >= 23 || h < 4) && G.burgleDay != G.day && G.money > 300 && irand(100) < 4) {
        momentCustom("Midnight Noise", "CRASH. Someone is downstairs. A shadow in a beanie is stuffing your stuff into a bag!", "Confront them!", "Call the police", "Hide under the covers", burglar, 0, -1);
        return true;
    }
    return false;
}

// ---------------------------------------------------------------- shady socials
void crimeSocialOptions(int t, std::vector<SocialOpt>& v) {
    const Sim& me = MC();
    const Sim& s = G.sims[t];
    if (me.stage < ST_TEEN || s.stage < ST_CHILD || t == G.cop) return;
    if (!s.household) v.push_back({SO_PICKPOCKET, "Pickpocket", SC_MEAN});
    int di = dirtIndex(t);
    if (di < 0) v.push_back({SO_SNOOP, "Snoop Their Phone", SC_MEAN});
    else {
        const Dirt& d = G.dirt[di];
        if (d.state != 2 && (d.state != 1 || G.day >= d.day + 5)) v.push_back({SO_BLACKMAIL, "Blackmail Them", SC_MEAN});
        if (d.state != 2) v.push_back({SO_EXPOSE, "Expose Their Secret", SC_MEAN});
        if (d.state == 0) v.push_back({SO_KEEPSECRET, "Promise to Keep Secret", SC_FRIENDLY});
    }
}
void crimeDoSocial(int id, int t) {
    Sim& me = MC();
    Sim& s = G.sims[t];
    int di = dirtIndex(t);
    switch (id) {
    case SO_PICKPOCKET: {
        float ch = 0.62f + mischiefLevel() * 0.03f - (copHere() ? 0.3f : 0.f);
        if (frand() < ch) { crimeAttempt(CRIME_PICKPOCKET, t); return; }
        say(t, "HEY! Hands OFF my purse!"); changeRel(G.mc, t, -25, -10); repDown(5); heatUp(8);
        if (copHere()) busted(CRIME_PICKPOCKET);
        else { addMoodlet(me, "Caught Red-Handed", EM_TENSE, 2, 240); toast(s.name + " caught you! (-Reputation)"); }
        return;
    }
    case SO_SNOOP: {
        float ch = 0.5f + mischiefLevel() * 0.04f;
        say(G.mc, "Ooh, can I see that pic?");
        if (frand() < ch) {
            G.dirt.push_back({(uint16_t)t, (uint8_t)secretOf(t), 0, G.day});
            toast("You found out: " + s.name + " " + SECRETS[secretOf(t)] + "!");
            addMoodlet(me, "Know a Secret", EM_PLAYFUL, 1, 240); mischiefUp(2.f); sfxYay();
        } else { say(t, "Are you reading my PHONE?!"); changeRel(G.mc, t, -18, -6); addMoodlet(me, "Awkward...", EM_TENSE, 1, 120); }
        return;
    }
    case SO_BLACKMAIL: {
        if (di < 0) return;
        Dirt& d = G.dirt[di];
        say(G.mc, "So... I know your little secret.");
        if (irand(100) < 25) {
            say(t, "Go ahead. And I'll tell everyone YOU tried this.");
            changeRel(G.mc, t, -35, -20); d.state = 2;
            npcScandal(t, SC_SECRET, d.secret); scandalBreak(SC_BLACKMAILER);
            return;
        }
        int amt = 60 + irand(140) + (s.isAdult() ? 50 : 0);
        G.money += amt; d.state = 1; d.day = G.day;
        say(t, irand(2) ? "...fine. Take it. Just keep quiet." : "You're unbelievable. Here.");
        changeRel(G.mc, t, -30, -15); heatUp(5); mischiefUp(2.f);
        char b[96]; snprintf(b, sizeof b, "%s paid you $%d to keep quiet.", s.name.c_str(), amt); toast(b);
        addMoodlet(me, "Power Move", EM_CONFIDENT, 1, 180);
        return;
    }
    case SO_EXPOSE: {
        if (di < 0) return;
        Dirt& d = G.dirt[di];
        d.state = 2;
        uint32_t f = 120 + G.followers / 12;
        G.followers += f;
        npcScandal(t, SC_SECRET, d.secret);
        say(G.mc, "Everybody! Guess what?!"); say(t, "You did NOT just say that!!");
        changeRel(G.mc, t, -40, -20); repDown(6);
        Post p; p.sim = (int16_t)G.mc; p.text = "the TEA: " + s.name + " " + SECRETS[d.secret] + " lol"; p.likes = 200 + irand(800); p.kind = 0;
        G.posts.insert(G.posts.begin(), p);
        char b[96]; snprintf(b, sizeof b, "You spilled the tea on %s. +%u followers.", s.name.c_str(), (unsigned)f); toast(b);
        return;
    }
    case SO_KEEPSECRET:
        if (di < 0) return;
        G.dirt[di].state = 3;
        say(G.mc, "Your secret's safe with me. Promise."); say(t, "Wait, you KNEW? ...Thank you. Seriously.");
        changeRel(G.mc, t, 14, 0); G.rep = (int16_t)std::min(100, G.rep + 3);
        return;
    }
}

// ---------------------------------------------------------------- The Tea (tabloid app)
static const Btn tBtn[4] = {{6, 34, 150, 34}, {164, 34, 150, 34}, {6, 74, 150, 34}, {164, 74, 150, 34}};
static Btn rowSpread(int i) { return Btn{180, 128 + i * 26.f, 64, 22}; }
static Btn rowDefend(int i) { return Btn{250, 128 + i * 26.f, 64, 22}; }
static std::string g_teaMsg;

static int othersShown(int out[3]) {
    int n = 0;
    for (size_t i = 0; i < G.tea.size() && n < 3; i++) if (G.tea[i].sim != G.mc && !G.tea[i].handled) out[n++] = (int)i;
    return n;
}
void teaAppUpdate(const Input& in) {
    if (!in.touch) return;
    Sim& me = MC();
    if (G.myScandal && G.damageDay != G.day) {
        for (int k = 0; k < 4; k++) {
            if (!tBtn[k].in(in.tp)) continue;
            G.damageDay = G.day;
            int dv = 0;
            switch (k) {
            case 0: { float ch = 0.4f + skillLevel(me, SK_CHARISMA) * 0.06f; if (frand() < ch) { dv = -45; G.followers += 50; g_teaMsg = "Your tearful apology video went viral - in a GOOD way."; } else { dv = 10; g_teaMsg = "The apology felt fake. The comments are brutal."; } break; }
            case 1: if (irand(2)) { dv = -50; g_teaMsg = "\"Fake news!\" ...and somehow, people believe you."; } else { dv = 25; repDown(5); g_teaMsg = "Then someone posted receipts. Yikes."; } break;
            case 2: {
                int pick = -1;
                for (const Rel& r : G.rels) { int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1; if (o >= 0 && o != G.cop && !G.sims[o].household && G.sims[o].stage >= ST_TEEN && irand(2)) { pick = o; break; } }
                if (pick < 0) { g_teaMsg = "There's nobody to blame. Awkward."; G.damageDay = 0xFFFF; return; }
                dv = -45; repDown(8); changeRel(G.mc, pick, -40, -20); npcScandal(pick, G.myScandalKind, 0);
                g_teaMsg = "You pinned it all on " + G.sims[pick].name + ". They will NEVER forgive you.";
                break;
            }
            default: {
                float ch = 0.25f + skillLevel(me, SK_CHARISMA) * 0.05f + (me.hasTrait(TR_GLAMOROUS) || me.hasTrait(TR_OUTGOING) ? 0.25f : 0.f);
                if (frand() < ch) { dv = -30; uint32_t f = G.followers / 3 + 100; G.followers += f; g_teaMsg = "You turned the scandal into a BRAND. Iconic. +" + std::to_string(f) + " followers."; }
                else { dv = 15; g_teaMsg = "The spin did NOT land. Now it's a meme."; }
                break;
            }
            }
            G.myScandal = (uint8_t)clampv((int)G.myScandal + dv, 0, 100);
            if (!G.myScandal) g_teaMsg += " The scandal is OVER!";
            sfxTap();
            return;
        }
    }
    int idx[3]; int n = othersShown(idx);
    for (int i = 0; i < n; i++) {
        Scandal& s = G.tea[idx[i]];
        if (s.sim < 0 || s.sim >= (int)G.sims.size()) continue;
        if (rowSpread(i).in(in.tp)) { s.handled = 1; changeRel(G.mc, s.sim, -10, 0); G.followers += 30; G.cliqueStand[0] = (uint8_t)std::min(100, G.cliqueStand[0] + 3); g_teaMsg = "You reposted it with a laughing emoji. Messy!"; sfxTap(); return; }
        if (rowDefend(i).in(in.tp)) { s.handled = 1; changeRel(G.mc, s.sim, 8, 0); G.rep = (int16_t)std::min(100, G.rep + 2); g_teaMsg = "You stood up for " + G.sims[s.sim].name + ". They noticed."; sfxTap(); return; }
    }
}
void teaAppDrawTop(float t) {
    grad(0, 0, TOP_W, TOP_H, C(255, 240, 246), C(255, 214, 232));
    // masthead
    rect(0, 0, TOP_W, 40, C(255, 46, 140));
    rect(0, 40, TOP_W, 2, C(UI_INK));
    textOutline("THE TEA", 12, 2, 1.0f, C(255, 255, 255), C(UI_INK), 0, 1.4f);
    textOutline("Starlight Bay's #1 gossip mag", 392, 6, 0.36f, C(255, 236, 120), C(UI_INK), 2, 0.8f);
    char b[96]; snprintf(b, sizeof b, "Rap sheet: %d   Secrets known: %d", G.rapSheet, (int)G.dirt.size());
    textOutline(b, 392, 22, 0.36f, C(255, 255, 255), C(UI_INK), 2, 0.8f);
    // wanted level
    for (int i = 0; i < 5; i++) { float x = 240 + i * 14; sparkle(x, 50 + 4, 6, C(i < heatStars() ? RGB{255, 200, 40} : RGB{220, 200, 210})); }
    text("Heat", 232, 46, 0.36f, C(UI_INK), 2);
    float y = 64;
    for (size_t i = 0; i < G.tea.size() && i < 5; i++) {
        const Scandal& s = G.tea[i];
        bool me = s.sim == G.mc;
        flashPanel(8, y, 384, 30, me ? RGB{255, 210, 226} : RGB{255, 255, 255}, 6, 245);
        if (s.sim >= 0 && s.sim < (int)G.sims.size()) drawSimPortrait(G.sims[s.sim], 24, y + 15, 12, t);
        auto lines = wrapText(scandalHeadline(s), 0.4f, 340);
        for (size_t k = 0; k < lines.size() && k < 2; k++) text(lines[k].c_str(), 42, y + 2 + k * 13.f, 0.4f, C(me ? RGB{200, 20, 90} : UI_INK));
        y += 34;
    }
    if (G.tea.empty()) textOutline("No tea yet... suspiciously quiet.", 200, 110, 0.55f, C(255, 46, 140), C(255, 255, 255), 1);
    if (G.myScandal) { flashPanel(110, 232 - 22, 180, 18, RGB{255, 46, 140}, 6); snprintf(b, sizeof b, "YOUR SCANDAL: %d%%", G.myScandal); textOutline(b, 200, 211, 0.4f, C(255, 255, 255), C(UI_INK), 1); }
}
void teaAppDrawBottom(float t) {
    (void)t;
    if (G.myScandal) {
        static const char* L[4] = {"Apology Video", "Deny Everything", "Blame Someone", "Spin It!"};
        bool ok = G.damageDay != G.day;
        textOutline(ok ? "DAMAGE CONTROL (once a day):" : "Done for today - let it simmer.", 160, 18, 0.42f, C(255, 236, 120), C(UI_INK), 1);
        for (int k = 0; k < 4; k++) button(tBtn[k], L[k], false, ok ? k % 3 : 1, 0.44f);
    } else textOutline("You're scandal-free. For now.", 160, 50, 0.5f, C(255, 255, 255), C(UI_INK), 1);
    int idx[3]; int n = othersShown(idx);
    if (n) textOutline("Town gossip:", 8, 112, 0.42f, C(255, 255, 255), C(UI_INK));
    for (int i = 0; i < n; i++) {
        const Scandal& s = G.tea[idx[i]];
        std::string nm = s.sim >= 0 && s.sim < (int)G.sims.size() ? G.sims[s.sim].name : "?";
        flashPanel(6, 128 + i * 26.f, 168, 22, RGB{255, 255, 255}, 5, 240);
        text(nm.c_str(), 12, 130 + i * 26.f, 0.42f, C(UI_INK));
        button(rowSpread(i), "Spread", false, 0, 0.38f);
        button(rowDefend(i), "Defend", false, 1, 0.38f);
    }
    if (!g_teaMsg.empty()) { auto ls = wrapText(g_teaMsg, 0.4f, 300); for (size_t k = 0; k < ls.size() && k < 2; k++) textOutline(ls[k].c_str(), 160, 186 + k * 13.f, 0.4f, C(255, 255, 255), C(UI_INK), 1, 0.8f); }
}
