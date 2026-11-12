#include "lockup.hpp"
// See lifeplus.hpp. Original mechanics written for Sugar Bunny Life 2.
#include "lifeplus.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "social.hpp"
#include "goals.hpp"
#include "status.hpp"
#include "audio.hpp"
#include "draw.hpp"
#include "extras.hpp"
#include <cstring>
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>

using namespace col;
void toast(const std::string& m);
bool lifeSimHere(int s);

static bool valid(int s) { return s >= 0 && s < (int)G.sims.size(); }

// ================================================================ favorites & zodiac
const char* FAV_COLOR_NAMES[12] = {"Pink", "Red", "Orange", "Yellow", "Green", "Mint", "Blue", "Purple", "Lilac", "Black", "White", "Brown"};
static const RGB FAV_COLORS[12] = {{255, 140, 190}, {220, 40, 50}, {255, 150, 60}, {250, 214, 60}, {90, 170, 80}, {150, 230, 200}, {70, 120, 220}, {130, 60, 170}, {200, 170, 240}, {30, 30, 34}, {246, 246, 246}, {120, 80, 50}};
const char* ZODIAC_NAMES[12] = {"Aries", "Taurus", "Gemini", "Cancer", "Leo", "Virgo", "Libra", "Scorpio", "Sagittarius", "Capricorn", "Aquarius", "Pisces"};
int zodiacCompat(int a, int b) {
    int ea = a % 4, eb = b % 4;   // 0 fire, 1 earth, 2 air, 3 water
    if (ea == eb) return 2;
    if ((ea == 0 && eb == 2) || (ea == 2 && eb == 0) || (ea == 1 && eb == 3) || (ea == 3 && eb == 1)) return 1;
    return 0;
}
static bool nearColor(RGB a, RGB b) { return std::abs(a.r - b.r) + std::abs(a.g - b.g) + std::abs(a.b - b.b) < 140; }

// ================================================================ genetics
static const Lineage* lineOf(int kid) { for (const Lineage& l : G.lineage) if (l.kid == kid) return &l; return nullptr; }
static bool textured(int hs) {
    const char* n = HAIRSTYLE_NAMES[hs % NUM_HAIRSTYLES];
    static const char* K[] = {"Curl", "Coil", "Afro", "Fro", "Braid", "Locs", "Puff", "Twist", "Fulani", "Cornrow", "Knot"};
    for (const char* k : K) if (strstr(n, k)) return true;
    return false;
}
static int grandparent(int parent) { const Lineage* l = lineOf(parent); if (!l) return -1; int g = irand(2) ? l->pa : l->pb; return valid(g) ? g : -1; }

void geneticsOnBirth(int kid, int pa, int pb) {
    if (!valid(kid)) return;
    Lineage L; L.kid = (uint16_t)kid; L.pa = (int16_t)pa; L.pb = (int16_t)pb;
    G.lineage.push_back(L);
    if (G.lineage.size() > 300) G.lineage.erase(G.lineage.begin());
    if (!valid(pa) || !valid(pb)) return;
    Sim& k = G.sims[kid]; const Sim& A = G.sims[pa]; const Sim& B = G.sims[pb];
    // skin: either parent, or somewhere in between
    int r = irand(10);
    k.look.skin = r < 4 ? A.look.skin : r < 8 ? B.look.skin : (uint8_t)((A.look.skin + B.look.skin + irand(2)) / 2);
    // face features, one at a time, from either parent
    auto pickF = [&](uint8_t a, uint8_t b) { return irand(2) ? a : b; };
    k.look.f.shape = pickF(A.look.f.shape, B.look.f.shape); k.look.f.eyes = pickF(A.look.f.eyes, B.look.f.eyes);
    k.look.f.brows = pickF(A.look.f.brows, B.look.f.brows); k.look.f.nose = pickF(A.look.f.nose, B.look.f.nose);
    k.look.f.lipShape = pickF(A.look.f.lipShape, B.look.f.lipShape);
    k.look.f.marks = ((A.look.f.marks & 1) || (B.look.f.marks & 1)) && irand(2) ? 1 : 0;
    k.look.f.glasses = 0; k.look.f.pierce = 0; k.look.f.shadow = 0; k.look.f.liner = 0; k.look.f.blush = 0;
    // eye & hair colour, with a small chance of a grandparent's "recessive" gene
    int gA = grandparent(pa), gB = grandparent(pb);
    k.look.f.eyeCol = pickF(A.look.f.eyeCol, B.look.f.eyeCol);
    k.look.hairCol = pickF(A.look.hairCol, B.look.hairCol);
    if (A.look.hairCol == 7) k.look.hairCol = B.look.hairCol; else if (B.look.hairCol == 7) k.look.hairCol = A.look.hairCol;   // grey isn't genetic
    if (irand(100) < 15) { int g = gA >= 0 ? gA : gB; if (g >= 0) { k.look.f.eyeCol = G.sims[g].look.f.eyeCol; if (G.sims[g].look.hairCol != 7) k.look.hairCol = G.sims[g].look.hairCol; toast("Surprise! " + k.name + " got Grandma & Grandpa's genes!"); } }
    toast(geneticsBlurb(kid));
}

std::string geneticsBlurb(int kid) {
    const Lineage* l = lineOf(kid);
    if (!valid(kid) || !l || !valid(l->pa) || !valid(l->pb)) return valid(kid) ? G.sims[kid].name + " is one of a kind!" : "";
    const Sim& k = G.sims[kid]; const Sim& A = G.sims[l->pa]; const Sim& B = G.sims[l->pb];
    std::vector<std::string> a, b;
    (k.look.f.eyeCol == A.look.f.eyeCol ? a : b).push_back("eyes");
    (k.look.skin == A.look.skin ? a : k.look.skin == B.look.skin ? b : a).push_back("skin");
    (k.look.hairCol == A.look.hairCol ? a : b).push_back("hair color");
    (k.look.f.nose == A.look.f.nose ? a : b).push_back("nose");
    (k.look.f.lipShape == A.look.f.lipShape ? a : b).push_back("smile");
    auto join = [](const std::vector<std::string>& v) { std::string s; for (size_t i = 0; i < v.size() && i < 3; i++) { if (i) s += ", "; s += v[i]; } return s; };
    std::string fa = A.name.substr(0, A.name.find(' ')), fb = B.name.substr(0, B.name.find(' '));
    std::string s = k.name.substr(0, k.name.find(' ')) + " has ";
    if (!a.empty()) s += fa + "'s " + join(a);
    if (!a.empty() && !b.empty()) s += " and ";
    if (!b.empty()) s += fb + "'s " + join(b);
    return s + "!";
}

void geneticsOnAgeUp(int kid) {
    const Lineage* l = lineOf(kid);
    if (!valid(kid) || !l) return;
    Sim& k = G.sims[kid];
    int pa = l->pa, pb = l->pb;
    // hair texture runs in the family
    if ((k.stage == ST_TODDLER || k.stage == ST_CHILD || k.stage == ST_TEEN) && (valid(pa) || valid(pb))) {
        bool tex = (valid(pa) && textured(G.sims[pa].look.hairStyle)) || (valid(pb) && textured(G.sims[pb].look.hairStyle));
        if (tex && !textured(k.look.hairStyle)) {
            std::vector<int> opts;
            for (int h = 0; h < NUM_HAIRSTYLES; h++) if (textured(h) && (!k.look.guy || h < 30 || h >= 58)) opts.push_back(h);
            if (!opts.empty()) k.look.hairStyle = (uint8_t)opts[irand((int)opts.size())];
        }
    }
    // personality: a 50% chance any newly-earned trait comes from a parent
    int slots = traitSlotsFor(k.stage);
    if (slots > 0 && irand(2)) {
        std::vector<uint8_t> pool;
        for (int p : {pa, pb}) if (valid(p)) for (int i = 0; i < 3; i++) { uint8_t t = G.sims[p].traits[i]; if (t != TR_NONE && t < TR_COUNT && !k.hasTrait(t)) pool.push_back(t); }
        if (!pool.empty()) {
            uint8_t t = pool[irand((int)pool.size())];
            k.traits[slots - 1] = t;
            const int par = valid(pa) && G.sims[pa].hasTrait(t) ? pa : pb;
            if (k.household && valid(par)) toast(k.name + " is " + TRAIT_NAMES[t] + ", just like " + G.sims[par].name + "!");
        }
    }
}

// ================================================================ sentiments
static const char* SENT_NAMES[SN_COUNT] = {"Best Memories", "Grateful", "Hurt", "Furious", "Embarrassed", "Guilty"};
const char* sentimentName(int k) { return k >= 0 && k < SN_COUNT ? SENT_NAMES[k] : ""; }
static bool positive(int k) { return k == SN_BESTMEM || k == SN_GRATEFUL; }
void addSentiment(int sim, int kind, int days) {
    if (!valid(sim) || sim == G.mc) return;
    for (Sentiment& s : G.sentiments) if (s.sim == sim) { s.kind = (uint8_t)kind; s.until = (uint16_t)(G.day + days); return; }
    if (G.sentiments.size() >= 200) G.sentiments.erase(G.sentiments.begin());
    Sentiment s; s.sim = (uint16_t)sim; s.kind = (uint8_t)kind; s.until = (uint16_t)(G.day + days); G.sentiments.push_back(s);
}
int sentimentWith(int sim) { for (const Sentiment& s : G.sentiments) if (s.sim == sim && s.until >= G.day) return s.kind; return -1; }

// ================================================================ lot traits
bool hasLot(int t) { return (G.lotTraits >> t) & 1; }
float lotCookBonus() { return hasLot(LT_CHEF) ? 1.25f : 1.f; }
static const char* LOT_NAMES[LT_COUNT] = {"Cozy", "Party House", "Chef's Kitchen", "Romantic Aura", "Peaceful", "Book Nook", "Green Thumb", "Haunted", "Gremlins", "Porch Pirates"};
static const char* LOT_DESC[LT_COUNT] = {
    "Everyone rests easier here. Extra energy at home.", "Great for guests: social & fun rise when 3+ are home.",
    "More time on every Chef Bea step + extra cooking XP.", "Flirts, kisses & dates at home land extra sweet.",
    "Calm vibes: a daily good mood and fewer family fights.", "You learn faster here: logic & creativity grow at home.",
    "The morning dew waters your garden for you.", "Spooky stuff at night... thrilling or terrifying!",
    "Things break for no reason. Repairs cost money.", "Packages left on the porch might go missing!"};
static bool lotBad(int t) { return t >= LT_HAUNTED; }

// ================================================================ fights
static int fightTarget = -1;
static int npcMove(int t) {   // 0 swing, 1 dodge, 2 trash talk
    const Sim& s = G.sims[t];
    int w[3] = {3, 3, 3};
    if (s.hasTrait(TR_HOTHEAD)) w[0] += 4; if (s.hasTrait(TR_SNEAKY)) w[1] += 4; if (s.hasTrait(TR_MEAN) || s.hasTrait(TR_SNOB)) w[2] += 4;
    int r = irand(w[0] + w[1] + w[2]);
    return r < w[0] ? 0 : r < w[0] + w[1] ? 1 : 2;
}
static std::string fightResult(int style, int t) {
    if (!valid(t)) return "";
    Sim& me = MC(); Sim& s = G.sims[t];
    static const char* MV[3] = {"swing", "dodge", "trash talk"};
    int mine = 0, theirs = 0; std::string log;
    for (int round = 0; round < 3; round++) {
        int m = round == 0 ? style : (style + (irand(3) == 0 ? 1 : 0)) % 3, o = npcMove(t);
        bool win = (m == 0 && o == 2) || (m == 1 && o == 0) || (m == 2 && o == 1);
        bool tie = m == o;
        if (tie) { int a = skillLevel(me, SK_FITNESS) + irand(6), b = skillLevel(s, SK_FITNESS) + irand(6); win = a >= b; }
        (win ? mine : theirs)++;
        if (round < 2) { log += win ? "W " : "L "; (void)MV; }
    }
    changeRel(G.mc, t, -20, -10);
    std::string place = G.room < R_COUNT ? ROOM_NAMES[G.room] : "town";
    bool public_ = !roomIsHome(G.room);
    brawlStart(G.mc, t, public_ && G.room != R_JAIL);
    if (mine > theirs) {
        addMoodlet(me, "Won the Fight", EM_CONFIDENT, 2, 480); addMoodlet(s, "Lost a Fight", EM_SAD, 2, 480);
        addSentiment(t, SN_EMBARRASSED, 10); skillGain(me, SK_FITNESS, 3);
        if (public_) statusPostKind(ST_ACC_TEA, SP_RUMOR, "FIGHT at " + place + "!!! " + me.name + " vs " + s.name + "... and " + me.name + " WON.", G.mc, t);
        return "You won " + std::to_string(mine) + "-" + std::to_string(theirs) + "! " + s.name + " storms off, totally embarrassed.";
    }
    addMoodlet(me, "Lost a Fight", EM_SAD, 2, 480); addMoodlet(s, "Won the Fight", EM_CONFIDENT, 2, 480);
    addSentiment(t, SN_FURIOUS, 7); me.needs[N_ENERGY] = std::max(0.f, me.needs[N_ENERGY] - 15);
    if (public_) statusPostKind(ST_ACC_TEA, SP_RUMOR, "someone filmed " + me.name + " losing a fight to " + s.name + " at " + place + " LMAOOO", t, G.mc);
    return "You lost " + std::to_string(mine) + "-" + std::to_string(theirs) + ". Dust cloud, messy hair, bruised ego. " + s.name + " won't let you forget it.";
}
static std::string mFight(int ch, int t) { return fightResult(ch, t); }
void startFight(int t) {
    if (!valid(t) || momentActive()) return;
    fightTarget = t;
    momentCustom("FIGHT!", "It's going DOWN with " + G.sims[t].name + "! Pick your fighting style:", "Swing first", "Dodge & counter", "Trash talk", mFight, t, t);
}
static std::string mConfronted(int ch, int t) {
    if (!valid(t)) return "";
    if (ch == 0) return fightResult(irand(3), t);
    if (ch == 1) { addMoodlet(MC(), "Took the High Road", EM_FINE, 1, 300); changeRel(G.mc, t, -3, 0); return "You walked away. " + G.sims[t].name + " yells after you, but it's over... for now."; }
    changeRel(G.mc, t, 12, 0); addSentiment(t, SN_GRATEFUL, 4); return "\"I'm sorry. Really.\" " + G.sims[t].name + " lets out a breath. Maybe you can move on.";
}

// ================================================================ hooks
static bool friendly(int id) { return id == SO_CHAT || id == SO_COMPLIMENT || id == SO_JOKE || id == SO_HUG || id == SO_DANCE; }
static bool romanticSo(int id) { return id == SO_FLIRT || id == SO_WINK || id == SO_KISS || id == SO_SWEET || id == SO_HOLDHANDS || id == SO_SNUGGLE || id == SO_SLOWDANCE; }
void lifeplusAfterSocial(int id, int t) {
    if (!valid(t)) return;
    int sn = sentimentWith(t);
    if (friendly(id) && sn >= 0) {
        if (positive(sn)) changeRel(G.mc, t, 2, 0);
        else if (irand(3) == 0) { changeRel(G.mc, t, -2, 0); toast(G.sims[t].name + " is still " + std::string(sn == SN_FURIOUS ? "furious" : sn == SN_HURT ? "hurt" : "embarrassed") + " about what happened."); }
    }
    if (romanticSo(id) && hasLot(LT_ROMANTIC) && roomIsHome(G.room)) changeRel(G.mc, t, 0, 2);
    const Sim& ts = G.sims[t];
    if (friendly(id) && (MC().hasTrait(TR_GENEROUS) || ts.hasTrait(TR_INSECURE))) changeRel(G.mc, t, 2, 0);
    bool meanSo = id == SO_INSULT || id == SO_MOCKOUTFIT || id == SO_EYEROLL || id == SO_BACKHANDED || id == SO_CALLOUT || id == SO_RUMOR;
    if (meanSo && ts.hasTrait(TR_INSECURE)) { changeRel(G.mc, t, -4, 0); addSentiment(t, SN_HURT, 5); }
    if (meanSo && MC().hasTrait(TR_DRAMATIC) && irand(4) == 0) toast("Of COURSE it turned into a whole scene. Dramatic much?");
    if (id == SO_FIGHT) startFight(t);
    if (id == SO_APOLOGIZE && sn >= SN_HURT) { G.sentiments.erase(std::remove_if(G.sentiments.begin(), G.sentiments.end(), [&](const Sentiment& s) { return s.sim == t; }), G.sentiments.end()); toast("You cleared the air with " + G.sims[t].name + "."); }
}

void lifeplusHourly(int hour) {
    if (G.sims.empty()) return;
    Sim& me = MC();
    // optimists shake off sadness; favorite color makes you smile
    if (me.hasTrait(TR_OPTIMIST) && irand(4) == 0) me.moodlets.erase(std::remove_if(me.moodlets.begin(), me.moodlets.end(), [](const Moodlet& m) { return m.emotion == EM_SAD && m.power <= 2; }), me.moodlets.end());
    if (hour == 8) {
        RGB top = me.outfit > 0 && me.outfit < NUM_OUTFITS ? OUTFITS[me.outfit].top : me.look.top;
        if (nearColor(top, FAV_COLORS[me.favColor % 12])) { std::string mm = std::string("Wearing My Color (") + FAV_COLOR_NAMES[me.favColor % 12] + ")"; addMoodlet(me, mm.c_str(), EM_CONFIDENT, 1, 480); }
    }
    // sentiments wear off
    G.sentiments.erase(std::remove_if(G.sentiments.begin(), G.sentiments.end(), [](const Sentiment& s) { return s.until < G.day || s.sim >= G.sims.size(); }), G.sentiments.end());
    // someone furious with you comes at you
    if (!momentActive() && hour >= 9 && hour <= 21 && irand(me.hasTrait(TR_DRAMATIC) ? 3 : 6) == 0)
        for (const Sentiment& s : G.sentiments)
            if (s.kind == SN_FURIOUS && lifeSimHere(s.sim) && G.sims[s.sim].stage >= ST_TEEN && me.stage >= ST_TEEN) {
                momentCustom("Confrontation!", G.sims[s.sim].name + " marches right up to you: \"We need to settle this.\"", "Fight back", "Walk away", "Apologize", mConfronted, s.sim, s.sim);
                break;
            }
    // lot traits (only while you're home)
    if (!roomIsHome(G.room)) return;
    int home = 0; for (const Sim& s : G.sims) if (s.household) home++;
    if (hasLot(LT_COZY)) me.needs[N_ENERGY] = std::min(100.f, me.needs[N_ENERGY] + 3);
    if (hasLot(LT_PARTY) && home >= 3) { me.needs[N_SOCIAL] = std::min(100.f, me.needs[N_SOCIAL] + 4); me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 2); }
    if (hasLot(LT_BOOKNOOK)) { skillGain(me, SK_LOGIC, 0.4f); skillGain(me, SK_CREATIVITY, 0.4f); }
    if (hasLot(LT_PEACEFUL) && hour == 9) addMoodlet(me, "Peaceful Home", EM_FINE, 1, 480);
    if (hasLot(LT_GREEN) && hour == 7) { int n = 0; for (Plot& p : G.plots) if (p.crop && !p.watered) { p.watered = 1; n++; } if (n) toast("Morning dew watered your garden!"); }
    if (hasLot(LT_HAUNTED) && (hour >= 22 || hour <= 3) && irand(8) == 0 && !momentActive()) {
        static const char* SP[] = {"The lights flickered... and a whisper said your name.", "A cold breeze. The rocking chair is rocking. By itself.", "Something giggled in the closet. Nobody's in there.", "The TV turned on to static. Then a face. Then nothing."};
        toast(SP[irand(4)]);
        if (me.hasTrait(TR_ACTIVE) || me.hasTrait(TR_GOOFBALL) || irand(2)) { addMoodlet(me, "Spooky Thrill", EM_PLAYFUL, 1, 180); me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 15); }
        else addMoodlet(me, "Totally Spooked", EM_TENSE, 2, 240);
    }
    if (hasLot(LT_GREMLINS) && irand(28) == 0) {
        static const char* BR[] = {"The toilet is overflowing!", "The TV just died.", "The fridge is making a weird noise...", "The shower only runs cold now."};
        int cost = (G.eco.subs & 8) ? 0 : 25 + irand(40); G.money = std::max<int32_t>(0, G.money - cost);
        char b[96]; snprintf(b, sizeof b, "%s Repair: -$%d (thanks, gremlins)", BR[irand(4)], cost); toast(b); sfxNo();
    }
}

// ================================================================ My Lot app
static int lotSel = 0;
static Btn lotB(int i) { return Btn{4 + (i % 2) * 158.f, 26 + (i / 2) * 32.f, 154, 29}; }
void lotAppUpdate(const Input& in) {
    if (!in.touch) return;
    for (int i = 0; i < LT_COUNT; i++) if (lotB(i).in(in.tp)) {
        lotSel = i;
        if (hasLot(i)) { G.lotTraits &= (uint16_t)~(1u << i); toast(std::string("Removed lot trait: ") + LOT_NAMES[i]); sfxTap(); }
        else {
            int n = 0; for (int k = 0; k < LT_COUNT; k++) if (hasLot(k)) n++;
            if (n >= 3) { toast("Your home can have up to 3 lot traits. Tap one to remove it."); sfxNo(); }
            else { G.lotTraits |= (uint16_t)(1u << i); toast(std::string("Lot trait added: ") + LOT_NAMES[i] + "!"); sfxYay(); }
        }
        return;
    }
}
void lotAppDrawTop(float t) {
    grad(0, 0, 400, 240, C(232, 244, 255), C(214, 230, 250));
    // a little house
    tri(140, 110, 200, 60, 260, 110, C(220, 110, 140)); rect(150, 110, 100, 80, C(255, 246, 236)); rect(188, 140, 24, 50, C(150, 100, 70)); rect(160, 120, 20, 18, C(170, 220, 250)); rect(220, 120, 20, 18, C(170, 220, 250));
    if (hasLot(LT_HAUNTED)) { circle(270, 70 + std::sin(t * 2) * 5, 10, C(255, 255, 255, 180)); }
    if (hasLot(LT_GREEN)) for (int i = 0; i < 5; i++) circle(150 + i * 25.f, 192, 6, C(110, 190, 90));
    textOutline("My Lot", 200, 10, 0.7f, C(255, 255, 255), C(UI1), 1, 1.2f);
    text(LOT_NAMES[lotSel], 200, 200, 0.55f, C(lotBad(lotSel) ? RGB{200, 50, 60} : RGB{40, 110, 200}), 1);
    text(LOT_DESC[lotSel], 200, 218, 0.4f, C(UI1), 1);
    int n = 0; for (int k = 0; k < LT_COUNT; k++) if (hasLot(k)) n++;
    char b[40]; snprintf(b, sizeof b, "%d/3 lot traits", n); text(b, 330, 40, 0.42f, C(UI1), 1);
}
void lotAppDrawBottom(float) {
    for (int i = 0; i < LT_COUNT; i++) button(lotB(i), LOT_NAMES[i], hasLot(i), hasLot(i) ? 0 : lotBad(i) ? 1 : 2, 0.42f);
    text("Pink = negative (more drama!)", 160, 192, 0.38f, C(CREAM), 1);
}
