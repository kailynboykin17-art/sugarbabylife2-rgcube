#include "town2.hpp"
#include "funpark.hpp"
#include "spicy.hpp"
#include "hotdate.hpp"
#include "nightlife.hpp"
// Districts: each town destination is a little strip of walkable screens (L/R to
// move between them) full of objects you walk up to and use - clothes racks, a claw
// machine, a jukebox, a cupcake case, an ice cream cart, your neighbours' front doors...
#include "bgs.hpp"
#include "worldsim.hpp"
#include "travelcourt.hpp"
#include "health.hpp"
#include "lifestory.hpp"
#include "district.hpp"
#include "farm.hpp"
#include "lockup.hpp"
#include "travelcourt.hpp"
#include "economy.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "goals.hpp"
#include "extras.hpp"
#include "catalog.hpp"
#include "social.hpp"
#include "cutscene.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>

using namespace col;

// ---------------------------------------------------------------- districts
static const int DIST[][6] = {
    {R_MALL, R_MALL_BEAUTY, R_ARCADE, R_MARKET, R_FOODCOURT, -1}, {R_CAFE, R_BAKERY, -1}, {R_PARK, R_PLAYGROUND, -1}, {R_RESTAURANT, R_DINER, R_SUSHI, -1, -1, -1}, {R_STREET, -1, -1, -1, -1, -1},
    {R_HOSPITAL, R_PETSHOP, R_LIBRARY, R_GYM, R_HOTEL, R_CHAPEL},
    {R_CORNER, R_THRIFT, R_TENNIS, R_HAUNTED, R_FRAT, -1},
    {R_COVESUITE, R_ISLAND, R_ISLANDTOWN, R_COVESIGHTS, -1, -1},
    {R_PEAKSUITE, R_SLOPES, R_LODGE, R_PEAKSIGHTS, -1, -1},
    {R_BELLESUITE, R_BOULEVARD, R_TOWER, R_BELLESIGHTS, -1, -1},
    {R_JAIL, -1, -1, -1, -1, -1},
    {R_BOARDING, -1, -1, -1, -1, -1},
    {R_GLOWBOWL, R_KARAOKE, R_PAINTSIP, R_DANCESTUDIO, R_NIGHTMKT, R_SERENITY},
    {R_DOWNTOWN, R_DTBOUTIQUE, R_DTBISTRO, R_VELVET, -1, -1},
    {R_FUNPARK, R_FUNRIDES, -1, -1, -1, -1},
    {R_SUBWAY, R_PIXELARCADE, R_LAUNDRO, -1, -1, -1},
    {R_AQUARIUM, R_MUSEUM, R_GARDENS, R_ROOFTOP, -1, -1},
    {R_SCHOOL, R_HALLWAY, R_CLASSROOM, R_CAFETERIA, R_SCHOOLGYM, -1},
    {R_DAYCARE, R_CHURCH, R_COMMUNITY, R_POOL, R_BLOCKPARTY, -1},
    {R_COLLEGE, R_DORM, -1, -1, -1, -1},
    {R_SALON, R_COURTHOUSE, -1, -1, -1, -1},
    {R_ROLLER, R_NIGHTMARKET, -1, -1, -1, -1},
    {R_AVSQUARE, R_AVFARM, R_AVBARN, -1, -1, -1},
};
static const char* DIST_NAMES[] = {"Glam Mall", "Cafe & Bakery Corner", "City Park", "Restaurant Row", "Maple Lane", "Uptown", "Old Town", "Coral Cove", "Frostpeak", "Belle Ville", "County Jail", "Rosewood Academy", "Neon Strip", "Downtown", "Sparkle Land", "Metro Central", "Uptown Line", "Maple Valley School", "Community", "Maple U", "Main Street", "Boardwalk", "Apple Valley"};
static const int ND = 23;
static bool findDist(int room, int& d, int& k) {
    for (d = 0; d < ND; d++) for (k = 0; k < 6; k++) if (DIST[d][k] == room) return true;
    return false;
}
static int distLen(int d) { int n = 0; for (int k = 0; k < 6; k++) if (DIST[d][k] >= 0) n++; return n; }
int districtNext(int room, int dir) {
    int d, k;
    if (!findDist(room, d, k)) return -1;
    int n = distLen(d);
    if (n < 2) return -1;
    return DIST[d][((k + dir) % n + n) % n];
}
const char* districtName(int room) { int d, k; return findDist(room, d, k) ? DIST_NAMES[d] : nullptr; }

// ---------------------------------------------------------------- spots
static const Spot SPOTS[] = {
    // Glam Mall: Fashion Wing
    {R_MALL, 56, 200, SP_RACK, "Clothing Rack"}, {R_MALL, 150, 196, SP_SHOES, "Shoe Wall"}, {R_MALL, 250, 198, SP_ACCESS, "Accessory Stand"}, {R_MALL, 346, 200, SP_MIRROR, "Fitting Room"},
    // Beauty & Food Court
    {R_MALL_BEAUTY, 52, 198, SP_MAKEUP, "Makeup Counter"}, {R_MALL_BEAUTY, 138, 198, SP_NAILS, "Nail Bar"}, {R_MALL_BEAUTY, 246, 198, SP_FOODSTALL, "Food Stalls"}, {R_MALL_BEAUTY, 344, 198, SP_PHOTOBOOTH, "Photo Booth"},
    // Fun Zone Arcade
    {R_ARCADE, 56, 200, SP_ARCADE, "Arcade Cabinets"}, {R_ARCADE, 150, 200, SP_CLAW, "Claw Machine"}, {R_ARCADE, 300, 200, SP_FURNITURE, "Furniture Showroom"},
    // Cafe Latte & Sugar Rush Bakery
    {R_CAFE, 70, 198, SP_ESPRESSO, "Espresso Bar"}, {R_CAFE, 320, 198, SP_STAGE, "Karaoke Stage"},
    {R_BAKERY, 70, 200, SP_DISPLAY, "Cupcake Case"}, {R_BAKERY, 200, 204, SP_DECORATE, "Decorating Table"}, {R_BAKERY, 330, 198, SP_OVEN, "Baker's Oven"},
    // City Park & Playground
    {R_PARK, 70, 214, SP_PICNIC, "Picnic Blanket"}, {R_PARK, 326, 200, SP_POND, "Duck Pond"},
    {R_PLAYGROUND, 64, 204, SP_SWINGS, "Swing Set"}, {R_PLAYGROUND, 160, 214, SP_SANDBOX, "Sandbox"}, {R_PLAYGROUND, 252, 196, SP_HOOP, "Basketball Hoop"}, {R_PLAYGROUND, 344, 204, SP_ICECREAM, "Ice Cream Cart"},
    // Restaurant Row
    {R_RESTAURANT, 200, 210, SP_DINNERTABLE, "Candlelit Table"},
    {R_DINER, 80, 206, SP_BOOTH, "Diner Booth"}, {R_DINER, 210, 198, SP_SHAKE, "Milkshake Counter"}, {R_DINER, 336, 196, SP_JUKEBOX, "Jukebox"},
    {R_SUSHI, 120, 202, SP_CONVEYOR, "Sushi Conveyor"}, {R_SUSHI, 316, 200, SP_SUSHICLASS, "Chef's Counter"},
    // Maple Lane
    {R_STREET, 44, 186, SP_HOMEDOOR, "Your Front Door"}, {R_STREET, 126, 186, SP_NDOOR0, "Lilac House"}, {R_STREET, 208, 186, SP_NDOOR1, "Mint House"}, {R_STREET, 290, 186, SP_NDOOR2, "Peach House"},
    {R_STREET, 360, 214, SP_MAILBOX, "Mailbox"}, {R_STREET, 84, 226, SP_REALTY, "For Sale Sign"}, {R_STREET, 250, 228, SP_LEMONADE, "Lemonade Stand"},
    // a neighbour's living room
    // Uptown
    {R_HOSPITAL, 70, 200, SP_RECEPTION, "Reception"}, {R_HOSPITAL, 200, 204, SP_EXAMBED, "Exam Room"}, {R_HOSPITAL, 330, 198, SP_NURSERYWIN, "Nursery Window"},
    {R_PETSHOP, 80, 206, SP_PETPEN, "Adoption Pens"}, {R_PETSHOP, 210, 200, SP_PETSUPPLY, "Treat Shelf"}, {R_PETSHOP, 330, 204, SP_GROOM, "Grooming & Vet"},
    {R_LIBRARY, 64, 200, SP_BOOKSHELF, "Bookshelves"}, {R_LIBRARY, 196, 208, SP_STUDYTABLE, "Study Table"}, {R_LIBRARY, 334, 212, SP_STORYRUG, "Story Corner"},
    {R_GYM, 60, 204, SP_TREADMILL, "Treadmills"}, {R_GYM, 160, 206, SP_WEIGHTS, "Weight Rack"}, {R_GYM, 250, 214, SP_YOGAMAT, "Yoga Mats"}, {R_GYM, 344, 214, SP_LAPPOOL, "Lap Pool"},
    {R_HOTEL, 70, 202, SP_FRONTDESK, "Front Desk"}, {R_HOTEL, 210, 214, SP_HOTELPOOL, "Rooftop Pool"}, {R_HOTEL, 336, 206, SP_LOUNGE, "Piano Lounge"},
    {R_CHAPEL, 200, 196, SP_ALTAR, "Flower Altar"}, {R_CHAPEL, 66, 208, SP_BANQUET, "Banquet Table"}, {R_CHAPEL, 334, 214, SP_DANCEFLOOR, "Dance Floor"},
    // Sugar Market (grocery store at the end of the Glam Mall strip)
    {R_MARKET, 46, 204, SP_PRODUCE, "Fresh Produce"}, {R_MARKET, 128, 200, SP_AISLES, "Grocery Aisles"}, {R_MARKET, 210, 200, SP_BEAUTYAISLE, "Beauty Aisle"},
    {R_MARKET, 292, 200, SP_FITRACK, "Cute Fits Rack"}, {R_MARKET, 366, 208, SP_CHECKOUT, "Checkout"},
    {R_CORNER, 80, 204, SP_SNACKS, "Snack Aisle"}, {R_CORNER, 300, 204, SP_REGISTER, "Register"},
    {R_THRIFT, 120, 204, SP_RACKS, "Thrift Racks"}, {R_THRIFT, 320, 208, SP_DONATE, "Donation Bin"},
    {R_TENNIS, 170, 206, SP_COURT, "Tennis Court"}, {R_TENNIS, 350, 200, SP_BLEACHERS, "Bleachers"},
    {R_HAUNTED, 110, 206, SP_CREAKY, "Creaky Staircase"}, {R_HAUNTED, 300, 206, SP_SEANCE, "Seance Table"},
    {R_FRAT, 130, 206, SP_PARTYFLOOR, "Party Floor"}, {R_FRAT, 320, 206, SP_FRATCOUCH, "Lawn Couch"},
    {R_ISLAND, 120, 210, SP_LOUNGER, "Sun Lounger"}, {R_ISLAND, 310, 196, SP_LAGOON, "Lagoon"},
    {R_ISLANDTOWN, 120, 206, SP_TIKIBAR, "Tiki Bar"}, {R_ISLANDTOWN, 310, 206, SP_SOUVENIR, "Souvenir Stand"},
    {R_SLOPES, 110, 206, SP_SKILIFT, "Ski Lift"}, {R_SLOPES, 300, 210, SP_SNOWFIELD, "Snow Field"},
    {R_LODGE, 120, 206, SP_FIREPLACE, "Fireplace"}, {R_LODGE, 310, 210, SP_HOTTUB, "Hot Tub"},
    {R_BOULEVARD, 110, 206, SP_CREPECART, "Crepe Cart"}, {R_BOULEVARD, 300, 206, SP_COUTURE, "Couture Boutique"},
    {R_TOWER, 130, 206, SP_LOOKOUT, "Tower Lookout"}, {R_TOWER, 310, 206, SP_ARTIST, "Street Artist"},
    {R_COVESUITE, 70, 208, SP_HOTELBED, "Suite Bed"}, {R_COVESUITE, 168, 206, SP_ROOMSVC, "Room Service"}, {R_COVESUITE, 250, 206, SP_MINIBAR, "Minibar"}, {R_COVESUITE, 340, 208, SP_BALCONY, "Balcony Window"},
    {R_COVESIGHTS, 80, 208, SP_LANDMARK, "Waterfall"}, {R_COVESIGHTS, 175, 206, SP_TOURDESK, "Tour Guide"}, {R_COVESIGHTS, 262, 206, SP_STREETFOOD, "Fruit Stand"}, {R_COVESIGHTS, 345, 206, SP_GIFTSHOP, "Shell Shop"},
    {R_PEAKSUITE, 70, 208, SP_HOTELBED, "Suite Bed"}, {R_PEAKSUITE, 168, 206, SP_ROOMSVC, "Room Service"}, {R_PEAKSUITE, 250, 206, SP_MINIBAR, "Minibar"}, {R_PEAKSUITE, 340, 208, SP_BALCONY, "Balcony Window"},
    {R_PEAKSIGHTS, 80, 208, SP_LANDMARK, "Ice Sculptures"}, {R_PEAKSIGHTS, 175, 206, SP_TOURDESK, "Tour Guide"}, {R_PEAKSIGHTS, 262, 206, SP_STREETFOOD, "Pretzel Hut"}, {R_PEAKSIGHTS, 345, 206, SP_GIFTSHOP, "Gift Chalet"},
    {R_BELLESUITE, 70, 208, SP_HOTELBED, "Suite Bed"}, {R_BELLESUITE, 168, 206, SP_ROOMSVC, "Room Service"}, {R_BELLESUITE, 250, 206, SP_MINIBAR, "Minibar"}, {R_BELLESUITE, 340, 208, SP_BALCONY, "Balcony Window"},
    {R_BELLESIGHTS, 80, 208, SP_LANDMARK, "Grand Gallery"}, {R_BELLESIGHTS, 175, 206, SP_TOURDESK, "Tour Guide"}, {R_BELLESIGHTS, 262, 206, SP_STREETFOOD, "Crepe Stand"}, {R_BELLESIGHTS, 345, 206, SP_GIFTSHOP, "Museum Shop"},
    {R_HOTELLOBBY, 90, 206, SP_HFRONTDESK, "Front Desk"}, {R_HOTELLOBBY, 210, 210, SP_HLOUNGE, "Lobby Lounge"}, {R_HOTELLOBBY, 330, 206, SP_BELLHOP, "Bell Desk"},
    {R_HOTELBATH, 80, 206, SP_RAINSHOWER, "Rain Shower"}, {R_HOTELBATH, 210, 214, SP_SOAKTUB2, "Soaking Tub"}, {R_HOTELBATH, 330, 206, SP_HVANITY, "Double Vanity"},
    {R_HOTELPOOL, 120, 214, SP_INFINITY, "Infinity Pool"}, {R_HOTELPOOL, 260, 208, SP_MASSAGE, "Spa Cabana"}, {R_HOTELPOOL, 350, 206, SP_POOLBAR, "Pool Bar"},
    {R_GLOWBOWL, 120, 210, SP_LANES, "Glow Lanes"}, {R_GLOWBOWL, 320, 206, SP_BOWLSNACK, "Cosmic Snack Bar"},
    {R_KARAOKE, 130, 206, SP_KMIC, "Main Stage Mic"}, {R_KARAOKE, 320, 206, SP_KBOOTH, "Private Booth"},
    {R_PAINTSIP, 120, 208, SP_EASEL, "Easels"}, {R_PAINTSIP, 320, 206, SP_SIPBAR, "Sip Bar"},
    {R_DANCESTUDIO, 150, 212, SP_DFLOOR, "Dance Floor"}, {R_DANCESTUDIO, 340, 206, SP_BARRE, "Ballet Barre"},
    {R_NIGHTMKT, 110, 206, SP_VENDOR, "Vex's Stall"}, {R_NIGHTMKT, 300, 206, SP_NFOOD, "Skewer Cart"},
    {R_SERENITY, 120, 206, SP_REHABDESK, "Intake Desk"}, {R_SERENITY, 300, 210, SP_CIRCLE, "Support Circle"},
    {R_DOWNTOWN, 50, 206, SP_TAXI, "Taxi Stand"}, {R_DOWNTOWN, 140, 206, SP_HOTDOG, "Hot Dog Stand"}, {R_DOWNTOWN, 250, 206, SP_ICECART, "Ice Cream Counter"}, {R_DOWNTOWN, 350, 206, SP_NEWSSTAND, "Newsstand"},
    {R_DTBOUTIQUE, 50, 206, SP_DRACK0, "Fashion Rack"}, {R_DTBOUTIQUE, 125, 206, SP_DRACK1, "Formal Rack"}, {R_DTBOUTIQUE, 200, 206, SP_DRACK2, "Swim & Pet Rack"}, {R_DTBOUTIQUE, 280, 206, SP_DRESSBOOTH, "Dressing Booth"}, {R_DTBOUTIQUE, 360, 206, SP_DREGISTER, "Register"},
    {R_DTBISTRO, 50, 206, SP_PODIUM, "Host Podium"}, {R_DTBISTRO, 200, 212, SP_DTABLE, "Your Booth"}, {R_DTBISTRO, 345, 206, SP_STOVE, "Kitchen"},
    {R_VELVET, 50, 206, SP_VVIP, "VIP Booth"}, {R_VELVET, 200, 208, SP_VSTAGE, "Main Stage"}, {R_VELVET, 290, 206, SP_VMGR, "Manager"}, {R_VELVET, 350, 206, SP_VBAR, "Bar"},
    {R_FUNPARK, 55, 206, SP_TICKETS, "Ticket Booth"}, {R_FUNPARK, 200, 212, SP_CAROUSEL, "Carousel"}, {R_FUNPARK, 335, 206, SP_CANDY, "Candy Stand"},
    {R_FUNRIDES, 70, 210, SP_COASTER, "Sparkle Coaster"}, {R_FUNRIDES, 220, 210, SP_FERRIS, "Ferris Wheel"}, {R_FUNRIDES, 300, 206, SP_GAMEBOOTH, "Ring Toss"}, {R_FUNRIDES, 360, 206, SP_BUMPER, "Bumper Cars"},
    {R_SUBWAY, 90, 206, SP_TURNSTILE, "Turnstiles"}, {R_SUBWAY, 280, 206, SP_BUSKER, "Busker"},
    {R_PIXELARCADE, 70, 206, SP_CABINETS, "Arcade Cabinets"}, {R_PIXELARCADE, 210, 210, SP_DDR, "Dance Machine"}, {R_PIXELARCADE, 340, 206, SP_PRIZES, "Prize Counter"},
    {R_LAUNDRO, 160, 206, SP_WASHER, "Washers"},
    {R_AQUARIUM, 120, 206, SP_TANK, "Jellyfish Tank"}, {R_AQUARIUM, 300, 210, SP_TOUCHPOOL, "Touch Pool"},
    {R_MUSEUM, 120, 206, SP_EXHIBIT, "Exhibit Hall"}, {R_MUSEUM, 300, 206, SP_SCULPT, "Sculpture"},
    {R_GARDENS, 120, 210, SP_ROSES, "Rose Garden"}, {R_GARDENS, 300, 206, SP_GREENHOUSE, "Greenhouse"},
    {R_ROOFTOP, 120, 206, SP_ROOFBAR, "Rooftop Bar"}, {R_ROOFTOP, 300, 206, SP_SKYVIEW, "Skyline View"},
    {R_FOODCOURT, 70, 206, SP_FCFOOD, "Food Stands"}, {R_FOODCOURT, 180, 212, SP_FCTABLE, "Tables"}, {R_FOODCOURT, 280, 206, SP_T2PHOTOBOOTH, "Photo Booth"}, {R_FOODCOURT, 360, 206, SP_PIERCE, "Piercing Kiosk"},
    {R_HALLWAY, 100, 206, SP_LOCKER, "My Locker"}, {R_HALLWAY, 300, 206, SP_BULLETIN, "Bulletin Board"},
    {R_CLASSROOM, 110, 212, SP_DESK, "My Desk"}, {R_CLASSROOM, 290, 206, SP_CHALK, "Chalkboard"},
    {R_CAFETERIA, 90, 206, SP_LUNCHLINE, "Lunch Line"}, {R_CAFETERIA, 280, 212, SP_LUNCHTABLE, "Lunch Table"},
    {R_SCHOOLGYM, 80, 206, SP_T2BLEACHERS, "Bleachers"}, {R_SCHOOLGYM, 280, 212, SP_T2COURT, "Gym Floor"},
    {R_DAYCARE, 100, 206, SP_CRIB, "Drop-Off Desk"}, {R_DAYCARE, 290, 212, SP_PLAYMAT, "Play Mat"},
    {R_CHURCH, 140, 210, SP_PEW, "Pews"}, {R_CHURCH, 320, 206, SP_BAKESALE, "Bake Sale"},
    {R_COMMUNITY, 100, 206, SP_CCDESK, "Front Desk"}, {R_COMMUNITY, 300, 212, SP_T2STORYRUG, "Story Rug"},
    {R_POOL, 150, 212, SP_POOLWATER, "Pool"}, {R_POOL, 340, 206, SP_SNACKBAR, "Snack Bar"},
    {R_BLOCKPARTY, 70, 206, SP_GRILL, "Grill"}, {R_BLOCKPARTY, 200, 212, SP_BOUNCE, "Bounce House"}, {R_BLOCKPARTY, 330, 206, SP_DJ, "DJ Table"},
    {R_DORM, 80, 206, SP_DORMDESK, "Desk"}, {R_DORM, 200, 210, SP_DORMBUNK, "Bunk"}, {R_DORM, 330, 206, SP_GREEK, "Greek Row Flyer"},
    {R_SALON, 110, 210, SP_SALONCHAIR, "Stylist Chair"}, {R_SALON, 300, 206, SP_NAILBAR, "Nail Bar"},
    {R_ROLLER, 200, 212, SP_RINKFLOOR, "Rink Floor"}, {R_ROLLER, 350, 206, SP_SKATERENT, "Skate Rental"},
    {R_NIGHTMARKET, 120, 206, SP_MKSTALL, "Food Stalls"}, {R_NIGHTMARKET, 300, 206, SP_MKGAMES, "Boardwalk Games"},
    {R_COURTHOUSE, 110, 206, SP_CLERK, "Clerk's Window"}, {R_COURTHOUSE, 300, 210, SP_JUDGE, "Courtroom"},
    {R_AVSQUARE, 64, 204, SP_AVSTORE, "General Store"}, {R_AVSQUARE, 160, 210, SP_AVERRAND, "Errand Board"}, {R_AVSQUARE, 250, 212, SP_AVFAIR, "Fair Grounds"}, {R_AVSQUARE, 340, 204, SP_AVPUB, "Cider Pub"},
    {R_AVFARM, 60, 208, SP_AVGARDEN, "Garden Plots"}, {R_AVFARM, 160, 214, SP_AVMEADOW, "Wild Meadow"}, {R_AVFARM, 250, 208, SP_AVCOOP, "Chicken Coop"}, {R_AVFARM, 340, 204, SP_AVNECTAR, "Farm Kitchen"},
    {R_AVBARN, 60, 210, SP_AVSTALL, "Cow & Llama Stalls"}, {R_AVBARN, 160, 214, SP_AVPEN, "Mini Goat & Sheep Pen"}, {R_AVBARN, 250, 212, SP_AVPIGS, "Pigs & Ducks"}, {R_AVBARN, 340, 206, SP_AVLOOM, "Cross-Stitch Nook"},
    {R_JAIL, 60, 206, SP_BUNK, "Cell Bunk"}, {R_JAIL, 150, 204, SP_PAYPHONE, "Payphone"}, {R_JAIL, 255, 208, SP_JAILYARD, "Rec Yard"}, {R_JAIL, 350, 206, SP_GUARD, "Guard Desk"},
    {R_BOARDING, 60, 206, SP_DORMBED, "Dorm Bunk"}, {R_BOARDING, 155, 206, SP_STUDYDESK, "Study Desk"}, {R_BOARDING, 255, 208, SP_COMMONS, "Common Room"}, {R_BOARDING, 352, 206, SP_GATE, "Front Gate"},
    {R_NEIGHBOR, 90, 206, SP_NCOUCH, "Their Couch"}, {R_NEIGHBOR, 300, 196, SP_NFRIDGE, "Their Fridge"}, {R_NEIGHBOR, 370, 200, SP_NEXIT, "Front Door"},
};
static const int NUM_SPOTS = sizeof(SPOTS) / sizeof(SPOTS[0]);
const Spot* spotsIn(int room, int& n) {
    int first = -1; n = 0;
    for (int i = 0; i < NUM_SPOTS; i++) if (SPOTS[i].room == room) { if (first < 0) first = i; n++; }
    return first < 0 ? nullptr : &SPOTS[first];
}

// ---------------------------------------------------------------- neighbours
static int g_host = -1;
int neighborFor(int door) {
    std::vector<int> v;
    for (size_t i = 0; i < G.sims.size(); i++) if (!G.sims[i].household && (int)i != G.cop && G.sims[i].isAdult()) v.push_back((int)i);
    if (v.empty()) return -1;
    return v[(door * 7 + 3) % v.size()];
}
int neighborHost() { return g_host; }
void districtArrive(int room) {
    farmArrive(room);
    if (room != R_NEIGHBOR) return;
    RoomState& rs = curRoom();
    std::vector<Actor> keep;
    for (const Actor& a : rs.actors) if (a.kind == 0 && (G.sims[a.id].household || a.id == G.mc)) keep.push_back(a);
    rs.actors = keep;
    if (g_host >= 0 && g_host < (int)G.sims.size()) {
        Actor a; a.kind = 0; a.id = (uint16_t)g_host; a.x = 250; a.tx = 250; a.y = 214;
        static const char* HI[] = {"Come in, come in! Want some tea?", "Oh hi! Excuse the mess!", "Neighbor! Make yourself at home."};
        a.say = HI[irand(3)]; a.sayT = 3;
        rs.actors.push_back(a);
        rel(G.mc, g_host).met = true;
    }
}

// ---------------------------------------------------------------- actions
static void need(int n, float d) { Sim& me = MC(); me.needs[n] = clampv(me.needs[n] + d, 0.f, 100.f); }
static bool pay(int amt) { if (G.money < amt) { toast("Not enough money!"); sfxNo(); return false; } G.money -= amt; return true; }
static void dateBonus(float d) { if (G.date.active) G.date.score += d; }
static uint16_t mailDay = 0xFFFF;

void districtAction(int act) {
    if (act >= DA_HD0 && act <= DA_HDLAST) { hotdateAction(act - DA_HD0); return; }
    if (act >= DA_BOWLSNACK && nightAction(act)) return;
    if (act >= DA_JAILSLEEP && lockupAction(act)) return;
    Sim& me = MC();
    Activity a;
    char b[160];
    switch (act) {
    case DA_SHOES: {
        if (!pay(45)) return;
        uint8_t ns; do ns = (uint8_t)irand(NUM_SHOES - 1); while (ns == me.look.shoeStyle);
        static const RGB SC[] = {{255, 46, 140}, {255, 255, 255}, {30, 26, 30}, {224, 184, 104}, {150, 96, 230}, {200, 40, 60}, {84, 132, 204}};
        me.look.shoeStyle = ns; me.look.shoes = SC[irand(7)];
        snprintf(b, sizeof b, "New %s! Your feet have never looked better.", SHOE_NAMES[ns]); toast(b);
        addMoodlet(me, "New Shoes!", EM_CONFIDENT, 2, 300); goalNotify(GE_BUYOUTFIT, 0); sfxYay();
        return;
    }
    case DA_ACCESS: {
        if (!pay(35)) return;
        static const uint16_t A[] = {ACC_BAG, ACC_NECKLACE, ACC_HOOPS, ACC_SHADES, ACC_BANGLES, ACC_BAG | ACC_SHADES, ACC_NECKLACE | ACC_HOOPS, ACC_PEARLS};
        static const char* N[] = {"a cute handbag", "gold chains", "big hoops", "designer shades", "stacked bangles", "a bag & shades combo", "chains & hoops", "pearls"};
        int k = irand(8); me.look.acc = A[k];
        static const RGB AC[] = {{255, 90, 160}, {224, 184, 104}, {40, 30, 34}, {120, 200, 230}};
        me.look.accCol = AC[irand(4)];
        toast(std::string("You bought ") + N[k] + "! Accessorize!");
        addMoodlet(me, "Accessorized", EM_CONFIDENT, 1, 240); sfxYay();
        return;
    }
    case DA_LIPSAMPLE: need(N_FUN, 8); say(G.mc, "Ooh, this shade is SO me."); addMoodlet(me, "Free Sample", EM_HAPPY, 1, 90); return;
    case DA_MAKEOVER: {
        if (!pay(20)) return;
        me.look.f.shadow = (uint8_t)irand(7); me.look.f.liner = (uint8_t)irand(3);
        static const RGB LIPS[] = {{196, 40, 96}, {200, 30, 50}, {240, 120, 150}, {190, 110, 100}, {170, 96, 120}, {150, 40, 120}};
        me.look.lips = LIPS[irand(6)];
        toast("Makeup makeover done! Check the mirror.");
        addMoodlet(me, "Fresh Face", EM_CONFIDENT, 2, 300); goalNotify(GE_MAKEUP); skillGain(me, SK_STYLE, 2); sfxYay();
        return;
    }
    case DA_BUBBLETEA: if (!pay(6)) return; need(N_HUNGER, 15); need(N_FUN, 10); say(G.mc, "Extra boba!"); dateBonus(8); return;
    case DA_CLAW:
        if (!pay(3)) return;
        if (irand(100) < 32) { toast("The claw grabbed something!"); collectSearch(2); addMoodlet(me, "Claw Champ", EM_PLAYFUL, 1, 180); }
        else { static const char* M[] = {"SO close!!", "It slipped! Rigged, honestly.", "One more try..."}; say(G.mc, M[irand(3)]); need(N_FUN, 4); }
        dateBonus(5);
        return;
    case DA_CHALLENGE: {
        int o = -1;
        for (const Actor& x : curRoom().actors) if (x.kind == 0 && x.id != G.mc && !G.sims[x.id].isLittle()) { o = x.id; break; }
        if (o < 0) { toast("Nobody here to challenge!"); return; }
        bool win = frand() < 0.5f + skillLevel(me, SK_LOGIC) * 0.04f;
        changeRel(G.mc, o, win ? 4 : 7, canRomance(G.mc, o) ? 3 : 0);
        say(G.mc, win ? "HIGH SCORE! Bow down." : "Okay okay, rematch!");
        say(o, win ? "No fair, you've been practicing!" : "Get rekt, haha!");
        need(N_FUN, 20); need(N_SOCIAL, 12); dateBonus(14);
        return;
    }
    case DA_FURNITURE: {
        if (!pay(60)) return;
        static const int SH[] = {SH_PLANT, SH_PAINTING, SH_LAMP, SH_FLOWERS, SH_BEANBAG};
        int sh = SH[irand(5)];
        Placed p; p.item = (uint16_t)itemId(sh, irand(NUM_COLORS)); p.x = (int16_t)(60 + irand(280)); p.y = (int16_t)(SHAPES[sh].wall ? 90 : 200 + irand(20));
        G.rooms[G.house][R_LIVING].items.push_back(p);
        toast(std::string("Delivered to your living room: ") + itemName(p.item) + "!");
        sfxYay();
        return;
    }
    case DA_CUPCAKE: if (!pay(6)) return; need(N_HUNGER, 20); addMoodlet(me, "Sugar Rush", EM_PLAYFUL, 1, 120); say(G.mc, "Sprinkles = happiness."); dateBonus(8); return;
    case DA_CAKE: if (!pay(30)) return; need(N_HUNGER, 30); need(N_SOCIAL, 10); addMoodlet(me, "Party Cake!", EM_HAPPY, 2, 240); toast("A whole cake! Celebrate something!"); return;
    case DA_DECORATE: a = makeAct("Decorating Cupcakes", 45, 8); a.rate[N_FUN] = 30; a.skill = SK_CREATIVITY; a.skillRate = 5; startActivity(a); say(G.mc, "A pink frosting masterpiece!"); return;
    case DA_BAKECLASS: if (!pay(15)) return; a = makeAct("Baking Class", 90, 10); a.rate[N_FUN] = 15; a.rate[N_SOCIAL] = 10; a.skill = SK_COOKING; a.skillRate = 8; startActivity(a); goalNotify(GE_COOK); return;
    case DA_SWING: a = makeAct("Swinging", 30, 8); a.pose = PO_DANCE; a.rate[N_FUN] = 40; startActivity(a); say(G.mc, me.isAdult() ? "Being a grown-up is overrated!" : "Higher! HIGHER!"); return;
    case DA_SANDBOX: a = makeAct("Playing in the Sand", 40, 8); a.rate[N_FUN] = 35; a.skill = SK_CREATIVITY; a.skillRate = 3; startActivity(a); return;
    case DA_HOOPS: a = makeAct("Shooting Hoops", 45, 8); a.pose = PO_DANCE; a.rate[N_FUN] = 25; a.rate[N_ENERGY] = -10; a.skill = SK_FITNESS; a.skillRate = 6; startActivity(a); say(G.mc, irand(2) ? "Nothing but net!" : "...air ball."); return;
    case DA_ICECREAM: if (!pay(4)) return; need(N_HUNGER, 12); need(N_FUN, 12); addMoodlet(me, "Brain Freeze", EM_PLAYFUL, 1, 90); say(G.mc, "Strawberry with sprinkles!"); dateBonus(10); return;
    case DA_PEOPLEWATCH: a = makeAct("People-Watching", 45, 8); a.rate[N_FUN] = 15; a.rate[N_SOCIAL] = 8; a.rate[N_ENERGY] = 8; startActivity(a); return;
    case DA_BURGER: { int c = G.date.active ? 26 : 14; if (!pay(c)) return; a = makeAct("Burger & Shake", 45, 8); a.rate[N_HUNGER] = 90; a.rate[N_FUN] = 15; startActivity(a); dateBonus(18); say(G.mc, "Extra fries, please!"); return; }
    case DA_SHAKE: if (!pay(8)) return; need(N_HUNGER, 15); dateBonus(20); if (G.date.active && G.date.partner >= 0) { changeRel(G.mc, G.date.partner, 3, 6); say(G.mc, "Two straws, one shake?"); } else say(G.mc, "Chocolate. Obviously."); return;
    case DA_JUKEBOX: a = makeAct("Dancing to the Jukebox", 30, 8); a.pose = PO_DANCE; a.kind = AK_DANCE; a.rate[N_FUN] = 35; startActivity(a); musicPlay(1); goalNotify(GE_DANCE); dateBonus(12); return;
    case DA_SUSHI: if (!pay(18)) return; a = makeAct("Grabbing Sushi Plates", 40, 8); a.rate[N_HUNGER] = 85; startActivity(a); dateBonus(16); say(G.mc, "Ooh, the salmon one!"); return;
    case DA_SUSHICLASS: if (!pay(25)) return; a = makeAct("Sushi Class", 90, 10); a.rate[N_FUN] = 15; a.skill = SK_COOKING; a.skillRate = 9; startActivity(a); return;
    case DA_MAIL: {
        if (mailDay == G.day) { toast("Nothing new in the mailbox today."); return; }
        mailDay = G.day;
        int r = irand(6);
        if (r == 0) { int c = 20 + irand(40); G.money += c; snprintf(b, sizeof b, "A coupon refund check: +$%d!", c); }
        else if (r == 1) { uint32_t f = 40 + G.followers / 40; G.followers += f; snprintf(b, sizeof b, "Fan mail! Someone drew you. +%u followers.", (unsigned)f); }
        else if (r == 2) { G.money = std::max(0, (int)G.money - 30); snprintf(b, sizeof b, "Ugh, a late fee: -$30."); }
        else if (r == 3) { addMoodlet(me, "Secret Admirer", EM_FLIRTY, 2, 360); snprintf(b, sizeof b, "A pink envelope... \"You light up Maple Lane. -A secret admirer\""); }
        else if (r == 4) { snprintf(b, sizeof b, "Junk mail. Pizza coupons. Keeping them."); need(N_FUN, 3); }
        else { G.satisfaction += 50; snprintf(b, sizeof b, "A thank-you card from a neighbor! (+50 points)"); }
        dlgSay(-1, b);
        return;
    }
    case DA_LEMONADE: if (!pay(2)) return; need(N_HUNGER, 6); need(N_FUN, 6); say(G.mc, "Best lemonade on the block!"); G.rep = (int16_t)std::min(100, G.rep + 1); return;
    case DA_NHANG: {
        int h = g_host;
        a = makeAct("Hanging Out", 60, 8); a.rate[N_FUN] = 20; a.rate[N_SOCIAL] = 45; startActivity(a);
        if (h >= 0) { changeRel(G.mc, h, 8, 0); say(h, irand(2) ? "So, tell me EVERYTHING." : "Have you met the people at the Peach House?"); }
        return;
    }
    case DA_NFRIDGE: {
        need(N_HUNGER, 30);
        if (g_host >= 0 && irand(2)) { say(g_host, "Um... are you eating my leftovers?!"); changeRel(G.mc, g_host, -8, 0); addMoodlet(me, "Caught Snacking", EM_TENSE, 1, 120); }
        else say(G.mc, "*sneaks a cookie*");
        return;
    }
    // ---------------- Uptown
    case DA_CHECKUP: if (!pay(30)) return; a = makeAct("Getting a Checkup", 45, 8); a.rate[N_ENERGY] = 10; startActivity(a); addMoodlet(me, "Clean Bill of Health", EM_HAPPY, 2, 720); toast("Doctor says: you're doing great! Drink water."); return;
    case DA_FLUSHOT: if (!pay(10)) return; say(G.mc, "Ow! ...okay, that wasn't so bad."); addMoodlet(me, "Brave Little Toaster", EM_CONFIDENT, 1, 240); return;
    case DA_VOLUNTEER: a = makeAct("Volunteering", 120, 12); a.rate[N_SOCIAL] = 25; a.rate[N_FUN] = 5; a.rate[N_ENERGY] = -12; a.skill = SK_CHARISMA; a.skillRate = 3; startActivity(a); G.rep = (int16_t)std::min(100, G.rep + 4); G.satisfaction += 40; toast("You volunteered at the hospital. (+Reputation)"); return;
    case DA_BABIES: need(N_FUN, 10); addMoodlet(me, "Baby Fever", EM_FLIRTY, 1, 240); say(G.mc, irand(2) ? "Look at the tiny hats!!" : "That one's yawning, I can't."); return;
    case DA_PETTREATS: if (!pay(12)) return; G.satisfaction += 15; toast(G.pets.empty() ? "Treats bought... maybe adopt someone to eat them?" : "Your pets are going to LOVE these treats."); return;
    case DA_GROOM: if (!pay(25)) return; if (G.pets.empty()) { toast("You don't have a pet yet! Visit the Adoption Pens."); G.money += 25; return; } toast("Your pet got a bubble bath and a bow. So fluffy!"); addMoodlet(me, "Proud Pet Parent", EM_HAPPY, 2, 360); return;
    case DA_VETCHECK: if (!pay(40)) return; if (G.pets.empty()) { toast("No pets to check! Adopt one first."); G.money += 40; return; } toast("The vet says your pet is perfectly healthy!"); addMoodlet(me, "Relieved", EM_FINE, 1, 240); return;
    case DA_READ: a = makeAct("Reading", 60, 8); a.rate[N_FUN] = 15; a.rate[N_ENERGY] = 5; a.skill = SK_LOGIC; a.skillRate = 5; startActivity(a); goalNotify(GE_READ); return;
    case DA_BUYBOOK: if (!pay(15)) return; skillGain(me, SK_CREATIVITY, 2); toast("You bought a bestseller. No spoilers please!"); return;
    case DA_STUDY: a = makeAct("Studying", 90, 10); a.kind = AK_HOMEWORK; a.rate[N_FUN] = -5; a.skill = SK_LOGIC; a.skillRate = 7; startActivity(a); goalNotify(GE_HOMEWORK); return;
    case DA_STORYTIME: a = makeAct("Story Time", 40, 8); a.rate[N_FUN] = 30; a.rate[N_SOCIAL] = 15; a.skill = SK_CREATIVITY; a.skillRate = 3; startActivity(a); say(G.mc, me.isAdult() ? "Once upon a time..." : "And then the dragon said WHAT?"); return;
    case DA_POETRY: a = makeAct("Poetry Slam", 45, 8); a.rate[N_FUN] = 20; a.rate[N_SOCIAL] = 15; a.skill = SK_CHARISMA; a.skillRate = 4; startActivity(a); say(G.mc, "Roses are red... my outfit is too..."); return;
    case DA_TREADMILL: a = makeAct("Running", 45, 8); a.kind = AK_DANCE; a.rate[N_ENERGY] = -20; a.rate[N_HYGIENE] = -20; a.rate[N_FUN] = 8; a.skill = SK_FITNESS; a.skillRate = 8; startActivity(a); return;
    case DA_WEIGHTS: a = makeAct("Lifting Weights", 45, 8); a.rate[N_ENERGY] = -22; a.rate[N_HYGIENE] = -18; a.skill = SK_FITNESS; a.skillRate = 9; startActivity(a); say(G.mc, "Do you even lift? I do now."); return;
    case DA_YOGA: if (!pay(10)) return; a = makeAct("Yoga Class", 60, 8); a.rate[N_ENERGY] = 10; a.rate[N_FUN] = 15; a.rate[N_SOCIAL] = 10; a.skill = SK_FITNESS; a.skillRate = 5; startActivity(a); addMoodlet(me, "Zen", EM_FINE, 2, 300); return;
    case DA_LAPS: a = makeAct("Swimming Laps", 45, 8); a.kind = AK_SWIM; a.rate[N_HYGIENE] = 20; a.rate[N_ENERGY] = -15; a.rate[N_FUN] = 15; a.skill = SK_FITNESS; a.skillRate = 7; startActivity(a); return;
    case DA_HOTELNIGHT: { int c = hotelStaying() ? 0 : std::max(20, (int)G.hotelRate); if (c && !pay(c)) return; a = makeAct("Hotel Stay", 480, 40); a.kind = AK_SLEEP; a.pose = PO_SLEEP; a.rate[N_ENERGY] = 14; a.rate[N_HYGIENE] = 10; a.rate[N_FUN] = 4; startActivity(a);
        int p = partnerOf(G.mc); if (p >= 0) { changeRel(G.mc, p, 6, 12); addMemoryOnce(MEM_LOVE, G.mc, p, "A romantic getaway at the Grand Starlight Hotel."); }
        addMoodlet(me, "Vacation Mode", EM_HAPPY, 3, 1440); toast("Checked in! Fluffy robes and a view of the whole city."); return; }
    case DA_ROOMSERVICE: if (!pay(35)) return; need(N_HUNGER, 70); addMoodlet(me, "Fancy", EM_CONFIDENT, 1, 240); say(G.mc, "Strawberries AND cheesecake? Iconic."); dateBonus(15); return;
    case DA_HOTELPOOL: a = makeAct("Lounging Poolside", 60, 8); a.kind = AK_SWIM; a.rate[N_FUN] = 30; a.rate[N_HYGIENE] = 10; startActivity(a); dateBonus(12); return;
    case DA_LOUNGE: a = makeAct("Piano Lounge", 45, 8); a.rate[N_FUN] = 20; a.rate[N_SOCIAL] = 20; startActivity(a); dateBonus(15); return;
    case DA_WEDDING: {
        int p = -1;
        for (const Rel& r : G.rels) if ((r.a == G.mc || r.b == G.mc) && r.status == RS_ENGAGED) p = r.a == G.mc ? r.b : r.a;
        if (p < 0) { toast("You need to be engaged first! Propose to your sweetheart."); return; }
        if (!pay(500)) return;
        doSocial(SO_MARRY, p);
        G.satisfaction += 300; addMemoryOnce(MEM_LOVE, G.mc, p, "Our dream wedding at the Lovebird Chapel!");
        toast("A dream wedding at the Lovebird Chapel!"); return;
    }
    case DA_VOWRENEW: {
        int p = partnerOf(G.mc);
        if (p < 0 || rel(G.mc, p).status != RS_MARRIED) { toast("Only married couples can renew their vows."); return; }
        if (!pay(100)) return; changeRel(G.mc, p, 10, 20); addMoodlet(me, "Still In Love", EM_FLIRTY, 3, 1440); addMemoryOnce(MEM_LOVE, G.mc, p, "We renewed our vows!"); cutscenePlay(CS_WEDDING, true); return;
    }
    case DA_PARTY: { if (!pay(200)) return; a = makeAct("Throwing a Party", 120, 12); a.kind = AK_DANCE; a.pose = PO_DANCE; a.rate[N_FUN] = 35; a.rate[N_SOCIAL] = 40; startActivity(a);
        for (const Actor& x : curRoom().actors) if (x.kind == 0 && x.id != G.mc) changeRel(G.mc, x.id, 6, 0);
        G.followers += 80 + G.followers / 30; toast("Party at the Event Hall! Everyone's talking about it."); return; }
    case DA_BANQUET: if (!pay(20)) return; need(N_HUNGER, 60); say(G.mc, "Is that a chocolate fountain?!"); return;
    // ---- Sugar Market
    case DA_FRUIT: if (!pay(3)) return; need(N_HUNGER, 12); say(G.mc, "Strawberries for the road!"); addMoodlet(me, "Fresh & Fruity", EM_HAPPY, 1, 90); return;
    case DA_SEEDS: if (!pay(8)) return; skillGain(me, SK_CREATIVITY, 2); toast("Seed packets bought! Plant them in your backyard garden."); goalNotify(GE_COLLECT); return;
    case DA_GROCERIES: if (!pay(60)) return; economySpend(EC_GROCERY, 60); economyGroceriesStocked(14, 1); toast("Fridge stocked: 14 home-cooked meals' worth of groceries!"); addMoodlet(me, "Fully Stocked", EM_CONFIDENT, 1, 240); sfxYay(); return;
    case DA_SNACKHAUL: if (!pay(15)) return; economySpend(EC_GROCERY, 15); economyGroceriesStocked(4, 0); need(N_FUN, 10); toast("Snack haul! Chips, gummies and way too many cookies (+4 meals)."); return;
    case DA_LIPGLOSS: {
        if (!pay(8)) return;
        static const RGB G2[] = {{255, 150, 190}, {240, 110, 150}, {220, 70, 120}, {230, 160, 150}, {196, 40, 96}};
        static const char* N[] = {"Bubblegum Pop", "Strawberry Milk", "Berry Kiss", "Peachy Nude", "Cherry Bomb"};
        int k = irand(5); me.look.lips = G2[k];
        toast(std::string("New lip gloss: ") + N[k] + "! So shiny."); addMoodlet(me, "Glossy", EM_CONFIDENT, 1, 240); goalNotify(GE_MAKEUP); sfxYay(); return;
    }
    case DA_BLUSH: if (!pay(14)) return; me.look.f.blush = 1; toast("Blush on! That just-pinched glow."); addMoodlet(me, "Rosy Cheeks", EM_HAPPY, 1, 240); goalNotify(GE_MAKEUP); return;
    case DA_LASHES: if (!pay(12)) return; me.look.f.liner = 2; toast("Lashes & a sharp wing. Serving looks."); addMoodlet(me, "Lash Queen", EM_CONFIDENT, 1, 240); goalNotify(GE_MAKEUP); return;
    case DA_PALETTE: {
        if (!pay(22)) return;
        int s = 1 + irand(6); me.look.f.shadow = (uint8_t)s;
        static const char* N[] = {"", "Cotton Candy", "Smoky Night", "Golden Hour", "Lilac Dream", "Ocean Babe", "Bronze Goddess"};
        toast(std::string("Eyeshadow palette: ") + N[s] + "!"); addMoodlet(me, "Palette Haul", EM_INSPIRED, 1, 240); skillGain(me, SK_STYLE, 1); goalNotify(GE_MAKEUP); sfxYay(); return;
    }
    case DA_NAILPOLISH: if (!pay(6)) return; need(N_FUN, 10); toast("Cute new polish! Paint them at the Nail Bar."); addMoodlet(me, "Fresh Polish", EM_HAPPY, 1, 120); return;
    case DA_CUTEFIT: case DA_FITHAUL: {
        int cost = act == DA_CUTEFIT ? 30 : 75, n = act == DA_CUTEFIT ? 1 : 3, got = 0, last = -1;
        if (!pay(cost)) return;
        for (int tries = 0; tries < 40 && got < n; tries++) { int k = 32 + irand(FIRST_KID_OUTFIT - 32); if (!ownsOutfit(k)) { giveOutfit(k); got++; last = k; } }
        if (got == 0) { G.money += cost; toast("You already own every cute fit here! (Refunded)"); return; }
        me.outfit = (uint8_t)last; goalNotify(GE_BUYOUTFIT);
        if (n == 1) toast(std::string("Bought & wearing: ") + OUTFITS[last].name + "!");
        else { char b[96]; snprintf(b, sizeof b, "Fit haul! %d new looks in your closet. Wearing %s.", got, OUTFITS[last].name); toast(b); }
        addMoodlet(me, "New Fit Energy", EM_CONFIDENT, 2, 300); sfxYay(); return;
    }
    case DA_CASHIER: {
        a = makeAct("Working the Register", 120, 0); a.rate[N_FUN] = -4; a.rate[N_ENERGY] = -6; startActivity(a);
        G.money += 28; skillGain(me, SK_CHARISMA, 2); toast("Shift at Sugar Market: +$28. \"Paper or plastic?\""); return;
    }
    case DA_FLOWERS: { if (!pay(12)) return; int p = partnerOf(G.mc); if (p >= 0) { changeRel(G.mc, p, 2, 5); toast("Grocery-store flowers for " + G.sims[p].name + ". Simple, sweet, perfect."); } else { addMoodlet(me, "Treat Yourself", EM_HAPPY, 1, 240); toast("Flowers for yourself. You deserve them."); } return; }
    case DA_TAN: a = makeAct("Soaking Up the Sun", 90, 0); a.rate[N_FUN] = 15; a.rate[N_ENERGY] = 6; startActivity(a); addMoodlet(me, "Sun-Kissed", EM_HAPPY, 1, 360); return;
    case DA_SNORKEL: a = makeAct("Snorkeling", 60, 8); a.rate[N_FUN] = 25; a.rate[N_HYGIENE] = 10; a.skill = SK_FITNESS; a.skillRate = 6; startActivity(a); toast(irand(2) ? "You swam with a sea turtle!!" : "A school of rainbow fish swirled around you."); return;
    case DA_SHELLS: a = makeAct("Collecting Shells", 45, 0); a.rate[N_FUN] = 12; startActivity(a); if (irand(4) == 0) { G.money += 30; toast("You found a rare pink conch! A collector paid $30."); } return;
    case DA_MOCKTAIL: if (!pay(8)) return; me.needs[N_HUNGER] = std::fmin(100.f, me.needs[N_HUNGER] + 15); addMoodlet(me, "Tropical Vibes", EM_HAPPY, 1, 240); toast("A mango mocktail with a tiny umbrella. Perfection."); return;
    case DA_HULA: a = makeAct("Hula Lesson", 45, 8); a.kind = AK_DANCE; a.pose = PO_DANCE; a.rate[N_FUN] = 20; a.skill = SK_FITNESS; a.skillRate = 6; startActivity(a); return;
    case DA_SOUVENIR: if (!pay(15)) return; { static const char* S_[] = {"a shell necklace", "a snow globe", "a tiny tower keychain", "a postcard set", "a beret"}; toast(std::string("You bought ") + S_[irand(5)] + " to remember the trip!"); addMemoryOnce(MEM_FUN, G.mc, -1, "Bought a souvenir on vacation."); } return;
    case DA_FLYHOME: travelDo(11); return;
    case DA_SKI: a = makeAct("Skiing", 90, 10); a.rate[N_FUN] = 28; a.rate[N_ENERGY] = -12; a.skill = SK_FITNESS; a.skillRate = 10; startActivity(a); if (irand(6) == 0) toast("You wiped out spectacularly. Nobody saw. (Everybody saw.)"); return;
    case DA_SNOWBOARD: if (!pay(30)) return; a = makeAct("Snowboard Lesson", 60, 10); a.rate[N_ENERGY] = -10; a.skill = SK_FITNESS; a.skillRate = 16; startActivity(a); return;
    case DA_SNOWMAN: a = makeAct("Building a Snowman", 45, 0); a.rate[N_FUN] = 18; a.skill = SK_CREATIVITY; a.skillRate = 5; startActivity(a); return;
    case DA_SNOWBALL: a = makeAct("Snowball Fight", 30, 8); a.rate[N_FUN] = 25; a.rate[N_SOCIAL] = 15; startActivity(a); addMoodlet(me, "Snowball Champion", EM_PLAYFUL, 1, 180); return;
    case DA_COCOA: if (!pay(5)) return; me.needs[N_HUNGER] = std::fmin(100.f, me.needs[N_HUNGER] + 15); addMoodlet(me, "Cozy Cocoa", EM_HAPPY, 1, 240); toast("Hot cocoa with a mountain of marshmallows."); return;
    case DA_COZY: a = makeAct("Cozying Up by the Fire", 60, 0); a.rate[N_ENERGY] = 12; a.rate[N_FUN] = 10; startActivity(a); return;
    case DA_HOTTUB: a = makeAct("Soaking in the Hot Tub", 45, 0); a.rate[N_HYGIENE] = 15; a.rate[N_FUN] = 15; a.rate[N_ENERGY] = 8; startActivity(a); addMoodlet(me, "Totally Relaxed", EM_HAPPY, 2, 300); return;
    case DA_CREPE: if (!pay(9)) return; me.needs[N_HUNGER] = std::fmin(100.f, me.needs[N_HUNGER] + 35); toast("Strawberry-Nutella crepe. Life-changing."); addMoodlet(me, "Oh La La", EM_HAPPY, 1, 240); return;
    case DA_COUTURE: { if (!pay(120)) return; int k = 32 + irand(FIRST_KID_OUTFIT - 32); giveOutfit(k); toast(std::string("Belle Ville couture: ") + OUTFITS[k].name + "! So chic."); addMoodlet(me, "Couture Queen", EM_CONFIDENT, 2, 480); return; }
    case DA_VIEW: a = makeAct("Taking in the View", 30, 0); a.rate[N_FUN] = 15; startActivity(a); addMoodlet(me, "Breathtaking", EM_INSPIRED, 2, 300); return;
    case DA_TOPSELFIE: G.followers += 80 + irand(200); addMoodlet(me, "Bucket List!", EM_HAPPY, 1, 240); toast("Selfie at the top of the tower. Your followers are SO jealous."); return;
    case DA_PORTRAIT: if (!pay(25)) return; a = makeAct("Getting a Portrait Painted", 45, 0); a.rate[N_FUN] = 10; startActivity(a); addMemoryOnce(MEM_FUN, G.mc, -1, "Got my portrait painted by a street artist in Belle Ville."); toast("The artist made you look like a movie star."); return;
    case DA_GHOSTHUNT: { a = makeAct("Ghost Hunting", 60, 8); a.rate[N_FUN] = 20; a.rate[N_ENERGY] = -6; startActivity(a);
        static const char* G_[] = {"A cold breeze... and a door slammed. You SCREAMED.", "You caught an orb on camera! Viral material.", "Something giggled upstairs. Nope. Nope. Nope.", "Turns out it was just a raccoon. Iconic."};
        int g = irand(4); toast(G_[g]); if (g == 2) addMoodlet(me, "Screamed in Public", EM_EMBARRASSED, 2, 120); addMoodlet(me, g == 1 ? "Ghost Content Queen" : "Spooked!", g == 1 ? EM_CONFIDENT : EM_PLAYFUL, 1, 200); if (g == 1) G.followers += 40; return; }
    case DA_SEANCE: { a = makeAct("Holding a Seance", 45, 0); a.rate[N_FUN] = 18; a.rate[N_SOCIAL] = 6; startActivity(a);
        static const char* S_[] = {"The candles flickered. A spirit said... \"nice outfit.\"", "The board spelled out G-O-H-O-M-E. Rude.", "Grandma's ghost says to drink more water."};
        { std::string sc = seanceContact(); toast(sc.empty() ? std::string(S_[irand(3)]) : sc); } return; }
    case DA_DARE: a = makeAct("Spending the Night on a Dare", 120, 0); a.rate[N_ENERGY] = -8; a.rate[N_FUN] = 25; startActivity(a); skillGain(me, SK_CHARISMA, 2); addMoodlet(me, "Fearless", EM_CONFIDENT, 2, 300); toast("You survived the night at Hollow Oak Manor. Legend status."); return;
    case DA_FRATPARTY: a = makeAct("Partying at the Frat", 90, 10); a.kind = AK_DANCE; a.pose = PO_DANCE; a.rate[N_FUN] = 30; a.rate[N_SOCIAL] = 30; a.rate[N_ENERGY] = -10; startActivity(a); toast("The bass is shaking the walls. Rush week!"); return;
    case DA_FRATHANG: a = makeAct("Hanging on the Lawn Couch", 45, 0); a.rate[N_SOCIAL] = 20; a.rate[N_FUN] = 10; startActivity(a); return;
    case DA_CSNACK: if (!pay(4)) return; me.needs[N_HUNGER] = std::fmin(100.f, me.needs[N_HUNGER] + 20); toast("Hot chips and a slushie. Corner store classic."); return;
    case DA_SCRATCH: { if (!pay(5)) return; int w = irand(10) == 0 ? 100 : irand(3) == 0 ? 10 : 0; G.money += w; toast(w ? "Scratch-off winner! +$" + std::to_string(w) : "Scratch-off: nothing. Classic."); return; }
    case DA_BROWSE: { if (!pay(10)) return; int k = 32 + irand(FIRST_KID_OUTFIT - 32); giveOutfit(k); toast(std::string("Thrift find: ") + OUTFITS[k].name + "!"); return; }
    case DA_DONATE: addMoodlet(me, "Decluttered", EM_HAPPY, 1, 200); toast("You donated a bag of clothes. Good karma!"); return;
    case DA_TENNIS: if (irand(100) < 4) catchIllness(G.mc, IL_INJURY); a = makeAct("Playing Tennis", 60, 10); a.rate[N_FUN] = 15; a.rate[N_ENERGY] = -12; a.skill = SK_FITNESS; a.skillRate = 8; startActivity(a); return;
    case DA_LESSON: if (!pay(25)) return; a = makeAct("Tennis Lesson", 60, 10); a.rate[N_ENERGY] = -10; a.skill = SK_FITNESS; a.skillRate = 16; startActivity(a); return;
    case DA_WATCH: a = makeAct("Watching a Match", 45, 0); a.rate[N_FUN] = 12; startActivity(a); return;
    case DA_SLOWDANCE: { int p = partnerOf(G.mc); a = makeAct("Slow Dancing", 30, 8); a.kind = AK_DANCE; a.pose = PO_DANCE; a.rate[N_FUN] = 25; startActivity(a); if (p >= 0) changeRel(G.mc, p, 3, 8); dateBonus(15); return; }
    case DA_NEXIT: if (g_host >= 0) say(g_host, "Come back anytime!"); travelTo(R_STREET); return;
    default:
        if (act >= DA_KNOCK0 && act < DA_KNOCK0 + 3) {
            int n = neighborFor(act - DA_KNOCK0);
            int h = (int)(G.minute / 60);
            if (n < 0 || h < 8 || h >= 22 || irand(100) < 22) { toast("You knock... nobody's home."); return; }
            g_host = n;
            say(G.mc, "*knock knock*");
            travelTo(R_NEIGHBOR);
            districtArrive(R_NEIGHBOR);
            snprintf(b, sizeof b, "%s invited you in!", G.sims[n].name.c_str()); toast(b);
        }
        return;
    }
}

// ---------------------------------------------------------------- drawing
static void ol(float x, float y, float w, float h, RGB c, float r = 3) { rrect(x - 1.5f, y - 1.5f, w + 3, h + 3, r + 1.5f, C(UI_INK)); rrect(x, y, w, h, r, C(c)); }
static void store(float x, float w, RGB c, const char* name) {   // a shopfront on the back wall
    ol(x, 34, w, 118, shade(c, 1.15f), 2);
    ol(x + 6, 58, w - 12, 64, RGB{190, 236, 250}, 2);
    for (int i = 0; i < 3; i++) quad(x + 12 + i * (w - 24) / 3.f, 60, x + 18 + i * (w - 24) / 3.f, 60, x + 8 + i * (w - 24) / 3.f, 120, x + 2 + i * (w - 24) / 3.f, 120, C(255, 255, 255, 70));
    ol(x + 4, 38, w - 8, 16, c, 4);
    textOutline(name, x + w / 2, 38.5f, 0.36f, C(255, 255, 255), C(UI_INK), 1, 0.7f);
    for (int i = 0; i < (int)(w / 12); i++) tri(x + i * 12.f, 126, x + i * 12.f + 12, 126, x + i * 12.f + 6, 134, C(i % 2 ? RGB{255, 255, 255} : c));
}
static void mallFloor(RGB wall) {
    grad(0, 0, TOP_W, 158, C(wall), C(shade(wall, 0.9f)));
    for (int i = 0; i < 20; i++) rect(i * 20.f, 0, 2, 30, C(255, 255, 255, 40));
    rect(0, 156, TOP_W, 2, C(UI_INK));
    for (int r = 0; r < 4; r++) for (int k = 0; k < 10; k++) rect(k * 40.f + (r % 2) * 20, 158 + r * 21.f, 40, 21, C((r + k) % 2 ? RGB{250, 236, 244} : RGB{236, 214, 230}));
}
void districtDrawRoom(int room, float t, bool night) {
    switch (room) {
    case R_COVESUITE: case R_PEAKSUITE: case R_BELLESUITE: {   // five-star suite: warm neutrals, symmetry, layered light
        const int v = room == R_COVESUITE ? 0 : room == R_PEAKSUITE ? 1 : 2;
        grad(0, 0, TOP_W, 158, C(238, 228, 214), C(224, 212, 196));
        for (int k = 0; k < 10; k++) rect(k * 40.f + 20, 0, 1, 158, C(214, 202, 186, 120));   // fine wall paneling
        rect(0, 10, TOP_W, 3, C(250, 236, 200, 160));                                            // hidden cove LED
        // floor-to-ceiling window with the destination view + sheer drapes
        ol(288, 18, 104, 140, RGB{60, 52, 46}, 2);
        if (v == 0) { grad(290, 20, 100, 70, C(120, 200, 250), C(200, 236, 255)); grad(290, 90, 100, 66, C(40, 190, 210), C(90, 220, 220)); circle(360, 46, 9, C(255, 240, 170)); }
        else if (v == 1) { grad(290, 20, 100, 136, C(150, 190, 240), C(225, 240, 255)); tri(290, 130, 330, 60, 370, 130, C(240, 246, 255)); tri(330, 130, 372, 74, 390, 130, C(220, 230, 245)); rect(290, 128, 100, 28, C(250, 252, 255)); }
        else { grad(290, 20, 100, 136, C(70, 60, 120), C(240, 160, 140)); tri(330, 150, 340, 40, 350, 150, C(60, 50, 70)); rect(337, 60, 6, 4, C(255, 220, 140)); for (int k = 0; k < 8; k++) rect(296 + k * 12.f, 120 + (k % 3) * 8.f, 8, 40, C(80, 70, 100)); }
        rect(339, 18, 2, 140, C(60, 52, 46));
        rect(282, 14, 14, 146, C(250, 246, 238, 230)); rect(384, 14, 14, 146, C(250, 246, 238, 230));                     // sheers
        rect(276, 10, 124, 4, C(255, 236, 190, 200));                                                                       // valance glow
        // tall upholstered headboard (jewel-tone velvet) with glow behind it
        rrect(16, 30, 116, 112, 8, C(255, 232, 180, 70));
        rrect(22, 36, 104, 104, 6, C(UI_INK)); rrect(24, 38, 100, 100, 5, v == 1 ? C(40, 90, 80) : v == 2 ? C(40, 50, 90) : C(70, 100, 120));
        for (int k = 0; k < 4; k++) rect(30 + k * 24.f, 42, 1, 92, C(0, 0, 0, 40));   // channel tufting
        // symmetrical brass sconces
        for (int sx : {12, 136}) { rect(sx, 64, 2, 12, C(190, 150, 70)); ol(sx - 5, 56, 12, 9, RGB{255, 240, 200}, 1); circle(sx + 1, 70, 8, C(255, 230, 160, 50)); }
        // floor: plush carpet + silk rug
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(206, 192, 176));
        for (int k = 0; k < 40; k++) rect((k * 37) % 400, 160 + (k * 13) % 78, 3, 1, C(196, 182, 166));
        ellipse(200, 214, 120, 20, C(226, 214, 196)); ellipse(200, 214, 112, 16, C(236, 226, 210));
        // lounge nook: velvet chaise under the window
        ol(250, 128, 46, 18, RGB{200, 186, 166}, 6); rrect(286, 112, 10, 34, 4, C(200, 186, 166));
        if (night) { rect(0, 0, TOP_W, TOP_H, C(20, 14, 30, 110)); circle(13, 70, 18, C(255, 220, 150, 50)); circle(137, 70, 18, C(255, 220, 150, 50)); rect(0, 10, TOP_W, 3, C(255, 220, 150, 160)); }
        break; }
    case R_HOTELLOBBY: {   // marble, brass and a chandelier; the view outside matches the trip
        const int v = vacationDest();
        grad(0, 0, TOP_W, 158, C(244, 238, 228), C(230, 222, 208));
        for (int k = 0; k < 6; k++) { rect(20 + k * 72.f, 0, 12, 158, C(252, 250, 244)); rect(16 + k * 72.f, 150, 20, 8, C(220, 210, 190)); }
        ol(150, 30, 100, 70, RGB{60, 52, 46}, 2);
        if (v == 1) grad(152, 32, 96, 66, C(120, 200, 250), C(60, 200, 210)); else if (v == 2) grad(152, 32, 96, 66, C(180, 210, 245), C(245, 250, 255)); else grad(152, 32, 96, 66, C(80, 70, 130), C(240, 160, 140));
        for (int k = 0; k < 9; k++) { float a = k * 0.7f; circle(200 + std::cos(a) * 26, 20 + std::sin(a) * 6, 2.5f, C(255, 236, 170)); line(200, 4, 200 + std::cos(a) * 26, 20 + std::sin(a) * 6, 1, C(200, 170, 90)); }
        circle(200, 22, 10, C(255, 240, 200, 90));
        rect(0, 156, TOP_W, 2, C(UI_INK));
        for (int k = 0; k < 10; k++) rect(k * 40.f, 158, 40, 82, C(k % 2 ? RGB{246, 242, 236} : RGB{232, 226, 218}));
        for (int k = 0; k < 6; k++) line(k * 70.f, 158, k * 70.f + 30, 240, 1, C(210, 204, 196));   // marble veins
        if (night) rect(0, 0, TOP_W, TOP_H, C(20, 14, 30, 90));
        break; }
    case R_HOTELBATH:   // spa-like marble bath: book-matched slabs, backlit mirror, towel rack
        grad(0, 0, TOP_W, 158, C(246, 244, 240), C(232, 228, 222));
        for (int k = 0; k < 8; k++) { line(k * 50.f, 0, k * 50.f + 40, 158, 1.5f, C(200, 196, 192, 150)); line(k * 50.f + 20, 0, k * 50.f - 10, 158, 1, C(210, 206, 200, 120)); }
        rrect(282, 40, 96, 56, 26, C(255, 244, 210, 120)); rrect(288, 46, 84, 44, 22, C(200, 220, 230));   // backlit mirror
        for (int k = 0; k < 3; k++) { rrect(160 + k * 18.f, 40, 14, 40, 3, C(255, 255, 255)); line(156, 40, 220, 40, 2, C(190, 150, 70)); }   // heated towel rack
        rect(0, 156, TOP_W, 2, C(UI_INK)); for (int k = 0; k < 8; k++) rect(k * 50.f, 158, 50, 82, C(k % 2 ? RGB{240, 238, 234} : RGB{228, 224, 218}));
        if (night) rect(0, 0, TOP_W, TOP_H, C(20, 14, 30, 80));
        break;
    case R_HOTELPOOL: {
        const int v = vacationDest();
        if (v == 2) grad(0, 0, TOP_W, 120, C(170, 205, 245), C(240, 248, 255)); else if (v == 3) grad(0, 0, TOP_W, 120, C(80, 70, 130), C(240, 160, 140)); else grad(0, 0, TOP_W, 120, C(110, 200, 250), C(210, 240, 255));
        if (v == 2) { tri(0, 120, 100, 30, 200, 120, C(250, 252, 255)); tri(160, 120, 270, 40, 380, 120, C(236, 242, 250)); }
        else if (v == 3) { for (int k = 0; k < 8; k++) rect(10 + k * 50.f, 60 + (k % 3) * 14.f, 34, 60, C(80, 70, 100)); tri(300, 120, 312, 20, 324, 120, C(60, 50, 70)); }
        else { grad(0, 90, TOP_W, 30, C(40, 190, 210), C(90, 220, 220)); }
        rect(0, 118, TOP_W, 40, C(240, 232, 218));
        rrect(40, 124, 200, 30, 6, C(60, 190, 230)); for (int k = 0; k < 5; k++) rect(52 + k * 38.f + 6 * std::sin(t * 2 + k), 132 + (k % 2) * 8.f, 20, 2, C(255, 255, 255, 170));
        if (v == 2) for (int k = 0; k < 6; k++) circle(60 + k * 34.f, 128 + std::sin(t * 3 + k) * 2, 5, C(255, 255, 255, 120));   // steam: heated pool
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(236, 226, 210));
        if (night) rect(0, 0, TOP_W, TOP_H, C(10, 20, 50, 100));
        break; }
    case R_COVESIGHTS:
        grad(0, 0, TOP_W, 158, C(120, 210, 170), C(200, 240, 210));
        rect(40, 0, 70, 158, C(110, 120, 110)); for (int k = 0; k < 14; k++) rect(56 + (k % 3) * 12.f, std::fmod(k * 13.f + t * 90, 158.f), 4, 16, C(220, 245, 255, 200));   // waterfall
        ellipse(76, 160, 70, 14, C(60, 190, 210));
        for (int k = 0; k < 4; k++) { float x = 160 + k * 66.f; line(x, 158, x + 6, 70, 5, C(140, 100, 60)); for (int f = 0; f < 5; f++) { float a = f * 1.25f; line(x + 6, 70, x + 6 + std::cos(a) * 26, 70 + std::sin(a) * 12 + 6, 4, C(60, 170, 80)); } }
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(240, 222, 170));
        if (night) rect(0, 0, TOP_W, TOP_H, C(10, 20, 50, 100));
        break;
    case R_PEAKSIGHTS:
        grad(0, 0, TOP_W, 158, C(170, 205, 245), C(235, 245, 255));
        tri(0, 120, 90, 30, 180, 120, C(250, 252, 255)); tri(140, 120, 250, 20, 360, 120, C(236, 242, 250)); tri(300, 120, 380, 50, 400, 120, C(250, 252, 255));
        for (int k = 0; k < 4; k++) { float x = 20 + k * 100.f; rect(x, 90, 70, 60, C(150, 100, 70)); tri(x - 8, 92, x + 35, 62, x + 78, 92, C(250, 252, 255)); rect(x + 25, 118, 20, 32, C(110, 70, 40)); rect(x + 6, 100, 14, 12, C(255, 220, 140)); }
        for (int k = 0; k < 20; k++) circle(std::fmod(k * 41.f + t * 12, 400.f), std::fmod(k * 29.f + t * 30, 158.f), 1.5f, C(255, 255, 255));
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(244, 248, 255));
        if (night) rect(0, 0, TOP_W, TOP_H, C(10, 20, 50, 100));
        break;
    case R_BELLESIGHTS:
        grad(0, 0, TOP_W, 158, C(236, 228, 212), C(220, 210, 192));
        for (int k = 0; k < 5; k++) { rect(16 + k * 80.f, 0, 14, 158, C(250, 246, 236)); rect(12 + k * 80.f, 0, 22, 8, C(230, 220, 200)); }   // columns
        static const RGB ART[4] = {{200, 80, 90}, {70, 110, 170}, {230, 180, 80}, {90, 150, 110}};
        for (int k = 0; k < 4; k++) { float x = 40 + k * 80.f; ol(x, 40, 52, 40, RGB{190, 150, 70}, 3); rect(x + 4, 44, 44, 32, C(ART[k])); circle(x + 26, 60, 8, C(255, 240, 200, 160)); }
        rect(0, 156, TOP_W, 2, C(UI_INK)); for (int k = 0; k < 10; k++) rect(k * 40.f, 158, 40, 82, C(k % 2 ? RGB{200, 180, 150} : RGB{214, 196, 168}));
        if (night) rect(0, 0, TOP_W, TOP_H, C(10, 10, 30, 90));
        break;
    case R_GLOWBOWL: case R_KARAOKE: case R_PAINTSIP: case R_DANCESTUDIO: case R_NIGHTMKT: case R_SERENITY: nightDrawRoom(room, t, night); break;
    case R_DOWNTOWN: case R_DTBOUTIQUE: case R_DTBISTRO: hotdateDrawRoom(room, t, night); break;
    case R_VELVET: spicyDrawRoom(room, t, night); break;
    case R_FUNPARK: case R_FUNRIDES: case R_SUBWAY: case R_PIXELARCADE: case R_LAUNDRO: case R_AQUARIUM: case R_MUSEUM: case R_GARDENS: case R_ROOFTOP: funparkDrawRoom(room, t, night); break;
    case R_FOODCOURT: case R_HALLWAY: case R_CLASSROOM: case R_CAFETERIA: case R_SCHOOLGYM: case R_DAYCARE: case R_CHURCH: case R_COMMUNITY: case R_POOL: case R_BLOCKPARTY: case R_DORM: case R_SALON: case R_ROLLER: case R_NIGHTMARKET: case R_COURTHOUSE: town2DrawRoom(room, t, night); break;
    case R_AVSQUARE: case R_AVFARM: case R_AVBARN: farmDrawRoom(room, t, night); break;
    case R_JAIL:
        rect(0, 0, TOP_W, 158, C(150, 156, 162));
        for (int k = 0; k < 8; k++) rect(0, 20 + k * 18.f, TOP_W, 1, C(130, 136, 142));
        for (int k = 0; k < 20; k++) rect(k * 20.f + ((k / 1) % 2) * 0, 0, 1, 158, C(138, 144, 150, 120));
        ol(8, 36, 112, 118, RGB{90, 96, 104}, 2); rect(10, 38, 108, 114, C(60, 64, 72));   // the cell
        for (int k = 0; k < 9; k++) rect(14 + k * 12.f, 38, 4, 116, C(40, 42, 48));
        rect(10, 60, 108, 3, C(40, 42, 48)); rect(10, 120, 108, 3, C(40, 42, 48));
        ol(228, 30, 74, 46, RGB{240, 240, 236}, 2); for (int k = 0; k < 4; k++) rect(232, 36 + k * 9.f, 66, 1, C(40, 42, 48));   // line-up height chart
        text("6'", 300, 32, 0.3f, C(UI_INK)); text("5'", 300, 50, 0.3f, C(UI_INK));
        ol(130, 14, 140, 18, RGB{40, 50, 70}, 4); textOutline("WESTBROOK COUNTY", 200, 15, 0.38f, C(255, 255, 255), C(UI_INK), 1, 0.6f);
        rect(316, 40, 70, 60, C(120, 170, 220, 180)); for (int k = 0; k < 6; k++) rect(318 + k * 12.f, 40, 2, 60, C(60, 64, 72));   // barred window
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(120, 124, 128));
        for (int q = 0; q < 4; q++) for (int k = 0; k < 10; k++) rect(k * 40.f + (q % 2) * 20, 158 + q * 21.f, 39, 20, C((q + k) % 2 ? RGB{128, 132, 136} : RGB{114, 118, 122}));
        if (night) rect(0, 0, TOP_W, TOP_H, C(10, 10, 30, 90));
        break;
    case R_BOARDING:
        grad(0, 0, TOP_W, 158, C(110, 60, 70), C(150, 90, 90));
        for (int k = 0; k < 10; k++) rect(k * 40.f, 0, 2, 158, C(90, 46, 56));   // wood panels
        ol(140, 10, 120, 22, RGB{30, 50, 90}, 4); textOutline("ROSEWOOD ACADEMY", 200, 13, 0.4f, C(255, 220, 120), C(UI_INK), 1, 0.6f);
        ol(160, 40, 80, 70, RGB{250, 240, 210}, 3); rect(164, 44, 72, 62, C(140, 190, 230)); rect(198, 44, 4, 62, C(250, 240, 210)); rect(164, 72, 72, 4, C(250, 240, 210));   // tall window
        for (int i = 0; i < 3; i++) { ol(20 + i * 30.f, 50, 22, 50, RGB{30, 50, 90}, 2); rect(22 + i * 30.f, 54, 18, 6, C(255, 220, 120)); }   // pennants & crest banners
        ol(280, 46, 100, 56, RGB{80, 50, 30}, 2); for (int k = 0; k < 3; k++) for (int j = 0; j < 7; j++) rect(284 + j * 13.f, 50 + k * 17.f, 10, 14, C(j % 3 == 0 ? RGB{200, 60, 70} : j % 3 == 1 ? RGB{60, 90, 160} : RGB{220, 200, 120}));   // bookshelf
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(150, 30, 50));
        for (int k = 0; k < 10; k++) rect(k * 40.f, 158, 2, 82, C(130, 24, 44));
        rect(0, 196, TOP_W, 2, C(220, 190, 110, 180));
        if (night) rect(0, 0, TOP_W, TOP_H, C(10, 10, 40, 80));
        break;
    case R_MALL_BEAUTY:
        mallFloor(RGB{255, 214, 236});
        store(8, 120, RGB{255, 46, 140}, "GLOSS & GO"); store(140, 110, RGB{150, 96, 230}, "NAIL BAR");
        store(262, 130, RGB{255, 168, 36}, "FOOD COURT");
        for (int i = 0; i < 3; i++) { ol(270 + i * 40.f, 64, 30, 22, RGB{255, 255, 255}, 6); circle(285 + i * 40.f, 75, 6, C(i == 0 ? RGB{240, 80, 60} : i == 1 ? RGB{255, 200, 60} : RGB{120, 200, 120})); }
        break;
    case R_ARCADE:
        grad(0, 0, TOP_W, 158, C(40, 20, 70), C(80, 30, 110));
        for (int i = 0; i < 14; i++) { float a = 0.5f + 0.5f * std::sin(t * 3 + i); circle(14 + i * 28.f, 14, 3, C(i % 3 == 0 ? RGB{255, 60, 170} : i % 3 == 1 ? RGB{60, 220, 255} : RGB{255, 230, 60}, (uint8_t)(120 + 135 * a))); }
        textOutline("FUN ZONE", 120, 24, 0.9f, C(255, 90, 200), C(255, 255, 255), 1, 1.0f);
        store(240, 150, RGB{40, 192, 210}, "COZY HOME DECOR");
        rect(0, 156, TOP_W, 2, C(UI_INK));
        for (int r = 0; r < 4; r++) for (int k = 0; k < 10; k++) rect(k * 40.f + (r % 2) * 20, 158 + r * 21.f, 40, 21, C((r + k) % 2 ? RGB{60, 30, 90} : RGB{80, 40, 110}));
        for (int i = 0; i < 6; i++) sparkle(30 + i * 60.f, 170 + (i % 2) * 40, 2 + std::fabs(std::sin(t * 2 + i)) * 2, C(255, 255, 255, 120));
        break;
    case R_BAKERY:
        grad(0, 0, TOP_W, 158, C(255, 236, 214), C(255, 220, 230));
        for (int i = 0; i < 20; i++) rect(i * 20.f, 0, 10, 158, C(255, 255, 255, 50));
        ol(110, 20, 180, 30, RGB{255, 120, 180}, 12);
        textOutline("SUGAR RUSH BAKERY", 200, 24, 0.55f, C(255, 255, 255), C(UI_INK), 1);
        for (int i = 0; i < 4; i++) { ol(40 + i * 90.f, 70, 40, 34, RGB{180, 236, 255}, 3); }
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(200, 150, 110));
        for (int k = 0; k < 20; k++) rect(k * 20.f, 158, 1, 82, C(170, 120, 90));
        break;
    case R_PLAYGROUND: {
        grad(0, 0, TOP_W, 160, night ? C(30, 30, 80) : C(120, 210, 250), night ? C(80, 60, 120) : C(220, 245, 255));
        if (!night) { flashSun(350, 36, 16, t); flashCloud(90, 40, 0.8f); }
        for (int i = 0; i < 6; i++) { float x = 20 + i * 76.f; rect(x - 3, 110, 6, 50, C(130, 90, 60)); circle(x, 100, 24, C(UI_INK)); circle(x, 100, 22, C(90, 180, 90)); circle(x - 10, 94, 12, C(110, 200, 100)); }
        rect(0, 158, TOP_W, 82, C(120, 200, 100)); rect(0, 157, TOP_W, 2, C(UI_INK));
        quad(0, 240, 400, 240, 320, 170, 80, 170, C(230, 210, 170, 140));
        break;
    }
    case R_DINER:
        grad(0, 0, TOP_W, 158, C(150, 230, 230), C(120, 210, 214));
        for (int i = 0; i < 20; i++) rect(i * 20.f, 100, 20, 8, C(i % 2 ? RGB{255, 255, 255} : RGB{255, 60, 100}));
        ol(100, 14, 200, 34, RGB{255, 60, 100}, 14);
        textOutline("SUNNY SIDE DINER", 200, 20, 0.6f, C(255, 255, 255), C(UI_INK), 1);
        for (int i = 0; i < 3; i++) ol(40 + i * 120.f, 56, 80, 36, RGB{200, 240, 255}, 4);
        rect(0, 156, TOP_W, 2, C(UI_INK));
        for (int r = 0; r < 4; r++) for (int k = 0; k < 20; k++) rect(k * 20.f, 158 + r * 21.f, 20, 21, C((r + k) % 2 ? RGB{30, 26, 30} : RGB{250, 250, 250}));
        break;
    case R_SUSHI:
        grad(0, 0, TOP_W, 158, C(250, 236, 214), C(236, 214, 190));
        for (int i = 0; i < 8; i++) { ol(10 + i * 50.f, 30, 40, 70, RGB{255, 250, 240}, 1); line(30 + i * 50.f, 30, 30 + i * 50.f, 100, 1, C(180, 150, 120)); }
        for (int i = 0; i < 5; i++) { float x = 40 + i * 80.f; circle(x, 14, 9, C(UI_INK)); circle(x, 14, 7.5f, C(255, 80, 80)); }
        ol(130, 108, 140, 24, RGB{60, 40, 40}, 6);
        textOutline("SAKURA SUSHI", 200, 110, 0.5f, C(255, 200, 220), C(UI_INK), 1);
        for (int i = 0; i < 6; i++) { float bx = std::fmod(i * 70.f + t * 10, 420.f) - 20; ellipse(bx + 20, 46, 6, 4, C(255, 160, 200, 200)); }
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(190, 150, 110));
        for (int k = 0; k < 10; k++) rect(k * 40.f, 158, 1, 82, C(160, 120, 90));
        break;
    case R_STREET: {
        grad(0, 0, TOP_W, 150, night ? C(24, 24, 70) : C(110, 205, 250), night ? C(70, 50, 110) : C(220, 245, 255));
        if (!night) { flashSun(370, 30, 14, t); flashCloud(160, 26, 0.7f); }
        static const RGB HC[4][2] = {{{255, 220, 236}, {255, 70, 150}}, {{226, 206, 255}, {150, 96, 230}}, {{200, 245, 220}, {60, 170, 120}}, {{255, 222, 196}, {255, 140, 90}}};
        for (int i = 0; i < 4; i++) {
            float x = 12 + i * 82.f;
            ol(x, 92, 66, 64, HC[i][0], 1);
            tri(x - 6, 93, x + 72, 93, x + 33, 56, C(UI_INK)); tri(x - 3, 92, x + 69, 92, x + 33, 59, C(HC[i][1]));
            ol(x + 25, 122, 16, 34, shade(HC[i][1], 0.8f), 2); circle(x + 37, 140, 1.4f, C(GOLD));
            ol(x + 6, 102, 14, 12, RGB{180, 230, 255}, 1); ol(x + 46, 102, 14, 12, RGB{180, 230, 255}, 1);
            if (night) { rect(x + 6, 102, 14, 12, C(255, 220, 120, 200)); }
        }
        textOutline("YOU", 45, 76, 0.4f, C(255, 255, 255), C(UI_INK), 1);
        rect(0, 156, TOP_W, 84, C(130, 205, 110)); rect(0, 155, TOP_W, 2, C(UI_INK));
        rect(0, 166, TOP_W, 26, C(220, 214, 210)); for (int k = 0; k < 20; k++) rect(k * 20.f, 166, 1, 26, C(190, 184, 180));
        for (int i = 0; i < 4; i++) rect(12 + i * 82.f + 25, 156, 16, 10, C(220, 214, 210));
        break;
    }
    case R_HOSPITAL:
        grad(0, 0, TOP_W, 158, C(220, 244, 246), C(196, 230, 236));
        rect(0, 96, TOP_W, 6, C(64, 190, 200));
        ol(130, 16, 140, 26, RGB{255, 255, 255}, 6);
        rect(146, 22, 14, 4, C(230, 50, 70)); rect(151, 17, 4, 14, C(230, 50, 70));
        textOutline("STARLIGHT HOSPITAL", 214, 20, 0.4f, C(40, 120, 150), C(255, 255, 255), 1, 0.5f);
        for (int i = 0; i < 3; i++) ol(30 + i * 130.f, 48, 60, 40, RGB{190, 236, 255}, 2);
        rect(0, 156, TOP_W, 2, C(UI_INK));
        for (int r = 0; r < 4; r++) for (int k = 0; k < 10; k++) rect(k * 40.f + (r % 2) * 20, 158 + r * 21.f, 40, 21, C((r + k) % 2 ? RGB{240, 248, 250} : RGB{214, 232, 238}));
        break;
    case R_PETSHOP:
        grad(0, 0, TOP_W, 158, C(255, 240, 200), C(250, 226, 180));
        for (int i = 0; i < 12; i++) { float x = 16 + i * 34.f; circle(x, 30 + (i % 2) * 40, 4, C(230, 170, 120, 120)); for (int k = 0; k < 3; k++) circle(x - 4 + k * 4.f, 24 + (i % 2) * 40, 2, C(230, 170, 120, 120)); }
        ol(120, 12, 160, 28, RGB{255, 140, 60}, 10);
        textOutline("PAWS & CLAWS", 200, 16, 0.55f, C(255, 255, 255), C(UI_INK), 1);
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(150, 210, 130));
        for (int k = 0; k < 20; k++) rect(k * 20.f, 158, 1, 82, C(130, 190, 110));
        break;
    case R_LIBRARY:
        grad(0, 0, TOP_W, 158, C(120, 70, 50), C(100, 56, 40));
        for (int sh = 0; sh < 3; sh++) for (int i = 0; i < 40; i++) { static const RGB BK[5] = {{200, 60, 60}, {60, 120, 180}, {230, 180, 60}, {80, 160, 100}, {150, 90, 170}}; rect(10 + i * 9.5f, 24 + sh * 40.f, 8, 30 - (i % 3) * 3, C(BK[(i * 7 + sh) % 5])); }
        for (int sh = 0; sh < 4; sh++) rect(0, 54 + sh * 40.f - 40, TOP_W, 4, C(70, 40, 28));
        ol(130, 4, 140, 16, RGB{255, 220, 140}, 4);
        textOutline("PAGE TURNER LIBRARY", 200, 4, 0.36f, C(120, 60, 30), C(255, 255, 255), 1, 0.5f);
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(170, 40, 50));
        for (int k = 0; k < 10; k++) rect(k * 40.f + 6, 160, 2, 80, C(150, 30, 40));
        break;
    case R_GYM:
        grad(0, 0, TOP_W, 158, C(200, 210, 224), C(176, 188, 206));
        for (int i = 0; i < 4; i++) ol(14 + i * 100.f, 20, 80, 50, RGB{150, 220, 255}, 2);
        ol(140, 84, 120, 20, RGB{255, 90, 60}, 6);
        textOutline("FITLIFE GYM", 200, 86, 0.45f, C(255, 255, 255), C(UI_INK), 1);
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(60, 64, 76));
        for (int k = 0; k < 20; k++) rect(k * 20.f, 158, 1, 82, C(80, 84, 96));
        break;
    case R_HOTEL:
        grad(0, 0, TOP_W, 158, C(250, 236, 214), C(236, 214, 186));
        for (int i = 0; i < 6; i++) { rect(20 + i * 68.f, 0, 10, 158, C(224, 190, 120)); circle(25 + i * 68.f, 30, 7, C(255, 220, 120)); }
        for (int i = 0; i < 9; i++) { line(200, 4, 160 + i * 10.f, 30, 1, C(255, 240, 180)); circle(160 + i * 10.f, 32, 2.5f, C(255, 255, 230)); }
        ol(120, 40, 160, 24, RGB{60, 40, 80}, 6);
        textOutline("GRAND STARLIGHT HOTEL", 200, 44, 0.4f, C(255, 220, 120), C(UI_INK), 1);
        rect(0, 156, TOP_W, 2, C(UI_INK));
        for (int r = 0; r < 4; r++) for (int k = 0; k < 10; k++) rect(k * 40.f + (r % 2) * 20, 158 + r * 21.f, 40, 21, C((r + k) % 2 ? RGB{250, 246, 240} : RGB{40, 36, 44}));
        break;
    case R_ISLAND:
        grad(0, 0, TOP_W, 110, C(120, 210, 255), C(200, 240, 255)); circle(330, 36, 18, C(255, 240, 160));
        grad(0, 110, TOP_W, 60, C(40, 200, 210), C(90, 230, 220)); for (int k = 0; k < 6; k++) rect(20 + k * 64 + 10 * std::sin(t + k), 130 + (k % 3) * 10.f, 30, 2, C(255, 255, 255, 160));
        rect(0, 168, TOP_W, 72, C(250, 230, 180)); for (int k = 0; k < 2; k++) { float x = 40 + k * 320.f; line(x, 168, x + 10, 70, 5, C(140, 100, 60)); for (int f = 0; f < 5; f++) { float a = f * 1.25f; line(x + 10, 70, x + 10 + std::cos(a) * 34, 70 + std::sin(a) * 16 + 8, 4, C(60, 170, 80)); } }
        break;
    case R_ISLANDTOWN:
        grad(0, 0, TOP_W, 158, C(255, 190, 140), C(255, 230, 190));
        for (int k = 0; k < 3; k++) { float x = 30 + k * 130.f; rect(x, 80, 90, 70, C(200, 150, 90)); for (int s = 0; s < 4; s++) line(x - 10 + s * 3, 80 - s * 8, x + 100 - s * 3, 80 - s * 8, 6, C(220, 190, 110)); rect(x + 35, 110, 20, 40, C(110, 70, 40)); }
        for (int k = 0; k < 8; k++) circle(20 + k * 52.f, 40 + 4 * std::sin(t * 2 + k), 4, C(k % 2 ? RGB{255, 120, 80} : RGB{255, 220, 90}));
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(240, 215, 160));
        break;
    case R_SLOPES:
        grad(0, 0, TOP_W, 160, C(150, 200, 255), C(230, 245, 255));
        for (int k = 0; k < 3; k++) { float x = k * 150.f - 20; quad(x, 160, x + 110, 40 + k * 10, x + 220, 160, x + 110, 160, C(240, 248, 255)); quad(x + 80, 70 + k * 10, x + 110, 40 + k * 10, x + 140, 70 + k * 10, x + 110, 80 + k * 10, C(255, 255, 255)); }
        for (int k = 0; k < 20; k++) circle(std::fmod(k * 53.f + t * 10, 400.f), std::fmod(k * 37.f + t * 30, 240.f), 1.5f, C(255, 255, 255));
        rect(0, 160, TOP_W, 80, C(250, 252, 255));
        break;
    case R_LODGE:
        grad(0, 0, TOP_W, 158, C(150, 95, 60), C(120, 75, 45)); for (int k = 0; k < 7; k++) rect(0, 12 + k * 21.f, TOP_W, 2, C(100, 60, 35));
        rect(60, 50, 100, 60, C(230, 245, 255)); rect(108, 50, 4, 60, C(110, 70, 40)); rect(60, 78, 100, 4, C(110, 70, 40));
        rect(250, 30, 110, 10, C(90, 60, 40)); ol(270, 40, 70, 26, RGB{60, 90, 60}, 4);
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(170, 40, 50)); for (int k = 0; k < 10; k++) rect(k * 40.f, 158, 20, 82, C(150, 30, 45));
        break;
    case R_BOULEVARD:
        grad(0, 0, TOP_W, 158, C(200, 225, 255), C(250, 240, 230));
        for (int k = 0; k < 4; k++) { float x = 4 + k * 100.f; rect(x, 30, 92, 126, C(245, 235, 215)); rect(x, 26, 92, 6, C(90, 110, 130)); for (int w = 0; w < 3; w++) for (int h = 0; h < 2; h++) rect(x + 10 + w * 28.f, 44 + h * 34.f, 16, 22, C(120, 150, 180));
            for (int s = 0; s < 6; s++) rect(x + 4 + s * 14.f, 112, 14, 10, C(s % 2 ? RGB{220, 60, 80} : RGB{255, 255, 255})); }
        rect(0, 156, TOP_W, 2, C(UI_INK)); for (int q = 0; q < 4; q++) for (int k = 0; k < 20; k++) rect(k * 20.f + (q % 2) * 10, 158 + q * 21.f, 19, 20, C(190, 185, 180));
        break;
    case R_TOWER:
        grad(0, 0, TOP_W, 158, C(60, 50, 120), C(250, 160, 150));
        for (int k = 0; k < 12; k++) rect(k * 34.f, 120 - (k * 37 % 50), 30, 40 + (k * 37 % 50), C(70, 60, 110));
        { float cx = 200; line(cx - 60, 156, cx, 20, 3, C(80, 70, 100)); line(cx + 60, 156, cx, 20, 3, C(80, 70, 100)); for (int k = 0; k < 6; k++) { float y = 40 + k * 20.f, w = (y - 20) * 60 / 136; line(cx - w, y, cx + w, y, 2, C(80, 70, 100)); line(cx - w, y, cx + w * 0.6f, y + 20, 1, C(80, 70, 100)); }
          for (int k = 0; k < 10; k++) circle(cx - 50 + k * 11 + std::sin(t + k) * 2, 30 + k * 12.f, 1.5f, C(255, 240, 160, (uint8_t)(150 + 100 * std::sin(t * 4 + k)))); }
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(160, 150, 140));
        break;
    case R_HAUNTED:
        grad(0, 0, TOP_W, 158, C(40, 30, 60), C(90, 70, 110));
        circle(340, 40, 18, C(240, 240, 210));
        ol(110, 18, 180, 26, RGB{60, 40, 70}, 8); textOutline("HOLLOW OAK MANOR", 200, 21, 0.45f, C(200, 255, 200), C(UI_INK), 1);
        for (int i = 0; i < 3; i++) { float x = 60 + i * 120.f; rect(x, 60, 40, 56, C(30, 24, 40)); rect(x + 3, 63, 34, 50, C(255, 220, 120, 60 + (int)(40 * std::sin(t * 2 + i)))); }
        for (int i = 0; i < 2; i++) { float gx = 100 + i * 200 + 20 * std::sin(t + i), gy = 120 + 6 * std::sin(t * 2 + i); circle(gx, gy, 10, C(255, 255, 255, 120)); rect(gx - 10, gy, 20, 12, C(255, 255, 255, 120)); }
        rect(0, 156, TOP_W, 2, C(UI_INK));
        for (int q = 0; q < 4; q++) for (int k = 0; k < 10; k++) rect(k * 40.f + (q % 2) * 20, 158 + q * 21.f, 40, 21, C((q + k) % 2 ? RGB{90, 60, 50} : RGB{70, 46, 40}));
        break;
    case R_FRAT:
        grad(0, 0, TOP_W, 158, C(120, 160, 230), C(200, 220, 255));
        ol(90, 18, 220, 26, RGB{40, 60, 140}, 8); textOutline("ALPHA BETA KAPPA", 200, 21, 0.5f, C(255, 255, 255), C(UI_INK), 1);
        for (int i = 0; i < 12; i++) circle(20 + i * 33.f, 70 + 8 * std::sin(t * 3 + i), 4, C(i % 3 == 0 ? RGB{255, 80, 160} : i % 3 == 1 ? RGB{80, 220, 255} : RGB{255, 230, 80}));
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(110, 180, 90));
        break;
    case R_CORNER: case R_THRIFT: case R_TENNIS: {
        bool tn = room == R_TENNIS;
        grad(0, 0, TOP_W, 158, tn ? C(170, 220, 255) : C(250, 240, 220), tn ? C(220, 245, 255) : C(236, 220, 200));
        const char* nm = room == R_CORNER ? "QUIK STOP" : tn ? "WESTBROOK TENNIS CLUB" : "SECOND CHANCE THRIFT";
        ol(100, 18, 200, 26, room == R_CORNER ? RGB{230, 60, 60} : tn ? RGB{60, 170, 90} : RGB{170, 110, 200}, 8);
        textOutline(nm, 200, 21, 0.45f, C(255, 255, 255), C(UI_INK), 1);
        rect(0, 156, TOP_W, 2, C(UI_INK));
        if (tn) { rect(0, 158, TOP_W, 82, C(60, 140, 200)); rect(20, 166, 360, 2, C(255, 255, 255)); rect(199, 158, 2, 82, C(255, 255, 255)); }
        else for (int q = 0; q < 4; q++) for (int k = 0; k < 10; k++) rect(k * 40.f + (q % 2) * 20, 158 + q * 21.f, 40, 21, C((q + k) % 2 ? RGB{250, 246, 240} : RGB{200, 180, 160}));
        break;
    }
    case R_MARKET:
        grad(0, 0, TOP_W, 158, C(236, 250, 240), C(214, 240, 226));
        for (int i = 0; i < 10; i++) rect(i * 40.f, 0, 20, 12, C(i % 2 ? RGB{255, 140, 180} : RGB{255, 250, 252}));   // striped awning trim
        ol(120, 18, 160, 26, RGB{255, 110, 170}, 8);
        textOutline("SUGAR MARKET", 200, 21, 0.55f, C(255, 255, 255), C(UI_INK), 1);
        heart(132, 31, 4, C(255, 255, 255)); heart(268, 31, 4, C(255, 255, 255));
        for (int i = 0; i < 4; i++) { float x = 20 + i * 100.f; ol(x, 56, 70, 18, RGB{255, 250, 236}, 3); static const char* SIGN[4] = {"PRODUCE", "PANTRY", "BEAUTY", "FITS"}; text(SIGN[i], x + 35, 58, 0.36f, C(UI_INK), 1); }
        for (int i = 0; i < 8; i++) { float x = 14 + i * 50.f; circle(x, 96, 3, C(255, 255, 220)); line(x, 78, x, 93, 1, C(180, 180, 180)); }
        rect(0, 156, TOP_W, 2, C(UI_INK));
        for (int r = 0; r < 4; r++) for (int k = 0; k < 10; k++) rect(k * 40.f + (r % 2) * 20, 158 + r * 21.f, 40, 21, C((r + k) % 2 ? RGB{255, 250, 252} : RGB{214, 240, 226}));
        break;
    case R_CHAPEL:
        grad(0, 0, TOP_W, 158, C(255, 236, 244), C(250, 220, 236));
        for (int i = 0; i < 3; i++) { float x = 70 + i * 130.f; ol(x - 18, 20, 36, 70, RGB{190, 220, 255}, 18); line(x, 22, x, 88, 1.5f, C(UI_INK)); line(x - 16, 50, x + 16, 50, 1.5f, C(UI_INK)); }
        for (int i = 0; i < 16; i++) heart(12 + i * 25.f, 108 + (i % 2) * 8, 4, C(255, 120, 170, 160));
        ol(130, 4, 140, 14, RGB{255, 90, 150}, 5);
        textOutline("LOVEBIRD CHAPEL", 200, 3, 0.36f, C(255, 255, 255), C(UI_INK), 1);
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(255, 250, 252));
        quad(170, 240, 230, 240, 212, 158, 188, 158, C(220, 40, 80));
        for (int i = 0; i < 10; i++) { float px = std::fmod(i * 47.f + t * 12, 400.f), py = std::fmod(i * 29.f + t * 18, 158.f); ellipse(px, py, 4, 2.5f, C(255, 170, 200, 180)); }
        break;
    case R_NEIGHBOR:
        grad(0, 0, TOP_W, 158, C(220, 236, 214), C(200, 222, 196));
        for (int i = 0; i < 12; i++) rect(i * 36.f, 0, 2, 158, C(255, 255, 255, 80));
        ol(150, 40, 100, 70, RGB{180, 220, 250}, 2); line(200, 40, 200, 110, 2, C(UI_INK)); line(150, 75, 250, 75, 2, C(UI_INK));
        ol(30, 50, 46, 34, RGB{255, 240, 200}, 1); circle(53, 67, 9, C(255, 140, 120));
        rect(0, 156, TOP_W, 2, C(UI_INK)); rect(0, 158, TOP_W, 82, C(170, 130, 100));
        for (int k = 0; k < 20; k++) rect(k * 20.f, 158, 1, 82, C(140, 100, 80));
        ellipse(200, 214, 220, 30, C(230, 200, 220));
        break;
    }
}

// the objects themselves (drawn over the backdrop, behind the people)
static void drawSpot(const Spot& s, float t) {
    const float x = s.x, y = s.y;
    switch (s.kind) {
    case SP_RACK:
        line(x - 22, y - 50, x + 22, y - 50, 3, C(UI_INK)); line(x - 20, y - 50, x - 20, y, 3, C(UI_INK)); line(x + 20, y - 50, x + 20, y, 3, C(UI_INK));
        for (int i = 0; i < 5; i++) { static const RGB R[5] = {{255, 46, 140}, {84, 132, 204}, {255, 220, 60}, {150, 96, 230}, {255, 255, 255}}; ol(x - 18 + i * 8.f, y - 47, 6, 26, R[i], 1); }
        break;
    case SP_SHOES: ol(x - 26, y - 60, 52, 60, RGB{255, 240, 246}, 2); for (int r = 0; r < 3; r++) for (int k = 0; k < 3; k++) { rect(x - 24, y - 44 + r * 18.f, 48, 2, C(UI_INK)); ellipse(x - 14 + k * 14.f, y - 48 + r * 18.f, 10, 5, C(k == 0 ? RGB{255, 46, 140} : k == 1 ? RGB{30, 26, 30} : RGB{255, 255, 255})); } break;
    case SP_ACCESS: ol(x - 16, y - 34, 32, 34, RGB{255, 255, 255}, 3); circle(x - 6, y - 40, 7, C(UI_INK)); circle(x - 6, y - 40, 5.5f, C(255, 90, 160)); sparkle(x + 8, y - 42, 5, C(GOLD)); ellipse(x, y - 24, 18, 6, C(GOLD)); break;
    case SP_MIRROR: ol(x - 18, y - 74, 36, 74, RGB{200, 236, 255}, 14); rect(x - 12, y - 66, 3, 50, C(255, 255, 255, 140)); sparkle(x + 8, y - 60, 4 + std::sin(t * 3), C(255, 255, 255)); break;
    case SP_MAKEUP: ol(x - 26, y - 30, 52, 30, RGB{255, 214, 236}, 3); for (int i = 0; i < 4; i++) { rect(x - 18 + i * 10.f, y - 42, 5, 12, C(UI_INK)); rect(x - 17 + i * 10.f, y - 41, 3, 10, C(i % 2 ? RGB{200, 30, 80} : RGB{255, 140, 180})); } break;
    case SP_NAILS: ol(x - 22, y - 26, 44, 26, RGB{240, 220, 255}, 3); for (int i = 0; i < 5; i++) { ol(x - 16 + i * 8.f, y - 36, 4, 9, RGB{static_cast<uint8_t>(150 + i * 20), 80, 220}, 1); } break;
    case SP_FOODSTALL: ol(x - 30, y - 34, 60, 34, RGB{255, 200, 120}, 3); ellipse(x - 12, y - 40, 14, 8, C(240, 180, 90)); circle(x + 12, y - 42, 6, C(240, 80, 60)); break;
    case SP_PHOTOBOOTH: ol(x - 20, y - 78, 40, 78, RGB{255, 46, 140}, 4); ol(x - 14, y - 64, 28, 48, RGB{40, 30, 50}, 2); textOutline("PHOTO", x, y - 76, 0.32f, C(255, 255, 255), C(UI_INK), 1, 0.6f); break;
    case SP_ARCADE: for (int i = 0; i < 2; i++) { float ax = x - 14 + i * 28.f; ol(ax - 11, y - 64, 22, 64, i ? RGB{40, 192, 210} : RGB{255, 46, 140}, 3); ol(ax - 8, y - 56, 16, 14, RGB{20, 20, 40}, 1); rect(ax - 6, y - 52, 12 * (0.5f + 0.5f * std::sin(t * 5 + i)), 6, C(120, 255, 160)); circle(ax - 3, y - 34, 2, C(255, 230, 60)); } break;
    case SP_CLAW: ol(x - 20, y - 76, 40, 76, RGB{255, 230, 60}, 4); ol(x - 15, y - 70, 30, 40, RGB{200, 240, 255}, 2); for (int i = 0; i < 4; i++) circle(x - 9 + i * 6.f, y - 34, 3.4f, C(i % 2 ? RGB{255, 140, 200} : RGB{150, 200, 255})); line(x + std::sin(t) * 8, y - 70, x + std::sin(t) * 8, y - 54, 1.5f, C(UI_INK)); break;
    case SP_FURNITURE: ol(x - 40, y - 26, 80, 22, RGB{255, 160, 200}, 8); ol(x - 40, y - 40, 14, 36, RGB{255, 160, 200}, 5); ol(x + 26, y - 40, 14, 36, RGB{255, 160, 200}, 5); ol(x + 46, y - 60, 4, 60, RGB{200, 170, 120}, 1); ellipse(x + 48, y - 64, 20, 12, C(255, 240, 180)); break;
    case SP_ESPRESSO: ol(x - 28, y - 32, 56, 32, RGB{120, 80, 60}, 3); ol(x - 14, y - 52, 22, 20, RGB{200, 200, 210}, 3); circle(x + 18, y - 38, 4, C(255, 255, 255)); break;
    case SP_STAGE: ol(x - 36, y - 10, 72, 10, RGB{150, 96, 230}, 2); line(x, y - 10, x, y - 52, 2, C(UI_INK)); circle(x, y - 54, 4, C(UI_INK)); sparkle(x + 20, y - 60, 4 + std::sin(t * 4), C(255, 230, 60)); break;
    case SP_DISPLAY: ol(x - 32, y - 46, 64, 46, RGB{200, 240, 255}, 4); for (int r = 0; r < 2; r++) for (int k = 0; k < 4; k++) { float cx = x - 22 + k * 14.f, cy = y - 34 + r * 18.f; quad(cx - 4, cy, cx + 4, cy, cx + 3, cy + 6, cx - 3, cy + 6, C(220, 170, 120)); circle(cx, cy - 1, 4.5f, C(k % 2 ? RGB{255, 160, 200} : RGB{255, 250, 240})); } break;
    case SP_DECORATE: ol(x - 34, y - 18, 68, 8, RGB{255, 255, 255}, 2); line(x - 28, y - 10, x - 28, y, 2, C(UI_INK)); line(x + 28, y - 10, x + 28, y, 2, C(UI_INK)); for (int i = 0; i < 3; i++) circle(x - 16 + i * 16.f, y - 23, 5, C(i == 1 ? RGB{255, 120, 180} : RGB{255, 240, 220})); break;
    case SP_OVEN: ol(x - 24, y - 50, 48, 50, RGB{220, 220, 230}, 3); ol(x - 18, y - 38, 36, 24, RGB{60, 40, 40}, 2); rect(x - 16, y - 30, 32, 10, C(255, 160, 60, (uint8_t)(140 + 80 * std::sin(t * 3)))); break;
    case SP_PICNIC: quad(x - 34, y - 6, x + 34, y - 6, x + 40, y + 8, x - 40, y + 8, C(UI_INK)); quad(x - 32, y - 5, x + 32, y - 5, x + 38, y + 7, x - 38, y + 7, C(255, 90, 110)); for (int i = 0; i < 5; i++) line(x - 26 + i * 13.f, y - 5, x - 30 + i * 15.f, y + 7, 2, C(255, 255, 255)); ol(x + 10, y - 16, 16, 10, RGB{200, 150, 90}, 3); break;
    case SP_POND: ellipse(x, y + 6, 74, 22, C(UI_INK)); ellipse(x, y + 6, 70, 18, C(110, 190, 240)); ellipse(x - 10 + std::sin(t) * 6, y + 2, 10, 6, C(255, 255, 255)); circle(x - 4 + std::sin(t) * 6, y - 2, 3, C(255, 255, 255)); break;
    case SP_SWINGS: line(x - 30, y - 70, x + 30, y - 70, 3, C(UI_INK)); line(x - 30, y - 70, x - 36, y, 3, C(UI_INK)); line(x + 30, y - 70, x + 36, y, 3, C(UI_INK)); for (int i = 0; i < 2; i++) { float sw = std::sin(t * 2 + i) * 6, sx = x - 12 + i * 24.f; line(sx, y - 70, sx + sw, y - 24, 1, C(UI_INK)); ol(sx + sw - 6, y - 25, 12, 3, RGB{255, 46, 140}, 1); } break;
    case SP_SANDBOX: ol(x - 32, y - 10, 64, 12, RGB{150, 100, 70}, 2); ellipse(x, y - 8, 56, 8, C(250, 220, 150)); circle(x + 10, y - 13, 4, C(255, 60, 90)); break;
    case SP_HOOP: line(x, y, x, y - 86, 3, C(UI_INK)); ol(x - 14, y - 100, 28, 18, RGB{255, 255, 255}, 1); ellipse(x, y - 80, 16, 4, C(255, 120, 40)); circle(x + 18, y - 4, 5, C(255, 140, 40)); break;
    case SP_ICECREAM: ol(x - 24, y - 34, 48, 30, RGB{255, 255, 255}, 4); circle(x - 14, y - 2, 5, C(UI_INK)); circle(x + 14, y - 2, 5, C(UI_INK)); line(x + 20, y - 34, x + 20, y - 60, 2, C(UI_INK)); tri(x - 4, y - 60, x + 40, y - 60, x + 20, y - 72, C(255, 160, 200)); circle(x, y - 42, 6, C(255, 200, 220)); tri(x - 5, y - 40, x + 5, y - 40, x, y - 30, C(220, 170, 120)); break;
    case SP_DINNERTABLE: ol(x - 30, y - 22, 60, 6, RGB{255, 255, 255}, 2); line(x, y - 16, x, y, 3, C(UI_INK)); rect(x - 2, y - 32, 4, 10, C(255, 250, 230)); circle(x, y - 34, 2 + std::sin(t * 8) * 0.5f, C(255, 200, 60)); break;
    case SP_BOOTH: ol(x - 40, y - 40, 14, 40, RGB{255, 60, 100}, 4); ol(x + 26, y - 40, 14, 40, RGB{255, 60, 100}, 4); ol(x - 24, y - 26, 48, 6, RGB{240, 240, 240}, 2); line(x, y - 20, x, y, 3, C(UI_INK)); break;
    case SP_SHAKE: ol(x - 40, y - 30, 80, 30, RGB{255, 255, 255}, 3); for (int i = 0; i < 3; i++) { float sx = x - 22 + i * 22.f; ol(sx - 4, y - 46, 8, 14, RGB{255, 180, 210}, 2); line(sx + 1, y - 46, sx + 4, y - 54, 1.2f, C(255, 60, 100)); } break;
    case SP_JUKEBOX: ol(x - 18, y - 62, 36, 62, RGB{255, 168, 36}, 14); ol(x - 12, y - 48, 24, 20, RGB{255, 255, 220}, 4); for (int i = 0; i < 3; i++) rect(x - 10 + i * 8.f, y - 22, 4, 16, C(i % 2 ? RGB{255, 60, 100} : RGB{60, 200, 255}, (uint8_t)(150 + 100 * std::sin(t * 4 + i)))); break;
    case SP_CONVEYOR: { ol(x - 90, y - 26, 180, 10, RGB{120, 120, 130}, 4); for (int i = 0; i < 6; i++) { float px = x - 84 + std::fmod(i * 30.f + t * 18, 170.f); ellipse(px, y - 30, 16, 5, C(255, 255, 255)); circle(px - 3, y - 33, 3, C(255, 120, 90)); circle(px + 3, y - 33, 3, C(250, 250, 250)); } break; }
    case SP_SUSHICLASS: ol(x - 30, y - 30, 60, 30, RGB{220, 180, 140}, 3); ol(x - 12, y - 42, 24, 12, RGB{255, 255, 255}, 4); break;
    case SP_MAILBOX: line(x, y, x, y - 30, 3, C(UI_INK)); ol(x - 10, y - 44, 20, 14, RGB{40, 120, 220}, 6); line(x + 10, y - 44, x + 10, y - 54, 2, C(255, 46, 140)); break;
    case SP_REALTY: line(x, y, x, y - 34, 2.5f, C(UI_INK)); ol(x - 18, y - 46, 36, 18, RGB{255, 255, 255}, 2); textOutline("SALE", x, y - 45, 0.32f, C(255, 46, 140), C(255, 255, 255), 1, 0.4f); break;
    case SP_LEMONADE: ol(x - 22, y - 24, 44, 24, RGB{255, 240, 120}, 2); line(x - 20, y - 24, x - 20, y - 44, 2, C(UI_INK)); line(x + 20, y - 24, x + 20, y - 44, 2, C(UI_INK)); ol(x - 24, y - 50, 48, 8, RGB{255, 220, 60}, 2); textOutline("25c", x, y - 21, 0.32f, C(255, 255, 255), C(UI_INK), 1, 0.5f); break;
    case SP_NCOUCH: ol(x - 44, y - 26, 88, 22, RGB{150, 200, 230}, 8); ol(x - 44, y - 42, 88, 18, RGB{130, 180, 214}, 6); break;
    case SP_NFRIDGE: ol(x - 16, y - 70, 32, 70, RGB{240, 240, 248}, 4); line(x - 16, y - 46, x + 16, y - 46, 1.5f, C(UI_INK)); rect(x + 8, y - 62, 2, 10, C(UI_INK)); circle(x - 6, y - 58, 3, C(255, 90, 160)); break;
    case SP_RECEPTION: ol(x - 34, y - 34, 68, 34, RGB{240, 248, 250}, 3); ol(x - 10, y - 46, 20, 12, RGB{40, 50, 70}, 1); rect(x - 7, y - 43, 14, 6, C(120, 230, 200)); break;
    case SP_EXAMBED: ol(x - 36, y - 26, 72, 10, RGB{180, 220, 240}, 3); line(x - 30, y - 16, x - 30, y, 2, C(UI_INK)); line(x + 30, y - 16, x + 30, y, 2, C(UI_INK)); ol(x - 34, y - 34, 16, 8, RGB{255, 255, 255}, 3); ol(x + 40, y - 70, 6, 70, RGB{200, 200, 210}, 1); ol(x + 34, y - 78, 18, 10, RGB{120, 230, 160}, 2); break;
    case SP_NURSERYWIN: ol(x - 40, y - 86, 80, 50, RGB{220, 240, 255}, 3); for (int i = 0; i < 3; i++) { ol(x - 30 + i * 22.f, y - 52, 16, 10, RGB{255, 255, 255}, 2); circle(x - 22 + i * 22.f, y - 56, 4, C(i % 2 ? RGB{150, 200, 255} : RGB{255, 180, 210})); } break;
    case SP_PETPEN: for (int i = 0; i < 2; i++) { float px = x - 22 + i * 44.f; ol(px - 18, y - 34, 36, 34, RGB{255, 230, 190}, 3); for (int k = 0; k < 5; k++) rect(px - 16 + k * 8.f, y - 34, 2, 34, C(UI_INK)); circle(px, y - 12, 6, C(i ? RGB{240, 240, 240} : RGB{200, 140, 80})); circle(px - 3, y - 13, 1, C(UI_INK)); circle(px + 3, y - 13, 1, C(UI_INK)); } break;
    case SP_PETSUPPLY: ol(x - 30, y - 64, 60, 64, RGB{255, 240, 220}, 2); for (int r = 0; r < 3; r++) { rect(x - 28, y - 44 + r * 20.f, 56, 2, C(UI_INK)); for (int k = 0; k < 3; k++) ol(x - 22 + k * 16.f, y - 58 + r * 20.f, 10, 12, RGB{static_cast<uint8_t>(r == 0 ? 255 : 120), static_cast<uint8_t>(k == 1 ? 200 : 120), 140}, 2); } break;
    case SP_GROOM: ol(x - 32, y - 30, 64, 30, RGB{200, 240, 255}, 3); ol(x - 26, y - 44, 52, 14, RGB{255, 255, 255}, 6); for (int i = 0; i < 4; i++) circle(x - 18 + i * 12.f, y - 50 - (i % 2) * 4, 3, C(255, 255, 255, 220)); break;
    case SP_BOOKSHELF: ol(x - 30, y - 80, 60, 80, RGB{150, 90, 60}, 2); for (int r = 0; r < 4; r++) for (int k = 0; k < 7; k++) rect(x - 26 + k * 7.5f, y - 76 + r * 19.f, 6, 15, C(k % 3 == 0 ? RGB{200, 60, 80} : k % 3 == 1 ? RGB{60, 140, 200} : RGB{240, 200, 70})); break;
    case SP_STUDYTABLE: ol(x - 44, y - 24, 88, 6, RGB{180, 120, 80}, 2); line(x - 38, y - 18, x - 38, y, 2, C(UI_INK)); line(x + 38, y - 18, x + 38, y, 2, C(UI_INK)); ol(x - 20, y - 30, 16, 6, RGB{255, 255, 255}, 1); ol(x + 14, y - 40, 4, 16, RGB{60, 60, 70}, 1); ellipse(x + 16, y - 42, 14, 6, C(140, 220, 120)); break;
    case SP_STORYRUG: ellipse(x, y - 2, 90, 20, C(UI_INK)); ellipse(x, y - 2, 86, 16, C(255, 200, 80)); ellipse(x, y - 2, 60, 10, C(255, 120, 160)); ol(x + 30, y - 40, 22, 34, RGB{120, 200, 240}, 6); break;
    case SP_TREADMILL: for (int i = 0; i < 2; i++) { float tx = x - 20 + i * 40.f; ol(tx - 16, y - 10, 32, 8, RGB{70, 70, 80}, 2); line(tx + 12, y - 10, tx + 14, y - 44, 2, C(UI_INK)); ol(tx + 6, y - 50, 14, 8, RGB{40, 220, 140}, 2); } break;
    case SP_WEIGHTS: ol(x - 30, y - 50, 60, 6, RGB{120, 120, 130}, 1); for (int i = 0; i < 4; i++) { circle(x - 22 + i * 15.f, y - 30, 7, C(UI_INK)); circle(x - 22 + i * 15.f, y - 30, 5, C(i % 2 ? RGB{255, 90, 60} : RGB{60, 60, 70})); } line(x - 26, y - 20, x - 26, y, 2, C(UI_INK)); line(x + 26, y - 20, x + 26, y, 2, C(UI_INK)); break;
    case SP_YOGAMAT: for (int i = 0; i < 2; i++) { quad(x - 30 + i * 32.f, y - 2, x - 6 + i * 32.f, y - 2, x - 2 + i * 32.f, y + 8, x - 34 + i * 32.f, y + 8, C(i ? RGB{150, 96, 230} : RGB{40, 192, 210})); } break;
    case SP_LAPPOOL: ellipse(x, y + 2, 84, 22, C(UI_INK)); ellipse(x, y + 2, 80, 18, C(80, 180, 240)); for (int i = 0; i < 3; i++) line(x - 34, y - 4 + i * 6.f, x + 34, y - 4 + i * 6.f, 1, C(255, 255, 255, 140)); break;
    case SP_FRONTDESK: ol(x - 40, y - 34, 80, 34, RGB{120, 70, 50}, 3); rect(x - 38, y - 28, 76, 3, C(255, 210, 110)); circle(x + 20, y - 40, 4, C(255, 210, 110)); break;
    case SP_HOTELPOOL: { ellipse(x, y + 4, 110, 24, C(UI_INK)); ellipse(x, y + 4, 106, 20, C(70, 200, 230)); for (int i = 0; i < 4; i++) circle(x - 40 + i * 26.f + std::sin(t + i) * 3, y + 2, 2, C(255, 255, 255, 180)); ol(x + 46, y - 60, 3, 56, RGB{255, 255, 255}, 1); tri(x + 30, y - 58, x + 66, y - 58, x + 48, y - 72, C(255, 120, 160)); break; }
    case SP_LOUNGE: ol(x - 30, y - 30, 60, 30, RGB{30, 26, 34}, 4); for (int i = 0; i < 8; i++) rect(x - 26 + i * 7.f, y - 34, 5, 6, C(i % 2 ? RGB{30, 26, 34} : RGB{250, 250, 250})); ol(x + 30, y - 46, 6, 46, RGB{200, 160, 60}, 1); break;
    case SP_ALTAR: ol(x - 36, y - 18, 72, 18, RGB{255, 255, 255}, 3); for (int i = 0; i < 9; i++) { float a = 3.14159f * i / 8; float fx = x - std::cos(a) * 46, fy = y - 30 - std::sin(a) * 50; circle(fx, fy, 6, C(UI_INK)); circle(fx, fy, 5, C(i % 2 ? RGB{255, 140, 190} : RGB{255, 255, 255})); } break;
    case SP_BANQUET: ol(x - 40, y - 24, 80, 8, RGB{255, 255, 255}, 2); for (int i = 0; i < 3; i++) circle(x - 22 + i * 22.f, y - 28, 5, C(i == 1 ? RGB{120, 70, 40} : RGB{255, 200, 120})); ol(x - 6, y - 48, 12, 20, RGB{255, 240, 250}, 3); break;
    case SP_PRODUCE:   // fruit & veg crates
        for (int k = 0; k < 3; k++) { ol(x - 32 + k * 22.f, y - 22, 20, 22, RGB{200, 150, 100}, 2); static const RGB F[3] = {{230, 50, 60}, {255, 180, 40}, {120, 200, 80}}; for (int j = 0; j < 4; j++) circle(x - 26 + k * 22.f + (j % 2) * 8, y - 26 + (j / 2) * 5, 4, C(F[k])); }
        break;
    case SP_AISLES:   // shelves of groceries
        ol(x - 30, y - 70, 60, 70, RGB{250, 250, 246}, 2);
        for (int r = 0; r < 3; r++) { rect(x - 28, y - 48 + r * 22.f, 56, 2, C(UI_INK)); for (int k = 0; k < 5; k++) { static const RGB P[5] = {{255, 120, 160}, {120, 190, 255}, {255, 220, 90}, {160, 220, 140}, {200, 150, 230}}; ol(x - 26 + k * 11.f, y - 64 + r * 22.f, 8, 14, P[(k + r) % 5], 1); } }
        break;
    case SP_BEAUTYAISLE:   // pink makeup shelf
        ol(x - 30, y - 64, 60, 64, RGB{255, 230, 242}, 3);
        for (int r = 0; r < 3; r++) { rect(x - 28, y - 44 + r * 20.f, 56, 2, C(UI_INK)); for (int k = 0; k < 6; k++) { static const RGB M[6] = {{220, 40, 100}, {255, 150, 190}, {200, 120, 90}, {150, 60, 160}, {255, 90, 140}, {240, 200, 210}}; rect(x - 25 + k * 9.f, y - 58 + r * 20.f, 4, 12, C(M[(k + r * 2) % 6])); } }
        sparkle(x + 24, y - 66, 4 + std::sin(t * 4), C(255, 255, 255));
        break;
    case SP_FITRACK:   // cute fits on a rack
        line(x - 26, y - 56, x + 26, y - 56, 3, C(UI_INK)); line(x - 24, y - 56, x - 24, y, 3, C(UI_INK)); line(x + 24, y - 56, x + 24, y, 3, C(UI_INK));
        for (int i = 0; i < 6; i++) { static const RGB R[6] = {{255, 170, 200}, {250, 240, 220}, {120, 50, 60}, {170, 210, 250}, {30, 30, 36}, {255, 150, 80}}; ol(x - 22 + i * 8.f, y - 53, 6, 28, R[i], 1); }
        textOutline("CUTE FITS", x, y - 70, 0.34f, C(255, 110, 170), C(UI_INK), 1, 0.5f);
        break;
    case SP_CHECKOUT:   // register counter
        ol(x - 30, y - 30, 60, 30, RGB{240, 236, 250}, 3); ol(x - 6, y - 46, 22, 16, RGB{60, 60, 80}, 2); rect(x - 2, y - 42, 14, 6, C(120, 255, 180));
        ol(x - 26, y - 36, 16, 6, RGB{255, 200, 120}, 1);
        break;
    case SP_LOUNGER: case SP_HOTTUB: ol(x - 34, y - 14, 68, 14, s.kind == SP_HOTTUB ? RGB{120, 200, 230} : RGB{255, 255, 255}, 4); if (s.kind == SP_LOUNGER) { line(x + 30, y - 40, x + 30, y, 2, C(150, 110, 70)); quad(x + 6, y - 40, x + 54, y - 40, x + 44, y - 50, x + 16, y - 50, C(255, 120, 150)); } else for (int k = 0; k < 4; k++) circle(x - 20 + k * 13.f, y - 18 - std::fmod(t * 8 + k * 3, 10.f), 2, C(255, 255, 255, 160)); break;
    case SP_LAGOON: case SP_SNOWFIELD: ellipse(x, y - 4, 70, 14, s.kind == SP_LAGOON ? C(60, 210, 220) : C(214, 228, 245)); if (s.kind == SP_SNOWFIELD) { circle(x + 20, y - 14, 10, C(150, 170, 200)); circle(x + 20, y - 14, 9, C(255, 255, 255)); circle(x + 20, y - 28, 7, C(150, 170, 200)); circle(x + 20, y - 28, 6, C(255, 255, 255)); quad(x + 21, y - 29, x + 28, y - 28, x + 21, y - 27, x + 21, y - 28, C(255, 140, 40)); circle(x + 18, y - 29, 1, C(UI_INK)); } break;
    case SP_TIKIBAR: case SP_SOUVENIR: case SP_CREPECART: case SP_COUTURE: case SP_ARTIST:
        ol(x - 30, y - 34, 60, 34, s.kind == SP_TIKIBAR ? RGB{180, 130, 70} : s.kind == SP_CREPECART ? RGB{255, 200, 220} : s.kind == SP_COUTURE ? RGB{30, 30, 40} : RGB{230, 220, 200}, 4);
        if (s.kind == SP_ARTIST) { rect(x + 30, y - 60, 3, 60, C(120, 80, 50)); rect(x + 18, y - 58, 26, 30, C(255, 255, 255)); circle(x + 31, y - 46, 6, C(255, 180, 160)); }
        else quad(x - 34, y - 34, x + 34, y - 34, x + 26, y - 46, x - 26, y - 46, C(s.kind == SP_TIKIBAR ? RGB{220, 190, 110} : RGB{220, 60, 80}));
        break;
    case SP_SKILIFT: line(x - 40, y - 70, x + 60, y - 110, 1.5f, C(UI_INK)); rect(x + 6, y - 92, 22, 14, C(220, 60, 60)); break;
    case SP_FIREPLACE: ol(x - 30, y - 50, 60, 50, RGB{140, 130, 120}, 4); rect(x - 18, y - 30, 36, 30, C(30, 20, 20)); for (int k = 0; k < 3; k++) ellipse(x - 8 + k * 8.f, y - 10 - 4 * std::sin(t * 9 + k), 8, 16, C(255, 140 + k * 30, 40)); break;
    case SP_HFRONTDESK: ol(x - 34, y - 34, 68, 34, RGB{120, 90, 60}, 3); rect(x - 34, y - 34, 68, 4, C(240, 236, 228)); circle(x + 20, y - 38, 3, C(GOLD)); ol(x - 20, y - 70, 40, 18, RGB{190, 150, 70}, 2); break;
    case SP_HLOUNGE: ol(x - 40, y - 24, 30, 20, RGB{60, 90, 80}, 6); ol(x + 10, y - 24, 30, 20, RGB{60, 90, 80}, 6); circle(x, y - 16, 8, C(240, 236, 228)); break;
    case SP_BELLHOP: line(x - 16, y, x - 16, y - 50, 3, C(190, 150, 70)); line(x + 16, y, x + 16, y - 50, 3, C(190, 150, 70)); line(x - 18, y - 50, x + 18, y - 50, 3, C(190, 150, 70)); ol(x - 12, y - 30, 24, 20, RGB{200, 80, 90}, 3); break;
    case SP_RAINSHOWER: ol(x - 26, y - 90, 52, 90, RGB{220, 236, 245}, 2); rect(x - 10, y - 88, 20, 4, C(180, 180, 190)); for (int k = 0; k < 6; k++) line(x - 8 + k * 3.f, y - 84, x - 10 + k * 4.f, y - 40, 1, C(140, 190, 230, 150)); break;
    case SP_SOAKTUB2: ellipse(x, y - 14, 46, 16, C(UI_INK)); ellipse(x, y - 14, 44, 14, C(255, 255, 255)); ellipse(x, y - 18, 36, 7, C(190, 225, 245)); break;
    case SP_HVANITY: ol(x - 40, y - 34, 80, 12, RGB{240, 236, 228}, 2); circle(x - 20, y - 30, 5, C(220, 230, 240)); circle(x + 20, y - 30, 5, C(220, 230, 240)); for (int k = 0; k < 4; k++) rect(x - 34 + k * 6.f, y - 44, 4, 10, C(k % 2 ? RGB{200, 170, 140} : RGB{60, 60, 64})); break;
    case SP_INFINITY: break;   // the pool is part of the backdrop
    case SP_MASSAGE: ol(x - 30, y - 70, 60, 6, RGB{250, 246, 236}, 2); rect(x - 28, y - 64, 4, 40, C(250, 246, 236, 200)); rect(x + 24, y - 64, 4, 40, C(250, 246, 236, 200)); ol(x - 26, y - 24, 52, 10, RGB{255, 255, 255}, 3); break;
    case SP_POOLBAR: ol(x - 26, y - 30, 52, 30, RGB{140, 100, 60}, 3); for (int k = 0; k < 3; k++) { rect(x - 16 + k * 14.f, y - 40, 6, 10, C(k == 0 ? RGB{255, 140, 120} : k == 1 ? RGB{120, 220, 200} : RGB{255, 220, 120})); } break;
    case SP_HOTELBED:   // crisp white linens, four pillows, duvet folded at the foot, matching nightstands
        ol(x - 70, y - 22, 26, 22, RGB{90, 70, 56}, 2); ol(x + 44, y - 22, 26, 22, RGB{90, 70, 56}, 2);
        ol(x - 64, y - 30, 14, 8, RGB{220, 236, 245}, 1); ol(x + 50, y - 30, 14, 8, RGB{220, 236, 245}, 1);
        ol(x - 44, y - 30, 88, 30, RGB{252, 250, 246}, 3);
        for (int k = 0; k < 4; k++) ol(x - 40 + k * 20.f, y - 42, 18, 12, RGB{255, 255, 255}, 3);
        rect(x + 14, y - 30, 30, 30, C(196, 178, 150)); rect(x + 14, y - 30, 30, 3, C(170, 150, 120)); break;
    case SP_ROOMSVC: ol(x - 16, y - 34, 32, 4, RGB{230, 230, 236}, 1); rect(x - 2, y - 30, 4, 30, C(190, 190, 200)); ellipse(x, y - 40, 14, 8, C(220, 220, 230)); circle(x, y - 48, 2, C(190, 190, 200)); break;
    case SP_MINIBAR: ol(x - 16, y - 40, 32, 40, RGB{70, 56, 48}, 3); rect(x - 12, y - 36, 24, 32, C(40, 34, 30)); for (int k = 0; k < 4; k++) rect(x - 10 + k * 6.f, y - 30, 4, 10, C(k % 2 ? RGB{90, 200, 140} : RGB{240, 120, 140})); break;
    case SP_BALCONY: for (int k = 0; k < 5; k++) rect(x - 28 + k * 14.f, y - 28, 2, 28, C(60, 52, 46)); rect(x - 30, y - 30, 60, 3, C(60, 52, 46)); break;
    case SP_LANDMARK: ol(x - 14, y - 70, 28, 70, RGB{230, 220, 200}, 3); tri(x - 18, y - 70, x, y - 92, x + 18, y - 70, C(200, 170, 110)); sparkle(x + 22, y - 80, 5, C(GOLD)); break;
    case SP_TOURDESK: ol(x - 24, y - 30, 48, 30, RGB{60, 110, 170}, 3); rect(x - 2, y - 66, 3, 36, C(UI_INK)); tri(x + 1, y - 66, x + 22, y - 60, x + 1, y - 54, C(255, 200, 60)); break;
    case SP_STREETFOOD: case SP_GIFTSHOP:
        ol(x - 28, y - 30, 56, 30, s.kind == SP_STREETFOOD ? RGB{230, 160, 90} : RGB{150, 120, 200}, 3);
        for (int k = 0; k < 4; k++) tri(x - 32 + k * 16.f, y - 48, x - 16 + k * 16.f, y - 48, x - 24 + k * 16.f, y - 40, C(k % 2 ? RGB{255, 255, 255} : RGB{240, 90, 120}));
        rect(x - 32, y - 52, 64, 4, C(240, 90, 120)); break;
    case SP_BUNK: case SP_DORMBED:
        ol(x - 34, y - 52, 68, 6, RGB{70, 74, 82}, 1); ol(x - 34, y - 22, 68, 10, s.kind == SP_BUNK ? RGB{200, 200, 196} : RGB{60, 90, 160}, 2);
        rect(x - 34, y - 52, 3, 52, C(70, 74, 82)); rect(x + 31, y - 52, 3, 52, C(70, 74, 82)); ol(x - 30, y - 58, 20, 6, RGB{240, 240, 240}, 2); break;
    case SP_PAYPHONE: ol(x - 12, y - 64, 24, 34, RGB{70, 80, 90}, 3); rect(x - 7, y - 58, 14, 10, C(150, 200, 170)); rect(x - 16, y - 54, 5, 16, C(30, 30, 30)); line(x - 13, y - 38, x - 8, y - 30, 1, C(30, 30, 30)); break;
    case SP_JAILYARD: line(x + 20, y, x + 20, y - 70, 3, C(UI_INK)); ol(x + 10, y - 78, 26, 16, RGB{255, 255, 255}, 1); circle(x + 18, y - 60, 6, C(230, 90, 40)); ellipse(x - 10, y - 2, 50, 8, C(160, 150, 120)); break;
    case SP_GUARD: ol(x - 32, y - 32, 64, 32, RGB{60, 70, 90}, 3); ol(x - 10, y - 46, 26, 14, RGB{30, 30, 40}, 2); rect(x - 6, y - 43, 18, 8, C(120, 200, 255)); ol(x + 18, y - 38, 10, 6, RGB{255, 210, 60}, 1); break;
    case SP_STUDYDESK: ol(x - 30, y - 30, 60, 6, RGB{120, 80, 50}, 2); rect(x - 26, y - 24, 4, 24, C(120, 80, 50)); rect(x + 22, y - 24, 4, 24, C(120, 80, 50)); ol(x - 14, y - 40, 22, 10, RGB{250, 250, 240}, 1); line(x + 14, y - 30, x + 18, y - 52, 2, C(60, 60, 60)); circle(x + 18, y - 54, 5, C(255, 230, 140)); break;
    case SP_COMMONS: ol(x - 38, y - 26, 76, 22, RGB{60, 90, 70}, 6); ol(x - 38, y - 40, 12, 36, RGB{60, 90, 70}, 4); ol(x + 26, y - 40, 12, 36, RGB{60, 90, 70}, 4); circle(x - 10, y - 30, 6, C(250, 240, 220)); break;
    case SP_GATE: for (int k = 0; k < 7; k++) rect(x - 30 + k * 10.f, y - 70, 3, 70, C(40, 40, 46)); rect(x - 32, y - 70, 64, 4, C(40, 40, 46)); rect(x - 32, y - 40, 64, 3, C(40, 40, 46)); circle(x, y - 76, 6, C(GOLD)); break;
    case SP_LOOKOUT: rect(x - 36, y - 26, 72, 4, C(80, 70, 100)); for (int k = 0; k < 6; k++) rect(x - 34 + k * 13.f, y - 24, 2, 24, C(80, 70, 100)); break;
    case SP_CREAKY: for (int k = 0; k < 5; k++) rect(x - 30 + k * 10.f, y - 10 - k * 12.f, 24, 6, C(110, 70, 50)); break;
    case SP_SEANCE: ellipse(x, y - 14, 60, 18, C(80, 30, 60)); for (int k = 0; k < 3; k++) { rect(x - 16 + k * 14.f, y - 30, 4, 12, C(250, 240, 220)); circle(x - 14 + k * 14.f, y - 33, 2.5f, C(255, 200, 80)); } break;
    case SP_PARTYFLOOR: for (int k = 0; k < 4; k++) rect(x - 40 + k * 20.f, y - 6, 20, 10, C(((k + (int)(t * 4)) % 2) ? RGB{255, 90, 170} : RGB{90, 200, 255}, 200)); break;
    case SP_FRATCOUCH: ol(x - 34, y - 26, 68, 26, RGB{150, 110, 70}, 4); break;
    case SP_SNACKS: case SP_RACKS: ol(x - 36, y - 60, 72, 60, RGB{200, 170, 140}, 3); for (int k = 0; k < 6; k++) rect(x - 30 + k * 10.f, y - 52, 8, 40, C(k % 2 ? RGB{255, 120, 170} : RGB{120, 200, 240})); break;
    case SP_REGISTER: case SP_DONATE: ol(x - 26, y - 30, 52, 30, RGB{120, 90, 70}, 3); rect(x - 10, y - 44, 20, 14, C(60, 60, 70)); break;
    case SP_COURT: line(x - 40, y - 20, x + 40, y - 20, 2, C(255, 255, 255)); circle(x + 20, y - 30, 4, C(220, 250, 80)); break;
    case SP_BLEACHERS: for (int k = 0; k < 3; k++) rect(x - 36 + k * 6.f, y - 10 - k * 12.f, 72 - k * 12.f, 6, C(200, 200, 210)); break;
    case SP_DANCEFLOOR: for (int r = 0; r < 2; r++) for (int k = 0; k < 4; k++) quad(x - 48 + k * 24.f + r * 4, y - 6 + r * 10.f, x - 26 + k * 24.f + r * 4, y - 6 + r * 10.f, x - 24 + k * 24.f + r * 4, y + 4 + r * 10.f, x - 46 + k * 24.f + r * 4, y + 4 + r * 10.f, C(((r + k + (int)(t * 3)) % 2) ? RGB{255, 90, 170} : RGB{120, 220, 255}, 200)); break;
    default: break;   // doors are part of the backdrop
    }
}
void districtDrawSpots(int room, float t, float mcX) {
    int n; const Spot* sp = spotsIn(room, n);
    for (int i = 0; i < n; i++) {
        if (!g_spotMarkers) { drawSpot(sp[i], t); continue; }
        // painted background: the props are already in the art, so just mark where to tap
        float bob = 2 * std::sin(t * 3 + i), mx = sp[i].x, my = sp[i].y - 34 + bob;
        circle(mx, my, 7, C(UI_INK)); circle(mx, my, 6, C(255, 90, 170)); heart(mx, my, 3.5f, C(255, 255, 255));
    }
    for (int i = 0; i < n; i++) if (std::fabs(sp[i].x - mcX) < 34) {   // a little sign over what's in reach
        float w = textW(sp[i].name, 0.34f) + 12, by = sp[i].y - (sp[i].kind == SP_HOOP ? 112 : sp[i].kind >= SP_HOMEDOOR && sp[i].kind <= SP_NDOOR2 ? 74 : 86);
        flashPanel(sp[i].x - w / 2, by, w, 15, RGB{255, 46, 140}, 6, 230);
        textOutline(sp[i].name, sp[i].x, by - 0.5f, 0.34f, C(255, 255, 255), C(UI_INK), 1, 0.6f);
    }
    // district arrows: L/R to the next screen
    int l = districtNext(room, -1), r = districtNext(room, 1);
    if (l >= 0 && l != room) { flashPanel(2, 120, 16, 22, RGB{40, 192, 210}, 5, 220); textOutline("L", 10, 122, 0.4f, C(255, 255, 255), C(UI_INK), 1); }
    if (r >= 0 && r != room && r != l) { flashPanel(382, 120, 16, 22, RGB{40, 192, 210}, 5, 220); textOutline("R", 390, 122, 0.4f, C(255, 255, 255), C(UI_INK), 1); }
    else if (r >= 0 && r == l && r != room) { flashPanel(382, 120, 16, 22, RGB{40, 192, 210}, 5, 220); textOutline("R", 390, 122, 0.4f, C(255, 255, 255), C(UI_INK), 1); }
}
