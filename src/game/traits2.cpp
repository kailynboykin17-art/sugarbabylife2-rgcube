#include "traits2.hpp"
#include "sim.hpp"
#include "extras.hpp"
#include "emote.hpp"
#include <string>
#include "lifeext.hpp"
#include <algorithm>
static const uint8_t MINST[] = {0, 0, 2, 0, 0, 0, 1, 2, 2, 0, 0, 2, 2, 2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 2, 0, 2};   // 0 any, 1 teen+, 2 young adult+
bool traitOk(int t, int st) {
    if (t < TR_FLIRTY || t >= TR_COUNT) return t >= 0 && t < TR_COUNT;
    const int m = MINST[t - TR_FLIRTY];
    return m == 0 || (m == 1 && st >= ST_TEEN) || (m == 2 && st >= ST_YA);
}
static std::string first(int s) { const std::string& n = G.sims[s].name; return n.substr(0, n.find(' ')); }
void traitsDaily() {
    Sim& me = MC();
    auto has = [&](int t) { return me.hasTrait((uint8_t)t); };
    auto sk = [&](int k, float a) { me.skills[k] = std::min(10.f, me.skills[k] + a); };
    if (has(TR_FARMER)) { sk(SK_CREATIVITY, 0.05f); lxSkill(XS_GARDEN, 3); }
    if (has(TR_BOXER)) sk(SK_FITNESS, 0.1f);
    if (has(TR_DANCER)) { sk(SK_FITNESS, 0.05f); if (irand(6) == 0) { emotePlay(G.mc, EMO_MUSIC); toast("You danced around the house for an hour. Dancer life."); } }
    if (has(TR_NERD)) sk(SK_LOGIC, 0.1f);
    if (has(TR_MAKEUP) || has(TR_VAIN)) sk(SK_STYLE, 0.08f);
    if (has(TR_HOOPS) && irand(5) == 0) { me.needs[N_FUN] = std::min(100.f, me.needs[N_FUN] + 20); toast("You shot hoops till the streetlights came on."); }
    if (has(TR_SILVERSPOON)) { G.money += 40; if (irand(7) == 0) toast("Your family's monthly \"allowance\" hit your account. Must be nice."); }
    if (has(TR_SCAMMER) && irand(5) == 0) { if (irand(3)) { const int p = 50 + irand(150); G.money += p; toast("Your latest hustle paid off. +$" + std::to_string(p)); } else { const int l = 80 + irand(120); G.money = G.money > l ? G.money - l : 0; toast("Your scheme backfired. -$" + std::to_string(l)); emotePlay(G.mc, EMO_SWEAT); } }
    if (has(TR_TRAPSTAR) && irand(4) == 0) { const int p = 40 + irand(80); G.money += p; toast("On the grind. +$" + std::to_string(p)); }
    if (has(TR_BLACKMAILER) && irand(8) == 0) { int v = -1; for (int t = 0; t < 10 && v < 0; t++) { const int i = irand((int)G.sims.size()); if (i != G.mc && rel(G.mc, i).met) v = i; }
        if (v >= 0) { G.money += 150; changeRel(G.mc, v, -12, 0); toast("You \"convinced\" " + first(v) + " to pay $150 to keep a secret quiet."); } }
    if (has(TR_PLUG) && irand(6) == 0) { G.money += 60; toast("Someone owed you a favor. The plug always gets paid. +$60"); }
    if (has(TR_BOUGIE) && irand(6) == 0) { addMoodlet(me, "Looks Expensive", EM_CONFIDENT, 1, 720); toast("Clearance rack find that looks designer. Finesse."); }
    if (has(TR_PASTOR) && irand(6) == 0) { addMoodlet(me, "Blessed", EM_HAPPY, 1, 720); toast("You led the community food drive. The neighborhood loves you."); lxRepAdd(CI_FAMILY, 3, "your ministry"); }
    if (has(TR_HOUSEWIFE)) { lxClean(8); if (irand(6) == 0) toast("The house is SPOTLESS. Housewife excellence."); }
    if (has(TR_VEGAN) && irand(6) == 0) toast("You told three people you're vegan today. Personal record.");
    if (has(TR_SHY)) me.needs[N_SOCIAL] = std::min(100.f, me.needs[N_SOCIAL] + 5);   // small doses are plenty
    if (has(TR_DIVA) && irand(6) == 0) { addMoodlet(me, "Center of Attention", EM_CONFIDENT, 1, 600); emotePlay(G.mc, EMO_HAPPY, "Diva moment"); }
    if (has(TR_QUICKTEMPER) && irand(6) == 0) { addMoodlet(me, "Snapped", EM_ANGRY, 1, 240); emotePlay(G.mc, EMO_ANGRY); toast("Someone cut you in line. 0 to 100. Real quick."); }
    if (has(TR_SHADY) && irand(7) == 0) { emotePlay(G.mc, EMO_SHOCK, "*side-eye*"); toast("You threw a slick comment. It LANDED."); }
    if (has(TR_HOODRAT) && irand(6) == 0) { addMoodlet(me, "Neighborhood Pride", EM_CONFIDENT, 1, 720); toast("Everybody on the block knows your name. Love from the neighborhood."); }
    if ((has(TR_FANGIRL) || has(TR_FANBOY)) && irand(6) == 0) { emotePlay(G.mc, EMO_SHOCK, "THEY DROPPED!"); toast("Your faves just announced something. You are NOT okay."); }
    if (has(TR_GIRLYGIRL) && irand(7) == 0) emotePlay(G.mc, EMO_LOVE, "So cute!");
    if (has(TR_PICKME) && irand(7) == 0) toast("You told the group you \"don't really get along with girls.\" Side-eyes all around.");
}
