// Love life for the player (see romance.hpp). Teen-rated: flirting, dates, kisses and drama only.
#include "romance.hpp"
#include "lifeplus.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "social.hpp"
#include "goals.hpp"
#include "status.hpp"
#include "audio.hpp"
#include "drama.hpp"
#include "extras.hpp"
#include <string>
#include <algorithm>

void toast(const std::string& m);
int lifeCompat(int a, int b);
bool lifeSimHere(int s);
ChatThread& threadFor(int sim);

static LoveState& LV() { return G.love; }
static bool valid(int s) { return s >= 0 && s < (int)G.sims.size() && s != G.mc; }
static const std::string& nm(int s) { return G.sims[s].name; }
static bool romantic(int id) {
    return id == SO_FLIRT || id == SO_WINK || id == SO_SWEET || id == SO_HOLDHANDS || id == SO_KISS || id == SO_SLOWDANCE ||
           id == SO_SNUGGLE || id == SO_SERENADE || id == SO_LOVENOTE || id == SO_MESSAROUND || id == SO_SITU;
}
static void addEx(int s) {
    LoveState& L = LV();
    for (size_t i = 0; i < L.exes.size(); i++) if (L.exes[i] == s) { L.exDay[i] = G.day; return; }
    if (L.exes.size() >= 12) { L.exes.erase(L.exes.begin()); L.exDay.erase(L.exDay.begin()); }
    L.exes.push_back((uint16_t)s); L.exDay.push_back(G.day);
}
static void removeEx(int s) {
    LoveState& L = LV();
    for (size_t i = 0; i < L.exes.size(); i++) if (L.exes[i] == s) { L.exes.erase(L.exes.begin() + i); L.exDay.erase(L.exDay.begin() + i); return; }
}
static void endItWith(int p, bool theyDumped, bool cheated) {
    Rel& r = rel(G.mc, p);
    bool wasMarried = r.status == RS_MARRIED;
    r.status = RS_NONE; r.ro = (int8_t)std::min<int>(r.ro, 0); r.fr = (int8_t)std::max(-100, r.fr - 20);
    addEx(p);
    if (LV().situ == p) LV().situ = -1;
    LV().trust = 70; LV().spark = 60;
    if (theyDumped) {
        addMoodlet(MC(), wasMarried ? "Divorced" : "Got Dumped", EM_SAD, 3, 2880);
        statusPostKind(p, SP_BREAKUP, cheated ? "loyalty is dead. i'm done." : "some chapters just end", G.mc);
    }
}

int romanceCompat(int t) { return valid(t) ? lifeCompat(G.mc, t) + zodiacCompat(MC().zodiac, G.sims[t].zodiac) : 0; }
const char* romanceVibe(int t) {
    int c = romanceCompat(t);
    return c >= 6 ? "Soulmates?!" : c >= 3 ? "Great chemistry" : c >= 0 ? "Could work" : c >= -2 ? "Rocky vibes" : "Total opposites";
}

// ---------------------------------------------------------------- moments
static std::string mPostBreakup(int ch, int t) {
    Sim& me = MC();
    if (ch == 0) { addMoodlet(me, "Ice Cream & Sad Songs", EM_SAD, 1, 600); me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 20); return "You cried, you ate a whole pint, you feel a tiny bit better."; }
    if (ch == 1) { addMoodlet(me, "Glow-Up Era", EM_CONFIDENT, 3, 1440); skillGain(me, SK_STYLE, 3); return "New hair energy. New you. They're going to regret EVERYTHING."; }
    if (valid(t)) { statusPostKind(G.mc, SP_SUBTWEET, "some people really show their true colors huh", t); changeRel(G.mc, t, -15, 0); }
    addMoodlet(me, "Petty & Proud", EM_PLAYFUL, 1, 300);
    return "Posted. The comments are going CRAZY.";
}
static std::string mFirstDate(int ch, int t) {
    if (!valid(t)) return "";
    int ro = rel(G.mc, t).ro, c = romanceCompat(t);
    if (ch == 0) {
        if (ro >= 25 || c >= 3) { changeRel(G.mc, t, 3, 10); addMoodlet(MC(), "First Date Butterflies", EM_FLIRTY, 2, 600); addMemoryOnce(MEM_LOVE, G.mc, t, "First date with " + nm(t) + " - and a goodnight kiss!"); addSentiment(t, SN_BESTMEM, 10); return "A goodnight kiss on the doorstep. Butterflies EVERYWHERE."; }
        changeRel(G.mc, t, -2, -6); addMoodlet(MC(), "Awkward Lean-In", EM_TENSE, 1, 240); return "You leaned in... they went for a hug. Oof.";
    }
    if (ch == 1) { changeRel(G.mc, t, 4, 3); ChatThread& th = threadFor(t); th.msgs.push_back({false, "home safe! that was really fun :)"}); th.unread = true; return "A hug and a \"text me when you get home.\" Cute and safe."; }
    changeRel(G.mc, t, 0, -15); return "You let them down gently. Not every spark catches.";
}
static std::string mSitu(int ch, int t) {
    LoveState& L = LV();
    if (!valid(t)) { L.situ = -1; return ""; }
    if (ch == 0) {
        if (rel(G.mc, t).ro >= 40 && partnerOf(G.mc) < 0 && partnerOf(t) < 0) { rel(G.mc, t).status = RS_DATING; L.situ = -1; addMoodlet(MC(), "It's Official!", EM_HAPPY, 2, 1440); statusPostKind(t, SP_COUPLE, "ok fine. we're official :)", G.mc); return nm(t) + " smiled SO big. You're officially together!"; }
        changeRel(G.mc, t, 0, -8); L.situ = -1; addEx(t); addMoodlet(MC(), "Situationship Over", EM_SAD, 2, 600); return nm(t) + " said they're \"not looking for anything serious.\" Ouch.";
    }
    if (ch == 1) {
        if (++L.situDodges >= 3) { L.situ = -1; addEx(t); changeRel(G.mc, t, -5, -10); return nm(t) + " got tired of waiting and stopped texting back. Ghosted."; }
        changeRel(G.mc, t, 0, -3); L.situDay = G.day; return "\"Let's not put a label on it.\" " + nm(t) + " looks a little hurt.";
    }
    L.situ = -1; addEx(t); changeRel(G.mc, t, -3, -12); return "You ended it. Clean break... mostly.";
}
static std::string mCaught(int ch, int p) {
    LoveState& L = LV();
    if (!valid(p)) return "";
    Sim& me = MC();
    if (ch == 0) { L.trust = (uint8_t)std::max(0, L.trust - 10); changeRel(G.mc, p, -5, -5); addMoodlet(me, "Guilty Conscience", EM_SAD, 2, 720); return "You apologized for real. " + nm(p) + " says they need time... but they're still here."; }
    if (ch == 1) {
        bool lie = irand(100) < (me.hasTrait(TR_SNEAKY) ? 65 : 35);
        if (lie) { addMoodlet(me, "Got Away With It", EM_TENSE, 1, 600); return "\"It was NOTHING.\" " + nm(p) + " believes you... for now."; }
        L.trust = (uint8_t)std::max(0, L.trust - 35); changeRel(G.mc, p, -15, -20); addSentiment(p, SN_HURT, 14);
        statusPostKind(ST_ACC_TEA, SP_RUMOR, "CHEATING SCANDAL: " + me.name + " got caught AND lied about it. " + nm(p) + " has the receipts.", G.mc, p);
        if (L.trust < 20) { endItWith(p, true, true); return nm(p) + " pulled up the screenshots. It's over - and the whole town knows."; }
        return "The lie fell apart. " + nm(p) + " is FURIOUS.";
    }
    endItWith(p, false, true); addMoodlet(me, "Messy Breakup", EM_SAD, 2, 1440);
    return "\"Maybe we should just... end it.\" " + nm(p) + " walks out without a word.";
}
static std::string mRival(int ch, int rv) {
    LoveState& L = LV();
    int p = partnerOf(G.mc);
    if (!valid(rv) || !valid(p)) { L.rival = -1; return ""; }
    if (ch == 0) {
        changeRel(G.mc, rv, -20, 0); dramaAddGrudge(rv, 5);
        if (romanceCompat(p) >= 2) { changeRel(G.mc, p, 3, 6); L.rival = -1; return "You told " + nm(rv) + " to back OFF. " + nm(p) + " thought it was kind of hot, honestly."; }
        changeRel(G.mc, p, -3, -4); return "You made a scene. " + nm(p) + " is embarrassed and " + nm(rv) + " is loving the drama.";
    }
    if (ch == 1) { L.spark = (uint8_t)std::min(100, L.spark + 15); changeRel(G.mc, p, 3, 8); L.rival = -1; addMoodlet(MC(), "Main Character Energy", EM_CONFIDENT, 2, 600); return "You turned the romance ALL the way up. " + nm(rv) + " doesn't stand a chance."; }
    if (irand(100) < 70 || L.trust >= 70 || G.sims[p].hasTrait(TR_LOYAL)) { L.trust = (uint8_t)std::min(100, L.trust + 10); L.rival = -1; return "You trust " + nm(p) + ". And they proved you right: they shut " + nm(rv) + " down."; }
    L.trust = (uint8_t)std::max(0, L.trust - 15); changeRel(G.mc, p, -4, -8); return "You stayed cool... but you just saw " + nm(p) + " laughing a LITTLE too hard at " + nm(rv) + "'s jokes.";
}
static std::string mEx(int ch, int ex) {
    if (!valid(ex)) return "";
    int p = partnerOf(G.mc);
    if (ch == 0) {
        if (p >= 0) {
            changeRel(G.mc, ex, 4, 12); addMoodlet(MC(), "Playing With Fire", EM_FLIRTY, 2, 600);
            if (irand(2)) { LV().confront = (int16_t)p; statusPostKind(ST_ACC_TEA, SP_RUMOR, "spotted: " + MC().name + " getting cozy with their EX " + nm(ex) + "... doesn't " + MC().name + " have a partner??", G.mc, ex); }
            return "You and " + nm(ex) + " talked for hours. Old feelings, new problems...";
        }
        if (canRomance(G.mc, ex)) { rel(G.mc, ex).status = RS_DATING; changeRel(G.mc, ex, 8, 20); removeEx(ex); addMoodlet(MC(), "Back Together", EM_HAPPY, 2, 1440); statusPostKind(ex, SP_COUPLE, "back together. don't @ us", G.mc); return "Round two! You and " + nm(ex) + " are back together."; }
        changeRel(G.mc, ex, 6, 0); return "You caught up as friends.";
    }
    if (ch == 1) { changeRel(G.mc, ex, 12, -5); removeEx(ex); addMoodlet(MC(), "Closure", EM_FINE, 1, 600); return "Just friends. It actually feels... nice? Closure unlocked."; }
    changeRel(G.mc, ex, -30, -20); removeEx(ex); addMoodlet(MC(), "Blocked & Free", EM_CONFIDENT, 1, 600); return "Blocked. Deleted. Thriving.";
}
static std::string mSpark(int ch, int p) {
    LoveState& L = LV();
    if (!valid(p)) return "";
    if (ch == 0) { if (G.money < 40) { changeRel(G.mc, p, 0, 2); return "You're broke, so it's a picnic on the floor. Still kinda romantic."; } G.money -= 40; L.spark = (uint8_t)std::min(100, L.spark + 30); changeRel(G.mc, p, 4, 12); L.lastRomDay = G.day; return "Date night! Candles, pasta, slow dancing in the kitchen. The spark is BACK."; }
    if (ch == 1) { L.trust = (uint8_t)std::min(100, L.trust + 5); L.spark = (uint8_t)std::min(100, L.spark + 8); return "You promised to make more time. " + nm(p) + " squeezes your hand."; }
    L.trust = (uint8_t)std::max(0, L.trust - 12); changeRel(G.mc, p, -8, -10); addMoodlet(MC(), "Big Fight", EM_ANGRY, 2, 600);
    if (L.trust < 20 && irand(2)) { endItWith(p, true, false); return "It escalated. " + nm(p) + " says they can't do this anymore. It's over."; }
    return "Doors slammed. Somebody's sleeping on the couch tonight.";
}
static std::string mFight(int ch, int p) {
    if (!valid(p)) return "";
    if (ch == 0) { changeRel(G.mc, p, 3, 4); LV().trust = (uint8_t)std::min(100, LV().trust + 4); return "You actually listened. Different isn't bad - it's just different."; }
    if (ch == 1) { changeRel(G.mc, p, -6, -6); return "You won the argument. You lost the vibe."; }
    changeRel(G.mc, p, 1, 0); addMoodlet(MC(), "Unresolved", EM_TENSE, 1, 480); return "You walked away to cool off. It'll come up again.";
}
static std::string mAnniv(int ch, int p) {
    if (!valid(p)) return "";
    if (ch == 0) { if (G.money < 80) return "Not enough money for the fancy place - you cook together instead. Still perfect."; G.money -= 80; changeRel(G.mc, p, 5, 15); LV().spark = 100; addMemoryOnce(MEM_LOVE, G.mc, p, "Anniversary dinner with " + nm(p) + "!"); return "Rooftop dinner, sparklers on the dessert. Happy anniversary!"; }
    if (ch == 1) { changeRel(G.mc, p, 4, 9); LV().spark = (uint8_t)std::min(100, LV().spark + 20); return "A handmade scrapbook of your favorite memories. " + nm(p) + " cried (happy tears)."; }
    changeRel(G.mc, p, -6, -12); LV().trust = (uint8_t)std::max(0, LV().trust - 8); return "You... forgot. " + nm(p) + " did not.";
}
static std::string mVows(int ch, int p) {
    if (!valid(p)) return "";
    if (ch == 0) { changeRel(G.mc, p, 4, 12); return "Your vows made the whole chapel go \"awww.\""; }
    if (ch == 1) { changeRel(G.mc, p, 8, 6); return "Everyone was crying laughing. Best wedding ever."; }
    changeRel(G.mc, p, 4, 14); addMoodlet(MC(), "Ugly-Cried at the Altar", EM_HAPPY, 2, 600); return "You couldn't get a single word out. It was perfect.";
}
static std::string mConfront(int ch, int p) { return mCaught(ch, p); }

// ---------------------------------------------------------------- hooks
void romanceAfterSocial(int id, int t, bool) {
    if (!valid(t) || G.sims.empty()) return;
    LoveState& L = LV();
    Sim& me = MC();
    const Rel& r = rel(G.mc, t);
    int p = partnerOf(G.mc);
    if (romantic(id)) {
        if (t == p) { L.lastRomDay = G.day; L.spark = (uint8_t)std::min(100, L.spark + 4); }
        // compatibility: chemistry or no spark
        int c = romanceCompat(t);
        if (id == SO_FLIRT || id == SO_WINK) {
            if (c >= 5 && irand(3) == 0) { changeRel(G.mc, t, 1, 4); toast("Chemistry! You and " + nm(t) + " just click."); }
            else if (c <= -2 && irand(3) == 0) { changeRel(G.mc, t, 0, -3); toast("No spark... you and " + nm(t) + " are total opposites."); }
        }
        // cheating
        if (p >= 0 && t != p && !momentActive()) {
            bool seen = lifeSimHere(p);
            int odds = id == SO_KISS || id == SO_MESSAROUND ? 45 : 18;
            if (seen) {
                L.trust = (uint8_t)std::max(0, L.trust - 20); changeRel(G.mc, p, -10, -15); addSentiment(p, SN_HURT, 10);
                momentCustom("Caught!", nm(p) + " saw EVERYTHING. They're standing right there, arms crossed. \"Explain. Now.\"",
                             "Apologize for real", "Deny it", "Maybe we're done", mCaught, p, p);
            } else if (irand(100) < odds) {
                L.confront = (int16_t)p;
                statusPostKind(ST_ACC_TEA, SP_RUMOR, "spotted: " + me.name + " flirting with " + nm(t) + "... isn't " + me.name + " with " + nm(p) + "??", G.mc, t);
                toast("Uh oh... someone posted about you and " + nm(t) + " on STATUS.");
            }
        }
    }
    switch (id) {
    case SO_VIBES: {
        int c = romanceCompat(t);
        std::string h; for (int i = 0; i < 5; i++) h += i < std::max(0, std::min(5, (c + 3) * 5 / 11)) ? "<3 " : "- ";
        { int sn = sentimentWith(t); toast("Vibe check with " + nm(t) + " (" + ZODIAC_NAMES[G.sims[t].zodiac % 12] + "): " + romanceVibe(t) + "  " + h + (sn >= 0 ? std::string("  Feeling: ") + sentimentName(sn) : "")); }
        break;
    }
    case SO_SITU:
        if (L.situ < 0 || L.situ == t) { L.situ = (int16_t)t; L.situDay = G.day; L.situDodges = 0; toast("You and " + nm(t) + " are... a thing? Kinda? It's a situationship."); }
        break;
    case SO_DTR:
        if (L.situ == t) momentCustom("What Are We?", "You and " + nm(t) + " have been talking for a while. It's time to define the relationship...", "Let's make it official", "Keep it casual", "End it", mSitu, t, t);
        break;
    case SO_GOSTEADY: if (r.status == RS_DATING && L.situ == t) L.situ = -1; if (r.status == RS_DATING) { removeEx(t); L.trust = std::max<uint8_t>(L.trust, 60); L.spark = 70; } break;
    case SO_BREAKUP:
        addEx(t); if (L.situ == t) L.situ = -1; L.trust = 70; L.spark = 60; addSentiment(t, SN_HURT, 10);
        statusPostKind(t, SP_BREAKUP, "so that happened. single now i guess", G.mc);
        momentCustom("After the Breakup", "It's over with " + nm(t) + ". How are you coping?", "Ice cream & sad songs", "Glow-up era", "Subtweet them", mPostBreakup, t, -1);
        break;
    case SO_PROPOSE:
        if (r.status == RS_ENGAGED) {
            statusAnnounce(me.name + " just PROPOSED to " + nm(t) + " and the answer was YES!!! wedding when??", G.mc);
            Event e; e.day = (uint16_t)(G.day + 3); e.hour = 14; e.type = EV_PLAN; e.loc = R_CHAPEL; e.sim = (int16_t)t; e.text = "Your wedding with " + nm(t) + " at Lovebird Chapel!";
            G.events.push_back(e);
            toast("Wedding planned at Lovebird Chapel in 3 days! (Or say \"I do\" anytime.)");
        }
        break;
    case SO_MARRY:
        L.annivDay = G.day; L.trust = std::max<uint8_t>(L.trust, 80); L.spark = 90; removeEx(t); addSentiment(t, SN_BESTMEM, 28);
        statusAnnounce(me.name + " & " + nm(t) + " are MARRIED!!! congrats to the happy couple", G.mc);
        momentCustom("Your Vows", "Everyone's watching. " + nm(t) + " is holding your hands. What do you say?", "Sweet & heartfelt", "Make everyone laugh", "Ugly-cry through it", mVows, t, t);
        break;
    }
}

void romanceAfterDate(int t, float score) {
    if (!valid(t)) return;
    LoveState& L = LV();
    if (std::find(L.dated.begin(), L.dated.end(), (uint16_t)t) != L.dated.end()) return;
    if (L.dated.size() >= 40) L.dated.erase(L.dated.begin());
    L.dated.push_back((uint16_t)t);
    if (momentActive()) return;
    std::string how = score >= 55 ? "The date was AMAZING." : score >= 25 ? "The date was... fine?" : "The date was kind of a disaster.";
    momentCustom("First Date!", how + " Now you're at the door with " + nm(t) + ". Vibe: " + romanceVibe(t) + ".", "Kiss goodnight", "Hug & text later", "Not feeling it", mFirstDate, t, t);
}

void romanceHourly(int hour) {
    LoveState& L = LV();
    if (G.sims.empty() || !MC().isAdult()) return;
    if (L.situ >= 0 && !valid(L.situ)) L.situ = -1;
    if (L.rival >= 0 && !valid(L.rival)) L.rival = -1;
    if (L.confront >= 0 && (!valid(L.confront) || partnerOf(G.mc) != L.confront)) L.confront = -1;
    if (hour != 10 && hour != 19) return;
    if (momentActive() || L.evDay == G.day) return;
    int p = partnerOf(G.mc);
    // 1. someone told your partner
    if (L.confront >= 0 && p == L.confront) {
        L.evDay = G.day; int c = L.confront; L.confront = -1;
        L.trust = (uint8_t)std::max(0, L.trust - 15);
        momentCustom("We Need to Talk", nm(c) + " saw the STATUS post. \"Is it true?\"", "Apologize for real", "Deny it", "Maybe we're done", mConfront, c, c);
        return;
    }
    // 2. situationship reaches "what are we?"
    if (L.situ >= 0 && G.day - L.situDay >= 4 && irand(2) == 0) {
        L.evDay = G.day;
        momentCustom("What Are We?", nm(L.situ) + " texts: \"so... what ARE we exactly?\"", "Let's make it official", "Keep it casual", "End it", mSitu, L.situ, L.situ);
        return;
    }
    if (hour != 19) return;
    if (p >= 0) {
        const Rel& r = rel(G.mc, p);
        int c = romanceCompat(p);
        // 3. anniversary (every 4 weeks of marriage)
        if (r.status == RS_MARRIED && G.day > L.annivDay && (G.day - L.annivDay) % 28 == 0) { L.evDay = G.day; momentCustom("Anniversary!", "It's your anniversary with " + nm(p) + "!", "Fancy dinner ($80)", "Handmade gift", "...was that today?", mAnniv, p, p); return; }
        // 4. the spark fades if you never make time
        if (G.day - L.lastRomDay > 3 && L.spark > 0) L.spark = (uint8_t)std::max(0, L.spark - 6);
        if (L.spark < 25 && irand(2) == 0) { L.evDay = G.day; momentCustom("Losing the Spark", nm(p) + ": \"We never spend time together anymore.\"", "Plan date night ($40)", "Promise to do better", "Snap back", mSpark, p, p); return; }
        // 5. compatibility consequences
        if (c <= -2 && irand(4) == 0) { L.evDay = G.day; momentCustom("Couple Fight", "You and " + nm(p) + " are fighting about something tiny... again. (Vibe: " + romanceVibe(p) + ")", "Talk it out", "Win the argument", "Walk away", mFight, p, p); return; }
        if (c >= 5 && irand(5) == 0) { L.evDay = G.day; changeRel(G.mc, p, 2, 4); toast(nm(p) + " left you a sweet note. Soulmate energy."); return; }
        // 6. a romantic rival appears
        if (L.rival < 0 && irand(8) == 0) {
            std::vector<int> pool;
            for (size_t i = 0; i < G.sims.size(); i++) if ((int)i != G.mc && (int)i != p && !G.sims[i].household && canRomance((int)i, p) && partnerOf((int)i) < 0) pool.push_back((int)i);
            if (!pool.empty()) {
                L.rival = (int16_t)pool[irand((int)pool.size())]; L.evDay = G.day;
                changeRel(L.rival, p, 5, 15);
                momentCustom("Rival Alert", nm(L.rival) + " keeps flirting with " + nm(p) + " - right in front of you.", "Confront them", "Turn up the romance", "Trust your partner", mRival, L.rival, L.rival);
                return;
            }
        }
        // 7. trust at rock bottom: they leave
        if (L.trust < 15 && irand(3) == 0) { L.evDay = G.day; endItWith(p, true, false); toast(nm(p) + " ended things. The trust was gone."); return; }
    }
    // 8. exes come back around
    for (size_t i = 0; i < L.exes.size(); i++) {
        int ex = L.exes[i];
        if (!valid(ex) || G.day - L.exDay[i] < 7 || irand(10)) continue;
        L.evDay = G.day; L.exDay[i] = G.day;
        momentCustom("Your Ex Is Back", nm(ex) + " is back in town and texted: \"hey stranger. can we talk?\"", "Give it another shot", "Just friends", "Block them", mEx, ex, ex);
        return;
    }
}
