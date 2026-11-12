// STATUS: Starlight Bay's messy social network.
// Everything lives in G.st (saved). Townies post every waking hour based on who they are
// (traits, mood, relationships, grudges, current tea), reply to each other, delete posts
// that the gossip account then screenshots, run fake accounts, spread rumours of varying
// truth, and remember what was said about them.
#include "status.hpp"
#include "game.hpp"
#include "draw.hpp"
#include "sim.hpp"
#include "people.hpp"
#include "audio.hpp"
#include "crime.hpp"
#include "drama.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

// ------------------------------------------------------------------ kinds & tags
enum Kind : uint8_t { K_MOOD, K_OUTFIT, K_COUPLE, K_SUBTWEET, K_SHADE, K_RUMOR, K_CONFESS, K_LEAK, K_SHOT, K_FAKE, K_NOTME, K_UNFOLLOW,
                      K_HYPE, K_CALLOUT, K_APOLOGY, K_DEBUNK, K_SPILL, K_EVENT, K_STORY, K_BREAKUP, K_BREAKING, K_BLOCKED, K_EXPOSEANON, K_COUNT };
enum Flag : uint8_t { F_DELETED = 1, F_LIKED = 2, F_RT = 4, F_LEAKED = 8, F_STORY = 16, F_FOLLOWUP = 32, F_MYANON = 64, F_SEEN = 128 };
static bool negative(int k) { return k == K_SUBTWEET || k == K_SHADE || k == K_RUMOR || k == K_LEAK || k == K_SHOT || k == K_FAKE || k == K_UNFOLLOW || k == K_CALLOUT || k == K_SPILL || k == K_BLOCKED || k == K_EXPOSEANON; }
static const char* TAGS[] = {"", "#OOTD", "#StarlightStyle", "#HardLaunch", "#SoftLaunch", "#Exposed", "#Receipts", "#Confessions", "#SubtweetSzn",
                             "#NotMe", "#FakeAcct", "#GCLeak", "#Blocked", "#Unfollowed", "#NotesAppApology", "#Rumors", "#Breakup", "#MainCharacter",
                             "#Mood", "#TREND", "#BayTea", "#Hype", "#IsItTrue"};
enum { TG_NONE, TG_OOTD, TG_STYLE, TG_HARD, TG_SOFT, TG_EXPOSED, TG_RECEIPTS, TG_CONFESS, TG_SUBTWEET, TG_NOTME, TG_FAKE, TG_GC, TG_BLOCKED,
       TG_UNFOLLOW, TG_SORRY, TG_RUMORS, TG_BREAKUP, TG_MAIN, TG_MOOD, TG_TREND, TG_TEA, TG_HYPE, TG_TRUE, TG_COUNT };

static int trendTop() { return (int)((G.day / 7) * 5 + 3) % NUM_TOPS; }
static std::string trendTag() {
    std::string s = "#";
    for (const char* c = TOP_NAMES[trendTop()]; *c; c++) if (isalnum((unsigned char)*c)) s += *c;
    return s + "Szn";
}
static std::string tagName(int t) { return t == TG_TREND ? trendTag() : TAGS[t % TG_COUNT]; }

// ------------------------------------------------------------------ helpers
static bool validSim(int s) { return s >= 0 && s < (int)G.sims.size(); }
static bool isCast(int s) { return validSim(s) && G.sims[s].bio.rfind("[Cast]", 0) == 0; }
static bool blocked(int s) { return std::find(G.stBlock.begin(), G.stBlock.end(), (uint16_t)s) != G.stBlock.end(); }
static bool following(int s) { return std::find(G.stFollow.begin(), G.stFollow.end(), (uint16_t)s) != G.stFollow.end(); }
static std::string lower(const std::string& n) { std::string o; for (char c : n) if (isalnum((unsigned char)c)) o += (char)tolower((unsigned char)c); return o.empty() ? "user" : o; }
std::string statusHandle(int who, int fakeOf) {
    if (who == ST_ACC_TEA) return "@baybaytea";
    if (who == ST_ACC_CONFESS) return "@starlight.confess";
    if (who == ST_ACC_FAKE) return "@" + (validSim(fakeOf) ? lower(G.sims[fakeOf].name) : std::string("someone")) + "_real";
    if (!validSim(who)) return "@ghost";
    static const char* SUF[10] = {"", ".xo", "_", "babyy", "2real", ".bb", "x", "_official", "lol", ".k"};
    if (who == G.mc) return "@" + lower(G.sims[who].name);
    return "@" + lower(G.sims[who].name) + SUF[(who * 7 + 3) % 10];
}
static std::string dispName(int who, int fakeOf = -1) {
    if (who == ST_ACC_TEA) return "Bay Tea";
    if (who == ST_ACC_CONFESS) return "Confessions";
    if (who == ST_ACC_FAKE) return validSim(fakeOf) ? G.sims[fakeOf].name + " (real)" : "???";
    return validSim(who) ? G.sims[who].name : "?";
}
static std::string nm(int s) { return validSim(s) ? G.sims[s].name : std::string("someone"); }
static int ageMin(const StPost& p) { return (int)(G.day - p.day) * 1440 + (int)G.minute - (int)p.minute; }
static std::string ago(const StPost& p) {
    int m = std::max(0, ageMin(p));
    char b[16];
    if (m < 2) return "now";
    if (m < 60) snprintf(b, sizeof b, "%dm", m);
    else if (m < 1440) snprintf(b, sizeof b, "%dh", m / 60);
    else snprintf(b, sizeof b, "%dd", m / 1440);
    return b;
}
static std::string fmtK(uint32_t n) { char b[16]; if (n >= 10000) snprintf(b, sizeof b, "%uk", n / 1000); else if (n >= 1000) snprintf(b, sizeof b, "%u.%uk", n / 1000, (n % 1000) / 100); else snprintf(b, sizeof b, "%u", n); return b; }
static int fr(int a, int b) { const Rel* r = findRel(a, b); return r ? r->fr : 0; }
static const char* topOf(int s) { return validSim(s) ? TOP_NAMES[G.sims[s].look.topStyle % NUM_TOPS] : "outfit"; }
static const char* botOf(int s) { return validSim(s) ? BOTTOM_NAMES[G.sims[s].look.bt % NUM_BOTTOMS] : "pants"; }
static const char* PLACES[] = {"the cafe", "the mall", "Sunset Beach", "the club", "the rink", "Glitter High", "the park", "the cinema", "the boutique", "the hotel pool", "the gym", "the library"};
static const char* placeNow() { return PLACES[(G.day * 3 + (int)(G.minute / 60)) % 12]; }

// "{A}" author-ish name, "{B}" subject, "{C}" third, "{T}" subject's top, "{Bt}" bottoms, "{P}" place, "{S}" secret, "{TR}" trend item
static std::string fill(const char* t, int A, int B, int Cc = -1, int secret = -1) {
    std::string o;
    for (const char* p = t; *p; p++) {
        if (*p == '{') {
            const char* e = strchr(p, '}');
            if (e) {
                std::string k(p + 1, e - p - 1);
                if (k == "A") o += nm(A); else if (k == "B") o += nm(B); else if (k == "C") o += nm(Cc);
                else if (k == "T") o += topOf(B >= 0 ? B : A); else if (k == "Bt") o += botOf(B >= 0 ? B : A);
                else if (k == "P") o += placeNow(); else if (k == "S") o += secret >= 0 ? SECRETS[secret % NUM_SECRETS] : "did something weird";
                else if (k == "TR") o += TOP_NAMES[trendTop()];
                else if (k == "h") o += statusHandle(B);
                p = e; continue;
            }
        }
        o += *p;
    }
    // tidy: lowercase the first letter for that casual posting voice (names stay capitalised)
    return o;
}
// pick from a pool without repeating a recent line
static uint32_t g_recent[16]; static int g_recentI = 0;
template <size_t N> static const char* pick(const char* const (&pool)[N]) {
    const char* best = pool[irand((int)N)];
    for (int tries = 0; tries < 6; tries++) {
        const char* c = pool[irand((int)N)];
        uint32_t h = (uint32_t)(uintptr_t)c;
        bool seen = false; for (uint32_t r : g_recent) if (r == h) seen = true;
        best = c; if (!seen) break;
    }
    g_recent[g_recentI++ % 16] = (uint32_t)(uintptr_t)best;
    return best;
}

// ------------------------------------------------------------------ the writing
static const char* const MOOD_HAPPY[] = {"today was literally perfect and nobody can ruin it", "smiling for no reason. this is new for me", "good hair day + good friends = unstoppable", "the sun came out and so did i", "in my happy era and it shows"};
static const char* const MOOD_FLIRTY[] = {"someone's been on my mind all day and they know who they are", "eye contact at {P} should honestly be illegal", "if you text me first today... i might text back", "crushing is so embarrassing. anyway hi"};
static const char* const MOOD_CONF[] = {"main character energy. that's it. that's the post", "woke up and chose me", "not everyone gets access to this version of me", "my own biggest fan and i'm not sorry"};
static const char* const MOOD_INSP[] = {"new idea at 3am. sleep is for people without visions", "sketchbook is FULL. need a new one. and a nap", "inspired by literally everything today"};
static const char* const MOOD_PLAY[] = {"it's giving chaos and i'm here for it", "who wants to do something dumb and fun tonight", "just got kicked out of {P} for laughing too loud. worth it"};
static const char* const MOOD_ENER[] = {"ran 3 miles before breakfast. yes i'm insufferable now", "too much iced coffee. i can hear colors", "gym then {P} then who knows"};
static const char* const MOOD_SAD[] = {"why do i always care more than the other person", "deleting this later", "just one of those days. don't ask", "i miss how things used to be", "being the strong friend is exhausting fr"};
static const char* const MOOD_ANGRY[] = {"some people really think they can get away with anything", "i'm SO calm. so calm right now. (i am not calm)", "do NOT test me today", "the disrespect in this town is crazy"};
static const char* const MOOD_TENSE[] = {"overthinking a text i sent 4 hours ago", "if everyone could stop talking about me that'd be great thx", "group chat went quiet the second i joined. cool cool cool"};
static const char* const MOOD_BORED[] = {"starlight bay is so boring today. someone start drama", "bored. reply with your worst take", "nothing ever happens here. (please let something happen)"};
static const char* const MOOD_FINE[] = {"just vibing at {P}", "anyway how's everyone's day", "normal day. suspiciously normal.", "iced coffee count: 3"};

static const char* const OUTFIT_P[] = {"fit check: {T} + {Bt}. rate it 1-10 and be nice", "wore my {T} today and three people asked where it's from. gatekeeping", "OOTD: {T}, {Bt}, attitude", "not to be dramatic but this {T} changed my life", "dressed for the life i want, not the one i have"};
static const char* const OUTFIT_TREND[] = {"jumping on the {TR} trend. did i pull it off??", "{TR} season and i'm the blueprint", "i was wearing a {TR} before it was cool btw"};
static const char* const COUPLE_P[] = {"hard launch. yes it's {B}. no i won't be taking questions", "{B} made me breakfast today. i'm keeping them", "me and {B} against the world", "date night at {P} with {B}. sorry to everyone who's single", "{B} said i look cute today and now i'm unbearable"};
static const char* const SUBTWEET_P[] = {"funny how some people act sweet to your face and then talk about you at {P}", "not naming names but your {T} isn't saving you from your personality", "some of y'all would be SO quiet if i posted the screenshots", "love when people pretend they didn't see my text. you saw it.", "the audacity of SOME people. truly unmatched", "be careful who you trust. that's all i'm saying", "you know who you are. and so does everyone else"};
static const char* const SHADE_P[] = {"who let {B} out of the house in that {T} again", "{B} really wore the {Bt} to {P}. iconic. derogatory.", "respectfully, {B}, the {T} is giving clearance rack", "{B}'s fit today... we need to talk", "{B} copied my {T} and thought i wouldn't notice. i notice everything"};
static const char* const HYPE_P[] = {"{B} is the best person in this whole town and i won't be taking criticism", "shoutout to {B} for showing up for me today", "{B}'s style is unmatched. everyone copy their homework", "if you're not friends with {B} you're missing out. fact", "{B} deserves the world honestly"};
static const char* const CALLOUT_P[] = {"{h} i know you're reading this. say it to my face next time", "{h} we need to talk. now.", "not {h} lying about me at {P}. i have witnesses", "{h} blocked me so i'm posting this instead. you're fake"};
static const char* const UNFOLLOW_P[] = {"unfollowed {B}. growth looks good on me", "unfollowed someone today and the air feels lighter", "cleaning up my following list. you know who you are"};
static const char* const EVENT_P[] = {"who's going to {P} tonight??", "{P} is PACKED right now. come thru", "best night at {P} in forever. if you weren't there, sorry"};
static const char* const STORY_P[] = {"at {P} rn", "outfit of the hour", "this song on repeat", "guess where i am", "late night snack run", "mirror selfie, obviously"};
static const char* const CONFESS_P[] = {"i still have feelings for {B} and they're dating {C}. this is fine", "i ate the last slice at {P} and blamed {B}. never telling", "i pretend to hate {B} but i'd honestly do anything for them", "my crush is {B} and if you screenshot this i will cry", "i've worn the same {T} 3 days in a row and nobody noticed", "i rehearsed saying hi to {B} for a week and still said 'you too'", "i lied about being busy to stay home and watch cartoons. three times this week"};
static const char* const NOTME_P[] = {"that account is NOT me. report it. whoever made it, i'm coming for you", "for the record i did not make that account. i have taste", "whoever is pretending to be me online... get a life"};
static const char* const FAKE_P[] = {"hi it's {B}!! i love being cringe!! follow me!!", "i am {B} and i still sleep with a nightlight :)", "{B} here. my outfits are borrowed and my personality is too"};
static const char* const DEBUNK_P[] = {"that rumor about me is FAKE. who even makes this stuff up", "y'all believe anything. it's not true. moving on", "i'm laughing. that's not what happened. not even close"};
static const char* const CONFIRM_P[] = {"ok yes the rumor is true. are we happy now", "i'm not going to deny it. it happened. let me live"};
static const char* const BREAKUP_P[] = {"it's over between me and {B}. please respect our privacy (spread it everywhere i don't care)", "single. don't ask. don't text {B} either", "me and {B} are done. i'm fine. i'm totally fine"};
static const char* const APOLOGY_P[] = {"i've been doing a lot of reflecting... i'm sorry to anyone i hurt. i'm learning", "taking a step back from STATUS for a bit. i wasn't my best self lately. love you guys", "i owe some people an apology and i'm working on it. thank you for the grace"};

// replies
static const char* const R_AGREE[] = {"SAY IT LOUDER", "not you posting what we were all thinking", "this.", "facts tbh", "the way i screamed"};
static const char* const R_DEFEND[] = {"leave {B} alone fr", "{B} has done nothing to you. log off", "this is so weird of you", "{B} is literally the nicest person ever. try again", "jealousy is not cute on you"};
static const char* const R_CONFRONT[] = {"say it to my face.", "everyone knows this is about me. grow up", "i'm literally right here??", "screenshotted. enjoy", "wow ok. noted."};
static const char* const R_LOL[] = {"lmaooo", "grabbing popcorn", "STATUS is so messy today i love it", "not the comments turning into a courtroom", "who is this about", "drop names coward", "i'm telling", "the tea is SCALDING"};
static const char* const R_HYPE[] = {"YESSS", "stunning omg", "obsessed with this", "10/10 no notes", "the {T}!!!", "ok main character", "save some slay for the rest of us"};
static const char* const R_CRIT[] = {"it's... a choice", "6/10 the shoes are carrying", "bold of you", "the {T} is fighting for its life"};
static const char* const R_SPILL[] = {"i heard it was worse than that", "wait til you hear the full story", "there's more. dm me", "not the only time either"};
static const char* const R_SORRY_OK[] = {"proud of you for this", "we love growth", "accepted. come to brunch"};
static const char* const R_SORRY_NO[] = {"notes app apology lol", "this is a PR move", "say it in person"};
// the player's reply lines by tone
static const char* const ME_HYPE[] = {"obsessed with this", "you're so real for this", "YES!! love this", "this made my day"};
static const char* const ME_DEFEND[] = {"leave {B} alone. seriously", "{B} is my friend and this is not it", "nobody asked. {B} is amazing", "be so for real right now"};
static const char* const ME_CLAP[] = {"the confidence of someone who's never been right", "wow. that's crazy. anyway", "say less... no seriously, say less", "this you?"};
static const char* const ME_SPILL[] = {"oh there's MORE to this story", "i was there. it was worse", "not me knowing the full tea", "should i tell them or will you"};

// ------------------------------------------------------------------ posting
static StPost& push(int who, int kind, const std::string& text, int about = -1, int about2 = -1, int tag = TG_NONE) {
    StPost p;
    p.who = (int16_t)who; p.kind = (uint8_t)kind; p.text = text; p.about = (int16_t)about; p.about2 = (int16_t)about2; p.tag = (uint8_t)tag;
    p.day = G.day; p.minute = (uint16_t)G.minute;
    uint32_t base = 3 + irand(25);
    if (isCast(who)) base = base * 3 + 40;
    if (who == ST_ACC_TEA) base = 60 + irand(140);
    if (who == G.mc) base += G.followers / 10 + 5;
    if (negative(kind)) { p.heat = (uint8_t)(20 + irand(40)); base += p.heat; }
    if (kind == K_COUPLE || kind == K_BREAKUP || kind == K_BREAKING) p.heat = (uint8_t)std::max<int>(p.heat, 25 + irand(25));
    p.likes = base;
    if (kind == K_STORY) p.flags |= F_STORY;
    G.st.insert(G.st.begin(), p);
    // trim: drop the oldest posts that aren't yours
    while (G.st.size() > 90) {
        auto it = std::find_if(G.st.rbegin(), G.st.rend(), [](const StPost& q) { return q.who != G.mc; });
        if (it == G.st.rend()) { G.st.pop_back(); continue; }
        G.st.erase(std::next(it).base());
    }
    return G.st.front();
}
static void reply(StPost& p, int who, int tone, const std::string& t) { if (who == -1) return; if (p.replies.size() < 6) { StReply r; r.who = (int16_t)who; r.tone = (uint8_t)tone; r.text = t; p.replies.push_back(r); } }

// people who care about a sim (friends) and people who'd side with the author
static std::vector<int> friendsOf(int s, int minFr, int exclude1, int exclude2) {
    std::vector<int> v;
    for (const Rel& r : G.rels) {
        if (r.fr < minFr) continue;
        int o = r.a == s ? r.b : r.b == s ? r.a : -1;
        if (o < 0 || o == exclude1 || o == exclude2 || !validSim(o) || G.sims[o].stage < ST_TEEN) continue;
        v.push_back(o);
    }
    return v;
}
static int randomTownie(int notA = -1, int notB = -1) {
    for (int tries = 0; tries < 30; tries++) {
        int s = irand((int)G.sims.size());
        if (s == notA || s == notB || s == G.cop || G.sims[s].stage < ST_TEEN) continue;
        return s;
    }
    return -1;
}
// how the town reacts in the replies
static void genReplies(StPost& p) {
    int A = p.who, B = p.about;
    bool neg = negative(p.kind);
    if (neg && validSim(B)) {
        if (B != G.mc && irand(100) < (G.sims[B].hasTrait(TR_HOTHEAD) ? 95 : 55)) { reply(p, B, 2, pick(R_CONFRONT)); if (validSim(A)) changeRel(A, B, -3, 0); }
        auto d = friendsOf(B, 40, A, B);
        for (int k = 0; k < 2 && !d.empty(); k++) { int x = d[irand((int)d.size())]; if (x == G.mc) continue; reply(p, x, 1, fill(pick(R_DEFEND), A, B)); if (validSim(A)) changeRel(x, A, -2, 0); }
        if (validSim(A)) { auto f = friendsOf(A, 40, A, B); if (!f.empty()) { int x = f[irand((int)f.size())]; if (x != G.mc) reply(p, x, 0, pick(R_AGREE)); } }
        if (irand(2)) { int x = randomTownie(A, B); if (x >= 0 && x != G.mc) reply(p, x, irand(3) ? 4 : 3, irand(3) ? pick(R_LOL) : pick(R_SPILL)); }
    } else if (p.kind == K_OUTFIT || p.kind == K_COUPLE || p.kind == K_HYPE || p.kind == K_MOOD) {
        int who = A >= 0 ? A : B;
        auto f = friendsOf(who, 25, who, -1);
        int n = 1 + irand(3);
        for (int k = 0; k < n && !f.empty(); k++) { int x = f[irand((int)f.size())]; if (x == G.mc) continue; reply(p, x, 0, fill(pick(R_HYPE), x, who)); }
        if (p.kind == K_OUTFIT && irand(4) == 0) { int x = randomTownie(who); if (x >= 0 && x != G.mc && G.sims[x].hasTrait(TR_FASHIONISTA)) reply(p, x, 3, fill(pick(R_CRIT), x, who)); }
        if (p.replies.empty() && irand(2)) { int x = randomTownie(who); if (x >= 0 && x != G.mc) reply(p, x, 0, fill(pick(R_HYPE), x, who)); }
    } else if (p.kind == K_APOLOGY) {
        for (int k = 0; k < 2; k++) { int x = randomTownie(A); if (x >= 0 && x != G.mc) reply(p, x, 0, fr(x, A) > 10 ? pick(R_SORRY_OK) : pick(R_SORRY_NO)); }
    } else if (irand(3) == 0) { int x = randomTownie(A); if (x >= 0 && x != G.mc) reply(p, x, 4, pick(R_LOL)); }
}
// side effects of an NPC (or account) post
static void consequences(StPost& p) {
    int A = p.who, B = p.about;
    if (!validSim(B)) return;
    if (negative(p.kind)) {
        if (validSim(A)) changeRel(A, B, -4, 0);
        if (B == G.mc) {
            toast("STATUS: " + statusHandle(A, p.about2) + " posted about you!");
            G.rep = (int16_t)std::max(-100, G.rep - 1);
            if (p.kind == K_RUMOR && p.cred == 0 && G.affair >= 0) G.affairRisk = (uint8_t)std::min(100, G.affairRisk + 15);
        }
    } else if (p.kind == K_HYPE || p.kind == K_COUPLE) {
        if (validSim(A)) changeRel(A, B, 3, p.kind == K_COUPLE ? 3 : 0);
        if (B == G.mc) { toast("STATUS: " + statusHandle(A) + (p.kind == K_COUPLE ? " posted you! <3" : " hyped you up! <3")); G.followers += 8 + irand(12); }
    }
}

// ------------------------------------------------------------------ NPCs post
static int enemyOf(int s) {
    if (dramaHasGrudge(s)) return G.mc;
    int best = -1, lo = -10;
    for (const Rel& r : G.rels) {
        int o = r.a == s ? r.b : r.b == s ? r.a : -1;
        if (o < 0 || !validSim(o) || G.sims[o].stage < ST_TEEN) continue;
        if (r.fr < lo) { lo = r.fr; best = o; }
    }
    return best;
}
static int bestieOf(int s) {
    int best = -1, hi = 50;
    for (const Rel& r : G.rels) {
        int o = r.a == s ? r.b : r.b == s ? r.a : -1;
        if (o < 0 || !validSim(o) || G.sims[o].stage < ST_TEEN) continue;
        if (r.fr > hi) { hi = r.fr; best = o; }
    }
    return best;
}
static void npcPost(int s) {
    const Sim& me = G.sims[s];
    int enemy = enemyOf(s), bestie = bestieOf(s), partner = partnerOf(s);
    struct Opt { int kind, w; };
    std::vector<Opt> o = {{K_MOOD, 4}, {K_OUTFIT, 2 + (me.hasTrait(TR_FASHIONISTA) || me.hasTrait(TR_GLAMOROUS) ? 4 : 0)}, {K_EVENT, me.hasTrait(TR_OUTGOING) ? 2 : 1}, {K_STORY, 3},
                          {K_SHADE, 1 + (me.hasTrait(TR_GLAMOROUS) ? 2 : 0)}};
    if (validSim(partner)) o.push_back({K_COUPLE, me.hasTrait(TR_ROMANTIC) ? 5 : 3});
    if (enemy >= 0) { o.push_back({K_SUBTWEET, 5}); o.push_back({K_UNFOLLOW, 1}); if (me.hasTrait(TR_HOTHEAD)) o.push_back({K_CALLOUT, 3}); }
    if (bestie >= 0) o.push_back({K_HYPE, me.hasTrait(TR_CHEERFUL) ? 4 : 2});
    int tot = 0; for (auto& x : o) tot += x.w;
    int r = irand(tot), kind = K_MOOD;
    for (auto& x : o) { if (r < x.w) { kind = x.kind; break; } r -= x.w; }
    switch (kind) {
    case K_MOOD: {
        const char* t;
        switch (emotionOf(me)) {
        case EM_HAPPY: t = pick(MOOD_HAPPY); break; case EM_FLIRTY: t = pick(MOOD_FLIRTY); break; case EM_CONFIDENT: t = pick(MOOD_CONF); break;
        case EM_INSPIRED: t = pick(MOOD_INSP); break; case EM_PLAYFUL: t = pick(MOOD_PLAY); break; case EM_ENERGIZED: t = pick(MOOD_ENER); break;
        case EM_SAD: t = pick(MOOD_SAD); break; case EM_ANGRY: t = pick(MOOD_ANGRY); break; case EM_TENSE: t = pick(MOOD_TENSE); break;
        case EM_BORED: t = pick(MOOD_BORED); break; default: t = pick(MOOD_FINE); break;
        }
        StPost& p = push(s, K_MOOD, fill(t, s, -1), -1, -1, irand(3) ? TG_NONE : TG_MOOD);
        if (emotionOf(me) == EM_SAD && irand(3) == 0) p.ttl = (uint8_t)(2 + irand(3));   // "deleting this later"
        genReplies(p);
        break;
    }
    case K_OUTFIT: {
        bool tr = me.look.topStyle == trendTop();
        StPost& p = push(s, K_OUTFIT, fill(tr ? pick(OUTFIT_TREND) : pick(OUTFIT_P), s, s), s, -1, tr ? TG_TREND : (irand(2) ? TG_OOTD : TG_STYLE));
        if (tr) p.likes = p.likes * 2 + 30;
        p.about = -1;
        genReplies(p);
        break;
    }
    case K_EVENT: { StPost& p = push(s, K_EVENT, fill(pick(EVENT_P), s, -1)); genReplies(p); break; }
    case K_STORY: push(s, K_STORY, fill(pick(STORY_P), s, -1)); break;
    case K_COUPLE: { StPost& p = push(s, K_COUPLE, fill(pick(COUPLE_P), s, partner), partner, -1, irand(2) ? TG_HARD : TG_SOFT); genReplies(p); consequences(p); break; }
    case K_SUBTWEET: {
        StPost& p = push(s, K_SUBTWEET, fill(pick(SUBTWEET_P), s, enemy), enemy, -1, TG_SUBTWEET);
        if (irand(4) == 0) p.ttl = (uint8_t)(1 + irand(3));
        genReplies(p); consequences(p);
        break;
    }
    case K_SHADE: {
        int t = enemy >= 0 && irand(2) ? enemy : randomTownie(s);
        if (t < 0) return;
        StPost& p = push(s, K_SHADE, fill(pick(SHADE_P), s, t), t, -1, TG_STYLE);
        if (irand(3) == 0) p.ttl = (uint8_t)(1 + irand(3));
        genReplies(p); consequences(p);
        break;
    }
    case K_HYPE: { StPost& p = push(s, K_HYPE, fill(pick(HYPE_P), s, bestie), bestie, -1, TG_HYPE); genReplies(p); consequences(p); break; }
    case K_CALLOUT: { StPost& p = push(s, K_CALLOUT, fill(pick(CALLOUT_P), s, enemy), enemy, -1, TG_TEA); genReplies(p); consequences(p); break; }
    case K_UNFOLLOW: {
        StPost& p = push(s, K_UNFOLLOW, fill(pick(UNFOLLOW_P), s, enemy), enemy, -1, TG_UNFOLLOW);
        genReplies(p); consequences(p);
        break;
    }
    }
}
// the gossip account
static void teaPost() {
    // 1. receipts: screenshot a deleted post nobody leaked yet
    for (StPost& q : G.st) if ((q.flags & F_DELETED) && !(q.flags & F_LEAKED) && q.who >= 0 && q.who != G.mc && ageMin(q) < 600 && irand(2)) {
        q.flags |= F_LEAKED;
        std::string t = "RECEIPTS: " + nm(q.who) + " deleted this but we saw it -> \"" + q.text + "\"";
        StPost& p = push(ST_ACC_TEA, K_SHOT, t, q.who, -1, TG_RECEIPTS);
        p.heat = (uint8_t)std::min(100, p.heat + 25);
        genReplies(p); consequences(p);
        return;
    }
    // 2. breaking: today's tabloid headlines
    for (const Scandal& s : G.tea) if (s.day + 1 >= G.day && validSim(s.sim)) {
        bool done = false; for (const StPost& q : G.st) if (q.kind == K_BREAKING && q.about == s.sim && q.day >= s.day) done = true;
        if (done) continue;
        StPost& p = push(ST_ACC_TEA, K_BREAKING, "BREAKING: " + scandalHeadline(s), s.sim, -1, TG_EXPOSED);
        p.cred = 0;
        genReplies(p); if (s.sim == G.mc) toast("STATUS: you're trending... and not in a good way.");
        return;
    }
    int roll = irand(10);
    if (roll < 2) {   // leaked group chat
        int a = randomTownie(), b = randomTownie(a), c = randomTownie(a, b);
        if (a < 0 || b < 0 || c < 0) return;
        if (irand(4) == 0 && G.mc != a && G.mc != c) b = G.mc;
        static const char* const GC[] = {"\"ok but has anyone actually seen {B} be nice\"", "\"{B}'s new {T}... thoughts??\"", "\"don't tell {B} but i'm not going to their thing\"", "\"is it just me or is {B} SO dramatic lately\""};
        std::string t = "LEAKED GC: " + nm(a) + ": " + fill(pick(GC), a, b) + "  " + nm(c) + ": \"lmaooo stop\"";
        StPost& p = push(ST_ACC_TEA, K_LEAK, t, b, a, TG_GC);
        changeRel(a, b, -6, 0); changeRel(c, b, -3, 0);
        genReplies(p); consequences(p);
        if (b == G.mc) thoughtPopup(a, "Ugh, who leaked the group chat?!", -1);
        return;
    }
    // 3. rumours of varying truth
    int a = randomTownie();
    if (a < 0) return;
    // you're more of a target when your clout is low or you're having an affair
    if ((G.stClout < -30 && irand(3) == 0) || (G.affair >= 0 && G.affairRisk > 30 && irand(3) == 0)) a = G.mc;
    int pa = partnerOf(a);
    std::string t; int about2 = -1; uint8_t cred = 2;
    int kind = irand(6);
    if (a == G.mc && G.affair >= 0 && validSim(G.affairBff)) {
        t = "ANON TIP: someone has been VERY cozy with " + nm(G.affairBff) + "'s man. we know who. do you??"; cred = 0; about2 = G.affair;
    } else if (kind == 0) {
        int b = validSim(pa) && irand(2) ? pa : randomTownie(a);
        if (b < 0) return;
        t = fill("SPOTTED: {A} and {B} sharing ONE milkshake at {P}. friends do that... right??", a, b); about2 = b;
        cred = (validSim(pa) && pa == b) ? 0 : (fr(a, b) > 40 ? 1 : 2);
    } else if (kind == 1 && validSim(pa)) {
        t = fill("trouble in paradise? {A} and {B} haven't posted each other in a WEEK", a, pa); about2 = pa; cred = 1;
    } else if (kind == 2) {
        t = fill("sources say {A}'s 'designer' bag is from the mall kiosk. we're not saying it's fake. we're asking.", a, -1); cred = (uint8_t)(irand(3) ? 2 : 0);
    } else if (kind == 3) {
        t = fill("has anyone seen {A}?? no posts in 3 days. we're worried. (we're nosy)", a, -1); cred = 2;
    } else if (kind == 4 && validSim(pa)) {
        t = fill("who was {A} texting at 2am... because it wasn't {B}", a, pa); about2 = pa; cred = (uint8_t)(irand(3) == 0 ? 0 : 2);
    } else {
        int sc = secretOf(a);
        t = fill("allegedly {A} {S}. allegedly.", a, -1, -1, sc); cred = (uint8_t)(irand(2) ? 0 : 1);
    }
    StPost& p = push(ST_ACC_TEA, K_RUMOR, t, a, about2, irand(2) ? TG_RUMORS : TG_TRUE);
    p.cred = cred;
    genReplies(p); consequences(p);
}
static void confessPost() {
    int b = randomTownie(), c = randomTownie(b);
    if (b < 0) return;
    if (irand(6) == 0) b = G.mc;
    StPost& p = push(ST_ACC_CONFESS, K_CONFESS, fill(pick(CONFESS_P), -1, b, c), b, -1, TG_CONFESS);
    genReplies(p);
    if (b == G.mc) toast("STATUS: someone confessed something about you...");
}
static void fakePost() {
    int b = -1;
    for (int k = 0; k < 6 && b < 0; k++) { int s = randomTownie(); if (s >= 0 && (isCast(s) || irand(3) == 0)) b = s; }
    if (b < 0) return;
    if (irand(8) == 0 && G.followers > 300) b = G.mc;
    StPost& p = push(ST_ACC_FAKE, K_FAKE, fill(pick(FAKE_P), -1, b), b, b, TG_FAKE);
    genReplies(p); consequences(p);
}

// a few hours later: the subject of a post answers it
static void followUps() {
    for (size_t i = 0; i < G.st.size(); i++) {
        StPost& q = G.st[i];
        if (q.flags & F_FOLLOWUP) continue;
        int age = ageMin(q);
        if (age < 50 || age > 60 * 8) continue;
        int B = q.about;
        if (!validSim(B) || B == G.mc) { if (age > 120) q.flags |= F_FOLLOWUP; continue; }
        if (q.kind == K_RUMOR && irand(2)) {
            q.flags |= F_FOLLOWUP;
            if (q.cred == 2 || (q.cred == 1 && irand(2))) { q.cred = 2; StPost& p = push(B, K_DEBUNK, pick(DEBUNK_P), B, -1, TG_NOTME); genReplies(p); }
            else if (irand(3) == 0) { q.cred = 0; StPost& p = push(B, K_DEBUNK, pick(CONFIRM_P), B, -1, TG_TRUE); genReplies(p); }
            return;
        }
        if (q.kind == K_FAKE) { q.flags |= F_FOLLOWUP; StPost& p = push(B, K_NOTME, pick(NOTME_P), B, -1, TG_NOTME); genReplies(p); return; }
        if ((q.kind == K_SUBTWEET || q.kind == K_SHADE) && validSim(q.who) && G.sims[B].hasTrait(TR_HOTHEAD) && irand(2)) {
            q.flags |= F_FOLLOWUP; StPost& p = push(B, K_CALLOUT, fill(pick(CALLOUT_P), B, q.who), q.who, -1, TG_TEA); genReplies(p); consequences(p); return;
        }
        if (q.kind == K_COUPLE && irand(14) == 0 && validSim(q.who) && validSim(B) && partnerOf(q.who) != B) {
            q.flags |= F_FOLLOWUP; StPost& p = push(q.who, K_BREAKUP, fill(pick(BREAKUP_P), q.who, B), B, -1, TG_BREAKUP); genReplies(p); return;
        }
        if (age > 120) q.flags |= F_FOLLOWUP;
    }
}

void statusPostAs(int who, const std::string& text, int about) {
    if (G.sims.size() < 3 || who == -1 || who >= (int)G.sims.size() || who < -4) return;
    if (about >= (int)G.sims.size()) about = -1;
    bool neg = about >= 0 && who != about && text.find("love") == std::string::npos;
    StPost& p = push(who, neg ? K_SUBTWEET : K_MOOD, text, about, -1, neg ? TG_TEA : TG_MOOD);
    genReplies(p);
    if (neg) consequences(p);
}
static_assert(SP_COUPLE == K_COUPLE && SP_SUBTWEET == K_SUBTWEET && SP_RUMOR == K_RUMOR && SP_HYPE == K_HYPE && SP_EVENT == K_EVENT && SP_BREAKUP == K_BREAKUP && SP_BREAKING == K_BREAKING, "STATUS kinds");
void statusPostKind(int who, int kind, const std::string& text, int about, int about2) {
    if (G.sims.size() < 3 || who == -1 || who >= (int)G.sims.size() || who < -4 || kind < 0 || kind >= K_COUNT) return;
    if (about >= (int)G.sims.size() || about < -1) about = -1;
    if (about2 >= (int)G.sims.size() || about2 < -1) about2 = -1;
    if (validSim(who) && blocked(who) && about == G.mc) return;
    int tag = kind == K_COUPLE ? TG_SOFT : kind == K_BREAKUP ? TG_BREAKUP : kind == K_SUBTWEET ? TG_SUBTWEET : kind == K_RUMOR ? TG_RUMORS
            : kind == K_HYPE ? TG_HYPE : (kind == K_EVENT || kind == K_BREAKING) ? TG_TEA : TG_MOOD;
    StPost& p = push(who, kind, text, about, about2, tag);
    if (kind == K_RUMOR) p.cred = 0;   // life-engine rumours are true
    genReplies(p);
    consequences(p);
}
void statusAnnounce(const std::string& text, int about) {
    StPost& p = push(ST_ACC_TEA, K_HYPE, text, about, -1, TG_TEA);
    p.likes = p.likes * 4 + G.followers / 5; p.heat = 40;
    genReplies(p);
    if (about == G.mc) toast("STATUS: you're trending on @baybaytea!");
}
void statusSeed() {
    if (G.sims.size() < 3) return;
    for (int i = 0; i < 10; i++) { int s = randomTownie(); if (s >= 0) npcPost(s); }
    teaPost(); confessPost();
    for (StPost& p : G.st) { int back = irand(300); int m = (int)p.minute - back; if (m < 0) { m += 1440; if (p.day) p.day--; } p.minute = (uint16_t)m; }
}
void statusHourly(int hour) {
    if (G.sims.size() < 3) return;
    if (G.st.empty()) { statusSeed(); return; }
    // likes keep rolling in, stories expire, regret sets in
    for (StPost& p : G.st) {
        int age = ageMin(p);
        if (age < 1440 && !(p.flags & F_DELETED)) p.likes += p.likes / 7 + p.heat / 3 + irand(4);
        if (p.ttl) { if (--p.ttl == 0) p.flags |= F_DELETED; }
        if (p.heat > 0 && age > 720) p.heat = (uint8_t)(p.heat * 3 / 4);
    }
    G.st.erase(std::remove_if(G.st.begin(), G.st.end(), [](const StPost& p) { return (p.flags & F_STORY) && ageMin(p) > 1440; }), G.st.end());
    bool awake = hour >= 8 || hour <= 1;
    if (!awake) return;
    int n = 1 + irand(2) + (hour >= 19 ? 1 : 0);
    for (int i = 0; i < n; i++) { int s = randomTownie(G.mc); if (s >= 0) npcPost(s); }
    if (irand(100) < 45) teaPost();
    if (irand(100) < 18) confessPost();
    if (irand(100) < 5) fakePost();
    followUps();
    // your confession can get traced back to you
    for (StPost& p : G.st) if ((p.flags & F_MYANON) && ageMin(p) > 120 && irand(10) == 0) {
        p.flags &= (uint8_t)~F_MYANON;
        const int crush = p.about;
        StPost& q = push(ST_ACC_TEA, K_EXPOSEANON, "UPDATE: the confession about " + nm(crush) + " was sent by... " + MC().name + ". you're welcome", G.mc, crush, TG_EXPOSED);
        q.cred = 0;
        toast("STATUS: your anonymous confession got traced back to you!!");
        if (validSim(crush)) { const Rel* r = findRel(G.mc, crush); if (r && r->ro > 20) { changeRel(G.mc, crush, 2, 8); thoughtPopup(crush, "Wait... that confession was from YOU? That's kind of cute.", 2); } else changeRel(G.mc, crush, -4, 0); }
        break;
    }
    // your posts earn you followers as the likes come in
    uint32_t mine = 0; for (const StPost& p : G.st) if (p.who == G.mc && ageMin(p) < 180) mine += p.likes;
    if (mine) G.followers += mine / 60 + (G.stPrivate ? 0 : 1);
}

static int g_talkGuard = -1;
void statusOnTalk(int t) {
    if (!validSim(t) || t == g_talkGuard || irand(3)) return;
    for (const StPost& p : G.st) {
        if (ageMin(p) > 3 * 1440) break;
        if (p.flags & F_DELETED) continue;
        if (p.who == G.mc && p.about == t) {
            g_talkGuard = t;
            if (negative(p.kind)) thoughtPopup(t, "Still not over that post you made about me.", -1);
            else thoughtPopup(t, "Still smiling about your post about me!", 1);
            return;
        }
        if (p.who == t && p.about == G.mc && negative(p.kind)) { g_talkGuard = t; thoughtPopup(t, "Hope you saw my post. ;)", -1); return; }
        if (p.who == G.mc && (p.kind == K_OUTFIT) && irand(4) == 0) { g_talkGuard = t; thoughtPopup(t, "Saw your fit pic on STATUS. Cute!", 1); return; }
    }
}

// ================================================================== the app
enum Tab { TB_FORYOU, TB_FOLLOW, TB_TEA, TB_STORIES, TB_ME, TB_COUNT };
static const char* TAB_NAMES[TB_COUNT] = {"For You", "Following", "Tea", "Stories", "Me"};
enum Mode { MD_FEED, MD_COMPOSE, MD_PICK, MD_REPLY };
static int g_tab = TB_FORYOU, g_sel = 0, g_mode = MD_FEED, g_action = 0, g_pickPage = 0;
static std::string g_note;
static float g_noteT = 0;
static void note(const std::string& s) { g_note = s; g_noteT = 3.f; }

static bool visible(const StPost& p) {
    if (validSim(p.who) && blocked(p.who)) return false;
    switch (g_tab) {
    case TB_STORIES: return (p.flags & F_STORY) != 0;
    case TB_FOLLOW: return !(p.flags & F_STORY) && (p.who == G.mc || (validSim(p.who) && following(p.who)));
    case TB_ME: return !(p.flags & F_STORY) && (p.who == G.mc || (p.flags & F_RT) || p.about == G.mc || p.about2 == G.mc);
    default: return !(p.flags & F_STORY);
    }
}
static float score(const StPost& p) {
    float age = ageMin(p) / 60.f;
    float s = std::log((float)p.likes + 2.f) * 6.f - age * 2.5f;
    if (g_tab == TB_TEA) s += p.heat * 0.6f + (negative(p.kind) ? 14 : 0) + (p.tag == TG_TREND ? 10 : 0) - age * 1.5f;
    else s += p.heat * 0.15f;
    if (p.about == G.mc) s += 18;
    if (validSim(p.who) && following(p.who)) s += 10;
    if (isCast(p.who)) s += 6;
    if (p.tag == TG_TREND) s += 5;
    if (p.flags & F_DELETED) s -= 30;
    return s;
}
static std::vector<int> feed() {
    std::vector<int> v;
    for (size_t i = 0; i < G.st.size(); i++) if (visible(G.st[i])) v.push_back((int)i);
    if (g_tab == TB_FORYOU || g_tab == TB_TEA) std::stable_sort(v.begin(), v.end(), [](int a, int b) { return score(G.st[a]) > score(G.st[b]); });
    return v;
}
static StPost* selected() { auto v = feed(); if (v.empty()) return nullptr; g_sel = std::clamp(g_sel, 0, (int)v.size() - 1); return &G.st[v[g_sel]]; }

// trending hashtags
static std::vector<std::pair<int, int>> trending() {
    int sc[TG_COUNT] = {0};
    for (const StPost& p : G.st) if (ageMin(p) < 2880 && !(p.flags & F_DELETED) && p.tag) sc[p.tag] += 1 + (int)(p.likes / 40) + p.heat / 4;
    sc[TG_TREND] += 6;
    std::vector<std::pair<int, int>> v;
    for (int t = 1; t < TG_COUNT; t++) if (sc[t]) v.push_back({sc[t], t});
    std::sort(v.rbegin(), v.rend());
    if (v.size() > 5) v.resize(5);
    return v;
}

// ------------------------------------------------------------------ the player's posts
enum Act { A_OUTFIT, A_STORY, A_MOOD, A_HYPE, A_LAUNCH, A_SUBTWEET, A_SPILL, A_RUMOR, A_CONFESS, A_APOLOGY, A_COUNT };
static const char* ACT_NAMES[A_COUNT] = {"Outfit Pic", "Post a Story", "Mood Update", "Hype a Friend", "Hard Launch", "Subtweet", "Spill Tea", "Start a Rumor", "Anon Confession", "Apology Post"};
static bool needsTarget(int a) { return a == A_HYPE || a == A_LAUNCH || a == A_SUBTWEET || a == A_SPILL || a == A_RUMOR || a == A_CONFESS; }
static std::vector<int> candidates(int a) {
    std::vector<int> v;
    if (a == A_SPILL) { for (const Dirt& d : G.dirt) if (d.state == 0 && validSim(d.sim)) v.push_back(d.sim); return v; }
    if (a == A_LAUNCH) { int p = partnerOf(G.mc); if (validSim(p)) v.push_back(p); if (validSim(G.affair) && G.affair != p) v.push_back(G.affair); return v; }
    for (const Rel& r : G.rels) {
        int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1;
        if (o < 0 || !validSim(o) || G.sims[o].stage < ST_TEEN || !r.met) continue;
        if (a == A_HYPE && r.fr < 10) continue;
        v.push_back(o);
    }
    std::sort(v.begin(), v.end(), [&](int x, int y) { return a == A_HYPE ? fr(G.mc, x) > fr(G.mc, y) : fr(G.mc, x) < fr(G.mc, y); });
    v.erase(std::unique(v.begin(), v.end()), v.end());
    return v;
}
static int myStyle() { return skillLevel(MC(), SK_STYLE) + (MC().hasTrait(TR_FASHIONISTA) ? 3 : 0) + (MC().hasTrait(TR_GLAMOROUS) ? 2 : 0); }
static void clout(int d) { G.stClout = (int16_t)std::clamp(G.stClout + d, -100, 100); }
static void playerPost(int a, int t) {
    const int me = G.mc;
    G.stPostsMade++; G.postsMade++;
    float priv = G.stPrivate ? 0.45f : 1.f;
    auto seen = [&]() { return irand(100) < (G.stPrivate ? 35 : 70); };
    switch (a) {
    case A_OUTFIT: {
        bool tr = MC().look.topStyle == trendTop();
        StPost& p = push(me, K_OUTFIT, fill(tr ? pick(OUTFIT_TREND) : pick(OUTFIT_P), me, me), -1, -1, tr ? TG_TREND : TG_OOTD);
        p.likes = (uint32_t)((20 + G.followers / 8 + myStyle() * 18 + (G.luck > 0 ? 40 : G.luck < 0 ? -10 : 0)) * (tr ? 1.7f : 1.f) * priv) + irand(20);
        G.followers += p.likes / 8; clout(1);
        genReplies(p);
        if (myStyle() < 3 && irand(2)) { int x = randomTownie(me); if (x >= 0) reply(p, x, 3, fill(pick(R_CRIT), x, me)); }
        note(tr ? "You're on trend! The likes are pouring in." : "Fit pic posted!");
        break;
    }
    case A_STORY: { StPost& p = push(me, K_STORY, fill(pick(STORY_P), me, -1)); p.likes = (uint32_t)(G.followers / 3 * priv) + 3; G.followers += 2 + irand(4); note("Story posted. It disappears in 24 hours."); break; }
    case A_MOOD: {
        const char* tx;
        switch (emotionOf(MC())) { case EM_HAPPY: tx = pick(MOOD_HAPPY); break; case EM_SAD: tx = pick(MOOD_SAD); break; case EM_ANGRY: tx = pick(MOOD_ANGRY); break; case EM_FLIRTY: tx = pick(MOOD_FLIRTY); break;
                                   case EM_CONFIDENT: tx = pick(MOOD_CONF); break; case EM_BORED: tx = pick(MOOD_BORED); break; case EM_TENSE: tx = pick(MOOD_TENSE); break; default: tx = pick(MOOD_FINE); break; }
        StPost& p = push(me, K_MOOD, fill(tx, me, -1), -1, -1, TG_MOOD); genReplies(p); note("Posted!");
        break;
    }
    case A_HYPE: {
        StPost& p = push(me, K_HYPE, fill(pick(HYPE_P), me, t), t, -1, TG_HYPE);
        changeRel(me, t, 6, 0); clout(3); p.reposts++;
        reply(p, t, 0, "omg stoppp i love you <3");
        thoughtPopup(t, "Aww, " + MC().name + " posted about me!", 1);
        genReplies(p); note(nm(t) + " reposted it!");
        break;
    }
    case A_LAUNCH: {
        StPost& p = push(me, K_COUPLE, fill(pick(COUPLE_P), me, t), t, -1, TG_HARD);
        p.likes = p.likes * 2 + 40; G.followers += 20 + irand(30);
        if (t == G.affair && validSim(G.affairBff)) {
            p.heat = 100;
            int bff = G.affairBff;
            changeRel(me, bff, -45, 0); dramaAddGrudge(bff, 12);
            reply(p, bff, 2, "ARE YOU SERIOUS RIGHT NOW. HE'S MY BOYFRIEND.");
            reply(p, ST_ACC_TEA, 4, "we are SCREAMING. the betrayal. the hard launch. the AUDACITY");
            changeRel(me, t, 0, 10);
            G.affair = -1; G.affairBff = -1; G.affairRisk = 0;
            scandalBreak(SC_TWOTIMING); clout(-25);
            thoughtPopup(bff, "I can't believe I found out from STATUS.", -1);
            note("You hard-launched your BFF's boyfriend. The whole town saw.");
        } else {
            changeRel(me, t, 3, 8); clout(2);
            reply(p, t, 0, "finally!! <3"); thoughtPopup(t, "We're official official now!", 2);
            genReplies(p); note("Hard launch complete. Everyone's freaking out.");
        }
        break;
    }
    case A_SUBTWEET: {
        StPost& p = push(me, K_SUBTWEET, fill(pick(SUBTWEET_P), me, t), t, -1, TG_SUBTWEET);
        G.followers += 4 + p.heat / 4; clout(-3);
        if (seen()) {
            changeRel(me, t, -8, 0); dramaAddGrudge(t, 3);
            reply(p, t, 2, pick(R_CONFRONT));
            thoughtPopup(t, "Wow. Real mature, " + MC().name + ".", -1);
            auto d = friendsOf(t, 40, me, t);
            for (int k = 0; k < 2 && !d.empty(); k++) { int x = d[irand((int)d.size())]; changeRel(me, x, -3, 0); reply(p, x, 1, fill(pick(R_DEFEND), me, t)); }
            note(nm(t) + " saw it. Obviously.");
        } else { reply(p, randomTownie(me, t), 4, "who is this about??"); note("Posted. Nobody's sure who it's about... yet."); }
        break;
    }
    case A_SPILL: {
        int di = dirtIndex(t);
        int sec = di >= 0 ? G.dirt[di].secret : secretOf(t);
        StPost& p = push(me, K_SPILL, fill("{B} {S}. you didn't hear it from me", me, t, -1, sec), t, -1, TG_EXPOSED);
        p.heat = 90; p.cred = 0; p.likes = p.likes * 3 + 80;
        if (di >= 0) G.dirt[di].state = 2;
        npcScandal(t, SC_SECRET, sec);
        changeRel(me, t, -25, 0); dramaAddGrudge(t, 10);
        G.followers += 30 + irand(60); G.rep = (int16_t)std::max(-100, G.rep - 3); clout(-6);
        reply(p, t, 2, "HOW do you even know that. i'm done with you.");
        thoughtPopup(t, "I trusted you!", -1);
        genReplies(p); note("The tea has been spilled. There's no un-spilling it.");
        break;
    }
    case A_RUMOR: {
        static const char* const RM[] = {"heard {B} got kicked out of {P}. allegedly", "not saying {B}'s bag is fake but... the logo is upside down", "{B} and their ex were spotted at {P} last night. just saying", "someone said {B} pays people to like their posts. can't confirm"};
        StPost& p = push(me, K_RUMOR, fill(pick(RM), me, t), t, -1, TG_RUMORS);
        p.cred = 2; G.followers += 10 + irand(25); clout(-4);
        genReplies(p); changeRel(me, t, -5, 0);
        note("Rumor's out. Hope it doesn't get traced back...");
        break;
    }
    case A_CONFESS: {
        static const char* const MC_C[] = {"i think about {B} way more than i should", "i've had a crush on {B} since forever. if you know, you know", "{B} said hi to me today and i forgot how to walk"};
        StPost& p = push(ST_ACC_CONFESS, K_CONFESS, fill(pick(MC_C), -1, t), t, -1, TG_CONFESS);
        p.flags |= F_MYANON;
        genReplies(p); note("Sent anonymously. Probably. Hopefully.");
        break;
    }
    case A_APOLOGY: {
        StPost& p = push(me, K_APOLOGY, pick(APOLOGY_P), -1, -1, TG_SORRY);
        genReplies(p);
        bool sincere = G.stClout < 0 || G.myScandal > 0 || !G.grudges.empty();
        if (G.myScandal) G.myScandal = (uint8_t)std::max(0, (int)G.myScandal - 15);
        if (!G.grudges.empty()) { G.grudges.erase(G.grudges.begin()); }
        clout(sincere ? 8 : -2);
        note(sincere ? "People are starting to forgive you." : "Apologizing for... nothing? People are confused.");
        break;
    }
    }
    sfxTap();
}

// ------------------------------------------------------------------ interacting with posts
static void doReplyTone(StPost& p, int tone) {
    const int me = G.mc, A = p.who, B = p.about;
    std::string t;
    switch (tone) {
    case 0: t = fill(pick(ME_HYPE), me, A); if (validSim(A) && A != me) { changeRel(me, A, 3, 0); thoughtPopup(A, "Love that you commented!", 1); } break;
    case 1:
        if (!validSim(B) || B == me) { t = "this isn't it. be nice"; break; }
        t = fill(pick(ME_DEFEND), me, B); changeRel(me, B, 5, 0); clout(2);
        if (validSim(A) && A != me) changeRel(me, A, -4, 0);
        thoughtPopup(B, MC().name + " defended me on STATUS!", 1);
        break;
    case 2:
        t = pick(ME_CLAP);
        if (validSim(A) && A != me) changeRel(me, A, -6, 0);
        G.followers += 5 + p.heat / 5;
        if (B == me) { G.rep = (int16_t)std::min(100, G.rep + 1); clout(1); } else clout(-1);
        p.heat = (uint8_t)std::min(100, p.heat + 10);
        if (validSim(A) && A != me && irand(100) < 45) reply(p, A, 2, pick(R_CONFRONT));
        break;
    case 3:
        t = pick(ME_SPILL); p.heat = (uint8_t)std::min(100, p.heat + 15);
        if (validSim(B) && B != me) { changeRel(me, B, -6, 0); dramaAddGrudge(B, 2); }
        G.followers += 6 + irand(10); clout(-2);
        break;
    }
    reply(p, me, (uint8_t)tone, t);
    p.likes += 2;
    sfxTap();
}
static void like(StPost& p) {
    if (p.flags & F_LIKED) { p.flags &= (uint8_t)~F_LIKED; if (p.likes) p.likes--; return; }
    p.flags |= F_LIKED; p.likes++;
    if (validSim(p.who) && p.who != G.mc) changeRel(G.mc, p.who, 1, 0);
    if (negative(p.kind) && validSim(p.about) && p.about != G.mc && irand(2)) { changeRel(G.mc, p.about, -3, 0); thoughtPopup(p.about, "Did " + MC().name + " just LIKE that post about me?", -1); }
    sfxTap();
}
static void repost(StPost& p) {
    if (p.flags & F_RT) { note("You already reposted this."); return; }
    p.flags |= F_RT; p.reposts++; p.likes += 3;
    if (validSim(p.who) && p.who != G.mc) changeRel(G.mc, p.who, 3, 0);
    if (negative(p.kind) && validSim(p.about) && p.about != G.mc) { changeRel(G.mc, p.about, -6, 0); dramaAddGrudge(p.about, 2); clout(-2); note(nm(p.about) + " saw you repost that..."); }
    else note("Reposted to your profile.");
    G.followers += 1 + p.heat / 20;
    sfxTap();
}
static void screenshotOrDelete(StPost& p) {
    if (p.who == G.mc) {
        if (p.flags & F_DELETED) { note("Already deleted."); return; }
        p.flags |= F_DELETED;
        if (negative(p.kind) && irand(100) < 40) {
            p.flags |= F_LEAKED;
            StPost& q = push(ST_ACC_TEA, K_SHOT, "RECEIPTS: " + MC().name + " deleted this but we saw it -> \"" + p.text + "\"", G.mc, -1, TG_RECEIPTS);
            genReplies(q); clout(-3);
            note("Deleted... but @baybaytea already screenshotted it.");
        } else note("Deleted. Nobody saw. Probably.");
        return;
    }
    if (p.flags & F_LEAKED) { note("Someone already leaked this one."); return; }
    p.flags |= F_LEAKED;
    std::string t = "screenshotted this before it gets deleted -> \"" + p.text + "\"";
    const int author = p.who, heat = std::min(100, p.heat + 20);
    StPost& q = push(G.mc, K_SHOT, t, validSim(author) ? author : p.about, -1, TG_RECEIPTS);
    q.heat = (uint8_t)heat;
    if (validSim(author)) { changeRel(G.mc, author, -10, 0); dramaAddGrudge(author, 3); thoughtPopup(author, "Who screenshots people?! Seriously?", -1); }
    G.followers += 10 + q.heat / 4; clout(-2);
    genReplies(q);
    note("Screenshotted and posted. Receipts!");
    sfxTap();
}
static void followToggle(int s) {
    if (!validSim(s) || s == G.mc) { note("That's a special account. You can't follow it."); return; }
    auto it = std::find(G.stFollow.begin(), G.stFollow.end(), (uint16_t)s);
    if (it != G.stFollow.end()) {
        G.stFollow.erase(it); changeRel(G.mc, s, -5, 0);
        if (irand(2)) thoughtPopup(s, "Did " + MC().name + " just unfollow me?!", -1);
        note("Unfollowed " + nm(s) + ".");
    } else {
        G.stFollow.push_back((uint16_t)s); changeRel(G.mc, s, 2, 0);
        if (fr(G.mc, s) > 10 || irand(3) == 0) { G.followers++; note(nm(s) + " followed you back!"); } else note("Following " + nm(s) + ".");
    }
    sfxTap();
}
static void blockToggle(int s) {
    if (!validSim(s) || s == G.mc) { note("You can't block that one."); return; }
    auto it = std::find(G.stBlock.begin(), G.stBlock.end(), (uint16_t)s);
    if (it != G.stBlock.end()) { G.stBlock.erase(it); note("Unblocked " + nm(s) + "."); return; }
    G.stBlock.push_back((uint16_t)s);
    auto f = std::find(G.stFollow.begin(), G.stFollow.end(), (uint16_t)s); if (f != G.stFollow.end()) G.stFollow.erase(f);
    changeRel(G.mc, s, -15, 0);
    if (irand(2)) {
        StPost& p = push(ST_ACC_TEA, K_BLOCKED, MC().name + " BLOCKED " + nm(s) + "?? what happened between them??", s, G.mc, TG_BLOCKED);
        reply(p, s, 2, "blocked me for telling the truth. ok.");
        genReplies(p);
    }
    note("Blocked " + nm(s) + ". Their posts are gone from your feed.");
    sfxTap();
}

// ------------------------------------------------------------------ layout
static Btn tabB(int i) { return Btn{4 + i * 63.f, 22, 60, 22}; }
static Btn actB(int i) { return Btn{6 + (i % 4) * 77.f, 126 + (i / 4) * 28.f, 74, 25}; }
static const Btn bNew{6, 182, 150, 24}, bPriv{162, 182, 152, 24}, bCancel{240, 210, 76, 26};
static Btn gridB(int i) { return Btn{6 + (i % 2) * 156.f, 24 + (i / 2) * 31.f, 152, 28}; }
static const Btn bMore{86, 210, 150, 26};

void statusAppUpdate(const Input& in) {
    if (g_noteT > 0) g_noteT -= in.dt;
    if (MC().stage < ST_TEEN) return;
    if (G.st.empty()) statusSeed();
    const touchPosition& tp = in.tp;
    if (g_mode == MD_FEED) {
        if (in.down & KEY_L) { g_tab = (g_tab + TB_COUNT - 1) % TB_COUNT; g_sel = 0; sfxTap(); }
        if (in.down & KEY_R) { g_tab = (g_tab + 1) % TB_COUNT; g_sel = 0; sfxTap(); }
        if (in.down & KEY_UP) g_sel = std::max(0, g_sel - 1);
        if (in.down & KEY_DOWN) g_sel++;
        if (in.down & KEY_A) { StPost* p = selected(); if (p) like(*p); }
        if (!in.touch) return;
        for (int i = 0; i < TB_COUNT; i++) if (tabB(i).in(tp)) { g_tab = i; g_sel = 0; sfxTap(); return; }
        if (bNew.in(tp)) { g_mode = MD_COMPOSE; sfxTap(); return; }
        if (bPriv.in(tp)) { G.stPrivate = !G.stPrivate; note(G.stPrivate ? "Private account: only friends see your posts." : "Public account: everyone can see you."); sfxTap(); return; }
        StPost* p = selected();
        for (int i = 0; i < 8; i++) if (actB(i).in(tp)) {
            if (i == 6) { g_sel = std::max(0, g_sel - 1); sfxTap(); return; }
            if (i == 7) { g_sel++; selected(); sfxTap(); return; }
            if (!p) return;
            if (p->flags & F_DELETED && i != 3) { note("This post was deleted."); return; }
            switch (i) {
            case 0: like(*p); break;
            case 1: repost(*p); break;
            case 2: g_mode = MD_REPLY; sfxTap(); break;
            case 3: screenshotOrDelete(*p); break;
            case 4: followToggle(p->who == ST_ACC_FAKE ? -1 : p->who); break;
            case 5: blockToggle(p->who); break;
            }
            return;
        }
    } else if (g_mode == MD_COMPOSE) {
        if ((in.touch && bCancel.in(tp))) { g_mode = MD_FEED; sfxTap(); return; }
        if (!in.touch) return;
        for (int i = 0; i < A_COUNT; i++) if (gridB(i).in(tp)) {
            if (needsTarget(i)) {
                if (candidates(i).empty()) { note(i == A_SPILL ? "You don't know anyone's secrets yet. Eavesdrop!" : i == A_LAUNCH ? "You need someone to launch first!" : "You need to meet more people first."); sfxNo(); return; }
                g_action = i; g_pickPage = 0; g_mode = MD_PICK; sfxTap(); return;
            }
            playerPost(i, -1); g_mode = MD_FEED; g_tab = TB_ME; g_sel = 0; return;
        }
    } else if (g_mode == MD_PICK) {
        if (!in.touch) return;
        if (bCancel.in(tp)) { g_mode = MD_COMPOSE; sfxTap(); return; }
        auto v = candidates(g_action);
        int pages = std::max(1, ((int)v.size() + 7) / 8);
        if (bMore.in(tp)) { g_pickPage = (g_pickPage + 1) % pages; sfxTap(); return; }
        for (int i = 0; i < 8; i++) { int k = g_pickPage * 8 + i; if (k < (int)v.size() && gridB(i).in(tp)) { playerPost(g_action, v[k]); g_mode = MD_FEED; g_tab = g_action == A_CONFESS ? TB_FORYOU : TB_ME; g_sel = 0; return; } }
    } else if (g_mode == MD_REPLY) {
        if (!in.touch) return;
        if (bCancel.in(tp)) { g_mode = MD_FEED; sfxTap(); return; }
        StPost* p = selected();
        for (int i = 0; i < 4; i++) if (gridB(i + 2).in(tp)) { if (p) doReplyTone(*p, i); g_mode = MD_FEED; return; }
    }
}

// ------------------------------------------------------------------ drawing
static void avatar(int who, int fakeOf, float cx, float cy, float r, float t) {
    if (validSim(who)) { circle(cx, cy, r + 1.5f, C(UI_INK)); circle(cx, cy, r, C(255, 236, 244)); drawSimPortrait(G.sims[who], cx, cy, r, t); return; }
    RGB c = who == ST_ACC_TEA ? RGB{255, 46, 140} : who == ST_ACC_CONFESS ? RGB{150, 96, 230} : RGB{120, 120, 130};
    circle(cx, cy, r + 1.5f, C(UI_INK)); circle(cx, cy, r, C(c));
    std::string L = who == ST_ACC_TEA ? "T" : who == ST_ACC_CONFESS ? "?" : (validSim(fakeOf) ? G.sims[fakeOf].name.substr(0, 1) : std::string("!"));
    textOutline(L.c_str(), cx, cy - r * 0.75f, r / 15.f, C(255, 255, 255), C(UI_INK), 1, 0.8f);
}
static void card(const StPost& p, float y, bool sel, float t) {
    const float x = 6, w = 388, h = 46;
    RGB fill = sel ? RGB{255, 228, 240} : p.who == G.mc ? RGB{240, 236, 255} : RGB{255, 255, 255};
    flashPanel(x, y, w, h, fill, 6, 248);
    if (sel) outline(x, y, w, h, 2, C(255, 46, 140));
    avatar(p.who, p.about2, x + 17, y + 18, 11, t);
    if (p.flags & F_STORY) { for (int k = 0; k < 16; k++) { float a = k * 0.3927f; circle(x + 17 + std::cos(a) * 13.5f, y + 18 + std::sin(a) * 13.5f, 1.3f, C(255, 120, 60)); } }
    std::string head = dispName(p.who, p.about2);
    text(head.c_str(), x + 34, y + 2, 0.42f, C(UI_INK));
    float hw = textW(head.c_str(), 0.42f);
    if (isCast(p.who) || p.who == ST_ACC_TEA) sparkle(x + 40 + hw, y + 9, 4.5f, C(60, 160, 250));
    std::string meta = statusHandle(p.who, p.about2) + "  " + ago(p);
    if (p.who == G.mc && G.stPrivate) meta += "  [private]";
    text(meta.c_str(), x + 48 + hw, y + 4, 0.34f, C(150, 130, 150));
    if (p.tag) { std::string tg = tagName(p.tag); text(tg.c_str(), x + w - 6, y + 3, 0.34f, C(p.tag == TG_TREND ? RGB{255, 120, 40} : RGB{60, 140, 230}), 2); }
    if (p.flags & F_DELETED) text("[this post has been deleted]", x + 34, y + 18, 0.38f, C(170, 160, 170));
    else {
        auto ls = wrapText(p.text, 0.38f, 290);
        for (size_t k = 0; k < ls.size() && k < 2; k++) { std::string L = ls[k]; if (k == 1 && ls.size() > 2) L += "..."; text(L.c_str(), x + 34, y + 16 + k * 13.f, 0.38f, C(UI_INK)); }
    }
    char b[48];
    heart(x + w - 46, y + 22, 4, C((p.flags & F_LIKED) ? RGB{255, 46, 120} : RGB{210, 190, 200}));
    text(fmtK(p.likes).c_str(), x + w - 6, y + 16, 0.34f, C(UI_INK), 2);
    snprintf(b, sizeof b, "RT %u  re %u", (unsigned)p.reposts, (unsigned)p.replies.size());
    text(b, x + w - 6, y + 30, 0.32f, C((p.flags & F_RT) ? RGB{40, 170, 120} : RGB{150, 130, 150}), 2);
    if (p.kind == K_RUMOR) {
        const char* badge = (p.flags & F_FOLLOWUP) ? (p.cred == 2 ? "DEBUNKED" : p.cred == 0 ? "CONFIRMED" : "UNCONFIRMED") : "UNCONFIRMED";
        RGB bc = !strcmp(badge, "DEBUNKED") ? RGB{120, 120, 130} : !strcmp(badge, "CONFIRMED") ? RGB{220, 40, 70} : RGB{240, 160, 40};
        float bw = textW(badge, 0.3f) + 8, bx = x + w - 8 - (p.tag ? textW(tagName(p.tag).c_str(), 0.34f) + 6 : 0) - bw;
        rrect(bx, y + 3, bw, 11, 3, C(bc)); text(badge, bx + 4, y + 3, 0.3f, C(255, 255, 255));
    }
}
void statusAppDrawTop(float t) {
    grad(0, 0, TOP_W, TOP_H, C(244, 240, 252), C(232, 224, 248));
    grad(0, 0, TOP_W, 22, C(120, 80, 240), C(255, 60, 150));
    textOutline("STATUS", 8, 0, 0.62f, C(255, 255, 255), C(UI_INK), 0, 1.1f);
    char b[96];
    snprintf(b, sizeof b, "%s  |  %s followers  |  clout %+d", TAB_NAMES[g_tab], fmtK(G.followers).c_str(), (int)G.stClout);
    text(b, 392, 4, 0.38f, C(255, 255, 255), 2);
    if (MC().stage < ST_TEEN) { textOutline("STATUS is for teens and up!", 200, 110, 0.6f, C(255, 46, 140), C(255, 255, 255), 1); return; }
    // trending strip
    auto tr = trending();
    std::string ts = "Trending: ";
    for (size_t i = 0; i < tr.size() && i < 4; i++) ts += tagName(tr[i].second) + "  ";
    rect(0, 22, TOP_W, 15, C(255, 255, 255, 200));
    text(ts.c_str(), 8, 23, 0.36f, C(120, 60, 200));
    auto v = feed();
    if (g_tab == TB_ME) {
        flashPanel(6, 40, 388, 34, RGB{255, 236, 246}, 6);
        avatar(G.mc, -1, 24, 57, 13, t);
        snprintf(b, sizeof b, "%s  %s", MC().name.c_str(), statusHandle(G.mc).c_str());
        text(b, 44, 42, 0.44f, C(UI_INK));
        snprintf(b, sizeof b, "%s followers   %d following   %d posts   %s", fmtK(G.followers).c_str(), (int)G.stFollow.size(), (int)G.stPostsMade, G.stPrivate ? "Private" : "Public");
        text(b, 44, 58, 0.34f, C(120, 100, 130));
        const char* rep = G.stClout >= 40 ? "Beloved" : G.stClout >= 10 ? "Liked" : G.stClout > -10 ? "Neutral" : G.stClout > -40 ? "Messy" : "Problematic";
        text(rep, 388, 44, 0.4f, C(G.stClout >= 0 ? RGB{40, 170, 120} : RGB{220, 40, 70}), 2);
    }
    float y0 = g_tab == TB_ME ? 78 : 40;
    int rows = g_tab == TB_ME ? 3 : 4;
    if (v.empty()) { text(g_tab == TB_FOLLOW ? "Follow people to fill this feed!" : g_tab == TB_STORIES ? "No stories right now." : "Nothing here yet... post something!", 200, y0 + 50, 0.5f, C(150, 120, 160), 1); return; }
    g_sel = std::clamp(g_sel, 0, (int)v.size() - 1);
    int start = std::clamp(g_sel - 1, 0, std::max(0, (int)v.size() - rows));
    for (int k = 0; k < rows && start + k < (int)v.size(); k++) card(G.st[v[start + k]], y0 + k * 49.f, start + k == g_sel, t);
    snprintf(b, sizeof b, "%d / %d", g_sel + 1, (int)v.size());
    text(b, 392, 228, 0.32f, C(150, 130, 160), 2);
}
void statusAppDrawBottom(float t) {
    bottomBackdrop(t);
    if (MC().stage < ST_TEEN) { textOutline("Come back when you're a teen!", 160, 100, 0.5f, C(255, 255, 255), C(UI_INK), 1); return; }
    char b[96];
    if (g_mode == MD_FEED) {
        for (int i = 0; i < TB_COUNT; i++) button(tabB(i), TAB_NAMES[i], g_tab == i, 3, 0.36f);
        StPost* p = selected();
        flashPanel(6, 48, 308, 74, RGB{255, 255, 255}, 6, 235);
        if (p) {
            float y = 50;
            if (p->replies.empty()) text("No replies yet. Be the first!", 160, 76, 0.4f, C(150, 130, 150), 1);
            for (size_t k = 0; k < p->replies.size() && y < 112; k++) {
                const StReply& r = p->replies[k];
                std::string line = statusHandle(r.who) + ": " + r.text;
                auto ls = wrapText(line, 0.34f, 296);
                RGB c = r.who == G.mc ? RGB{120, 60, 200} : r.tone == 2 ? RGB{220, 40, 70} : r.tone == 1 ? RGB{40, 150, 110} : RGB{70, 50, 70};
                for (size_t j = 0; j < ls.size() && j < 2 && y < 112; j++) { text(ls[j].c_str(), 12, y, 0.34f, C(c)); y += 11; }
                y += 1;
            }
        }
        bool mine = p && p->who == G.mc;
        bool fol = p && validSim(p->who) && following(p->who), blk = p && validSim(p->who) && blocked(p->who);
        const char* L[8] = {p && (p->flags & F_LIKED) ? "Unlike" : "Like", "Repost", "Reply", mine ? "Delete" : "Screenshot", fol ? "Unfollow" : "Follow", blk ? "Unblock" : "Block", "Up", "Down"};
        for (int i = 0; i < 8; i++) button(actB(i), L[i], false, i < 4 ? i % 3 : (i < 6 ? 0 : 1), 0.38f);
        button(bNew, "+ New Post", false, 0, 0.44f);
        button(bPriv, G.stPrivate ? "Account: Private" : "Account: Public", G.stPrivate, 3, 0.4f);
    } else if (g_mode == MD_COMPOSE) {
        textOutline("What are you posting?", 160, 4, 0.45f, C(255, 255, 255), C(UI_INK), 1);
        for (int i = 0; i < A_COUNT; i++) button(gridB(i), ACT_NAMES[i], false, i < 5 ? 1 : (i < 8 ? 0 : 3), 0.42f);
        button(bCancel, "Cancel", false, 1, 0.42f);
    } else if (g_mode == MD_PICK) {
        snprintf(b, sizeof b, "%s: who?", ACT_NAMES[g_action]);
        textOutline(b, 160, 4, 0.45f, C(255, 255, 255), C(UI_INK), 1);
        auto v = candidates(g_action);
        for (int i = 0; i < 8; i++) {
            int k = g_pickPage * 8 + i; if (k >= (int)v.size()) break;
            snprintf(b, sizeof b, "%s (%s)", nm(v[k]).c_str(), relLabel(G.mc, v[k]));
            button(gridB(i), b, false, i % 4, 0.36f);
        }
        int pages = std::max(1, ((int)v.size() + 7) / 8);
        if (pages > 1) { snprintf(b, sizeof b, "More (%d/%d)", g_pickPage + 1, pages); button(bMore, b, false, 3, 0.4f); }
        button(bCancel, "Back", false, 1, 0.42f);
    } else if (g_mode == MD_REPLY) {
        textOutline("Reply how?", 160, 4, 0.45f, C(255, 255, 255), C(UI_INK), 1);
        StPost* p = selected();
        if (p) { auto ls = wrapText(p->text, 0.36f, 296); for (size_t k = 0; k < ls.size() && k < 3; k++) text(ls[k].c_str(), 12, 26 + k * 12.f, 0.36f, C(255, 255, 255)); }
        static const char* T[4] = {"Hype it up", "Defend them", "Clap back", "Spill more tea"};
        for (int i = 0; i < 4; i++) button(gridB(i + 2), T[i], false, i, 0.44f);
        button(bCancel, "Cancel", false, 1, 0.42f);
    }
    if (g_noteT > 0 && !g_note.empty()) {
        auto ls = wrapText(g_note, 0.4f, 300);
        flashPanel(10, g_mode == MD_FEED ? 2 : 192, 300, 16 + 12.f * std::min<size_t>(1, ls.size() - 1), RGB{255, 46, 140}, 6, 240);
        for (size_t k = 0; k < ls.size() && k < 2; k++) text(ls[k].c_str(), 160, (g_mode == MD_FEED ? 3 : 193) + k * 12.f, 0.4f, C(255, 255, 255), 1);
    }
}
