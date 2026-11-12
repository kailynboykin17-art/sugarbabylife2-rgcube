#include "tween.hpp"
#include "staff.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "people.hpp"
#include <string>
#include <cmath>
#include <algorithm>

struct StaffPost { int16_t room; float x, x2; uint8_t pose; const char* name; uint8_t guy; const char* lines[3]; };
static const StaffPost POSTS[] = {
    {R_DTBOUTIQUE, 362, 362, PO_IDLE, "Jo Clerk", 0, {"Find everything okay?", "Ring up whenever you're ready!", "That would look SO cute on you."}},
    {R_DTBISTRO, 52, 52, PO_IDLE, "Marcel Host", 1, {"Welcome to Le Bistro!", "Right this way.", "Your booth is ready."}},
    {R_DTBISTRO, 130, 270, PO_WALK, "Sofia Waiter", 0, {"Can I start you with drinks?", "Chef's special is divine tonight.", "Order up!"}},
    {R_DTBISTRO, 348, 348, PO_IDLE, "Andre Chef", 1, {"*sizzle*", "Plating with love!", "ORDER UP!"}},
    {R_DOWNTOWN, 140, 140, PO_IDLE, "Benny Vendor", 1, {"Hot dogs! Get 'em hot!", "Extra mustard?", "Best dogs in Maple Valley."}},
    {R_VELVET, 200, 200, PO_DANCE, "Roxy Dancer", 0, {"*spins on the pole*", "Thank you, honey!", "Tips appreciated!"}},
    {R_VELVET, 350, 350, PO_IDLE, "Dom Bartender", 1, {"What can I get you?", "Mocktails only on my watch.", "Cheers!"}},
    {R_PIXELARCADE, 340, 340, PO_IDLE, "Pixel Kid", 1, {"Got tickets?", "High score's still mine.", "Insert coin!"}},
    {R_FUNPARK, 55, 55, PO_IDLE, "Tess Tickets", 0, {"Day passes here!", "Have a sparkly day!", "Rides are that way!"}},
    {R_KARAOKE, 130, 130, PO_DANCE, "Mic Mia", 0, {"*belts a power ballad*", "Who's next on the mic?", "Encore!"}},
    {R_NIGHTMKT, 110, 110, PO_IDLE, "Vex", 1, {"Psst.", "Fresh stock tonight.", "Keep it quiet."}},
};
static const int NP = sizeof(POSTS) / sizeof(POSTS[0]);
static int ids[NP];
static bool init = false;
static float talkT = 0, partyT = 0;

static int staffSim(int k) {
    if (!init) { for (int i = 0; i < NP; i++) ids[i] = -1; init = true; }
    if (ids[k] >= 0 && ids[k] < (int)G.sims.size() && G.sims[ids[k]].name == POSTS[k].name) return ids[k];
    for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].name == POSTS[k].name) return ids[k] = i;
    if (G.sims.size() >= 120) return -1;
    Sim s = randomSim(ST_YA, POSTS[k].guy); s.name = POSTS[k].name; s.bio = "works here"; s.household = false;
    G.sims.push_back(s); return ids[k] = (int)G.sims.size() - 1;
}

void staffFrame(float dt, bool partyOn) {
    if (G.sims.empty()) return;
    auto& acts = G.rooms[G.house][G.room].actors;
    talkT += dt;
    for (int k = 0; k < NP; k++) {
        if (POSTS[k].room != G.room) continue;
        const int s = staffSim(k); if (s < 0) continue;
        Actor* a = nullptr; for (Actor& x : acts) if (x.kind == 0 && x.id == s) { a = &x; break; }
        if (!a) { Actor n; n.kind = 0; n.id = (uint16_t)s; n.x = POSTS[k].x; n.tx = n.x; n.y = 214; acts.push_back(n); a = &acts.back(); }
        a->stay = -1;
        if (POSTS[k].x2 != POSTS[k].x) {   // the waiter shuttles between the kitchen and the tables
            if (std::fabs(a->x - a->tx) < 3) a->tx = (a->tx < (POSTS[k].x + POSTS[k].x2) / 2) ? POSTS[k].x2 : POSTS[k].x;
            if (a->tx != POSTS[k].x && a->tx != POSTS[k].x2) a->tx = POSTS[k].x;
        } else { a->tx = POSTS[k].x; if (std::fabs(a->x - POSTS[k].x) > 40) a->x = POSTS[k].x; }
        if (POSTS[k].pose == PO_DANCE) { a->pose = PO_DANCE; a->poseT += dt; tweenLoop(s, G.room == R_VELVET ? TW_SPIN : TW_SWAY); }
        if (POSTS[k].x2 != POSTS[k].x) tweenLoop(s, TW_CHEER);   // the waiter's bouncy step
        if (talkT > 7.f && irand(NP) == 0 && a->sayT <= 0) { a->say = POSTS[k].lines[irand(3)]; a->sayT = 3.f; if (POSTS[k].pose != PO_DANCE) tweenPlay(s, TW_WAVE, 1.2f); }
    }
    if (talkT > 7.f) talkT = 0;
    // party guests: dance, snack, tidy and chat on their own
    if (partyOn && roomIsHome(G.room)) {
        partyT += dt;
        if (partyT > 4.f) {
            partyT = 0;
            std::vector<Actor*> guests; for (Actor& x : acts) if (x.kind == 0 && x.id < G.sims.size() && !G.sims[x.id].household) guests.push_back(&x);
            if (!guests.empty()) {
                Actor* g = guests[irand((int)guests.size())];
                static const char* DO[6] = {"*dances*", "*grabs snacks*", "*tidies up cups*", "Who made this dip?!", "*mixes a mocktail*", "Turn it UP!"};
                const int d = irand(6);
                g->say = DO[d]; g->sayT = 3.f;
                if (d == 0 || d == 5) { g->pose = PO_DANCE; g->poseT = 0; tweenPlay(g->id, d == 5 ? TW_ARMSUP : TW_SPIN, 1.2f); }
                else if (d == 3) tweenPlay(g->id, TW_CLAP, 1.f);
                else if (d == 2) tweenPlay(g->id, TW_BOW, 1.f);
                else g->tx = 40 + irand(320);
            }
        }
    }
}
