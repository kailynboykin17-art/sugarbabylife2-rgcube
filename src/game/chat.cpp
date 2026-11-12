// "Alive" texting: conversations open about what's actually going on (your STATUS posts,
// grudges, the weather, the hour, their mood, gossip, jealousy), branch over a few turns,
// and every person answers in their own voice (constant texter, slow replier, flirt,
// mysterious, awkward, ghost). Replies take time to arrive ("typing..."), leaving someone
// on read gets you double-texted, follow-ups make plans / dates / spill secrets, and people
// remember how your last chat went.
#include "chat.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "people.hpp"
#include "audio.hpp"
#include "drama.hpp"
#include "status.hpp"
#include "crime.hpp"
#include "life.hpp"
#include "phone.hpp"
#include "goals.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

int lovebugStyle(int sim);   // lovebug.cpp: 0 fast, 1 slow, 2 flirt, 3 mysterious, 4 awkward, 5 ghost

namespace {
enum Tone { TN_SWEET, TN_FUNNY, TN_FLIRTY, TN_SHADY, TN_DEEP, TN_IGNORE, TN_YES, TN_MAYBE, TN_NO };
enum Topic {
    TP_HI, TP_WYD, TP_COMPLIMENT, TP_FLIRT, TP_PLANS, TP_APOLOGY, TP_TEA, TP_POST, TP_VENT, TP_MEME,   // you start these
    TP_N_SAWPOST, TP_N_SUBTWEET, TP_N_GRUDGE, TP_N_WEATHER, TP_N_GM, TP_N_UP, TP_N_SAD, TP_N_HYPE,      // they start these
    TP_N_GOSSIP, TP_N_MISS, TP_N_JEALOUS, TP_N_CALLBACK, TP_N_HOBBY, TP_N_GOAL,
    TP_N_LOVE, TP_N_BREAKUP, TP_N_ENGAGED, TP_N_BABY, TP_N_DONE, TP_COUNT };   // life news
enum Follow { FQ_NONE, FQ_HANG, FQ_DATE, FQ_SECRET, FQ_ADVICE, FQ_RATE };

bool valid(int s) { return s >= 0 && s < (int)G.sims.size(); }
std::string nm(int s) { return valid(s) ? G.sims[s].name : std::string("someone"); }
int fr(int s) { const Rel* r = findRel(G.mc, s); return r ? r->fr : 0; }
int ro(int s) { const Rel* r = findRel(G.mc, s); return r ? r->ro : 0; }
uint32_t hsh(uint32_t a) { a ^= a >> 16; a *= 0x7feb352du; a ^= a >> 15; a *= 0x846ca68bu; return a ^ (a >> 16); }
std::string fill(const char* t, int sim, int about) {
    std::string o;
    for (const char* p = t; *p; p++) {
        if (*p == '{' && p[2] == '}') {
            char k = p[1];
            if (k == 'x') o += nm(about); else if (k == 'n') o += MC().name; else if (k == 's') o += nm(sim);
            else if (k == 'h') o += lifeHobbyName(sim); else if (k == 'g') o += lifeGoalName(sim);
            else if (k == 'p') o += lifeFavPlaceName(sim);
            p += 2; continue;
        }
        o += *p;
    }
    return o;
}
// their texting voice
std::string voice(int sim, std::string s, uint32_t seed) {
    int st = lovebugStyle(sim);
    const Sim& o = G.sims[sim];
    auto low = [](std::string x) { for (char& c : x) c = (char)tolower((unsigned char)c); return x; };
    Emotion em = emotionOf(o);
    if (em == EM_SAD && !(seed % 3)) { s = low(s); while (!s.empty() && (s.back() == '!' || s.back() == '.')) s.pop_back(); s += " :("; }
    switch (st) {
    case 0: s = low(s); if (seed % 2) s += "!!"; if (seed % 5 == 0) s = "omg " + s; break;
    case 1: if (seed % 3 == 0) s = "sorry just saw this. " + s; break;
    case 2: if (seed % 2) s += " ;)"; break;
    case 3: { size_t c = s.find_first_of(".!?,"); if (c != std::string::npos && c > 6 && seed % 2) s = s.substr(0, c) + "..."; break; }
    case 4: if (seed % 2) s = "haha " + s; if (seed % 4 == 0) s += " (sorry that was weird)"; break;
    default: break;
    }
    if (o.hasTrait(TR_GLAMOROUS) && seed % 4 == 1) s += " babe";
    if (o.hasTrait(TR_HOTHEAD) && seed % 5 == 2) for (char& c : s) c = (char)toupper((unsigned char)c);
    return s;
}

// ------------------------------------------------------------------ what each topic says
struct TopicDef { const char* opener; const char* npcReply0; Tone tones[3]; const char* labels[3]; Follow follow; };
// opener: what starts it (you, for TP_ < TP_N_SAWPOST; them otherwise). npcReply0: their answer to your opener.
const TopicDef TOPICS[TP_COUNT] = {
    {"heyyy :)", "hiii! what's up?", {TN_SWEET, TN_FUNNY, TN_FLIRTY}, {"just thinking about u", "nothing, i'm bored. entertain me", "missed u ;)"}, FQ_HANG},
    {"wyd?", "nothing much, just {h}. u?", {TN_SWEET, TN_FUNNY, TN_DEEP}, {"wanna do that together sometime?", "watching paint dry lol", "honestly kinda lonely today"}, FQ_HANG},
    {"just wanted to say ur style is amazing", "STOP. u made my whole day", {TN_SWEET, TN_FLIRTY, TN_FUNNY}, {"i mean it!", "u always look good tho ;)", "ok now hype ME up"}, FQ_RATE},
    {"can't stop thinking about u", "...really? ;)", {TN_FLIRTY, TN_SWEET, TN_FUNNY}, {"really really", "ur kinda my favorite person", "jk... unless?"}, FQ_DATE},
    {"we should hang out soon!", "yes!! when?", {TN_YES, TN_MAYBE, TN_FUNNY}, {"today!", "this weekend?", "whenever ur free lol"}, FQ_HANG},
    {"hey... i'm sorry about before", "...", {TN_DEEP, TN_SWEET, TN_FUNNY}, {"i was wrong and i miss u", "can we start over?", "i brought snacks as an apology"}, FQ_NONE},
    {"ok i have TEA", "SPILL. NOW.", {TN_SHADY, TN_DEEP, TN_FUNNY}, {"{x} is so fake lol", "i'm kinda worried about {x} tho", "{x} tripped at the mall"}, FQ_SECRET},
    {"saw ur post!!", "omg u did?? what'd u think", {TN_SWEET, TN_FUNNY, TN_SHADY}, {"obsessed with it", "the caption killed me lol", "it was... a choice"}, FQ_RATE},
    {"ugh today was so bad", "noo what happened??", {TN_DEEP, TN_FUNNY, TN_SHADY}, {"just feeling kinda down", "everything went wrong lol", "people are so annoying"}, FQ_HANG},
    {"[sends a meme]", "LMAOOO", {TN_FUNNY, TN_SWEET, TN_FLIRTY}, {"[sends 14 more memes]", "this is so us", "ur laugh is cute btw"}, FQ_NONE},
    // ---- they start these
    {"saw ur post omg!!", "", {TN_SWEET, TN_FUNNY, TN_FLIRTY}, {"thank u!! <3", "i know i ate lol", "dressed up for u ;)"}, FQ_RATE},
    {"was that post about me??", "", {TN_SWEET, TN_SHADY, TN_DEEP}, {"no!! not at all", "if the shoe fits", "...kinda. can we talk?"}, FQ_NONE},
    {"are u still mad about the other day?", "", {TN_SWEET, TN_SHADY, TN_DEEP}, {"nah we're good <3", "yeah. i am.", "i just need some time"}, FQ_HANG},
    {"it's pouring. movie night??", "", {TN_YES, TN_FUNNY, TN_NO}, {"YES bring blankets", "only if u bring popcorn lol", "i'm staying in alone sorry"}, FQ_HANG},
    {"good morning :)", "", {TN_SWEET, TN_FLIRTY, TN_FUNNY}, {"morning!! sleep ok?", "morning cutie ;)", "it's too early for words"}, FQ_DATE},
    {"u up?", "", {TN_SWEET, TN_FLIRTY, TN_IGNORE}, {"yeah can't sleep. u ok?", "for u? always ;)", "[pretend to be asleep]"}, FQ_DATE},
    {"having a really bad day...", "", {TN_DEEP, TN_SWEET, TN_FUNNY}, {"wanna talk about it?", "i'm here. always", "want me to send u memes?"}, FQ_HANG},
    {"ok i'm SO proud of u btw", "", {TN_SWEET, TN_FUNNY, TN_DEEP}, {"stoppp ur gonna make me cry", "i know i'm amazing lol", "that means a lot coming from u"}, FQ_NONE},
    {"EMERGENCY. it's about {x}", "", {TN_SHADY, TN_DEEP, TN_FUNNY}, {"tell me everything", "wait is {x} ok??", "*grabs popcorn*"}, FQ_SECRET},
    {"i miss u. like a lot", "", {TN_FLIRTY, TN_SWEET, TN_DEEP}, {"come over then ;)", "i miss u too!!", "we should talk more"}, FQ_DATE},
    {"why were u hanging out with {x}??", "", {TN_SWEET, TN_SHADY, TN_DEEP}, {"we're just friends i promise", "i can hang out with whoever", "are u jealous? that's kinda cute"}, FQ_NONE},
    {"still thinking about our last convo lol", "", {TN_SWEET, TN_FUNNY, TN_DEEP}, {"same honestly", "i'm unforgettable lol", "it meant a lot to me"}, FQ_HANG},
    {"ok i'm getting really into {h}", "", {TN_SWEET, TN_FUNNY, TN_DEEP}, {"teach me!!", "nerd alert lol", "that's so cool, show me"}, FQ_HANG},
    {"i'm so close to my dream: {g}!!", "", {TN_SWEET, TN_DEEP, TN_SHADY}, {"u got this!!", "what's the next step?", "if u say so lol"}, FQ_ADVICE},
    // ---- life news (life.cpp)
    {"ok don't freak out but me and {x} are a thing now!!", "", {TN_SWEET, TN_FUNNY, TN_SHADY}, {"OMG i'm so happy for u!!", "took u long enough lol", "{x}? really?"}, FQ_HANG},
    {"so... {x} and i broke up", "", {TN_DEEP, TN_SWEET, TN_SHADY}, {"are u ok? want me to come over?", "ur too good for them anyway", "honestly? saw it coming"}, FQ_HANG},
    {"{x} PROPOSED!!! i said YES", "", {TN_SWEET, TN_FUNNY, TN_DEEP}, {"AHHH congrats!!!", "can i be in the wedding lol", "i'm so happy for u. really"}, FQ_NONE},
    {"the baby's here!! {x} and i are parents now", "", {TN_SWEET, TN_FUNNY, TN_DEEP}, {"omg!! send pics!!", "can i babysit lol", "that's so beautiful"}, FQ_HANG},
    {"I DID IT!! i actually reached my dream!!", "", {TN_SWEET, TN_FUNNY, TN_SHADY}, {"I'M SO PROUD OF U", "ok legend behavior", "cool i guess"}, FQ_NONE},
};
struct FollowDef { const char* q; const char* labels[3]; Tone tones[3]; };
const FollowDef FOLLOWS[6] = {
    {"", {"", "", ""}, {TN_YES, TN_YES, TN_YES}},
    {"wanna hang out later?", {"yes! come over", "tomorrow after school/work?", "can't, sorry"}, {TN_YES, TN_MAYBE, TN_NO}},
    {"so... when are u taking me out? ;)", {"tonight. pick a place", "soon, i promise ;)", "let's just be friends"}, {TN_YES, TN_MAYBE, TN_NO}},
    {"ok but can u keep a secret?", {"always. spill", "don't tell me drama", "only if it's juicy lol"}, {TN_YES, TN_NO, TN_MAYBE}},
    {"should i actually go for it?", {"YES go for it", "take it slow", "honestly? no"}, {TN_YES, TN_MAYBE, TN_NO}},
    {"ok be honest, rate my fit today 1-10", {"11!!", "solid 8", "...it's giving 5"}, {TN_YES, TN_MAYBE, TN_NO}},
};
// how they react: [tone][liked]
const char* const REACT[9][2][3] = {
    {{"oh... ok", "um thanks i guess", "ok"}, {"aww ur so sweet <3", "this is why ur my favorite", "ok now i'm smiling"}},
    {{"...was that supposed to be funny", "lol ok", "i don't get it"}, {"LMAOOO i'm crying", "STOP i'm wheezing", "ur so funny omg"}},
    {{"haha... ok", "um. let's just be friends?", "that's kinda forward lol"}, {"ok ur making me blush", "...say that again ;)", "u can't just SAY that"}},
    {{"wow. ok.", "rude much?", "noted. bye"}, {"LMAO ur evil, i love it", "the shade. iconic", "ok that was funny"}},
    {{"that's a lot for a text lol", "ok...", "um k"}, {"i feel the same honestly", "i'm really glad u told me", "thank u for being real with me"}},
    {{"??", "hello??", "ok so ur ignoring me"}, {"??", "hello??", "ok so ur ignoring me"}},
    {{"ok cool", "yay", "ok!"}, {"YAY!! omw", "ok it's a plan!!", "best day ever"}},
    {{"ok...", "maybe means no", "fine"}, {"ok!! can't wait", "deal :)", "perfect"}},
    {{"oh. ok.", "ouch", "wow ok"}, {"fair enough lol", "no worries!", "ok honest, i respect it"}},
};
const char* const CLOSE[2][4] = {{"anyway ttyl", "ok bye", "k", "gotta go"}, {"ttyl <3", "ok talk later!!", "ur the best. bye!", "night <3"}};

// does this person like that tone right now?
bool likes(int sim, Tone t, int topic) {
    const Sim& o = G.sims[sim];
    int f = fr(sim), r = ro(sim);
    switch (t) {
    case TN_SWEET: return f > -20 || o.hasTrait(TR_CHEERFUL);
    case TN_FUNNY: return o.hasTrait(TR_GOOFBALL) || o.hasTrait(TR_OUTGOING) || f > 15 || (hsh(sim * 31 + G.day) % 3);
    case TN_FLIRTY: return canRomance(G.mc, sim) && (r >= 12 || lovebugStyle(sim) == 2 || o.hasTrait(TR_ROMANTIC));
    case TN_SHADY: return topic == TP_TEA || topic == TP_N_GOSSIP ? (f > 20 && !o.hasTrait(TR_CHEERFUL)) : o.hasTrait(TR_GOOFBALL);
    case TN_DEEP: return o.hasTrait(TR_BOOKWORM) || o.hasTrait(TR_FAMILY) || o.hasTrait(TR_LONER) || f > 30;
    case TN_IGNORE: return false;
    case TN_YES: case TN_MAYBE: return true;
    case TN_NO: return f > 40 || o.hasTrait(TR_CHEERFUL);
    }
    return true;
}

// runtime: replies on their way, last convo memory, unanswered timers
struct Pending { int sim; float t; std::string text; int nextPrompt; int about; };
std::vector<Pending> g_pend;
struct Last { int tone = -1; bool liked = false; uint16_t day = 0; };
Last g_last[512];
struct Seen { int sim; float hours; };
std::vector<Seen> g_seen;

float delayFor(int sim) {
    switch (lovebugStyle(sim)) { case 0: return 1.2f; case 1: return 6.f + irand(6); case 3: return 4.f; default: return 2.5f; }
}
void deliver(int sim, const std::string& text, int nextPrompt, int about, float extra = 0) {
    g_pend.push_back({sim, delayFor(sim) + extra, text, nextPrompt, about});
}
int promptFor(int topic, int stage) { return CHAT_BASE + topic * 4 + stage; }
void decode(int p, int& topic, int& stage) { topic = (p - CHAT_BASE) / 4; stage = (p - CHAT_BASE) % 4; }
void trim(ChatThread& th) { while (th.msgs.size() > 24) th.msgs.erase(th.msgs.begin()); }
void bump(int sim) { for (size_t i = 0; i < G.threads.size(); i++) if (G.threads[i].sim == sim && i > 0) { ChatThread t = G.threads[i]; G.threads.erase(G.threads.begin() + i); G.threads.insert(G.threads.begin(), t); return; } }
int someoneElse(int sim) {
    std::vector<int> v;
    for (const Rel& r : G.rels) { int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1; if (valid(o) && o != sim && r.met && G.sims[o].stage >= ST_PRETEEN) v.push_back(o); }
    return v.empty() ? -1 : v[hsh(sim * 7 + G.day) % v.size()];
}
// NPC opens a conversation with you
void npcOpen(int sim, int topic, int about) {
    ChatThread& th = threadFor(sim);
    if (th.prompt >= 0) return;
    th.msgs.push_back({false, voice(sim, fill(TOPICS[topic].opener, sim, about), hsh(sim + G.day * 13 + topic))});
    th.prompt = (int16_t)promptFor(topic, 1);
    th.about = (int16_t)about;
    th.unread = true;
    trim(th); bump(sim);
    g_seen.push_back({sim, 0});
    toast("New text from " + nm(sim) + "!");
    sfxTap();
}
void apply(int sim, Tone t, bool lk, int topic) {
    int dfr = 0, dro = 0;
    switch (t) {
    case TN_SWEET: dfr = lk ? 4 : 1; break;
    case TN_FUNNY: dfr = lk ? 4 : -1; break;
    case TN_FLIRTY: dfr = lk ? 1 : -2; dro = lk ? 6 : -3; break;
    case TN_SHADY: dfr = lk ? 2 : -7; break;
    case TN_DEEP: dfr = lk ? 6 : 0; break;
    case TN_IGNORE: dfr = -3; break;
    case TN_YES: dfr = 3; break;
    case TN_MAYBE: dfr = 1; break;
    case TN_NO: dfr = lk ? 0 : -4; break;
    }
    if (topic == TP_APOLOGY && t != TN_IGNORE) { dfr += 6; dramaAddGrudge(sim, 0); }
    if (topic == TP_N_JEALOUS && t == TN_SHADY) { dfr -= 4; dro -= 6; }
    changeRel(G.mc, sim, dfr, canRomance(G.mc, sim) ? dro : 0);
    MC().needs[N_SOCIAL] = std::fmin(100.f, MC().needs[N_SOCIAL] + 5);
    if (dro >= 5) addMoodlet(MC(), "Cute Texts", EM_FLIRTY, 1, 120);
    if (t == TN_SHADY && !lk) {
        if (G.sims[sim].hasTrait(TR_HOTHEAD)) { dramaAddGrudge(sim, 3); if (irand(2)) statusPostAs(sim, "some people text like they're allergic to being nice", G.mc); }
        lifeRemember(sim, LM_HURT, G.mc);
    } else if (lk && (t == TN_SWEET || t == TN_DEEP)) lifeRemember(sim, LM_KIND, G.mc);
    if (valid(sim) && sim < 512) g_last[sim] = {t, lk, G.day};
    if (irand(4) == 0) {   // what they're thinking (Sims-style)
        static const char* const GOOD[4] = {"{n} always knows what to say.", "I love texting {n}.", "{n} is SO funny.", "{n} gets me."};
        static const char* const BAD[4] = {"Why would {n} say that?", "{n} is being weird.", "Ugh. {n}.", "Was {n} even listening?"};
        thoughtPopup(sim, fill((lk ? GOOD : BAD)[irand(4)], sim, -1), lk ? (dro > 0 ? 2 : 1) : -1);
    }
}
void doFollow(ChatThread& th, int sim, int follow, int choice) {
    Tone t = FOLLOWS[follow].tones[choice];
    bool lk = likes(sim, t, -1);
    th.msgs.push_back({true, FOLLOWS[follow].labels[choice]});
    apply(sim, t, lk, -1);
    std::string reply;
    if (follow == FQ_HANG) {
        if (t == TN_YES) {
            if (roomIsHome(G.room)) {
                Actor a; a.kind = 0; a.id = (uint16_t)sim; a.x = 380; a.tx = 300; a.y = 200 + irand(30); a.say = "I'm here!!"; a.sayT = 3; a.stay = 180;
                curRoom().actors.push_back(a);
                reply = "omw!!";
            } else reply = "ok text me when ur home!";
        } else if (t == TN_MAYBE) {
            Event e; e.day = (uint16_t)(G.day + 1); e.hour = 17; e.type = EV_PLAN; e.sim = (int16_t)sim; e.text = "Hang out with " + nm(sim); G.events.push_back(e);
            reply = "it's a plan! (check ur calendar)";
            toast("Plans with " + nm(sim) + " tomorrow at 5!");
        } else reply = REACT[t][lk][irand(3)];
    } else if (follow == FQ_DATE) {
        if (t == TN_YES && canRomance(G.mc, sim) && MC().isAdult()) { reply = "ahh ok!! where are we going?"; th.prompt = -1; deliver(sim, voice(sim, reply, hsh(sim + 5)), -2, -1); return; }
        if (t == TN_NO) { changeRel(G.mc, sim, 0, -10); reply = "oh. ok. friends it is"; }
        else reply = REACT[t][lk][irand(3)];
    } else if (follow == FQ_SECRET) {
        if (t == TN_NO) reply = "ok fine, ur no fun";
        else {
            int x = th.about >= 0 && th.about != G.mc ? th.about : someoneElse(sim);
            if (valid(x)) {
                int sec = secretOf(x);
                reply = "ok so... " + nm(x) + " " + SECRETS[sec % NUM_SECRETS] + ". DON'T tell anyone";
                bool known = false; for (const Dirt& d : G.dirt) if (d.sim == x) known = true;
                if (!known) { G.dirt.push_back({(uint16_t)x, (uint8_t)(sec % NUM_SECRETS), 0, G.day}); toast("You learned a secret about " + nm(x) + "!"); }
                changeRel(G.mc, sim, 3, 0);
            } else reply = "ugh nvm i forgot lol";
        }
    } else if (follow == FQ_ADVICE) {
        reply = t == TN_YES ? "ok!! ur right. i'm doing it" : t == TN_MAYBE ? "yeah... slow and steady" : "wow ok. i'm doing it anyway";
        if (t == TN_YES) lifeBoostGoal(sim, 10);
    } else if (follow == FQ_RATE) {
        reply = t == TN_YES ? "ok i'm screenshotting this" : t == TN_MAYBE ? "i'll take it lol" : "WOW. ok. honesty hurts";
        if (t == TN_NO) changeRel(G.mc, sim, -4, 0);
    }
    th.prompt = -1;
    std::string close = CLOSE[lk ? 1 : 0][irand(4)];
    deliver(sim, voice(sim, reply, hsh(sim * 3 + G.day)), -1, -1);
    deliver(sim, voice(sim, close, hsh(sim * 5 + G.day)), -1, -1, 2.f);
}
}   // namespace

// ================================================================== API
std::vector<ChatOpt> chatStarters(int sim) {
    std::vector<ChatOpt> v;
    if (!valid(sim)) return v;
    v.push_back({"Say hey", TP_HI, 0});
    v.push_back({"Ask wyd", TP_WYD, 1});
    if (dramaHasGrudge(sim)) v.push_back({"Apologize", TP_APOLOGY, 1});
    if (canRomance(G.mc, sim) && (ro(sim) >= 5 || findRel(G.mc, sim) && findRel(G.mc, sim)->matched)) v.push_back({"Flirt", TP_FLIRT, 0});
    v.push_back({"Make plans", TP_PLANS, 2});
    for (const StPost& p : G.st) if (p.who == sim && G.day - p.day <= 1) { v.push_back({"Their STATUS post", TP_POST, 3}); break; }
    if (someoneElse(sim) >= 0 && fr(sim) > 0) v.push_back({"Spill tea", TP_TEA, 3});
    Emotion em = emotionOf(MC());
    if (em == EM_SAD || em == EM_ANGRY || em == EM_TENSE || em == EM_BORED) v.push_back({"Vent to them", TP_VENT, 1});
    v.push_back({"Compliment them", TP_COMPLIMENT, 0});
    v.push_back({"Send a meme", TP_MEME, 2});
    if (v.size() > 8) v.resize(8);
    return v;
}
void chatStart(int sim, int topic) {
    if (!valid(sim) || topic < 0 || topic >= TP_N_SAWPOST) return;
    ChatThread& th = threadFor(sim);
    int about = topic == TP_TEA ? someoneElse(sim) : -1;
    th.msgs.push_back({true, fill(TOPICS[topic].opener, sim, about)});
    th.prompt = -1; th.about = (int16_t)about;
    trim(th);
    if (lovebugStyle(sim) == 5 && irand(3) == 0) { g_seen.push_back({sim, -99}); return; }   // ghosted
    std::string r = TOPICS[topic].npcReply0;
    if (topic == TP_APOLOGY) r = fr(sim) > -30 ? "...ok. i'm listening" : "now u wanna apologize?";
    if (topic == TP_FLIRT && !likes(sim, TN_FLIRTY, topic)) r = "haha... ok?";
    deliver(sim, voice(sim, fill(r.c_str(), sim, about), hsh(sim + topic * 7 + G.day)), promptFor(topic, 1), about);
}
std::vector<ChatOpt> chatChoices(const ChatThread& th) {
    std::vector<ChatOpt> v;
    if (!chatLive(th.prompt)) return v;
    int topic, stage; decode(th.prompt, topic, stage);
    if (topic < 0 || topic >= TP_COUNT) return v;
    const Sim& me = MC();
    static const char* const TAG[9] = {"Sweet", "Funny", "Flirty", "Shady", "Deep", "", "", "", ""};
    if (stage == 1) {
        const TopicDef& d = TOPICS[topic];
        for (int i = 0; i < 3; i++) {
            if (d.tones[i] == TN_FLIRTY && !canRomance(G.mc, th.sim)) continue;
            std::string lb = fill(d.labels[i], th.sim, th.about);
            if (d.tones[i] <= TN_DEEP) lb = std::string("[") + TAG[d.tones[i]] + "] " + lb;
            v.push_back({lb, i, d.tones[i] == TN_SHADY ? 1 : d.tones[i] == TN_FLIRTY ? 0 : d.tones[i] == TN_FUNNY ? 2 : 3});
        }
        // a reply your traits unlock
        if (me.hasTrait(TR_HOTHEAD)) v.push_back({"[Hothead] who even asked", 10 + TN_SHADY, 1});
        else if (me.hasTrait(TR_GOOFBALL)) v.push_back({"[Goofball] [sends a voice memo of u singing]", 10 + TN_FUNNY, 2});
        else if (me.hasTrait(TR_ROMANTIC) && canRomance(G.mc, th.sim)) v.push_back({"[Romantic] writes them a little poem", 10 + TN_FLIRTY, 0});
        else if (me.hasTrait(TR_BOOKWORM)) v.push_back({"[Bookworm] actually that reminds me of this book...", 10 + TN_DEEP, 3});
        else if (me.hasTrait(TR_CHEERFUL)) v.push_back({"[Cheerful] YAYYYY!!! <3 <3", 10 + TN_SWEET, 0});
        v.push_back({"Leave them on read", 20, 1});
    } else if (stage == 2) {
        int f = TOPICS[topic].follow;
        for (int i = 0; i < 3; i++) v.push_back({FOLLOWS[f].labels[i], 30 + i, i == 2 ? 1 : i});
    }
    return v;
}
void chatChoose(ChatThread& th, int i) {
    auto v = chatChoices(th);
    if (i < 0 || i >= (int)v.size()) return;
    int topic, stage; decode(th.prompt, topic, stage);
    int sim = th.sim, code = v[i].tone;
    for (size_t k = 0; k < g_seen.size(); k++) if (g_seen[k].sim == sim) { g_seen.erase(g_seen.begin() + k); break; }
    th.unread = false;
    if (stage == 2) { doFollow(th, sim, TOPICS[topic].follow, code - 30); trim(th); return; }
    if (code == 20) {   // left on read
        th.msgs.push_back({true, "[Seen]"});
        th.prompt = -1;
        apply(sim, TN_IGNORE, false, topic);
        g_seen.push_back({sim, 0.5f});
        trim(th);
        return;
    }
    Tone t; bool strong = false;
    if (code >= 10) { t = (Tone)(code - 10); strong = true; }
    else t = TOPICS[topic].tones[code];
    th.msgs.push_back({true, v[i].label.substr(v[i].label[0] == '[' ? v[i].label.find(']') + 2 : 0)});
    bool lk = likes(sim, t, topic) || (strong && irand(3));
    apply(sim, t, lk, topic);
    if (strong) changeRel(G.mc, sim, lk ? 2 : -2, 0);
    std::string r = REACT[t][lk ? 1 : 0][hsh(sim + topic + G.day) % 3];
    int follow = TOPICS[topic].follow;
    if (follow == FQ_DATE && !(canRomance(G.mc, sim) && MC().isAdult())) follow = FQ_HANG;
    th.prompt = -1;
    trim(th);
    if (follow != FQ_NONE && lk && t != TN_SHADY) {
        deliver(sim, voice(sim, r, hsh(sim * 11 + G.day)), -1, th.about);
        deliver(sim, voice(sim, FOLLOWS[follow].q, hsh(sim * 13 + G.day)), promptFor(topic, 2), th.about, 1.5f);
    } else {
        deliver(sim, voice(sim, r, hsh(sim * 11 + G.day)), -1, -1);
        if (lk) deliver(sim, voice(sim, CLOSE[1][irand(4)], hsh(sim * 17 + G.day)), -1, -1, 2.f);
    }
}
void chatTyped(int sim, const std::string& msg) {
    if (!valid(sim) || msg.empty()) return;
    ChatThread& th = threadFor(sim);
    th.msgs.push_back({true, msg});
    th.prompt = -1;
    trim(th);
    std::string m; for (char c : msg) m += (char)tolower((unsigned char)c);
    auto has = [&](const char* k) { return m.find(k) != std::string::npos; };
    Tone t = TN_SWEET; int topic = TP_HI;
    if (has("sorry") || has("apolog") || has("my bad")) { t = TN_DEEP; topic = TP_APOLOGY; }
    else if (has("love") || has("cute") || has("miss u") || has("miss you") || has("date") || has("kiss") || has(";)") || has("<3")) { t = TN_FLIRTY; topic = TP_FLIRT; }
    else if (has("hate") || has("ugly") || has("shut up") || has("annoying") || has("fake") || has("ew")) t = TN_SHADY;
    else if (has("lol") || has("lmao") || has("haha") || has("meme")) t = TN_FUNNY;
    else if (has("hang") || has("come over") || has("party") || has("wanna") || has("plans")) { t = TN_YES; topic = TP_PLANS; }
    else if (has("sad") || has("bad day") || has("tired") || has("stressed") || has("lonely")) { t = TN_DEEP; topic = TP_VENT; }
    else if (has("tea") || has("gossip") || has("heard")) { t = TN_SHADY; topic = TP_TEA; }
    else if (has("pretty") || has("outfit") || has("slay") || has("amazing") || has("best")) t = TN_SWEET;
    bool lk = likes(sim, t, topic);
    apply(sim, t, lk, topic);
    std::string r;
    if (topic == TP_PLANS) r = lk ? "yes!! when??" : "maybe another time";
    else if (topic == TP_VENT) r = "noo :( want me to come over?";
    else if (topic == TP_TEA) r = "wait WHAT. tell me more";
    else if (msg.back() == '?') { static const char* const Q[4] = {"hmm good question lol", "honestly? yes", "idk... maybe?", "why do u ask ;)"}; r = Q[hsh(sim + (uint32_t)msg.size()) % 4]; }
    else r = REACT[t][lk ? 1 : 0][hsh(sim + (uint32_t)msg.size()) % 3];
    int next = topic == TP_PLANS && lk ? promptFor(TP_PLANS, 2) : -1;
    deliver(sim, voice(sim, r, hsh(sim * 19 + (uint32_t)msg.size())), next, -1);
}
void chatNews(int sim, int kind, int about) {
    if (!valid(sim) || sim == G.mc || G.sims[sim].household || G.sims[sim].stage < ST_PRETEEN) return;
    int h = (int)(G.minute / 60);
    if (h < 8 || h > 23 || fr(sim) < 15) return;
    const ChatThread* th = nullptr; for (const ChatThread& t : G.threads) if (t.sim == sim) th = &t;
    if ((th && th->prompt >= 0) || chatWaiting(sim)) return;
    int topic = kind == LM_ROMANCE ? TP_N_LOVE : kind == LM_BREAKUP ? TP_N_BREAKUP : kind == LM_ENGAGED ? TP_N_ENGAGED : kind == LM_BABY ? TP_N_BABY : kind == LM_GOAL ? TP_N_DONE : -1;
    if (topic < 0) return;
    npcOpen(sim, topic, about);
}
bool chatTyping(int sim) { for (const Pending& p : g_pend) if (p.sim == sim && p.t < 3.5f) return true; return false; }
bool chatWaiting(int sim) { for (const Pending& p : g_pend) if (p.sim == sim) return true; return false; }

void chatTick(float dt) {
    for (size_t i = 0; i < g_pend.size();) {
        Pending& p = g_pend[i];
        // only one reply "in flight" per person at a time: later ones wait their turn
        bool first = true; for (size_t k = 0; k < i; k++) if (g_pend[k].sim == p.sim) first = false;
        if (first) p.t -= dt;
        if (p.t > 0 || !valid(p.sim)) { if (!valid(p.sim)) g_pend.erase(g_pend.begin() + i); else i++; continue; }
        ChatThread& th = threadFor(p.sim);
        th.msgs.push_back({false, p.text});
        if (p.nextPrompt == -2) { th.prompt = -1; phoneDatePickLater(p.sim); }
        else if (p.nextPrompt >= 0) { th.prompt = (int16_t)p.nextPrompt; th.about = (int16_t)p.about; }
        th.unread = true;
        trim(th); bump(p.sim);
        sfxTap();
        g_pend.erase(g_pend.begin() + i);
    }
}

void chatHourly(int hour) {
    if (G.sims.size() < 3) return;
    // double texts when you leave someone on read
    for (size_t i = 0; i < g_seen.size();) {
        Seen& s = g_seen[i];
        if (s.hours < -50) { g_seen.erase(g_seen.begin() + i); continue; }
        s.hours += 1;
        if (s.hours >= 2 && valid(s.sim)) {
            ChatThread& th = threadFor(s.sim);
            static const char* const DT[4] = {"??", "helloooo", "ok so ur ignoring me", "...k"};
            th.msgs.push_back({false, voice(s.sim, DT[irand(4)], hsh(s.sim + hour))}); th.unread = true; trim(th); bump(s.sim);
            changeRel(G.mc, s.sim, -2, 0);
            g_seen.erase(g_seen.begin() + i);
            continue;
        }
        i++;
    }
    bool night = hour >= 23 || hour < 2;
    if ((hour < 7 || hour > 23) && !night) return;
    if (irand(100) >= 30) return;
    // who'd text you, and about what
    struct C { int sim, topic, about, w; };
    std::vector<C> c;
    int partner = partnerOf(G.mc);
    for (const Rel& r : G.rels) {
        int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1;
        if (!valid(o) || !r.met || G.sims[o].household || G.sims[o].stage < ST_PRETEEN || o == G.cop) continue;
        const ChatThread* th = nullptr; for (const ChatThread& t : G.threads) if (t.sim == o) th = &t;
        if (th && th->prompt >= 0) continue;
        if (chatWaiting(o)) continue;
        int f = r.fr;
        for (const StPost& p : G.st) {
            if (G.day - p.day > 0) break;
            if (p.who == G.mc && p.about == o && (p.kind == 3 || p.kind == 4)) c.push_back({o, TP_N_SUBTWEET, -1, 8});
            else if (p.who == G.mc && f > 10 && p.kind == 1) c.push_back({o, TP_N_SAWPOST, -1, 3});
        }
        if (dramaHasGrudge(o)) c.push_back({o, TP_N_GRUDGE, -1, 4});
        if ((G.weather == WX_RAIN || G.weather == WX_STORM || G.weather == WX_SNOW) && f > 20) c.push_back({o, TP_N_WEATHER, -1, 2});
        if (o == partner && hour >= 7 && hour <= 9) c.push_back({o, TP_N_GM, -1, 6});
        if (night && r.ro >= 20 && MC().isAdult()) c.push_back({o, TP_N_UP, -1, 3});
        if (emotionOf(G.sims[o]) == EM_SAD && f > 15) c.push_back({o, TP_N_SAD, -1, 4});
        if (f > 40 && irand(6) == 0) c.push_back({o, TP_N_HYPE, -1, 1});
        if (f > 25) { int x = someoneElse(o); if (x >= 0) c.push_back({o, TP_N_GOSSIP, x, 3}); }
        if (r.ro >= 40) c.push_back({o, TP_N_MISS, -1, 2});
        if (o == partner) {   // jealousy: you're close with someone else
            for (const Rel& q : G.rels) { int x = q.a == G.mc ? q.b : q.b == G.mc ? q.a : -1; if (valid(x) && x != o && q.ro >= 30) { c.push_back({o, TP_N_JEALOUS, x, G.sims[o].hasTrait(TR_JEALOUS) ? 14 : 5}); break; } }
        }
        if (o < 512 && g_last[o].tone >= 0 && G.day - g_last[o].day <= 2) c.push_back({o, TP_N_CALLBACK, -1, 2});
        if (f > 10) { c.push_back({o, TP_N_HOBBY, -1, 1}); c.push_back({o, TP_N_GOAL, -1, 1}); }
        if (f > 5 && irand(3) == 0) c.push_back({o, irand(2) ? TP_HI : TP_WYD, -1, 1});
    }
    if (c.empty()) return;
    int tot = 0; for (auto& x : c) tot += x.w;
    int r = irand(tot);
    for (auto& x : c) {
        if (r < x.w) {
            if (x.topic == TP_HI || x.topic == TP_WYD) {   // casual: they open, you get the stage-1 replies
                ChatThread& th = threadFor(x.sim);
                th.msgs.push_back({false, voice(x.sim, x.topic == TP_HI ? "heyyy" : fill("wyd? i'm just {h}", x.sim, -1), hsh(x.sim + hour))});
                th.prompt = (int16_t)promptFor(x.topic, 1); th.about = -1; th.unread = true; trim(th); bump(x.sim);
                toast("New text from " + nm(x.sim) + "!");
                g_seen.push_back({x.sim, 0});
            } else npcOpen(x.sim, x.topic, x.about);
            return;
        }
        r -= x.w;
    }
}
