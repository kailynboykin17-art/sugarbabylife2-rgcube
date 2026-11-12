#include "styling.hpp"
#include "draw.hpp"
#include "sim.hpp"
#include "audio.hpp"
#include <cmath>
#include <cstdio>

using namespace col;

enum Vibe { V_GLAM, V_CUTE, V_CASUAL, V_SPORTY, V_COUNT };
enum Tone { T_PINK, T_BLACK, T_GOLD, T_ANIMAL, T_CREAM, T_COUNT };
static const char* VIBE_NAMES[V_COUNT] = {"glam", "cute", "casual", "sporty"};
static const char* TONE_NAMES[T_COUNT] = {"pink", "black", "gold", "animal print", "neutral"};

struct Piece { const char* name; RGB c; uint8_t pattern; uint8_t vibe, tone; Bottom bt; };
static const Piece TOPS[] = {
    {"Zebra Tank", CREAM, 1, V_GLAM, T_ANIMAL, BT_PANTS}, {"Pink Cami", HOT, 0, V_CUTE, T_PINK, BT_PANTS},
    {"Leopard Top", BLACK, 2, V_GLAM, T_ANIMAL, BT_PANTS}, {"Cream Tee", CREAM, 0, V_CASUAL, T_CREAM, BT_PANTS},
    {"Velour Hoodie", PINK, 0, V_SPORTY, T_PINK, BT_PANTS}, {"Black Corset", BLACK, 0, V_GLAM, T_BLACK, BT_PANTS},
    {"Gold Halter", GOLD, 0, V_GLAM, T_GOLD, BT_PANTS}, {"Striped Polo", BLUSH, 3, V_CASUAL, T_PINK, BT_PANTS},
};
static const Piece BOTS[] = {
    {"Black Flares", BLACK, 0, V_GLAM, T_BLACK, BT_PANTS}, {"Pink Mini", BLUSH, 0, V_CUTE, T_PINK, BT_MINI},
    {"Low-Rise Jeans", BABYBLUE, 0, V_CASUAL, T_CREAM, BT_PANTS}, {"Velour Pants", PINK, 0, V_SPORTY, T_PINK, BT_PANTS},
    {"Gold Mini", GOLD, 0, V_GLAM, T_GOLD, BT_MINI}, {"Cream Shorts", CREAM, 0, V_CASUAL, T_CREAM, BT_SHORTS},
    {"Ball Gown", BERRY, 0, V_GLAM, T_PINK, BT_GOWN},
};
static const Piece SHOES[] = {
    {"Pink Heels", HOT, 0, V_GLAM, T_PINK, BT_PANTS}, {"Gold Heels", GOLD, 0, V_GLAM, T_GOLD, BT_PANTS},
    {"Black Stilettos", BLACK, 0, V_GLAM, T_BLACK, BT_PANTS}, {"White Sneakers", WHITE, 0, V_SPORTY, T_CREAM, BT_PANTS},
    {"Blush Flats", BLUSH, 0, V_CUTE, T_PINK, BT_PANTS}, {"Leopard Pumps", {214, 166, 98}, 0, V_GLAM, T_ANIMAL, BT_PANTS},
};
#define CNT(a) (int)(sizeof(a) / sizeof(a[0]))

static const char* OCCASIONS[] = {"a first date", "a girls' night out", "a job interview", "the beach", "brunch", "a red carpet party", "the gym", "a school dance"};

static int clientsLeft = 0, client = -1, vibe = 0, tone = 0, occ = 0, sel[3] = {0, 0, 0}, cat = 0;
static bool work = false, judged = false;
static int stars = 0, totalStars = 0;
static float reactT = 0;

static void newClient() {
    std::vector<int> pool;
    for (size_t i = 0; i < G.sims.size(); i++) if (!G.sims[i].household && !G.sims[i].look.guy && G.sims[i].stage >= ST_TEEN) pool.push_back((int)i);
    client = pool.empty() ? 1 : pool[irand((int)pool.size())];
    vibe = irand(V_COUNT); tone = irand(T_COUNT); occ = irand(8);
    if (occ == 6) vibe = V_SPORTY;
    if (occ == 5) vibe = V_GLAM;
    sel[0] = sel[1] = sel[2] = 0; cat = 0; judged = false; stars = 0;
}

void styleStart(int clients, bool forWork) { clientsLeft = clients; work = forWork; totalStars = 0; newClient(); }

static int scoreLook() {
    const Piece* p[3] = {&TOPS[sel[0]], &BOTS[sel[1]], &SHOES[sel[2]]};
    int s = 0;
    for (auto* x : p) { if (x->vibe == vibe) s++; if (x->tone == tone) s++; }
    s += skillLevel(MC(), SK_STYLE) >= 5 ? 1 : 0;
    return clampv(s * 5 / 6, 0, 5);   // 0..5 stars
}

static const Btn bCat[3] = {{4, 4, 100, 26}, {110, 4, 100, 26}, {216, 4, 100, 26}};
static Btn optB(int i) { return Btn{4 + (i % 2) * 158.f, 36 + (i / 2) * 32.f, 154, 28}; }
static const Btn bDone{110, 204, 100, 30}, bQuit{244, 204, 72, 30};

bool styleUpdate(const Input& in) {
    if (reactT > 0) {
        reactT -= in.dt;
        if (reactT <= 0) {
            if (--clientsLeft <= 0) {
                Sim& me = MC();
                if (work) finishShift(me, CAREERS[CR_STYLIST].hours, totalStars * 3.f);
                else { G.money += totalStars * 15; toast("Styling tips: $" + std::to_string(totalStars * 15)); }
                return false;
            }
            newClient();
        }
        return true;
    }
    if ((in.down & KEY_B) || (in.touch && bQuit.in(in.tp))) {
        if (work) { clientsLeft = 1; totalStars += 0; reactT = 0.01f; return true; }
        return false;
    }
    if (in.down & KEY_L) cat = (cat + 2) % 3;
    if (in.down & KEY_R) cat = (cat + 1) % 3;
    if (!in.touch) {
        if (in.down & KEY_A) goto judge;
        return true;
    }
    for (int i = 0; i < 3; i++) if (bCat[i].in(in.tp)) { cat = i; sfxTap(); return true; }
    {
        int n = cat == 0 ? CNT(TOPS) : cat == 1 ? CNT(BOTS) : CNT(SHOES);
        for (int i = 0; i < n; i++) if (optB(i).in(in.tp)) { sel[cat] = i; sfxTap(); return true; }
    }
    if (!bDone.in(in.tp)) return true;
judge:
    stars = scoreLook();
    totalStars += stars;
    judged = true;
    reactT = 2.6f;
    skillGain(MC(), SK_STYLE, 1.5f + stars);
    changeRel(G.mc, client, stars >= 3 ? 5 : -2, 0);
    if (stars >= 4) { sfxYay(); fxBurst(200, 80, 20); } else if (stars <= 1) sfxNo(); else sfxTap();
    return true;
}

void styleDrawTop(float t) {
    grad(0, 0, TOP_W, TOP_H, C(255, 214, 230), C(255, 186, 210));
    for (int r = 0; r < 4; r++) for (int k = 0; k < 9; k++) sparkle(k * 48.f + (r % 2) * 24 + 12, r * 60.f + 20, 2 + std::fabs(std::sin(t * 2 + k + r)) * 2, C(255, 255, 255, 140));
    rect(0, 200, TOP_W, 40, C(250, 244, 240));
    // runway mirror
    ellipse(110, 130, 130, 190, C(GOLD)); ellipse(110, 130, 120, 180, C(220, 236, 248));
    const Sim& c = G.sims[client];
    Sim tmp = c;
    tmp.outfit = 0;
    tmp.look.top = TOPS[sel[0]].c; tmp.look.topPattern = TOPS[sel[0]].pattern;
    tmp.look.bottom = BOTS[sel[1]].c; tmp.look.bt = BOTS[sel[1]].bt;
    tmp.look.shoes = SHOES[sel[2]].c;
    Pose pose = judged ? (stars >= 3 ? PO_DANCE : PO_IDLE) : PO_IDLE;
    drawSim(tmp, 110, 222, 1.25f, t, 0, pose, false);
    // request card
    rrect(214, 14, 178, 112, 10, C(UI1)); rrect(216, 16, 174, 108, 9, C(CREAM));
    char b[160];
    text(c.name.c_str(), 303, 20, 0.55f, C(HOT), 1);
    snprintf(b, sizeof b, "\"I need something %s for %s. Make it %s!\"", VIBE_NAMES[vibe], OCCASIONS[occ], TONE_NAMES[tone]);
    auto lines = wrapText(b, 0.42f, 164);
    for (size_t i = 0; i < lines.size() && i < 5; i++) text(lines[i].c_str(), 222, 42 + i * 14.f, 0.42f, C(UI1));
    snprintf(b, sizeof b, "Clients left: %d", clientsLeft);
    text(b, 303, 134, 0.42f, C(UI1), 1);
    if (judged) {
        rrect(214, 150, 178, 50, 10, C(HOT));
        static const char* react[] = {"Um... no.", "Not really me...", "It's okay!", "Cute!!", "I LOVE it!", "OMG I'm OBSESSED!"};
        text(react[stars], 303, 156, 0.5f, C(WHITE), 1);
        for (int i = 0; i < 5; i++) sparkle(254 + i * 20, 186, 7, C(i < stars ? GOLD : RGB{255, 255, 255}, i < stars ? 255 : 80));
    }
}

void styleDrawBottom(float t) {
    bottomBackdrop(t);
    static const char* catNames[3] = {"Tops", "Bottoms", "Shoes"};
    for (int i = 0; i < 3; i++) button(bCat[i], catNames[i], cat == i, 1, 0.48f);
    const Piece* list = cat == 0 ? TOPS : cat == 1 ? BOTS : SHOES;
    int n = cat == 0 ? CNT(TOPS) : cat == 1 ? CNT(BOTS) : CNT(SHOES);
    for (int i = 0; i < n; i++) {
        Btn b = optB(i);
        button(b, "", sel[cat] == i, 0, 0.42f);
        if (list[i].pattern == 1) zebraRect(b.x + 6, b.y + 6, 16, 16, C(CREAM), C(BLACK), i);
        else if (list[i].pattern == 2 || list[i].tone == T_ANIMAL) leopardRect(b.x + 6, b.y + 6, 16, 16, i);
        else rrect(b.x + 6, b.y + 6, 16, 16, 4, C(list[i].c));
        text(list[i].name, b.x + 28, b.y + 7, 0.42f, C(CREAM));
    }
    button(bDone, "Style It!", false, 2, 0.5f);
    button(bQuit, work ? "End Shift" : "Leave", false, 1, 0.42f);
    text("Match the vibe AND the colors!", 160, 188, 0.4f, C(CHAMP), 1);
}
