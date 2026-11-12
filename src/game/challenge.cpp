// Challenge Modes: Doll Wars, After Dark, The Whisper Network, Bellweather Academy.
#include "challenge.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <algorithm>
#include <vector>
#include "vfs.hpp"
#define fopen vfopen   // sidecar saves go into the one-file save pack
using namespace col;
void toast(const std::string& m);

namespace {
enum Flag : uint8_t { FL_NONE, FL_DANGER, FL_OUSTRIVAL, FL_SAVERIVAL, FL_ITEM };
struct Choice { const char* label; int8_t d[4]; const char* outcome; uint8_t flag; };
struct ChEvent { const char* code; const char* title; const char* text; Choice c[3]; };
struct Ending { const char* name; const char* text; };
struct Mode {
    const char* name; const char* tag; const char* intro; const char* stat[4]; int age;   // 0 anyone, 2 adults only
    bool rivals, lethalAllowed; RGB bg, ink; ChEvent ev[7]; int nev; Ending end[5];
};
const Mode MODES[4] = {
    {"Doll Wars: Last Look", "Adults only - fashion faction survival",
     "The Velvet District fashion expo locks its doors at midnight. Eight contestants, two factions - Velvet Clique and Vinyl Rebels - and one Last Look. "
     "Ally up, dress to survive, and outlast everyone.",
     {"Style", "Loyalty", "Courage", "Rep"}, 2, true, true, {60, 20, 60}, {255, 150, 220},
     {{"DW001", "Rumor at the Runway", "The Vinyl Rebels are whispering that you stole a rival's look. The whole backstage is staring.",
       {{"Confront them in front of everyone", {1, 0, 2, -1}, "You call it out. Half the room claps, half the room plots.", FL_NONE},
        {"Fact-check quietly with the stylists", {0, 1, 0, 2}, "Receipts! The rumor dies and your team trusts you more.", FL_NONE},
        {"Flip it on the rival who started it", {1, -1, 1, 1}, "It works... a little too well. Someone is going home tonight.", FL_OUSTRIVAL}}},
      {"DW002", "The Vanishing VIP Pass", "Every VIP pass just disappeared. Without one, nobody gets into the safe room after lights-out.",
       {{"Search the garment bags", {0, 0, 1, 0}, "You find two passes stuffed in a sequin gown. Safe room unlocked.", FL_ITEM},
        {"Accuse the quiet contestant", {0, -2, 1, -1}, "You were wrong. Very, very wrong.", FL_NONE},
        {"Pick the lock with a hairpin", {1, 0, 2, 0}, "Click. The door opens - and the lights go out.", FL_DANGER}}},
      {"DW003", "Midnight Alliance", "A rival slides into your mirror station: \"Pact? You and me to the final two.\"",
       {{"Shake on it", {0, 2, 0, 0}, "A pact under the spotlights. Time will tell if they meant it.", FL_NONE},
        {"Pretend to agree, then tell your team", {0, -1, 1, 2}, "Your team loves you. The rival does not.", FL_OUSTRIVAL},
        {"Turn it down politely", {1, 1, 0, 0}, "No deals. You walk your own runway.", FL_NONE}}},
      {"DW004", "Emergency Spotlight", "A rigging light swings loose over the runway - right above a rival.",
       {{"Shove them out of the way", {0, 2, 2, 2}, "You both hit the floor. They owe you their life - and they know it.", FL_SAVERIVAL},
        {"Grab the advantage token while everyone's distracted", {2, -2, 0, -1}, "You pocket the token. Behind you: a crash and a scream.", FL_DANGER},
        {"Yell for security", {0, 1, 0, 1}, "Security catches it just in time. Nobody's hurt.", FL_NONE}}},
      {"DW005", "The Backstage Blackout", "The generators die. Walkie-talkies crackle. Somebody is moving between the dress racks.",
       {{"Barricade the dressing room with your team", {0, 2, 1, 0}, "Safe until morning. Mostly.", FL_NONE},
        {"Go find the fuse box alone", {0, 0, 3, 1}, "Flashlight in your teeth, heart in your throat...", FL_DANGER},
        {"Hide in the costume trunk", {-1, 0, -1, 0}, "Cramped, dusty, alive.", FL_NONE}}},
      {"DW006", "Last Look Finale", "Final round: one runway, one trophy. Your allies' loyalty, your style and pure chance decide it.",
       {{"Walk in your boldest look", {3, 0, 1, 1}, "The crowd gasps. It's giving LAST LOOK.", FL_NONE},
        {"Share the spotlight with your ally", {1, 3, 0, 1}, "A joint finale nobody saw coming.", FL_NONE},
        {"Sabotage the last rival's heel", {1, -3, 0, -2}, "They stumble. You win ugly.", FL_OUSTRIVAL}}}}, 6,
     {{"Last Look Queen", "Sole survivor of the expo. The trophy is yours."}, {"Faction Majority", "Your faction holds the final runway together."},
      {"Joint Escape", "You and your allies negotiate a way out together - nobody left behind."}, {"Eliminated", "You were voted out of the Velvet District."},
      {"Disqualified with Dignity", "You leave with your head high and your best look intact."}}},
    {"After Dark: The Lockdown", "Adults only - city lockdown thriller",
     "Sirens. Radio static. Maple Valley's twelve-hour lockdown starts at dusk. Stock up, pick who you trust and make it to morning roll call.",
     {"Supplies", "Trust", "Nerve", "Energy"}, 2, false, true, {20, 24, 50}, {255, 200, 90},
     {{"AL001", "Neighbor at the Door", "Pounding on your door. \"Please - it's Marisol from 4B! Let me in!\"",
       {{"Let her in", {-1, 3, 0, 0}, "She brings batteries and gratitude. You're stronger together.", FL_NONE},
        {"Talk through the door", {0, 1, 1, 0}, "You slide her a flashlight under the door. She makes it home.", FL_NONE},
        {"Stay silent", {0, -2, 0, 1}, "The knocking stops. You don't sleep much.", FL_NONE}}},
      {"AL002", "Medicine Run", "Your neighbor's kid needs medicine. The pharmacy is three blocks into the HIGH RISK zone.",
       {{"Go now, fast", {1, 2, 2, -2}, "You sprint through the dark streets...", FL_DANGER},
        {"Call the clinic hotline instead", {0, 1, 0, 0}, "A nurse talks you through a safe home remedy. The kid sleeps.", FL_NONE},
        {"Wait until the CAUTION window", {0, 0, 1, 1}, "You time it with the patrol. Safe trip.", FL_NONE}}},
      {"AL003", "Power Outage", "Every light on the block dies. Your phone is at 9%.",
       {{"Light candles and share them", {-1, 2, 0, 0}, "Your stoop becomes the neighborhood's lighthouse.", FL_NONE},
        {"Ration your battery", {1, 0, 1, 0}, "Smart. You'll need that phone later.", FL_NONE},
        {"Check the breaker in the basement", {0, 0, 2, -1}, "Down the stairs. Something moves near the boiler...", FL_DANGER}}},
      {"AL004", "False Rumor", "The group chat says the north bridge is open. The radio says it isn't.",
       {{"Trust the radio", {0, 1, 1, 0}, "The radio was right. You warn everyone.", FL_NONE},
        {"Trust the group chat", {0, -1, 2, -1}, "The bridge is closed. You're stuck outside past curfew.", FL_DANGER},
        {"Post the correction yourself", {0, 2, 0, 0}, "You stop three people from walking into trouble.", FL_NONE}}},
      {"AL005", "The Last Hour", "Almost dawn. Someone is trying the handle of your back door.",
       {{"Brace the door", {0, 0, 2, -2}, "You hold it until the sirens fade.", FL_DANGER},
        {"Slip out the front to a neighbor", {0, 2, 1, -1}, "You knock twice. The door opens.", FL_NONE},
        {"Call out that the police are coming", {0, 0, 1, 0}, "Footsteps run away. You exhale for the first time in hours.", FL_NONE}}},
      {"AL006", "Morning Roll Call", "The lockdown lifts. The block gathers on the sidewalk to see who made it.",
       {{"Help organize a breakfast", {-1, 3, 0, 0}, "Pancakes on a camping stove. Everyone cries a little.", FL_NONE},
        {"Count your supplies", {2, 0, 0, 1}, "You made it - and you're ready for next time.", FL_NONE},
        {"Go straight to bed", {0, 0, 0, 3}, "You sleep for fourteen hours.", FL_NONE}}}}, 6,
     {{"Neighborhood Protector", "The whole block knows your name now - for all the right reasons."}, {"Quiet Survivor", "Stocked, careful and alive."},
      {"Opportunist", "You came out ahead. Not everyone will forget how."}, {"Family-First Escape", "You got your people out. That's all that matters."},
      {"Safe Recovery", "You didn't hit every goal, but you're safe. That counts."}}},
    {"The Whisper Network", "Teens or adults - social rumor mystery",
     "You just moved into the upscale side of Maple Valley. Then the first anonymous message arrives: \"I know what you did. -W\" "
     "Decide what to reveal, what to protect and what to disprove.",
     {"Popularity", "Trust", "Secrets", "Integrity"}, 0, false, false, {50, 20, 60}, {200, 170, 255},
     {{"WN001", "The Wrong Screenshot", "A screenshot of your private chat is going around - but it's been edited.",
       {{"Post the original", {1, 1, 0, 2}, "The truth spreads almost as fast as the lie did.", FL_NONE},
        {"Find out who edited it", {0, 0, 2, 1}, "The trail leads somewhere you didn't expect.", FL_NONE},
        {"Leak something worse about them", {2, -2, 1, -3}, "It works. You hate that it works.", FL_NONE}}},
      {"WN002", "Invite Only", "Everyone got an invite to the masked gala except your best friend.",
       {{"Give them your invite", {-1, 3, 0, 2}, "You stay home together with face masks and popcorn.", FL_NONE},
        {"Go anyway and report back", {2, -1, 1, 0}, "You go. The party is iconic. Your friend is quiet for a week.", FL_NONE},
        {"Find out who's leaving them out", {0, 1, 2, 1}, "A guest list, a grudge, and a name.", FL_NONE}}},
      {"WN003", "The Anonymous Tip", "\"-W\" texts you a secret about someone you love. It might be true.",
       {{"Ask them privately", {0, 2, 1, 2}, "They tell you the real story. It's complicated - and human.", FL_NONE},
        {"Keep it to yourself", {0, 0, 2, 0}, "Knowledge is power. It's also heavy.", FL_NONE},
        {"Repost it", {2, -3, 0, -3}, "Views explode. So does a friendship.", FL_NONE}}},
      {"WN004", "Masquerade Unmasking", "At the gala, someone in a silver mask whispers: \"Want to know who W is?\"",
       {{"Follow them", {0, 0, 3, 0}, "Down the hall, through the garden... and the mask comes off.", FL_NONE},
        {"Stay with your friends", {1, 2, 0, 1}, "Mystery can wait. Tonight you dance.", FL_NONE},
        {"Unmask them in front of everyone", {2, -1, 1, -1}, "Gasps. Flashes. Chaos.", FL_NONE}}},
      {"WN005", "Apology Goes Public", "The person you hurt (or who hurt you) posts a public apology. Everyone's waiting on your reply.",
       {{"Accept it graciously", {1, 2, 0, 2}, "Growth looks good on both of you.", FL_NONE},
        {"Reply with receipts", {1, -1, 1, 0}, "The comments are a war zone.", FL_NONE},
        {"Take it offline", {0, 2, 0, 1}, "You meet for coffee. The internet never finds out how it ends.", FL_NONE}}}}, 5,
     {{"Truth Seeker", "You found out who W was - and you told the truth about it."}, {"Queen of the Scene", "Every invite has your name on it now."},
      {"Private Peace", "You stepped back from the feed and found your people."}, {"Loyal Friend", "When it got ugly, you stayed loyal."},
      {"Scandal Survivor", "Rumors came and went. You're still standing."}}},
    {"Bellweather Academy", "Boarding school drama (teen-friendly)",
     "Welcome to Bellweather Academy: blazers, curfews and a four-week term. New roommate, new clubs, new rivals - and a scholarship on the line.",
     {"Grades", "Friends", "Spirit", "Discipline"}, 0, false, false, {30, 50, 90}, {255, 220, 120},
     {{"BA001", "Roommate Rules", "Your new roommate taped a line down the middle of the room. Their side is spotless. Yours... less so.",
       {{"Propose a fair chore chart", {0, 2, 0, 2}, "Signed and laminated. Peace is achieved.", FL_NONE},
        {"Ignore the line completely", {0, -1, 1, -2}, "Cold war in room 214.", FL_NONE},
        {"Decorate both sides together", {0, 2, 2, 0}, "Fairy lights everywhere. Instant best friends.", FL_NONE}}},
      {"BA002", "Dance Team Auditions", "Auditions are in an hour and you haven't practiced the eight-count.",
       {{"Practice with the team captain", {0, 1, 2, 1}, "You nail it. Welcome to the team!", FL_NONE},
        {"Freestyle it", {0, 0, 3, -1}, "Bold. Chaotic. The judges are... intrigued.", FL_NONE},
        {"Skip it and study instead", {2, 0, -1, 1}, "Your midterm score is the highest in the class.", FL_NONE}}},
      {"BA003", "Rumor in the Hall", "Someone wrote a mean rumor about a classmate on the bathroom mirror.",
       {{"Wipe it off and tell the dorm advisor", {0, 2, 0, 2}, "The advisor handles it. The classmate thanks you quietly.", FL_NONE},
        {"Start an anonymous kindness board", {0, 2, 2, 0}, "By Friday the whole hallway is covered in compliments.", FL_NONE},
        {"Laugh along with everyone", {0, -2, 0, -1}, "It isn't funny later.", FL_NONE}}},
      {"BA004", "The Scholarship Interview", "The scholarship panel asks: \"Tell us about a time you failed.\"",
       {{"Tell the honest story", {2, 0, 1, 2}, "The panel nods. One of them smiles.", FL_NONE},
        {"Turn it into a joke", {0, 1, 2, -1}, "They laugh. They also write something down.", FL_NONE},
        {"Say you've never failed", {-1, 0, 0, 0}, "Long silence. Very long.", FL_NONE}}},
      {"BA005", "Winter Formal", "The Winter Formal is Saturday. Your crush asked if you're going.",
       {{"Ask them to go together", {0, 2, 2, 0}, "Yes! Matching corsages and a slow dance.", FL_NONE},
        {"Go with your friend group", {0, 3, 1, 0}, "The best group photo in the yearbook.", FL_NONE},
        {"Stay in and finish your project", {2, 0, -1, 2}, "A+. Your friends send you a hundred photos.", FL_NONE}}}}, 5,
     {{"Valedictorian", "Top of the class - and you earned every point."}, {"Beloved Club Founder", "The club you started will outlast you."},
      {"Rebel with Growth", "You broke some rules, and learned a lot from it."}, {"Team Champion", "Your friends carried you, and you carried them."},
      {"Quiet Creative", "Your art hangs in the hallway now."}}},
};

// ---------------------------------------------------------------- run state (saved next to the slot)
struct Run {
    uint32_t magic = 0x43484131;   // "CHA1"
    int8_t mode = -1; uint8_t ev = 0, done = 0, endIdx = 0, items = 0, outs = 0;
    int8_t st[4] = {0, 0, 0, 0};
    int16_t riv[8] = {-1, -1, -1, -1, -1, -1, -1, -1}; uint8_t rivSt[8] = {};   // 0 in, 1 eliminated, 2 hospitalized, 3 didn't make it
    uint8_t tragedy = 1, violence = 1, permadeath = 0, confirmed = 0, reducedFlash = 0, playerOut = 0;
};
Run R;
enum Scr { S_HUB, S_SETTINGS, S_CONFIRM, S_EVENT, S_OUTCOME, S_END };
int scr = S_HUB, pickMode = 0;
std::string outText; int lastChoice = -1;
float fx = 0; int fxKind = 0;   // 0 none, 1 clean fade, 2 mild mark, 3 stylized drops
float dropX[10], dropY[10];

std::string fmt(const char* f, ...) { char b[400]; va_list a; va_start(a, f); vsnprintf(b, sizeof b, f, a); va_end(a); return b; }
std::string first(int s) { if (s < 0 || s >= (int)G.sims.size()) return "Someone"; const std::string& n = G.sims[s].name; return n.substr(0, n.find(' ')); }
int liveRivals() { int n = 0; for (int k = 0; k < 8; k++) if (R.riv[k] >= 0 && R.rivSt[k] == 0) n++; return n; }
int pickRival() { std::vector<int> v; for (int k = 0; k < 8; k++) if (R.riv[k] >= 0 && R.rivSt[k] == 0) v.push_back(k); return v.empty() ? -1 : v[irand((int)v.size())]; }
bool lethalOk() { return MODES[R.mode].lethalAllowed && R.permadeath && R.confirmed && R.tragedy > 0; }

void startRun(int m) {
    uint8_t tr = R.tragedy, vi = R.violence, pd = R.permadeath, rf = R.reducedFlash;
    R = Run{}; R.tragedy = tr; R.violence = vi; R.permadeath = pd; R.reducedFlash = rf;
    R.mode = (int8_t)m; R.confirmed = pd && MODES[m].lethalAllowed;
    if (MODES[m].rivals) {   // eight adult contestants from town; never kids, never your own household
        std::vector<int> pool; for (size_t i = 0; i < G.sims.size(); i++) if ((int)i != G.mc && !G.sims[i].household && G.sims[i].stage >= ST_YA) pool.push_back((int)i);
        for (int k = 0; k < 8 && !pool.empty(); k++) { int j = irand((int)pool.size()); R.riv[k] = (int16_t)pool[j]; pool.erase(pool.begin() + j); }
    }
    scr = S_EVENT; sfxYay();
}
void dangerRoll(int choiceCourage) {
    static const int BASE[4] = {0, 15, 30, 50};
    int risk = BASE[R.tragedy % 4] - R.st[2] * 4 - choiceCourage * 2 + R.items * -5;
    if (R.tragedy == 0 || irand(100) >= std::max(5, risk)) { outText += "\nYou made it through unhurt."; return; }
    // somebody gets hurt: a rival in Doll Wars, a neighbor in After Dark; the player only if no one else is around
    fxKind = R.violence == 0 ? 1 : R.violence == 1 ? 2 : 3; fx = 0;
    for (int k = 0; k < 10; k++) { dropX[k] = 150 + irand(100); dropY[k] = 140 + irand(60); }
    int k = MODES[R.mode].rivals ? pickRival() : -1;
    if (k >= 0) {
        const std::string n = first(R.riv[k]);
        if (lethalOk() && irand(100) < 40 + R.tragedy * 10) { R.rivSt[k] = 3; outText += fmt("\n%s didn't make it out of the Velvet District. The expo goes silent.", n.c_str()); }
        else { R.rivSt[k] = 2; outText += fmt("\n%s is hurt and taken out on a stretcher - out of the competition, but alive.", n.c_str()); }
        R.outs++;
    } else {
        R.st[3] -= 2; R.st[2] -= 1;
        outText += lethalOk() && irand(100) < 15 * R.tragedy ? "\nA neighbor down the block didn't make it to morning. The whole street mourns." : "\nYou get hurt and limp home. You'll heal - but you'll never forget tonight.";
    }
    if (!R.reducedFlash) sfxNo();
}
void choose(int c) {
    const Mode& M = MODES[R.mode]; const Choice& ch = M.ev[R.ev].c[c];
    for (int s = 0; s < 4; s++) R.st[s] = (int8_t)std::clamp(R.st[s] + ch.d[s], -20, 30);
    outText = ch.outcome; fxKind = 0; lastChoice = c;
    switch (ch.flag) {
    case FL_DANGER: dangerRoll(ch.d[2]); break;
    case FL_OUSTRIVAL: { int k = pickRival(); if (k >= 0) { R.rivSt[k] = 1; R.outs++; outText += fmt("\n%s is eliminated.", first(R.riv[k]).c_str()); } break; }
    case FL_SAVERIVAL: R.st[1] += 2; break;
    case FL_ITEM: R.items++; outText += "\n+1 safe-room pass."; break;
    }
    // Doll Wars: end of every round someone goes home (temporary elimination) - maybe you
    if (M.rivals && R.ev < M.nev - 1) {
        if (R.st[3] + R.st[1] < -4 && irand(3) == 0) { R.playerOut = 1; outText += "\nThe vote is in... and it's YOU. You're out."; }
        else { int k = pickRival(); if (k >= 0 && liveRivals() > 2) { R.rivSt[k] = 1; R.outs++; outText += fmt("\nRound vote: %s goes home.", first(R.riv[k]).c_str()); } }
    }
    scr = S_OUTCOME; sfxTap();
}
int endingFor() {
    const Mode& M = MODES[R.mode]; (void)M;
    const int* s = nullptr; int st[4]; for (int k = 0; k < 4; k++) st[k] = R.st[k]; s = st;
    switch (R.mode) {
    case 0: if (R.playerOut) return 3; if (liveRivals() == 0 || (lastChoice == 0 && s[0] >= 6)) return 0; if (lastChoice == 1 || s[1] >= 6) return s[2] >= 4 ? 2 : 1; return 4;
    case 1: if (s[1] >= 7) return 0; if (s[0] >= 4 && s[1] < 2) return 2; if (s[0] >= 3) return 1; if (s[2] >= 5) return 3; return 4;
    case 2: if (s[2] >= 5 && s[3] >= 3) return 0; if (s[0] >= 5) return 1; if (s[1] >= 6) return 3; if (s[3] >= 4) return 2; return 4;
    case 3: if (s[0] >= 5) return 0; if (s[2] >= 6) return 1; if (s[3] <= -2 && s[2] >= 3) return 2; if (s[1] >= 6) return 3; return 4;
    }
    return 4;
}
void advance() {
    const Mode& M = MODES[R.mode];
    if (R.playerOut || R.ev + 1 >= M.nev) {
        R.done = 1; R.endIdx = (uint8_t)endingFor(); scr = S_END;
        addMemoryOnce(MEM_FUN, G.mc, -1, std::string("Played ") + M.name + ": " + M.end[R.endIdx].name + ".");
        G.money += 200 + 100 * std::max(0, (int)R.st[0]); sfxYay();
        return;
    }
    R.ev++; scr = S_EVENT;
}
bool allowed(int m) { return MODES[m].age == 0 || MC().stage >= ST_YA; }

Btn rowB(int k) { return Btn{10, 30 + k * 40.f, 300, 36}; }
const Btn bBack{4, 208, 72, 28}, bGo{164, 208, 152, 28}, bSet{82, 208, 76, 28};
}

void challengeOpen() { scr = R.mode >= 0 && !R.done ? S_EVENT : S_HUB; }

bool challengeUpdate(const Input& in) {
    if (fxKind) fx += in.dt;
    if (in.down & KEY_B) { if (scr == S_HUB) return false; scr = S_HUB; return true; }
    if (!in.touch && !(in.down & KEY_A)) return true;
    const auto tp = in.tp;
    switch (scr) {
    case S_HUB:
        for (int m = 0; m < 4; m++) if (in.touch && rowB(m).in(tp)) { pickMode = m; sfxTap();
            if (!allowed(m)) { toast("This mode is for adults only (your Sim must be a young adult or older)."); sfxNo(); return true; }
            scr = S_CONFIRM; return true; }
        if (in.touch && bBack.in(tp)) return false;
        if (in.touch && bSet.in(tp)) { scr = S_SETTINGS; sfxTap(); }
        if (in.touch && bGo.in(tp) && R.mode >= 0 && !R.done) { scr = S_EVENT; sfxTap(); }
        return true;
    case S_SETTINGS:
        if (in.touch && rowB(0).in(tp)) R.tragedy = (R.tragedy + 1) % 4;
        if (in.touch && rowB(1).in(tp)) R.violence = (R.violence + 1) % 3;
        if (in.touch && rowB(2).in(tp)) { R.permadeath ^= 1; if (!R.permadeath) R.confirmed = 0; }
        if (in.touch && rowB(3).in(tp)) R.reducedFlash ^= 1;
        if (in.touch && bBack.in(tp)) scr = S_HUB;
        sfxTap(); return true;
    case S_CONFIRM:
        if (in.touch && bBack.in(tp)) { scr = S_HUB; return true; }
        if (in.touch && rowB(3).in(tp)) { R.permadeath = 0; R.confirmed = 0; sfxTap(); return true; }   // revert to the safer preset
        if ((in.touch && bGo.in(tp)) || (in.down & KEY_A)) startRun(pickMode);
        return true;
    case S_EVENT:
        for (int c = 0; c < 3; c++) if (in.touch && rowB(c + 2).in(tp)) { choose(c); return true; }
        if (in.touch && bBack.in(tp)) { scr = S_HUB; return true; }
        return true;
    case S_OUTCOME:
        if ((in.touch && bGo.in(tp)) || (in.down & KEY_A)) { fxKind = 0; advance(); }
        return true;
    case S_END:
        if ((in.touch && bGo.in(tp)) || (in.down & KEY_A) || (in.touch && bBack.in(tp))) { R.mode = -1; scr = S_HUB; }
        return true;
    }
    return true;
}

void challengeDrawTop(float t) {
    const int m = scr == S_HUB || scr == S_SETTINGS ? -1 : scr == S_CONFIRM ? pickMode : R.mode;
    const RGB bg = m >= 0 ? MODES[m].bg : RGB{50, 30, 70}, ink = m >= 0 ? MODES[m].ink : RGB{255, 180, 230};
    grad(0, 0, TOP_W, TOP_H, C(bg), C(bg.r / 2, bg.g / 2, bg.b / 2));
    for (int i = 0; i < 12; i++) sparkle(std::fmod(i * 61.f + t * 6, 400.f), 20 + (i * 43) % 200, 2.5f, C(ink.r, ink.g, ink.b, 90));
    if (m < 0) {
        textOutline("CHALLENGE MODES", 200, 30, 0.8f, C(255, 255, 255), C(ink), 1);
        text("Self-contained story runs. Your town save is never erased.", 200, 80, 0.42f, C(255, 255, 255), 1);
        static const char* TR[4] = {"Off", "Rare", "Moderate", "Chaotic"}, *VI[3] = {"Clean", "Mild", "Stylized"};
        text(fmt("Tragedy: %s   Violence: %s   Permanent death: %s", TR[R.tragedy % 4], VI[R.violence % 3], R.permadeath ? "ON" : "Off").c_str(), 200, 120, 0.4f, C(255, 230, 240), 1);
        text("Teens & kids are never victims. Bellweather & Whisper are always nonlethal.", 200, 150, 0.36f, C(220, 210, 230), 1);
        return;
    }
    const Mode& M = MODES[m];
    textOutline(M.name, 200, 4, 0.6f, C(255, 255, 255), C(ink), 1);
    // contestants strip (Doll Wars)
    if (M.rivals && scr != S_CONFIRM) for (int k = 0; k < 8; k++) {
        if (R.riv[k] < 0) continue;
        float x = 28 + k * 49.f;
        drawSimPortrait(G.sims[R.riv[k]], x, 44, 18, t);
        if (R.rivSt[k]) { rect(x - 19, 25, 38, 38, C(0, 0, 0, 150)); text(R.rivSt[k] == 3 ? "RIP" : R.rivSt[k] == 2 ? "HURT" : "OUT", x, 36, 0.36f, C(R.rivSt[k] == 3 ? RGB{255, 90, 90} : RGB{255, 255, 255}), 1); }
    }
    float y = M.rivals && scr != S_CONFIRM ? 72 : 30;
    auto para = [&](const std::string& s, float sc, RGB c) { for (auto& l : wrapText(s, sc, 372)) { if (y > 196) break; text(l.c_str(), 14, y, sc, C(c)); y += sc * 32; } };
    if (scr == S_CONFIRM) {
        para(M.tag, 0.42f, ink); para(M.intro, 0.42f, RGB{255, 255, 255});
        if (M.lethalAllowed && R.permadeath) para("WARNING: Permanent Death is ON. Contestants or neighbors can die in this run (stylized, non-graphic). Tap 'Safer preset' to turn it off.", 0.4f, RGB{255, 140, 140});
    } else if (scr == S_EVENT) { const ChEvent& e = M.ev[R.ev]; para(fmt("%s  (%d/%d)", e.title, R.ev + 1, M.nev), 0.5f, ink); para(e.text, 0.44f, RGB{255, 255, 255}); }
    else if (scr == S_OUTCOME) { para(outText, 0.44f, RGB{255, 255, 255}); }
    else if (scr == S_END) { para(std::string("ENDING: ") + M.end[R.endIdx].name, 0.55f, ink); para(M.end[R.endIdx].text, 0.44f, RGB{255, 255, 255}); para("+ a memory in your Life Book and some cash for your trouble.", 0.38f, RGB{220, 210, 230}); }
    // stats
    for (int s = 0; s < 4; s++) {
        float x = 14 + s * 96.f;
        text(M.stat[s], x, 204, 0.36f, C(255, 255, 255));
        rrect(x, 220, 84, 8, 3, C(255, 255, 255, 60)); rrect(x, 220, 84 * std::clamp((R.st[s] + 10) / 30.f, 0.f, 1.f), 8, 3, C(ink));
    }
    // the danger moment, at the chosen Visual Violence level (never gore)
    if (scr == S_OUTCOME && fxKind) {
        if (fxKind == 1) rect(0, 0, TOP_W, TOP_H, C(0, 0, 0, (uint8_t)(std::max(0.f, 1.f - fx) * 200)));   // Clean: a fade to black
        else {
            if (!R.reducedFlash && fx < 0.25f) rect(0, 0, TOP_W, TOP_H, C(180, 0, 20, 90));               // brief red flash
            const int n = fxKind == 2 ? 1 : 7;
            for (int k = 0; k < n; k++) { circle(dropX[k] * 2 - 150 + (k ? 0 : 0), dropY[k] + 20, fxKind == 2 ? 1.5f : 2.f + (k % 3), C(130, 0, 20, 220)); }
            if (fxKind == 3) { ellipse(200, 196, 16, 3, C(120, 0, 20, 160)); }
        }
    }
}

void challengeDrawBottom(float t) {
    (void)t;
    grad(0, 0, BOT_W, BOT_H, C(40, 28, 52), C(70, 40, 80));
    switch (scr) {
    case S_HUB:
        text("Pick a challenge", 160, 6, 0.45f, C(255, 255, 255), 1);
        for (int m = 0; m < 4; m++) button(rowB(m), (std::string(MODES[m].name) + (allowed(m) ? "" : "  (18+)")).c_str(), R.mode == m && !R.done, allowed(m) ? (m < 2 ? 1 : 3) : 1, 0.4f);
        button(bBack, "Back", false, 1, 0.4f); button(bSet, "Options", false, 0, 0.38f);
        if (R.mode >= 0 && !R.done) button(bGo, "Continue Run", true, 2, 0.4f);
        break;
    case S_SETTINGS: {
        static const char* TR[4] = {"Off", "Rare", "Moderate", "Chaotic"}, *VI[3] = {"Clean (no blood)", "Mild", "Stylized pixel blood"};
        text("Danger options (saved with this slot)", 160, 6, 0.4f, C(255, 255, 255), 1);
        button(rowB(0), (std::string("Tragedy Frequency: ") + TR[R.tragedy % 4]).c_str(), false, 0, 0.4f);
        button(rowB(1), (std::string("Visual Violence: ") + VI[R.violence % 3]).c_str(), false, 0, 0.4f);
        button(rowB(2), R.permadeath ? "Permanent Death: ON" : "Permanent Death: Off", R.permadeath, 1, 0.4f);
        button(rowB(3), R.reducedFlash ? "Reduced Flashing: ON" : "Reduced Flashing: Off", false, 3, 0.4f);
        button(bBack, "Done", false, 1, 0.4f);
        break; }
    case S_CONFIRM:
        text(MODES[pickMode].name, 160, 6, 0.45f, C(255, 255, 255), 1);
        text(MODES[pickMode].tag, 160, 30, 0.38f, C(220, 200, 230), 1);
        if (MODES[pickMode].lethalAllowed && R.permadeath) button(rowB(3), "Safer preset (turn off permanent death)", false, 3, 0.36f);
        button(bBack, "Back", false, 1, 0.4f); button(bGo, R.mode >= 0 && !R.done ? "Start Over" : "Start!", true, 2, 0.42f);
        break;
    case S_EVENT: {
        const ChEvent& e = MODES[R.mode].ev[R.ev];
        text("What do you do?", 160, 54, 0.45f, C(255, 255, 255), 1);
        for (int c = 0; c < 3; c++) button(rowB(c + 2), e.c[c].label, false, c, 0.38f);
        button(bBack, "Pause", false, 1, 0.4f);
        break; }
    case S_OUTCOME: button(bGo, "Continue", true, 2, 0.42f); break;
    case S_END: text("Run complete!", 160, 90, 0.6f, C(255, 255, 255), 1); button(bGo, "Back to Modes", true, 2, 0.4f); break;
    }
}

int challengeDanger(int w) { return w == 0 ? R.tragedy : w == 1 ? R.violence : w == 2 ? R.permadeath : R.reducedFlash; }
void challengeSaveTo(const std::string& path) {
    FILE* f = fopen(path.c_str(), "wb"); if (!f) return;
    fwrite(&R, sizeof R, 1, f); fclose(f);
}
void challengeLoadFrom(const std::string& path) {
    R = Run{};
    FILE* f = fopen(path.c_str(), "rb"); if (!f) return;
    Run r; size_t n = fread(&r, sizeof r, 1, f); fclose(f);
    if (n != 1 || r.magic != 0x43484131) return;
    if (r.mode < -1 || r.mode > 3) r.mode = -1;
    if (r.mode >= 0 && r.ev >= MODES[r.mode].nev) r.ev = 0;
    r.endIdx %= 5; r.tragedy %= 4; r.violence %= 3;
    for (auto& v : r.riv) if (v >= (int)G.sims.size()) v = -1;
    R = r;
}
