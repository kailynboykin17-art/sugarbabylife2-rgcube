#include "tween.hpp"
#include "sevents.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "audio.hpp"
#include "extras.hpp"
#include "lifestory.hpp"
#include "lifeext.hpp"
#include "spicy.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include "vfs.hpp"
#define fopen vfopen   // sidecar saves go into the one-file save pack

void lifeMarry(int a, int b);

enum { W_FRIENDS, W_FAMILY, W_PARTNER, W_TEENS, W_KIDS, W_SOLO };
struct SeType { const char* name; int16_t room; uint8_t hour, who, len; const char* goals[5]; const char* lines[2]; };
static const SeType T[] = {
    {"Wedding", R_CHAPEL, 16, W_FAMILY, 4, {"Walk Down the Aisle", "Exchange Vows", "Cut the Cake", "First Dance", "Throw the Bouquet"}, {"I'm not crying, YOU'RE crying!", "Best wedding EVER."}},
    {"A Night Out", R_CLUB, 21, W_FRIENDS, 4, {"Hit the Dance Floor", "Order a Round of Mocktails", "Take Group Pics", "Dance Until Late", nullptr}, {"Girls' night!!", "We needed this."}},
    {"Meet the Baby", R_LIVING, 14, W_FAMILY, 3, {"Introduce the Baby", "Pass the Baby Around", "Open Baby Gifts", "Take a Family Photo", nullptr}, {"She has your nose!", "So tiny!!"}},
    {"Bachelor(ette) Party", R_CLUB, 21, W_FRIENDS, 4, {"Toast the Last Night of Freedom", "Party Games", "Tip the Dancers", "Dance All Night", nullptr}, {"Last night of freedom!", "WOOO!"}},
    {"Hot Springs Day", R_HOTELPOOL, 11, W_FRIENDS, 3, {"Soak in the Springs", "Get a Massage", "Steam Room", "Cucumber Water Toast", nullptr}, {"So relaxing...", "My skin is glowing."}},
    {"Beach Party", R_BEACH, 13, W_FRIENDS, 4, {"Build a Sandcastle", "Beach Volleyball", "Splash in the Waves", "Bonfire at Sunset", nullptr}, {"Surf's up!", "Pass the sunscreen!"}},
    {"Camping Trip", R_PARK, 15, W_FAMILY, 5, {"Pitch the Tent", "Roast Marshmallows", "Tell Ghost Stories", "Stargaze", nullptr}, {"Did you hear that?!", "Best s'mores ever."}},
    {"Celebration", R_LIVING, 18, W_FRIENDS, 3, {"Make a Toast", "Cut the Cake", "Give a Speech", "Dance", nullptr}, {"So proud of you!", "Cheers!!"}},
    {"University Party", R_COLLEGE, 21, W_FRIENDS, 4, {"Pics with the Mascot", "Dance-Off", "Play Party Games", "Chant the Fight Song", nullptr}, {"GO MAPLES!", "This party is LIT."}},
    {"Romantic Date", R_RESTAURANT, 19, W_PARTNER, 3, {"Share a Meal", "Hold Hands", "Deep Talk", "Kiss Goodnight", nullptr}, {"I love this.", "You look amazing tonight."}},
    {"Day of Grieving", R_PARK, 12, W_FAMILY, 3, {"Share Memories", "Light a Candle", "Hug It Out", "Look at Old Photos", nullptr}, {"They'd be proud of you.", "I miss them too."}},
    {"Easter Egg Hunt", R_YARD, 10, W_FAMILY, 3, {"Hide the Eggs", "Hunt for Eggs", "Find the Golden Egg", "Candy Feast", nullptr}, {"I found one!", "No fair, you peeked!"}},
    {"Family Gathering", R_LIVING, 15, W_FAMILY, 4, {"Catch Up", "Big Family Meal", "Family Photo", "Play Board Games", nullptr}, {"You've grown so much!", "Pass the potatoes."}},
    {"Field Trip", R_MUSEUM, 9, W_KIDS, 4, {"Tour the Exhibit", "Ask the Guide a Question", "Gift Shop", "Group Photo", nullptr}, {"Whoa, cool!", "Can we come back?"}},
    {"Game Night", R_LIVING, 19, W_FRIENDS, 3, {"Board Game Battle", "Video Game Tournament", "Snack Break", "Trash Talk", nullptr}, {"I demand a rematch!", "GG!"}},
    {"Graduation Party", R_LIVING, 17, W_FAMILY, 4, {"Wear the Cap & Gown", "Graduation Photos", "Give a Speech", "Open Cards", nullptr}, {"Congrats, grad!", "The future is yours!"}},
    {"Gym Day", R_GYM, 8, W_FRIENDS, 2, {"Warm Up", "Lift Weights", "Cardio", "Post-Workout Smoothie", nullptr}, {"One more rep!", "Feel the burn!"}},
    {"Hangout", R_CAFE, 14, W_FRIENDS, 3, {"Chat", "Get Drinks", "Tell Jokes", "Plan the Next Hangout", nullptr}, {"Same time next week?", "Lol stop."}},
    {"Honeymoon", R_HOTEL, 15, W_PARTNER, 6, {"Check into the Suite", "Couples Massage", "Romantic Dinner", "Sunset Stroll", nullptr}, {"Just us, finally.", "Best trip ever."}},
    {"Housewarming Party", R_LIVING, 18, W_FRIENDS, 3, {"Give a House Tour", "Serve Snacks", "Open Housewarming Gifts", "Toast the New Home", nullptr}, {"Love what you did here!", "Brought you a plant!"}},
    {"Karaoke Night", R_KARAOKE, 20, W_FRIENDS, 3, {"Sing a Solo", "Sing a Duet", "Cheer for Friends", "Encore!", nullptr}, {"Hit that high note!", "ENCORE!"}},
    {"Pizza Party", R_LIVING, 18, W_FRIENDS, 3, {"Order Pizza", "Pizza Toppings Debate", "Movie on the Couch", "Last Slice Showdown", nullptr}, {"Pineapple belongs!", "No it doesn't!"}},
    {"Meet My Partner", R_LIVING, 17, W_FAMILY, 3, {"Introduce Your Partner", "Answer Awkward Questions", "Family Dinner", "Get Their Blessing", nullptr}, {"So... what do you do?", "They seem lovely!"}},
    {"Movie Night", R_LIVING, 20, W_FRIENDS, 3, {"Pick a Movie", "Make Popcorn", "Cuddle Up", "Discuss the Ending", nullptr}, {"No spoilers!", "That twist!!"}},
    {"Moving Out", R_LIVING, 16, W_FAMILY, 3, {"Break the News", "Comfort Everyone", "Pack Together", "Promise to Visit", nullptr}, {"I'll miss you so much.", "Call every day!"}},
    {"Playground Day", R_PLAYGROUND, 10, W_KIDS, 3, {"Slide", "Swing", "Tag", "Make a New Friend", nullptr}, {"Push me higher!", "You're it!"}},
    {"Pool Party", R_HOTELPOOL, 13, W_FRIENDS, 3, {"Cannonball!", "Chicken Fight", "Float & Chill", "Poolside Snacks", nullptr}, {"Cannonball!!", "The water's perfect!"}},
    {"Pregnancy Announcement", R_LIVING, 17, W_FAMILY, 3, {"Gather Everyone", "Make the Announcement", "Share the Ultrasound", "Accept Hugs", nullptr}, {"A BABY?!", "I'm gonna be an auntie!"}},
    {"Romantic Proposal", R_ROOFTOP, 20, W_PARTNER, 3, {"Set the Mood", "Give a Speech", "Pop the Question", "Celebrate", nullptr}, {"Oh my gosh...", "YES!"}},
    {"Protest", R_STREET, 12, W_FRIENDS, 3, {"Make Signs", "Chant", "Give a Speech", "Rally the Crowd", nullptr}, {"Change starts now!", "Louder!"}},
    {"Reality Show Taping", R_DOWNTOWN, 14, W_FRIENDS, 3, {"Confessional", "Stir Up Drama", "Make a Big Entrance", "Win Over the Viewers", nullptr}, {"Can I get that on camera?", "The viewers LOVE you!"}},
    {"Seasonal Party", R_LIVING, 18, W_FAMILY, 4, {"Decorate", "Serve a Holiday Feast", "Exchange Gifts", "Sing Carols", nullptr}, {"Happy holidays!", "Who invited Uncle Ray?"}},
    {"Shopping Spree", R_DTBOUTIQUE, 12, W_FRIENDS, 3, {"Try On Outfits", "Ask for Shopping Money", "Buy Something Fab", "Lunch Break", nullptr}, {"That's SO you!", "We deserve this."}},
    {"Sleepover", R_LIVING, 20, W_FRIENDS, 8, {"Pillow Fight!", "Truth or Dare", "Face Masks", "Midnight Snacks", nullptr}, {"*hits you with a pillow*", "Spill the tea!"}},
    {"Love Finder", R_CLUB, 20, W_SOLO, 8, {"Chat Up a Stranger", "Flirt", "Get a Number", "Ask Someone Out", nullptr}, {"Hey there...", "Come here often?"}},
    {"Friendship Finder", R_CAFE, 14, W_SOLO, 4, {"Say Hi to Someone New", "Find Common Ground", "Swap Numbers", "Make Plans", nullptr}, {"We should hang out!", "Omg same!"}},
    {"Study Session", R_LIBRARY, 16, W_FRIENDS, 3, {"Review Notes", "Quiz Each Other", "Complain About Homework", "Ace the Practice Test", nullptr}, {"I'll never use algebra.", "Focus!"}},
    {"Teen Party", R_LIVING, 19, W_TEENS, 4, {"Blast the Music", "Dance", "Spin the Bottle (Truth)", "Group Selfie", nullptr}, {"Turn it up!", "Best. Party. Ever."}},
    {"University Acceptance", R_LIVING, 17, W_FAMILY, 2, {"Open the Letter", "Announce It", "Happy Tears", "Plan Your Dorm", nullptr}, {"I KNEW you'd get in!", "So proud!"}},
    {"Vacation Day", R_HOTEL, 10, W_FAMILY, 8, {"Hire a Nanny", "Room Service", "Explore the Sights", "Spa Treatment", nullptr}, {"Finally, a break.", "This is paradise."}},
    {"Prom", R_SCHOOL, 20, W_TEENS, 4, {"Grand Entrance", "Slow Dance", "Prom Photos", "Crown Royalty", nullptr}, {"You look amazing!", "Who's prom royalty?"}},
    {"Half Birthday", R_LIVING, 15, W_FRIENDS, 3, {"Half a Cake", "Make a Wish", "Open Gifts", "Party Games", nullptr}, {"Happy half birthday!", "Half the cake, all the fun."}},
};
static const int NT = sizeof(T) / sizeof(T[0]);

struct Plan { int8_t type = -1; uint16_t day = 0; uint8_t hour = 0; int16_t partner = -1; };
struct SE {
    uint32_t magic = 0x53455631;
    Plan plans[8];
    int8_t active = -1; uint8_t done = 0, hoursLeft = 0, golds = 0;
    int16_t partner = -1, guests[5] = {-1, -1, -1, -1, -1};
};
static SE S;
static int planDay = 0;

static bool okSim(int s) { return s >= 0 && s < (int)G.sims.size() && !simDead(s); }
static std::string first(int s) { const std::string& n = G.sims[s].name; return n.substr(0, n.find(' ')); }
static int nGoals(int t) { int n = 0; while (n < 5 && T[t].goals[n]) n++; return n; }
static int partnerOf() { int best = -1, bs = -1; for (const Rel& r : G.rels) { int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1; if (o >= 0 && okSim(o) && r.status >= RS_DATING && r.status <= RS_MARRIED && r.ro > bs) { bs = r.ro; best = o; } } return best; }

static void seSay(int s, const std::string& line) { for (Actor& a : G.rooms[G.house][G.room].actors) if (a.kind == 0 && a.id == s) { a.say = line; a.sayT = 3.f; } }
static void bringGuests(int t) {
    std::vector<int> g; const SeType& e = T[t];
    if (S.partner >= 0) g.push_back(S.partner);
    for (const Rel& r : G.rels) {
        if (g.size() >= 5) break;
        int o = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1; if (o < 0 || !okSim(o) || G.sims[o].household || o == S.partner) continue;
        const Stage st = G.sims[o].stage;
        const bool fam = isFamily(G.mc, o);
        bool ok = e.who == W_FAMILY ? (fam || r.fr >= 50) : e.who == W_TEENS ? st == ST_TEEN : e.who == W_KIDS ? (st == ST_CHILD || st == ST_TEEN) : e.who == W_PARTNER ? false : e.who == W_SOLO ? false : r.fr >= 30;
        if (ok && (MC().stage >= ST_YA) == (st >= ST_YA || e.who == W_FAMILY)) g.push_back(o);
    }
    auto& acts = G.rooms[G.house][G.room].actors;
    for (size_t k = 0; k < 5; k++) S.guests[k] = k < g.size() ? (int16_t)g[k] : -1;
    for (int s : g) {
        bool here = false; for (const Actor& a : acts) if (a.kind == 0 && a.id == s) here = true;
        if (!here) { for (int h = 0; h < 2; h++) for (int r = 0; r < R_COUNT; r++) { auto& v = G.rooms[h][r].actors; v.erase(std::remove_if(v.begin(), v.end(), [&](const Actor& a) { return a.kind == 0 && a.id == s; }), v.end()); }
            Actor a; a.kind = 0; a.id = (uint16_t)s; a.x = 40 + irand(320); a.tx = a.x; a.y = 205 + irand(25); a.stay = 240; acts.push_back(a); }
    }
}
static void startEvent(int t, int partner) {
    S.active = (int8_t)t; S.done = 0; S.hoursLeft = T[t].len; S.partner = (int16_t)(partner >= 0 ? partner : (T[t].who == W_PARTNER || t == 0 ? partnerOf() : -1));
    if (G.room != T[t].room) travelTo(T[t].room);
    bringGuests(t);
    toast(std::string(T[t].name) + " has started! Open Phone > Events for your goals.");
    sfxYay();
}
static void endEvent() {
    if (S.active < 0) return;
    const int t = S.active, n = nGoals(t), d = __builtin_popcount(S.done);
    const int medal = d >= n - 1 && d >= 3 ? 3 : d >= n / 2 + 1 ? 2 : d >= 1 ? 1 : 0;
    static const char* M[4] = {"No medal", "Bronze", "Silver", "GOLD"};
    char b[120]; snprintf(b, sizeof b, "%s ended: %s! (%d of %d goals)", T[t].name, M[medal], d, n); toast(b);
    if (medal == 3) {
        S.golds++;
        for (Sim& s : G.sims) if (s.household) addMoodlet(s, (std::string("Memory: ") + T[t].name).c_str(), EM_HAPPY, 2, 2880);
        addMemoryOnce(t == 0 ? MEM_LOVE : MEM_FUN, G.mc, S.partner, std::string("A gold-medal ") + T[t].name + "!");
        lxNews(std::string("A picture-perfect ") + T[t].name + " for " + first(G.mc));
        for (int g : S.guests) if (okSim(g)) changeRel(G.mc, g, 6, 0);
    } else if (medal >= 1) for (int g : S.guests) if (okSim(g)) changeRel(G.mc, g, 2, 0);
    S.active = -1;
}

SeMenu seApp() {
    SeMenu v; char b[96];
    if (S.active >= 0) {
        const SeType& e = T[S.active];
        snprintf(b, sizeof b, "%s - %d hr left", e.name, S.hoursLeft); v.push_back({b, 14999});
        for (int k = 0; k < nGoals(S.active); k++) { snprintf(b, sizeof b, "%s %s", (S.done >> k) & 1 ? "[x]" : "[ ]", e.goals[k]); v.push_back({b, (S.done >> k) & 1 ? 14999 : 14010 + k}); }
        v.push_back({"End the Event", 14020});
        return v;
    }
    snprintf(b, sizeof b, "Gold events: %d. Upcoming:", S.golds); v.push_back({b, 14999});
    for (const Plan& p : S.plans) if (p.type >= 0) { snprintf(b, sizeof b, "Day %d %d:00 - %s", p.day % 28 + 1, p.hour, T[p.type].name); v.push_back({b, 14999}); }
    v.push_back({"Start an Event Now...", 14030});
    return v;
}
SeMenu sePlanMenu(int day) {
    planDay = day; SeMenu v; v.push_back({"Plan a social event:", 14999});
    for (int k = 0; k < NT; k++) v.push_back({T[k].name, 14100 + k});
    return v;
}
SeMenu seOption(int id) {
    Sim& me = MC(); Activity a; char b[120];
    if (id >= 14100 && id < 14100 + NT) {
        const int t = id - 14100;
        if ((t == 0 || t == 18 || t == 28 || t == 9) && partnerOf() < 0) { toast("You need a partner for that one."); return {}; }
        if (t == 3 && me.stage < ST_YA) { toast("Adults only."); return {}; }
        int s = -1; for (int i = 0; i < 8; i++) if (S.plans[i].type < 0) { s = i; break; } if (s < 0) { toast("Your calendar is full (8 events)."); return {}; }
        S.plans[s].type = (int8_t)t; S.plans[s].day = (uint16_t)std::max<int>(planDay, G.day); S.plans[s].hour = T[t].hour; S.plans[s].partner = (int16_t)partnerOf();
        Event e; e.day = S.plans[s].day; e.hour = T[t].hour; e.type = EV_PLAN; e.loc = T[t].room; e.text = T[t].name; G.events.push_back(e);
        snprintf(b, sizeof b, "%s planned for day %d at %d:00!", T[t].name, S.plans[s].day % 28 + 1, T[t].hour); toast(b); return {};
    }
    if (id >= 14200 && id < 14200 + NT) {
        if (S.active >= 0) { toast("Finish the current event first."); return {}; }
        const int t = id - 14200; if ((t == 0 || t == 18 || t == 28 || t == 9) && partnerOf() < 0) { toast("You need a partner for that one."); return {}; }
        startEvent(t, -1); return seApp();
    }
    if (id == 14030) { SeMenu v; v.push_back({"Start which event?", 14999}); for (int k = 0; k < NT; k++) v.push_back({T[k].name, 14200 + k}); return v; }
    if (id == 14020) { endEvent(); return {}; }
    if (id >= 14010 && id < 14015 && S.active >= 0) {
        const int t = S.active, k = id - 14010; const SeType& e = T[t];
        std::string gname = e.goals[k];
        if (t == 3 && k == 2 && spicyLevel() == 0) gname = "Party Games";   // dancers only with Spicy Mode on
        a = makeAct(gname.c_str(), 30, 8); a.rate[N_FUN] = 12; a.rate[N_SOCIAL] = 12; startActivity(a);
        S.done |= (uint8_t)(1 << k);
        if (t == 0 && k == 1) {   // the vows: actually get married
            const int p = S.partner;
            if (okSim(p)) { Rel& r = rel(G.mc, p); if (r.status < RS_ENGAGED) r.status = RS_ENGAGED; if (r.status == RS_ENGAGED) { lifeMarry(G.mc, p); toast("You said \"I do\"! You're MARRIED to " + first(p) + "!"); addMoodlet(me, "Just Married!", EM_HAPPY, 3, 2880); addMemoryOnce(MEM_LOVE, G.mc, p, "Married " + first(p) + "!"); } }
        }
        if (t == 28 && k == 2) { const int p = S.partner; if (okSim(p)) { if (irand(100) < 40 + rel(G.mc, p).ro / 2) { rel(G.mc, p).status = RS_ENGAGED; toast(first(p) + " said YES! You're engaged!"); addMemoryOnce(MEM_LOVE, G.mc, p, "Got engaged to " + first(p) + "!"); } else { toast(first(p) + ": \"I... I'm not ready.\""); addMoodlet(me, "Proposal Rejected", EM_SAD, 3, 1440); } } }
        if (t == 34 && k >= 2) { for (int o = 0; o < (int)G.sims.size(); o++) if (okSim(o) && !G.sims[o].household && canRomance(G.mc, o) && irand(10) == 0) { changeRel(G.mc, o, 10, 12); toast("You hit it off with " + first(o) + "!"); break; } }
        if (t == 35 && k >= 1) { for (int o = 0; o < (int)G.sims.size(); o++) if (okSim(o) && !G.sims[o].household && G.sims[o].stage == me.stage && irand(10) == 0) { changeRel(G.mc, o, 20, 0); toast("New friend: " + first(o) + "!"); break; } }
        if (t == 32 && k == 1) { G.money += 50 + irand(100); toast("Family chipped in some shopping money!"); }
        if (t == 11 && k == 2 && me.stage >= ST_ADULT) { G.money += 25; toast("The golden egg had $25 inside!"); }
        for (int g : S.guests) if (okSim(g) && irand(2)) { seSay(g, e.lines[irand(2)]); changeRel(G.mc, g, 2, 0); tweenPlay(g, irand(2) ? TW_CHEER : TW_CLAP, 1.f); }
        if (gname.find("Pop the Question") != std::string::npos || gname.find("Vows") != std::string::npos) tweenPlay(G.mc, TW_KNEEL, 2.5f);
        else if (gname.find("Pillow") != std::string::npos) { tweenPlay(G.mc, TW_SWING, 1.5f, 2); for (int g : S.guests) if (okSim(g)) tweenPlay(g, TW_SWING, 1.5f, 2); }
        else if (gname.find("Dance") != std::string::npos) { tweenPlay(G.mc, TW_SPIN, 1.4f); if (okSim(S.partner)) tweenPlay(S.partner, TW_SPIN, 1.4f); }
        else if (gname.find("Hug") != std::string::npos || gname.find("Hold Hands") != std::string::npos) { tweenPlay(G.mc, TW_LEANR); if (okSim(S.partner)) tweenPlay(S.partner, TW_LEANL); }
        else tweenPlay(G.mc, TW_HOP, 0.6f);
        return seApp();
    }
    return {};
}

void seHourly(int hour) {
    if (G.sims.empty()) return;
    if (S.active >= 0) { if (S.hoursLeft) S.hoursLeft--; if (!S.hoursLeft) endEvent(); else if (irand(3) == 0) for (int g : S.guests) if (okSim(g) && irand(3) == 0) { seSay(g, T[S.active].lines[irand(2)]); break; } }
    for (Plan& p : S.plans) if (p.type >= 0 && p.day == G.day && p.hour == hour) {
        if (S.active >= 0) endEvent();
        const int t = p.type, partner = p.partner; p.type = -1; startEvent(t, partner); break;
    }
    for (Plan& p : S.plans) if (p.type >= 0 && p.day < G.day) p.type = -1;   // missed
}

void seSaveTo(const std::string& path) { FILE* f = fopen(path.c_str(), "wb"); if (!f) return; fwrite(&S, sizeof S, 1, f); fclose(f); }
void seLoadFrom(const std::string& path) {
    S = SE{}; FILE* f = fopen(path.c_str(), "rb"); if (!f) return;
    SE s; size_t n = fread(&s, 1, sizeof s, f); fclose(f);
    if (n != sizeof s || s.magic != 0x53455631) return;
    for (Plan& p : s.plans) if (p.type >= NT) p.type = -1;
    if (s.active >= NT) s.active = -1;
    S = s;
}
