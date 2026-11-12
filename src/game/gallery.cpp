#include "lifeext.hpp"
// Home Gallery: an offline house catalog (no internet, accounts or downloads - everything ships in the game).
#include "gallery.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "catalog.hpp"
#include "extras.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <string>
using namespace col;
void toast(const std::string& m);
void furnishExtraRooms(RoomState* r);
void farmOnMainMove(int from, int to, bool furnished);

namespace {
void ol(float x, float y, float w, float h, RGB c, float r = 3) { rrect(x - 1.5f, y - 1.5f, w + 3, h + 3, r + 1.5f, C(UI_INK)); rrect(x, y, w, h, r, C(c)); }
enum HType { HT_APT, HT_HOUSE, HT_TOWN, HT_DUPLEX, HT_LOFT, HT_CONDO, HT_PENT, HT_VILLA, HT_FARM, HT_SHARED, HT_LOT };
const char* HT_N[] = {"Apartment", "House", "Townhouse", "Duplex", "Loft", "Condo", "Penthouse", "Villa", "Farmhouse", "Shared House", "Empty Lot"};
struct House {
    const char* code; const char* name; int type; int price; bool rent; int beds, baths; const char* hood; const char* tags; const char* features;
    int built;   // 0 = not yet built; else style id (1..)
    int base;    // which home layout it uses: 0 cottage-style, 1 apartment-style
};
// style 1 cottage starter, 2 penthouse starter, 3 cozy studio, 4 garden cottage, 5 pink condo, 6 y2k pad, 7 nightlife penthouse, 8 farmhouse
const House H[] = {
    {"S01", "Cocoa Cottage", HT_HOUSE, 0, false, 2, 1, "Willow Lane", "starter,cozy", "your original home", 1, 0},
    {"S02", "Pink Penthouse", HT_PENT, 0, false, 2, 1, "Downtown", "starter,pink", "your original home", 2, 1},
    {"H001", "Rosewood Studio", HT_APT, 180, true, 1, 1, "Maple Heights", "starter,cozy,urban", "tiny kitchenette; balcony nook", 3, 1},
    {"H002", "Maple Corner Flat", HT_APT, 300, true, 1, 1, "Maple Heights", "starter,y2k,urban", "open-plan kitchen; pastel bedroom", 6, 1},
    {"H003", "Sunset Walk-Up", HT_APT, 450, true, 2, 1, "Maple Heights", "retro,y2k,urban", "fire-escape window; breakfast bar", 16, 1},
    {"H004", "Metro Microloft", HT_APT, 275, true, 1, 1, "Downtown", "minimal,modern,urban", "foldaway bed; compact kitchen", 10, 1},
    {"H005", "Larkspur Cottage", HT_HOUSE, 18000, false, 2, 1, "Willow Lane", "starter,cozy,cottage", "garden; front porch", 4, 0},
    {"H006", "Honeysuckle Bungalow", HT_HOUSE, 28000, false, 2, 1, "Willow Lane", "family,cottage,vintage", "laundry nook; backyard", 4, 0},
    {"H007", "Riverview Townhouse", HT_TOWN, 52000, false, 3, 2, "Riverside", "family,modern,townhouse", "two floors; balcony", 15, 0},
    {"H008", "The Peach Duplex", HT_DUPLEX, 43000, false, 2, 2, "Willow Lane", "modern,cozy,duplex", "split building; separate entry", 22, 0},
    {"H009", "Studio Artist Loft", HT_LOFT, 650, true, 1, 1, "Arts District", "artsy,industrial,urban", "easel corner; studio windows", 13, 1},
    {"H010", "Cherry Blossom Condo", HT_CONDO, 58000, false, 2, 2, "Downtown", "pink,luxury,modern", "vanity; rooftop access", 5, 1},
    {"H011", "Clubhouse Row", HT_TOWN, 82000, false, 3, 2, "Riverside", "social,modern,family", "patio; party kitchen", 22, 0},
    {"H012", "Bluebird Family Home", HT_HOUSE, 95000, false, 4, 2, "Willow Lane", "family,cozy,suburban", "nursery; playroom", 22, 0},
    {"H013", "Y2K Dream Pad", HT_APT, 900, true, 2, 2, "Downtown", "y2k,pink,luxury", "glossy bedroom; walk-in wardrobe", 6, 1},
    {"H014", "Nightlife Penthouse", HT_PENT, 2400, true, 3, 2, "Downtown", "luxury,urban,party", "rooftop lounge; skyline view", 7, 1},
    {"H015", "Glasshouse Villa", HT_VILLA, 320000, false, 5, 4, "East Crest", "luxury,modern,pool", "pool; formal dining", 14, 0},
    {"H016", "Old Brick Manor", HT_HOUSE, 155000, false, 4, 3, "East Crest", "vintage,legacy,family", "library; attic room", 17, 0},
    {"H017", "Maple Family Farmhouse", HT_FARM, 120000, false, 4, 2, "Countryside", "rustic,family,garden", "greenhouse; large kitchen", 8, 0},
    {"H018", "Solstice Courtyard", HT_HOUSE, 110000, false, 3, 2, "Riverside", "modern,zen,garden", "inner courtyard; patio", 13, 0},
    {"H019", "Starlight Starter", HT_HOUSE, 15000, false, 1, 1, "Willow Lane", "starter,minimal,suburban", "simple upgradeable layout", 3, 0},
    {"H020", "Aspen Student House", HT_SHARED, 550, true, 3, 2, "Arts District", "student,social,budget", "shared kitchen; study area", 23, 0},
    {"H021", "Harbor Breeze Cottage", HT_HOUSE, 73000, false, 3, 2, "Harbor Side", "beach,cozy,family", "deck; storage shed", 15, 0},
    {"H022", "Velvet Sky Suite", HT_PENT, 4000, true, 4, 3, "Downtown", "luxury,pink,urban", "walk-in closet; private terrace", 7, 1},
    {"H023", "Greenhouse Hideaway", HT_HOUSE, 80000, false, 2, 2, "Countryside", "plant,cozy,garden", "sunroom; herb garden", 21, 0},
    {"H024", "Blank Slate Lot", HT_LOT, 5000, false, 0, 0, "Willow Lane", "empty,build,custom", "place a new house blueprint here", 0, 0},
    // Sugar Bunny Life 2 house styles pack
    {"S101", "Villa Toscana", HT_VILLA, 140000, false, 4, 3, "East Crest", "tuscan,y2k,luxury", "terracotta, arches & a fountain", 9, 0},
    {"S102", "Neon Pod 2049", HT_APT, 1200, true, 1, 1, "Downtown", "cyber,y2k,futuristic", "chrome walls; LED everything", 10, 1},
    {"S103", "The Brownstone Study", HT_TOWN, 98000, false, 3, 2, "Riverside", "urban,intellectual,luxury", "library wall; jazz corner", 11, 0},
    {"S104", "Harborview Colonial", HT_HOUSE, 115000, false, 4, 2, "Harbor Side", "preppy,colonial,family", "navy shutters; sailing stripes", 12, 0},
    {"S105", "Indigo Row House", HT_TOWN, 46000, false, 2, 1, "Arts District", "indie,boho,artsy", "murals; plants everywhere", 13, 0},
    {"S106", "Glossy Glam Estate", HT_VILLA, 210000, false, 5, 4, "East Crest", "glam,2010s,modern", "marble, gold & a grand staircase", 14, 0},
    {"S107", "Maple Craftsman", HT_HOUSE, 62000, false, 3, 2, "Willow Lane", "cozy,craftsman,family", "front porch swing; fireplace", 15, 0},
    {"S108", "Mirror Ball Loft", HT_LOFT, 1600, true, 2, 1, "Downtown", "y2k,glam,silver", "mirrors & silver chrome", 16, 1},
    {"S109", "Velvet Heritage Home", HT_HOUSE, 135000, false, 4, 3, "East Crest", "classic,luxury,2000s", "rich velvet & family portraits", 17, 0},
    {"S110", "Studio B Baddie Pad", HT_APT, 1100, true, 2, 1, "Downtown", "y2k,music,baddie", "recording booth bedroom", 18, 1},
    {"S111", "Princess Double-Wide", HT_HOUSE, 9000, false, 2, 1, "Sunny Acres", "trailer,retro,glam", "pink flamingos & fairy lights", 19, 0},
    {"S112", "Woodgrain '78", HT_HOUSE, 7500, false, 2, 1, "Sunny Acres", "trailer,1970s,retro", "wood paneling & shag carpet", 20, 0},
    {"S113", "Boho Airstream", HT_HOUSE, 11000, false, 1, 1, "Sunny Acres", "trailer,boho,maximalist", "macrame, rugs & a garden", 21, 0},
    {"S114", "The Family Place", HT_HOUSE, 88000, false, 4, 2, "Willow Lane", "suburban,family,classic", "big kitchen; three generations of photos", 22, 0},
    {"S115", "Brick Row Rental", HT_APT, 400, true, 2, 1, "Maple Heights", "brick,affordable,city", "radiator heat; stoop hangouts", 23, 1},
    {"S116", "Community Court Apt", HT_APT, 250, true, 1, 1, "Maple Heights", "community,affordable,cozy", "courtyard barbecues", 24, 1},
};
const int NH = sizeof(H) / sizeof(H[0]);

// list state
enum View { V_LIST, V_DETAIL, V_CONFIRM };
int view = V_LIST, page = 0, sel = 0, tab = 0, filt = 0, sortBy = 0, confirmFurn = 1;
bool onlyAfford = false;
std::vector<int> shown;

bool owned(int i) { return i < 2 || ((G.homeOwned >> i & 1)); }
bool isFav(int i) { return G.homeFav >> i & 1; }
int cost(int i) { return owned(i) ? 0 : H[i].price + (confirmFurn && H[i].built && !H[i].rent ? H[i].price / 10 : 0); }
void rebuild() {
    shown.clear();
    for (int i = 0; i < NH; i++) {
        if (tab == 1 && !isFav(i)) continue;
        if (filt == 1 && !H[i].rent) continue;
        if (filt == 2 && (H[i].rent || i < 2)) continue;
        if (onlyAfford && !owned(i) && H[i].price > G.money) continue;
        shown.push_back(i);
    }
    if (sortBy == 1) std::stable_sort(shown.begin(), shown.end(), [](int a, int b) { return H[a].price < H[b].price; });
    if (sortBy == 2) std::stable_sort(shown.begin(), shown.end(), [](int a, int b) { return std::string(H[a].name) < H[b].name; });
    auto rank = [](int i) { return H[i].built >= 9 ? 0 : (H[i].built || i < 2) ? 1 : 2; };   // new style pack first, then built, then planned
    std::stable_sort(shown.begin(), shown.end(), [&](int a, int b) { return rank(a) < rank(b); });
    int pages = std::max(1, ((int)shown.size() + 5) / 6); if (page >= pages) page = pages - 1;
}
std::string money(int v) { char b[24]; if (v >= 1000) snprintf(b, sizeof b, "$%d,%03d", v / 1000, v % 1000); else snprintf(b, sizeof b, "$%d", v); return b; }
std::string priceTag(int i) { if (i < 2) return "Owned"; if (owned(i)) return "Owned"; return money(H[i].price) + (H[i].rent ? "/wk" : ""); }

// ---- furnishing presets (the same furniture items as the catalog, so they work like any other piece)
void add(RoomState& r, int sh, int c, int x, int y) { r.items.push_back({(uint16_t)itemId(sh, c), (int16_t)x, (int16_t)y}); }
void theme(RoomState* r, int style) {
    struct T { uint8_t lw, lp, lf, lfp, bw, bp, bf, bfp, kw, kf, kfp; };
    static const T TH[25] = {
        {1, 9, 0, 4, 0, 4, 3, 4, 1, 3, 2}, {1, 9, 0, 4, 0, 4, 3, 4, 1, 3, 2}, {0, 6, 4, 5, 0, 6, 4, 5, 10, 7, 0},
        {8, 8, 8, 1, 1, 2, 0, 1, 1, 3, 2},     // cozy studio: loft walls, honey planks
        {7, 5, 0, 4, 6, 5, 3, 4, 1, 3, 2},     // garden cottage: sage & fairy vines
        {0, 2, 3, 0, 0, 7, 4, 5, 10, 7, 0},    // cherry blossom condo: blush damask, gloss
        {4, 9, 5, 5, 0, 6, 4, 5, 1, 4, 6},     // y2k dream pad: hot pink kawaii, fluffy floors
        {9, 6, 6, 0, 9, 3, 6, 0, 9, 6, 2},     // nightlife penthouse: black & starry
        {8, 11, 1, 1, 1, 1, 0, 1, 1, 11, 2},   // farmhouse: sage living, wood
        {8, 2, 11, 1, 8, 2, 11, 1, 8, 8, 2},    // tuscan: warm sand, damask, terracotta
        {9, 6, 7, 0, 9, 6, 10, 0, 9, 7, 0},     // cyber: black & starry, chrome
        {3, 2, 2, 1, 2, 2, 2, 4, 1, 3, 2},      // brownstone study: deep wood
        {10, 1, 3, 1, 6, 1, 3, 4, 10, 10, 2},   // preppy colonial: white & navy stripes
        {5, 5, 11, 1, 7, 5, 8, 4, 1, 3, 2},     // boho row: lilac & vines
        {10, 2, 7, 0, 10, 2, 3, 0, 10, 7, 0},   // high glam: white damask, marble gloss
        {8, 8, 0, 1, 1, 8, 0, 1, 1, 0, 1},      // craftsman: loft wood
        {10, 6, 7, 0, 9, 6, 7, 0, 10, 7, 0},    // mirror & silver
        {11, 2, 2, 1, 11, 2, 2, 3, 1, 2, 1},    // velvet heritage: berry damask
        {4, 9, 6, 0, 9, 6, 5, 5, 4, 6, 2},      // baddie music: hot pink & black
        {0, 9, 4, 6, 0, 1, 4, 5, 0, 4, 6},      // trailer princess: pink tiles
        {2, 8, 1, 3, 2, 8, 11, 3, 8, 1, 1},     // 70s woodgrain & shag
        {7, 5, 8, 3, 5, 5, 8, 3, 1, 8, 1},      // boho trailer
        {1, 11, 0, 4, 6, 2, 0, 4, 1, 3, 2},     // family classic
        {2, 0, 1, 1, 8, 0, 1, 1, 1, 3, 2},      // brick row rental
        {6, 0, 3, 1, 7, 0, 3, 1, 1, 7, 2},      // community apartment
    };
    const T& t = TH[std::clamp(style, 0, 24)];
    r[R_LIVING].wall = t.lw; r[R_LIVING].wallPat = t.lp; r[R_LIVING].floor = t.lf; r[R_LIVING].floorPat = t.lfp;
    r[R_BEDROOM].wall = t.bw; r[R_BEDROOM].wallPat = t.bp; r[R_BEDROOM].floor = t.bf; r[R_BEDROOM].floorPat = t.bfp;
    r[R_KITCHEN].wall = t.kw; r[R_KITCHEN].floor = t.kf; r[R_KITCHEN].floorPat = t.kfp;
}
void furnish(RoomState* r, int styleIn) {
    const int style = styleIn >= 9 ? 3 + (styleIn - 9) % 6 : styleIn;   // the style pack reuses the furniture sets, with its own colors
    for (int rm : {R_LIVING, R_KITCHEN, R_BEDROOM, R_CLOSET}) r[rm].items.clear();
    // kitchen: every home needs the essentials (fridge, counter, oven, sink)
    const int kc = style == 7 ? 9 : style == 8 ? 5 : style == 6 ? 1 : 4;
    add(r[R_KITCHEN], style == 6 ? SH_RETROFRIDGE : SH_FRIDGE, style == 7 ? 9 : 0, 190, 196); add(r[R_KITCHEN], SH_COUNTER, kc, 250, 196); add(r[R_KITCHEN], SH_OVEN, 9, 312, 196); add(r[R_KITCHEN], SH_SINK, kc, 370, 196);
    add(r[R_KITCHEN], SH_TABLE, kc, 120, 232); add(r[R_KITCHEN], SH_CHAIR, kc, 80, 232);
    // bathroom: toilet, shower/tub, vanity
    add(r[R_CLOSET], SH_TOILET, 4, 160, 206); add(r[R_CLOSET], style == 5 || style == 7 ? SH_SOAKTUB : SH_SHOWER, 4, 300, 220); add(r[R_CLOSET], SH_BATHVANITY, 4, 60, 206); add(r[R_CLOSET], SH_MIRROR, 7, 60, 80);
    switch (style) {
    case 3:   // Rosewood Studio: small & cozy
        add(r[R_LIVING], SH_PLAIDSOFA, 5, 220, 206); add(r[R_LIVING], SH_TV, 9, 340, 176); add(r[R_LIVING], SH_FLOORLAMP, 7, 120, 200); add(r[R_LIVING], SH_PLANTSHELF, 4, 60, 70); add(r[R_LIVING], SH_RUG, 5, 220, 228);
        add(r[R_BEDROOM], SH_IRONBED, 4, 300, 214); add(r[R_BEDROOM], SH_DESKLAMP, 7, 200, 214); add(r[R_BEDROOM], SH_WINDOWSEAT, 4, 90, 210); add(r[R_BEDROOM], SH_POLAROIDS, 0, 300, 80); break;
    case 4:   // Larkspur Cottage: garden florals
        add(r[R_LIVING], SH_FLOWERCUSHION, 0, 140, 226); add(r[R_LIVING], SH_SOFA, 4, 240, 206); add(r[R_LIVING], SH_TV, 9, 350, 176); add(r[R_LIVING], SH_FLOWERS, 1, 40, 226); add(r[R_LIVING], SH_MONSTERA, 7, 380, 230); add(r[R_LIVING], SH_VINES, 7, 200, 40);
        add(r[R_BEDROOM], SH_PATCHWORKBED, 0, 300, 214); add(r[R_BEDROOM], SH_NIGHTSTAND, 4, 220, 214); add(r[R_BEDROOM], SH_PLANTSHELF, 7, 100, 80); add(r[R_BEDROOM], SH_DRESSER, 4, 90, 206); break;
    case 5:   // Cherry Blossom Condo: pink luxury, vanity
        add(r[R_LIVING], SH_PLUSHSOFA, 0, 232, 206); add(r[R_LIVING], SH_TV, 9, 350, 176); add(r[R_LIVING], SH_FLUFFYRUG, 0, 230, 226); add(r[R_LIVING], SH_FLOWERS, 0, 60, 226); add(r[R_LIVING], SH_DRUMLAMP, 7, 120, 200);
        add(r[R_BEDROOM], SH_CANOPYBED, 0, 300, 214); add(r[R_BEDROOM], SH_HOLLYVANITY, 7, 80, 206); add(r[R_BEDROOM], SH_STILETTOCHAIR, 1, 150, 228); add(r[R_BEDROOM], SH_ROUNDRUG, 0, 230, 232); break;
    case 6:   // Y2K Dream Pad: glossy pink + walk-in wardrobe vibes
        add(r[R_LIVING], SH_EGGCHAIR, 1, 140, 226); add(r[R_LIVING], SH_PLUSHSOFA, 1, 250, 206); add(r[R_LIVING], SH_TV, 9, 350, 176); add(r[R_LIVING], SH_HEARTRUG, 1, 240, 230); add(r[R_LIVING], SH_DISCOBALL, 7, 200, 40);
        add(r[R_BEDROOM], SH_PLUSHBED, 1, 300, 214); add(r[R_BEDROOM], SH_WARDROBE, 0, 70, 206); add(r[R_BEDROOM], SH_CLOTHESRACK, 1, 150, 210); add(r[R_BEDROOM], SH_LIPSPILLOW, 1, 220, 232); add(r[R_BEDROOM], SH_STARLIGHTS, 1, 300, 54); break;
    case 7:   // Nightlife Penthouse: black gloss, party-ready
        add(r[R_LIVING], SH_SOFA, 9, 232, 206); add(r[R_LIVING], SH_TV, 9, 350, 176); add(r[R_LIVING], SH_SPEAKER, 9, 60, 200); add(r[R_LIVING], SH_DISCOBALL, 7, 200, 40); add(r[R_LIVING], SH_RUG, 10, 230, 228); add(r[R_LIVING], SH_RECORDPLAYER, 9, 130, 210);
        add(r[R_BEDROOM], SH_BED, 9, 300, 214); add(r[R_BEDROOM], SH_NIGHTSTAND, 9, 220, 214); add(r[R_BEDROOM], SH_FLOORMIRROR, 7, 80, 206); add(r[R_BEDROOM], SH_RUG, 10, 300, 232); break;
    case 8:   // Maple Family Farmhouse: rustic + big kitchen + greenery
        add(r[R_LIVING], SH_PLAIDSOFA, 8, 232, 206); add(r[R_LIVING], SH_TV, 6, 350, 176); add(r[R_LIVING], SH_BOOKSHELF, 6, 60, 200); add(r[R_LIVING], SH_MONSTERA, 7, 380, 230); add(r[R_LIVING], SH_RUG, 8, 230, 228);
        add(r[R_KITCHEN], SH_PLANTSHELF, 7, 120, 70); add(r[R_KITCHEN], SH_FLOWERS, 1, 30, 226);
        add(r[R_BEDROOM], SH_PATCHWORKBED, 8, 300, 214); add(r[R_BEDROOM], SH_WARDROBE, 6, 80, 206); add(r[R_BEDROOM], SH_CANDLES, 7, 210, 214); add(r[R_BEDROOM], SH_RUG, 8, 300, 232); break;
    default:  // the two original homes
        add(r[R_LIVING], SH_PLUSHSOFA, 0, 232, 206); add(r[R_LIVING], SH_TV, 9, 340, 176); add(r[R_LIVING], SH_FLUFFYRUG, 0, 220, 222); add(r[R_LIVING], SH_DRUMLAMP, 7, 110, 200);
        add(r[R_BEDROOM], SH_BRASSBED, 0, 326, 214); add(r[R_BEDROOM], SH_OVALVANITY, 7, 64, 204); add(r[R_BEDROOM], SH_NIGHTSTAND, 7, 252, 200); break;
    }
    theme(r, styleIn);
    furnishExtraRooms(r);
}
void moveIn(int i, bool furnished) {
    const House& h = H[i];
    const int from = G.house, to = h.base;
    // household (and pets) follow you; everyone else stays behind
    std::vector<Actor> moving;   // gather first: moving within the same layout must not re-visit people we just placed
    for (int r = 0; r < R_COUNT; r++) {
        auto& v = G.rooms[from][r].actors;
        for (size_t k = 0; k < v.size();) {
            if (v[k].kind == 1 || (v[k].id < G.sims.size() && G.sims[v[k].id].household)) { moving.push_back(v[k]); v.erase(v.begin() + k); }
            else k++;
        }
    }
    for (Actor a : moving) {
        a.x = 60 + irand(280); a.tx = a.x; a.y = 200 + irand(30);
        int dest = (a.kind == 0 && G.sims[a.id].stage == ST_INFANT) ? R_NURSERY : R_LIVING;
        G.rooms[to][dest].actors.push_back(a);
    }
    if (!furnished && to != from)   // bring your own furniture: it comes with you
        for (int r = 0; r < R_COUNT; r++) if (roomIsHome(r)) std::swap(G.rooms[from][r].items, G.rooms[to][r].items);
    if (furnished) furnish(G.rooms[to], h.built);
    else theme(G.rooms[to], h.built);
    farmOnMainMove(from, to, furnished);   // a second home (Apple Valley farmhouse) shifts to the free slot
    G.house = to; G.room = R_LIVING;
    G.homeId = (uint8_t)i; G.rentMiss = 0;
    addMemory(MEM_HOME, G.mc, -1, std::string("Moved into ") + h.name + " in " + h.hood + "! A fresh start.");
    addMoodlet(MC(), "New Home!", EM_HAPPY, 2, 1440);
}
// ---- layout
Btn tabB(int k) { return Btn{4 + k * 80.f, 2, 76, 20}; }
Btn cardB(int k) { return Btn{6 + (k % 2) * 156.f, 50 + (k / 2) * 50.f, 152, 46}; }
const Btn bBack{4, 208, 64, 28}, bPrev{72, 208, 44, 28}, bNext{120, 208, 44, 28}, bSort{168, 208, 72, 28}, bAfford{244, 208, 72, 28};
const Btn bFav{6, 150, 100, 26}, bFurn{112, 150, 202, 26}, bBare{112, 180, 202, 26}, bDBack{6, 208, 100, 28}, bYes{170, 208, 144, 28};
}

int galleryHouseCount() { return NH; }
void galleryMoveIn(int i) { moveIn(std::max(0, std::min(i, NH - 1)), true); }
const char* galleryHomeName() { return H[std::min<int>(G.homeId, NH - 1)].name; }
void galleryOpen() { view = V_LIST; rebuild(); }
void galleryFurnishStyle(int slot, int style) { if (slot >= 0 && slot < 2) furnish(G.rooms[slot], style); }

void galleryDaily() {
    if (G.sims.empty()) return;
    const int i = std::min<int>(G.homeId, NH - 1);
    if (!H[i].rent || owned(i) || G.day % 7 != 0) return;
    const int due = lxRentShare(H[i].price) + (G.rentMiss ? 25 : 0);   // roommates pay their share; late fee
    if (G.money >= due) { G.money -= due; G.rentMiss = 0; toast(std::string("Rent paid for ") + H[i].name + ": " + money(due) + (due < H[i].price ? " (your share - roommates split the rest)." : ".")); return; }
    G.rentMiss++;
    if (G.rentMiss >= 4) { toast(std::string("The landlord's move-out date arrived. You leave ") + H[i].name + " for Cocoa Cottage."); sfxNo(); moveIn(0, false); return; }
    static const char* W[3] = {"Landlord: \"Rent's late - there's a $25 late fee next week.\"", "Landlord: \"FINAL warning. Pay next week.\"", "Landlord: \"Eviction notice. Pay by next week or you're moving out.\""};
    toast(W[std::min(2, G.rentMiss - 1)]); sfxNo();
}

bool galleryUpdate(const Input& in) {
    if (in.down & KEY_START) return false;
    if (view == V_LIST) {
        if (in.down & KEY_B) return false;
        if (in.down & (KEY_R | KEY_DRIGHT)) { page++; rebuild(); }
        if (in.down & (KEY_L | KEY_DLEFT)) { page = std::max(0, page - 1); }
        if (!in.touch) return true;
        for (int k = 0; k < 2; k++) if (tabB(k).in(in.tp)) { tab = k; page = 0; rebuild(); sfxTap(); return true; }
        for (int k = 0; k < 3; k++) if (Btn{164 + k * 52.f, 2, 50, 20}.in(in.tp)) { filt = k; page = 0; rebuild(); sfxTap(); return true; }
        for (int k = 0; k < 6; k++) {
            int idx = page * 6 + k; if (idx >= (int)shown.size()) break;
            if (cardB(k).in(in.tp)) { sel = shown[idx]; view = V_DETAIL; confirmFurn = 1; sfxTap(); return true; }
        }
        if (bBack.in(in.tp)) return false;
        if (bPrev.in(in.tp)) { page = std::max(0, page - 1); sfxTap(); }
        if (bNext.in(in.tp)) { page++; rebuild(); sfxTap(); }
        if (bSort.in(in.tp)) { sortBy = (sortBy + 1) % 3; rebuild(); sfxTap(); }
        if (bAfford.in(in.tp)) { onlyAfford = !onlyAfford; page = 0; rebuild(); sfxTap(); }
        return true;
    }
    if (in.down & KEY_B) { view = view == V_CONFIRM ? V_DETAIL : V_LIST; return true; }
    if (!in.touch) return true;
    const House& h = H[sel];
    if (view == V_DETAIL) {
        if (bDBack.in(in.tp)) { view = V_LIST; rebuild(); sfxTap(); return true; }
        if (bFav.in(in.tp)) { G.homeFav ^= (1ull << sel); sfxTap(); return true; }
        if (!h.built) return true;
        if (sel == G.homeId) { if (bFurn.in(in.tp) || bBare.in(in.tp)) { toast("You already live here!"); sfxNo(); } return true; }
        if (bFurn.in(in.tp)) { confirmFurn = 1; view = V_CONFIRM; sfxTap(); }
        else if (bBare.in(in.tp)) { confirmFurn = 0; view = V_CONFIRM; sfxTap(); }
        return true;
    }
    // confirm
    if (bDBack.in(in.tp)) { view = V_DETAIL; sfxTap(); return true; }
    if (bYes.in(in.tp)) {
        const int c = h.rent ? h.price : cost(sel);   // rent: first week up front
        if (G.money < c) { toast("Not enough money for this home!"); sfxNo(); return true; }
        if (MC().stage < ST_YA && !h.rent && !owned(sel)) { toast("You need to be a young adult to buy a home."); sfxNo(); return true; }
        G.money -= c;   // charged once, right as the move succeeds
        if (!h.rent) G.homeOwned |= (1ull << sel);
        moveIn(sel, confirmFurn != 0);
        toast(std::string("Welcome home to ") + h.name + "!" + (h.rent ? " Rent is due every 7 days." : "")); sfxYay();
        return false;
    }
    return true;
}

// ---------------------------------------------------------------- drawing
static void exterior(int i, float x, float y, float t) {
    const House& h = H[i];
    grad(x, y, 200, 130, C(170, 210, 250), C(235, 245, 255));
    rect(x, y + 112, 200, 18, C(140, 200, 120));
    const RGB acc = h.built == 5 || h.built == 6 ? RGB{255, 170, 200} : h.built == 7 ? RGB{60, 50, 70} : h.built == 8 ? RGB{200, 90, 70} : RGB{240, 220, 190};
    switch (h.type) {
    case HT_APT: case HT_LOFT: case HT_SHARED:
        ol(x + 50, y + 22, 100, 92, RGB{190, 120, 100}, 2);
        for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) rect(x + 58 + c * 22.f, y + 30 + r * 20.f, 14, 12, C(255, 236, 170));
        rect(x + 90, y + 98, 20, 16, C(80, 50, 40)); break;
    case HT_CONDO: case HT_PENT:
        ol(x + 66, y + 6, 68, 108, acc, 2);
        for (int r = 0; r < 8; r++) for (int c = 0; c < 3; c++) rect(x + 72 + c * 20.f, y + 12 + r * 12.f, 14, 8, C(r == 0 ? RGB{255, 220, 120} : RGB{170, 210, 240}));
        if (h.type == HT_PENT) { rect(x + 60, y + 4, 80, 4, C(UI_INK)); circle(x + 128, y + 2, 3, C(255, 230, 120)); } break;
    case HT_FARM:
        ol(x + 30, y + 54, 110, 60, acc, 2); tri(x + 22, y + 56, x + 85, y + 20, x + 148, y + 56, C(110, 70, 60));
        rect(x + 75, y + 82, 20, 32, C(250, 240, 220)); rect(x + 40, y + 66, 18, 14, C(255, 236, 170)); rect(x + 112, y + 66, 18, 14, C(255, 236, 170));
        ol(x + 150, y + 64, 36, 50, RGB{220, 220, 210}, 2); tri(x + 146, y + 66, x + 168, y + 44, x + 190, y + 66, C(120, 120, 130)); break;   // silo
    case HT_LOT: rect(x + 30, y + 100, 140, 14, C(170, 140, 100)); ol(x + 92, y + 70, 4, 32, RGB{140, 100, 60}, 1); ol(x + 76, y + 64, 36, 14, RGB{250, 250, 240}, 1); break;
    default:
        ol(x + 45, y + 58, 110, 56, acc, 2); tri(x + 36, y + 60, x + 100, y + 22, x + 164, y + 60, C(h.built == 4 ? RGB{120, 160, 200} : RGB{150, 90, 80}));
        rect(x + 90, y + 82, 20, 32, C(120, 80, 60)); rect(x + 56, y + 70, 22, 16, C(255, 236, 170)); rect(x + 122, y + 70, 22, 16, C(255, 236, 170));
        if (h.built == 4) for (int k = 0; k < 6; k++) circle(x + 20 + k * 30.f, y + 114, 5, C(k % 2 ? RGB{255, 140, 180} : RGB{255, 220, 120})); break;
    }
    for (int k = 0; k < 3; k++) circle(x + 20 + k * 80.f + std::sin(t + k) * 6, y + 14 + k * 4.f, 6, C(255, 255, 255, 200));
}
static void floorPlan(int i, float x, float y) {
    const House& h = H[i];
    ol(x, y, 176, 110, RGB{250, 250, 245}, 2);
    if (h.type == HT_LOT) { text("Empty lot", x + 88, y + 46, 0.45f, C(UI_INK), 1); return; }
    struct Rm { const char* n; RGB c; };
    std::vector<Rm> rooms{{"Living", {255, 226, 236}}, {"Kitchen", {255, 240, 200}}};
    for (int k = 0; k < h.beds; k++) rooms.push_back({"Bed", {220, 210, 250}});
    for (int k = 0; k < h.baths; k++) rooms.push_back({"Bath", {200, 236, 240}});
    const int n = (int)rooms.size(), cols = n <= 4 ? 2 : 3, rows = (n + cols - 1) / cols;
    const float cw = 168.f / cols, ch = 102.f / rows;
    for (int k = 0; k < n; k++) {
        float rx = x + 4 + (k % cols) * cw, ry = y + 4 + (k / cols) * ch;
        rect(rx, ry, cw - 2, ch - 2, C(rooms[k].c)); text(rooms[k].n, rx + cw / 2 - 1, ry + ch / 2 - 7, 0.36f, C(UI_INK), 1);
    }
}
void galleryDrawTop(float t) {
    grad(0, 0, TOP_W, TOP_H, C(255, 240, 246), C(236, 228, 255));
    textOutline("HOME GALLERY", 200, 2, 0.6f, C(255, 255, 255), C(BERRY), 1);
    const int i = view == V_LIST ? (shown.empty() ? 0 : shown[std::min<int>((int)shown.size() - 1, page * 6)]) : sel;
    exterior(i, 8, 26, t);
    floorPlan(i, 216, 26);
    const House& h = H[i];
    char b[160];
    text(h.name, 10, 162, 0.55f, C(BERRY));
    snprintf(b, sizeof b, "%s  -  %s  -  %d bed / %d bath", HT_N[h.type], h.hood, h.beds, h.baths); text(b, 10, 182, 0.4f, C(UI_INK));
    snprintf(b, sizeof b, "%s   %s", priceTag(i).c_str(), h.rent ? "(rent, due weekly)" : owned(i) ? "" : "(buy)"); text(b, 10, 198, 0.4f, C(UI_INK));
    snprintf(b, sizeof b, "Features: %s", h.features); text(b, 10, 214, 0.36f, C(110, 90, 120));
    if (!h.built) textOutline("NOT YET BUILT", 108, 80, 0.55f, C(255, 255, 255), C(150, 120, 160), 1);
    snprintf(b, sizeof b, "You: %s   Home: %s", money(G.money).c_str(), galleryHomeName()); text(b, 390, 228, 0.34f, C(110, 90, 120), 2);
}
void galleryDrawBottom(float t) {
    (void)t;
    grad(0, 0, BOT_W, BOT_H, C(255, 246, 250), C(244, 236, 255));
    if (view == V_LIST) {
        button(tabB(0), "All Homes", tab == 0, 0, 0.38f); button(tabB(1), "Favorites", tab == 1, 3, 0.38f);
        static const char* F[3] = {"All", "Rent", "Buy"};
        for (int k = 0; k < 3; k++) button(Btn{164 + k * 52.f, 2, 50, 20}, F[k], filt == k, 2, 0.36f);
        char b[64]; const int pages = std::max(1, ((int)shown.size() + 5) / 6);
        snprintf(b, sizeof b, "%d homes  -  page %d/%d", (int)shown.size(), page + 1, pages); text(b, 160, 28, 0.38f, C(UI_INK), 1);
        if (shown.empty()) text(tab == 1 ? "No favorites yet - tap the heart on a home!" : "Nothing matches these filters.", 160, 110, 0.4f, C(UI_INK), 1);
        for (int k = 0; k < 6; k++) {
            int idx = page * 6 + k; if (idx >= (int)shown.size()) break;
            const int i = shown[idx]; const House& h = H[i]; const Btn c = cardB(k);
            const bool here = i == G.homeId;
            rrect(c.x, c.y, c.w, c.h, 6, C(h.built ? (here ? RGB{255, 220, 236} : RGB{255, 255, 255}) : RGB{226, 222, 230}));
            ol(c.x, c.y, c.w, c.h, h.built ? RGB{255, 255, 255} : RGB{226, 222, 230}, 1);
            text(h.name, c.x + 6, c.y + 4, 0.36f, C(h.built ? UI_INK : RGB{140, 130, 150}));
            snprintf(b, sizeof b, "%s  %db/%dba", priceTag(i).c_str(), h.beds, h.baths); text(b, c.x + 6, c.y + 18, 0.34f, C(h.built ? BERRY : RGB{150, 140, 160}));
            text(here ? "You live here" : h.built ? HT_N[h.type] : "Not yet built", c.x + 6, c.y + 31, 0.32f, C(110, 100, 120));
            if (isFav(i)) heart(c.x + c.w - 12, c.y + 10, 6, C(255, 90, 140));
        }
        button(bBack, "Back", false, 1, 0.4f); button(bPrev, "<", false, 0, 0.45f); button(bNext, ">", false, 0, 0.45f);
        static const char* S[3] = {"Sort: --", "Sort: $", "Sort: A-Z"};
        button(bSort, S[sortBy], false, 3, 0.36f); button(bAfford, onlyAfford ? "Can afford" : "Any price", onlyAfford, 2, 0.34f);
        return;
    }
    const House& h = H[sel];
    char b[200];
    text(h.name, 160, 6, 0.5f, C(BERRY), 1);
    snprintf(b, sizeof b, "%s in %s", HT_N[h.type], h.hood); text(b, 160, 26, 0.38f, C(UI_INK), 1);
    snprintf(b, sizeof b, "Tags: %s", h.tags); text(b, 160, 42, 0.34f, C(110, 100, 120), 1);
    if (view == V_DETAIL) {
        if (!h.built) { text("This home is NOT YET BUILT.", 160, 80, 0.45f, C(UI_INK), 1); text("It's planned for a future update.", 160, 100, 0.38f, C(110, 100, 120), 1); }
        else if (sel == G.homeId) text("You live here!", 160, 90, 0.5f, C(BERRY), 1);
        else {
            snprintf(b, sizeof b, "%s %s", h.rent ? "Rent:" : owned(sel) ? "You own this home." : "Price:", h.rent || !owned(sel) ? priceTag(sel).c_str() : "");
            text(b, 160, 70, 0.45f, C(UI_INK), 1);
            text("Furnished = new matching furniture for the main rooms.", 160, 96, 0.32f, C(110, 100, 120), 1);
            text("Bring my furniture = your current pieces move with you.", 160, 110, 0.32f, C(110, 100, 120), 1);
            if (!h.rent && !owned(sel)) { snprintf(b, sizeof b, "(furnished adds %s for the furniture)", money(h.price / 10).c_str()); text(b, 160, 126, 0.32f, C(110, 100, 120), 1); }
            button(bFurn, "Move In Furnished", true, 2, 0.4f);
            button(bBare, "Move In - Bring My Furniture", false, 0, 0.38f);
        }
        button(bFav, isFav(sel) ? "Unfavorite" : "Favorite", isFav(sel), 3, 0.38f);
        button(bDBack, "Back", false, 1, 0.4f);
        return;
    }
    const int c = h.rent ? h.price : cost(sel);
    snprintf(b, sizeof b, "Move into %s %s?", h.name, confirmFurn ? "(furnished)" : "(your furniture)"); text(b, 160, 70, 0.4f, C(UI_INK), 1);
    snprintf(b, sizeof b, "You pay now: %s", money(c).c_str()); text(b, 160, 96, 0.45f, C(BERRY), 1);
    if (h.rent) text("Then the same rent every 7 days. Miss it twice = evicted.", 160, 120, 0.34f, C(110, 100, 120), 1);
    if (confirmFurn && h.base == G.house) text("Furnished replaces the furniture in your main rooms.", 160, 138, 0.32f, C(200, 60, 90), 1);
    snprintf(b, sizeof b, "Your money: %s", money(G.money).c_str()); text(b, 160, 160, 0.38f, C(UI_INK), 1);
    button(bDBack, "Cancel", false, 1, 0.4f);
    button(bYes, G.money >= c ? "Confirm & Move" : "Not enough money", G.money >= c, 2, 0.38f);
}
