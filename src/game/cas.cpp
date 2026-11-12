#include "famdrama.hpp"
#include "traits2.hpp"
#include "kidlife.hpp"
#include "cas.hpp"
#include "lifeplus.hpp"
#include "cook.hpp"
#include "draw.hpp"
#include "sim.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include "goals.hpp"
#include "oc.hpp"
#include <cmath>
#include <cstdio>

using namespace col;

static Sim me;
static int page = 0, houseSel = 0;
static int stageSel = 1;   // 0 teen, 1 young adult, 2 adult
static const int NCOL = 16;
static const RGB CAS_COLORS[NCOL] = {HOT, BLUSH, PINK, BERRY, CREAM, BLACK, GOLD, LILAC, BABYBLUE, MINT, CHOC, WHITE, {84, 132, 204}, {206, 40, 64}, {50, 54, 92}, {120, 130, 70}};
static const RGB LIPS[10] = {{196, 40, 96}, {200, 30, 50}, {240, 120, 150}, {190, 110, 100}, {170, 96, 120}, {120, 60, 50}, {150, 40, 120}, {230, 150, 120}, {250, 110, 90}, {90, 20, 40}};
enum { P_LOOK, P_FACE, P_GLAM, P_PRETTY, P_OUTFIT, P_BOTTOM, P_TRAITS, P_FAVS, P_LIFE, NP };
static int traitPage = 0, homeTypeSel = 1;
static const int TRAITS_PER_PAGE = 20;
static const int TR_PAGES = (TR_COUNT + TRAITS_PER_PAGE - 1) / TRAITS_PER_PAGE;
// accessory sets you can pick in Create-a-Sim
static const uint16_t ACC_SETS[] = {0, ACC_BAG, ACC_NECKLACE, ACC_SHADES, ACC_HOOPS, ACC_BANGLES, ACC_PEARLS, ACC_BAG | ACC_SHADES, ACC_NECKLACE | ACC_HOOPS, ACC_CHOKER, ACC_BAG | ACC_NECKLACE | ACC_BANGLES, ACC_CLIPS};
static const char* ACC_SET_NAMES[] = {"None", "Handbag", "Gold Chains", "Sunglasses", "Big Hoops", "Bangles", "Pearls", "Bag + Shades", "Chains + Hoops", "Choker", "Full Glam", "Butterfly Clips"};
static const int NACC = sizeof(ACC_SETS) / sizeof(ACC_SETS[0]);
static int accSel = 0, accColSel = 0;
static int topCol = 0, botCol = 5, shoeCol = 0, lipSel = 0;
// Custom Head face match: the doll face is redrawn to look like the Sims head it came from (SBL2 pixel style)
static void applyHeadFace(Sim& me, const std::string& name) {
    struct HF { const char* key; uint8_t skin, eyeCol, brows, lipShape, lipCol, shadow, liner, lash, marks, eyes; };
    static const HF T[] = {
        {"Afro",     5, 1, 2, 0, 3, 0, 1, 0, 0, 0},   // deep skin, dark brown almond eyes, bold brows, full nude lips
        {"Ginger",   6, 3, 2, 3, 2, 1, 1, 1, 1, 3},   // rosy fair, green doe eyes, freckles, pink gloss pout, doll lashes
        {"Blonde",   1, 4, 0, 3, 7, 8, 1, 1, 1, 3},   // light, blue eyes, sunset shadow, peach pout, light freckles
        {"Espresso", 3, 0, 0, 7, 7, 6, 2, 1, 0, 0},   // tan, brown eyes, bronze shadow, winged liner, glossy lips
    };
    for (const HF& h : T) if (name.find(h.key) != std::string::npos) {
        FaceDef& f = me.look.f;
        me.look.skin = h.skin; me.look.hairCol = (uint8_t)(h.key[0] == 'A' ? 0 : h.key[0] == 'G' ? 5 : h.key[0] == 'B' ? 3 : 1); f.nose = (uint8_t)(h.key[0] == 'A' ? 8 : 1); f.eyeCol = h.eyeCol; f.brows = h.brows; f.lipShape = h.lipShape; me.look.lips = LIPS[h.lipCol];
        f.shadow = h.shadow; f.liner = h.liner; f.marks = h.marks; f.eyes = h.eyes; f.glasses = 0; me.look.hat = 0;
        f.shape = (uint8_t)((f.shape & ~(7 << 2)) | ((h.lash & 7) << 2));
        return;
    }
}
static int editSim = -1, editPrice = 0;   // >= 0 while restyling an existing sim at the Glam Station
static int hhEdit = 0;   // Create-a-Household: which member the CAS pages are editing (0 = you)
static int numPages() { return editSim >= 0 ? P_TRAITS : hhEdit > 0 ? P_LIFE : NP; }

bool askText(const char* hint, std::string& out, int maxLen, bool numeric) {
    SwkbdState sw;
    char buf[512] = {0};
    swkbdInit(&sw, numeric ? SWKBD_TYPE_NUMPAD : SWKBD_TYPE_NORMAL, 2, maxLen);
    swkbdSetHintText(&sw, hint);
    if (!out.empty()) swkbdSetInitialText(&sw, out.c_str());
    if (!numeric) swkbdSetFeatures(&sw, SWKBD_PREDICTIVE_INPUT);
    SwkbdButton b = swkbdInputText(&sw, buf, sizeof buf);
    if (b == SWKBD_BUTTON_CONFIRM && buf[0]) { out = buf; return true; }
    return false;
}

static void applyColors() {
    me.look.top = CAS_COLORS[topCol];
    me.look.bottom = CAS_COLORS[botCol];
    me.look.shoes = CAS_COLORS[shoeCol];
    me.look.lips = LIPS[lipSel];
    me.look.acc = ACC_SETS[accSel % NACC];
    me.look.accCol = CAS_COLORS[accColSel % NCOL];
}

static int nearestIdx(const RGB* pal, int n, RGB c) {
    int best = 0, bd = 1 << 30;
    for (int i = 0; i < n; i++) { int d = (pal[i].r - c.r) * (pal[i].r - c.r) + (pal[i].g - c.g) * (pal[i].g - c.g) + (pal[i].b - c.b) * (pal[i].b - c.b); if (d < bd) { bd = d; best = i; } }
    return best;
}
void casStartEdit(int sim, int price) {
    editSim = sim; editPrice = price; page = P_LOOK;
    me = G.sims[sim];
    topCol = nearestIdx(CAS_COLORS, NCOL, me.look.top);
    botCol = nearestIdx(CAS_COLORS, NCOL, me.look.bottom);
    shoeCol = nearestIdx(CAS_COLORS, NCOL, me.look.shoes);
    accColSel = nearestIdx(CAS_COLORS, NCOL, me.look.accCol);
    accSel = 0; for (int i = 0; i < NACC; i++) if (ACC_SETS[i] == me.look.acc) accSel = i;
    lipSel = nearestIdx(LIPS, 10, me.look.lips);
}
bool casEditing() { return editSim >= 0; }

static void hhReset();
void casStart(bool) {
    editSim = -1;
    hhReset();
    me = Sim();
    me.name = "Maya";
    // the Stick Figure Ver. star: warm brown skin, a sleek bob, pink tank, denim shorts & sandals
    me.look.guy = false; me.look.skin = 8; me.look.hairCol = 0; me.look.hairStyle = 3;
    me.look.bt = BT_SHORTS; me.look.topPattern = 0; me.look.topStyle = TS_TANK; me.look.shoeStyle = SS_SANDALS; me.look.hat = HT_NONE;
    topCol = 0; botCol = 12; shoeCol = 0; lipSel = 2; accSel = 4; accColSel = 0;
#ifdef DIVA_LIFE
    // the official Dream Dollhouse girl: long dark-brown hair, pink baby tee, denim mini, white kicks
    me.name = "Jasmine"; me.look.skin = 1; me.look.hairCol = 1; me.look.hairStyle = 0;
    me.look.bt = BT_MINI; me.look.topStyle = TS_TEE; me.look.shoeStyle = SS_SNEAKERS;
    topCol = 2; botCol = 12; shoeCol = 11; accSel = 0;
#endif
    me.look.f.eyes = 3; me.look.f.eyeCol = 0; me.look.f.brows = 0; me.look.f.lipShape = 1;
    me.look.f.shadow = 1; me.look.f.liner = 1; me.look.f.blush = 0;
    applyColors();
    me.traits[0] = TR_FASHIONISTA; me.traits[1] = TR_GLAMOROUS; me.traits[2] = TR_ROMANTIC;
    me.interest = 0;
    page = 0; stageSel = 1; houseSel = 0;
}

// --- layout helpers
static Btn rowL(float y) { return Btn{150, y, 34, 24}; }
static Btn rowR(float y) { return Btn{280, y, 34, 24}; }
static int numPages();
static Btn tabB(int i) { const float w = 316.f / numPages(); return Btn{2 + i * w, 4, w - 1.5f, 24}; }
static const Btn bBack{4, 208, 90, 28}, bNext{226, 208, 90, 28}, bCast{98, 208, 124, 28};
static int castPick = -1;
static int g_beautySel = 0;   // -1: your own sim, else play as a Starlight Cast member
static Btn traitB(int i) { i %= TRAITS_PER_PAGE; return Btn{4 + (i % 4) * 79.f, 30 + (i / 4) * 25.f, 76, 23}; }
static const Btn bTrPrev{4, 156, 40, 22}, bTrNext{276, 156, 40, 22};

static int cyc(int v, int d, int n) { return (v + d + n) % n; }

// ------------------------------------------------------------------ Create-a-Household
// After making your first sim, add up to 5 more (spouse, kids, siblings, parents, roommates).
enum HRel { HR_YOU, HR_SPOUSE, HR_CHILD, HR_SIBLING, HR_PARENT, HR_ROOMMATE, HR_COUNT };
static const char* HREL_NAMES[HR_COUNT] = {"You", "Spouse", "Child", "Sibling", "Parent", "Roommate"};
static const Stage HH_STAGES[] = {ST_TODDLER, ST_CHILD, ST_PRETEEN, ST_TEEN, ST_YA, ST_ADULT, ST_MIDDLE, ST_ELDER};
constexpr int NHH_STAGES = sizeof(HH_STAGES) / sizeof(HH_STAGES[0]);
static std::vector<Sim> hh;
static std::vector<uint8_t> hhRel;
static std::vector<uint8_t> hhDyn;
static bool hhScreen = false;   // showing the household screen
static void hhReset() { hh.clear(); hhRel.clear(); hhDyn.clear(); hhEdit = 0; hhScreen = false; }
static Btn hhRow(int k) { return Btn{4, 30 + k * 28.f, 92, 26}; }
static Btn hhRelB(int k) { return Btn{100, 30 + k * 28.f, 66, 26}; }
static Btn hhDynB(int k) { return Btn{170, 30 + k * 28.f, 62, 26}; }   // family dynamic with the head
static Btn hhAgeB(int k) { return Btn{236, 30 + k * 28.f, 52, 26}; }
static Btn hhDelB(int k) { return Btn{292, 30 + k * 28.f, 24, 26}; }
static void syncSel() {
    topCol = nearestIdx(CAS_COLORS, NCOL, me.look.top); botCol = nearestIdx(CAS_COLORS, NCOL, me.look.bottom);
    shoeCol = nearestIdx(CAS_COLORS, NCOL, me.look.shoes); accColSel = nearestIdx(CAS_COLORS, NCOL, me.look.accCol);
    accSel = 0; for (int k = 0; k < NACC; k++) if (ACC_SETS[k] == me.look.acc) accSel = k;
    lipSel = nearestIdx(LIPS, 10, me.look.lips);
}
static int stageIdx(Stage s) { for (int i = 0; i < NHH_STAGES; i++) if (HH_STAGES[i] == s) return i; return 4; }
static int traitCap(Stage s) { return s <= ST_PRETEEN ? 1 : s == ST_TEEN ? 2 : 3; }
static void addMember() {
    if (hh.size() >= 6) { sfxNo(); return; }
    bool hasSpouse = false; for (uint8_t r : hhRel) hasSpouse |= r == HR_SPOUSE;
    uint8_t r = (!hasSpouse && hh[0].isAdult()) ? HR_SPOUSE : hh[0].isAdult() ? HR_CHILD : HR_SIBLING;
    Stage st = r == HR_SPOUSE ? hh[0].stage : r == HR_CHILD ? ST_CHILD : hh[0].stage;
    Sim s = randomSim(st, irand(2));
    s.look.skin = hh[0].look.skin;   // family resemblance to start with
    s.household = true;
    hh.push_back(s); hhRel.push_back(r); hhDyn.push_back(0);
    hhEdit = (int)hh.size() - 1; me = hh[hhEdit]; syncSel();
    hhScreen = false; page = P_LOOK;
}
static void linkHousehold(const std::vector<int>& idx) {
    // everyone's related to everyone in a sensible way
    for (size_t i = 0; i < idx.size(); i++)
        for (size_t j = i + 1; j < idx.size(); j++) {
            int ri = hhRel[i], rj = hhRel[j];
            Rel& r = rel(idx[i], idx[j]);
            r.met = true;
            auto has = [&](int a, int b) { return (ri == a && rj == b) || (ri == b && rj == a); };
            if (has(HR_YOU, HR_SPOUSE)) { r.status = RS_MARRIED; r.fr = 60; r.ro = 70; }
            else if (ri == HR_ROOMMATE || rj == HR_ROOMMATE) r.fr = 30;
            else if (has(HR_SPOUSE, HR_SIBLING) || has(HR_SPOUSE, HR_PARENT)) r.fr = 35;   // in-laws
            else { r.status = RS_FAMILY; r.fr = (has(HR_YOU, HR_CHILD) || has(HR_SPOUSE, HR_CHILD)) ? 60 : 45; }
        }
}

static int finishEdit(bool keep) {
    int s = editSim;
    editSim = -1;
    if (!keep || s < 0 || s >= (int)G.sims.size()) { sfxTap(); return 3; }
    if (G.money < editPrice) { toast("Not enough money for the salon!"); sfxNo(); return 3; }
    G.money -= editPrice;
    Sim& dst = G.sims[s];
    dst.look = me.look; dst.outfit = me.outfit; dst.name = me.name;
    addMoodlet(dst, "Fresh New Look", EM_CONFIDENT, editPrice ? 3 : 2, 360);
    if (s == G.mc) goalNotify(GE_MAKEUP);
    addMemoryOnce(MEM_FUN, s, -1, dst.name + " debuted a brand-new look!");
    toast(editPrice ? "Salon glow-up complete! Gorgeous!" : "Restyled! Looking fresh!");
    fxBurst(200, 90, 20); sfxYay();
    return 2;
}

// the household screen ("Move In!" builds the world)
static int householdUpdate(const touchPosition& tp) {
    if (bBack.in(tp)) { hhScreen = false; hhEdit = 0; me = hh[0]; syncSel(); page = P_LIFE; sfxTap(); return 0; }
    if (bCast.in(tp)) { addMember(); sfxTap(); return 0; }
    for (int k = 0; k < (int)hh.size(); k++) {
        if (hhRow(k).in(tp)) { hhEdit = k; me = hh[k]; syncSel(); hhScreen = false; page = P_LOOK; sfxTap(); return 0; }
        if (k == 0) continue;
        if (hhRelB(k).in(tp)) { hhRel[k] = (uint8_t)(1 + (hhRel[k] % (HR_COUNT - 1))); sfxTap(); return 0; }
        if (hhDynB(k).in(tp)) {
            if (hhDyn.size() < hh.size()) hhDyn.resize(hh.size(), 0);
            const bool gap = (hh[0].stage >= ST_YA) != (hh[k].stage >= ST_YA);
            static const uint8_t SIB[] = {DY_NONE, DY_CLOSE, DY_DISTANT, DY_DIFFICULT, DY_JOKESTERS};
            static const uint8_t GAP[] = {DY_NONE, DY_SUPPORTIVE, DY_PERMISSIVE, DY_STRICT, DY_CLOSE, DY_DISTANT, DY_DIFFICULT};
            const uint8_t* L = gap ? GAP : SIB; const int n = gap ? 7 : 5; int at = 0; for (int i = 0; i < n; i++) if (L[i] == hhDyn[k]) at = i;
            hhDyn[k] = L[(at + 1) % n]; sfxTap(); return 0;
        }
        if (hhAgeB(k).in(tp)) { hh[k].stage = HH_STAGES[(stageIdx(hh[k].stage) + 1) % NHH_STAGES]; sfxTap(); return 0; }
        if (hhDelB(k).in(tp)) { hh.erase(hh.begin() + k); hhRel.erase(hhRel.begin() + k); if (k < (int)hhDyn.size()) hhDyn.erase(hhDyn.begin() + k); sfxTap(); return 0; }
    }
    if (!bNext.in(tp)) return 0;
    {   // move in!
        Sim head = hh[0];
        bool aging = G.aging, preg = G.pregnancyOn;
        buildWorld(G, head);
        std::vector<int> idx{G.mc};
        for (size_t k = 1; k < hh.size(); k++) {
            Sim s = hh[k];
            s.household = true;
            for (int t = traitCap(s.stage); t < 3; t++) s.traits[t] = TR_NONE;
            if (s.traits[0] == TR_NONE) s.traits[0] = TR_CHEERFUL;
            s.stageDays = 0;
            G.sims.push_back(s);
            int i = (int)G.sims.size() - 1;
            idx.push_back(i);
            Actor a; a.kind = 0; a.id = (uint16_t)i; a.x = 120.f + k * 46; a.tx = a.x; a.y = 214.f + (k % 2) * 8;
            G.rooms[G.house][R_LIVING].actors.push_back(a);
        }
        linkHousehold(idx);
        for (size_t k = 1; k < idx.size() && k < hhDyn.size(); k++) if (hhDyn[k]) famSetDynamic(idx[0], idx[k], hhDyn[k]);   // dynamics chosen in CAS
        me = head;
        hh.clear(); hhRel.clear(); hhDyn.clear(); hhEdit = 0; hhScreen = false;
        G.aging = aging; G.pregnancyOn = preg;
        G.artStyle = (uint8_t)g_artStyle;
        giveOutfit(9); giveOutfit(11); giveOutfit(17); giveOutfit(21);   // starter looks
        if (me.outfit) giveOutfit(me.outfit);
        G.house = houseSel; G.houseType[0] = G.houseType[1] = (uint8_t)homeTypeSel;
        if (houseSel != 0) {   // move the starting actors to the chosen house
            G.rooms[houseSel][R_LIVING].actors = G.rooms[0][R_LIVING].actors;
            G.rooms[0][R_LIVING].actors.clear();
        }
        { Memory mm; mm.day = G.day; mm.type = MEM_HOME; mm.a = (int16_t)G.mc; mm.text = std::string("Moved into ") + HOUSE_NAMES[houseSel] + "! A fresh start."; G.memories.push_back(mm); }
        sfxYay();
        return 1;
    }
    return 0;
}

int casUpdate(const Input& in) {
    const int np = numPages();
    if (editSim >= 0 && (in.down & KEY_B)) return finishEdit(false);
    if (!in.touch) {
        if (hhScreen) return 0;
        if (in.down & KEY_L) page = cyc(page, -1, np);
        if (in.down & KEY_R) page = cyc(page, 1, np);
        return 0;
    }
    const touchPosition& tp = in.tp;
    if (hhScreen) return householdUpdate(tp);
    for (int i = 0; i < np; i++) if (tabB(i).in(tp)) { page = i; sfxTap(); return 0; }
    if (editSim >= 0) {
        if (bBack.in(tp)) return finishEdit(false);
        if (bNext.in(tp)) return finishEdit(true);
    }
    if (page == P_LOOK && hhEdit == 0 && bCast.in(tp)) {   // cycle: your own sim -> each cast member
        castPick = castPick + 1 >= OC_COUNT ? -1 : castPick + 1;
        if (castPick < 0) { casStart(true); castPick = -1; }
        else {
            Sim c = ocMakeSim(castPick);
            uint8_t tr[3] = {me.traits[0], me.traits[1], me.traits[2]};
            me = c; me.household = true;
            if (tr[0] != TR_NONE) for (int k = 0; k < 3; k++) me.traits[k] = tr[k];
            stageSel = c.stage == ST_TEEN ? 0 : 1;
            topCol = nearestIdx(CAS_COLORS, NCOL, me.look.top); botCol = nearestIdx(CAS_COLORS, NCOL, me.look.bottom);
            shoeCol = nearestIdx(CAS_COLORS, NCOL, me.look.shoes); accColSel = nearestIdx(CAS_COLORS, NCOL, me.look.accCol);
            accSel = 0; for (int k = 0; k < NACC; k++) if (ACC_SETS[k] == me.look.acc) accSel = k;
            lipSel = nearestIdx(LIPS, 10, me.look.lips);
        }
        sfxTap(); return 0;
    }
    if (bBack.in(tp)) { if (page == 0) return -1; page--; return 0; }
    if (bNext.in(tp)) {
        if (page < P_LIFE && !(hhEdit > 0 && page == P_FAVS)) { page++; return 0; }
        if (hhEdit == 0) me.stage = stageSel == 0 ? ST_TEEN : stageSel == 1 ? ST_YA : ST_ADULT;
        int n = 0;
        for (uint8_t t : me.traits) if (t != TR_NONE) n++;
        if (n < (hhEdit > 0 ? 1 : me.stage == ST_TEEN ? 2 : 3)) { toast(hhEdit > 0 ? "Pick at least 1 trait!" : "Pick your traits first!"); sfxNo(); page = P_TRAITS; return 0; }
        if (me.stage == ST_TEEN && hhEdit == 0) me.traits[2] = TR_NONE;
        me.household = true;
        if (hh.empty()) { hh.push_back(me); hhRel.push_back(HR_YOU); hhDyn.push_back(0); }
        else hh[hhEdit] = me;
        hhScreen = true; sfxTap();
        return 0;
    }
    for (int r = 0; r < 6; r++) {
        float y = 40 + r * 28.f;
        int d = rowL(y).in(tp) ? -1 : rowR(y).in(tp) ? 1 : 0;
        if (!d || page == P_TRAITS) continue;
        if (page == P_FAVS) {
            switch (r) {
            case 0: me.favFood = (uint8_t)cyc(me.favFood, d, F_COUNT); break;
            case 1: me.favColor = (uint8_t)cyc(me.favColor, d, 12); break;
            case 2: me.zodiac = (uint8_t)cyc(me.zodiac, d, 12); break;
            case 3: if (hhEdit == 0) homeTypeSel = cyc(homeTypeSel, d, 3); break;
            case 4: me.favFood = (uint8_t)irand(F_COUNT); me.favColor = (uint8_t)irand(12); me.zodiac = (uint8_t)irand(12); break;
            }
            sfxTap(); continue;
        }
        FaceDef& f = me.look.f;
        sfxTap();
        if (page == P_LOOK) {
            switch (r) {
            case 0: askText("Your sim's name", me.name, 14); break;
            case 1: if (editSim >= 0) { toast("Gender is set - restyle everything else!"); break; }
                    me.look.guy = !me.look.guy; me.look.hairStyle = me.look.guy ? 15 : 3;
                    if (me.look.guy) { me.look.bt = BT_SHORTS; me.look.topStyle = TS_TANK; me.look.hat = HT_NONE; me.interest = 1; f.shadow = 0; f.liner = 0; f.blush = 0; accSel = 0; }
                    else { me.interest = 0; f.beard = 0; f.shadow = 1; f.liner = 1; f.blush = 0; } break;
            case 2: { int k = 0; for (int i = 0; i < NUM_SKINS; i++) if (SKIN_ORDER[i] == me.look.skin) k = i; me.look.skin = SKIN_ORDER[cyc(k, d, NUM_SKINS)]; break; }
            case 3: me.look.hairCol = (uint8_t)cyc(me.look.hairCol, d, NUM_HAIRCOLS); break;
            case 4: me.look.hairStyle = (uint8_t)cyc(me.look.hairStyle, d, NUM_HAIRSTYLES); break;
#ifdef SBL2_CAST
            case 5: f.pad = (uint8_t)((f.pad & ~3) | cyc(f.pad % 4, d, 4)); break;   // body type (keeps nail polish bits)
#else
            case 5: f.shape = (uint8_t)cyc(f.shape, d, 5); break;
#endif
            }
        } else if (page == P_FACE) {
            switch (r) {
            case 0: f.eyes = (uint8_t)cyc(f.eyes % 12, d, 12); break;
            case 1: f.eyeCol = (uint8_t)cyc(f.eyeCol, d, 10); break;
            case 2: f.brows = (uint8_t)cyc(f.brows, d, 10); break;
            case 3: f.nose = (uint8_t)cyc(f.nose, d, 10); break;
            case 4: f.lipShape = (uint8_t)cyc(f.lipShape, d, 10); break;
            case 5: f.glasses = (uint8_t)cyc(f.glasses, d, 10); break;
            }
        } else if (page == P_GLAM) {
            switch (r) {
            case 0: f.shadow = (uint8_t)cyc(f.shadow, d, 10); break;
            case 1: if (me.look.guy) f.beard = (uint8_t)cyc(f.beard, d, 10); else f.liner = (uint8_t)cyc(f.liner, d, 10); break;
            case 2: f.blush = (uint8_t)cyc(f.blush, d, 10); break;
            case 3: lipSel = cyc(lipSel, d, 10); break;
            case 4: f.marks = (uint8_t)cyc(f.marks, d, 10); break;
            case 5: f.pierce = (uint8_t)cyc(f.pierce, d, 10); break;
            }
        } else if (page == P_PRETTY) {
            auto setBits = [&](int shift, int mask, int v) { f.shape = (uint8_t)((f.shape & ~(mask << shift)) | ((v & mask) << shift)); };
            switch (r) {
            case 0: {   // beauty types: a whole look at once (eyes, brows, lashes, liner, blush, contour, lips)
                g_beautySel = cyc(g_beautySel, d, 9); const int b = g_beautySel; if (!b) break;
                struct BT { uint8_t eyes, brows, lash, liner, blush, contour, bstyle, lips, glow; };
                static const BT P[9] = {{}, {0, 0, 0, 1, 5, 1, 0, 0, 1}, {10, 7, 1, 3, 4, 3, 1, 0, 0}, {1, 5, 3, 0, 2, 0, 0, 2, 1}, {3, 1, 2, 5, 1, 0, 3, 1, 0},
                                        {9, 9, 2, 0, 1, 0, 0, 3, 1}, {2, 3, 4, 2, 3, 2, 1, 0, 0}, {7, 6, 6, 2, 8, 2, 1, 0, 1}, {1, 0, 5, 0, 1, 0, 2, 1, 0}};
                if (me.look.guy) {   // male beauty types (guys only)
                    struct MB { uint8_t eyes, brows, contour, lips, beard, marks; };
                    // polar bear, cat, deer, bunny, german shepherd, black lab, golden retriever, fox
                    static const MB Q[9] = {{}, {9, 7, 3, 1, 1, 0}, {10, 6, 3, 0, 1, 0}, {0, 9, 1, 3, 0, 0}, {1, 5, 0, 2, 0, 1}, {11, 7, 3, 0, 0, 0}, {3, 9, 1, 1, 0, 0}, {1, 5, 0, 2, 0, 1}, {2, 6, 2, 0, 6, 2}};
                    const MB& q = Q[b]; f.eyes = q.eyes; f.brows = q.brows; f.lipShape = q.lips; f.beard = q.beard; f.marks = q.marks; f.liner = 0; f.blush = 0;
                    setBits(2, 7, 0); setBits(0, 3, q.contour); setBits(5, 3, 0); setBits(7, 1, 0);
                    break;
                }
                const BT& p = P[b];
                f.eyes = p.eyes; f.brows = p.brows; f.liner = p.liner; f.blush = p.blush; f.lipShape = p.lips;
                setBits(2, 7, p.lash); setBits(0, 3, p.contour); setBits(5, 3, p.bstyle); setBits(7, 1, p.glow);
                if (b == 8) f.marks = 1;   // bunny: freckles
                break; }
            case 1: setBits(2, 7, cyc((f.shape >> 2) & 7, d, 7)); break;
            case 2: setBits(0, 3, cyc(f.shape & 3, d, 4)); break;
            case 3: setBits(5, 3, cyc((f.shape >> 5) & 3, d, 4)); break;
            case 4: setBits(7, 1, ((f.shape >> 7) & 1) ^ 1); break;
            case 5: { const int n = ocHeadCount(); if (n == 0) { toast("No custom heads found. Put head PNGs in LifeSim/Heads on your SD card."); break; }
                      const int h = cyc((f.pad >> 5) & 7, d, n + 1); f.pad = (uint8_t)((f.pad & 0x1F) | (h << 5));
                      if (h) { applyHeadFace(me, ocHeadName(h)); lipSel = nearestIdx(LIPS, 10, me.look.lips); toast(ocHeadName(h) + " head on - face matched! Tweak it on Face & Glam."); } break; }   // Custom Head (was a 2nd Eye Shape row)
            }
        } else if (page == P_BOTTOM) {
            switch (r) {
            case 0: me.look.bt = (Bottom)cyc(me.look.bt, d, NUM_BOTTOMS); break;
            case 1: botCol = cyc(botCol, d, NCOL); break;
            case 2: me.look.shoeStyle = (uint8_t)cyc(me.look.shoeStyle, d, NUM_SHOES); break;
            case 3: shoeCol = cyc(shoeCol, d, NCOL); break;
            case 4: accColSel = cyc(accColSel, d, NCOL); break;
            case 5: {   // surprise me: a random mix & match look
                me.look.topStyle = (uint8_t)irand(NUM_TOPS); me.look.bt = (Bottom)irand(NUM_BOTTOMS); me.look.shoeStyle = (uint8_t)irand(NUM_SHOES - 1);
                topCol = irand(NCOL); botCol = irand(NCOL); shoeCol = irand(NCOL); me.look.topPattern = (uint8_t)(irand(3) ? 0 : 1 + irand(8));
                me.look.hat = (uint8_t)(irand(3) ? 0 : 1 + irand(NUM_HATS - 1)); accSel = irand(NACC); accColSel = irand(NCOL); me.outfit = 0;
                break;
            }
            }
        } else if (page == P_OUTFIT) {
            switch (r) {
            case 0: me.look.topStyle = (uint8_t)cyc(me.look.topStyle, d, NUM_TOPS); break;
            case 1: topCol = cyc(topCol, d, NCOL); break;
            case 2: me.look.topPattern = (uint8_t)cyc(me.look.topPattern, d, 9); break;
            case 3: me.look.hat = (uint8_t)cyc(me.look.hat, d, NUM_HATS); break;
            case 4: accSel = cyc(accSel, d, NACC); break;
            case 5: { int k = me.outfit;
                      for (int tries = 0; tries < NUM_OUTFITS + 1; tries++) {   // at the Glam Station you can only pick outfits you own
                          k = k == 0 ? (d > 0 ? FIRST_AESTHETIC : NUM_OUTFITS - 1) : k + d;
                          if (k < FIRST_AESTHETIC || k >= NUM_OUTFITS) k = 0;
                          if (k == 0 || ((editSim < 0 || ownsOutfit(k) || k >= FIRST_KID_OUTFIT) && outfitFits(k, me.stage, me.look.guy))) break;
                      }
                      me.outfit = (uint8_t)k; break; }
            }
        } else if (page == P_LIFE) {
            switch (r) {
            case 0: stageSel = cyc(stageSel, d, 3); break;
            case 1: me.interest = (uint8_t)cyc(me.interest, d, 3); break;
            case 2: houseSel = 1 - houseSel; break;
            case 3: G.aging = !G.aging; break;
            case 4: setArtStyle(g_artStyle + d); break;
            case 5: G.pregnancyOn = !G.pregnancyOn; break;
            }
        }
        applyColors();
        return 0;
    }
    if (page == P_TRAITS && bTrPrev.in(tp)) { traitPage = (traitPage + TR_PAGES - 1) % TR_PAGES; sfxTap(); return 0; }
    if (page == P_TRAITS && bTrNext.in(tp)) { traitPage = (traitPage + 1) % TR_PAGES; sfxTap(); return 0; }
    if (page == P_TRAITS)
        for (int i = traitPage * TRAITS_PER_PAGE; i < std::min<int>(TR_COUNT, (traitPage + 1) * TRAITS_PER_PAGE); i++)
            if (traitB(i).in(tp)) {
                if (!traitOk(i, me.stage)) { sfxNo(); return 0; }   // adult-only trait on a kid/teen
                sfxTap();
                for (int k = 0; k < 3; k++) if (me.traits[k] == i) { me.traits[k] = TR_NONE; return 0; }
                for (int k = 0; k < 3; k++) if (me.traits[k] == TR_NONE) { me.traits[k] = (uint8_t)i; return 0; }
                me.traits[2] = (uint8_t)i;
                return 0;
            }
    return 0;
}

void casDrawTop(float t) {
    grad(0, 0, TOP_W, TOP_H, C(UIM), C(UI1));
    if (hhScreen) {   // the whole household, side by side
        textOutline("Your Household", 200, 8, 0.7f, C(CREAM), C(UI_INK), 1, 1.2f);
        ellipse(200, 222, 190, 18, C(HOT, 70));
        float step = std::fmin(70.f, 380.f / std::max<size_t>(1, hh.size()));
        float x0 = 200 - step * (hh.size() - 1) / 2.f;
        for (size_t k = 0; k < hh.size(); k++) {
            float x = x0 + k * step;
            drawSim(hh[k], x, 210, 1.0f, t + k * 0.7f, 0, k == 0 ? PO_WAVE : PO_IDLE, false);
            text(hh[k].name.c_str(), x, 212, 0.4f, C(CREAM), 1);
            text(HREL_NAMES[hhRel[k]], x, 226, 0.34f, C(CHAMP), 1);
        }
        return;
    }
    for (int i = 0; i < 8; i++) line(10, 20 + i * 28, 390, 22 + i * 28, 1, C(CREAM, 20));
    swirl(40, 200, 26, 1.8f, 2, C(CHAMP, 70)); swirl(360, 40, 22, 1.6f, 2, C(CHAMP, 70), true);
    ellipse(130, 226, 140, 22, C(HOT, 80)); ellipse(130, 226, 110, 14, C(BLUSH, 90));
    Sim prev = me;
    if (editSim < 0 && hhEdit == 0) prev.stage = stageSel == 0 ? ST_TEEN : stageSel == 1 ? ST_YA : ST_ADULT;
    Pose pose = std::fmod(t, 8.f) < 2.5f ? PO_PHONE : std::fmod(t, 8.f) < 4.f ? PO_WAVE : PO_IDLE;
    if (page == P_FACE || page == P_GLAM || (page == P_LOOK && (artIsGlam(g_artStyle) || artIsDoll(g_artStyle)))) {   // beauty close-up + small full body
        circle(110, 118, 98, C(BLUSH, 60)); circle(110, 118, 92, C(255, 236, 240, 90));
        drawSimPortrait(prev, 110, 112, 92, t);
        drawSim(prev, 196, 226, 0.85f, t, 0, PO_IDLE, false);
    } else drawSim(prev, 130, 224, 1.45f, t, 0, pose, false);
    for (int i = 0; i < 4; i++) sparkle(14 + i * 64, 18 + (i % 2) * 200, 3 + std::fabs(std::sin(t * 2 + i)) * 4, C(GOLD));
    textShadow(editSim >= 0 ? "Glam Station" : "Create Your Sim", 300, 14, 0.7f, C(BLUSH), C(BLACK), 1);
    if (editSim >= 0) {
        rrect(222, 46, 170, 170, 10, C(UI2, 230));
        text(me.name.c_str(), 307, 54, 0.65f, C(GOLD), 1);
        text(STAGE_NAMES[prev.stage], 307, 76, 0.45f, C(CREAM), 1);
        text("Restyle hair, face,", 307, 104, 0.42f, C(BLUSH), 1);
        text("makeup and outfit.", 307, 120, 0.42f, C(BLUSH), 1);
        text(editPrice ? ("Salon price: $" + std::to_string(editPrice)).c_str() : "Free at home!", 307, 146, 0.45f, C(GOLD), 1);
        text("B: cancel", 307, 194, 0.38f, C(CREAM, 160), 1);
        return;
    }
    rrect(222, 46, 170, 170, 10, C(UI2, 230));
    text(me.name.c_str(), 307, 54, 0.65f, C(GOLD), 1);
    text(STAGE_NAMES[prev.stage], 307, 76, 0.45f, C(CREAM), 1);
    for (int k = 0; k < 3; k++) if (me.traits[k] != TR_NONE) text(TRAIT_NAMES[me.traits[k]], 307, 100 + k * 16.f, 0.45f, C(BLUSH), 1);
    static const char* ints[3] = {"Into: Guys", "Into: Girls", "Into: Everyone"};
    text(ints[me.interest], 307, 152, 0.42f, C(CHAMP), 1);
    text(HOUSE_NAMES[houseSel], 307, 170, 0.42f, C(CHAMP), 1);
    text("L/R: change page", 307, 194, 0.38f, C(CREAM, 160), 1);
}

static void row(int r, const char* label, const std::string& val) {
    float y = 40 + r * 28.f;
    text(label, 8, y + 4, 0.48f, C(CREAM));
    button(rowL(y), "<", false, 1, 0.5f);
    rrect(188, y, 88, 24, 6, C(UI2));
    text(val.c_str(), 232, y + 4, 0.44f, C(BLUSH), 1);
    button(rowR(y), ">", false, 1, 0.5f);
}

void casDrawBottom(float t) {
    bottomBackdrop(t);
    if (hhScreen) {
        text("Tap a name to edit. Max 6 people.", 160, 8, 0.4f, C(CREAM), 1);
        for (int k = 0; k < (int)hh.size(); k++) {
            char b[48]; snprintf(b, sizeof b, "%s", hh[k].name.c_str());
            button(hhRow(k), b, k == hhEdit, 1, 0.42f);
            if (k == 0) { text(STAGE_NAMES[stageSel == 0 ? ST_TEEN : stageSel == 1 ? ST_YA : ST_ADULT], 170, 36, 0.4f, C(CHAMP), 1); text("(you)", 250, 36, 0.4f, C(CHAMP), 1); continue; }
            button(hhRelB(k), HREL_NAMES[hhRel[k]], false, 3, 0.38f);
            button(hhDynB(k), k < (int)hhDyn.size() && hhDyn[k] ? DYN_NAMES[hhDyn[k]] : "Dynamic", k < (int)hhDyn.size() && hhDyn[k] != 0, 0, 0.34f);
            button(hhAgeB(k), STAGE_NAMES[hh[k].stage], false, 0, 0.36f);
            button(hhDelB(k), "X", false, 1, 0.4f);
        }
        button(bBack, "Back", false, 1, 0.48f);
        if (hh.size() < 6) button(bCast, "+ Add Member", false, 3, 0.42f);
        button(bNext, "Move In!", false, 2, 0.5f);
        return;
    }
    static const char* tn[NP] = {"Look", "Face", "Glam", "Pretty", "Top", "Bottom", "Traits", "Faves", "Life"};
    for (int i = 0; i < numPages(); i++) button(tabB(i), tn[i], page == i, 1, 0.36f);
    const FaceDef& f = me.look.f;
    static const char* shapeN[5] = {"Oval", "Round", "Heart", "Square", "Long"};
    static const char* eyeN[12] = {"Almond", "Round", "Cat Eye", "Doe", "Hooded", "Monolid", "Sleepy", "Upturned", "Sparkle", "Wide", "Siren", "Downturned"};
    static const char* eyeColN[10] = {"Brown", "Dark Brown", "Hazel", "Green", "Blue", "Gray", "Violet", "Amber", "Teal", "Ruby"};
    static const char* browN[10] = {"Soft Arch", "Straight", "Bold", "High Arch", "Bushy", "Thin", "Feathered", "Angled", "Brow Slit", "Rounded"};
    static const char* noseN[10] = {"Button", "Straight", "Wide", "Upturned", "Small", "Roman", "Snub", "Pointed", "Broad", "Long"};
    static const char* lipShN[10] = {"Full", "Heart", "Thin", "Pouty", "Big Smile", "Small", "Cupid Bow", "Glossy", "Smirk", "Round"};
    static const char* glassN[10] = {"None", "Round", "Cat-Eye", "Square", "Rimless", "Heart", "Star", "Aviator", "Pink Tint", "Nerd Tape"};
    static const char* shadowN[10] = {"None", "Pink", "Smoky", "Gold", "Lilac", "Blue", "Bronze", "Mint", "Sunset", "Berry"};
    static const char* linerN[10] = {"None", "Thin", "Winged", "Cat Eye", "Smudged", "Lower Line", "Graphic", "Glitter", "Teal Pop", "Double Wing"};
    static const char* beardN[10] = {"None", "Stubble", "Goatee", "Full", "Mustache", "Chin Strap", "Soul Patch", "Boxed", "Mutton Chops", "Handlebar"};
    static const char* blushN[10] = {"None", "Pink", "Peach", "Berry", "Bronzer", "Rose", "Orchid", "Apricot", "Coral", "Glow"};
    static const char* marksN[10] = {"None", "Freckles", "Beauty Mark", "Both", "Heart Sticker", "Star Sticker", "Glitter Cheeks", "Face Gem", "Butterfly", "Rhinestones"};
    static const char* pierceN[10] = {"None", "Studs", "Hoops", "Nose Stud", "Hoops+Nose", "Septum", "Brow Ring", "Lip Ring", "Triple Studs", "Huggies"};
    static const char* colNames[NCOL] = {"Hot Pink", "Blush", "Pink", "Berry", "Cream", "Black", "Gold", "Lilac", "Baby Blue", "Mint", "Choco", "White", "Denim", "Red", "Navy", "Olive"};
    static const char* patNames[9] = {"Solid", "Zebra", "Leopard", "Stripes", "Plaid", "Checker", "Hearts", "Rhinestones", "Graphic"};
    static const char* lipNames[10] = {"Berry", "Red", "Pink Gloss", "Nude", "Mauve", "Brown", "Plum", "Peach", "Coral", "Black Cherry"};
    char b[16];
    if (page == P_LOOK) {
        row(0, "Name (tap)", me.name);
        row(1, "Gender", me.look.guy ? (editSim >= 0 ? "Guy (set)" : "Guy") : (editSim >= 0 ? "Girl (set)" : "Girl"));
        row(2, "Skin", SKIN_NAMES[me.look.skin % NUM_SKINS]);
        row(3, "Hair Color", HAIRCOL_NAMES[me.look.hairCol % NUM_HAIRCOLS]);
        row(4, "Hairstyle", HAIRSTYLE_NAMES[me.look.hairStyle % NUM_HAIRSTYLES]);
#ifdef SBL2_CAST
        { static const char* BT[4] = {"Slim", "Mid-Size", "Plus-Size", "Body Tea"}; static const char* BTM[4] = {"Slim", "Mid-Size", "Plus-Size", "Athletic (Abs)"}; row(5, "Body Type", (me.look.guy ? BTM : BT)[f.pad % 4]); }
#else
        row(5, "Face Shape", shapeN[f.shape % 5]);
#endif
    } else if (page == P_FACE) {
        row(0, "Eyes", eyeN[f.eyes % 12]);
        row(1, "Eye Color", eyeColN[f.eyeCol % 10]);
        row(2, "Brows", browN[f.brows % 10]);
        row(3, "Nose", noseN[f.nose % 10]);
        row(4, "Lips", lipShN[f.lipShape % 10]);
        row(5, "Glasses", glassN[f.glasses % 10]);
    } else if (page == P_GLAM) {
        row(0, "Eyeshadow", shadowN[f.shadow % 10]);
        if (me.look.guy) row(1, "Facial Hair", beardN[f.beard % 10]); else row(1, "Eyeliner", linerN[f.liner % 10]);
        row(2, "Blush", blushN[f.blush % 10]);
        row(3, "Lip Color", lipNames[lipSel]);
        row(4, "Face Art", marksN[f.marks % 10]);
        row(5, "Piercings", pierceN[f.pierce % 10]);
    } else if (page == P_PRETTY) {
        static const char* BEAUTY[9] = {"(pick one)", "Swan", "Jaguar", "Bird", "Puppy", "Deer", "Cat", "Fox", "Bunny"};   // girls only
        static const char* BEAUTY_M[9] = {"(pick one)", "Polar Bear", "Cat", "Deer", "Bunny", "German Shepherd", "Black Lab", "Golden Retriever", "Fox"};   // guys only
        static const char* LASHN[8] = {"Natural", "Mink", "Doll", "Wispy", "Spiky", "Bottom Lash", "Fluttery", "Natural 2"};
        static const char* CONT[4] = {"None", "Soft", "Sculpted", "Snatched"};
        static const char* BST[4] = {"Cheeks", "Draped", "Sunkissed", "Low & Soft"};
        row(0, "Beauty Type", (me.look.guy ? BEAUTY_M : BEAUTY)[g_beautySel % 9]);
        row(1, "Lashes", LASHN[(f.shape >> 2) & 7]);
        row(2, "Contour", CONT[f.shape & 3]);
        row(3, "Blush Style", BST[(f.shape >> 5) & 3]);
        row(4, "Highlighter", (f.shape >> 7) & 1 ? "Glowy" : "Off");
        { static std::string hn; hn = ocHeadName((f.pad >> 5) & 7); row(5, "Custom Head", hn.c_str()); }
    } else if (page == P_OUTFIT) {
        row(0, "Top", TOP_NAMES[me.look.topStyle % NUM_TOPS]);
        row(1, "Top Color", colNames[topCol % NCOL]);
        row(2, "Print", patNames[me.look.topPattern % 9]);
        row(3, "Hat", HAT_NAMES[me.look.hat % NUM_HATS]);
        row(4, "Accessories", ACC_SET_NAMES[accSel % NACC]);
        row(5, "Preset Look", me.outfit ? OUTFITS[me.outfit].name : "Mix & Match");
    } else if (page == P_BOTTOM) {
        row(0, "Bottoms", BOTTOM_NAMES[me.look.bt % NUM_BOTTOMS]);
        row(1, "Bottom Color", colNames[botCol % NCOL]);
        row(2, "Shoes", SHOE_NAMES[me.look.shoeStyle % NUM_SHOES]);
        row(3, "Shoe Color", colNames[shoeCol % NCOL]);
        row(4, "Bag/Bow Color", colNames[accColSel % NCOL]);
        row(5, "Surprise Me!", "Shuffle");
    } else if (page == P_TRAITS) {
        for (int i = traitPage * TRAITS_PER_PAGE; i < std::min<int>(TR_COUNT, (traitPage + 1) * TRAITS_PER_PAGE); i++) {
            bool bad = i == TR_LAZY || i == TR_CLUMSY || i == TR_JEALOUS || i == TR_SNOB || i == TR_MEAN || i == TR_HOTHEAD || i == TR_DRAMATIC || i == TR_MESSY || i == TR_SHOPAHOLIC || i == TR_INSECURE || i == TR_SNEAKY;
            if (!traitOk(i, me.stage)) { Btn b = traitB(i); rrect(b.x, b.y, b.w, b.h, 7, C(90, 80, 100)); text(TRAIT_NAMES[i], b.x + b.w / 2, b.y + 4, 0.28f, C(170, 160, 180), 1); text("adults only", b.x + b.w / 2, b.y + 13, 0.24f, C(200, 150, 170), 1); continue; }
            button(traitB(i), TRAIT_NAMES[i], me.hasTrait((uint8_t)i), bad ? 1 : 0, 0.34f);
        }
        button(bTrPrev, "<", false, 3, 0.5f); button(bTrNext, ">", false, 3, 0.5f);
        { char pg[24]; snprintf(pg, sizeof pg, "Page %d/%d", traitPage + 1, TR_PAGES); text(pg, 160, 160, 0.42f, C(CREAM), 1); }
        int hov = me.traits[2] != TR_NONE ? me.traits[2] : me.traits[1] != TR_NONE ? me.traits[1] : me.traits[0];
        if (hov != TR_NONE) text(TRAIT_DESC[hov], 160, 182, 0.38f, C(CHAMP), 1);
        text("Pick 3 traits (teens pick 2). Blue = tricky traits", 160, 196, 0.36f, C(CREAM), 1);
    } else if (page == P_FAVS) {
        static const char* HT[3] = {"Apartment", "House", "Two-Story Home"};
        row(0, "Favorite Food", FOOD_NAMES[me.favFood % F_COUNT]);
        row(1, "Favorite Color", FAV_COLOR_NAMES[me.favColor % 12]);
        row(2, "Zodiac Sign", ZODIAC_NAMES[me.zodiac % 12]);
        if (hhEdit == 0) row(3, "Home Type", HT[homeTypeSel]);
        row(4, "Surprise Me!", "Shuffle");
    } else {
        static const char* st[3] = {"Teen", "Young Adult", "Adult"};
        static const char* ints[3] = {"Guys", "Girls", "Everyone"};
        row(0, "Life Stage", st[stageSel]);
        row(1, "Romance", ints[me.interest]);
        row(2, "Home", houseSel ? "Penthouse" : "Cottage");
        row(3, "Aging", G.aging ? "On" : "Off");
        row(4, "Art Style", artStyleName(g_artStyle));
        row(5, "Babies", G.pregnancyOn ? "On" : "Off");
    }
    if (editSim >= 0) {
        button(bBack, "Cancel", false, 1, 0.48f);
        button(bNext, editPrice ? ("Done $" + std::to_string(editPrice)).c_str() : "Done!", false, 2, 0.5f);
        return;
    }
    if (page == P_LOOK && hhEdit == 0) { std::string lb = castPick < 0 ? "Play as Cast >" : std::string("Cast: ") + ocDef(castPick).name; button(bCast, lb.c_str(), castPick >= 0, 3, 0.4f); }
    button(bBack, page == 0 ? "Title" : "Back", false, 1, 0.48f);
    button(bNext, hhEdit > 0 && page >= P_FAVS ? "Done" : page == P_LIFE ? "Family >" : "Next", false, 2, 0.5f);
    if (hhEdit > 0) text("Editing a family member", 160, 192, 0.36f, C(CHAMP), 1);
}
