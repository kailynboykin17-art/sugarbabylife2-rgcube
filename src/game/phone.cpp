#include "settings2.hpp"
#include "growup.hpp"
#include "lifeext.hpp"
#include "hotdate.hpp"
#include "lockup.hpp"
#include "worldsim.hpp"
#include "travelcourt.hpp"
#include "levels.hpp"
#include "health.hpp"
#include "phone.hpp"
#include "lifeplus.hpp"
#include "economy.hpp"
#include "status.hpp"
#include "lovebug.hpp"
#include "extras.hpp"
#include "crime.hpp"
#include "oc.hpp"
#include "squads.hpp"
#include "goals.hpp"
#include "draw.hpp"
#include "sim.hpp"
#include "audio.hpp"
#include "cas.hpp"
#include "chat.hpp"
#include "life.hpp"
#include "kids.hpp"
#include "family.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace col;

static int app = AP_HOME, threadIdx = 0, scroll = 0, sel = 0;
static int threadSim = -1, pendingDate = -1;
static bool famPlan = false;   // Family app: the planner page
static std::vector<int> choreKids() { std::vector<int> v; for (size_t i = 0; i < G.sims.size(); i++) if (G.sims[i].household && (int)i != G.mc && G.sims[i].stage >= ST_CHILD && v.size() < 4) v.push_back((int)i); return v; }
static NpcLife& lifeOf(int s) { if (G.life.size() < G.sims.size()) G.life.resize(G.sims.size()); return G.life[s]; }   // threads re-sort as texts arrive, so track the open one by person
static ChatThread* curThread() { for (ChatThread& t : G.threads) if (t.sim == threadSim) return &t; return nullptr; }
static void openThreadAt(int ti) {
    if (ti < 0 || ti >= (int)G.threads.size()) return;
    threadIdx = ti; threadSim = G.threads[ti].sim; G.threads[ti].unread = false;
    app = AP_THREAD;
}
static std::vector<int> swipes;
static int swipeI = 0;
static float swipeAnim = 0; static int swipeDir = 0;
static int calWeek = 0;
static int planAct = 0, planHour = 18, planDay = 0;
static int dateTarget = -1;
static std::string banner;
static float bannerT = 0;

static const int NUM_APPS = 58;
extern int g_phoneJump;
static const char* APP_NAMES[NUM_APPS] = {"Texts", "LOVEBUG", "InstaGlam", "Map", "Calendar", "Planner", "Family", "People", "Me", "Jobs", "Goals", "Scrapbook", "The Tea", "Cast", "STATUS", "ShopNow", "My Lot", "Wallet", "Yum Run", "Hotel", "Backpack", "Clubs", "Daily", "Realty", "Cheats", "Music", "Spicy 18+", "FanVault", "Chores", "Host", "Invites", "Rep & Tea", "Traditions", "Ambitions", "Body", "Bedrooms", "Skills+", "Glam Live", "TikTak", "Chemistry", "Wants", "Habits", "Promises", "Households", "Contests", "News", "Teen Life", "Group", "Events", "Odd Jobs", "Outfits", "Kid Life", "Grow Up", "Daily Life", "Photo Mode", "Dynasty", "Game Settings", "Life+"};
static const int APP_IDS[NUM_APPS] = {AP_TEXTS, AP_DATING, AP_SOCIAL, AP_MAP, AP_CAL, AP_PLANNER, AP_FAMILY, AP_PEOPLE, AP_ME, AP_JOBS, AP_GOALS, AP_SCRAPBOOK, AP_TEA, AP_CAST, AP_STATUS, AP_SHOP, AP_LOT, AP_WALLET, AP_YUM, AP_HOTEL, -101, -102, -103, -104, -105, -106, -107, -108, -109, -110, -111, -112, -113, -114, -115, -116, -117, -118, -119, -120, -121, -122, -123, -124, -125, -126, -127, -128, -129, -130, -131, -132, -133, -134, -135, -136, -138, -139};
static const RGB APP_COLS[NUM_APPS] = {{120, 220, 140}, {255, 70, 120}, {200, 120, 230}, {120, 190, 240}, {255, 180, 90},
                                 {240, 210, 120}, {255, 160, 190}, {250, 120, 160}, {170, 200, 230}, {200, 170, 130}, {64, 196, 214}, {255, 150, 120}, {255, 46, 140}, {150, 96, 230}, {110, 80, 240}, {255, 130, 60}, {90, 180, 150}, {60, 170, 110}, {240, 60, 90}, {214, 160, 80}, {170, 120, 90}, {236, 90, 170}, {90, 170, 230}, {60, 150, 120}, {40, 40, 50}, {255, 90, 200}, {200, 30, 70}, {40, 170, 240}, {120, 200, 120}, {255, 150, 60}, {90, 200, 220}, {180, 90, 200}, {200, 150, 100}, {230, 70, 50}, {240, 120, 160}, {150, 120, 200}, {70, 160, 200}, {230, 60, 120}, {20, 20, 24}, {255, 80, 120}, {250, 190, 60}, {110, 190, 140}, {120, 140, 230}, {200, 120, 80}, {250, 200, 40}, {90, 90, 100}, {255, 110, 200}, {100, 200, 255}, {255, 170, 90}, {120, 210, 170}, {240, 140, 200}, {255, 200, 90}, {140, 200, 255}, {255, 140, 120}, {255, 120, 200}, {200, 160, 60}, {120, 130, 150}, {255, 110, 170}};
static const int MAP_N = 16;
static const int MAP_ROOMS[MAP_N] = {R_LIVING, R_BEACH, R_SCHOOL, R_CLUB, R_RESTAURANT, R_BOUTIQUE, R_COLLEGE, R_CAFE, R_SPA, R_MALL, R_PARK, R_CINEMA, R_RINK, R_STREET, R_HOSPITAL, R_MARKET};
static const float MAP_POS[MAP_N][2] = {{60, 72}, {330, 200}, {150, 56}, {300, 64}, {220, 124}, {110, 150}, {240, 36}, {50, 196}, {350, 124}, {160, 196}, {250, 186}, {370, 44}, {70, 124}, {140, 100}, {300, 160}, {362, 166}};
static int mapPage = 0;
static const int MAP2[14] = {R_MARKET, R_CORNER, R_THRIFT, R_TENNIS, R_LIBRARY, R_GYM, R_CLUB, R_MALL, R_HOTEL, R_PETSHOP, R_CHAPEL, R_SUSHI, R_HAUNTED, R_FRAT};
static int rhOpen = -1;   // rabbit hole whose activity list is showing
static int mapRoom(int i) {   // -2 next page, -3 previous page, <= -10 rabbit hole
    { int R[8]; int n = vacationMapRooms(R); if (n) return i < n ? R[i] : -1; }   // on a trip: the destination's own map
    if (mapPage == 0) return i == 15 ? -2 : MAP_ROOMS[i];
    if (mapPage == 1) return i == 15 ? -2 : i == 14 ? -3 : i < 14 ? MAP2[i] : -1;
    static const int NEON[4] = {R_GLOWBOWL, R_FUNPARK, R_DANCESTUDIO, R_DOWNTOWN};
    static const int METRO[11] = {R_SUBWAY, R_PIXELARCADE, R_LAUNDRO, R_AQUARIUM, R_MUSEUM, R_GARDENS, R_ROOFTOP, R_KARAOKE, R_AVSQUARE, R_AVFARM, R_AVBARN};   // + Apple Valley farm line
    if (mapPage == 3) return i == 15 ? -3 : i == 14 ? -2 : i < 11 ? METRO[i] : -1;
    static const int TOWN[14] = {R_FOODCOURT, R_HALLWAY, R_DAYCARE, R_CHURCH, R_COMMUNITY, R_POOL, R_BLOCKPARTY, R_DORM, R_COLLEGE, R_SALON, R_HOSPITAL, R_ROLLER, R_NIGHTMARKET, R_COURTHOUSE};
    if (mapPage == 4) return i == 15 ? -3 : i < 14 ? TOWN[i] : -1;
    return i == 15 ? -3 : i == 14 ? -2 : i < RH_COUNT ? -10 - i : i < RH_COUNT + 4 ? NEON[i - RH_COUNT] : -1;
}
static Btn mapB(int i) { return i == 15 ? Btn{214, 196, 101, 30} : Btn{4 + (i % 3) * 105.f, 26 + (i / 3) * 34.f, 101, 30}; }
static const char* PLAN_NAMES[6] = {"Gym Session", "Shopping Spree", "Study Time", "Spa Day", "Brunch w/ Besties", "Party!"};

int phoneUnread() { int n = 0; for (const ChatThread& t : G.threads) if (t.unread) n++; return n; }

void phoneOpen(int a) {
    app = a; scroll = 0; sel = 0;
    if (a == AP_DATING) { swipes = swipeCandidates(); swipeI = 0; }
    planDay = G.day;
}

static bool placeOpen(int room, std::string* why) {
    int h = (int)(G.minute / 60);
    const Sim& me = MC();
    if (room == R_LIVING) return true;
    if (!lvlRoomOk(room, why)) return false;   // Levels mode locks
    if (!vacationOpen(room, why)) return false;
    if (lockupBlocks(room, why)) return false;
    if (me.isLittle()) { if (why) *why = "Too little to go out alone!"; return false; }
    if (room == R_CLUB) { if (!me.isAdult()) { if (why) *why = "Club Glitz is 18+ only!"; return false; } if (h < 18 && h >= 3) { if (why) *why = "Opens at 6 PM"; return false; } return true; }
    if (room >= R_HALLWAY && room <= R_SCHOOLGYM) { if (h < 7 || h >= 17) { if (why) *why = "School is closed"; return false; } return true; }
    if (room == R_DAYCARE && (h < 7 || h >= 18)) { if (why) *why = "Daycare is closed (7 AM - 6 PM)"; return false; }
    if (room == R_SCHOOL) { if (h < 7 || h >= 17) { if (why) *why = "School is closed"; return false; } return true; }
    if (room == R_CINEMA && (h < 12 && h >= 2)) { if (why) *why = "Opens at noon"; return false; }
    if ((room == R_MALL || room == R_RINK) && (h < 9 || h >= 22)) { if (why) *why = "Open 9 AM - 10 PM"; return false; }
    if (room == R_PARK && (h < 6 || h >= 23)) { if (why) *why = "Park closed at night"; return false; }
    if (room == R_HOSPITAL) return true;   // Uptown: the hospital never closes
    if (h < 7 && h >= 2) { if (why) *why = "Closed (opens 7 AM)"; return false; }
    return true;
}

// ------------------------------------------------------------- layout
static const int APPS_P1 = 23;   // page 1: core apps; page 2: Realty, Cheats, Music, Spicy 18+, FanVault, Chores
static int appPage = 0;
// page 1 = the 23 core apps; later pages hold 24 each; slot 24 is always the page button
static int pageFirst(int p) { return p == 0 ? 0 : APPS_P1 + (p - 1) * 24; }
static int pageEnd(int p) { return std::min(NUM_APPS, p == 0 ? APPS_P1 : pageFirst(p) + 24); }
static int appPages() { return 2; }   // page 1 = core apps, page 2 = folders
// app folders (indices into APP_NAMES)
static const char* FOLDER_NAMES[7] = {"Family", "School", "Social", "Money", "Style", "18+", "Life"};
static const RGB FOLDER_COLS[7] = {{255, 150, 120}, {120, 170, 255}, {200, 120, 230}, {90, 190, 120}, {255, 110, 180}, {200, 30, 70}, {250, 190, 60}};
static const int8_t FOLDERS[7][12] = {
    {28, 43, 35, 32, 51, 52, 55, -1}, {46, 36, 47, 51, 52, -1}, {57, 29, 30, 31, 37, 38, 39, 42, 48, 45, -1},
    {23, 49, 33, 44, 24, 53, -1}, {54, 50, 34, 25, -1}, {26, 27, -1}, {57, 56, 55, 53, 52, 40, 41, 25, -1}};
static int openFolder = -1;
static int folderLen(int f) { int n = 0; while (n < 12 && FOLDERS[f][n] >= 0) n++; return n; }
static const char* APP_TIPS[NUM_APPS] = {
    "Texts: read & reply to messages from friends and family.", "LOVEBUG: swipe right to match, then text or date.", "InstaGlam: post photos and gain followers.",
    "Map: travel anywhere. Tap Next > for more pages (Metro & Town).", "Calendar: plan holidays, events & getaways.", "Planner: set your day's plan.",
    "Family: see your household and relationships.", "People: everyone in town and how close you are.", "Me: your stats, traits & skills.",
    "Jobs: get hired, enroll in college, go to work.", "Goals: life goals & wishes.", "Scrapbook: your saved memories.", "The Tea: town gossip.",
    "Cast: edit sims in Create-a-Sim.", "STATUS: social feed.", "ShopNow: buy outfits & items.", "My Lot: build & decorate.", "Wallet: money & bills.",
    "Yum Run: order food delivery.", "Hotel: book a stay.", "Backpack: your inventory.", "Clubs: join or start a club.", "Daily: daily goals.",
    "Realty: buy, sell & move homes.", "Cheats: money & needs cheats.", "Music: change the background song.", "Spicy 18+: adult content settings (adults only).",
    "FanVault: creator app (18+ adults only).", "Chores: do chores to clean the house.", "Host: throw parties & sleepovers.", "Invites: RSVP to NPC events.",
    "Rep & Tea: your reputation in each circle.", "Traditions: family holiday traditions.", "Ambitions: career levels & work scenes.", "Body: body, cycles, tattoos & birth control.",
    "Bedrooms: assign up to 4 bedrooms.", "Skills+: 12 extra skills.", "Glam Live: go live and earn tips.", "TikTak: post short videos.",
    "Chemistry: romance compatibility & turn-ons.", "Wants: wants & fears.", "Habits: good & bad habits.", "Promises: promises & lies.", "Households: switch families.",
    "Contests: enter town contests.", "News: the local news feed.", "Teen Life: teen hangouts & rebellion.", "Group: bring friends along.", "Events: social events & weddings.",
    "Odd Jobs: 7 playable job minigames.", "Outfits: outfits by category.", "Kid Life: toys, first crush, school & the purse toggle.",
    "Grow Up: family life, teens, school, story mode, difficulty & checklist.",
    "Daily Life: fridge & recipes, sick days, likes & dislikes, BFFs, exes, business, loans & holidays.",
    "Photo Mode: hide the HUD, pose sims, add filters & frames, then SNAP a real photo to your SD card.",
    "Dynasty: 10-generation legacy scoring, random life events, and rotational play between households.",
    "Game Settings: switch systems on/off, How to Play, and What's New.",
    "Life+: minigames, aspirations, bucket list, challenges, cliques, nemesis, secrets, events, vacations."};
static Btn appB(int i) { return Btn{6 + (i % 5) * 62.f, 26 + (i / 5) * 36.f, 58, 34}; }
static const Btn bBack{4, 208, 76, 28}, bHomeB{122, 210, 76, 26};
static int jobPage = 0;
static Btn listB(int i) { return Btn{6, 30 + i * 34.f, 308, 30}; }
static Btn half(int i, float y0 = 30, float h = 30) { return Btn{6 + (i % 2) * 156.f, y0 + (i / 2) * (h + 4), 152, h}; }

static void say2(const std::string& s) { banner = s; bannerT = 2.5f; }
static std::vector<int> peopleList() {
    std::vector<const Rel*> list;
    for (const Rel& r : G.rels) if ((r.a == G.mc || r.b == G.mc) && r.met && r.a < G.sims.size() && r.b < G.sims.size()) list.push_back(&r);
    std::stable_sort(list.begin(), list.end(), [](const Rel* a, const Rel* b) { return (a->fr + a->ro) > (b->fr + b->ro); });
    std::vector<int> v;
    for (const Rel* r : list) v.push_back(r->a == G.mc ? r->b : r->a);
    return v;
}
void phoneOpenThread(int sim) {
    threadFor(sim);
    for (size_t i = 0; i < G.threads.size(); i++) if (G.threads[i].sim == sim) openThreadAt((int)i);
    if (pendingDate == sim) { pendingDate = -1; dateTarget = sim; app = AP_DATELOC; }
}
void phoneDatePick(int sim) { phoneOpenThread(sim); dateTarget = sim; app = AP_DATELOC; }
void phoneDatePickLater(int sim) {
    if (app == AP_THREAD && threadSim == sim) { dateTarget = sim; app = AP_DATELOC; return; }
    pendingDate = sim;
    toast(G.sims[sim].name + " said yes! Open their texts to pick a place.");
}

// ------------------------------------------------------------- update
bool phoneUpdate(const Input& in) {
    if (bannerT > 0) bannerT -= in.dt;
    if (swipeAnim > 0) swipeAnim -= in.dt * 3;
    if (in.down & (KEY_X)) return false;
    bool back = (in.down & KEY_B) || (in.touch && bBack.in(in.tp));
    if (back && app == AP_FAMILY && famPlan) { famPlan = false; sfxTap(); return true; }
    if (back) {
        if (app == AP_HOME) return false;
        if (app == AP_THREAD) app = AP_TEXTS;
        else if (app == AP_DATELOC) app = AP_THREAD;
        else app = AP_HOME;
        sfxTap();
        return true;
    }
    if (in.down & KEY_UP) scroll = std::max(0, scroll - 1);
    if (in.down & KEY_DOWN) scroll++;
    const touchPosition& tp = in.tp;
    Sim& me = MC();
    switch (app) {
    case AP_HOME:
        if (in.touch && appB(24).in(tp)) { if (openFolder >= 0) openFolder = -1; else appPage = (appPage + 1) % appPages(); sfxTap(); break; }
        if (in.down & KEY_L) { openFolder = -1; appPage = (appPage + appPages() - 1) % appPages(); break; }
        if (in.down & KEY_R) { openFolder = -1; appPage = (appPage + 1) % appPages(); break; }
        if (in.down & KEY_B && openFolder >= 0) { openFolder = -1; break; }
        if (in.touch) {
            auto launch = [&](int i) -> bool {
                sfxTap();
                if (!guTipSeen(i)) { guTipMark(i); if (gsOn(GS_TIPS)) toast(APP_TIPS[i]); }
                if (APP_IDS[i] < 0) { g_phoneJump = -100 - APP_IDS[i]; return true; }
                phoneOpen(APP_IDS[i]); return false;
            };
            if (appPage == 0) { for (int i = 0; i < APPS_P1; i++) if (appB(i).in(tp)) { if (launch(i)) return false; break; } }
            else if (openFolder < 0) { for (int f = 0; f < 7; f++) if (appB(f).in(tp)) { openFolder = f; sfxTap(); break; } }
            else for (int k = 0; k < folderLen(openFolder); k++) if (appB(k).in(tp)) { if (launch(FOLDERS[openFolder][k])) return false; break; }
        }
        break;
    case AP_TEXTS:
        if (in.touch)
            for (int i = 0; i < 5; i++) {
                int ti = scroll + i;
                if (ti < (int)G.threads.size() && listB(i).in(tp)) { int who = G.threads[ti].sim; openThreadAt(ti); if (pendingDate == who) { pendingDate = -1; dateTarget = who; app = AP_DATELOC; } sfxTap(); }
            }
        scroll = std::min(scroll, std::max(0, (int)G.threads.size() - 5));
        break;
    case AP_THREAD: {
        ChatThread* thp = curThread();
        if (!thp) { app = AP_TEXTS; break; }
        ChatThread& th = *thp;
        th.unread = false;
        if (chatLive(th.prompt) && chatChoices(th).empty()) th.prompt = -1;
        if (!in.touch) break;
        const int who = th.sim;
        if (chatLive(th.prompt)) {
            auto v = chatChoices(th);
            for (int i = 0; i < (int)v.size() && i < 5; i++) if (listB(i).in(tp)) { sfxTap(); chatChoose(th, i); break; }
        } else if (th.prompt >= 0 && th.prompt < NUM_PROMPTS) {
            for (int i = 0; i < 3; i++) if (listB(i).in(tp)) { answerPrompt(th, i); sfxTap(); }
        } else {
            if (!chatWaiting(who)) {
                auto v = chatStarters(who);
                for (int i = 0; i < (int)v.size() && i < 8; i++) if (half(i, 30, 26).in(tp)) { sfxTap(); chatStart(who, v[i].tone); return true; }
            }
            for (int i = 0; i < 4; i++)
                if (Btn{6 + i * 77.f, 150, 74, 26}.in(tp)) {
                    sfxTap();
                    if (i == 0) { std::string msg; if (askText("Type a message...", msg, 80)) chatTyped(who, msg); }
                    else if (i == 3) {
                        if (!canRomance(G.mc, who)) { say2(me.isAdult() ? "Not a romance option" : "Dating unlocks at Young Adult!"); sfxNo(); }
                        else { dateTarget = who; app = AP_DATELOC; }
                    } else sendText(who, i);   // 1 selfie, 2 invite over
                }
        }
        break;
    }
    case AP_DATELOC: {
        static const int locs[4] = {R_RESTAURANT, R_CLUB, R_BEACH, R_CAFE};
        if (in.touch) for (int i = 0; i < 4; i++) if (half(i).in(tp)) { askOnDate(dateTarget, locs[i]); app = AP_THREAD; }
        break;
    }
    case AP_DATING: lovebugUpdate(in); break;
    case 9999: {
        if (!me.isAdult() || swipeI >= (int)swipes.size()) break;
        int dir = 0;
        if ((in.down & (KEY_RIGHT | KEY_R | KEY_A)) || (in.touch && half(1, 168, 36).in(tp))) dir = 1;
        if ((in.down & (KEY_LEFT | KEY_L)) || (in.touch && half(0, 168, 36).in(tp))) dir = -1;
        if (!dir) break;
        if (dir > 0) {
            if (swipeRight(swipes[swipeI])) { say2("IT'S A MATCH! Check Texts <3"); sfxYay(); fxBurst(200, 100, 25); }
            else { say2("Liked! No match... yet"); sfxTap(); }
        } else sfxTap();
        swipeDir = dir; swipeAnim = 1; swipeI++;
        break;
    }
    case AP_SOCIAL:
        if (in.touch) for (int i = 0; i < 6; i++) if (half(i, 96, 30).in(tp)) { makePost(i); sfxTap(); }
        scroll = std::min(scroll, std::max(0, (int)G.posts.size() - 1));
        break;
    case AP_MAP:
        if (rhOpen >= 0) {
            if (in.down & KEY_B) { rhOpen = -1; break; }
            if (!in.touch) break;
            auto opts = rabbitHoleOptions(rhOpen);
            for (int k = 0; k < (int)opts.size(); k++) if (Btn{20, 60 + k * 34.f, 280, 30}.in(tp)) {
                if (opts[k].second < 0) { sfxNo(); break; }
                sfxTap(); bool go = rabbitHoleDo(rhOpen, opts[k].second); if (go) { rhOpen = -1; return false; } break;
            }
            if (Btn{214, 196, 101, 30}.in(tp)) { rhOpen = -1; sfxTap(); }
            break;
        }
        if (in.touch)
            for (int i = 0; i < MAP_N; i++)
                if (mapB(i).in(tp)) {
                    int target = mapRoom(i);
                    if (target == -1) continue;
                    if (target == -2 || target == -3) { mapPage = std::max(0, std::min(4, mapPage + (target == -2 ? 1 : -1))); sfxTap(); break; }
                    if (target <= -10) { rhOpen = -10 - target; sfxTap(); break; }
                    std::string why;
                    if (!placeOpen(target, &why)) { say2(why); sfxNo(); break; }
                    if (target == R_LIVING && roomIsHome(G.room)) { sfxTap(); g_phoneJump = 37; return false; }   // dollhouse view
                    if (!hotdateTaxi(target)) break;
                    if (mapPage == 3) { if (G.money < 3) { say2("Subway fare is $3."); sfxNo(); break; } G.money -= 3; }
                    travelTo(target);
                    sfxYay();
                    return false;
                }
        break;
    case AP_CAL:
        if (lxCalUpdate(in)) { g_phoneJump = 40; return false; }
        break;
        if (in.down & (KEY_LEFT | KEY_L)) calWeek--;
        if (in.down & (KEY_RIGHT | KEY_R)) calWeek++;
        if (in.touch && Btn{6, 170, 70, 28}.in(tp)) calWeek--;
        if (in.touch && Btn{244, 170, 70, 28}.in(tp)) calWeek++;
        calWeek = clampv(calWeek, -(int)(G.day / 7), 20);
        break;
    case AP_PLANNER:
        if (!in.touch) break;
        for (int i = 0; i < 6; i++) if (half(i, 28, 26).in(tp)) { planAct = i; sfxTap(); }
        if (Btn{6, 124, 40, 26}.in(tp)) planDay = std::max<int>(G.day, planDay - 1);
        if (Btn{120, 124, 40, 26}.in(tp)) planDay = std::min<int>(G.day + 13, planDay + 1);
        if (Btn{166, 124, 40, 26}.in(tp)) planHour = std::max(6, planHour - 1);
        if (Btn{274, 124, 40, 26}.in(tp)) planHour = std::min(23, planHour + 1);
        if (Btn{100, 160, 120, 30}.in(tp)) {
            Event e; e.day = (uint16_t)planDay; e.hour = (uint8_t)planHour; e.type = EV_PLAN; e.text = PLAN_NAMES[planAct];
            if (e.day == G.day && planHour <= (int)(G.minute / 60)) { say2("That time already passed!"); sfxNo(); break; }
            G.events.push_back(e);
            say2("Added to your Calendar!"); sfxYay();
        }
        break;
    case AP_FAMILY: {
        std::vector<int> fam;
        for (size_t i = 0; i < G.sims.size(); i++) if (G.sims[i].household) fam.push_back((int)i);
        if (!in.touch) break;
        if (famPlan) {
            if (Btn{236, 4, 80, 22}.in(tp)) { famPlan = false; sfxTap(); break; }
            if (Btn{124, 28, 30, 24}.in(tp)) { G.curfew = (uint8_t)std::max(19, G.curfew - 1); sfxTap(); }
            if (Btn{246, 28, 30, 24}.in(tp)) { G.curfew = (uint8_t)std::min(23, G.curfew + 1); sfxTap(); }
            auto kids = choreKids();
            for (size_t i = 0; i < kids.size(); i++) if (Btn{150, 58 + i * 24.f, 164, 22}.in(tp)) { NpcLife& k = lifeOf(kids[i]); k.chore = (uint8_t)((k.chore + 1) % CHR_COUNT); sfxTap(); }
            for (int e = 0; e < FEV_COUNT; e++) if (Btn{4 + (e % 3) * 105.f, 158 + (e / 3) * 25.f, 102, 23}.in(tp)) { G.famEvents ^= (uint8_t)(1 << e); sfxTap(); }
            if (Btn{204, 210, 112, 26}.in(tp)) { G.allowance = (uint8_t)((G.allowance + 1) % 4); sfxTap(); }
            break;
        }
        if (Btn{236, 4, 80, 22}.in(tp)) { famPlan = true; sfxTap(); break; }
        for (int i = 0; i < 5; i++) {
            int k = scroll + i;
            if (k < (int)fam.size() && listB(i).in(tp)) {
                int idx = fam[k];
                if (idx == G.mc) { say2("You're already playing as " + G.sims[idx].name); break; }
                cancelActivity();
                G.mc = idx;
                for (int r = 0; r < R_COUNT; r++)
                    for (const Actor& a : G.rooms[G.house][r].actors)
                        if (a.kind == 0 && a.id == idx) G.room = r;
                say2("Now playing as " + G.sims[idx].name + "!"); sfxYay();
                return false;
            }
        }
        if (Btn{204, 208, 112, 28}.in(tp)) {
            if (!me.isAdult()) { say2("Only adults can adopt."); sfxNo(); break; }
            if (G.money < 500) { say2("Adoption costs $500."); sfxNo(); break; }
            G.money -= 500;
            int b = makeBaby(-1, -1);
            say2("Welcome home, " + G.sims[b].name + "! (Nursery)"); sfxYay(); fxBurst(200, 100, 30);
        }
        break;
    }
    case AP_PEOPLE: {
        auto v = peopleList();
        scroll = std::min(scroll, std::max(0, (int)v.size() - 1));
        if (in.touch && !v.empty() && Btn{110, 172, 100, 28}.in(tp)) { sfxTap(); phoneOpenThread(v[scroll]); }
        break;
    }
    case AP_ME: break;
    case AP_GOALS: goalsAppUpdate(in); break;
    case AP_SCRAPBOOK: scrapbookUpdate(in); break;
    case AP_TEA: teaAppUpdate(in); break;
    case AP_CAST: ocAppUpdate(in); break;
    case AP_STATUS: statusAppUpdate(in); break;
    case AP_SHOP: shopAppUpdate(in); break;
    case AP_LOT: lotAppUpdate(in); break;
    case AP_WALLET: walletAppUpdate(in); break;
    case AP_YUM: yumAppUpdate(in); break;
    case AP_HOTEL: hotelAppUpdate(in); break;
    case AP_JOBS:
        if (!in.touch) break;
        if (Btn{240, 208, 76, 28}.in(tp)) { jobPage = (jobPage + 1) % ((CR_COUNT - 1 + 3) / 4); sfxTap(); break; }
        for (int i = 1 + jobPage * 4; i < CR_COUNT && i < 5 + jobPage * 4; i++)
            if (listB(i - 1 - jobPage * 4).in(tp)) {
                const CareerDef& c = CAREERS[i];
                if (me.career == i) { me.career = CR_NONE; say2("You quit your job."); break; }
                if (i == CR_CELEBRITY && fameStars() < 2) { say2("Celebrity needs 2+ fame stars (5,000 followers). Get famous first!"); sfxNo(); break; }
                if (me.stage < ST_TEEN || (!c.teenOk && !me.isAdult())) { say2(me.stage < ST_TEEN ? "Too young to work!" : "Adults only (teens: Barista, Hostess, Courier)"); sfxNo(); break; }
                joinCareer(me, i); sfxYay();
            }
        if (listB(4).in(tp) && me.isAdult()) {
            if (me.enrolled) say2(me.degree ? "You already graduated!" : "Already enrolled! Attend class at the college.");
            else if (G.money < 300) { say2("Tuition is $300"); sfxNo(); }
            else { G.money -= 300; me.enrolled = true; say2("Enrolled at Starlight College!"); sfxYay(); }
        }
        break;
    }
    return true;
}

// ------------------------------------------------------------- drawing helpers
static void bar(float x, float y, float w, float v, RGB c) {
    rrect(x, y, w, 8, 4, C(UI2));
    rrect(x + 1, y + 1, (w - 2) * clampv(v, 0.f, 100.f) / 100.f, 6, 3, C(c));
}
static RGB needCol(float v) { return v > 60 ? RGB{110, 220, 130} : v > 30 ? RGB{240, 210, 80} : RGB{240, 80, 90}; }

static void phoneFrameTop(const char* title) {
    grad(0, 0, TOP_W, TOP_H, C(255, 218, 232), C(255, 192, 214));
    for (int i = 0; i < 6; i++) heart(30 + i * 70, 226, 5, C(255, 255, 255, 90));
    rect(0, 0, TOP_W, 22, C(UI1));
    text(title, 8, 3, 0.55f, C(BLUSH));
    char b[48];
    snprintf(b, sizeof b, "%s  %s", dateStr(G.day).c_str(), clockStr(G.minute).c_str());
    text(b, 392, 4, 0.45f, C(CREAM), 2);
}

static void drawAppIcon(int i, float cx, float cy, float r) {
    u32 w = C(255, 255, 255, 230);
    switch (i) {
    case 20: rrect(cx - r * 0.8f, cy - r * 0.6f, r * 1.6f, r * 1.6f, 3, w); rect(cx - r * 0.4f, cy - r, r * 0.8f, r * 0.5f, w); rect(cx - r * 0.5f, cy, r, r * 0.4f, C(APP_COLS[20])); break;
    case 21: circle(cx - r * 0.5f, cy - r * 0.2f, r * 0.45f, w); circle(cx + r * 0.5f, cy - r * 0.2f, r * 0.45f, w); circle(cx, cy - r * 0.6f, r * 0.45f, w); rect(cx - r, cy + r * 0.3f, r * 2, r * 0.5f, w); break;
    case 22: rrect(cx - r * 0.8f, cy - r * 0.9f, r * 1.6f, r * 1.8f, 2, w); for (int k = 0; k < 3; k++) rect(cx - r * 0.5f, cy - r * 0.5f + k * r * 0.5f, r, r * 0.2f, C(APP_COLS[22])); break;
    case 19: rect(cx - r * 0.7f, cy - r * 0.6f, r * 1.4f, r * 1.6f, w); for (int k = 0; k < 3; k++) for (int q = 0; q < 2; q++) rect(cx - r * 0.45f + q * r * 0.55f, cy - r * 0.4f + k * r * 0.45f, r * 0.3f, r * 0.25f, C(APP_COLS[19])); tri(cx - r, cy - r * 0.6f, cx + r, cy - r * 0.6f, cx, cy - r * 1.1f, w); break;
    case 0: rrect(cx - r, cy - r * 0.7f, r * 2, r * 1.3f, 5, w); tri(cx - r * 0.4f, cy + r * 0.5f, cx, cy + r * 0.5f, cx - r * 0.6f, cy + r, w); break;
    case 1: heart(cx, cy, r * 0.8f, w); break;
    case 2: rrect(cx - r, cy - r, r * 2, r * 2, 5, w); circle(cx, cy, r * 0.55f, C(APP_COLS[2])); circle(cx, cy, r * 0.35f, w); break;
    case 3: circle(cx, cy - r * 0.3f, r * 0.7f, w); tri(cx - r * 0.6f, cy, cx + r * 0.6f, cy, cx, cy + r, w); circle(cx, cy - r * 0.3f, r * 0.3f, C(APP_COLS[3])); break;
    case 4: rect(cx - r, cy - r * 0.8f, r * 2, r * 1.8f, w); rect(cx - r, cy - r * 0.8f, r * 2, r * 0.5f, C(RED)); break;
    case 5: rect(cx - r * 0.8f, cy - r, r * 1.6f, r * 2, w); for (int k = 0; k < 3; k++) rect(cx - r * 0.5f, cy - r * 0.5f + k * r * 0.5f, r, 2, C(APP_COLS[5])); break;
    case 6: circle(cx - r * 0.45f, cy - r * 0.3f, r * 0.4f, w); circle(cx + r * 0.45f, cy - r * 0.3f, r * 0.4f, w); ellipse(cx, cy + r * 0.5f, r * 2, r, w); break;
    case 7: circle(cx, cy - r * 0.3f, r * 0.45f, w); ellipse(cx, cy + r * 0.55f, r * 1.4f, r * 0.8f, w); heart(cx + r * 0.7f, cy - r * 0.7f, r * 0.3f, C(HOT)); break;
    case 8: sparkle(cx, cy, r, w); break;
    case 9: rrect(cx - r, cy - r * 0.4f, r * 2, r * 1.3f, 3, w); rect(cx - r * 0.4f, cy - r * 0.8f, r * 0.8f, r * 0.4f, w); break;
    case 10: sparkle(cx, cy - r * 0.1f, r * 1.1f, w); circle(cx, cy - r * 0.1f, r * 0.35f, C(GOLD)); break;
    case 13: circle(cx, cy - r * 0.35f, r * 0.38f, w); quad(cx - r * 0.6f, cy + r * 0.8f, cx + r * 0.6f, cy + r * 0.8f, cx + r * 0.4f, cy + r * 0.1f, cx - r * 0.4f, cy + r * 0.1f, w); sparkle(cx + r * 0.7f, cy - r * 0.7f, r * 0.3f, w); break;
    case 15: rrect(cx - r, cy - r * 0.6f, r * 2, r * 1.5f, 3, w); line(cx - r * 0.5f, cy - r * 0.6f, cx - r * 0.2f, cy - r * 1.1f, 2, w); line(cx + r * 0.5f, cy - r * 0.6f, cx + r * 0.2f, cy - r * 1.1f, 2, w); break;
    case 17: rrect(cx - r, cy - r * 0.7f, r * 2, r * 1.4f, 3, w); rect(cx - r, cy - r * 0.3f, r * 2, r * 0.3f, C(APP_COLS[17])); circle(cx + r * 0.5f, cy + r * 0.3f, r * 0.25f, C(APP_COLS[17])); break;
    case 18: ellipse(cx, cy + r * 0.2f, r * 2, r * 1.2f, w); rect(cx - r, cy + r * 0.2f, r * 2, r * 0.6f, w); line(cx - r * 0.3f, cy - r, cx - r * 0.3f, cy - r * 0.3f, 2, w); line(cx + r * 0.3f, cy - r, cx + r * 0.3f, cy - r * 0.3f, 2, w); break;
    case 16: tri(cx - r, cy, cx, cy - r, cx + r, cy, w); rect(cx - r * 0.75f, cy, r * 1.5f, r * 0.9f, w); rect(cx - r * 0.2f, cy + r * 0.3f, r * 0.4f, r * 0.6f, C(APP_COLS[16])); break;
    case 14: rrect(cx - r, cy - r * 0.8f, r * 2, r * 1.4f, 5, w); tri(cx - r * 0.5f, cy + r * 0.55f, cx - r * 0.1f, cy + r * 0.55f, cx - r * 0.6f, cy + r, w); textOutline("S", cx, cy - r * 0.75f, r / 18.f, C(APP_COLS[14]), w, 1, 0.5f); break;
    case 12: rrect(cx - r * 0.7f, cy - r * 0.2f, r * 1.4f, r * 1.1f, 3, w); line(cx + r * 0.7f, cy, cx + r, cy + r * 0.3f, 2, w); for (int k = 0; k < 3; k++) line(cx - r * 0.3f + k * r * 0.3f, cy - r * 0.4f, cx - r * 0.2f + k * r * 0.3f, cy - r, 1.5f, w); break;
    case 11: rrect(cx - r * 0.9f, cy - r, r * 1.8f, r * 2, 3, w); rect(cx - r * 0.6f, cy - r * 0.7f, r * 1.2f, r * 0.9f, C(APP_COLS[11])); heart(cx, cy + r * 0.55f, r * 0.3f, C(HOT)); break;
    }
}

// ------------------------------------------------------------- top screen
void phoneDrawTop(float t) {
    Sim& me = MC();
    char b[128];
    switch (app) {
    case AP_HOME: {
        phoneFrameTop("My Phone");
        // flip-phone wallpaper of your sim
        rrect(130, 30, 140, 200, 18, C(HOT)); rrect(138, 38, 124, 160, 8, C(UI1));
        for (int i = 0; i < 3; i++) swirl(160 + i * 40, 180, 10, 1.4f, 1.2f, C(CHAMP, 60), i % 2);
        drawSim(me, 200, 196, 1.15f, t, G.holiday, PO_PHONE, false);
        text(clockStr(G.minute).c_str(), 200, 42, 0.6f, C(CREAM), 1);
        drawWeatherIcon(168, 72); text(WEATHER_NAMES[G.weather % WX_COUNT], 180, 64, 0.42f, C(CREAM));
        sparkle(250, 60, 4 + std::sin(t * 4) * 2, C(GOLD));
        heart(200, 214, 6, C(BLUSH));
        snprintf(b, sizeof b, "$%d", (int)G.money); text(b, 20, 40, 0.6f, C(BERRY));
        snprintf(b, sizeof b, "%u followers", (unsigned)G.followers); text(b, 20, 62, 0.45f, C(UI1));
        for (int i = 0; i < 5; i++) sparkle(28 + i * 16, 90, 6, C(i < fameStars() ? GOLD : RGB{255, 255, 255}, i < fameStars() ? 255 : 120));
        text(fameTitle(), 20, 102, 0.45f, C(UI1));
        int u = phoneUnread();
        if (u) { snprintf(b, sizeof b, "%d unread text%s!", u, u > 1 ? "s" : ""); text(b, 290, 40, 0.45f, C(HOT)); }
        int shown = 0;
        for (const Event& e : G.events) {
            if (e.done || e.day < G.day || shown >= 4) continue;
            snprintf(b, sizeof b, "%s %d:00 %s", WEEKDAY_NAMES[weekday(e.day)], e.hour > 12 ? e.hour - 12 : e.hour, e.text.c_str());
            text(b, 286, 70 + shown * 16.f, 0.36f, C(UI1));
            shown++;
        }
        if (shown) text("Coming up:", 286, 58, 0.38f, C(BERRY));
        break;
    }
    case AP_TEXTS: case AP_THREAD: case AP_DATELOC: {
        phoneFrameTop("Texts");
        if (app == AP_TEXTS || !curThread()) {
            if (G.threads.empty()) text("No texts yet. Meet people & get their number!", 200, 110, 0.5f, C(UI1), 1);
            else text("Tap a conversation below", 200, 110, 0.55f, C(UI1), 1);
            break;
        }
        ChatThread& th = *curThread();
        const Sim& o = G.sims[th.sim];
        drawSim(o, 40, 230, 0.75f, t, 0, PO_PHONE, false);
        text(o.name.c_str(), 40, 26, 0.5f, C(BERRY), 1);
        text(relLabel(G.mc, th.sim), 40, 42, 0.4f, C(UI1), 1);
        { std::string where = lifeStatusLine(th.sim); if (where.size() > 30) where = where.substr(0, 30); text(where.c_str(), 8, 56, 0.32f, C(UI1)); }
        float y = 214;
        if (chatTyping(th.sim)) {   // they're typing...
            y -= 26;
            rrect(86, y, 46, 22, 8, C(255, 255, 255));
            for (int k = 0; k < 3; k++) circle(98 + k * 11.f, y + 11 + std::sin(t * 8 + k) * 2, 3, C(UI1));
        } else if (!th.msgs.empty() && th.msgs.back().me && th.msgs.back().text != "[Seen]") {
            text(chatWaiting(th.sim) ? "Delivered" : "Seen", 386, y - 12, 0.32f, C(UI1), 2);
            y -= 12;
        }
        for (int i = (int)th.msgs.size() - 1; i >= 0 && y > 26; i--) {
            const Msg& m = th.msgs[i];
            auto lines = wrapText(m.text, 0.45f, 200);
            float h = lines.size() * 14.f + 8;
            if (y - h - 4 < 24) break;   // don't draw over the header
            y -= h + 4;
            float w = 0;
            for (auto& l : lines) w = std::fmax(w, textW(l.c_str(), 0.45f));
            w += 14;
            float x = m.me ? 390 - w : 86;
            rrect(x, y, w, h, 8, m.me ? C(HOT) : C(255, 255, 255));
            for (size_t k = 0; k < lines.size(); k++) text(lines[k].c_str(), x + 7, y + 4 + k * 14.f, 0.45f, m.me ? C(WHITE) : C(UI1));
        }
        break;
    }
    case AP_DATING: lovebugDrawTop(t); break;
    case 9999: {
        grad(0, 0, TOP_W, TOP_H, C(255, 90, 140), C(200, 40, 100));
        for (int i = 0; i < 8; i++) heart(20 + i * 52, 20 + (i % 3) * 70 + std::sin(t + i) * 6, 6, C(255, 255, 255, 60));
        textShadow("Glam Date", 200, 4, 0.7f, C(WHITE), C(BERRY), 1);
        if (!me.isAdult()) { text("Glam Date is for Young Adults+", 200, 110, 0.55f, C(WHITE), 1); break; }
        if (swipeI >= (int)swipes.size()) { text("No more profiles nearby!", 200, 100, 0.6f, C(WHITE), 1); text("Check back after you meet more people.", 200, 124, 0.45f, C(WHITE), 1); break; }
        float off = swipeAnim > 0 ? swipeDir * (1 - swipeAnim) * -200 : 0;
        float cx = 200 + off;
        rrect(cx - 110, 30, 220, 204, 14, C(255, 255, 255));
        int s = swipes[swipeI];
        const Sim& o = G.sims[s];
        grad(cx - 104, 36, 208, 120, C(255, 220, 232), C(255, 196, 214));
        drawSim(o, cx, 154, 1.0f, t, 0, PO_IDLE, false);
        snprintf(b, sizeof b, "%s, %s", o.name.c_str(), STAGE_NAMES[o.stage]);
        text(b, cx, 160, 0.55f, C(UI1), 1);
        text(("\"" + o.bio + "\"").c_str(), cx, 180, 0.42f, C(BERRY), 1);
        std::string tr;
        for (uint8_t k : o.traits) if (k != TR_NONE) { if (!tr.empty()) tr += " / "; tr += TRAIT_NAMES[k]; }
        text(tr.c_str(), cx, 198, 0.36f, C(UIM), 1);
        int shared = 0; for (uint8_t k : me.traits) if (k != TR_NONE && o.hasTrait(k)) shared++;
        if (shared) { snprintf(b, sizeof b, "%d trait%s in common!", shared, shared > 1 ? "s" : ""); text(b, cx, 214, 0.38f, C(HOT), 1); }
        break;
    }
    case AP_SOCIAL: {
        phoneFrameTop("InstaGlam");
        float y = 28;
        for (int i = scroll; i < (int)G.posts.size() && y < 230; i++) {
            const Post& p = G.posts[i];
            bool mine = p.sim == G.mc;
            rrect(8, y, 384, 40, 8, C(255, 255, 255, 235));
            if (p.sim >= 0) drawSim(G.sims[p.sim], 30, y + 39, 0.3f, t, 0, PO_IDLE, false);
            text(p.sim >= 0 ? (mine ? "@me" : ("@" + G.sims[p.sim].name).c_str()) : "@someone", 52, y + 3, 0.42f, C(mine ? HOT : BERRY));
            text(p.text.c_str(), 52, y + 20, 0.42f, C(UI1));
            heart(330, y + 20, 5, C(HOT));
            snprintf(b, sizeof b, "%u", (unsigned)p.likes); text(b, 342, y + 13, 0.42f, C(UI1));
            y += 44;
        }
        if (G.posts.empty()) text("Your feed is empty. Post something!", 200, 110, 0.5f, C(UI1), 1);
        break;
    }
    case AP_MAP: {
        grad(0, 0, TOP_W, TOP_H, C(190, 230, 180), C(160, 210, 160));
        ellipse(360, 220, 160, 90, C(120, 190, 240));   // ocean
        for (int i = 0; i < 6; i++) line(0, 100 + i * 7, TOP_W, 110 + i * 3, 1, C(255, 255, 255, 30));
        line(70, 80, 220, 130, 6, C(230, 220, 210)); line(220, 130, 350, 130, 6, C(230, 220, 210)); line(220, 130, 110, 160, 6, C(230, 220, 210));
        line(220, 130, 240, 40, 6, C(230, 220, 210)); line(150, 60, 220, 130, 6, C(230, 220, 210)); line(300, 70, 220, 130, 6, C(230, 220, 210));
        line(110, 160, 60, 190, 6, C(230, 220, 210)); line(350, 130, 330, 190, 6, C(230, 220, 210));
        { int R[8]; int n = vacationMapRooms(R);
          if (n) {   // trip map
              const int v = vacationDest();
              grad(0, 0, TOP_W, TOP_H, v == 2 ? C(220, 232, 245) : v == 3 ? C(236, 226, 214) : C(140, 210, 240), v == 2 ? C(250, 252, 255) : v == 3 ? C(220, 208, 190) : C(240, 226, 180));
              static const float P[7][2] = {{200, 70}, {130, 60}, {270, 60}, {200, 130}, {80, 160}, {320, 160}, {200, 200}};
              for (int k = 4; k < 7; k++) line(200, 100, P[k][0], P[k][1], 5, C(255, 255, 255, 180));
              rrect(100, 36, 200, 110, 12, C(255, 255, 255, 120)); text("HOTEL", 200, 38, 0.4f, C(UI1), 1);
              for (int k = 0; k < n; k++) {
                  float x = P[k][0], y = P[k][1];
                  rrect(x - 14, y - 12, 28, 20, 4, C(k < 4 ? HOT : BERRY));
                  text(vacationShort(R[k]), x, y + 10, 0.36f, C(UI1), 1);
                  if (R[k] == G.room) heart(x, y - 26 + std::sin(t * 5) * 3, 6, C(GOLD));
              }
              textShadow("Trip Map", 8, 4, 0.6f, C(UI1), C(255, 255, 255, 180));
              break;
          } }
        if (mapPage == 1) { text("More places to go!", 200, 110, 0.6f, C(UI1), 1); break; }
        if (mapPage == 2) { textOutline("Rabbit Holes", 200, 80, 0.8f, C(255, 255, 255), C(UI1), 1); text("Clinic, hospital, school, work, police, city hall, bank & post office", 200, 120, 0.4f, C(UI1), 1); text(healthStatus(G.mc).c_str(), 200, 150, 0.45f, C(UI1), 1); break; }
        for (int i = 0; i < MAP_N - 1; i++) {
            float x = MAP_POS[i][0], y = MAP_POS[i][1];
            bool here = (MAP_ROOMS[i] == G.room) || (i == 0 && roomIsHome(G.room));
            rrect(x - 14, y - 12, 28, 20, 4, C(i == 0 ? HOT : BERRY));
            tri(x - 16, y - 12, x + 16, y - 12, x, y - 22, C(i == 0 ? HOT : BERRY));
            text(i == 0 ? HOUSE_NAMES[G.house] : MAP_ROOMS[i] == R_HOSPITAL ? "Uptown" : ROOM_NAMES[MAP_ROOMS[i]], x, y + 10, 0.38f, C(UI1), 1);
            if (here) { heart(x, y - 32 + std::sin(t * 5) * 3, 6, C(GOLD)); }
        }
        textShadow("Town Map", 8, 4, 0.6f, C(UI1), C(255, 255, 255, 180));
        break;
    }
    case AP_CAL: {
        lxCalDrawTop(t); break;
        phoneFrameTop("Calendar");
        if (G.day / 7 + calWeek < 0) calWeek = -(G.day / 7);
        int start = (G.day / 7 + calWeek) * 7;
        snprintf(b, sizeof b, "%s - Week %d", SEASON_NAMES[seasonOf(start)], start / 7 + 1);
        text(b, 200, 26, 0.5f, C(BERRY), 1);
        for (int d = 0; d < 7; d++) {
            int day = start + d;
            float x = 6 + d * 56.f;
            bool today = day == G.day;
            rrect(x, 46, 52, 186, 6, today ? C(HOT) : C(255, 255, 255, 220));
            text(WEEKDAY_NAMES[d], x + 26, 49, 0.42f, today ? C(WHITE) : C(UI1), 1);
            snprintf(b, sizeof b, "%d", day % 7 + 1); text(b, x + 26, 64, 0.5f, today ? C(WHITE) : C(BERRY), 1);
            float y = 86;
            u32 tc = today ? C(WHITE) : C(UI1);
            int dd = day % 28;
            if (dd == 18) { text("Spooky", x + 26, y, 0.34f, C(ORANGE), 1); text("Day!", x + 26, y + 11, 0.34f, C(ORANGE), 1); y += 26; }
            if (dd == 25) { text("Winter", x + 26, y, 0.34f, C(GREEN), 1); text("fest!", x + 26, y + 11, 0.34f, C(GREEN), 1); y += 26; }
            {
                const TownEvent* ev[6];
                int ne = eventsOn(day, ev, 6);
                for (int k = 0; k < ne && y < 210; k++) {
                    std::string nm = ev[k]->name;
                    size_t sp = nm.find(' ');
                    sparkle(x + 6, y + 6, 3, C(GOLD));
                    text(nm.substr(0, std::min<size_t>(sp == std::string::npos ? nm.size() : sp, 8)).c_str(), x + 28, y, 0.3f, today ? C(WHITE) : C(BERRY), 1);
                    y += 12;
                }
            }
            if (d >= 5) { text("Weekend", x + 26, y, 0.32f, tc, 1); y += 12; }
            if (d == 6) { text("Bills", x + 26, y, 0.32f, tc, 1); y += 12; }
            for (size_t i = 0; i < G.sims.size() && y < 220; i++) {
                const Sim& s = G.sims[i];
                if (!s.household || !G.aging || STAGE_DAYS[s.stage] == 255) continue;
                if (G.day + (STAGE_DAYS[s.stage] - s.stageDays) == day) { heart(x + 8, y + 6, 3, C(GOLD)); text(s.name.substr(0, 6).c_str(), x + 30, y, 0.32f, tc, 1); y += 12; }
            }
            for (const Event& e : G.events) {
                if (e.day != day || y > 220) continue;
                if (e.type == EV_DATE) heart(x + 6, y + 6, 3, C(today ? WHITE : HOT));
                snprintf(b, sizeof b, "%d%s", e.hour > 12 ? e.hour - 12 : e.hour, e.hour >= 12 ? "p" : "a");
                text(b, x + 12, y, 0.32f, tc);
                text(e.text.substr(0, 7).c_str(), x + 26, y + 10, 0.3f, tc, 1);
                y += 24;
            }
        }
        break;
    }
    case AP_PLANNER: {
        phoneFrameTop("Planner");
        text("Plan your week! You'll get a reminder.", 200, 28, 0.45f, C(UI1), 1);
        float y = 50;
        int n = 0;
        for (const Event& e : G.events) {
            if (e.day < G.day || n >= 10) continue;
            snprintf(b, sizeof b, "%s %s  %s  %s", WEEKDAY_NAMES[weekday(e.day)], clockStr(e.hour * 60.f).c_str(), e.text.c_str(), e.done ? "(done)" : "");
            rrect(20, y, 360, 16, 5, C(255, 255, 255, 200));
            if (e.type == EV_DATE) heart(30, y + 8, 4, C(HOT)); else sparkle(30, y + 8, 4, C(GOLD));
            text(b, 40, y + 1, 0.42f, C(UI1));
            y += 18; n++;
        }
        if (!n) text("Nothing planned yet!", 200, 110, 0.55f, C(UIM), 1);
        break;
    }
    case AP_FAMILY: {
        if (famPlan) {
            phoneFrameTop("Family Planner");
            text(familyStatusLine().c_str(), 200, 28, 0.45f, C(BERRY), 1);
            text("Chore Chart (done at 5 PM)", 20, 50, 0.42f, C(UI1));
            auto kids = choreKids(); float y = 68;
            for (int s : kids) { const NpcLife& k = lifeOf(s); snprintf(b, sizeof b, "%s: %s  %s", G.sims[s].name.c_str(), CHORE_NAMES[k.chore], k.chore ? (std::string(k.choresWeek, '*')).c_str() : ""); text(b, 30, y, 0.4f, C(UIM)); if (k.grounded) text("GROUNDED", 380, y, 0.36f, C(HOT), 2); y += 16; }
            text("Family Traditions", 20, 140, 0.42f, C(UI1)); y = 158;
            int on = 0;
            for (int e = 0; e < FEV_COUNT; e++) if (G.famEvents >> e & 1) { snprintf(b, sizeof b, "%s  -  every %s", FAMEV_NAMES[e], FAMEV_WHEN[e]); text(b, 30, y, 0.4f, C(UIM)); y += 15; on++; }
            if (!on) text("Tap traditions below to add them to the week!", 30, y, 0.38f, C(UIM));
            break;
        }
        phoneFrameTop("Family");
        std::vector<int> fam;
        for (size_t i = 0; i < G.sims.size(); i++) if (G.sims[i].household) fam.push_back((int)i);
        float step = std::fmin(70.f, 380.f / std::max<size_t>(1, fam.size()));
        for (size_t k = 0; k < fam.size(); k++) {
            const Sim& s = G.sims[fam[k]];
            float x = 40 + k * step;
            drawSim(s, x, 200, 0.9f, t + k, G.holiday, fam[k] == G.mc ? PO_WAVE : PO_IDLE, false);
            text(s.name.c_str(), x, 204, 0.38f, C(fam[k] == G.mc ? HOT : UI1), 1);
            text(STAGE_NAMES[s.stage], x, 218, 0.32f, C(UIM), 1);
            { std::string ks = kidsSummary(fam[k]); if (!ks.empty() && fam[k] != G.mc) text(ks.c_str(), x, 228, 0.28f, C(BERRY), 1); }
        }
        int p = partnerOf(G.mc);
        if (p >= 0) { snprintf(b, sizeof b, "%s with %s", REL_STATUS_NAMES[findRel(G.mc, p)->status], G.sims[p].name.c_str()); text(b, 392, 26, 0.42f, C(BERRY), 2); }
        break;
    }
    case AP_PEOPLE: {
        phoneFrameTop("People");
        auto ppl = peopleList();
        scroll = std::min(scroll, std::max(0, (int)ppl.size() - 1));
        float y = 26;
        for (int i = scroll; i < (int)ppl.size() && y < 226; i++) {
            int o = ppl[i];
            const Rel& r = *findRel(G.mc, o);
            rrect(6, y, 388, 24, 6, i == scroll ? C(255, 236, 246) : C(255, 255, 255, 210));
            text(G.sims[o].name.c_str(), 12, y + 4, 0.45f, C(UI1));
            text(relLabel(G.mc, o), 110, y + 4, 0.4f, C(BERRY));
            text("Friend", 196, y + 1, 0.3f, C(UIM));
            rrect(196, y + 13, 90, 7, 3, C(LIGHTGRAY)); rrect(241, y + 13, 45 * clampv((int)r.fr, -100, 100) / 100.f, 7, 3, C(r.fr >= 0 ? GREEN : RED));
            if (canRomance(G.mc, o)) {
                text("Romance", 296, y + 1, 0.3f, C(UIM));
                rrect(296, y + 13, 90, 7, 3, C(LIGHTGRAY)); rrect(341, y + 13, 45 * clampv((int)r.ro, -100, 100) / 100.f, 7, 3, C(r.ro >= 0 ? HOT : RED));
            }
            y += 26;
        }
        if (ppl.empty()) text("Go out and meet people!", 200, 110, 0.55f, C(UIM), 1);
        break;
    }
    case AP_ME: {
        phoneFrameTop(me.name.c_str());
        drawSim(me, 50, 226, 1.1f, t, G.holiday, PO_IDLE, false);
        Emotion e = emotionOf(me);
        circle(110, 40, 8, C(EMOTION_COLORS[e]));
        snprintf(b, sizeof b, "%s  (%s)", EMOTION_NAMES[e], STAGE_NAMES[me.stage]);
        text(b, 122, 32, 0.5f, C(UI1));
        for (int n = 0; n < N_COUNT; n++) { text(NEED_NAMES[n], 104, 54 + n * 16.f, 0.4f, C(UI1)); bar(160, 58 + n * 16.f, 80, me.needs[n], needCol(me.needs[n])); }
        for (int s = 0; s < SK_COUNT; s++) {
            snprintf(b, sizeof b, "%s %d", SKILL_NAMES[s], skillLevel(me, s));
            text(b, 254, 54 + s * 16.f, 0.4f, C(UI1));
            bar(330, 58 + s * 16.f, 62, (me.skills[s] - skillLevel(me, s) * 10) * 10, LILAC);
        }
        float y = 150;
        std::string tr;
        for (uint8_t k : me.traits) if (k != TR_NONE) { if (!tr.empty()) tr += ", "; tr += TRAIT_NAMES[k]; }
        text(("Traits: " + tr).c_str(), 104, y, 0.4f, C(BERRY)); y += 16;
        if (me.career) { snprintf(b, sizeof b, "Job: %s (perf %d%%)", CAREERS[me.career].titles[me.careerLevel], (int)me.performance); text(b, 104, y, 0.4f, C(UI1)); y += 16; }
        if (me.stage >= ST_CHILD && me.stage <= ST_TEEN) { text(("School grade: " + letterGrade(me.gradePts)).c_str(), 104, y, 0.4f, C(UI1)); y += 16; }
        if (me.enrolled || me.degree) { snprintf(b, sizeof b, me.degree ? "College: Graduated!" : "College credits: %d/12", (int)me.credits); text(b, 104, y, 0.4f, C(UI1)); y += 16; }
        if (G.aging && STAGE_DAYS[me.stage] != 255) { snprintf(b, sizeof b, "Next birthday in %d day(s)", STAGE_DAYS[me.stage] - me.stageDays); text(b, 104, y, 0.4f, C(UI1)); y += 16; }
        for (size_t i = 0; i < me.moodlets.size() && i < 4; i++) {
            const Moodlet& m = me.moodlets[i];
            rrect(250, 160 + i * 18.f, 144, 16, 5, C(EMOTION_COLORS[m.emotion], 200));
            text(m.name.c_str(), 256, 161 + i * 18.f, 0.4f, C(UI1));
        }
        break;
    }
    case AP_GOALS: goalsAppDrawTop(t); break;
    case AP_SCRAPBOOK: scrapbookDrawTop(t); break;
    case AP_TEA: teaAppDrawTop(t); break;
    case AP_CAST: ocAppDrawTop(t); break;
    case AP_STATUS: statusAppDrawTop(t); break;
    case AP_SHOP: shopAppDrawTop(t); break;
    case AP_LOT: lotAppDrawTop(t); break;
    case AP_WALLET: walletAppDrawTop(t); break;
    case AP_YUM: yumAppDrawTop(t); break;
    case AP_HOTEL: hotelAppDrawTop(t); break;
    case AP_JOBS: {
        phoneFrameTop("Jobs & School");
        float y = 30;
        for (int i = 1 + jobPage * 4; i < CR_COUNT && i < 5 + jobPage * 4; i++) {
            const CareerDef& c = CAREERS[i];
            rrect(8, y, 384, 40, 8, C(255, 255, 255, 220));
            text(c.name, 16, y + 3, 0.5f, C(BERRY));
            snprintf(b, sizeof b, "%s, %s-%s weekdays. $%d-%d/hr. Skill: %s%s", ROOM_NAMES[c.room], clockStr(c.startHour * 60.f).c_str(),
                     clockStr((c.startHour + c.hours) * 60.f).c_str(), c.pay[0], c.pay[4], SKILL_NAMES[c.skill], c.teenOk ? " (teens ok)" : "");
            text(b, 16, y + 22, 0.36f, C(UI1));
            y += 42;
        }
        text("College: $300 tuition, 12 credits. A degree starts most jobs a level higher!", 200, 204, 0.36f, C(UI1), 1);
        text("Most jobs have a playable shift - or work quietly to skip it.", 200, 220, 0.36f, C(UI1), 1);
        break;
    }
    }
    if (bannerT > 0) { rrect(60, 100, 280, 34, 10, C(UI1, 235)); text(banner.c_str(), 200, 109, 0.5f, C(BLUSH), 1); }
}

// ------------------------------------------------------------- bottom screen
void phoneDrawBottom(float t) {
    grad(0, 0, BOT_W, BOT_H, C(HOT), C(BERRY));
    for (int i = 0; i < 4; i++) sparkle(20 + i * 90, 14, 2 + std::fabs(std::sin(t * 2 + i)) * 2, C(255, 255, 255, 160));
    char b[96];
    Sim& me = MC();
    static const char* titles[] = {"Apps", "Texts", "Chat", "Glam Date", "InstaGlam", "Where to?", "Calendar", "Planner", "Family", "People", "Me", "Jobs", "Date spot?", "Goals", "Scrapbook", "The Tea", "", "", "ShopNow", "My Lot", "Wallet", "Yum Run", "Hotel"};
    text(titles[app % (int)(sizeof titles / sizeof *titles)], 160, 4, 0.55f, C(WHITE), 1);
    switch (app) {
    case AP_HOME:
        { Btn m = appB(24); rrect(m.x, m.y, m.w, m.h, 10, C(90, 90, 110)); char pg[16];
          if (openFolder >= 0) snprintf(pg, sizeof pg, "< Back"); else snprintf(pg, sizeof pg, appPage ? "< Apps" : "Folders >");
          text(pg, m.x + m.w / 2, m.y + 10, 0.38f, C(WHITE), 1); }
        if (appPage == 1 && openFolder < 0) {
            for (int f = 0; f < 7; f++) { Btn a = appB(f); rrect(a.x, a.y, a.w, a.h, 10, C(FOLDER_COLS[f])); rrect(a.x + 3, a.y + 2, a.w - 6, a.h * 0.35f, 6, C(255, 255, 255, 60));
                for (int k = 0; k < std::min(4, folderLen(f)); k++) rect(a.x + 12 + k * 9, a.y + 7, 7, 7, C(APP_COLS[FOLDERS[f][k]]));
                text(FOLDER_NAMES[f], a.x + a.w / 2, a.y + 19, 0.38f, C(WHITE), 1); }
            text("Tap a folder. L/R flips pages.", 160, 190, 0.34f, C(WHITE), 1);
            break;
        }
        if (openFolder >= 0) text(FOLDER_NAMES[openFolder], 160, 190, 0.45f, C(WHITE), 1);
        for (int slot = 0; slot < (openFolder >= 0 ? folderLen(openFolder) : APPS_P1); slot++) {
            const int i = openFolder >= 0 ? FOLDERS[openFolder][slot] : slot;
            Btn a = appB(slot);
            rrect(a.x, a.y, a.w, a.h, 10, C(APP_COLS[i]));
            rrect(a.x + 3, a.y + 2, a.w - 6, a.h * 0.35f, 6, C(255, 255, 255, 60));
            drawAppIcon(i, a.x + a.w / 2, a.y + 12, 6.5f);
            text(APP_NAMES[i], a.x + a.w / 2, a.y + 21, 0.34f, C(WHITE), 1);
            if (i == 0 && phoneUnread()) { circle(a.x + a.w - 6, a.y + 6, 7, C(RED)); snprintf(b, sizeof b, "%d", phoneUnread()); text(b, a.x + a.w - 6, a.y, 0.4f, C(WHITE), 1); }
        }
        break;
    case AP_TEXTS:
        for (int i = 0; i < 5; i++) {
            int ti = scroll + i;
            if (ti >= (int)G.threads.size()) break;
            const ChatThread& th = G.threads[ti];
            Btn r = listB(i);
            button(r, "", th.unread, 1, 0.45f);
            text(G.sims[th.sim].name.c_str(), r.x + 10, r.y + 3, 0.48f, C(CREAM));
            std::string last = th.msgs.empty() ? "" : th.msgs.back().text;
            if (last.size() > 30) last = last.substr(0, 30) + "...";
            text(last.c_str(), r.x + 100, r.y + 7, 0.38f, C(BLUSH));
            if (th.unread) circle(r.x + r.w - 10, r.y + 15, 4, C(GOLD));
        }
        if (G.threads.empty()) text("(no conversations)", 160, 100, 0.5f, C(WHITE), 1);
        break;
    case AP_THREAD: {
        ChatThread* thp = curThread();
        if (!thp) break;
        ChatThread& th = *thp;
        if (chatLive(th.prompt)) {
            auto v = chatChoices(th);
            for (int i = 0; i < (int)v.size() && i < 5; i++) button(listB(i), v[i].label.c_str(), false, v[i].style == 1 ? 1 : v[i].style == 0 ? 2 : 0, 0.4f);
        } else if (th.prompt >= 0 && th.prompt < NUM_PROMPTS) {
            text("Pick your reply:", 160, 20, 0.42f, C(BLUSH), 1);
            for (int i = 0; i < 3; i++) button(listB(i), PROMPTS[th.prompt].c[i].text, false, i == 2 ? 1 : 0, 0.45f);
        } else {
            if (chatWaiting(th.sim)) {
                text(chatTyping(th.sim) ? "typing..." : "Delivered. Waiting for a reply...", 160, 70, 0.45f, C(WHITE), 1);
                text("(people text back at their own speed)", 160, 90, 0.34f, C(BLUSH), 1);
            } else {
                auto v = chatStarters(th.sim);
                for (int i = 0; i < (int)v.size() && i < 8; i++) button(half(i, 30, 26), v[i].label.c_str(), false, v[i].style == 3 ? 1 : v[i].style == 0 ? 2 : 0, 0.38f);
            }
            static const char* q[4] = {"Type...", "Selfie", "Invite Over", "Date"};
            for (int i = 0; i < 4; i++) button(Btn{6 + i * 77.f, 150, 74, 26}, q[i], false, i == 3 ? 2 : 0, 0.38f);
            text(relLabel(G.mc, th.sim), 160, 186, 0.42f, C(WHITE), 1);
        }
        break;
    }
    case AP_DATELOC: {
        static const char* q[4] = {"Dinner @ Bella Notte", "Dancing @ Club Glitz", "Sunset Beach", "Coffee @ Cafe Latte"};
        for (int i = 0; i < 4; i++) button(half(i), q[i], false, 0, 0.4f);
        text("Dates start when you both arrive there!", 160, 110, 0.42f, C(WHITE), 1);
        break;
    }
    case AP_DATING: lovebugDrawBottom(t); break;
    case 9999:
        if (!me.isAdult()) { text("Grow up a little first!", 160, 100, 0.5f, C(WHITE), 1); break; }
        if (swipeI < (int)swipes.size()) {
            button(half(0, 168, 36), "Nope  (L)", false, 1, 0.5f);
            button(half(1, 168, 36), "Like <3  (R)", false, 0, 0.5f);
            heart(160, 100, 26 + std::sin(t * 4) * 3, C(255, 255, 255, 120));
            snprintf(b, sizeof b, "%d profiles left", (int)swipes.size() - swipeI); text(b, 160, 140, 0.42f, C(WHITE), 1);
        } else text("You've seen everyone!", 160, 100, 0.5f, C(WHITE), 1);
        break;
    case AP_SOCIAL: {
        snprintf(b, sizeof b, "%u followers", (unsigned)G.followers); text(b, 160, 24, 0.55f, C(WHITE), 1);
        for (int i = 0; i < 5; i++) sparkle(110 + i * 25, 54, 8, C(i < fameStars() ? GOLD : RGB{255, 255, 255}, i < fameStars() ? 255 : 90));
        text(fameTitle(), 160, 66, 0.45f, C(CREAM), 1);
        static const char* posts[6] = {"Post Selfie", "Post OOTD", "Post My Food", "Post Room Tour", "Post Date Pic", "Post Family"};
        for (int i = 0; i < 6; i++) button(half(i, 96, 30), posts[i], false, i % 2, 0.42f);
        if (G.postCooldown > 0) text("(rest a bit before posting again)", 160, 194, 0.38f, C(CREAM), 1);
        break;
    }
    case AP_MAP:
        if (rhOpen >= 0) {
            textOutline(rabbitHoleName(rhOpen), 160, 28, 0.55f, C(255, 255, 255), C(UI_INK), 1);
            auto opts = rabbitHoleOptions(rhOpen);
            for (int k = 0; k < (int)opts.size(); k++) button(Btn{20, 60 + k * 34.f, 280, 30}, opts[k].first.c_str(), false, opts[k].second < 0 ? 1 : 0, 0.4f);
            button(Btn{214, 196, 101, 30}, "< Map", false, 3, 0.42f);
            break;
        }
        if (mapPage == 2 && !vacationDest()) text("Rabbit holes: quick trips, no walking around", 112, 196, 0.3f, C(CREAM), 1);
        if (mapPage == 3 && !vacationDest()) text("Subway: $3 a ride", 112, 196, 0.36f, C(CREAM), 1);
        if (mapPage == 4 && !vacationDest()) text("Around town: school, community & campus", 112, 196, 0.3f, C(CREAM), 1);
        for (int i = 0; i < MAP_N; i++) {
            int rm = mapRoom(i);
            if (rm == -1) continue;
            if (rm == -2 || rm == -3) { button(mapB(i), rm == -2 ? (mapPage == 2 ? "Subway >" : "Next >") : "< Back", false, 3, 0.42f); continue; }
            if (rm <= -10) { button(mapB(i), rabbitHoleName(-10 - rm), false, 2, 0.36f); continue; }
            std::string why;
            bool open = placeOpen(rm, &why);
            bool ev = eventHere(rm) != nullptr;
            if (vacationDest()) { button(mapB(i), vacationShort(rm), rm == G.room, i < 4 ? 2 : 0, 0.38f); continue; }
            button(mapB(i), i == 0 && !mapPage ? "Home" : rm == R_HOSPITAL ? "Uptown" : ROOM_NAMES[rm], ev, open ? (i == 0 && !mapPage ? 2 : 0) : 1, 0.36f);
        }
        break;
    case AP_CAL:
        lxCalDrawBottom(t); break;
        button(Btn{6, 170, 70, 28}, "< Prev", false, 1, 0.45f);
        button(Btn{244, 170, 70, 28}, "Next >", false, 1, 0.45f);
        snprintf(b, sizeof b, "Today: %s", dateStr(G.day).c_str()); text(b, 160, 60, 0.5f, C(WHITE), 1);
        text("1 year = 4 seasons x 7 days", 160, 90, 0.42f, C(CREAM), 1);
        text("Hearts = dates & birthdays", 160, 110, 0.42f, C(CREAM), 1);
        text("Stars = town events & contests", 160, 128, 0.42f, C(CREAM), 1);
        break;
    case AP_PLANNER:
        for (int i = 0; i < 6; i++) button(half(i, 28, 26), PLAN_NAMES[i], planAct == i, 0, 0.4f);
        button(Btn{6, 124, 40, 26}, "<", false, 1, 0.5f); button(Btn{120, 124, 40, 26}, ">", false, 1, 0.5f);
        text(dateStr(planDay).substr(0, 12).c_str(), 83, 129, 0.38f, C(WHITE), 1);
        button(Btn{166, 124, 40, 26}, "<", false, 1, 0.5f); button(Btn{274, 124, 40, 26}, ">", false, 1, 0.5f);
        text(clockStr(planHour * 60.f).c_str(), 240, 129, 0.42f, C(WHITE), 1);
        button(Btn{100, 160, 120, 30}, "Add Plan", false, 2, 0.5f);
        break;
    case AP_FAMILY: {
        std::vector<int> fam;
        for (size_t i = 0; i < G.sims.size(); i++) if (G.sims[i].household) fam.push_back((int)i);
        if (famPlan) {
            static const char* const AL[4] = {"Allowance: off", "Allowance: $5", "Allowance: $10", "Allowance: $20"};
            text("Curfew", 60, 32, 0.45f, C(CREAM), 1);
            button(Btn{124, 28, 30, 24}, "<", false, 1, 0.45f); button(Btn{246, 28, 30, 24}, ">", false, 1, 0.45f);
            { int h = G.curfew; snprintf(b, sizeof b, "%d:00 %s", h > 12 ? h - 12 : h, h >= 12 ? "PM" : "AM"); text(b, 200, 32, 0.48f, C(GOLD), 1); }
            auto kids = choreKids();
            if (kids.empty()) text("No kids or housemates for chores yet.", 160, 90, 0.4f, C(BLUSH), 1);
            for (size_t i = 0; i < kids.size(); i++) {
                text(G.sims[kids[i]].name.c_str(), 12, 61 + i * 24.f, 0.42f, C(CREAM));
                button(Btn{150, 58 + i * 24.f, 164, 22}, CHORE_NAMES[lifeOf(kids[i]).chore], lifeOf(kids[i]).chore != 0, 3, 0.38f);
            }
            for (int e = 0; e < FEV_COUNT; e++) button(Btn{4 + (e % 3) * 105.f, 158 + (e / 3) * 25.f, 102, 23}, FAMEV_NAMES[e], (G.famEvents >> e) & 1, 2, 0.32f);
            button(Btn{236, 4, 80, 22}, "Family", false, 1, 0.38f);
            button(Btn{204, 210, 112, 26}, AL[G.allowance % 4], G.allowance != 0, 3, 0.36f);
            break;
        }
        scroll = std::min(scroll, std::max(0, (int)fam.size() - 5));
        for (int i = 0; i < 5; i++) {
            int k = scroll + i;
            if (k >= (int)fam.size()) break;
            const Sim& s = G.sims[fam[k]];
            Btn r = listB(i);
            button(r, "", fam[k] == G.mc, 1, 0.45f);
            snprintf(b, sizeof b, "%s - %s", s.name.c_str(), STAGE_NAMES[s.stage]);
            text(b, r.x + 10, r.y + 6, 0.45f, C(CREAM));
            text(fam[k] == G.mc ? "playing" : "Play as", r.x + r.w - 10, r.y + 7, 0.4f, C(GOLD), 2);
        }
        button(Btn{204, 208, 112, 28}, "Adopt ($500)", false, 2, 0.42f);
        button(Btn{236, 4, 80, 22}, "Planner", false, 3, 0.38f);
        break;
    }
    case AP_PEOPLE: {
        auto v = peopleList();
        if (v.empty()) { text("Up/Down to scroll", 160, 100, 0.5f, C(WHITE), 1); break; }
        int o = v[std::min(scroll, (int)v.size() - 1)];
        const Sim& os = G.sims[o];
        text(os.name.c_str(), 160, 26, 0.5f, C(WHITE), 1);
        text(lifeStatusLine(o).c_str(), 160, 44, 0.38f, C(BLUSH), 1);
        snprintf(b, sizeof b, "Hobby: %s", lifeHobbyName(o)); text(b, 14, 64, 0.4f, C(WHITE));
        snprintf(b, sizeof b, "Hangout: %s", lifeFavPlaceName(o)); text(b, 14, 80, 0.4f, C(WHITE));
        snprintf(b, sizeof b, "Dream: %s", lifeGoalName(o)); text(b, 14, 96, 0.4f, C(WHITE));
        rrect(14, 113, 292, 7, 3, C(255, 255, 255, 80)); rrect(14, 113, 292 * lifeGoalProgress(o) / 100.f, 7, 3, C(GOLD));
        { int p = partnerOf(o); const Rel* pr = p >= 0 ? findRel(o, p) : nullptr;
          snprintf(b, sizeof b, "%s", p < 0 ? "Single" : (std::string(REL_STATUS_NAMES[pr ? pr->status % 5 : 0]) + " " + G.sims[p].name).c_str());
          text(b, 14, 126, 0.4f, C(WHITE)); }
        std::string tr;
        for (uint8_t k : os.traits) if (k != TR_NONE) { if (!tr.empty()) tr += ", "; tr += TRAIT_NAMES[k]; }
        text(tr.c_str(), 14, 144, 0.36f, C(BLUSH));
        button(Btn{110, 172, 100, 28}, "Text", false, 2, 0.45f);
        text("Up/Down: next person", 250, 214, 0.32f, C(BLUSH), 1);
        break;
    }
    case AP_ME: {
        snprintf(b, sizeof b, "Money: $%d", (int)G.money); text(b, 160, 28, 0.5f, C(WHITE), 1);
        // reputation meter
        snprintf(b, sizeof b, "Reputation: %s", repLabel(G.rep)); text(b, 160, 48, 0.45f, C(CREAM), 1);
        rrect(30, 68, 260, 10, 5, C(UI2));
        rect(160, 68, 1, 10, C(WHITE));
        float rx = 160 + G.rep * 1.28f;
        circle(rx, 73, 6, C(G.rep < 0 ? RGB{200, 40, 70} : RGB{120, 230, 160}));
        text("mean", 30, 80, 0.32f, C(CREAM, 180)); text("sweet", 290, 80, 0.32f, C(CREAM, 180), 2);
        // squads
        snprintf(b, sizeof b, "Squad: %s", G.clique >= 0 ? CLIQUE_NAMES[G.clique] : "none yet"); text(b, 160, 96, 0.45f, C(GOLD), 1);
        for (int c = 0; c < CQ_COUNT; c++) {
            float y = 116 + c * 16.f;
            text(CLIQUE_NAMES[c], 12, y, 0.38f, C(G.clique == c ? GOLD : CREAM));
            rrect(120, y + 3, 120, 8, 4, C(UI2)); rrect(120, y + 3, 1.2f * G.cliqueStand[c], 8, 4, C(G.clique == c ? GOLD : BLUSH));
            if (G.cliqueStand[c] >= 60 && G.clique != c) text("invite soon!", 246, y, 0.32f, C(WHITE));
        }
        if (me.stage >= ST_CHILD && me.stage <= ST_TEEN) { snprintf(b, sizeof b, "Club: %s", G.club >= 0 ? CLUB_NAMES[G.club] : "join one at school!"); text(b, 160, 198, 0.4f, C(CREAM), 1); }
        else text("Be nice to a squad's members to get invited in.", 160, 198, 0.36f, C(CREAM), 1);
        break;
    }
    case AP_GOALS: goalsAppDrawBottom(t); break;
    case AP_SCRAPBOOK: scrapbookDrawBottom(t); break;
    case AP_TEA: teaAppDrawBottom(t); break;
    case AP_CAST: ocAppDrawBottom(t); break;
    case AP_STATUS: statusAppDrawBottom(t); break;
    case AP_SHOP: shopAppDrawBottom(t); break;
    case AP_LOT: lotAppDrawBottom(t); break;
    case AP_WALLET: walletAppDrawBottom(t); break;
    case AP_YUM: yumAppDrawBottom(t); break;
    case AP_HOTEL: hotelAppDrawBottom(t); break;
    case AP_JOBS:
        for (int i = 1 + jobPage * 4; i < CR_COUNT && i < 5 + jobPage * 4; i++) {
            bool mine = me.career == i;
            snprintf(b, sizeof b, mine ? "Quit %s" : "Join: %s", CAREERS[i].name);
            button(listB(i - 1 - jobPage * 4), b, mine, mine ? 1 : 0, 0.45f);
        }
        { char pg[24]; snprintf(pg, sizeof pg, "More (%d/%d)", jobPage + 1, (CR_COUNT - 1 + 3) / 4); button(Btn{240, 208, 76, 28}, pg, false, 2, 0.4f); }
        button(listB(4), me.degree ? "College: Graduated!" : me.enrolled ? "College: Enrolled" : "Enroll in College ($300)", me.enrolled, 2, 0.45f);
        break;
    }
    button(bBack, app == AP_HOME ? "Close" : "Back", false, 1, 0.45f);
}
