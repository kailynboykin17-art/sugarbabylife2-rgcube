#include "dailylife.hpp"
#include "famdrama.hpp"
#include "lifeext.hpp"
#include "tween.hpp"
#include "spicy.hpp"
#include "lockup.hpp"
#include "content.hpp"
#include "lockup.hpp"
#include "lifestory.hpp"
#include "kids.hpp"
#include "social.hpp"
#include "romance.hpp"
#include "lifeplus.hpp"
#include "cook.hpp"
#include "squads.hpp"
#include "goals.hpp"
#include "extras.hpp"
#include "crime.hpp"
#include "chatter.hpp"
#include "drama.hpp"
#include "cutscene.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

// ================================================================ texting
const TextPrompt PROMPTS[] = {
    {"hey!! what r u up to?", false,
     {{"Just chillin, u?", 3, 0, "same lol"}, {"Shoe shopping, obviously", 4, 0, "omg take me next time!!"}, {"Busy. Bye.", -5, 0, "ok then..."}}},
    {"did u see what happened at the club last nite??", false,
     {{"Spill the tea!!", 5, 0, "ok so basically..."}, {"I was THERE", 3, 0, "wait WHAT. call me"}, {"Don't care tbh", -4, 0, "wow ok"}}},
    {"I need outfit advice. zebra or leopard?", false,
     {{"Zebra, always", 4, 0, "ur right as always"}, {"Leopard is SO you", 5, 0, "ahh ty!! <3"}, {"Why not both??", 3, 0, "ur a genius"}}},
    {"wanna hang out sometime?", false,
     {{"Totally! <3", 6, 0, "yay!! this weekend?"}, {"Maybe this weekend", 2, 0, "cool cool"}, {"I'm good", -5, 0, "oh. ok"}}},
    {"my mom took my phone away lol so this is from my laptop", false,
     {{"LOL what did u do", 4, 0, "nothing!! (something)"}, {"Rip", 2, 0, "ikr"}, {"Why tell me?", -3, 0, "nvm"}}},
    {"rate my new hair 1-10", false,
     {{"11 obviously", 5, 0, "STOP <3"}, {"Solid 8", 1, 0, "ok fair"}, {"...it'll grow back", -4, 0, "wow."}}},
    {"can't stop thinking about u lol", true,
     {{"Same here ;)", 2, 6, "!!!! :)"}, {"Aww that's sweet", 3, 2, "hehe"}, {"That's kinda weird", -3, -6, "oh. sorry"}}},
    {"what's ur idea of a perfect date?", true,
     {{"Candlelit dinner", 2, 5, "same!! fancy"}, {"Dancing all night", 3, 4, "u can keep up? ;)"}, {"Staying home. Alone.", -1, -3, "haha... ok"}}},
    {"u looked amazing today", true,
     {{"I know ;)", 1, 3, "confident. i like it"}, {"Ur sweet, u too!", 3, 5, "<3"}, {"...ok", -2, -3, "never mind"}}},
    {"goodnight cutie", true,
     {{"Night! Sweet dreams", 2, 4, "<3"}, {"<3 <3 <3", 2, 6, "<3 <3 <3 <3"}, {"Who is this?", -4, -8, "WOW"}}},
    {"so... are we a thing or what? lol", true,
     {{"I'd like that :)", 3, 8, "me too!!"}, {"Let's see where it goes", 2, 2, "fair"}, {"Just friends", 2, -10, "oh. ok."}}},
    // ---- drama from rivals (kind 2) ----
    {"saw ur outfit today. bold choice lol", 2,
     {{"At least I HAVE style", -6, 0, "wow ok miss thing"}, {"Thanks babe, I know ;)", 2, 0, "...whatever"}, {"Blocked.", -10, 0, "[you left them on read]"}}},
    {"i heard what u said about me.", 2,
     {{"Prove it.", -6, 0, "oh it's ON"}, {"I'm sorry, that was messy", 8, 0, "...fine. we're good i guess"}, {"And? It's true.", -12, 0, "u are SO fake"}}},
    {"stay away from my friends.", 2,
     {{"Your friends? Cute.", -8, 0, "watch urself"}, {"Didn't know u felt that way", 4, 0, "...just saying"}, {"Make me.", -12, 0, "oh it's WAR"}}},
    {"everyone's talking about u btw. not in a good way", 2,
     {{"Let them talk ;)", 0, 0, "ur unbothered?? ugh fine"}, {"Who said what??", -2, 0, "wouldn't u like to know"}, {"Jealous much?", -6, 0, "as IF"}}},
    {"nice try at the party last night. embarrassing.", 2,
     {{"Says the uninvited one", -8, 0, "I WAS invited!!"}, {"I had fun tho :)", 2, 0, "...ok good for u i guess"}, {"Ur obsessed with me", -6, 0, "EW. no."}}},
    // ---- gossip about someone else (kind 3, {x} = their name) ----
    {"ugh {x} is SO annoying. whose side are u on??", 3,
     {{"Yours obviously", 6, 0, "THANK u", -8}, {"Stay out of it", 0, 0, "fair i guess", 0}, {"Theirs tbh", -8, 0, "wow. noted.", 6}}},
    {"omg did u hear {x} got dumped??", 3,
     {{"Spill. Now.", 4, 0, "ok so basically...", -3}, {"That's sad tho", 1, 0, "ur too nice lol", 2}, {"Not my business", -1, 0, "boring", 0}}},
    {"{x} said ur outfit was 'tragic' lmao", 3,
     {{"Tell them I said hi ;)", 2, 0, "ICONIC", -2}, {"They're just jealous", 3, 0, "FACTS", -5}, {"Wait really? :(", 2, 0, "sorry babe", -4}}},
    {"should i invite {x} to my party or nah", 3,
     {{"Nah, drama magnet", 3, 0, "lol ok", -6}, {"Invite them! Be nice", 2, 0, "ok fine", 4}, {"Only if I'm not invited", -2, 0, "dramatic much", -8}}},
    {"{x} is flirting with EVERYONE at the club rn", 3,
     {{"Not surprised lol", 3, 0, "RIGHT", -4}, {"Let them have fun!", 1, 0, "ur so chill", 2}, {"Send pics. For science.", 4, 0, "lmaooo omw", -2}}},
};
const char* SOCIAL_CAT_NAMES[SC_COUNT] = {"Friendly", "Romance", "Mean & Messy", "Family", "Special", "Funny"};

std::string fillName(const char* s, int about) {
    std::string out = s;
    size_t p = out.find("{x}");
    if (p != std::string::npos) out.replace(p, 3, about >= 0 && about < (int)G.sims.size() ? G.sims[about].name : std::string("someone"));
    return out;
}
const int NUM_PROMPTS = sizeof(PROMPTS) / sizeof(PROMPTS[0]);

ChatThread& threadFor(int sim) {
    for (ChatThread& t : G.threads) if (t.sim == sim) return t;
    ChatThread t; t.sim = (uint16_t)sim;
    G.threads.insert(G.threads.begin(), t);
    return G.threads.front();
}

static void bumpThread(int sim) {
    for (size_t i = 0; i < G.threads.size(); i++)
        if (G.threads[i].sim == sim && i > 0) { ChatThread t = G.threads[i]; G.threads.erase(G.threads.begin() + i); G.threads.insert(G.threads.begin(), t); return; }
}

static void trimThread(ChatThread& th) { while (th.msgs.size() > 24) th.msgs.erase(th.msgs.begin()); }

void npcTexts(int sim, int prompt) {
    if (prompt < 0) {
        const Rel* r = findRel(G.mc, sim);
        bool rom = canRomance(G.mc, sim) && r && (r->ro > 10 || r->matched);
        do prompt = irand(NUM_PROMPTS); while (PROMPTS[prompt].kind >= 2 || (PROMPTS[prompt].kind == 1 && !rom));
    }
    ChatThread& th = threadFor(sim);
    th.msgs.push_back({false, PROMPTS[prompt].line});
    th.prompt = (int16_t)prompt;
    th.about = -1;
    th.unread = true;
    trimThread(th);
    bumpThread(sim);
    toast("New text from " + G.sims[sim].name + "!");
}

void npcDrama(int sim, int about) {
    std::vector<int> pool;
    for (int i = 0; i < NUM_PROMPTS; i++) if (PROMPTS[i].kind == (about < 0 ? 2 : 3)) pool.push_back(i);
    if (pool.empty()) return;
    int prompt = pool[irand((int)pool.size())];
    ChatThread& th = threadFor(sim);
    th.msgs.push_back({false, fillName(PROMPTS[prompt].line, about)});
    th.prompt = (int16_t)prompt;
    th.about = (int16_t)about;
    th.unread = true;
    trimThread(th);
    bumpThread(sim);
    toast(about < 0 ? "Uh oh... " + G.sims[sim].name + " texted you. Drama!" : "New tea from " + G.sims[sim].name + "!");
}

static void dramaText(int sim, int prompt) {   // a specific rival prompt
    ChatThread& th = threadFor(sim);
    th.msgs.push_back({false, PROMPTS[prompt].line});
    th.prompt = (int16_t)prompt; th.about = -1; th.unread = true;
    trimThread(th); bumpThread(sim);
}

static int findPrompt(const char* start) {
    for (int i = 0; i < NUM_PROMPTS; i++) if (std::string(PROMPTS[i].line).rfind(start, 0) == 0) return i;
    return 0;
}

void dramaTick() {
    int hour = (int)(G.minute / 60);
    if (hour < 9 || irand(100) >= 9) return;
    std::vector<int> rivals, friends, known;
    for (const Rel& r : G.rels) {
        int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1;
        if (o < 0 || G.sims[o].household || !r.met || G.sims[o].stage < ST_PRETEEN) continue;
        known.push_back(o);
        if (r.fr <= -15) rivals.push_back(o);
        else if (r.fr >= 20) friends.push_back(o);
    }
    if (!rivals.empty() && irand(2) == 0) {
        int r = rivals[irand((int)rivals.size())];
        npcDrama(r, -1);
        if (irand(100) < 45) {   // a shady post too
            static const char* sub[] = {"some people need to stop trying so hard #notnamingnames", "fake friends are SO last season",
                                        "imagine copying someone's whole style lol", "not me being the main character again"};
            Post p; p.sim = (int16_t)r; p.text = sub[irand(4)]; p.likes = (uint32_t)(40 + irand(300));
            G.posts.insert(G.posts.begin(), p);
            if (G.posts.size() > 30) G.posts.pop_back();
            toast(G.sims[r].name + " posted a subtweet... about you?!");
        }
        return;
    }
    if (friends.size() >= 1 && known.size() >= 2) {
        int f = friends[irand((int)friends.size())], x = f;
        for (int tries = 0; tries < 12 && x == f; tries++) x = known[irand((int)known.size())];
        if (x != f) npcDrama(f, x);
    }
}

void answerPrompt(ChatThread& th, int choice) {
    if (th.prompt < 0) return;
    const TextChoice& c = PROMPTS[th.prompt].c[choice];
    int sim = th.sim;
    th.msgs.push_back({true, fillName(c.text, th.about)});
    th.msgs.push_back({false, fillName(c.reply, th.about)});
    bool drama = PROMPTS[th.prompt].kind >= 2;
    th.prompt = -1;
    trimThread(th);
    changeRel(G.mc, sim, c.fr, canRomance(G.mc, sim) ? c.ro : 0);
    if (th.about >= 0 && th.about != G.mc && c.aboutFr) changeRel(G.mc, th.about, c.aboutFr, 0);
    if (drama && c.fr <= -6) addMoodlet(MC(), "Messy Drama", EM_ANGRY, 1, 120);
    else if (drama && c.aboutFr < 0) addMoodlet(MC(), "Got the Tea", EM_PLAYFUL, 1, 120);
    th.about = -1;
    MC().needs[N_SOCIAL] = std::fmin(100.f, MC().needs[N_SOCIAL] + 6);
    if (c.ro > 4) addMoodlet(MC(), "Cute Texts", EM_FLIRTY, 1, 120);
}

bool askOnDate(int sim, int loc) {
    ChatThread& th = threadFor(sim);
    static const char* places[] = {"dinner", "the club", "the beach", "coffee"};
    int pi = loc == R_RESTAURANT ? 0 : loc == R_CLUB ? 1 : loc == R_BEACH ? 2 : 3;
    th.msgs.push_back({true, std::string("wanna go to ") + places[pi] + " with me tonight? :)"});
    const Rel* r = findRel(G.mc, sim);
    int ro = r ? r->ro : 0;
    float chance = 0.25f + ro / 100.f + skillLevel(MC(), SK_CHARISMA) * 0.03f + (r && r->matched ? 0.2f : 0) + fameStars() * 0.04f;
    if (!canRomance(G.mc, sim) || !MC().isAdult()) chance = 0;
    bool yes = frand() < chance;
    if (yes) {
        Event e;
        int hour = (int)(G.minute / 60);
        e.day = G.day; e.hour = 19;
        if (hour >= 21) e.day = G.day + 1;
        else if (hour >= 19) e.hour = (uint8_t)(hour + 1);
        e.type = EV_DATE; e.sim = (int16_t)sim; e.loc = (uint8_t)loc;
        e.text = "Date with " + G.sims[sim].name;
        G.events.push_back(e);
        th.msgs.push_back({false, "YES!! see u at " + clockStr(e.hour * 60.f) + " <3"});
        changeRel(G.mc, sim, 2, 4);
        addMoodlet(MC(), "Got a Date!", EM_FLIRTY, 2, 240);
        toast("Date set! Check your Calendar.");
    } else {
        th.msgs.push_back({false, ro < 10 ? "um... maybe we should get to know each other first?" : "can't tonight, sorry!!"});
        changeRel(G.mc, sim, 0, -2);
    }
    trimThread(th);
    return yes;
}

void sendText(int sim, int kind) {
    ChatThread& th = threadFor(sim);
    const Sim& me = MC();
    switch (kind) {
    case 0: {
        th.msgs.push_back({true, "heyyy :)"});
        static const char* replies[] = {"hiii!", "hey u!", "omg hi", "what's up?", "hey hey"};
        th.msgs.push_back({false, replies[irand(5)]});
        changeRel(G.mc, sim, 2, 0);
        break;
    }
    case 1: {
        th.msgs.push_back({true, "[sent a selfie]"});
        int st = skillLevel(me, SK_STYLE);
        if (st + irand(6) >= 4) { th.msgs.push_back({false, canRomance(G.mc, sim) ? "wow. just wow ;)" : "SLAYYY"}); changeRel(G.mc, sim, 3, canRomance(G.mc, sim) ? 4 : 0); }
        else { th.msgs.push_back({false, "lol cute"}); changeRel(G.mc, sim, 1, 0); }
        break;
    }
    case 2: {
        th.msgs.push_back({true, "come over to my place?"});
        const Rel* r = findRel(G.mc, sim);
        if (r && r->fr >= 15) {
            th.msgs.push_back({false, "omw!!"});
            if (roomIsHome(G.room)) {
                Actor a; a.kind = 0; a.id = (uint16_t)sim; a.x = 360; a.tx = 300; a.y = 200 + irand(30);
                a.say = "Hiii!"; a.sayT = 3;
                curRoom().actors.push_back(a);
            } else th.msgs.push_back({false, "wait ur not home lol"});
        } else th.msgs.push_back({false, "maybe another time"});
        break;
    }
    default: break;
    }
    MC().needs[N_SOCIAL] = std::fmin(100.f, MC().needs[N_SOCIAL] + 4);
    trimThread(th);
}

// ================================================================ dating app
std::vector<int> swipeCandidates() {
    std::vector<int> v;
    if (!MC().isAdult()) return v;
    for (size_t i = 0; i < G.sims.size(); i++) {
        const Sim& s = G.sims[i];
        if (s.household || (int)i == G.mc || s.stage < ST_YA || s.stage > ST_MIDDLE) continue;
        if (!canRomance(G.mc, (int)i)) continue;
        const Rel* r = findRel(G.mc, (int)i);
        if (r && (r->matched || r->status != RS_NONE)) continue;
        if (partnerOf((int)i) >= 0) continue;
        v.push_back((int)i);
    }
    return v;
}

bool swipeRight(int sim) {
    const Sim& me = MC(); const Sim& them = G.sims[sim];
    int shared = 0;
    for (uint8_t t : me.traits) if (t != TR_NONE && them.hasTrait(t)) shared++;
    float chance = 0.4f + shared * 0.12f + skillLevel(me, SK_CHARISMA) * 0.02f + skillLevel(me, SK_STYLE) * 0.02f + fameStars() * 0.05f;
    Rel& r = rel(G.mc, sim);
    r.met = true;
    if (frand() < chance) {
        r.matched = true;
        r.ro = (int8_t)std::max<int>(r.ro, 8);
        static const char* openers[] = {"hey :) ur profile made me smile", "ok ur outfits are iconic", "so... coffee sometime?", "hi!! u seem fun lol"};
        ChatThread& th = threadFor(sim);
        th.msgs.push_back({false, openers[irand(4)]});
        th.prompt = (int16_t)(6 + irand(3));
        th.unread = true;
        bumpThread(sim);
        addMoodlet(MC(), "It's a Match!", EM_FLIRTY, 2, 180);
        return true;
    }
    return false;
}

// ================================================================ social media
static const char* POST_TEXT[][3] = {
    {"felt cute, might delete later", "no filter needed ;)", "hoops + gloss = confidence"},
    {"OOTD: serving looks", "zebra print is NOT a phase", "new heels, who dis?"},
    {"made this from scratch!!", "chef's kiss", "dinner is served <3"},
    {"home tour!! rate my room", "just redecorated", "my happy place"},
    {"date night <3", "best night ever", "lucky me"},
    {"family is everything", "my little ones <3", "squad"},
};

void makePost(int kind) {
    Sim& me = MC();
    if (G.postCooldown > 0) { toast("Wait a bit before posting again!"); return; }
    float quality = 1.f;
    switch (kind) {
    case 0: quality = 0.8f + skillLevel(me, SK_STYLE) * 0.1f + (emotionOf(me) == EM_CONFIDENT ? 0.5f : 0); break;
    case 1: quality = 0.7f + skillLevel(me, SK_STYLE) * 0.15f + (me.outfit ? 0.3f : 0); break;
    case 2: quality = G.dishes > 0 ? 0.8f + skillLevel(me, SK_COOKING) * 0.12f : 0.3f; break;
    case 3: {
        int items = (int)curRoom().items.size();
        quality = roomIsHome(G.room) ? 0.5f + std::fmin(items, 14) * 0.08f : 0.3f;
        break;
    }
    case 4: quality = G.date.active ? 1.6f + G.date.score / 100.f : 0.2f; break;
    case 5: quality = householdCount() > 1 ? 1.2f : 0.3f; break;
    }
    if (me.hasTrait(TR_GLAMOROUS)) quality *= 1.5f;
    float base = 40 + G.followers * 0.06f;
    uint32_t likes = (uint32_t)(base * quality * (0.6f + frand() * 0.8f)) + irand(10);
    uint32_t newF = (uint32_t)(likes * (0.5f + frand() * 0.4f));
    int before = fameStars();
    newF = (uint32_t)(newF * perkFollowerMult());
    G.followers += newF;
    G.postsMade++;
    goalNotify(GE_POST);
    Post p; p.sim = (int16_t)G.mc; p.kind = (uint8_t)kind; p.likes = likes;
    p.text = POST_TEXT[kind][irand(3)];
    G.posts.insert(G.posts.begin(), p);
    if (G.posts.size() > 30) G.posts.pop_back();
    G.postCooldown = 60;
    me.needs[N_SOCIAL] = std::fmin(100.f, me.needs[N_SOCIAL] + 8);
    toast(std::to_string(likes) + " likes! +" + std::to_string(newF) + " followers");
    if (fameStars() > before) {
        toast(std::string("FAME UP! You're now: ") + fameTitle());
        addMoodlet(me, "Famous!", EM_CONFIDENT, 3, 720);
        fxBurst(200, 100, 40);
    }
}

void townieChatter() {
    static const char* lines[] = {"brunch with my girls <3", "dance floor was ON FIRE last night", "just adopted the cutest puppy!!",
                                  "new lip gloss, who dis", "studying... jk shopping", "beach day!!", "can't believe it's already {wd}",
                                  "someone take me to dinner pls", "mood: pink", "best latte of my life", "spa day = self care",
                                  "my closet is out of control lol"};
    std::vector<int> pool;
    for (size_t i = 0; i < G.sims.size(); i++) if (!G.sims[i].household && G.sims[i].stage >= ST_TEEN) pool.push_back((int)i);
    if (pool.empty()) return;
    Post p;
    p.sim = (int16_t)pool[irand((int)pool.size())];
    std::string t = lines[irand(12)];
    size_t pos = t.find("{wd}");
    if (pos != std::string::npos) t.replace(pos, 4, WEEKDAY_NAMES[weekday(G.day)]);
    p.text = t;
    p.likes = (uint32_t)(20 + irand(400));
    G.posts.insert(G.posts.begin(), p);
    if (G.posts.size() > 30) G.posts.pop_back();
}

// ================================================================ dates
void startDateIfDue() {
    if (G.date.active) return;
    int hour = (int)(G.minute / 60);
    for (Event& e : G.events) {
        if (e.done || e.type != EV_DATE || e.day != G.day || e.loc != G.room) continue;
        if (hour < e.hour - 1 || hour > e.hour + 3) continue;
        e.done = true;
        G.date.active = true; G.date.partner = e.sim; G.date.score = 10; G.date.loc = e.loc;
        Actor a; a.kind = 0; a.id = (uint16_t)e.sim; a.x = 240; a.tx = 240; a.y = 222;
        a.say = "You made it! <3"; a.sayT = 3;
        curRoom().actors.push_back(a);
        static const char* open[] = {"You made it! I've been looking forward to this all day.", "Wow... you look amazing tonight. Shall we?", "Hey you! I saved us the best spot."};
        dlgSay(e.sim, open[irand(3)]);
        toast("Date started! Talk, flirt, order dinner, dance!");
        addMoodlet(MC(), "On a Date", EM_FLIRTY, 2, 240);
        return;
    }
}

void endDate(bool silent) {
    if (!G.date.active) return;
    int p = G.date.partner;
    float sc = G.date.score;
    G.date.active = false;
    if (silent || p < 0) return;
    G.datesHad++;
    goalNotify(GE_DATE);
    const char* medal = sc >= 90 ? "GOLD" : sc >= 55 ? "Silver" : sc >= 25 ? "Bronze" : "Meh";
    int ro = sc >= 90 ? 15 : sc >= 55 ? 9 : sc >= 25 ? 4 : -3;
    changeRel(G.mc, p, ro / 2, ro);
    toast(std::string("Date over: ") + medal + " date with " + G.sims[p].name + "!");
    addMoodlet(MC(), sc >= 55 ? "Amazing Date" : "Okay Date", sc >= 55 ? EM_HAPPY : EM_FINE, sc >= 55 ? 2 : 1, 480);
    if (sc >= 55) { ChatThread& th = threadFor(p); th.msgs.push_back({false, "had SO much fun tonight <3"}); th.unread = true; }
    romanceAfterDate(p, sc);
}

// ================================================================ in-person socials
static Actor* roomActorOf(int sim) {
    for (Actor& a : curRoom().actors) if (a.kind == 0 && a.id == sim) return &a;
    return nullptr;
}

std::vector<SocialOpt> socialOptions(int t) {
    std::vector<SocialOpt> v;
    const Sim& me = MC();
    const Sim& s = G.sims[t];
    const Rel* r = findRel(G.mc, t);
    int fr = r ? r->fr : 0, ro = r ? r->ro : 0, st = r ? r->status : RS_NONE;
    if (s.isLittle()) {
        v.push_back({SO_FEED, s.stage == ST_INFANT ? "Feed Bottle" : "Feed Snack", SC_FAMILY});
        v.push_back({SO_PLAY, s.stage == ST_INFANT ? "Peekaboo" : "Play Blocks", SC_FAMILY});
        v.push_back({SO_CUDDLE, "Cuddle", SC_FAMILY});
        v.push_back({SO_BEDTIME, "Put to Bed", SC_FAMILY});
        kidsOptions(t, v);
        return v;
    }
    if (me.isLittle()) { v.push_back({SO_CHAT, "Babble", SC_FRIENDLY}); v.push_back({SO_HUG, "Hug", SC_FRIENDLY}); return v; }
    // friendly
    v.push_back({SO_CHAT, "Chat", SC_FRIENDLY});
    v.push_back({SO_COMPLIMENT, "Love Your Look!", SC_FRIENDLY});
    v.push_back({SO_SPILLTEA, "Spill the Tea", SC_FRIENDLY});
    v.push_back({SO_GOSSIP, "Gossip", SC_FRIENDLY});
    v.push_back({SO_SELFIE, "Selfie Together", SC_FRIENDLY});
    v.push_back({SO_RATEOUTFIT, "Rate My Outfit", SC_FRIENDLY});
    v.push_back({SO_HIGHFIVE, "High Five", SC_FRIENDLY});
    if (fr >= 25) v.push_back({SO_HUG, "Hug", SC_FRIENDLY});
    if (fr >= 20) v.push_back({SO_VENT, "Vent", SC_FRIENDLY});
    if (fr >= 30) v.push_back({SO_SECRET, "Share a Secret", SC_FRIENDLY});
    v.push_back({SO_DANCE, "Dance Together", SC_FRIENDLY});
    if (fr >= 15) v.push_back({SO_HANGOUT, "Hang Out", SC_FRIENDLY});
    v.push_back({SO_EARBUDS, "Share Earbuds", SC_FRIENDLY});
    if (fr >= 20) v.push_back({SO_DOHAIR, "Do Their Hair", SC_FRIENDLY});
    if (fr >= 40) v.push_back({SO_PINKY, "Pinky Promise", SC_FRIENDLY});
    if (fr >= 50) v.push_back({SO_TWINNING, "Plan Matching Outfits", SC_FRIENDLY});
    if (fr >= 30 && me.stage >= ST_CHILD && me.stage <= ST_TEEN && s.stage >= ST_CHILD && s.stage <= ST_TEEN && !s.household) v.push_back({SO_SLEEPOVER, "Invite to a Sleepover", SC_FRIENDLY});
    if (fr < 0) v.push_back({SO_APOLOGIZE, "Apologize", SC_FRIENDLY});
    // funny
    v.push_back({SO_JOKE, "Tell a Joke", SC_FUNNY});
    v.push_back({SO_IMPRESSION, "Do an Impression", SC_FUNNY});
    v.push_back({SO_PRANK, "Prank", SC_FUNNY});
    v.push_back({SO_BRAG, "Brag", SC_FUNNY});
    if (fr >= 30) v.push_back({SO_TICKLE, "Tickle Fight", SC_FUNNY});
    if (fr >= 20) v.push_back({SO_ROAST, "Friendly Roast", SC_FUNNY});
    v.push_back({SO_STEALFRY, "Steal a Fry", SC_FUNNY});
    v.push_back({SO_BIRTHDAYSONG, "Sing Off-Key", SC_FUNNY});
    // mean & messy
    if (me.stage >= ST_YA && s.stage >= ST_YA) v.push_back({SO_MURDER, "Murder...", SC_MEAN});   // adults only, never kids or teens
    v.push_back({SO_EYEROLL, "Eye Roll", SC_MEAN});
    v.push_back({SO_BACKHANDED, "Backhanded Compliment", SC_MEAN});
    v.push_back({SO_INSULT, "Throw Shade", SC_MEAN});
    v.push_back({SO_MOCKOUTFIT, "Roast Their Outfit", SC_MEAN});
    v.push_back({SO_COLDSHOULDER, "Cold Shoulder", SC_MEAN});
    v.push_back({SO_RUMOR, "Spread a Rumor", SC_MEAN});
    if (me.stage >= ST_TEEN) v.push_back({SO_SUBTWEET, "Subtweet Them", SC_MEAN});
    v.push_back({SO_COPYOUTFIT, "Copy Their Outfit", SC_MEAN});
    if (fr < 0) v.push_back({SO_CALLOUT, "Call Them Out", SC_MEAN});
    if (fr < 0 && (G.room == R_RESTAURANT || G.room == R_CLUB || G.room == R_CAFE || G.room == R_KITCHEN))
        v.push_back({SO_SPLASH, "Splash Drink", SC_MEAN});
    if (fr <= -30 && s.stage >= ST_PRETEEN && me.stage >= ST_PRETEEN) v.push_back({SO_CATFIGHT, "Catfight!", SC_MEAN});
    if (fr <= 15 && s.stage >= ST_TEEN && me.stage >= ST_TEEN) v.push_back({SO_FIGHT, "Pick a Fight", SC_MEAN});
    if (s.stage >= ST_CHILD && me.stage >= ST_CHILD) {
        v.push_back({SO_ZODIACTALK, "Talk Zodiac Signs", SC_FRIENDLY}); v.push_back({SO_FAVFOOD, "Ask Favorite Food", SC_FRIENDLY});
        v.push_back({SO_RPS, "Rock Paper Scissors", SC_FUNNY}); v.push_back({SO_SILLYFACE, "Make a Silly Face", SC_FUNNY});
        v.push_back({SO_PLAYLIST, "Share a Playlist", SC_FRIENDLY}); v.push_back({SO_TEACHDANCE, "Teach a Dance Move", SC_FRIENDLY});
        if (fr >= 30) v.push_back({SO_PIGGYBACK, "Piggyback Ride", SC_FUNNY});
        v.push_back({SO_MOCKZODIAC, "Mock Their Zodiac", SC_MEAN}); v.push_back({SO_SPOILER, "Spoil Their Show", SC_MEAN});
    }
    {   // roommates: anyone you live with who isn't your partner or your kid
        const bool roomie = s.household && me.household && s.stage >= ST_TEEN && me.stage >= ST_TEEN && st != RS_DATING && st != RS_ENGAGED && st != RS_MARRIED;
        if (roomie) {
            v.push_back({SO_DISHES, "Ask to Do the Dishes", SC_FAMILY}); v.push_back({SO_ROOMMOVIE, "Roommate Movie Night", SC_FAMILY});
            v.push_back({SO_SPLITBILLS, "Split the Bills", SC_FAMILY}); v.push_back({SO_BORROWCLOTHES, "Borrow Their Clothes", SC_FAMILY});
            v.push_back({SO_HOUSEMEETING, "Call a House Meeting", SC_FAMILY}); v.push_back({SO_LABELFOOD, "Label Your Food", SC_FAMILY});
            v.push_back({SO_NOISE, "Complain About Noise", SC_MEAN}); v.push_back({SO_LEFTOVERS, "Eat Their Leftovers", SC_MEAN});
        }
    }
    // family
    if (s.stage >= ST_CHILD && s.stage <= ST_TEEN && me.isAdult() && isFamily(G.mc, t)) v.push_back({SO_HOMEWORK, "Help Homework", SC_FAMILY});
    if (s.stage >= ST_CHILD && s.stage <= ST_TEEN && me.isAdult()) kidsOptions(t, v);
    famOptions(t, v);
    dlyLikeOptions(t, v);
    // special
    if (basketTotal() > 0 || contentHasGiftables()) v.push_back({SO_GIFT, "Give a Gift >", SC_SPECIAL});
    crimeSocialOptions(t, v);
    if (!s.household && (!r || !r->matched) && s.stage >= ST_TEEN && me.stage >= ST_TEEN) v.push_back({SO_ASKNUMBER, "Get Number", SC_SPECIAL});
    if (!s.household && (st >= RS_DATING || fr >= 60) && s.isAdult()) v.push_back({SO_MOVEIN, "Ask to Move In", SC_SPECIAL});
    // romance
    if (canRomance(G.mc, t)) {
        v.push_back({SO_VIBES, "Vibe Check", SC_ROMANCE});
        v.push_back({SO_FLIRT, "Flirt", SC_ROMANCE});
        if (st == RS_NONE && ro >= 30 && G.love.situ != t) v.push_back({SO_SITU, "Keep It Casual", SC_ROMANCE});
        if (st == RS_NONE && G.love.situ == t) v.push_back({SO_DTR, "What Are We?", SC_ROMANCE});
        v.push_back({SO_WINK, "Wink", SC_ROMANCE});
        if (ro >= 10) v.push_back({SO_SWEET, "Sweet Talk", SC_ROMANCE});
        if (ro >= 20) v.push_back({SO_SLOWDANCE, "Slow Dance", SC_ROMANCE});
        if (ro >= 25) v.push_back({SO_HOLDHANDS, "Hold Hands", SC_ROMANCE});
        if (ro >= 40) v.push_back({SO_KISS, "Kiss", SC_ROMANCE});
        if (ro >= 25) v.push_back({SO_SERENADE, "Serenade", SC_ROMANCE});
        if (ro >= 15) v.push_back({SO_LOVENOTE, "Slip a Love Note", SC_ROMANCE});
        if (ro >= 35) v.push_back({SO_SNUGGLE, "Snuggle", SC_ROMANCE});
        if (me.isAdult() && s.isAdult() && (st >= RS_DATING || ro >= 60) && (roomIsHome(G.room) || G.room == R_HOTEL)) v.push_back({SO_MESSAROUND, "Mess Around", SC_ROMANCE});
        if (ro >= 50 && st == RS_NONE) v.push_back({SO_GOSTEADY, "Ask to Go Steady", SC_ROMANCE});
        if (st == RS_DATING && ro >= 70) v.push_back({SO_PROPOSE, "Propose!", SC_ROMANCE});
        if (st == RS_ENGAGED) v.push_back({SO_MARRY, "Get Married!", SC_ROMANCE});
        if (st >= RS_DATING && st <= RS_MARRIED && s.household) v.push_back({SO_BABY, "Have a Baby", SC_ROMANCE});
        if (st >= RS_DATING && st <= RS_MARRIED) v.push_back({SO_BREAKUP, "Break Up", SC_MEAN});
    }
    return v;
}

static void dateBump(int t, float amt) { if (G.date.active && G.date.partner == t) G.date.score += amt; }

static void doSocialImpl(int id, int t);
void doSocial(int id, int t) {
    if (t < 0 || t >= (int)G.sims.size()) return;
    const Rel* r = findRel(G.mc, t);
    int fr0 = r ? r->fr : 0, ro0 = r ? r->ro : 0;
    doSocialImpl(id, t);
    const Rel* r2 = findRel(G.mc, t);
    {   // tweened reactions
        const int dfr = r2 ? r2->fr - fr0 : 0, dro = r2 ? r2->ro - ro0 : 0;
        if (id == SO_CHAT || id == SO_COMPLIMENT) tweenPlay(t, TW_WAVE, 1.2f);
        else if (id == SO_HUG) { tweenPlay(G.mc, TW_LEANR, 1.f); tweenPlay(t, TW_LEANL, 1.f); }
        else if (id == SO_DANCE) { tweenPlay(G.mc, TW_SPIN, 1.4f); tweenPlay(t, TW_SPIN, 1.4f); }
        else if (dro > 0) { tweenPlay(G.mc, TW_SWAY, 1.2f); tweenPlay(t, TW_SWAY, 1.2f); }
        else if (dfr > 3) tweenPlay(t, TW_HOP, 0.6f);
        else if (dfr < 0) tweenPlay(t, TW_SHAKE, 0.7f);
    }
    if (r2) dramaAfterSocial(id, t, r2->fr - fr0, r2->ro - ro0);
    romanceAfterSocial(id, t, true);
    lifeplusAfterSocial(id, t);
    lifeStoryAfterSocial(t);
}
static void doSocialImpl(int id, int t) {
    if (id == SO_MURDER) { murderAttempt(t); return; }
    if (kidsDoSocial(id, t)) return;
    if (famDoSocial(id, t)) return;
    if (dlyLikeSocial(id, t)) return;
    Sim& me = MC();
    Sim& s = G.sims[t];
    Rel& r0 = rel(G.mc, t);
    r0.met = true;
    int fr = r0.fr, ro = r0.ro;
    float ch = 0.72f + skillLevel(me, SK_CHARISMA) * 0.025f + moodOf(me) / 400.f + repBonus() + perkSocialBonus() + dramaSocialBonus(t);
    {   // traits: kind / mean people are good at their own kind of social; snobs click with the glam crowd
        const bool meanAct = id == SO_INSULT || id == SO_EYEROLL || id == SO_BACKHANDED || id == SO_MOCKOUTFIT || id == SO_RUMOR || id == SO_COLDSHOULDER || id == SO_PRANK || id == SO_SPLASH || id == SO_CATFIGHT || id == SO_CALLOUT || id == SO_GOSSIP;
        if (me.hasTrait(TR_KIND)) ch += meanAct ? -0.15f : 0.08f;
        if (me.hasTrait(TR_MEAN)) ch += meanAct ? 0.15f : -0.05f;
        if (me.hasTrait(TR_SNOB)) ch += (s.hasTrait(TR_GLAMOROUS) || s.hasTrait(TR_FASHIONISTA)) ? 0.12f : -0.06f;
        if (s.hasTrait(TR_KIND)) ch += 0.05f;
        if (s.hasTrait(TR_MEAN) && !meanAct) ch -= 0.05f;
        if (me.hasTrait(TR_CLUMSY) && irand(12) == 0) { say(G.mc, "*trips* ...I meant to do that."); ch -= 0.2f; }
    }
    bool ok;
    auto fin = [&](const char* mine, const char* theirs) { say(G.mc, varyLine(mine, G.mc)); if (theirs) say(t, varyLine(theirs, t)); };
    me.needs[N_SOCIAL] = std::fmin(100.f, me.needs[N_SOCIAL] + 7);
    if (s.household) s.needs[N_SOCIAL] = std::fmin(100.f, s.needs[N_SOCIAL] + 7);
    skillGain(me, SK_CHARISMA, 0.6f);

    // jealousy: your partner (or your date) sees you getting flirty with someone else
    const bool flirty = id == SO_FLIRT || id == SO_WINK || id == SO_SWEET || id == SO_HOLDHANDS || id == SO_KISS || id == SO_SLOWDANCE || id == SO_SERENADE || id == SO_SNUGGLE || id == SO_MESSAROUND || id == SO_LOVENOTE;
    if (flirty) {
        int p = partnerOf(G.mc);
        int watcher = (p >= 0 && p != t && roomActorOf(p)) ? p : (G.date.active && G.date.partner != t && G.date.partner >= 0 && roomActorOf(G.date.partner)) ? G.date.partner : -1;
        if (watcher >= 0) {
            say(watcher, irand(2) ? "EXCUSE ME?!" : "Are you SERIOUS right now?!");
            dlgSay(watcher, "Wow. Flirting with " + s.name + " right in front of me? Unbelievable.");
            changeRel(G.mc, watcher, G.sims[watcher].hasTrait(TR_JEALOUS) ? -32 : -20, G.sims[watcher].hasTrait(TR_JEALOUS) ? -45 : -30);
            addMoodlet(me, "Caught!", EM_TENSE, 2, 240);
            toast("Caught! " + G.sims[watcher].name + " saw you flirting with " + s.name + "!");
            if (fameStars() >= 1 || G.followers >= 300) scandalBreak(SC_TWOTIMING); else npcScandal(G.mc, SC_TWOTIMING);
            if (G.date.active && G.date.partner == watcher) G.date.score -= 40;
            const Rel* wr = findRel(G.mc, watcher);
            if (wr && wr->status >= RS_DATING && wr->status <= RS_MARRIED && wr->ro < 15) {
                rel(G.mc, watcher).status = RS_NONE;
                toast(G.sims[watcher].name + " broke up with you!!");
                addMoodlet(me, "Dumped", EM_SAD, 3, 1440);
            }
        }
    }

    if (id >= SO_PICKPOCKET && id <= SO_KEEPSECRET) { crimeDoSocial(id, t); return; }
    socialRipple(id, t);
    goalNotify(GE_SOCIAL, id);
    switch (id) {
    case SO_CHAT: fin("So how's life?", "Pretty good!"); changeRel(G.mc, t, 3, 0); dateBump(t, 4); break;
    case SO_COMPLIMENT:
        ok = frand() < ch + 0.1f;
        if (ok) { fin("Love your look!!", "Aww stop! <3"); changeRel(G.mc, t, 5, canRomance(G.mc, t) ? 1 : 0); dateBump(t, 6); }
        else { fin("Love your... shoes?", "...thanks?"); changeRel(G.mc, t, 1, 0); }
        break;
    case SO_JOKE:
        ok = me.hasTrait(TR_GOOFBALL) || frand() < ch - 0.1f;
        if (ok) { fin("Why did the purse go to therapy?", "HAHAHA"); changeRel(G.mc, t, 6, 0); addMoodlet(me, "Made 'Em Laugh", EM_PLAYFUL, 1, 90); dateBump(t, 6); }
        else { fin("...it had too much baggage", "Ugh. Groan."); changeRel(G.mc, t, -2, 0); }
        break;
    case SO_GOSSIP: fin("Did you hear about...", "No WAY!"); changeRel(G.mc, t, fr > 20 ? 6 : 1, 0); break;
    case SO_HUG: fin("Come here!", "Awww"); changeRel(G.mc, t, 5, canRomance(G.mc, t) ? 2 : 0); addMoodlet(me, "Hugged", EM_HAPPY, 1, 90); dateBump(t, 6); break;
    case SO_DANCE:
        fin("Let's dance!", "Woo!"); changeRel(G.mc, t, 5, canRomance(G.mc, t) ? 3 : 0);
        me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 10); skillGain(me, SK_FITNESS, 1.f); dateBump(t, 10);
        break;
    case SO_INSULT:
        fin("Nice outfit... from 1995?", "EXCUSE me?!");
        changeRel(G.mc, t, me.hasTrait(TR_HOTHEAD) ? -6 : -12, -8);
        if (s.hasTrait(TR_HOTHEAD)) addMoodlet(me, "Got Yelled At", EM_ANGRY, 1, 120);
        dateBump(t, -25);
        break;
    case SO_FLIRT:
        ok = frand() < ch + (me.hasTrait(TR_ROMANTIC) ? 0.15f : 0) - (fr < 0 ? 0.3f : 0);
        if (ok) { fin("Is it hot in here, or is it you?", "Hehe ;)"); changeRel(G.mc, t, 2, 6); addMoodlet(me, "Flirty Vibes", EM_FLIRTY, 1, 120); dateBump(t, 10); }
        else { fin("So... come here often?", "Ew, no."); changeRel(G.mc, t, -1, -4); addMoodlet(me, "Embarrassed", EM_TENSE, 1, 60); }
        break;
    case SO_FIGHT: fin("You wanna go?!", "Oh, it is SO on."); break;
    case SO_VIBES: fin("So... what's your sign?", "Haha, why?"); changeRel(G.mc, t, 1, 1); break;
    case SO_SITU: fin("No labels... just vibes?", "I'm down ;)"); changeRel(G.mc, t, 2, 5); dateBump(t, 6); break;
    case SO_DTR: fin("Can we talk about us?", "...okay."); break;
    case SO_SWEET: fin("You make me smile, you know?", "You're sweet <3"); changeRel(G.mc, t, 3, 7); dateBump(t, 10); break;
    case SO_HOLDHANDS: fin("*holds hands*", "<3"); changeRel(G.mc, t, 2, 6); dateBump(t, 10); break;
    case SO_KISS:
        ok = frand() < ch + ro / 200.f;
        if (ok) { fin("*kiss*", "*blushes*"); changeRel(G.mc, t, 3, 10); addMemoryOnce(MEM_LOVE, G.mc, t, "First kiss with " + s.name + "!"); addMoodlet(me, "First Kiss Feels", EM_FLIRTY, 2, 240); milestone(G.mc, MI_KISS, t); milestone(t, MI_KISS, G.mc); fxBurst(200, 90, 12); dateBump(t, 18); }
        else { fin("*leans in*", "Whoa, slow down!"); changeRel(G.mc, t, -2, -6); }
        break;
    case SO_GOSTEADY:
        if (frand() < 0.4f + ro / 120.f) { rel(G.mc, t).status = RS_DATING; fin("Wanna be official?", "YES!!"); toast("You're dating " + s.name + "!"); cutscenePlay(CS_DATE); addMemoryOnce(MEM_LOVE, G.mc, t, "Made it official with " + s.name + "!"); addMoodlet(me, "In a Relationship", EM_HAPPY, 2, 1440); fxBurst(200, 90, 20); }
        else { fin("Wanna be official?", "I'm not ready..."); changeRel(G.mc, t, 0, -8); addMoodlet(me, "Rejected", EM_SAD, 2, 240); }
        break;
    case SO_PROPOSE:
        if (frand() < 0.5f + ro / 150.f) { rel(G.mc, t).status = RS_ENGAGED; fin("Will you marry me?", "YES YES YES!"); toast("Engaged to " + s.name + "!!"); addMemoryOnce(MEM_LOVE, G.mc, t, s.name + " said YES! We're engaged!"); fxBurst(200, 90, 40); addMoodlet(me, "Engaged!", EM_HAPPY, 3, 1440); }
        else { fin("Will you marry me?", "It's too soon..."); changeRel(G.mc, t, -5, -12); addMoodlet(me, "Proposal Rejected", EM_SAD, 3, 600); }
        break;
    case SO_MARRY:
        rel(G.mc, t).status = RS_MARRIED;
        fin("I do!", "I do! <3");
        toast("Just married!! " + s.name + " joins your family!");
        cutscenePlay(CS_WEDDING);
        addMemoryOnce(MEM_LOVE, G.mc, t, "Married " + s.name + "! Best day ever.");
        fxBurst(200, 90, 60);
        addMoodlet(me, "Just Married", EM_HAPPY, 3, 2880);
        moveIn(t);
        break;
    case SO_BABY: {
        Sim* mom = !me.look.guy ? &me : !s.look.guy ? &s : nullptr;
        if (me.stage > ST_MIDDLE || s.stage > ST_MIDDLE) { toast("Try adopting through the Family app."); break; }
        if (!mom) { toast("Try adopting through the Family app!"); break; }
        if (mom->pregnant) { toast("A baby is already on the way!"); break; }
        if (lxBirthControl((int)(mom - &G.sims[0]))) { toast("Birth control is on - turn it off in Phone > Body to try for a baby."); break; }
        if (frand() < 0.7f) { mom->pregnant = true; mom->pregDays = 3; toast("Baby on the way! Arriving in 3 days."); addMoodlet(me, "Expecting!", EM_HAPPY, 2, 1440); fxBurst(200, 90, 20); }
        else toast("No baby this time... try again later.");
        break;
    }
    case SO_BREAKUP:
        rel(G.mc, t).status = RS_NONE;
        changeRel(G.mc, t, -25, -40);
        fin("We need to talk...", "...seriously?!");
        addMoodlet(me, "Breakup", EM_SAD, 3, 1440);
        break;
    case SO_MOVEIN:
        if (fr >= 40 || r0.status >= RS_DATING) { fin("Move in with me?", "I'd love to!"); moveIn(t); }
        else { fin("Move in with me?", "We barely know each other!"); }
        break;
    case SO_ASKNUMBER:
        if (frand() < ch) { rel(G.mc, t).matched = true; fin("Can I get your number?", "Sure! Text me!"); threadFor(t).msgs.push_back({false, "hey it's " + s.name + "!"}); }
        else fin("Can I get your number?", "Haha, no.");
        break;
    // ---------------- mean & messy ----------------
    case SO_EYEROLL: fin("*rolls eyes*", "Um, rude!"); changeRel(G.mc, t, -4, -2); dateBump(t, -10); break;
    case SO_BACKHANDED:
        if (irand(100) < 30) { fin("You look... almost cute today!", "Aww, thanks!!"); changeRel(G.mc, t, 1, 0); addMoodlet(me, "Got Away With It", EM_PLAYFUL, 1, 90); }
        else { fin("You look... almost cute today!", "...ALMOST?!"); changeRel(G.mc, t, -6, -3); }
        dateBump(t, -15);
        break;
    case SO_MOCKOUTFIT:
        fin("Did your closet explode on you?", "Wow. Okay.");
        changeRel(G.mc, t, s.hasTrait(TR_FASHIONISTA) ? -12 : -8, -4);
        if (me.hasTrait(TR_FASHIONISTA)) addMoodlet(me, "Fashion Police", EM_CONFIDENT, 1, 90);
        dateBump(t, -20);
        break;
    case SO_COLDSHOULDER: fin("*ignores them*", "Hello?? I'm RIGHT here."); changeRel(G.mc, t, -5, -3); break;
    case SO_RUMOR: {
        std::vector<int> pool;
        for (const Rel& r : G.rels) { int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1; if (o >= 0 && o != t && r.met && G.sims[o].stage >= ST_PRETEEN) pool.push_back(o); }
        if (pool.empty()) { fin("I heard something juicy...", "About who?"); changeRel(G.mc, t, 1, 0); break; }
        int victim = pool[irand((int)pool.size())];
        static const char* rumors[] = {" wears fake designer!", " got kicked out of the club!", " still sleeps with a nightlight!", " copies everyone's outfits!", " has a secret crush on the barista!"};
        say(G.mc, "Did you know " + G.sims[victim].name + rumors[irand(5)]);
        say(t, "NO WAY!");
        changeRel(t, victim, -15, 0);
        changeRel(G.mc, t, 3, 0);
        addMoodlet(me, "Messy Mood", EM_PLAYFUL, 1, 120);
        if (irand(100) < 35) {
            changeRel(G.mc, victim, -15, -5);
            dramaText(victim, findPrompt("i heard what u said"));
            toast(G.sims[victim].name + " found out about the rumor... uh oh.");
        }
        break;
    }
    case SO_PRANK:
        if (irand(100) < 45 + skillLevel(me, SK_CHARISMA) * 3 + (me.hasTrait(TR_GOOFBALL) ? 20 : 0)) { fin("Gotcha!!", "HAHA you got me!"); changeRel(G.mc, t, 4, 0); me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 12); }
        else { fin("Gotcha!!", "Not. Funny."); changeRel(G.mc, t, -7, -2); }
        break;
    case SO_SPLASH: {
        fin("*splashes drink*", "AAAAH!! My OUTFIT!");
        changeRel(G.mc, t, -20, -15);
        addMoodlet(me, s.hasTrait(TR_HOTHEAD) ? "Started a War" : "Messy Moment", me.hasTrait(TR_HOTHEAD) ? EM_CONFIDENT : EM_PLAYFUL, 2, 240);
        uint32_t f = 60 + G.followers / 40;
        G.followers += f;
        toast("Someone filmed it... it's going viral! +" + std::to_string(f) + " followers");
        dramaText(t, findPrompt("everyone's talking"));
        fxBurst(200, 120, 14);
        dateBump(t, -40);
        break;
    }
    case SO_CATFIGHT: {
        say(G.mc, "Come at me!"); say(t, "Oh it's ON!");
        dlgSay(t, "You've been asking for this all week. Let's GO!");
        brawlStart(G.mc, t, !roomIsHome(G.room) && G.room != R_JAIL);
        int myPow = skillLevel(me, SK_FITNESS) * 2 + irand(12) + (me.hasTrait(TR_HOTHEAD) ? 3 : 0);
        int theirPow = skillLevel(s, SK_FITNESS) * 2 + irand(12);
        changeRel(G.mc, t, -15, -10);
        if (myPow >= theirPow) { toast("You won the catfight! Hair: still perfect."); addMoodlet(me, "Won the Fight", EM_CONFIDENT, 2, 300); }
        else { toast("You lost the catfight... and a hair clip."); addMoodlet(me, "Lost the Fight", EM_ANGRY, 2, 300); me.needs[N_HYGIENE] = std::fmax(0.f, me.needs[N_HYGIENE] - 20); }
        uint32_t f = 80 + G.followers / 30;
        G.followers += f;
        toast("The fight video hit InstaGlam: +" + std::to_string(f) + " followers");
        break;
    }
    case SO_CALLOUT:
        if (irand(100) < 40 + skillLevel(me, SK_CHARISMA) * 3) { fin("I know you've been talking about me.", "...okay, fine. I'm sorry."); changeRel(G.mc, t, 8, 0); }
        else { fin("I know you've been talking about me.", "Prove it, babe."); changeRel(G.mc, t, -6, 0); addMoodlet(me, "So Annoyed", EM_ANGRY, 1, 120); }
        break;
    case SO_BRAG:
        if (fameStars() >= 2 || G.money > 5000) { fin("I'm kind of a big deal.", "OMG you ARE!"); changeRel(G.mc, t, 3, 1); }
        else { fin("I'm kind of a big deal.", "Okay...?"); changeRel(G.mc, t, -2, 0); }
        break;
    case SO_APOLOGIZE:
        if (irand(100) < 50 + skillLevel(me, SK_CHARISMA) * 4) { fin("I'm sorry. Truly.", "...Apology accepted."); changeRel(G.mc, t, 14, 0); addMoodlet(me, "Made Up", EM_HAPPY, 1, 180); }
        else { fin("I'm sorry. Truly.", "Too little, too late."); changeRel(G.mc, t, 2, 0); }
        break;
    // ---------------- friendly & funny extras ----------------
    case SO_SELFIE: {
        fin("Selfie time!", "*poses*"); changeRel(G.mc, t, 4, canRomance(G.mc, t) ? 1 : 0);
        uint32_t f = 3 + G.followers / 200 + irand(8); G.followers += f;
        dateBump(t, 8);
        break;
    }
    case SO_ZODIACTALK: {
        int c = zodiacCompat(me.zodiac, s.zodiac);
        std::string a = std::string("I'm a ") + ZODIAC_NAMES[me.zodiac % 12] + "!", b = std::string("No way, I'm a ") + ZODIAC_NAMES[s.zodiac % 12] + (c > 0 ? ". We match!" : c < 0 ? "... uh oh." : ".");
        say(G.mc, a); say(t, b); changeRel(G.mc, t, c > 0 ? 5 : c < 0 ? 0 : 2, c > 0 ? 2 : 0); break;
    }
    case SO_FAVFOOD: { std::string b = std::string("Honestly? ") + FOOD_NAMES[s.favFood % F_COUNT] + ". Every time."; say(G.mc, "What's your favorite food?"); say(t, b); changeRel(G.mc, t, 3, 0); toast(s.name + "'s favorite food: " + FOOD_NAMES[s.favFood % F_COUNT] + ". Cook it for them!"); break; }
    case SO_RPS: { bool win = irand(3) == 0, tie = !win && irand(2); fin("Rock, paper, scissors... SHOOT!", win ? "No fair! Best two out of three!" : tie ? "Tie! Again!" : "HA! Paper covers rock!"); changeRel(G.mc, t, 3, 0); me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 8); break; }
    case SO_PIGGYBACK: fin("Hop on!", "Weeee! Giddy up!"); changeRel(G.mc, t, 5, 0); me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 10); skillGain(me, SK_FITNESS, 1); break;
    case SO_TEACHDANCE: fin("Okay, step-touch, then spin!", "Like this?! I'm a natural!"); changeRel(G.mc, t, 4, 0); skillGain(me, SK_FITNESS, 1); skillGain(s, SK_FITNESS, 2); break;
    case SO_PLAYLIST: fin("You HAVE to hear this song.", "Okay this slaps. Send it to me!"); changeRel(G.mc, t, 3, s.hasTrait(TR_MUSIC) ? 2 : 0); if (s.hasTrait(TR_MUSIC)) changeRel(G.mc, t, 3, 0); break;
    case SO_SILLYFACE: fin("*crosses eyes, puffs cheeks*", "HAHAHA stop it!"); changeRel(G.mc, t, 3, 0); break;
    case SO_MOCKZODIAC: { std::string a = std::string("Ugh, a ") + ZODIAC_NAMES[s.zodiac % 12] + "? That explains SO much."; say(G.mc, a); say(t, "Excuse me?!"); changeRel(G.mc, t, -8, 0); break; }
    case SO_SPOILER: fin("Oh, the main character dies at the end.", "WHY WOULD YOU TELL ME THAT?!"); changeRel(G.mc, t, -10, 0); break;
    case SO_DISHES: {
        bool ok = !s.hasTrait(TR_LAZY) && (irand(3) || s.hasTrait(TR_NEAT));
        fin("Can you do the dishes tonight?", ok ? "Yeah, I got it." : "Ugh. Why is it always me?!");
        changeRel(G.mc, t, ok ? 1 : -4, 0); if (ok) me.needs[N_HYGIENE] = std::fmin(100.f, me.needs[N_HYGIENE] + 5); break;
    }
    case SO_NOISE: fin("Can you keep it DOWN? Some of us sleep!", s.hasTrait(TR_HOTHEAD) ? "It's MY house too!" : "Sorry, sorry..."); changeRel(G.mc, t, -6, 0); break;
    case SO_LABELFOOD: fin("I labeled my food. Don't touch the yogurt.", "Wow. Okay. Noted."); changeRel(G.mc, t, -2, 0); break;
    case SO_ROOMMOVIE: fin("Movie night? I'll make popcorn.", "YES. Rom-com or horror?"); changeRel(G.mc, t, 6, 0); me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 15); s.needs[N_FUN] = std::fmin(100.f, s.needs[N_FUN] + 15); addMoodlet(me, "Roomie Movie Night", EM_HAPPY, 1, 240); break;
    case SO_SPLITBILLS: {
        int owe = 0; if (!G.eco.bill.paid) { for (int i = 0; i < BI_COUNT; i++) owe += G.eco.bill.item[i]; owe /= 2; }
        bool pays = owe > 0 && (s.hasTrait(TR_GENEROUS) || fr >= 20 || irand(2));
        if (owe <= 0) { fin("We should split the bills.", "Totally. Nothing's due right now though!"); break; }
        if (pays) { G.money += owe; char b[64]; snprintf(b, sizeof b, "%s chipped in $%d for the bills!", s.name.c_str(), owe); toast(b); fin("Wanna split the bills?", "Fair's fair. Here's my half."); changeRel(G.mc, t, 2, 0); }
        else { fin("Wanna split the bills?", "I'm broke till Friday, sorry..."); changeRel(G.mc, t, -3, 0); }
        break;
    }
    case SO_BORROWCLOTHES: {
        bool ok = fr >= 15 && !s.hasTrait(TR_SNOB);
        fin("Can I borrow that fit?", ok ? "Sure! Don't spill on it." : "Absolutely not.");
        if (ok) { me.outfit = s.outfit; changeRel(G.mc, t, 2, 0); toast("You borrowed " + s.name + "'s outfit!"); } else changeRel(G.mc, t, -2, 0);
        break;
    }
    case SO_HOUSEMEETING: {
        int n = 0; for (Sim& x : G.sims) if (x.household && x.stage >= ST_TEEN && &x != &me) { n++; }
        fin("House meeting! Living room, five minutes.", "Oh no, what did someone do now?");
        for (size_t i = 0; i < G.sims.size(); i++) if (G.sims[i].household && (int)i != G.mc && G.sims[i].stage >= ST_TEEN) changeRel(G.mc, (int)i, irand(3) ? 2 : -2, 0);
        char b[80]; snprintf(b, sizeof b, "House meeting with %d roommates: chores split, drama aired.", n); toast(b); break;
    }
    case SO_LEFTOVERS: fin("*eats their leftovers*", "Did you just eat my pad thai?!"); changeRel(G.mc, t, -10, 0); me.needs[N_HUNGER] = std::fmin(100.f, me.needs[N_HUNGER] + 25); addSentiment(t, SN_FURIOUS, 3); break;
    case SO_HIGHFIVE: fin("Up top!", "*high five*"); changeRel(G.mc, t, 3, 0); break;
    case SO_SECRET: fin("Okay, don't tell ANYONE...", "My lips are sealed!"); changeRel(G.mc, t, 8, 1); addMoodlet(me, "Trusted Bestie", EM_HAPPY, 1, 180); break;
    case SO_VENT: fin("Ugh, I just need to vent...", "I'm here for you."); changeRel(G.mc, t, 5, 0); me.needs[N_SOCIAL] = std::fmin(100.f, me.needs[N_SOCIAL] + 15); addMoodlet(me, "Got It Off My Chest", EM_HAPPY, 1, 180); break;
    case SO_RATEOUTFIT: {
        int sc = std::min(10, 4 + skillLevel(me, SK_STYLE) / 2 + irand(3) + (me.outfit ? 1 : 0));
        say(G.mc, "Rate my outfit!");
        say(t, sc >= 9 ? "11/10, ICON!" : sc >= 7 ? std::to_string(sc) + "/10, cute!" : std::to_string(sc) + "/10... it's giving 'trying'");
        changeRel(G.mc, t, 2, 0);
        if (sc >= 9) addMoodlet(me, "Looking Iconic", EM_CONFIDENT, 1, 180);
        break;
    }
    case SO_IMPRESSION:
        if (me.hasTrait(TR_GOOFBALL) || irand(100) < 55) { fin("*does a spot-on impression*", "STOP I'm crying!"); changeRel(G.mc, t, 5, 0); }
        else { fin("*does an impression*", "...who was that supposed to be?"); changeRel(G.mc, t, -1, 0); }
        break;
    case SO_SPILLTEA: {
        std::vector<int> pool;
        for (size_t i = 0; i < G.sims.size(); i++) if ((int)i != t && (int)i != G.mc && !G.sims[i].household && G.sims[i].stage >= ST_TEEN) pool.push_back((int)i);
        int x = pool.empty() ? -1 : pool[irand((int)pool.size())];
        static const char* tea[] = {" is SO fake.", " went on THREE dates this week!", " got caught regifting!", " has a crush on someone here..."};
        say(G.mc, "Spill the tea!");
        say(t, (x >= 0 ? G.sims[x].name : std::string("Someone")) + tea[irand(4)]);
        changeRel(G.mc, t, 4, 0);
        if (x >= 0) changeRel(G.mc, x, -2, 0);
        addMoodlet(me, "Got the Tea", EM_PLAYFUL, 1, 120);
        break;
    }
    case SO_WINK: fin("*winks*", "Oh? ;)"); changeRel(G.mc, t, 1, 3); dateBump(t, 5); break;
    // ---------------- more ways to hang out ----------------
    case SO_HANGOUT: fin("Wanna just hang out?", "Always!"); changeRel(G.mc, t, 7, 0); me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 12); addMoodlet(me, "Good Company", EM_HAPPY, 1, 180); dateBump(t, 8); break;
    case SO_PINKY: fin("Pinky promise?", "Pinky promise. Forever."); changeRel(G.mc, t, 9, 0); addMoodlet(me, "Ride or Die", EM_HAPPY, 2, 360); break;
    case SO_DOHAIR: { ok = frand() < ch; skillGain(me, SK_STYLE, 2.f);
        if (ok) { fin("Sit still, I'm braiding.", "Omg I look SO good!"); changeRel(G.mc, t, 7, 0); if (!s.household && irand(3) == 0) s.look.hairStyle = (uint8_t)((s.look.hairStyle + 1) % NUM_HAIRSTYLES); }
        else { fin("Hold on, it's... fine.", "Why is there a knot?!"); changeRel(G.mc, t, -2, 0); }
        break; }
    case SO_EARBUDS: fin("You HAVE to hear this song.", "Wait, this slaps!"); changeRel(G.mc, t, 4, canRomance(G.mc, t) ? 2 : 0); me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 8); dateBump(t, 6); break;
    case SO_TWINNING: fin("Matching outfits tomorrow?", "TWINNING! Yes!"); changeRel(G.mc, t, 8, 0); addMoodlet(me, "Twinning", EM_PLAYFUL, 1, 300); G.followers += 20; break;
    case SO_TICKLE: fin("Tickle fight!!", "NO-- hahaha STOP!"); changeRel(G.mc, t, 6, canRomance(G.mc, t) ? 3 : 0); me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 15); dateBump(t, 10); break;
    case SO_ROAST: ok = frand() < ch; if (ok) { fin("Nice shoes. Did your grandma pick them?", "LMAO okay that was good."); changeRel(G.mc, t, 5, 0); } else { fin("Nice shoes. Did your grandma pick them?", "...wow. Okay."); changeRel(G.mc, t, -6, 0); } break;
    case SO_STEALFRY: ok = frand() < 0.6f; if (ok) { fin("*steals a fry*", "Hey! ...fine, you can have ONE."); changeRel(G.mc, t, 3, 1); } else { fin("*steals a fry*", "Touch my fries again. I dare you."); changeRel(G.mc, t, -3, 0); } me.needs[N_HUNGER] = std::fmin(100.f, me.needs[N_HUNGER] + 2); break;
    case SO_BIRTHDAYSONG: fin("*sings wildly off-key*", "My EARS! Hahaha"); changeRel(G.mc, t, 4, 0); me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 10); break;
    case SO_SUBTWEET: fin("*types* some people are SO fake lol", nullptr); changeRel(G.mc, t, -8, -3); G.followers += 15; addMoodlet(me, "Petty", EM_PLAYFUL, 1, 120); if (irand(3) == 0) { say(t, "Was that about ME?!"); changeRel(G.mc, t, -8, 0); } break;
    case SO_COPYOUTFIT: fin("Oh, cute fit. Mine now.", "Are you... wearing MY look?!"); changeRel(G.mc, t, -6, 0); me.look.top = s.look.top; me.look.topStyle = s.look.topStyle; me.look.bottom = s.look.bottom; me.look.bt = s.look.bt; me.outfit = 0; break;
    case SO_SERENADE: { ok = frand() < ch + skillLevel(me, SK_CREATIVITY) * 0.03f;
        if (ok) { fin("*sings a love song*", "Stop... I'm literally melting."); changeRel(G.mc, t, 3, 9); addMoodlet(me, "Lovestruck", EM_FLIRTY, 2, 240); dateBump(t, 16); }
        else { fin("*sings a love song... badly*", "Haha... that was... brave."); changeRel(G.mc, t, 1, -2); }
        break; }
    case SO_LOVENOTE: fin("*slips a folded note*", "\"Do you like me? Y/N\"... Y!!"); changeRel(G.mc, t, 2, 7); dateBump(t, 8); break;
    case SO_SNUGGLE: fin("Come here, you.", "*snuggles up*"); changeRel(G.mc, t, 4, 8); addMoodlet(me, "Cozy", EM_FLIRTY, 2, 240); me.needs[N_SOCIAL] = std::fmin(100.f, me.needs[N_SOCIAL] + 12); dateBump(t, 12); break;
    case SO_MESSAROUND: {
        if (!(r0.status >= RS_DATING && r0.status <= RS_MARRIED) && frand() > 0.5f + ro / 200.f) { fin("Wanna... mess around?", "Whoa! Slow down there."); changeRel(G.mc, t, -4, -8); addMoodlet(me, "Rejected", EM_SAD, 1, 180); break; }
        fin("Wanna... mess around?", "*giggles* Lock the door.");
        if (!spicyBegin(G.mc, t)) dlgSay(-1, "The lights go off... lots of giggling... and a pillow fight that gets VERY friendly. (Some things are private!)");
        changeRel(G.mc, t, 8, 15);
        me.needs[N_FUN] = std::fmin(100.f, me.needs[N_FUN] + 30); me.needs[N_ENERGY] = std::fmax(0.f, me.needs[N_ENERGY] - 15);
        addMoodlet(me, "Glowing", EM_FLIRTY, 3, 720);
        fxBurst(200, 100, 30);
        // ... and sometimes a baby comes of it
        Sim* mom = !me.look.guy && s.look.guy ? &me : (me.look.guy && !s.look.guy ? &s : nullptr);
        if (mom && G.pregnancyOn && !mom->pregnant && mom->stage <= ST_MIDDLE && frand() < (lxBirthControl((int)(mom - &G.sims[0])) ? 0.01f : 0.35f)) {
            mom->pregnant = true; mom->pregDays = 3;
            if (mom == &me) toast("Surprise! You're expecting a baby! (3 days)");
            else toast(s.name + " is expecting your baby! The little one will come live with you.");
            addMemoryOnce(MEM_LOVE, G.mc, t, "We're having a baby!");
        }
        break;
    }
    case SO_SLEEPOVER: dramaSleepover(t); break;
    case SO_SLOWDANCE: fin("Dance with me?", "*slow dances*"); changeRel(G.mc, t, 3, 7); addMoodlet(me, "Swept Off My Feet", EM_FLIRTY, 2, 180); dateBump(t, 14); break;
    // ---------------- family ----------------
    case SO_FEED: s.needs[N_HUNGER] = std::fmin(100.f, s.needs[N_HUNGER] + 45); fin("Num num num!", nullptr); changeRel(G.mc, t, 3, 0); break;
    case SO_PLAY: s.needs[N_FUN] = std::fmin(100.f, s.needs[N_FUN] + 40); fin("Peekaboo!", "*giggles*"); changeRel(G.mc, t, 4, 0); me.needs[N_FUN] += me.hasTrait(TR_FAMILY) ? 10 : 4; break;
    case SO_CUDDLE: s.needs[N_SOCIAL] = std::fmin(100.f, s.needs[N_SOCIAL] + 40); fin("Who's my baby?", nullptr); changeRel(G.mc, t, 5, 0); if (me.hasTrait(TR_FAMILY)) addMoodlet(me, "Baby Snuggles", EM_HAPPY, 2, 180); break;
    case SO_BEDTIME: s.needs[N_ENERGY] = 100; s.needs[N_HYGIENE] = std::fmin(100.f, s.needs[N_HYGIENE] + 30); fin("Night night!", nullptr); break;
    case SO_HOMEWORK: s.homework = true; s.gradePts = std::fmin(100.f, s.gradePts + 3); skillGain(s, SK_LOGIC, 2); fin("Let's do this together.", "Thanks!"); changeRel(G.mc, t, 4, 0); break;
    }
}
