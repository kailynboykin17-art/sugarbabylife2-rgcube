#include "lockup.hpp"
// Health (fictional, cozy) + rabbit-hole places on the third map page.
#include "travelcourt.hpp"
#include "health.hpp"
#include "lifestory.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include <algorithm>
#include <cstdio>
void toast(const std::string& m);
bool askText(const char* hint, std::string& out, int maxLen, bool numeric);

namespace {
struct IllInfo { const char* name; const char* mood; Emotion emo; uint8_t minD, maxD; uint8_t medCost; float energy, hunger; bool contagious; const char* lines[2]; };
const IllInfo IL[IL_COUNT] = {
    {"Healthy", "", EM_FINE, 0, 0, 0, 0, 0, false, {"", ""}},
    {"Common Cold", "Sniffly & Sneezy", EM_UNCOMFORTABLE, 2, 3, 15, 1.f, 0, true, {"*achoo!*", "Ugh, my nose won't stop running."}},
    {"Flu", "Achy & Feverish", EM_SAD, 3, 5, 35, 3.f, 0, true, {"*cough cough*", "Everything hurts... blanket burrito time."}},
    {"Fever", "Burning Up", EM_UNCOMFORTABLE, 2, 3, 25, 2.f, 0, false, {"Is it hot in here or is it me?", "I need a cold cloth..."}},
    {"Allergies", "Itchy Eyes", EM_TENSE, 1, 3, 10, 0.5f, 0, false, {"*sneeze* Pollen is my enemy.", "My eyes are SO itchy."}},
    {"Stomach Bug", "Queasy", EM_UNCOMFORTABLE, 1, 2, 20, 1.f, 3.f, true, {"I can't even look at food right now.", "My tummy is doing flips..."}},
    {"Fatigue", "Totally Run Down", EM_BORED, 2, 4, 0, 4.f, 0, false, {"*yawn* I could sleep for a week.", "I'm running on empty."}},
    {"Minor Injury", "Sprained & Sore", EM_TENSE, 3, 5, 40, 1.f, 0, false, {"Ow ow ow, careful with my ankle!", "I'll be back on my feet soon."}},
    {"Headache Spell", "Pounding Head", EM_TENSE, 1, 1, 8, 1.f, 0, false, {"Shh... my head.", "Too loud, too bright, too much."}},
};
bool ok(int s) { return s >= 0 && s < (int)G.sims.size(); }
void fitH() { size_t n = G.sims.size(); if (G.illness.size() < n) G.illness.resize(n, 0); if (G.diag.size() < n) G.diag.resize(n, 0); if (G.sickDays.size() < n) G.sickDays.resize(n, 0); }
bool atHome(int s) { return ok(s) && (G.sims[s].household || s == G.mc); }
}

const char* illnessName(int il) { return IL[il % IL_COUNT].name; }
int illnessOf(int s) { fitH(); return ok(s) && G.sickDays[s] ? G.illness[s] % IL_COUNT : IL_NONE; }
bool isSick(int s) { return illnessOf(s) != IL_NONE; }
bool diagnosed(int s) { fitH(); return ok(s) && G.diag[s]; }
std::string healthStatus(int s) {
    if (simDead(s)) return "Passed away";
    if (!isSick(s)) return "Healthy";
    if (!diagnosed(s)) return "Feeling sick (not diagnosed)";
    char b[64]; snprintf(b, sizeof b, "%s (%d day%s left)", illnessName(illnessOf(s)), G.sickDays[s], G.sickDays[s] == 1 ? "" : "s"); return b;
}

void catchIllness(int s, int il) {
    fitH();
    if (!ok(s) || simDead(s) || isSick(s) || G.sims[s].stage < ST_TODDLER) return;
    if (il <= 0 || il >= IL_COUNT) {
        int season = seasonOf(G.day);   // spring = allergies, winter = colds & flu
        static const uint8_t POOL[4][6] = {{IL_ALLERGY, IL_ALLERGY, IL_COLD, IL_HEADACHE, IL_FATIGUE, IL_STOMACH}, {IL_FEVER, IL_STOMACH, IL_FATIGUE, IL_HEADACHE, IL_ALLERGY, IL_COLD},
                                         {IL_COLD, IL_FLU, IL_FATIGUE, IL_HEADACHE, IL_STOMACH, IL_FEVER}, {IL_COLD, IL_FLU, IL_FLU, IL_FEVER, IL_COLD, IL_FATIGUE}};
        il = POOL[season % 4][irand(6)];
    }
    const IllInfo& I = IL[il];
    G.illness[s] = (uint8_t)il; G.diag[s] = 0; G.sickDays[s] = (uint8_t)(I.minD + irand(I.maxD - I.minD + 1));
    Sim& o = G.sims[s];
    addMoodlet(o, I.mood, I.emo, 2, (uint16_t)(1440 * G.sickDays[s]));
    if (atHome(s)) {
        toast(o.name + (s == G.mc ? " (you)" : "") + " isn't feeling well. Visit the Clinic on the map (page 3) to get checked out.");
        for (size_t j = 0; j < G.sims.size(); j++) if ((int)j != s && G.sims[j].household && !simDead((int)j) && G.sims[j].stage >= ST_CHILD) {
            std::string m = "Worried About " + o.name.substr(0, o.name.find(' '));
            addMoodlet(G.sims[j], m.c_str(), EM_TENSE, 1, 480);
        }
    }
}

void healthTreat(int s, int level) {
    fitH(); if (!isSick(s)) return;
    Sim& o = G.sims[s];
    if (level >= 1) { G.sickDays[s] = 0; G.illness[s] = 0; o.moodlets.erase(std::remove_if(o.moodlets.begin(), o.moodlets.end(), [&](const Moodlet& m) { for (const IllInfo& I : IL) if (I.mood[0] && m.name == I.mood) return true; return false; }), o.moodlets.end()); addMoodlet(o, "Good as New", EM_HAPPY, 2, 360); return; }
    if (G.sickDays[s] > 1) G.sickDays[s] = 1;
    addMoodlet(o, "Medicated & Resting", EM_FINE, 1, 720);
}

void healthHourly(int hour) {
    fitH();
    for (size_t i = 0; i < G.sims.size(); i++) {
        int s = (int)i; if (!atHome(s) || !isSick(s)) continue;
        const IllInfo& I = IL[illnessOf(s)];
        Sim& o = G.sims[i];
        o.needs[N_ENERGY] = std::max(0.f, o.needs[N_ENERGY] - I.energy);
        o.needs[N_HUNGER] = std::min(100.f, std::max(0.f, o.needs[N_HUNGER] - I.hunger));
        if (hour >= 8 && hour <= 21 && irand(6) == 0) say(s, I.lines[irand(2)]);
    }
}

bool healthDaily(int s) {
    fitH();
    if (!ok(s) || simDead(s)) return false;
    Sim& o = G.sims[s];
    if (isSick(s)) {
        if (o.needs[N_ENERGY] > 75 && G.sickDays[s] > 1 && irand(2)) G.sickDays[s]--;   // rest helps
        if (--G.sickDays[s] == 0) { G.illness[s] = 0; if (atHome(s)) { toast(o.name + " is feeling better!"); addMoodlet(o, "Feeling Better", EM_HAPPY, 1, 360); } return false; }
        // sick day: household kids skip school, grown-ups call in (no penalty)
        if (o.household && isWeekday(G.day)) { o.schoolToday = true; if (s != G.mc) o.workedToday = true; }
        if (s == G.mc && isWeekday(G.day) && (o.career != CR_NONE || (o.stage >= ST_CHILD && o.stage <= ST_TEEN))) toast("You're sick! Map > page 3 > Workplace/School to call in sick, or power through.");
        // contagion at home
        if (IL[illnessOf(s)].contagious && o.household) for (size_t j = 0; j < G.sims.size(); j++) if ((int)j != s && G.sims[j].household && irand(100) < 12) catchIllness((int)j, illnessOf(s));
        if (G.aging && o.stage == ST_ELDER && s != G.mc && irand(100) < 3) return true;   // lifestory handles the passing
    } else if (irand(1000) < 14) catchIllness(s, -1);
    return false;
}

// ================================================================ rabbit holes
const char* rabbitHoleName(int k) {
    static const char* N[RH_COUNT] = {"Sunny Clinic", "Starlight General", "School", "Workplace", "Police Station", "City Hall", "Bay Bank", "Post Office", "Travel Agency", "Courthouse"};
    return N[k % RH_COUNT];
}
std::vector<std::pair<std::string, int>> rabbitHoleOptions(int k) {
    std::vector<std::pair<std::string, int>> v;
    Sim& me = MC(); const bool sick = isSick(G.mc);
    char b[80];
    switch (k) {
    case 0:
        v.push_back({"Doctor Visit & Diagnosis ($40)", 0});
        if (sick) { snprintf(b, sizeof b, "Get Medication ($%d)", diagnosed(G.mc) ? IL[illnessOf(G.mc)].medCost : 25); v.push_back({b, 1}); }
        v.push_back({"Flu Shot ($20)", 2}); v.push_back({"Bring a Sick Family Member ($40)", 3}); break;
    case 1:
        v.push_back({"Emergency Room ($150, full cure)", 0}); v.push_back({"Visit the Newborns", 1});
        if (me.stage >= ST_TEEN) v.push_back({"Volunteer Shift", 2});
        break;
    case 2:
        if (me.stage >= ST_CHILD && me.stage <= ST_TEEN) { v.push_back({"Extra Study Hall", 0}); v.push_back({sick ? "Call In Sick (excused)" : "Join an After-School Club", 1}); }
        if (me.isAdult()) v.push_back({"Parent-Teacher Conference", 2});
        for (auto& o : boardingSchoolOptions()) v.push_back(o);
        if (v.empty()) v.push_back({"(Nothing for you here yet)", -1});
        break;
    case 3:
        if (me.career == CR_NONE) { v.push_back({"(Get a job first - check the phone)", -1}); break; }
        v.push_back({"Work Overtime (+pay)", 0}); v.push_back({sick ? "Call In Sick (no penalty)" : "Ask for a Raise", 1}); v.push_back({"Office Potluck", 2}); break;
    case 4: v.push_back({"Police Ride-Along", 0}); v.push_back({"Check Lost & Found", 1}); v.push_back({"Community Safety Talk", 2}); break;
    case 5: v.push_back({"Rename Your Household", 0}); v.push_back({"Town Hall Meeting", 1}); if (me.isAdult()) v.push_back({"Get a Marriage License ($50)", 2}); break;
    case 6: v.push_back({"Financial Advice Session", 0}); v.push_back({"Cash In the Coin Jar", 1}); break;
    case 7: v.push_back({"Send a Care Package ($15)", 0}); v.push_back({"Pick Up Your Mail", 1}); break;
    case 8: return travelOptions();
    case 9: return courtOptions();
    }
    return v;
}

bool rabbitHoleDo(int k, int opt) {
    fitH();
    Sim& me = MC(); Activity a;
    auto pay = [](int c) { if (G.money < c) { toast("Not enough money!"); sfxNo(); return false; } G.money -= c; return true; };
    auto away = [&](const char* name, float mins) { cancelActivity(); a = makeAct(name, mins, 12); startActivity(a); };
    if (k == 8) return travelDo(opt);
    if (k == 9) return courtDo(opt);
    if (k == 2 && opt >= 50) return boardingSchoolDo(opt);
    switch (k * 10 + opt) {
    case 0: if (!pay(40)) return false; away("At the Clinic", 60);
        if (isSick(G.mc)) { G.diag[G.mc] = 1; toast(std::string("Diagnosis: ") + illnessName(illnessOf(G.mc)) + ". Rest up, and medication will speed things along."); }
        else { toast("Clean bill of health! The doctor gives you a sticker."); addMoodlet(me, "Healthy Checkup", EM_HAPPY, 1, 360); }
        return true;
    case 1: { int c = diagnosed(G.mc) ? IL[illnessOf(G.mc)].medCost : 25; if (!pay(c)) return false; if (!diagnosed(G.mc)) toast("Guessed at the medicine without a diagnosis... it helps a little."); healthTreat(G.mc, 0); if (!diagnosed(G.mc) && G.sickDays[G.mc] > 1) G.sickDays[G.mc]++; else toast("Medication picked up. Recovery should be quick now!"); away("At the Pharmacy", 30); return true; }
    case 2: if (!pay(20)) return false; addMoodlet(me, "Flu-Shot Brave", EM_CONFIDENT, 1, 1440); toast("Flu shot done! Just a tiny pinch."); away("Getting a Flu Shot", 30); return true;
    case 3: { int n = 0; for (size_t i = 0; i < G.sims.size(); i++) if (G.sims[i].household && (int)i != G.mc && isSick((int)i)) { G.diag[i] = 1; healthTreat((int)i, 0); n++; }
        if (!n) { toast("Nobody at home is sick right now."); return false; } if (!pay(40)) return false; toast("Got the family checked out and treated. Everyone will feel better soon."); away("Clinic Visit with Family", 90); return true; }
    case 10: if (!pay(150)) return false; healthTreat(G.mc, 1); toast("The ER team patched you right up. Good as new!"); away("At the Hospital", 180); return true;
    case 11: addMoodlet(me, "Baby Fever", EM_HAPPY, 1, 360); toast("So many tiny hats in the nursery window!"); away("Visiting the Nursery", 30); return true;
    case 12: skillGain(me, SK_CHARISMA, 4); addMoodlet(me, "Helped Others", EM_HAPPY, 2, 480); toast("You spent the afternoon cheering up patients."); away("Hospital Volunteering", 180); return true;
    case 20: me.gradePts = std::min(100.f, me.gradePts + 4); skillGain(me, SK_LOGIC, 3); addMoodlet(me, "In the Zone", EM_FOCUSED, 2, 180); away("Study Hall", 90); return true;
    case 21: if (isSick(G.mc)) { me.schoolToday = true; toast("Excused absence. Back to bed!"); return false; } skillGain(me, irand(2) ? SK_CREATIVITY : SK_FITNESS, 4); milestone(G.mc, MI_HOBBY); addMoodlet(me, "Club Kid", EM_PLAYFUL, 1, 240); away("After-School Club", 120); return true;
    case 22: { int n = 0; for (Sim& s : G.sims) if (s.household && s.stage >= ST_CHILD && s.stage <= ST_TEEN) { s.gradePts = std::min(100.f, s.gradePts + 5); n++; } toast(n ? "The teachers had great tips. The kids' grades are up!" : "You don't have school-age kids... the teacher is confused but polite."); away("Parent-Teacher Conference", 60); return true; }
    case 30: me.needs[N_ENERGY] = std::max(0.f, me.needs[N_ENERGY] - 25); me.performance += 15; G.money += 60 + me.careerLevel * 40; toast("Overtime done. Exhausted, but the paycheck is cute."); away("Working Overtime", 180); return true;
    case 31: if (isSick(G.mc)) { me.workedToday = true; toast("You called in sick. Your boss says feel better!"); return false; }
        if (me.performance >= 60 && irand(2)) { me.performance = 100; toast("Your boss said YES! Promotion incoming."); } else { me.performance -= 5; toast("\"Let's revisit this next quarter.\" Ugh."); } away("Meeting With the Boss", 30); return true;
    case 32: me.needs[N_HUNGER] = std::min(100.f, me.needs[N_HUNGER] + 30); addMoodlet(me, "Potluck Party", EM_HAPPY, 1, 240); away("Office Potluck", 60); return true;
    case 40: addMoodlet(me, "Adrenaline Rush", EM_ENERGIZED, 2, 240); toast("Sirens, snacks and a stakeout. Officer Jo let you work the radio!"); away("Police Ride-Along", 120); return true;
    case 41: { int w = irand(4) == 0 ? 25 : 5; G.money += w; toast("Lost & Found had $" + std::to_string(w) + " with your name on it (finders keepers, it's been 30 days)."); away("At the Police Station", 20); return true; }
    case 42: skillGain(me, SK_LOGIC, 2); toast("You learned the town's safety tips. Very responsible."); away("Safety Talk", 60); return true;
    case 50: { std::string n = G.hhName; if (askText("Household name", n, 30, false) && !n.empty()) { G.hhName = n; toast("Your household is now \"" + n + "\"!"); } return false; }
    case 51: skillGain(me, SK_CHARISMA, 2); addMoodlet(me, "Civic Pride", EM_CONFIDENT, 1, 240); toast("You spoke up about more pink benches downtown. The crowd agreed!"); away("Town Hall Meeting", 90); return true;
    case 52: { int p = partnerOf(G.mc); const Rel* r = p >= 0 ? findRel(G.mc, p) : nullptr; if (!r || r->status != RS_ENGAGED) { toast("You need to be engaged first!"); return false; }
        if (!pay(50)) return false; rel(G.mc, p).status = RS_MARRIED; milestone(G.mc, MI_MARRIED, p); milestone(p, MI_MARRIED, G.mc); addMoodlet(me, "Just Married!", EM_HAPPY, 3, 1440); toast("Courthouse wedding! You and " + G.sims[p].name + " are officially married."); sfxYay(); away("Courthouse Wedding", 60); return true; }
    case 60: skillGain(me, SK_LOGIC, 3); toast("Tip: pay yourself first, and maybe fewer porch packages."); away("Financial Advice", 60); return true;
    case 61: { int lastDay = G.coinJarDay; if (lastDay == G.day) { toast("You already cashed the coin jar today."); return false; } G.coinJarDay = G.day; int c = 8 + irand(20); G.money += c; toast("Coin jar cashed in: +$" + std::to_string(c)); away("At the Bank", 20); return true; }
    case 70: { if (!pay(15)) return false; int best = -1, bf = -999; for (size_t i = 0; i < G.sims.size(); i++) { int s = (int)i; if (s == G.mc || G.sims[i].household || simDead(s)) continue; if (familyRole(G.mc, s).empty()) continue; const Rel* r = findRel(G.mc, s); int f = r ? r->fr : 0; if (f > bf) { bf = f; best = s; } }
        if (best < 0) for (const Rel& r : G.rels) if ((r.a == G.mc || r.b == G.mc) && r.fr > bf) { bf = r.fr; best = r.a == G.mc ? r.b : r.a; }
        if (best >= 0) { changeRel(G.mc, best, 6, 0); toast("Care package mailed to " + G.sims[best].name + ". They'll love it!"); } else toast("Care package sent to... yourself. Self-care!");
        away("At the Post Office", 30); return true; }
    case 71: { int r = irand(3); toast(r == 0 ? "Mail: a coupon! +$10" : r == 1 ? "Mail: a postcard from a friend. Cute!" : "Mail: just bills. Classic."); if (r == 0) G.money += 10; if (r == 1) addMoodlet(me, "Happy Mail", EM_HAPPY, 1, 240); return false; }
    }
    return false;
}
