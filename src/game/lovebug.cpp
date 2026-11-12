// LOVEBUG: the dating app (replaces Glam Date). Every single adult in town gets a profile
// generated from who they are (traits, wardrobe, job); matching weighs shared interests,
// aesthetics, intentions and traits. Matches text in their own style (constant texter,
// slow replier, flirt, mysterious, awkward, ghost), townies match each other, and the
// dating drama spills onto STATUS.
#include "lovebug.hpp"
#include "game.hpp"
#include "draw.hpp"
#include "sim.hpp"
#include "people.hpp"
#include "audio.hpp"
#include "oc.hpp"
#include "drama.hpp"
#include "status.hpp"
#include "phone.hpp"
#include <algorithm>
#include <cmath>

namespace {
const char* const AESTHETIC[8] = {"Y2K Glam", "Soft Girl", "Streetwear", "Grunge", "Old Money", "Sporty", "Boho", "Clean Girl"};
const char* const HOBBY[16] = {"thrifting", "brunch", "karaoke", "gym", "reading", "baking", "skating", "concerts", "beach days", "gaming", "photography", "dancing", "cooking", "art museums", "true crime", "road trips"};
const char* const INTENT[4] = {"something serious", "seeing where it goes", "brunch dates only", "a plus-one for weddings"};
const char* const MUSIC[8] = {"R&B", "pop punk", "K-pop", "Y2K pop", "lo-fi", "reggaeton", "country (sorry)", "throwback hip-hop"};
const char* const STYLE_N[6] = {"replies instantly", "replies... eventually", "big flirt", "mysterious", "a little awkward", "might ghost"};
enum TxStyle { TX_FAST, TX_SLOW, TX_FLIRT, TX_MYST, TX_AWK, TX_GHOST };
const char* const BIOS[12] = {"looking for someone to split fries with", "my love language is sending you memes at 3am", "will judge you by your shoes (lovingly)",
                              "i make a mean grilled cheese and i'm not humble about it", "swipe right if you can parallel park", "certified golden retriever energy",
                              "if you don't like dogs, we can't be friends", "i've been told i'm 'a lot'. in a good way", "my mom thinks i'm a catch. she's biased",
                              "here for a good time AND a long time", "6'0 in platforms", "let's skip the small talk. what's your sign"};
const char* const CATFISH_BIO = "model, millionaire, astronaut. DM for exclusive pics ;)";

uint32_t hsh(int s) { uint32_t h = (uint32_t)s * 2654435761u + 0x9e37u; h ^= h >> 13; h *= 0x5bd1e995u; return h ^ (h >> 15); }
bool valid(int s) { return s >= 0 && s < (int)G.sims.size(); }
bool isCast(int s) { return valid(s) && G.sims[s].bio.rfind("[Cast]", 0) == 0; }
bool catfish(int s) { return !isCast(s) && hsh(s) % 13 == 5; }
int aesthetic(int s) {
    const Sim& o = G.sims[s];
    if (o.hasTrait(TR_GLAMOROUS) || o.hasTrait(TR_FASHIONISTA)) return hsh(s) % 2 ? 0 : 4;
    if (o.hasTrait(TR_ACTIVE)) return 5;
    if (o.hasTrait(TR_CREATIVE) || o.hasTrait(TR_LONER)) return hsh(s) % 2 ? 3 : 6;
    if (o.hasTrait(TR_CHEERFUL) || o.hasTrait(TR_ROMANTIC)) return 1;
    return (int)(hsh(s) % 8);
}
int hobbyA(int s) { const Sim& o = G.sims[s]; if (o.hasTrait(TR_FOODIE)) return 1; if (o.hasTrait(TR_BOOKWORM)) return 4; if (o.hasTrait(TR_ACTIVE)) return 3; if (o.hasTrait(TR_OUTGOING)) return 11; return (int)(hsh(s) % 16); }
int hobbyB(int s) { int a = hobbyA(s), b = (int)(hsh(s + 7) % 16); return b == a ? (b + 5) % 16 : b; }
int intent(int s) { const Sim& o = G.sims[s]; if (o.hasTrait(TR_ROMANTIC) || o.hasTrait(TR_FAMILY)) return 0; if (o.hasTrait(TR_LAZY)) return 2; return (int)(hsh(s + 3) % 4); }
int txStyle(int s) { const Sim& o = G.sims[s]; if (o.hasTrait(TR_OUTGOING)) return TX_FAST; if (o.hasTrait(TR_ROMANTIC)) return TX_FLIRT; if (o.hasTrait(TR_LONER)) return TX_MYST; if (o.hasTrait(TR_LAZY)) return TX_SLOW; return (int)(hsh(s + 11) % 6); }
int ageOf(int s) { const Sim& o = G.sims[s]; int base = o.stage == ST_YA ? 21 : o.stage == ST_ADULT ? 29 : o.stage == ST_MIDDLE ? 44 : 60; return base + (int)(hsh(s + 5) % 7); }
int myHobby() { return hobbyA(G.mc); }
int compat(int s) {
    const Sim& me = MC(); const Sim& o = G.sims[s];
    int c = 38;
    for (uint8_t t : me.traits) if (t != TR_NONE && o.hasTrait(t)) c += 12;
    if (hobbyA(s) == myHobby() || hobbyB(s) == myHobby() || hobbyA(s) == hobbyB(G.mc)) c += 14;
    if (aesthetic(s) == aesthetic(G.mc)) c += 10;
    if (intent(s) == G.lbIntent) c += 12;
    if (me.hasTrait(TR_HOTHEAD) && o.hasTrait(TR_HOTHEAD)) c -= 15;
    if (catfish(s)) c = 99;   // suspiciously perfect
    return std::clamp(c + (int)(hsh(s + G.mc) % 9) - 4, 5, 99);
}
bool blockedLB(int s) { return std::find(G.lbBlock.begin(), G.lbBlock.end(), (uint16_t)s) != G.lbBlock.end(); }
bool likesMe(int s) { return std::find(G.lbLikes.begin(), G.lbLikes.end(), (uint16_t)s) != G.lbLikes.end(); }
void dropLike(int s) { G.lbLikes.erase(std::remove(G.lbLikes.begin(), G.lbLikes.end(), (uint16_t)s), G.lbLikes.end()); }
std::vector<int> discover() {
    std::vector<int> v;
    if (!MC().isAdult() || G.lbDeleted) return v;
    for (int i : swipeCandidates()) {
        if (blockedLB(i)) continue;
        if (G.lbFilter && hobbyA(i) != G.lbFilter - 1 && hobbyB(i) != G.lbFilter - 1) continue;
        v.push_back(i);
    }
    std::stable_sort(v.begin(), v.end(), [](int a, int b) { return (likesMe(a) ? 100 : 0) + compat(a) > (likesMe(b) ? 100 : 0) + compat(b); });
    return v;
}
std::vector<int> matches() {
    std::vector<int> v;
    for (const Rel& r : G.rels) {
        if (!r.matched) continue;
        int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1;
        if (!valid(o) || blockedLB(o) || !canRomance(G.mc, o)) continue;
        v.push_back(o);
    }
    return v;
}
const char* stageOf(int s) {
    const Rel* r = findRel(G.mc, s);
    if (!r) return "Match";
    if (r->status == RS_MARRIED) return "Married";
    if (r->status == RS_ENGAGED) return "Engaged";
    if (r->status == RS_DATING) return r->ro >= 80 ? "Long-term" : "Exclusive";
    if (r->ro >= 45) return "Situationship";
    if (r->ro >= 30) return "Casual dating";
    if (r->ro >= 18) return "First date vibes";
    bool talked = false;
    for (const ChatThread& t : G.threads) if (t.sim == s) for (const Msg& m : t.msgs) if (m.me) talked = true;
    return talked ? "Talking stage" : "New match";
}
void post(int who, const std::string& text, int about) { statusPostAs(who, text, about); }

// ------------------------------------------------------------------ app state
enum Tab { T_DISCOVER, T_LIKES, T_MATCHES, T_ME, T_COUNT };
const char* const TABN[T_COUNT] = {"Discover", "Likes You", "Matches", "My Profile"};
int g_tab = T_DISCOVER, g_card = 0, g_photo = 0, g_sel = 0;
float g_anim = 0; int g_dir = 0; float g_burst = 0;
std::string g_msg; float g_msgT = 0;
void say(const std::string& s) { g_msg = s; g_msgT = 2.6f; }
// pending NPC replies (not saved: replies that were still "typing" just arrive later)
struct Pending { int sim; float t; std::string text; };
std::vector<Pending> g_pend;
float g_typingT = 0; int g_typingSim = -1;

void addText(int s, const std::string& t) { ChatThread& th = threadFor(s); th.msgs.push_back({false, t}); th.unread = true; if (th.msgs.size() > 24) th.msgs.erase(th.msgs.begin()); }
void queueReply(int s, const std::string& t) {
    float d = 1.5f;
    switch (txStyle(s)) { case TX_FAST: d = 0.8f; break; case TX_SLOW: d = 25.f + irand(30); break; case TX_MYST: d = 8.f; break; case TX_GHOST: if (irand(3) == 0) return; d = 15.f; break; default: d = 3.f; break; }
    g_pend.push_back({s, d, t});
    g_typingSim = s; g_typingT = std::min(d, 4.f);
}
const char* opener(int s) {
    static const char* const O[6][2] = {{"hiiii omg we matched!!", "ok ur profile is SO cute. hi!!"}, {"hey", "sorry just saw this. hi :)"}, {"well hello there ;)", "matching with you? lucky me"},
                                        {"...", "interesting."}, {"hi! um. hello. that was two greetings. sorry", "haha hi. i've never done this before"}, {"hey :)", "sup"}};
    return O[txStyle(s)][irand(2)];
}
void doMatch(int s, bool compliment) {
    Rel& r = rel(G.mc, s);
    r.met = true; r.matched = true; r.ro = (int8_t)std::max<int>(r.ro, compliment ? 14 : 8);
    dropLike(s);
    G.lbMatches++;
    queueReply(s, opener(s));
    addMoodlet(MC(), "It's a Match!", EM_FLIRTY, 2, 180);
    g_burst = 1.5f;
    // drama: you matched with your BFF's crush
    int bff = dramaBff();
    if (valid(bff) && bff != s) { const Rel* br = findRel(bff, s); if (br && br->ro >= 25) { changeRel(G.mc, bff, -8, 0); thoughtPopup(bff, "You matched with " + G.sims[s].name + "?! You KNOW I like them.", -1); post(bff, "love when my best friend matches with my crush. love that for me", G.mc); } }
    // drama: you're taken and on LOVEBUG
    int p = partnerOf(G.mc);
    if (valid(p) && p != s && irand(3) == 0) { changeRel(G.mc, p, -10, -15); thoughtPopup(p, "Why are you on LOVEBUG?! We're TOGETHER.", -1); post(ST_ACC_TEA, "someone screenshotted " + MC().name + "'s LOVEBUG profile... aren't they TAKEN??", G.mc); }
}
bool tryLike(int s, bool compliment) {
    const Sim& me = MC();
    int c = compat(s);
    float chance = likesMe(s) ? 1.f : 0.15f + c / 160.f + skillLevel(me, SK_CHARISMA) * 0.02f + skillLevel(me, SK_STYLE) * 0.02f + fameStars() * 0.04f + (compliment ? 0.15f : 0.f);
    rel(G.mc, s).met = true;
    if (frand() < chance) { doMatch(s, compliment); return true; }
    return false;
}

// ------------------------------------------------------------------ layout
Btn tabB(int i) { return Btn{4 + i * 78.f, 22, 74, 24}; }
const Btn bPass{6, 150, 96, 34}, bLike{218, 150, 96, 34}, bComp{106, 150, 108, 34}, bPhoto{106, 188, 108, 22}, bReport{218, 188, 96, 22};
Btn rowB(int i) { return Btn{6, 50 + i * 26.f, 308, 23}; }
Btn actB(int i) { return Btn{6 + (i % 2) * 156.f, 50 + 4 * 26.f + 4 + (i / 2) * 30.f, 152, 26}; }
Btn meB(int i) { return Btn{6 + (i % 2) * 156.f, 52 + (i / 2) * 34.f, 152, 30}; }
}   // namespace
int lovebugStyle(int sim) { return (sim >= 0 && sim < (int)G.sims.size()) ? txStyle(sim) : 0; }

// ================================================================== hooks
void lovebugTick(float dt) {
    if (g_msgT > 0) g_msgT -= dt;
    if (g_anim > 0) g_anim -= dt * 3;
    if (g_burst > 0) g_burst -= dt;
    if (g_typingT > 0) g_typingT -= dt;
    for (size_t i = 0; i < g_pend.size();) {
        g_pend[i].t -= dt;
        if (g_pend[i].t <= 0) { if (valid(g_pend[i].sim)) { addText(g_pend[i].sim, g_pend[i].text); toast("LOVEBUG: " + G.sims[g_pend[i].sim].name + " sent you a message"); } g_pend.erase(g_pend.begin() + i); }
        else i++;
    }
}
void lovebugHourly(int hour) {
    if (G.sims.size() < 4 || hour < 9) return;
    // someone new likes you
    if (MC().isAdult() && !G.lbDeleted && irand(100) < 22 + fameStars() * 5) {
        auto c = swipeCandidates();
        if (!c.empty()) { int s = c[irand((int)c.size())]; if (!likesMe(s) && !blockedLB(s)) { G.lbLikes.push_back((uint16_t)s); if (G.lbLikes.size() > 12) G.lbLikes.erase(G.lbLikes.begin()); toast("LOVEBUG: someone new liked you! <3"); } }
    }
    // your matches text in their own way
    for (int s : matches()) {
        if (irand(100) >= 6) continue;
        static const char* const L[6][2] = {{"heyyy what are u up to", "omg i just thought of u lol"}, {"sorry i fell asleep 3 days ago", "hey! been busy, wbu"}, {"thinking about you ;)", "so when are you taking me out"},
                                            {"the moon is pretty tonight.", "do you believe in fate?"}, {"is it weird that i practiced texting you", "haha sorry, wrong chat. wait no, right chat. hi"}, {"...", "hey stranger"}};
        int st = txStyle(s);
        if (st == TX_GHOST && irand(3) == 0) {   // ghosted
            rel(G.mc, s).matched = false;
            toast("LOVEBUG: " + G.sims[s].name + " unmatched you. Ghosted!");
            post(s, "deleted a few matches today. not feeling it", -1);
            continue;
        }
        addText(s, L[st][irand(2)]);
        if (st == TX_FAST && irand(2)) addText(s, "??? :(");
    }
    // townies date each other through the app (and post about it)
    if (irand(100) < 20) {
        auto c = swipeCandidates();
        if (c.size() >= 2) {
            int a = c[irand((int)c.size())], b = c[irand((int)c.size())];
            if (a != b && canRomance(a, b)) {
                int k = irand(7);
                if (k <= 1 && partnerOf(a) < 0 && partnerOf(b) < 0) {
                    Rel& r = rel(a, b); r.met = true; r.ro = (int8_t)std::min(100, r.ro + 30); r.fr = (int8_t)std::min(100, r.fr + 20); r.status = RS_DATING;
                    post(a, "met " + G.sims[b].name + " on LOVEBUG and now i have to delete the app. sorry ladies/gentlemen", b);
                } else if (k == 2) post(a, "LOVEBUG date update: they brought their mom. to the date. i'm not joking", -1);
                else if (k == 3) post(a, "found my ex on LOVEBUG. bio says 'loyal'. LOL", -1);
                else if (k == 4) post(ST_ACC_TEA, "two people in Starlight Bay just found out they're dating the SAME person on LOVEBUG. names withheld... for now", -1);
                else if (k == 5) post(a, "deleted my LOVEBUG. ...redownloaded it. don't look at me", -1);
                else post(ST_ACC_TEA, "is " + G.sims[a].name + " on LOVEBUG with TWO profiles?? we have screenshots", a);
            }
        }
    }
}

void lovebugUpdate(const Input& in) {
    if (!MC().isAdult()) return;
    const touchPosition& tp = in.tp;
    if (in.touch) for (int i = 0; i < T_COUNT; i++) if (tabB(i).in(tp)) { g_tab = i; g_sel = 0; sfxTap(); return; }
    if (G.lbDeleted && g_tab != T_ME) { if (in.touch && Btn{60, 120, 200, 40}.in(tp)) { G.lbDeleted = false; say("Welcome back to LOVEBUG!"); post(G.mc, "...i'm back on LOVEBUG. we don't need to talk about it", -1); sfxYay(); } return; }
    switch (g_tab) {
    case T_DISCOVER: {
        auto v = discover();
        if (v.empty()) return;
        g_card = std::clamp(g_card, 0, (int)v.size() - 1);
        int s = v[g_card];
        int act = 0;
        if ((in.down & (KEY_R | KEY_A)) || (in.touch && bLike.in(tp))) act = 1;
        if ((in.down & KEY_L) || (in.touch && bPass.in(tp))) act = -1;
        if (in.touch && bComp.in(tp)) act = 2;
        if ((in.down & KEY_Y) || (in.touch && bPhoto.in(tp))) { g_photo = (g_photo + 1) % 3; sfxTap(); return; }
        if (in.touch && bReport.in(tp)) {
            if (catfish(s)) { G.lbBlock.push_back((uint16_t)s); say("Thanks! That profile was a CATFISH. Removed."); sfxYay(); }
            else { say("Reported... but they seem real. Oops."); sfxNo(); }
            return;
        }
        if (!act) return;
        if (act < 0) { dropLike(s); g_dir = -1; sfxTap(); }
        else {
            g_dir = 1;
            if (catfish(s) && act > 0) { say("They want your bank details. It's a catfish! Reported."); G.lbBlock.push_back((uint16_t)s); sfxNo(); }
            else if (tryLike(s, act == 2)) { say("IT'S A MATCH! <3"); sfxYay(); }
            else { say(act == 2 ? "Compliment sent! No match... yet." : "Liked! Fingers crossed."); sfxTap(); if (act == 2) rel(G.mc, s).ro = (int8_t)std::min(100, rel(G.mc, s).ro + 3); }
        }
        g_anim = 1; g_photo = 0;
        // the card list changes once someone matches, so just keep the index
        if (act < 0 || !rel(G.mc, s).matched) g_card++;
        if (g_card >= (int)discover().size()) g_card = 0;
        break;
    }
    case T_LIKES: {
        if (!in.touch) return;
        for (int i = 0; i < (int)G.lbLikes.size() && i < 6; i++) {
            int s = G.lbLikes[i];
            if (Btn{214, 50 + i * 30.f, 48, 26}.in(tp)) { if (valid(s)) { doMatch(s, false); say("Matched with " + G.sims[s].name + "!"); sfxYay(); } return; }
            if (Btn{266, 50 + i * 30.f, 48, 26}.in(tp)) { dropLike(s); sfxTap(); return; }
        }
        break;
    }
    case T_MATCHES: {
        auto v = matches();
        if (in.down & KEY_UP) g_sel = std::max(0, g_sel - 1);
        if (in.down & KEY_DOWN) g_sel = std::min((int)v.size() - 1, g_sel + 1);
        if (!in.touch || v.empty()) return;
        for (int i = 0; i < 4 && i < (int)v.size(); i++) { int k = std::max(0, g_sel - 3) + i; if (k < (int)v.size() && rowB(i).in(tp)) { g_sel = k; sfxTap(); return; } }
        g_sel = std::clamp(g_sel, 0, (int)v.size() - 1);
        int s = v[g_sel];
        if (actB(0).in(tp)) { phoneOpenThread(s); sfxTap(); return; }
        if (actB(1).in(tp)) { phoneDatePick(s); sfxTap(); return; }
        if (actB(2).in(tp)) {   // unmatch
            rel(G.mc, s).matched = false; changeRel(G.mc, s, -3, -8);
            if (rel(G.mc, s).ro > 30 && irand(2)) post(s, "got unmatched by someone i actually liked. cool. great. awesome", G.mc);
            say("Unmatched " + G.sims[s].name + "."); sfxTap(); return;
        }
        if (actB(3).in(tp)) { G.lbBlock.push_back((uint16_t)s); rel(G.mc, s).matched = false; changeRel(G.mc, s, -10, -15); say("Blocked " + G.sims[s].name + " on LOVEBUG."); sfxTap(); return; }
        break;
    }
    case T_ME: {
        if (!in.touch) return;
        for (int i = 0; i < 6; i++) if (meB(i).in(tp)) {
            sfxTap();
            switch (i) {
            case 0: G.lbIntent = (uint8_t)((G.lbIntent + 1) % 4); break;
            case 1: G.lbFilter = (uint8_t)((G.lbFilter + 1) % 17); g_card = 0; break;
            case 2: {
                G.lbStatus = (uint8_t)((G.lbStatus + 1) % 3);
                int p = partnerOf(G.mc);
                if (valid(p)) { if (G.lbStatus == 1) { changeRel(G.mc, p, 2, 6); thoughtPopup(p, "You set your status to Taken! <3", 2); } else if (G.lbStatus == 2) { changeRel(G.mc, p, -4, -8); thoughtPopup(p, "'It's complicated'?! Since WHEN?", -1); post(ST_ACC_TEA, MC().name + " just changed their LOVEBUG status to 'it's complicated'... " + G.sims[p].name + ", are you okay??", G.mc); } }
                break;
            }
            case 3: G.lbTheme = (uint8_t)((G.lbTheme + 1) % 4); break;
            case 4:
                G.lbDeleted = !G.lbDeleted;
                if (G.lbDeleted) { say("Account deleted. Time for a little you-time."); if (irand(2)) post(G.mc, "deleted LOVEBUG. healing era starts now", -1); }
                else say("Welcome back!");
                break;
            case 5: G.lbLikes.clear(); say("Cleared your likes."); break;
            }
            return;
        }
        break;
    }
    }
}

// ------------------------------------------------------------------ drawing
static RGB themeA() { static const RGB A[4] = {{255, 80, 150}, {150, 90, 240}, {255, 140, 80}, {60, 190, 200}}; return A[G.lbTheme % 4]; }
static RGB themeB() { static const RGB B[4] = {{200, 40, 110}, {90, 50, 180}, {220, 70, 90}, {40, 120, 170}}; return B[G.lbTheme % 4]; }
static void profilePic(int s, float x, float y, float w, float h, float t) {
    static const RGB BG[3][2] = {{{255, 220, 232}, {255, 190, 214}}, {{210, 230, 255}, {180, 200, 250}}, {{255, 240, 200}, {250, 210, 160}}};
    grad(x, y, w, h, C(BG[g_photo % 3][0]), C(BG[g_photo % 3][1]));
    if (g_photo == 1) for (int i = 0; i < 6; i++) sparkle(x + 10 + i * 20, y + 10 + (i % 2) * 14, 3, C(255, 255, 255));
    if (g_photo == 2) { rect(x, y + h - 26, w, 26, C(120, 190, 230)); circle(x + w - 20, y + 18, 9, C(255, 220, 120)); }
    if (isCast(s)) {
        int oc = -1; for (int i = 0; i < OC_COUNT; i++) if (ocSim(i) == s) oc = i;
        if (oc >= 0) { float sc = g_photo == 0 ? 0.72f : g_photo == 1 ? 1.3f : 0.62f; float yy = g_photo == 1 ? y - 6 : y + h - 196 * sc; ocDraw(oc, x + w / 2 - 48 * sc, yy, sc); return; }
    }
    if (g_photo == 1) drawSimPortrait(G.sims[s], x + w / 2, y + h / 2, h * 0.42f, t);
    else drawSim(G.sims[s], x + w / 2, y + h - 6, 0.95f, t, 0, g_photo == 2 ? PO_WAVE : PO_IDLE, g_photo == 2);
}
void lovebugDrawTop(float t) {
    grad(0, 0, TOP_W, TOP_H, C(themeA()), C(themeB()));
    for (int i = 0; i < 10; i++) heart(std::fmod(i * 47.f + t * 8, 400.f), 30 + (i * 53 % 200), 4 + i % 3, C(255, 255, 255, 50));
    textOutline("LOVEBUG", 10, 0, 0.66f, C(255, 255, 255), C(themeB()), 0, 1.0f);
    heart(118, 12, 6, C(255, 255, 255)); circle(118, 4, 1.4f, C(255, 255, 255));
    char b[128];
    snprintf(b, sizeof b, "%s   |   %d matches all-time", TABN[g_tab], (int)G.lbMatches);
    text(b, 392, 5, 0.38f, C(255, 255, 255), 2);
    if (!MC().isAdult()) { textOutline("LOVEBUG is for Young Adults and up!", 200, 110, 0.55f, C(255, 255, 255), C(themeB()), 1); return; }
    if (G.lbDeleted) { textOutline("You deleted LOVEBUG.", 200, 90, 0.6f, C(255, 255, 255), C(themeB()), 1); text("Healing era. Proud of you. (Or tap 'Come back' below.)", 200, 120, 0.42f, C(255, 255, 255), 1); return; }
    switch (g_tab) {
    case T_DISCOVER: {
        auto v = discover();
        if (v.empty()) { textOutline("No more profiles nearby!", 200, 100, 0.6f, C(255, 255, 255), C(themeB()), 1); text(G.lbFilter ? "Try clearing your interest filter in My Profile." : "Meet more people around town and check back.", 200, 128, 0.42f, C(255, 255, 255), 1); return; }
        g_card = std::clamp(g_card, 0, (int)v.size() - 1);
        int s = v[g_card];
        const Sim& o = G.sims[s];
        float off = g_anim > 0 ? g_dir * (1 - g_anim) * 260 : 0;
        float x0 = 24 + off;
        rrect(x0, 26, 352, 210, 14, C(255, 255, 255));
        profilePic(s, x0 + 8, 34, 130, 194, t);
        for (int i = 0; i < 3; i++) circle(x0 + 58 + i * 14, 222, 3, C(i == g_photo ? RGB{255, 255, 255} : RGB{255, 255, 255}, i == g_photo ? 255 : 110));
        float x = x0 + 148, y = 34;
        snprintf(b, sizeof b, "%s, %d", o.name.c_str(), ageOf(s));
        text(b, x, y, 0.6f, C(UI_INK));
        if (isCast(s)) { sparkle(x + textW(b, 0.6f) + 10, y + 10, 5, C(60, 160, 250)); text("verified", x + textW(b, 0.6f) + 18, y + 6, 0.32f, C(60, 160, 250)); }
        if (likesMe(s)) text("LIKES YOU!", x + 196, y + 2, 0.36f, C(themeB()), 2);
        y += 24;
        std::string job = o.career != CR_NONE ? CAREERS[o.career].titles[o.careerLevel % 5] : (o.enrolled ? "college student" : "between jobs (vibing)");
        snprintf(b, sizeof b, "%s  -  %s", AESTHETIC[aesthetic(s)], job.c_str()); text(b, x, y, 0.38f, C(150, 90, 150)); y += 16;
        snprintf(b, sizeof b, "Into: %s, %s", HOBBY[hobbyA(s)], HOBBY[hobbyB(s)]); text(b, x, y, 0.38f, C(UI_INK)); y += 15;
        snprintf(b, sizeof b, "Music: %s", MUSIC[hsh(s + 2) % 8]); text(b, x, y, 0.38f, C(UI_INK)); y += 15;
        snprintf(b, sizeof b, "Looking for: %s", INTENT[intent(s)]); text(b, x, y, 0.38f, C(UI_INK)); y += 15;
        std::string tr; for (uint8_t k : o.traits) if (k != TR_NONE) { if (!tr.empty()) tr += ", "; tr += TRAIT_NAMES[k]; }
        text(tr.c_str(), x, y, 0.36f, C(150, 130, 150)); y += 17;
        auto ls = wrapText(std::string("\"") + (catfish(s) ? CATFISH_BIO : BIOS[hsh(s + 9) % 12]) + "\"", 0.4f, 208);
        for (size_t k = 0; k < ls.size() && k < 3; k++) { text(ls[k].c_str(), x, y, 0.4f, C(themeB())); y += 14; }
        snprintf(b, sizeof b, "Texting style: %s", STYLE_N[txStyle(s)]); text(b, x, 196, 0.34f, C(150, 130, 150));
        int c = compat(s);
        rrect(x, 210, 200, 12, 6, C(240, 230, 240)); rrect(x, 210, 2.f * c, 12, 6, C(c >= 70 ? RGB{255, 80, 150} : c >= 45 ? RGB{255, 170, 60} : RGB{170, 170, 190}));
        snprintf(b, sizeof b, "%d%% compatible", c); text(b, x + 100, 209, 0.34f, C(UI_INK), 1);
        snprintf(b, sizeof b, "%d / %d", g_card + 1, (int)v.size()); text(b, 370, 228, 0.32f, C(255, 255, 255), 2);
        break;
    }
    case T_LIKES: {
        textOutline("These people liked YOU:", 200, 30, 0.5f, C(255, 255, 255), C(themeB()), 1);
        if (G.lbLikes.empty()) text("Nobody yet... post a cute fit on STATUS!", 200, 110, 0.45f, C(255, 255, 255), 1);
        for (size_t i = 0; i < G.lbLikes.size() && i < 6; i++) {
            int s = G.lbLikes[i]; if (!valid(s)) continue;
            float x = 14 + (i % 3) * 128.f, y = 56 + (i / 3) * 92.f;
            rrect(x, y, 120, 86, 10, C(255, 255, 255));
            drawSimPortrait(G.sims[s], x + 26, y + 30, 20, t);
            text(G.sims[s].name.c_str(), x + 52, y + 12, 0.42f, C(UI_INK));
            snprintf(b, sizeof b, "%d%% match", compat(s)); text(b, x + 52, y + 30, 0.36f, C(themeB()));
            text(AESTHETIC[aesthetic(s)], x + 8, y + 60, 0.34f, C(150, 130, 150));
        }
        break;
    }
    case T_MATCHES: {
        auto v = matches();
        if (v.empty()) { textOutline("No matches yet.", 200, 100, 0.6f, C(255, 255, 255), C(themeB()), 1); text("Go swipe! Your soulmate could be one tap away.", 200, 128, 0.42f, C(255, 255, 255), 1); break; }
        g_sel = std::clamp(g_sel, 0, (int)v.size() - 1);
        int s = v[g_sel];
        rrect(14, 28, 372, 206, 14, C(255, 255, 255));
        profilePic(s, 22, 36, 120, 190, t);
        float x = 154, y = 40;
        text(G.sims[s].name.c_str(), x, y, 0.6f, C(UI_INK)); y += 26;
        snprintf(b, sizeof b, "Stage: %s", stageOf(s)); text(b, x, y, 0.46f, C(themeB())); y += 20;
        const Rel* r = findRel(G.mc, s);
        int ro = r ? r->ro : 0, fr = r ? r->fr : 0;
        text("Chemistry", x, y, 0.36f, C(150, 130, 150)); rrect(x + 70, y + 2, 140, 10, 5, C(240, 230, 240)); rrect(x + 70, y + 2, 1.4f * std::max(0, ro), 10, 5, C(255, 80, 150)); y += 16;
        text("Friendship", x, y, 0.36f, C(150, 130, 150)); rrect(x + 70, y + 2, 140, 10, 5, C(240, 230, 240)); rrect(x + 70, y + 2, 1.4f * std::max(0, fr), 10, 5, C(80, 190, 160)); y += 20;
        snprintf(b, sizeof b, "%d%% compatible  -  %s", compat(s), STYLE_N[txStyle(s)]); text(b, x, y, 0.38f, C(UI_INK)); y += 18;
        const ChatThread* th = nullptr; for (const ChatThread& c : G.threads) if (c.sim == s) th = &c;
        if (th && !th->msgs.empty()) {
            const Msg& m = th->msgs.back();
            text(m.me ? "You:" : "Them:", x, y, 0.36f, C(150, 130, 150)); y += 13;
            auto ls = wrapText(m.text, 0.4f, 220); for (size_t k = 0; k < ls.size() && k < 2; k++) { text(ls[k].c_str(), x, y, 0.4f, C(UI_INK)); y += 13; }
            text(m.me ? (th->unread ? "Delivered" : "Seen") : "", x + 220, y, 0.32f, C(150, 130, 150), 2);
        }
        if (g_typingT > 0 && g_typingSim == s) { float k = std::fmod(t * 3, 3.f); for (int i = 0; i < 3; i++) circle(x + 10 + i * 10, 214 - (i == (int)k ? 3 : 0), 3, C(200, 180, 200)); text("typing...", x + 40, 206, 0.34f, C(150, 130, 150)); }
        break;
    }
    case T_ME: {
        rrect(14, 28, 372, 206, 14, C(255, 255, 255));
        drawSim(MC(), 80, 220, 1.0f, t, 0, PO_IDLE, false);
        float x = 150, y = 40;
        snprintf(b, sizeof b, "%s, %d", MC().name.c_str(), ageOf(G.mc)); text(b, x, y, 0.6f, C(UI_INK)); y += 26;
        snprintf(b, sizeof b, "%s  -  into %s", AESTHETIC[aesthetic(G.mc)], HOBBY[myHobby()]); text(b, x, y, 0.4f, C(150, 90, 150)); y += 18;
        static const char* const ST[3] = {"Single", "Taken", "It's complicated"};
        snprintf(b, sizeof b, "Status: %s", ST[G.lbStatus % 3]); text(b, x, y, 0.4f, C(UI_INK)); y += 16;
        snprintf(b, sizeof b, "Looking for: %s", INTENT[G.lbIntent % 4]); text(b, x, y, 0.4f, C(UI_INK)); y += 16;
        snprintf(b, sizeof b, "Filter: %s", G.lbFilter ? HOBBY[(G.lbFilter - 1) % 16] : "everyone"); text(b, x, y, 0.4f, C(UI_INK)); y += 16;
        snprintf(b, sizeof b, "Likes waiting: %d   Matches now: %d", (int)G.lbLikes.size(), (int)matches().size()); text(b, x, y, 0.4f, C(UI_INK)); y += 22;
        text("Verified: profile photo matches your face. Wow.", x, y, 0.34f, C(60, 160, 250));
        break;
    }
    }
    if (g_burst > 0) { for (int i = 0; i < 16; i++) { float a = i * 0.3927f, r = (1.5f - g_burst) * 140; heart(200 + std::cos(a) * r, 120 + std::sin(a) * r, 6, C(255, 255, 255, (uint8_t)(200 * g_burst / 1.5f))); } textOutline("IT'S A MATCH!", 200, 100, 1.0f * std::min(1.f, (1.5f - g_burst) * 4), C(255, 255, 255), C(themeB()), 1, 1.2f); }
}
void lovebugDrawBottom(float t) {
    (void)t;
    { RGB d = themeB(); d.r = (uint8_t)(d.r * 7 / 10); d.g = (uint8_t)(d.g * 7 / 10); d.b = (uint8_t)(d.b * 7 / 10); grad(0, 0, 320, 240, C(themeB()), C(d)); }
    for (int i = 0; i < T_COUNT; i++) { char b[32]; if (i == T_LIKES && !G.lbLikes.empty()) snprintf(b, sizeof b, "Likes (%d)", (int)G.lbLikes.size()); else snprintf(b, sizeof b, "%s", TABN[i]); button(tabB(i), b, g_tab == i, 0, 0.36f); }
    if (!MC().isAdult()) return;
    if (G.lbDeleted && g_tab != T_ME) { button(Btn{60, 120, 200, 40}, "Come back to LOVEBUG", false, 0, 0.48f); return; }
    char b[96];
    switch (g_tab) {
    case T_DISCOVER:
        if (discover().empty()) { text("You've seen everyone!", 160, 100, 0.5f, C(255, 255, 255), 1); break; }
        heart(160, 100, 26 + std::sin(t * 4) * 3, C(255, 255, 255, 110));
        text("L = pass   R = like   Y = next photo", 160, 66, 0.38f, C(255, 255, 255), 1);
        button(bPass, "Pass", false, 1, 0.5f); button(bComp, "Compliment", false, 3, 0.44f); button(bLike, "Like <3", false, 0, 0.5f);
        button(bPhoto, "Next Photo", false, 2, 0.38f); button(bReport, "Report", false, 2, 0.38f);
        break;
    case T_LIKES:
        if (G.lbLikes.empty()) { text("No likes waiting.", 160, 100, 0.5f, C(255, 255, 255), 1); break; }
        for (size_t i = 0; i < G.lbLikes.size() && i < 6; i++) {
            int s = G.lbLikes[i]; if (!valid(s)) continue;
            flashPanel(6, 50 + i * 30.f, 204, 26, RGB{255, 255, 255}, 6, 235);
            snprintf(b, sizeof b, "%s (%d%%)", G.sims[s].name.c_str(), compat(s)); text(b, 14, 54 + i * 30.f, 0.42f, C(UI_INK));
            button(Btn{214, 50 + i * 30.f, 48, 26}, "<3", false, 0, 0.42f); button(Btn{266, 50 + i * 30.f, 48, 26}, "X", false, 1, 0.42f);
        }
        break;
    case T_MATCHES: {
        auto v = matches();
        if (v.empty()) { text("No matches yet!", 160, 100, 0.5f, C(255, 255, 255), 1); break; }
        int start = std::max(0, g_sel - 3);
        for (int i = 0; i < 4 && start + i < (int)v.size(); i++) {
            int s = v[start + i];
            bool unread = false; for (const ChatThread& c : G.threads) if (c.sim == s && c.unread) unread = true;
            snprintf(b, sizeof b, "%s  -  %s%s", G.sims[s].name.c_str(), stageOf(s), unread ? "  (new!)" : "");
            button(rowB(i), b, start + i == g_sel, unread ? 0 : 3, 0.4f);
        }
        button(actB(0), "Message", false, 0, 0.44f); button(actB(1), "Plan a Date", false, 2, 0.44f);
        button(actB(2), "Unmatch", false, 1, 0.44f); button(actB(3), "Block", false, 1, 0.44f);
        break;
    }
    case T_ME: {
        static const char* const ST[3] = {"Single", "Taken", "It's complicated"};
        const char* L[6] = {"Change: Looking for", "Change: Interest filter", "", "Change: App theme", G.lbDeleted ? "Restore account" : "Delete account", "Clear likes"};
        std::string st = std::string("Status: ") + ST[G.lbStatus % 3];
        for (int i = 0; i < 6; i++) button(meB(i), i == 2 ? st.c_str() : L[i], false, i == 4 ? 1 : i % 4, 0.38f);
        break;
    }
    }
    if (g_msgT > 0 && !g_msg.empty()) { flashPanel(10, 214 - 30, 300, 22, RGB{255, 255, 255}, 6, 240); text(g_msg.c_str(), 160, 187, 0.4f, C(themeB()), 1); }
}
