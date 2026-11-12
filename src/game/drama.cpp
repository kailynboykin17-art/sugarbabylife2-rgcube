// Drama & social life: Sims-style "what they think of you" thoughts, first impressions,
// grudges, best-friend status, mood that rubs off on you, lucky / bad hair days, allowance,
// favourite gifts, sleepovers, a secret romance with your BFF's boyfriend, and ten little
// story events (happy, sad and sweet).
#include "drama.hpp"
#include "status.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "social.hpp"
#include "goals.hpp"
#include "crime.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>

// ---------------------------------------------------------------- helpers
static int frOf(int a, int b) { const Rel* r = findRel(a, b); return r ? r->fr : 0; }
static bool here(int sim) { for (const Actor& a : curRoom().actors) if (a.kind == 0 && a.id == sim) return true; return false; }
static int grudgeIdx(int sim) { for (size_t i = 0; i < G.grudges.size(); i++) if (G.grudges[i].sim == sim) return (int)i; return -1; }
bool dramaHasGrudge(int sim) { return grudgeIdx(sim) >= 0; }
static uint32_t hsh(uint32_t v) { v ^= v >> 16; v *= 0x7feb352d; v ^= v >> 15; v *= 0x846ca68b; v ^= v >> 16; return v; }
int dramaFavoriteGift(int sim) { return (int)(hsh((uint32_t)sim * 31u + 7u) % 7u); }   // matches the gift basket kinds

// best friends: high friendship, both ways (relationships are shared)
int dramaBff() {
    int best = -1, bf = 79;
    for (const Rel& r : G.rels) {
        int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1;
        if (o < 0 || o >= (int)G.sims.size() || G.sims[o].household || r.status != RS_NONE) continue;
        if (r.fr > bf) { bf = r.fr; best = o; }
    }
    return best;
}
static int anyFriend(int minFr) {
    std::vector<int> v;
    for (const Rel& r : G.rels) { int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1; if (o >= 0 && o < (int)G.sims.size() && !G.sims[o].household && r.fr >= minFr && G.sims[o].stage >= ST_CHILD) v.push_back(o); }
    return v.empty() ? -1 : v[irand((int)v.size())];
}
static int anyTownie(Stage minSt, int avoid = -1) {
    std::vector<int> v;
    for (size_t i = 0; i < G.sims.size(); i++) if (!G.sims[i].household && (int)i != avoid && G.sims[i].stage >= minSt && (int)i != G.cop) v.push_back((int)i);
    return v.empty() ? -1 : v[irand((int)v.size())];
}
static int ageBand(const Sim& s) { return s.stage <= ST_PRETEEN ? 0 : s.stage == ST_TEEN ? 1 : 2; }

// ---------------------------------------------------------------- 1. thoughts: what they think of you
static const char* T_GOOD[] = {"%s is SO sweet.", "I really like hanging out with %s.", "%s gets me.", "%s is hilarious!", "Okay, %s is actually amazing.", "%s has the best vibe."};
static const char* T_ROM[] = {"Is it hot in here or is it %s?", "%s makes my heart do flips...", "Ugh, %s is SO cute.", "I think I'm falling for %s."};
static const char* T_BAD[] = {"Wow. %s is so rude.", "%s is lucky I'm nice.", "Who does %s think they are?!", "I'm NOT okay with how %s talks to me.", "%s is getting on my last nerve."};
static const char* T_FUN[] = {"%s is a total goofball. Love it.", "%s cracks me up!", "I can't breathe, %s is so funny."};
static const char* T_MEH[] = {"%s is... fine, I guess.", "Hmm. Not sure about %s yet.", "%s is kinda awkward. Cute, though."};
static void think(int sim, const char* const* pool, int n, int mood) {
    char b[120]; snprintf(b, sizeof b, pool[irand(n)], MC().name.c_str());
    thoughtPopup(sim, b, mood);
}
// 2. first impressions: the first time you really talk, they size up your style
static bool impressed(int sim) { return std::find(G.impressed.begin(), G.impressed.end(), (uint16_t)sim) != G.impressed.end(); }
static void firstImpression(int t) {
    if (impressed(t)) return;
    G.impressed.push_back((uint16_t)t);
    if (G.impressed.size() > 300) G.impressed.erase(G.impressed.begin());
    const Sim& me = MC(); const Sim& s = G.sims[t];
    int score = skillLevel(me, SK_STYLE) + skillLevel(me, SK_CHARISMA) + (me.hasTrait(TR_FASHIONISTA) ? 3 : 0) + irand(6) + (G.luck > 0 ? 3 : G.luck < 0 ? -3 : 0);
    if (s.hasTrait(TR_FASHIONISTA) && me.outfit == 0) score -= 1;
    char b[120];
    if (score >= 7) { snprintf(b, sizeof b, "First impression: %s's outfit is EVERYTHING.", me.name.c_str()); changeRel(G.mc, t, 6, 0); thoughtPopup(t, b, 1); }
    else if (score >= 3) { snprintf(b, sizeof b, "First impression: %s seems nice!", me.name.c_str()); changeRel(G.mc, t, 2, 0); thoughtPopup(t, b, 0); }
    else { snprintf(b, sizeof b, "First impression: %s seems... a lot.", me.name.c_str()); changeRel(G.mc, t, -3, 0); thoughtPopup(t, b, -1); }
}
// 3. grudges: being mean sticks around for a few days
static void addGrudge(int t, int days) {
    int g = grudgeIdx(t);
    if (g >= 0) G.grudges[g].days = (uint8_t)std::min(14, G.grudges[g].days + days);
    else { G.grudges.push_back({(uint16_t)t, (uint8_t)days}); toast(G.sims[t].name + " is holding a grudge against you!"); }
}
void dramaAddGrudge(int t, int days) { if (t >= 0 && t < (int)G.sims.size() && t != G.mc) addGrudge(t, days); }
static bool isMean(int id) { return id == SO_INSULT || id == SO_EYEROLL || id == SO_BACKHANDED || id == SO_MOCKOUTFIT || id == SO_RUMOR || id == SO_COLDSHOULDER || id == SO_SPLASH || id == SO_CATFIGHT || id == SO_CALLOUT || id == SO_SUBTWEET || id == SO_COPYOUTFIT || id == SO_EXPOSE || id == SO_BLACKMAIL; }
static bool isFunny(int id) { return id == SO_JOKE || id == SO_IMPRESSION || id == SO_PRANK || id == SO_TICKLE || id == SO_ROAST || id == SO_STEALFRY || id == SO_BIRTHDAYSONG; }
static bool isRomantic(int id) { return id == SO_FLIRT || id == SO_WINK || id == SO_SWEET || id == SO_HOLDHANDS || id == SO_KISS || id == SO_SLOWDANCE || id == SO_SERENADE || id == SO_SNUGGLE || id == SO_LOVENOTE || id == SO_MESSAROUND; }

void dramaAfterSocial(int id, int t, int dfr, int dro) {
    if (t < 0 || t >= (int)G.sims.size() || t == G.mc) return;
    const Sim& s = G.sims[t];
    if (s.stage <= ST_TODDLER) return;
    statusOnTalk(t);
    firstImpression(t);
    // grudges: mean things leave a mark; an apology or a gift melts them
    if (isMean(id) && dfr < 0) addGrudge(t, 2 + irand(3));
    int g = grudgeIdx(t);
    if (g >= 0 && (id == SO_APOLOGIZE || id == SO_GIFT)) { G.grudges.erase(G.grudges.begin() + g); thoughtPopup(t, "...okay. I forgive you.", 1); return; }
    // the thought bubble (not every single time, so it stays special)
    static int lastSim = -1; static uint16_t lastDay = 0xFFFF; static int count = 0;
    if (lastSim == t && lastDay == G.day && ++count > 3 && irand(3)) return;
    if (lastSim != t || lastDay != G.day) { lastSim = t; lastDay = G.day; count = 0; }
    if (dro >= 4 && isRomantic(id)) think(t, T_ROM, 4, 2);
    else if (dfr <= -3) think(t, T_BAD, 5, -1);
    else if (isFunny(id) && dfr > 0) think(t, T_FUN, 3, 1);
    else if (dfr >= 5) think(t, T_GOOD, 6, 1);
    else if (dfr <= 0 && irand(3) == 0) think(t, T_MEH, 3, 0);
    // 4. best friends forever
    if (frOf(G.mc, t) >= 80 && rel(G.mc, t).status == RS_NONE && !G.sims[t].household) {
        if (std::find(G.bffs.begin(), G.bffs.end(), (uint16_t)t) == G.bffs.end()) {
            G.bffs.push_back((uint16_t)t);
            rewardPopup("BEST FRIENDS!", G.sims[t].name + " is officially your BFF!");
            addMoodlet(MC(), "BFF Energy", EM_HAPPY, 2, 720);
        }
    }
}
float dramaSocialBonus(int t) {
    float b = G.luck > 0 ? 0.08f : G.luck < 0 ? -0.08f : 0.f;
    if (t >= 0 && dramaHasGrudge(t)) b -= 0.2f;
    return b;
}

// ---------------------------------------------------------------- 5. mood is contagious
void dramaHourly(int hour) {
    (void)hour;
    if (G.sims.empty()) return;
    int good = 0, bad = 0;
    for (const Actor& a : curRoom().actors) {
        if (a.kind != 0 || a.id == G.mc || a.id >= G.sims.size()) continue;
        Emotion e = emotionOf(G.sims[a.id]);
        if (e == EM_HAPPY || e == EM_PLAYFUL || e == EM_ENERGIZED || e == EM_CONFIDENT) good++;
        if (e == EM_SAD || e == EM_TENSE) bad++;
    }
    if (good >= 3) addMoodlet(MC(), "Good Vibes Only", EM_HAPPY, 1, 90);
    else if (bad >= 2) addMoodlet(MC(), "Bad Vibes in Here", EM_TENSE, 1, 90);
    // grudge-holders throw a little shade when you're in the same room
    for (const Grudge& g : G.grudges) if (here(g.sim) && irand(4) == 0) { say(g.sim, irand(2) ? "Oh. It's YOU." : "*side-eye*"); break; }
}

// ---------------------------------------------------------------- 6. luck, 7. allowance, grudge decay, secret romance risk
void dramaDaily() {
    Sim& me = MC();
    // lucky day / bad hair day
    int r = irand(100);
    G.luck = r < 15 ? 1 : r < 27 ? -1 : 0;
    if (G.luck > 0) { rewardPopup("LUCKY DAY!", "Everything's going your way today. Socials go better!"); addMoodlet(me, "Lucky Day", EM_CONFIDENT, 1, 900); }
    else if (G.luck < 0) { toast("Bad Hair Day... socials are a little harder today."); addMoodlet(me, "Bad Hair Day", EM_TENSE, 1, 900); }
    // allowance for kids and teens, every Sunday
    if (me.stage >= ST_CHILD && me.stage <= ST_TEEN && weekday(G.day) == 6 && G.allowanceDay != G.day) {
        G.allowanceDay = G.day;
        int amt = me.stage == ST_TEEN ? 40 : 20;
        if (me.gradePts >= 85) amt += 10;
        G.money += amt;
        char b[80]; snprintf(b, sizeof b, "Weekly allowance: +$%d%s", amt, me.gradePts >= 85 ? " (bonus for good grades!)" : ""); toast(b);
    }
    // grudges fade
    for (auto it = G.grudges.begin(); it != G.grudges.end();) { if (it->days <= 1) { if (it->sim < G.sims.size()) toast(G.sims[it->sim].name + " finally let it go."); it = G.grudges.erase(it); } else { it->days--; ++it; } }
    // a secret romance always comes out eventually...
    if (G.affair >= 0) G.affairRisk = (uint8_t)std::min(100, G.affairRisk + 8);
}

// ---------------------------------------------------------------- 8. favourite gifts
const char* dramaGiftReaction(int sim, int kind) {
    if (kind == dramaFavoriteGift(sim)) { changeRel(G.mc, sim, 8, 0); thoughtPopup(sim, "OMG, my FAVORITE! You remembered?!", 1); return "favorite"; }
    return nullptr;
}

// ---------------------------------------------------------------- 9. sleepovers
void dramaSleepover(int t) {
    Sim& me = MC();
    const Sim& s = G.sims[t];
    int h = (int)(G.minute / 60);
    if (h < 17) { say(t, "Sleepover? Ask me tonight!"); return; }
    if (frOf(G.mc, t) < 30) { say(t, "Um, we're not THAT close yet..."); changeRel(G.mc, t, -1, 0); return; }
    say(G.mc, "Sleepover at my place?"); say(t, "YES! I'll bring snacks!");
    travelTo(R_LIVING);
    Actor a; a.kind = 0; a.id = (uint16_t)t; a.x = 250; a.tx = 250; a.y = 220; curRoom().actors.push_back(a);
    Activity act = makeAct("Sleepover", 480, 40); act.kind = AK_SLEEP; act.pose = PO_SLEEP; act.rate[N_ENERGY] = 12; act.rate[N_SOCIAL] = 10; act.rate[N_FUN] = 6; startActivity(act);
    changeRel(G.mc, t, 12, 0);
    dlgSay(-1, "Face masks, a scary movie, prank calls and whispered secrets until 3 AM. Iconic.");
    addMoodlet(me, "Best Sleepover Ever", EM_HAPPY, 2, 720);
    addMemoryOnce(MEM_FUN, G.mc, t, "Sleepover with " + s.name + "!");
}

// ---------------------------------------------------------------- 10. the BFF's boyfriend (secret romance)
// Your best friend is dating someone. One day he makes a move on you.
static int bffPartner(int bff) {
    for (const Rel& r : G.rels) {
        if (r.status < RS_DATING || r.status > RS_MARRIED) continue;
        int o = r.a == bff ? r.b : r.b == bff ? r.a : -1;
        if (o >= 0 && o != G.mc && o < (int)G.sims.size() && !G.sims[o].household) return o;
    }
    return -1;
}
static int pickBffCouple(int& bf) {   // make sure your BFF has a boyfriend (townies date each other!)
    int bff = dramaBff();
    if (bff < 0) return -1;
    bf = bffPartner(bff);
    if (bf < 0) {
        for (int tries = 0; tries < 30 && bf < 0; tries++) {
            int c = anyTownie(G.sims[bff].stage <= ST_TEEN ? ST_TEEN : ST_YA, bff);
            if (c >= 0 && G.sims[c].look.guy != G.sims[bff].look.guy && ageBand(G.sims[c]) == ageBand(G.sims[bff]) && c != G.mc) bf = c;
        }
        if (bf < 0) return -1;
        Rel& r = rel(bff, bf); r.met = true; r.fr = 50; r.ro = 70; r.status = RS_DATING;
    }
    return bff;
}
static std::string bffMove(int choice, int ctx) {
    int bf = ctx & 0xFFFF, bff = G.affairBff;
    if (bf >= (int)G.sims.size() || bff < 0 || bff >= (int)G.sims.size()) return "";
    Sim& me = MC();
    const std::string B = G.sims[bf].name, F = G.sims[bff].name;
    if (choice == 0) {   // kiss him
        G.affair = (int16_t)bf; G.affairRisk = 10;
        rel(G.mc, bf).met = true; changeRel(G.mc, bf, 10, 30);
        addMoodlet(me, "Forbidden Crush", EM_FLIRTY, 3, 720); addMoodlet(me, "Guilty Conscience", EM_TENSE, 2, 720);
        return "Behind the cafe, away from everyone... you kiss " + B + ". Your heart is pounding. " + F + " can NEVER find out. (Secret romance started.)";
    }
    if (choice == 1) {   // turn him down
        changeRel(G.mc, bf, -10, -15); addMoodlet(me, "Loyal", EM_CONFIDENT, 2, 480);
        return "\"She's my best friend. Never.\" " + B + " shrugs and walks away. You did the right thing.";
    }
    // tell your BFF
    if (irand(100) < 70) {
        changeRel(G.mc, bff, 15, 0); rel(bff, bf).status = RS_NONE; rel(bff, bf).ro = -20;
        addMoodlet(me, "Ride or Die", EM_HAPPY, 2, 720);
        return F + " cries, then hugs you SO tight. \"Thank you for telling me.\" She dumps " + B + " by text. Iconic.";
    }
    changeRel(G.mc, bff, -25, 0); addGrudge(bff, 5); addMoodlet(me, "Not Believed", EM_SAD, 2, 720);
    return F + " doesn't believe you. \"You're just JEALOUS!\" Ouch...";
}
static std::string affairCaught(int choice, int) {
    int bf = G.affair, bff = G.affairBff;
    G.affair = -1; G.affairRisk = 0;
    if (bf < 0 || bff < 0 || bf >= (int)G.sims.size() || bff >= (int)G.sims.size()) return "";
    Sim& me = MC();
    const std::string F = G.sims[bff].name;
    rel(bff, bf).status = RS_NONE;
    if (choice == 0) { changeRel(G.mc, bff, -30, 0); addGrudge(bff, 10); addMoodlet(me, "Heartbroken BFF", EM_SAD, 3, 1440); return "You tell " + F + " everything. She's crying. \"I trusted you.\" It might take a long time... but honesty was the only way back."; }
    if (choice == 1) { changeRel(G.mc, bff, -60, 0); addGrudge(bff, 14); scandalBreak(SC_TWOTIMING); addMoodlet(me, "Exposed", EM_SAD, 3, 1440); G.bffs.erase(std::remove(G.bffs.begin(), G.bffs.end(), (uint16_t)bff), G.bffs.end()); return "You deny it... and then " + F + " shows everyone the photos. The whole town knows. Your friendship is OVER."; }
    rel(G.mc, bf).status = RS_DATING; changeRel(G.mc, bff, -50, 0); addGrudge(bff, 14);
    G.bffs.erase(std::remove(G.bffs.begin(), G.bffs.end(), (uint16_t)bff), G.bffs.end());
    addMoodlet(me, "Choosing Love", EM_FLIRTY, 2, 720); addMoodlet(me, "Lost My BFF", EM_SAD, 2, 1440);
    return "\"We're together now. I'm sorry.\" " + F + " storms off. You and " + G.sims[bf].name + " are officially dating... but at what cost?";
}

// ---------------------------------------------------------------- 11. ten little story events
static int g_evSim = -1, g_evSim2 = -1;
static std::string ev(int choice, int id) {
    Sim& me = MC();
    const int a = g_evSim, b = g_evSim2;
    auto nm = [&](int s) { return s >= 0 && s < (int)G.sims.size() ? G.sims[s].name : std::string("someone"); };
    switch (id) {
    case 0:   // The New Girl copies your style
        if (choice == 0) { changeRel(G.mc, a, 10, 0); G.rep = (int16_t)std::min(100, G.rep + 3); return "\"Twinning is cute!\" You take " + nm(a) + " shopping. A new friend - and a mini-me."; }
        if (choice == 1) { changeRel(G.mc, a, -10, 0); G.followers += 40; return "You post a side-by-side: WHO WORE IT BETTER? The comments are brutal (for her)."; }
        skillGain(me, SK_STYLE, 4); return "You switch up your whole look overnight. Let her copy THAT.";
    case 1:   // Frenemy showdown at lunch
        if (choice == 0) { if (frand() < 0.5f + skillLevel(me, SK_CHARISMA) * 0.05f) { changeRel(G.mc, a, -6, 0); addMoodlet(me, "Won the Argument", EM_CONFIDENT, 2, 360); return "You clap back so hard the whole cafeteria goes \"OOOOH.\" " + nm(a) + " has no comeback."; } addMoodlet(me, "Embarrassed", EM_TENSE, 2, 240); return "Your comeback comes out wrong. " + nm(a) + " smirks. Everyone saw."; }
        if (choice == 1) { changeRel(G.mc, a, 8, 0); return "You sit down next to " + nm(a) + " and ask what's REALLY wrong. Turns out it's been a rough week. Truce."; }
        return "You put your headphones on and eat your fries in peace. Not your drama.";
    case 2:   // Class president election (school) / Neighborhood council (adults)
        if (choice == 0) { bool win = frand() < 0.3f + skillLevel(me, SK_CHARISMA) * 0.05f + fameStars() * 0.08f; if (win) { G.rep = (int16_t)std::min(100, G.rep + 10); rewardPopup("YOU WON!", "Madam President, at your service."); addMemoryOnce(MEM_GOAL, G.mc, -1, "Won the election!"); return "The votes are in... you WON! Speech time. You thank your mom, your BFF and your lip gloss."; } addMoodlet(me, "Lost the Election", EM_SAD, 2, 480); return "You lost by three votes. THREE. There's always next year."; }
        if (choice == 1) { changeRel(G.mc, a, 10, 0); return "You run " + nm(a) + "'s campaign instead. The posters are glitter-bombed and gorgeous."; }
        return "Politics? Hard pass.";
    case 3:   // Surprise party (happy)
        for (int s : {a, b}) if (s >= 0) changeRel(G.mc, s, 8, 0);
        me.needs[N_FUN] = 100; me.needs[N_SOCIAL] = 100; addMoodlet(me, "SURPRISE!", EM_HAPPY, 3, 720); addMemoryOnce(MEM_FUN, G.mc, -1, "My friends threw me a surprise party!");
        return choice == 0 ? "You scream, then cry, then dance on the couch. Best. Day. Ever." : choice == 1 ? "You act shocked even though you totally saw the balloons through the window. They're so proud." : "You make a heartfelt speech. Now EVERYONE is crying.";
    case 4:   // Secret admirer revealed (sweet)
        if (choice == 0) { rel(G.mc, a).met = true; changeRel(G.mc, a, 6, 20); addMoodlet(me, "Butterflies", EM_FLIRTY, 3, 720); return "You say yes to a date. " + nm(a) + " looks like they just won the lottery."; }
        if (choice == 1) { rel(G.mc, a).met = true; changeRel(G.mc, a, 10, -5); return "You let " + nm(a) + " down gently. \"Can we still be friends?\" \"...of course.\""; }
        rel(G.mc, a).met = true; changeRel(G.mc, a, 4, 8); return "You keep every single note in a shoebox. Maybe someday.";
    case 5:   // BFF moving away (sad)
        changeRel(G.mc, a, 15, 0); addMoodlet(me, "Missing My BFF", EM_SAD, 3, 1440);
        if (choice == 0) { addMemoryOnce(MEM_FRIEND, G.mc, a, "Friendship bracelets with " + nm(a) + " before the big move."); return "You make matching bracelets and promise to video call every night. " + nm(a) + " ends up moving across town, not across the country. Phew."; }
        if (choice == 1) return "You throw " + nm(a) + " the best going-away party ever. Glitter everywhere. Tears everywhere.";
        G.sims[a].bio = "moved across town - still your BFF"; return "You beg " + nm(a) + "'s parents to let her stay. They say... they'll only move across town. YOU DID IT.";
    case 6:   // Left out of the party (sad)
        if (choice == 0) { addMoodlet(me, "Left Out", EM_SAD, 2, 720); me.needs[N_FUN] = std::fmax(0.f, me.needs[N_FUN] - 25); return "You see the stories. Everyone's there but you. You cry into your ice cream. It's okay to be sad."; }
        if (choice == 1) { G.followers += 60; addMoodlet(me, "Main Character", EM_CONFIDENT, 2, 480); return "You throw your OWN party the same night. Half of their guests leave to come to yours."; }
        changeRel(G.mc, a, 6, 0); return "You text " + nm(a) + ": \"did I do something?\" Turns out the invite got lost. \"I'm SO sorry!!\" Phew.";
    case 7:   // Umbrella for two (sweet)
        if (choice == 0) { rel(G.mc, a).met = true; changeRel(G.mc, a, 6, 14); addMoodlet(me, "Rainy Day Romance", EM_FLIRTY, 2, 480); return "You squeeze under " + nm(a) + "'s umbrella. Your shoulders touch. Neither of you says anything. It's perfect."; }
        if (choice == 1) { me.needs[N_HYGIENE] = std::fmax(0.f, me.needs[N_HYGIENE] - 20); addMoodlet(me, "Soaked", EM_PLAYFUL, 1, 180); return "You run through the rain laughing. Your hair is ruined. Worth it."; }
        rel(G.mc, a).met = true; changeRel(G.mc, a, 8, 4); return "You give " + nm(a) + " YOUR umbrella and walk home in the rain. They're going to remember that forever.";
    case 8:   // Friendship bracelets (sweet)
        changeRel(G.mc, a, 10, 0);
        if (choice == 0) return "Pink, yellow and blue beads that spell BFF. " + nm(a) + " never takes it off.";
        if (choice == 1) { if (b >= 0) changeRel(G.mc, b, 8, 0); return "You make a whole set for the squad. Matching wrists, matching hearts."; }
        skillGain(me, SK_CREATIVITY, 3); return "You start selling them at school. $3 each. Sold out by lunch.";
    case 9:   // The cover shoot (happy)
        if (choice == 0) { G.followers += 300 + G.followers / 10; G.money += 150; rewardPopup("COVER GIRL!", "You're on the cover of Starlight Teen!"); addMemoryOnce(MEM_GOAL, G.mc, -1, "Made the magazine cover!"); return "Flash, flash, FLASH. You nail every pose. +$150 and the whole town has your face on their coffee table."; }
        if (choice == 1) { changeRel(G.mc, a, 12, 0); return "You bring " + nm(a) + " along and the photographer puts you BOTH on the cover. BFF goals."; }
        addMoodlet(me, "Nervous Wreck", EM_TENSE, 1, 240); return "You panic and hide in the bathroom. Maybe next time.";
    }
    return "";
}
struct EvDef { const char* title; const char* text; const char* o[3]; Stage minSt; int need; };   // need: 0 none, 1 friend, 2 townie, 3 two friends
static const EvDef EVS[10] = {
    {"The New Girl", "There's a new girl in town, {A}... and she's wearing YOUR exact outfit. Again. Third day in a row.", {"Take her shopping", "Who wore it better?", "Reinvent your look"}, ST_CHILD, 2},
    {"Lunch Table Showdown", "{A} slams a tray down at your table. \"You told everyone about my crush?!\" (You didn't.) The whole room is watching.", {"Clap back!", "Ask what's wrong", "Headphones on"}, ST_CHILD, 2},
    {"Election Day", "Elections are coming up and {A} says you'd be PERFECT for president. Posters, speeches, glitter...", {"Run for president!", "Run {A}'s campaign", "Pass"}, ST_CHILD, 1},
    {"SURPRISE!", "You walk in and the lights flick on - {A}, {B} and everyone you love jump out from behind the couch!", {"Scream & dance", "Act surprised", "Make a speech"}, ST_CHILD, 3},
    {"Secret Admirer", "For a week, notes have been appearing in your bag. Today you catch them in the act... it's {A}! They turn bright red.", {"Say yes to a date", "Let them down gently", "Keep the notes"}, ST_TEEN, 2},
    {"Moving Away", "{A} has news. Her family is moving. \"I don't want to go,\" she whispers, holding your hand.", {"Friendship bracelets", "Going-away party", "Beg her parents"}, ST_CHILD, 1},
    {"Not Invited", "Everyone's posting from {A}'s party tonight. Everyone. Except you.", {"Feel it", "Throw your OWN party", "Text {A}"}, ST_CHILD, 2},
    {"Umbrella for Two", "It's pouring. {A} runs up and holds out an umbrella. \"Want to share? It's a little small...\"", {"Squeeze in", "Run through the rain", "Give them yours"}, ST_TEEN, 2},
    {"Friendship Bracelets", "{A} brings over a giant box of beads. \"Craft night?\"", {"Make a BFF bracelet", "Make one for the squad", "Start a business"}, ST_CHILD, 1},
    {"The Cover Shoot", "A photographer from Starlight Teen spotted your posts. They want YOU for this month's cover!", {"Strike a pose", "Bring {A} along", "Chicken out"}, ST_CHILD, 1},
};
static std::string fillAB(const char* s) {
    std::string o = s; size_t k;
    while ((k = o.find("{A}")) != std::string::npos) o.replace(k, 3, g_evSim >= 0 ? G.sims[g_evSim].name : std::string("someone"));
    while ((k = o.find("{B}")) != std::string::npos) o.replace(k, 3, g_evSim2 >= 0 ? G.sims[g_evSim2].name : std::string("your friends"));
    return o;
}
static std::string o0, o1, o2;
static bool startEvent(int id) {
    const EvDef& d = EVS[id];
    if (MC().stage < d.minSt) return false;
    g_evSim = g_evSim2 = -1;
    if (d.need == 1) g_evSim = dramaBff() >= 0 ? dramaBff() : anyFriend(20);
    else if (d.need == 3) { g_evSim = anyFriend(20); g_evSim2 = anyFriend(20); if (g_evSim2 == g_evSim) g_evSim2 = -1; }
    else g_evSim = anyTownie(MC().stage <= ST_TEEN ? ST_CHILD : ST_TEEN);
    if (g_evSim < 0) return false;
    if (id == 0 && G.sims[g_evSim].look.guy) return false;   // the New Girl
    if ((id == 4 || id == 7) && !canRomance(G.mc, g_evSim)) return false;
    o0 = fillAB(d.o[0]); o1 = fillAB(d.o[1]); o2 = fillAB(d.o[2]);
    momentCustom(d.title, fillAB(d.text), o0.c_str(), o1.c_str(), o2.c_str(), ev, id, g_evSim);
    G.dramaDay = G.day;
    return true;
}

bool dramaMomentMaybe() {
    if (G.sims.empty() || MC().stage < ST_CHILD) return false;
    const int h = (int)(G.minute / 60);
    if (h < 8 || h > 22) return false;
    // a secret romance getting found out
    if (G.affair >= 0 && G.affairBff >= 0 && irand(100) < G.affairRisk / 3) {
        momentCustom("CAUGHT!", fillAB("") + G.sims[G.affairBff].name + " found the texts between you and " + G.sims[G.affair].name + ". She's shaking. \"Is it TRUE?!\"",
                     "Tell the truth", "Deny everything", "Choose him", affairCaught, 0, G.affairBff);
        return true;
    }
    if (G.dramaDay == G.day) return false;   // at most one story event a day
    // your BFF's boyfriend makes a move (teens and up, once in a while)
    if (MC().stage >= ST_TEEN && G.affair < 0 && irand(100) < 6) {
        int bf = -1, bff = pickBffCouple(bf);
        if (bff >= 0 && bf >= 0 && canRomance(G.mc, bf)) {
            G.affairBff = (int16_t)bff; G.dramaDay = G.day;
            std::string t = G.sims[bf].name + " - " + G.sims[bff].name + "'s boyfriend - corners you after she leaves. \"I can't stop thinking about you. She doesn't have to know...\"";
            momentCustom("Your BFF's Boyfriend", t, "Kiss him...", "Turn him down", "Tell your BFF", bffMove, bf, bf);
            return true;
        }
    }
    if (irand(100) < 14) return startEvent(irand(10));
    return false;
}
