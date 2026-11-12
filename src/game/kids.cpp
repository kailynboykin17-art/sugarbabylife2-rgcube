#include "kids.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "extras.hpp"
#include "goals.hpp"
#include "audio.hpp"
#include <algorithm>
#include <cstdio>

namespace {
NpcLife& K(int s) { if (G.life.size() < G.sims.size()) G.life.resize(G.sims.size()); return G.life[s]; }
bool valid(int s) { return s >= 0 && s < (int)G.sims.size(); }
bool mine(int t) { return valid(t) && (G.sims[t].household || isFamily(G.mc, t)); }
void up(uint8_t& v, int d) { v = (uint8_t)std::max(0, std::min(100, (int)v + d)); }
void tilt(int8_t& v, int d) { v = (int8_t)std::max(-100, std::min(100, (int)v + d)); }
enum Mile { MI_WORD, MI_STEPS, MI_POTTY, MI_IMAG, MI_SENTENCE, MI_READ, MI_BIKE, MI_FRIEND, MI_COUNT };
const char* const MILE_NAME[MI_COUNT] = {"First Word", "First Steps", "Potty Trained", "Big Imagination", "First Sentence", "Can Read!", "Rode a Bike!", "Made a Best Friend"};
const char* const MILE_LINE[MI_COUNT] = {"said their very first word: \"%s\"!", "took their first wobbly steps!", "is officially potty trained! No more diapers!",
                                         "has the BIGGEST imagination. Today the couch was a pirate ship.", "said a whole sentence! \"I love you, %s!\"",
                                         "read a whole book all by themselves!", "rode a bike with no training wheels!", "made a best friend at school!"};
void milestone(int t, int m) {
    NpcLife& k = K(t);
    if (k.miles & (1u << m)) return;
    k.miles |= (uint16_t)(1u << m);
    Sim& s = G.sims[t];
    char b[200];
    static const char* const WORDS[4] = {"mama", "dada", "kitty", "ba-ba"};
    snprintf(b, sizeof b, MILE_LINE[m], m == MI_WORD ? WORDS[(t + G.day) % 4] : MC().name.c_str());
    toast(s.name + " " + b);
    rewardPopup(std::string("Milestone: ") + MILE_NAME[m], s.name + " " + b);
    addMemory(MEM_FAMILY, t, G.mc, s.name + ": " + MILE_NAME[m] + "!");
    addMoodlet(MC(), "Proud Parent", EM_HAPPY, 2, 480);
    addMoodlet(s, "I Did It!", EM_HAPPY, 2, 240);
    G.satisfaction += 150;
    sfxYay();
}
void check(int t) {
    const Sim& s = G.sims[t];
    NpcLife& k = K(t);
    if (k.talk >= 35) milestone(t, MI_WORD);
    if (k.motor >= 40 && s.stage >= ST_INFANT) milestone(t, MI_STEPS);
    if (k.potty >= 90 && s.stage >= ST_TODDLER) milestone(t, MI_POTTY);
    if (k.imag >= 70) milestone(t, MI_IMAG);
    if (k.talk >= 75 && s.stage >= ST_TODDLER) milestone(t, MI_SENTENCE);
    if (k.talk >= 95 && s.stage >= ST_CHILD) milestone(t, MI_READ);
    if (k.motor >= 85 && s.stage >= ST_CHILD) milestone(t, MI_BIKE);
    if (k.social >= 70 && s.stage >= ST_CHILD) milestone(t, MI_FRIEND);
}
void say2(int a, const char* line) { say(a, line); }
}   // namespace

void kidsOptions(int t, std::vector<SocialOpt>& v) {
    if (!valid(t) || !MC().isAdult() && MC().stage < ST_TEEN) return;
    const Sim& s = G.sims[t];
    if (!mine(t) && s.stage > ST_TODDLER) return;
    switch (s.stage) {
    case ST_INFANT:
        v.push_back({SO_DIAPER, "Change Diaper", SC_FAMILY});
        v.push_back({SO_TUMMYTIME, "Tummy Time", SC_FAMILY});
        v.push_back({SO_LULLABY, "Sing Lullaby", SC_FAMILY});
        v.push_back({SO_TEACHTALK, "Baby Talk", SC_FAMILY});
        break;
    case ST_TODDLER:
        v.push_back({SO_TEACHTALK, "Teach Words", SC_FAMILY});
        v.push_back({SO_TEACHWALK, "Practice Walking", SC_FAMILY});
        v.push_back({SO_POTTY, "Potty Training", SC_FAMILY});
        v.push_back({SO_PICTUREBOOK, "Read Picture Book", SC_FAMILY});
        v.push_back({SO_KIDBATH, "Bubble Bath Time", SC_FAMILY});
        break;
    case ST_CHILD: case ST_PRETEEN:
        v.push_back({SO_BEDSTORY, "Bedtime Story", SC_FAMILY});
        v.push_back({SO_BIKE, "Teach to Ride a Bike", SC_FAMILY});
        v.push_back({SO_CATCH, "Play Catch", SC_FAMILY});
        v.push_back({SO_PLAYDATE, "Set Up a Playdate", SC_FAMILY});
        v.push_back({SO_PRAISE, "Praise Them", SC_FAMILY});
        v.push_back({SO_RULES, "Set House Rules", SC_FAMILY});
        v.push_back({SO_ALLOWANCE, "Give Allowance ($10)", SC_FAMILY});
        break;
    case ST_TEEN:
        if (!MC().isAdult()) break;
        v.push_back({SO_FEELINGS, "Talk About Feelings", SC_FAMILY});
        v.push_back({SO_PRAISE, "Praise Them", SC_FAMILY});
        v.push_back({SO_RULES, "Set a Curfew", SC_FAMILY});
        v.push_back({SO_GROUND, "Ground Them", SC_FAMILY});
        v.push_back({SO_ALLOWANCE, "Give Allowance ($20)", SC_FAMILY});
        break;
    default: break;
    }
}

bool kidsDoSocial(int id, int t) {
    if (id < SO_DIAPER || id > SO_PLAYDATE || !valid(t)) return false;
    Sim& me = MC();
    Sim& s = G.sims[t];
    NpcLife& k = K(t);
    const bool fam = me.hasTrait(TR_FAMILY), cranky = s.needs[N_HUNGER] < 25 || s.needs[N_ENERGY] < 20;
    int bonus = (fam ? 4 : 0) + (cranky ? -5 : 0);
    auto fin = [&](const char* a, const char* b) { say(G.mc, a); if (b) say(t, b); };
    switch (id) {
    case SO_DIAPER: s.needs[N_HYGIENE] = 100; s.needs[N_BLADDER] = 100; fin("Fresh and clean!", "*happy kicks*"); changeRel(G.mc, t, 3, 0); up(k.potty, 3); break;
    case SO_TUMMYTIME: up(k.motor, 9 + bonus); s.needs[N_FUN] = std::min(100.f, s.needs[N_FUN] + 15); fin("Look at you go!", "*wiggles*"); changeRel(G.mc, t, 3, 0); break;
    case SO_LULLABY: s.needs[N_ENERGY] = std::min(100.f, s.needs[N_ENERGY] + 40); up(k.talk, 5); fin("Twinkle twinkle...", "*yawn*"); changeRel(G.mc, t, 4, 0); tilt(k.warm, 3); skillGain(me, SK_CHARISMA, 0.5f); break;
    case SO_TEACHTALK: up(k.talk, (s.stage == ST_INFANT ? 7 : 11) + bonus + skillLevel(me, SK_CHARISMA) / 2); fin(s.stage == ST_INFANT ? "Can you say mama?" : "This is a... BUNNY!", s.stage == ST_INFANT ? "Ba-ba-ba!" : "Bun-nyyy!"); changeRel(G.mc, t, 3, 0); break;
    case SO_TEACHWALK: up(k.motor, 12 + bonus + skillLevel(me, SK_FITNESS) / 2); s.needs[N_ENERGY] = std::max(0.f, s.needs[N_ENERGY] - 10); fin("Come to me! You can do it!", "*toddles*"); changeRel(G.mc, t, 3, 0); break;
    case SO_POTTY:
        if (cranky) { fin("Potty time?", "NO! NO POTTY!"); addMoodlet(me, "Toddler Standoff", EM_TENSE, 1, 90); tilt(k.strict, 2); break; }
        up(k.potty, 14 + bonus); s.needs[N_BLADDER] = 100; fin("Big kid potty! Yay!", "I did it!!"); changeRel(G.mc, t, 2, 0); break;
    case SO_PICTUREBOOK: up(k.talk, 8 + bonus); up(k.imag, 8 + bonus); s.needs[N_FUN] = std::min(100.f, s.needs[N_FUN] + 20); fin("And the bunny hopped... all the way home!", "Again! AGAIN!"); changeRel(G.mc, t, 4, 0); tilt(k.warm, 2); break;
    case SO_KIDBATH: s.needs[N_HYGIENE] = 100; s.needs[N_FUN] = std::min(100.f, s.needs[N_FUN] + 20); up(k.imag, 4); fin("Bubble beard!", "*splash splash*"); changeRel(G.mc, t, 3, 0); break;
    case SO_BEDSTORY: s.needs[N_ENERGY] = std::min(100.f, s.needs[N_ENERGY] + 30); up(k.talk, 7 + bonus); up(k.imag, 9 + bonus); fin("Once upon a time, in Starlight Bay...", "...and then?!"); changeRel(G.mc, t, 5, 0); tilt(k.warm, 3); break;
    case SO_BIKE: {
        bool ok = irand(100) < 45 + k.motor / 2 + skillLevel(me, SK_FITNESS) * 3;
        up(k.motor, ok ? 12 : 5); s.needs[N_FUN] = std::min(100.f, s.needs[N_FUN] + 20);
        if (ok) fin("Pedal pedal pedal!", "I'M DOING IT!!"); else { fin("It's okay! Try again!", "*wobble* ...ow."); }
        changeRel(G.mc, t, 4, 0); skillGain(s, SK_FITNESS, 1.f); break;
    }
    case SO_CATCH: up(k.motor, 6); up(k.social, 4); s.needs[N_FUN] = std::min(100.f, s.needs[N_FUN] + 30); fin("Go long!", "Got it!!"); changeRel(G.mc, t, 4, 0); skillGain(s, SK_FITNESS, 0.6f); break;
    case SO_PLAYDATE: {
        int friendKid = -1;
        for (size_t i = 0; i < G.sims.size(); i++) if ((int)i != t && !G.sims[i].household && G.sims[i].stage >= ST_CHILD && G.sims[i].stage <= ST_PRETEEN) { friendKid = (int)i; if (irand(3) == 0) break; }
        if (friendKid < 0) { fin("Hmm, all the other kids are busy today.", nullptr); break; }
        Actor a; a.kind = 0; a.id = (uint16_t)friendKid; a.x = 380; a.tx = 280; a.y = 214; a.stay = 120; a.say = "Can " + s.name + " come play?!"; a.sayT = 3;
        curRoom().actors.push_back(a);
        changeRel(t, friendKid, 15, 0); up(k.social, 14); s.needs[N_SOCIAL] = 100; s.needs[N_FUN] = std::min(100.f, s.needs[N_FUN] + 25);
        toast(G.sims[friendKid].name + " came over for a playdate with " + s.name + "!");
        break;
    }
    case SO_PRAISE: tilt(k.warm, 6); up(k.disc, 2); addMoodlet(s, "Mom/Dad Is Proud", EM_CONFIDENT, 2, 240); fin("I'm SO proud of you.", "Really?! *beams*"); changeRel(G.mc, t, 6, 0); break;
    case SO_RULES:
        tilt(k.strict, 6); up(k.disc, 10);
        if (s.stage == ST_TEEN && k.warm < 10) { fin("Home by ten. No exceptions.", "Ugh, you're SO unfair!"); changeRel(G.mc, t, -4, 0); }
        else { fin("Homework before games, okay?", "...okay. Fine."); changeRel(G.mc, t, -1, 0); }
        break;
    case SO_GROUND: tilt(k.strict, 10); up(k.disc, 12); s.gradePts = std::min(100.f, s.gradePts + 3); fin("You're grounded. One week.", "I HATE IT HERE!"); changeRel(G.mc, t, -10, 0); addMoodlet(s, "Grounded", EM_ANGRY, 2, 720); break;
    case SO_ALLOWANCE: {
        int amt = s.stage == ST_TEEN ? 20 : 10;
        if (G.money < amt) { fin("Sorry, wallet's empty this week.", "Aww."); break; }
        G.money -= amt; tilt(k.warm, 2); fin("Here's your allowance. Spend it wisely!", "Candy time!!"); changeRel(G.mc, t, 4, 0); addMoodlet(s, "Got Allowance", EM_HAPPY, 1, 240); break;
    }
    case SO_FEELINGS: {
        tilt(k.warm, 8); s.needs[N_SOCIAL] = std::min(100.f, s.needs[N_SOCIAL] + 30);
        static const char* const T[4] = {"...people at school can be so fake.", "I think I like someone. Don't be weird.", "I'm stressed about grades.", "Can I tell you something? I'm kind of sad lately."};
        fin("Hey. How are you, really?", T[irand(4)]); changeRel(G.mc, t, 8, 0); addMoodlet(s, "Feels Heard", EM_HAPPY, 2, 360); break;
    }
    }
    me.needs[N_SOCIAL] = std::min(100.f, me.needs[N_SOCIAL] + 5);
    if (fam) me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 6);
    check(t);
    return true;
}

void kidsHourly(int hour) {
    if (hour < 7 || hour > 21) return;
    for (size_t i = 0; i < G.sims.size(); i++) {
        Sim& s = G.sims[i];
        if (!s.household || (int)i == G.mc || s.stage > ST_PRETEEN) continue;
        NpcLife& k = K((int)i);
        // little by little they grow on their own, faster with a good routine
        float avg = 0; for (float n : s.needs) avg += n; avg /= N_COUNT;
        if (avg > 60 && irand(4) == 0) { up(k.motor, 1); up(k.imag, 1); }
        // tantrums when they're cranky
        if (s.stage == ST_TODDLER && (s.needs[N_HUNGER] < 20 || s.needs[N_ENERGY] < 15 || s.needs[N_FUN] < 15) && irand(5) == 0) {
            say((int)i, irand(2) ? "WAAAAAH!!" : "NO NO NO NO!");
            toast(s.name + " is having a TANTRUM! Feed them, nap time, or play!");
            addMoodlet(MC(), "Toddler Tantrum", EM_TENSE, 1, 60);
        }
        // kids with good discipline do homework themselves
        if (s.stage >= ST_CHILD && isWeekday(G.day) && hour == 17 && !s.homework && irand(100) < 30 + k.disc / 2) {
            s.homework = true; s.gradePts = std::min(100.f, s.gradePts + 2);
            say((int)i, "I did my homework already!");
        }
        if (s.stage >= ST_CHILD && irand(20) == 0) up(k.social, 2);
        check((int)i);
    }
}

void kidsOnAgeUp(int t) {
    if (!valid(t)) return;
    Sim& s = G.sims[t];
    NpcLife& k = K(t);
    if (s.stage != ST_CHILD && s.stage != ST_TEEN) return;
    // the way they were raised picks the new trait
    uint8_t pick = TR_NONE;
    if (k.warm >= 30 && k.strict >= 20) pick = k.disc >= 50 ? TR_NEAT : TR_AMBITIOUS;
    else if (k.warm >= 30) pick = k.imag >= 60 ? TR_CREATIVE : (k.social >= 50 ? TR_OUTGOING : TR_KIND);
    else if (k.strict >= 30) pick = k.warm <= -10 ? TR_HOTHEAD : TR_PERFECTIONIST;
    else if (k.imag >= 60) pick = TR_CREATIVE;
    else if (k.talk >= 70) pick = TR_BOOKWORM;
    else if (k.motor >= 70) pick = TR_ACTIVE;
    if (pick == TR_NONE || s.hasTrait(pick)) return;
    for (int i = 2; i >= 0; i--) if (s.traits[i] != TR_NONE) { s.traits[i] = pick; break; }
    if (s.traits[0] == TR_NONE) s.traits[0] = pick;
    toast(s.name + " grew up " + TRAIT_NAMES[pick] + " - just how you raised them!");
}

std::string kidsSummary(int t) {
    if (!valid(t) || G.sims[t].stage > ST_TEEN) return "";
    const NpcLife& k = K(t);
    int n = 0; for (int m = 0; m < MI_COUNT; m++) n += (k.miles >> m) & 1;
    char b[48]; snprintf(b, sizeof b, "%d/%d milestones", n, (int)MI_COUNT);
    return b;
}
