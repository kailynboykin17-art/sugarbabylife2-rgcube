#include "lifeplus2.hpp"
#include "traits2.hpp"
#include "townpulse.hpp"
#include "choices.hpp"
#include "emote.hpp"
#include "dynasty.hpp"
#include "dailylife.hpp"
#include "growup.hpp"
#include "kidlife.hpp"
#include "famdrama.hpp"
#include "sevents.hpp"
#include "deeplife.hpp"
#include "lifeext.hpp"
#include "spicy.hpp"
#include "hotdate.hpp"
#include "nightlife.hpp"
#include "villa.hpp"
#include "clubs.hpp"
#include "content.hpp"
#include "gallery.hpp"
#include "lockup.hpp"
#include "audio.hpp"
#include "worldsim.hpp"
#include "travelcourt.hpp"
#include "health.hpp"
#include "lifestory.hpp"
#include "sim.hpp"
#include "farm.hpp"
#include "levels.hpp"
#include "life3.hpp"
#include "romance.hpp"
#include "lifeplus.hpp"
#include "economy.hpp"
#include "life.hpp"
#include "kids.hpp"
#include "seasons.hpp"
#include "family.hpp"
#include "chat.hpp"
#include "status.hpp"
#include "lovebug.hpp"
#include "squads.hpp"
#include "extras.hpp"
#include "crime.hpp"
#include "drama.hpp"
#include "district.hpp"
#include "cutscene.hpp"
#include "goals.hpp"
#include "catalog.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace col;

const char* WEEKDAY_NAMES[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
const char* SEASON_NAMES[4] = {"Spring", "Summer", "Fall", "Winter"};
const char* STAGE_NAMES[ST_COUNT] = {"Infant", "Toddler", "Kid", "Preteen", "Teen", "Young Adult", "Adult", "Middle Age", "Elder"};
const uint8_t STAGE_DAYS[ST_COUNT] = {2, 3, 5, 4, 7, 14, 14, 12, 255};
const char* NEED_NAMES[N_COUNT] = {"Hunger", "Energy", "Hygiene", "Fun", "Social", "Bladder"};
const char* SKILL_NAMES[SK_COUNT] = {"Style", "Cooking", "Charisma", "Fitness", "Creativity", "Logic"};
const char* TRAIT_NAMES[TR_COUNT] = {"Fashionista", "Cheerful", "Romantic", "Bookworm", "Foodie", "Creative", "Outgoing", "Loner",
                                     "Ambitious", "Lazy", "Active", "Neat", "Goofball", "Glamorous", "Family-Oriented", "Hot-Headed",
                                     "Music Lover", "Clumsy", "Sneaky", "Jealous", "Self-Assured", "Snob", "Pet Lover", "Night Owl", "Gamer", "Perfectionist", "Kind", "Mean",
                                     "Loyal", "Generous", "Frugal", "Optimist", "Dramatic", "Messy", "Shopaholic", "Insecure",
    "Flirty", "Diva", "Blackmailer", "Charming", "Brave", "Farmer", "Pick Me Girl", "Fuckboy", "Slut", "Girly Girl", "Tomboy", "Housewife", "Pastor", "Scammer", "Shy", "Silver Spoon", "Bitch", "Vain", "Vegan", "Boxer", "Dancer", "Hoops Lover", "Fangirl", "Fanboy", "Nerd", "Makeup Lover", "Trap Star", "Bougie on a Budget", "Shady Boots", "Plug Connect", "Quick Temper", "Hood Rat"};
const char* TRAIT_DESC[TR_COUNT] = {
    "Learns Style fast. New outfits = Confident.", "Happier than most. Sadness fades fast.",
    "Flirts land better. Loves dates.", "Learns Logic & Creativity fast.",
    "Learns Cooking fast. Gets hungry faster.", "Learns Creativity fast. Often Inspired.",
    "Loves people. Social fills faster.", "Likes alone time. Social drains slowly.",
    "Works harder. Promoted faster.", "Low energy use, slow Fitness.",
    "Learns Fitness fast. Loves dancing.", "Stays clean longer.",
    "Jokes always land. Often Playful.", "Gains followers 50% faster.",
    "Loves family time & babies.", "Gets Angry easily. Mean talk is easy.",
    "Learns Creativity fast. Music always lifts their mood.", "Trips, spills & burns things... but laughs it off.",
    "Crimes are harder to catch. Mischief grows fast.", "Gets SO upset when their partner flirts with others.",
    "Confidence doesn't crack. Bad moods fade fast.", "Charms the glam crowd. Basic people? Not so much.",
    "Pets make them so happy. Petting fills Fun.", "Full of energy at night, sleepy in the morning.",
    "Learns Logic fast. Arcade & games are extra fun.", "Skills grow faster, but mistakes make them Tense.",
    "Nice socials land better. Townies love them.", "Mean socials land better. Nice ones... less so.",
    "Ride or die. Partners trust them more.", "Gifts & kindness land extra sweet.",
    "Gets deals: 15% off online orders.", "Sad moods fade fast. Always looks on the bright side.",
    "Everything's a scene. Starts more drama & fights.", "Gets dirty faster. Doesn't care.",
    "Can't stop buying stuff online...", "Mean words hit harder. Compliments mean SO much.",
    "Flirts come naturally. Romance builds fast.", "Needs the spotlight. Loves attention.", "Collects secrets... and cashes them in.", "Everyone warms up to them fast.", "Fear? Never heard of her.", "Green thumb. Gardening grows fast.", "\"I'm not like other girls.\"", "Smooth talker. Commitment issues.", "Unapologetically loves romance & attention.", "Pink, glitter, and everything cute.", "Sneakers over heels, sports over shopping.", "Runs the home like a queen.", "Faith-filled. Uplifts the community.", "Always running a hustle. Sometimes it backfires.", "Socializing drains them. Small circles only.", "Born rich. Gets a family allowance.", "Says it like it is. Not here to be liked.", "Mirror is their best friend.", "Plant-powered. Will tell you about it.", "Fighter. Fitness grows fast.", "Always moving. Dance skill grows fast.", "Basketball is life.", "Obsessed with their faves.", "Knows every stat and lore drop.", "Brain-first. Logic grows fast.", "Beauty skill grows fast. Full beat daily.", "All about the hustle, the grind, and the code.", "Champagne taste, clearance-rack finesse.", "Side-eye and slick remarks.", "The hookup you can always count on.", "Goes from 0 to 100 real quick.", "Neighborhood pride, street-smart, never fake."};
const char* EMOTION_NAMES[EM_COUNT] = {"Fine", "Happy", "Flirty", "Confident", "Inspired", "Playful", "Energized", "Sad", "Angry", "Tense", "Bored", "Focused", "Embarrassed", "Uncomfortable"};
const RGB EMOTION_COLORS[EM_COUNT] = {{200, 200, 200}, {255, 214, 80}, {255, 110, 170}, {255, 150, 60}, {120, 200, 255}, {120, 230, 160},
                                      {255, 236, 80}, {110, 140, 220}, {230, 50, 50}, {190, 120, 220}, {150, 150, 150}, {60, 190, 190}, {255, 140, 150}, {170, 150, 90}};
const char* REL_STATUS_NAMES[5] = {"", "Dating", "Engaged", "Married", "Family"};

const CareerDef CAREERS[CR_COUNT] = {
    {"Unemployed", -1, 0, 0, SK_STYLE, true, {"", "", "", "", ""}, {0, 0, 0, 0, 0}},
    {"Fashion Stylist", R_BOUTIQUE, 10, 6, SK_STYLE, false,
     {"Shop Girl", "Junior Stylist", "Stylist", "Celebrity Stylist", "Creative Director"}, {14, 22, 34, 52, 80}},
    {"Chef", R_RESTAURANT, 16, 6, SK_COOKING, false,
     {"Dishwasher", "Line Cook", "Sous Chef", "Head Chef", "Celebrity Chef"}, {12, 20, 30, 48, 75}},
    {"Barista", R_CAFE, 8, 4, SK_CHARISMA, true,
     {"Barista Trainee", "Barista", "Latte Artist", "Shift Lead", "Cafe Manager"}, {10, 13, 17, 22, 30}},
    {"Restaurant Hostess", R_RESTAURANT, 17, 5, SK_CHARISMA, true,
     {"Host Trainee", "Hostess", "Head Hostess", "Floor Manager", "Restaurant Owner"}, {9, 12, 16, 24, 40}},
    {"Delivery Courier", R_MALL, 11, 4, SK_FITNESS, true,
     {"Bike Courier", "Scooter Courier", "Express Courier", "Dispatch Lead", "Delivery Boss"}, {10, 13, 17, 23, 32}},
    {"Teacher", R_SCHOOL, 8, 7, SK_LOGIC, false,
     {"Teaching Assistant", "Substitute Teacher", "Teacher", "Department Head", "Principal"}, {14, 20, 28, 38, 55}},
    {"Rideshare Driver", R_PARK, 18, 4, SK_CHARISMA, false,
     {"New Driver", "Driver", "Top-Rated Driver", "Luxury Chauffeur", "Fleet Owner"}, {13, 17, 22, 30, 45}},
    {"Fashion Model", R_BOUTIQUE, 12, 5, SK_STYLE, false,
     {"Catalog Model", "Runway Model", "Cover Model", "Supermodel", "Fashion Icon"}, {15, 25, 40, 65, 100}},
    {"Doctor", R_HOSPITAL, 8, 8, SK_LOGIC, false,
     {"Med Student", "Resident", "Doctor", "Surgeon", "Chief of Medicine"}, {16, 26, 42, 66, 98}},
    {"Writer", R_LIBRARY, 10, 5, SK_CREATIVITY, true,
     {"Blogger", "Copywriter", "Published Author", "Bestselling Author", "Literary Legend"}, {11, 18, 30, 50, 85}},
    {"Actor", R_CINEMA, 14, 6, SK_CHARISMA, true,
     {"Extra", "Commercial Actor", "TV Star", "Leading Lady", "Movie Icon"}, {12, 20, 36, 60, 105}},
    {"Business", R_HOTEL, 9, 8, SK_CHARISMA, false,
     {"Intern", "Assistant Manager", "Manager", "VP", "CEO"}, {14, 22, 36, 58, 96}},
    {"Gardener", R_PARK, 7, 5, SK_CREATIVITY, true,
     {"Weed Puller", "Garden Helper", "Florist", "Botanist", "Garden Guru"}, {10, 14, 20, 30, 46}},
    {"Influencer", R_MALL, 12, 4, SK_STYLE, true,
     {"Nano Influencer", "Micro Influencer", "Content Creator", "Brand Ambassador", "Mega Influencer"}, {9, 15, 26, 45, 80}},
    {"Celebrity", R_CINEMA, 14, 5, SK_CHARISMA, false,
     {"Local Buzz", "It Name", "Red Carpet Regular", "A-Lister", "Icon"}, {14, 24, 42, 75, 130}},
};

Activity ACT;

// ------------------------------------------------------------------ clock
std::string clockStr(float minute) {
    int m = (int)minute, h = m / 60, mm = m % 60;
    int h12 = h % 12 == 0 ? 12 : h % 12;
    char b[16];
    snprintf(b, sizeof b, "%d:%02d %s", h12, mm, h < 12 ? "AM" : "PM");
    return b;
}
std::string dateStr(int day) {
    char b[40];
    snprintf(b, sizeof b, "%s, %s %d", WEEKDAY_NAMES[weekday(day)], SEASON_NAMES[seasonOf(day)], day % 7 + 1);
    return b;
}

// ------------------------------------------------------------------ sim generation
static const char* GIRL_NAMES[] = {"Aaliyah", "Brooke", "Cami", "Daisy", "Elle", "Fifi", "Harlow", "Ivy", "Kenzie", "Lola",
                                   "Nia", "Remi", "Skye", "Talia", "Zara", "Monet", "Jolie", "Coco", "Dahlia", "Bree"};
static const char* GUY_NAMES[] = {"Ace", "Blake", "Cody", "Dante", "Eli", "Gavin", "Jace", "Kai", "Liam", "Miles",
                                  "Nico", "Reid", "Theo", "Zeke", "Andre", "Rafael", "Mason", "Jay", "Logan", "Omar"};
static const char* BIOS[] = {"pink is my personality", "brunch > everything", "looking for my dance partner",
                             "dog mom & proud", "shoe collector (send help)", "will steal ur fries", "aspiring fashion icon",
                             "gym, glam, repeat", "just here for the memes", "hopeless romantic <3", "coffee addict",
                             "beach days only", "future CEO", "karaoke queen/king", "reads more than u"};

std::string randomName(bool guy) { return guy ? GUY_NAMES[irand(20)] : GIRL_NAMES[irand(20)]; }

static const RGB TOPS[] = {HOT, BLUSH, CREAM, BLACK, GOLD, LILAC, PINK, BERRY, BABYBLUE, MINT, CHOC, WHITE};
static const RGB BOTS[] = {BLACK, CHOC, CREAM, BLUSH, HOT, LATTE, BABYBLUE, GOLD};
static const RGB LIPS[] = {{196, 40, 96}, {200, 30, 50}, {240, 120, 150}, {190, 110, 100}};

// mix & match clothes that suit the age and gender
void randomWardrobe(PersonDef& d, Stage st) {
    const bool guy = d.guy, kid = st <= ST_PRETEEN, elder = st == ST_ELDER;
    static const uint8_t GTOPS[] = {TS_TEE, TS_TANK, TS_CROP, TS_HOODIE, TS_BUTTON, TS_SWEATER, TS_DRESS, TS_JACKET, TS_TUBE, TS_CARDIGAN, TS_TEE, TS_TANK};
    static const uint8_t BTOPS[] = {TS_TEE, TS_TANK, TS_HOODIE, TS_JERSEY, TS_BUTTON, TS_SWEATER, TS_JACKET, TS_POLO, TS_TEE, TS_HOODIE};
    static const uint8_t GBOTS[] = {BT_JEANS, BT_MINI, BT_SHORTS, BT_PANTS, BT_PLEATED, BT_MAXI, BT_SWEATS, BT_JEANS, BT_OVERALLS};
    static const uint8_t BBOTS[] = {BT_JEANS, BT_PANTS, BT_SHORTS, BT_SWEATS, BT_JEANS, BT_OVERALLS};
    d.topStyle = guy ? BTOPS[irand(sizeof BTOPS)] : GTOPS[irand(sizeof GTOPS)];
    d.bt = (Bottom)(guy ? BBOTS[irand(sizeof BBOTS)] : GBOTS[irand(sizeof GBOTS)]);
    d.shoeStyle = irand(3) ? SS_SNEAKERS : (uint8_t)(guy ? (irand(2) ? SS_BOOTS : SS_LOAFERS) : (irand(2) ? SS_PLATFORMS : irand(2) ? SS_HEELS : SS_SANDALS));
    if (kid) { if (d.topStyle == TS_TUBE || d.topStyle == TS_CROP) d.topStyle = TS_TEE; if (d.shoeStyle == SS_HEELS || d.shoeStyle == SS_PLATFORMS) d.shoeStyle = SS_SNEAKERS; }
    if (elder) { static const uint8_t ET[] = {TS_CARDIGAN, TS_SWEATER, TS_BUTTON, TS_POLO}; d.topStyle = ET[irand(4)]; d.bt = guy ? BT_PANTS : (irand(2) ? BT_MAXI : BT_PANTS); d.shoeStyle = irand(2) ? SS_LOAFERS : SS_SLIPPERS; }
    d.hat = irand(5) == 0 ? (uint8_t)(guy ? (irand(2) ? HT_CAP : irand(2) ? HT_BEANIE : HT_BUCKET) : 1 + irand(NUM_HATS - 1)) : HT_NONE;
    if (d.hat == HT_TIARA && !kid && irand(2)) d.hat = HT_NONE;
    d.acc = 0;
    if (!guy && !kid && irand(5) < 2) { static const uint16_t A[] = {ACC_BAG, ACC_NECKLACE, ACC_HOOPS, ACC_SHADES, ACC_BANGLES, ACC_BAG | ACC_SHADES}; d.acc = A[irand(6)]; }
    if (guy && !kid && irand(6) == 0) d.acc = irand(2) ? ACC_NECKLACE : ACC_SHADES;
    static const RGB AC[] = {{255, 90, 160}, {224, 184, 104}, {40, 30, 34}, {120, 200, 230}, {200, 60, 70}};
    d.accCol = AC[irand(5)];
}

static void randomTraits(Sim& s, int n) {
    for (int i = 0; i < 3; i++) s.traits[i] = TR_NONE;
    for (int i = 0; i < n; i++) {
        uint8_t t;
        do t = (uint8_t)irand(TR_COUNT); while (s.hasTrait(t) || !traitOk(t, s.stage));
        s.traits[i] = t;
    }
}
int traitSlotsFor(Stage st) { return st <= ST_TODDLER ? 1 : st <= ST_PRETEEN ? 1 : st == ST_TEEN ? 2 : 3; }

Sim randomSim(Stage st, int gender) {
    Sim s;
    bool guy = gender < 0 ? irand(2) == 1 : gender == 1;
    s.look.guy = guy;
    s.look.age = ADULT;
    s.look.skin = (uint8_t)irand(6);
    s.look.hairCol = (uint8_t)irand(7);
    s.look.hairStyle = guy ? 5 : (uint8_t)irand(5);
    s.look.top = TOPS[irand(12)];
    s.look.bottom = BOTS[irand(8)];
    s.look.bt = guy ? BT_PANTS : (Bottom)irand(3);
    s.look.topPattern = irand(4) == 0 ? (uint8_t)(1 + irand(3)) : 0;
    s.look.shoes = TOPS[irand(12)];
    s.look.lips = LIPS[irand(4)];
    s.look.skin = (uint8_t)irand(NUM_SKINS);
    s.look.hairCol = irand(5) == 0 ? (uint8_t)irand(NUM_HAIRCOLS) : (uint8_t)irand(6);
    {   // hair textures for everyone
        static const uint8_t GIRL_HS[] = {0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 0, 1, 6};
        static const uint8_t GUY_HS[] = {5, 5, 5, 6, 8, 11, 10, 7};
        s.look.hairStyle = guy ? GUY_HS[irand(sizeof GUY_HS)] : GIRL_HS[irand(sizeof GIRL_HS)];
    }
    s.look.f = randomFace(guy, (uint32_t)rand());
    s.name = randomName(guy);
    s.stage = st;
    s.bio = BIOS[irand(15)];
    s.interest = irand(10) < 7 ? (guy ? 1 : 0) : 2;
    randomTraits(s, traitSlotsFor(st));
    for (int i = 0; i < SK_COUNT; i++) s.skills[i] = st >= ST_YA ? frand() * 30 : frand() * 10;
    s.stageDays = (uint16_t)irand(STAGE_DAYS[st] == 255 ? 5 : STAGE_DAYS[st]);
    // townies rock the different aesthetics
    {   // hairstyles for everyone (Stick Figure Ver. has 28)
        static const uint8_t GIRL_HS[] = {0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13, 22, 23, 25, 26, 21, 0, 1, 7, 3,
                                          28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 28, 29, 33, 35,
                                          40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 40, 47, 49, 41};
        static const uint8_t GUY_HS[] = {5, 15, 16, 17, 18, 19, 8, 11, 14, 20, 24, 27, 21, 6, 7, 5, 16, 15, 34, 38, 39, 39, 54, 55, 56, 57, 54, 57, 43};
        s.look.hairStyle = guy ? GUY_HS[irand(sizeof GUY_HS)] : GIRL_HS[irand(sizeof GIRL_HS)];
        if (st == ST_ELDER && irand(3)) s.look.hairCol = 7;
    }
    randomWardrobe(s.look, st);
    if (st >= ST_CHILD && irand(100) < 45) {
        static const uint8_t looks[] = {9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30};
        int o = looks[irand(sizeof looks)];
        if (st <= ST_PRETEEN && (o == 23 || o == 28 || o == 12 || o == 10)) o = 21;
        if (st == ST_ELDER && irand(2)) o = 24;
        s.outfit = (uint8_t)o;
    }
    s.favFood = (uint8_t)irand(18); s.favColor = (uint8_t)irand(12); s.zodiac = (uint8_t)irand(12);
    return s;
}

static Sim fromPreset(const PersonDef& d) {
    Stage st = d.age == ADULT ? ST_ADULT : d.age == TEEN ? ST_YA : d.age == KID ? ST_CHILD : ST_INFANT;
    if (d.hairCol == 7) st = ST_ELDER;
    else if (std::string(d.name).rfind("Mom", 0) == 0 || std::string(d.name).rfind("Dad", 0) == 0) st = ST_MIDDLE;
    Sim s = randomSim(st, d.guy ? 1 : 0);
    s.look = d;
    s.look.name = nullptr;
    { Bottom keep = d.bt; randomWardrobe(s.look, st); if (keep == BT_GOWN || keep == BT_MINI) s.look.bt = keep; }
    { uint32_t h = 2166136261u; for (const char* c = d.name; *c; c++) h = (h ^ (uint8_t)*c) * 16777619u; s.look.f = randomFace(d.guy, h); }
    const char* nm = d.name;
    s.name = nm;
    return s;
}

// extra rooms for two-story homes & apartments (save v29); also fills them in for older saves
void furnishExtraRooms(RoomState* r) {
    auto add = [](RoomState& rr, int sh, int c, int x, int y) { rr.items.push_back({(uint16_t)itemId(sh, c), (int16_t)x, (int16_t)y}); };
    if (r[R_BEDROOM2].items.empty()) { r[R_BEDROOM2].wall = 6; r[R_BEDROOM2].wallPat = 2; r[R_BEDROOM2].floor = 0; add(r[R_BEDROOM2], SH_BRASSBED, 7, 230, 214); add(r[R_BEDROOM2], SH_NIGHTSTAND, 7, 150, 214); add(r[R_BEDROOM2], SH_BOOKCUBBY, 4, 60, 214); add(r[R_BEDROOM2], SH_STARLIGHTS, 7, 230, 60); add(r[R_BEDROOM2], SH_FLUFFYRUG, 6, 230, 232); }
    if (r[R_BATHROOM].items.empty()) { r[R_BATHROOM].wall = 4; r[R_BATHROOM].floor = 3; r[R_BATHROOM].floorPat = 2; add(r[R_BATHROOM], SH_SHOWER, 7, 90, 210); add(r[R_BATHROOM], SH_TOILET, 7, 190, 214); add(r[R_BATHROOM], SH_BATHVANITY, 7, 290, 210); add(r[R_BATHROOM], SH_MIRROR, 7, 290, 70); add(r[R_BATHROOM], SH_PLANT, 4, 370, 226); }
    if (r[R_STUDY].items.empty()) { r[R_STUDY].wall = 8; r[R_STUDY].wallPat = 3; r[R_STUDY].floor = 6; add(r[R_STUDY], SH_GAMINGPC, 9, 110, 210); add(r[R_STUDY], SH_GAMERCHAIR, 1, 110, 230); add(r[R_STUDY], SH_TV, 9, 300, 176); add(r[R_STUDY], SH_BEANBAG, 6, 290, 230); add(r[R_STUDY], SH_POSTERWALL, 1, 200, 60); add(r[R_STUDY], SH_BOOKSHELF, 9, 380, 210); }
    if (r[R_TEENROOM].items.empty()) { r[R_TEENROOM].wall = 1; r[R_TEENROOM].wallPat = 9; r[R_TEENROOM].floor = 0; add(r[R_TEENROOM], SH_PLUSHBED, 0, 250, 214); add(r[R_TEENROOM], SH_DESK, 0, 90, 210); add(r[R_TEENROOM], SH_EGGCHAIR, 1, 170, 226); add(r[R_TEENROOM], SH_HEARTRUG, 1, 250, 232); add(r[R_TEENROOM], SH_POSTERWALL, 1, 120, 60); add(r[R_TEENROOM], SH_STARLIGHTS, 1, 300, 54); add(r[R_TEENROOM], SH_CLOTHESRACK, 7, 370, 210); }
}
// a starter home so the dollhouse never feels empty
static void furnishHome(RoomState* rooms, int house) {
    auto add = [](RoomState& r, int sh, int c, int x, int y) { r.items.push_back({(uint16_t)itemId(sh, c), (int16_t)x, (int16_t)y}); };
    int A = house == 0 ? 5 : 1;    // main accent: mocha cottage vs hot pink penthouse
    int B = house == 0 ? 10 : 9;   // zebra vs black gloss
    RoomState* r = rooms;
    r[R_LIVING].wall = 1; r[R_LIVING].wallPat = 9; r[R_LIVING].floor = 0; r[R_LIVING].floorPat = 4;   // kawaii pastel living room
    add(r[R_LIVING], SH_FLUFFYRUG, 0, 220, 222); (void)B; add(r[R_LIVING], SH_PLUSHSOFA, 0, 232, 206); (void)A; add(r[R_LIVING], SH_TABLE, 7, 230, 230);
    add(r[R_LIVING], SH_TV, 9, 340, 176);  add(r[R_LIVING], SH_DRUMLAMP, 7, 110, 200);
    add(r[R_LIVING], SH_PLANT, 7, 380, 230); add(r[R_LIVING], SH_MONSTERA, 7, 24, 232);
    r[R_KITCHEN].wall = 1; r[R_KITCHEN].floor = 3; r[R_KITCHEN].floorPat = 2;
    add(r[R_KITCHEN], SH_FRIDGE, house == 0 ? 0 : 9, 190, 196); add(r[R_KITCHEN], SH_COUNTER, 4, 250, 196); add(r[R_KITCHEN], SH_OVEN, 9, 312, 196);
    add(r[R_KITCHEN], SH_SINK, 4, 370, 196); add(r[R_KITCHEN], SH_TABLE, 4, 150, 232); add(r[R_KITCHEN], SH_CHAIR, 1, 110, 232);
    add(r[R_KITCHEN], SH_CLOCK, 7, 260, 60);
    // a cute anime-style bedroom: balcony door, curtains, brass bed, oval vanity, fluffy rug, tea table
    r[R_BEDROOM].wall = 0; r[R_BEDROOM].wallPat = house == 0 ? 4 : 6; r[R_BEDROOM].floor = 3; r[R_BEDROOM].floorPat = 4;
    add(r[R_BEDROOM], SH_FLUFFYRUG, 0, 196, 230); add(r[R_BEDROOM], SH_HEARTRUG, 0, 330, 238);
    add(r[R_BEDROOM], SH_OVALVANITY, 7, 64, 204); add(r[R_BEDROOM], SH_BRASSBED, 0, 326, 214); add(r[R_BEDROOM], SH_NIGHTSTAND, 7, 252, 200);
    add(r[R_BEDROOM], SH_TEATABLE, 7, 186, 234); add(r[R_BEDROOM], SH_FLOWERCUSHION, 0, 236, 238); add(r[R_BEDROOM], SH_POSTERWALL, 0, 326, 100);
    r[R_CLOSET].wall = 1; r[R_CLOSET].wallPat = 10; r[R_CLOSET].floor = 4; r[R_CLOSET].floorPat = 6;   // mint bath
    add(r[R_CLOSET], SH_SHOERACK, 9, 44, 196); add(r[R_CLOSET], SH_SHOERACK, 7, 106, 196); add(r[R_CLOSET], SH_DRESSER, 4, 210, 196);
    add(r[R_CLOSET], SH_MIRROR, 7, 210, 90); add(r[R_CLOSET], SH_RUG, 11, 200, 226);
    add(r[R_CLOSET], SH_TOILET, 4, 160, 206); add(r[R_CLOSET], SH_SOAKTUB, 0, 300, 236); add(r[R_CLOSET], SH_SHOWER, 4, 372, 198);
    r[R_NURSERY].wall = 6; r[R_NURSERY].wallPat = 1; r[R_NURSERY].floor = 3; r[R_NURSERY].floorPat = 3;
    add(r[R_NURSERY], SH_CRIB, 4, 230, 210); add(r[R_NURSERY], SH_TOYBOX, 0, 320, 226); add(r[R_NURSERY], SH_BOOKSHELF, 4, 360, 196);
    add(r[R_NURSERY], SH_CHAIR, 0, 150, 220);
    r[R_BASEMENT].wall = 11; r[R_BASEMENT].wallPat = 3; r[R_BASEMENT].floor = 6; r[R_BASEMENT].floorPat = 2;
    add(r[R_BASEMENT], SH_ARCADE, 1, 60, 200); add(r[R_BASEMENT], SH_DISCOBALL, 1, 320, 56); add(r[R_BASEMENT], SH_SPEAKER, 9, 330, 200);
    add(r[R_BASEMENT], SH_BEANBAG, 1, 250, 226); add(r[R_BASEMENT], SH_BEANBAG, 10, 160, 230); add(r[R_BASEMENT], SH_SOFA, 11, 300, 230);
    add(r[R_YARD], SH_GRILL, 9, 80, 200); add(r[R_YARD], SH_UMBRELLA, 0, 180, 214); add(r[R_YARD], SH_LOUNGER, 1, 180, 226);
    add(r[R_YARD], SH_FLOWERS, 1, 30, 226);
}

void buildWorld(Game& g, const Sim& player) {
    g = Game();
    Sim p = player;
    p.household = true;
    g.sims.push_back(p);
    g.mc = 0;
    for (int i = 0; i < NUM_PEOPLE; i++) {
        if (PEOPLE[i].age == BABY) continue;
        g.sims.push_back(fromPreset(PEOPLE[i]));
    }
    for (int i = 0; i < 14; i++) g.sims.push_back(randomSim(irand(3) == 0 ? ST_ADULT : ST_YA, i % 2));
    for (int i = 0; i < 4; i++) g.sims.push_back(randomSim(i < 2 ? ST_TEEN : ST_PRETEEN, i % 2));
    for (int h = 0; h < 2; h++) furnishHome(g.rooms[h], h);
    // player starts in the living room with the house pet
    g.pets.push_back(0);
    Actor a; a.kind = 0; a.id = 0; a.x = 200; a.y = 222; a.tx = 200;
    g.rooms[g.house][R_LIVING].actors.push_back(a);
    Actor pet; pet.kind = 1; pet.id = 0; pet.x = 300; pet.y = 228; pet.tx = 300;
    g.rooms[g.house][R_LIVING].actors.push_back(pet);
    g.minute = 8 * 60;
    // starter texts from a bestie
    ChatThread th; th.sim = 1; th.msgs.push_back({false, "omg welcome to the neighborhood!!"});
    th.prompt = 3; th.unread = true;
    g.threads.push_back(th);
    Rel r; r.a = 0; r.b = 1; r.fr = 25; r.met = true;
    g.rels.push_back(r);
}

// ------------------------------------------------------------------ mood
void addMoodlet(Sim& s, const char* name, Emotion e, int power, int minutes) {
    for (Moodlet& m : s.moodlets)
        if (m.name == name) { m.minutes = (uint16_t)minutes; m.power = (int8_t)power; return; }
    if (s.moodlets.size() >= 6) s.moodlets.erase(s.moodlets.begin());
    s.moodlets.push_back({name, (uint8_t)e, (int8_t)power, (uint16_t)minutes});
}

static float avg0(const Sim& s) { float a = 0; for (float n : s.needs) a += n; return a / N_COUNT; }
Emotion emotionOf(const Sim& s) { int v; return emotionOf(s, v); }
Emotion emotionOf(const Sim& s, int& strength) {
    int score[EM_COUNT] = {0};
    for (const Moodlet& m : s.moodlets) score[m.emotion] += m.power;
    if (s.needs[N_HUNGER] < 20 || s.needs[N_ENERGY] < 15) score[EM_TENSE] += 2;
    if (s.needs[N_FUN] < 20) score[EM_BORED] += 2;
    if (s.needs[N_SOCIAL] < 15) score[EM_SAD] += 2;
    if (s.needs[N_HYGIENE] < 15) score[EM_UNCOMFORTABLE] += 2;
    if (s.needs[N_BLADDER] < 15) score[EM_UNCOMFORTABLE] += 2;
    if (s.hasTrait(TR_CHEERFUL)) score[EM_HAPPY] += 1;
    if (s.hasTrait(TR_SELFASSURED)) { score[EM_CONFIDENT] += 1; score[EM_SAD] = score[EM_SAD] * 2 / 3; score[EM_TENSE] = score[EM_TENSE] * 2 / 3; }
    if (s.hasTrait(TR_PERFECTIONIST) && avg0(s) < 45) score[EM_TENSE] += 1;
    if (s.hasTrait(TR_NIGHTOWL)) { int h = (int)(G.minute / 60); if (h >= 21 || h < 3) score[EM_ENERGIZED] += 1; }
    float avg = 0;
    for (float n : s.needs) avg += n;
    avg /= N_COUNT;
    if (avg > 65) score[EM_HAPPY] += 1;
    if (!G.sims.empty() && G.mc >= 0 && G.mc < (int)G.sims.size() && &s == &G.sims[G.mc]) l3PermanentBuffs(score, EM_COUNT);
    int bestE = EM_FINE, bestV = 0;
    for (int e = 1; e < EM_COUNT; e++)
        if (score[e] > bestV) { bestV = score[e]; bestE = e; }
    strength = bestV;
    return (Emotion)bestE;
}

int moodOf(const Sim& s) {
    float avg = 0;
    for (float n : s.needs) avg += n;
    avg /= N_COUNT;
    int m = (int)((avg - 50) * 1.4f);
    for (const Moodlet& md : s.moodlets) {
        bool neg = md.emotion == EM_SAD || md.emotion == EM_ANGRY || md.emotion == EM_TENSE || md.emotion == EM_BORED;
        m += (neg ? -1 : 1) * md.power * 8;
    }
    return clampv(m, -100, 100);
}

static float skillMult(const Sim& s, int skill) {
    float m = 1.f;
    if (!G.sims.empty() && G.mc >= 0 && G.mc < (int)G.sims.size() && &s == &G.sims[G.mc]) m *= l3SkillMult(skill);
    Emotion e = emotionOf(s);
    switch (skill) {
    case SK_STYLE: if (s.hasTrait(TR_FASHIONISTA)) m += 0.5f; if (e == EM_CONFIDENT) m += 0.3f; break;
    case SK_COOKING: if (s.hasTrait(TR_FOODIE)) m += 0.5f; break;
    case SK_CHARISMA: if (s.hasTrait(TR_OUTGOING)) m += 0.3f; if (e == EM_CONFIDENT) m += 0.3f; break;
    case SK_FITNESS: if (s.hasTrait(TR_ACTIVE)) m += 0.5f; if (s.hasTrait(TR_LAZY)) m -= 0.4f; if (e == EM_ENERGIZED) m += 0.3f; break;
    case SK_CREATIVITY: if (s.hasTrait(TR_CREATIVE) || s.hasTrait(TR_BOOKWORM)) m += 0.5f; if (e == EM_INSPIRED) m += 0.4f; break;
    case SK_LOGIC: if (s.hasTrait(TR_BOOKWORM)) m += 0.5f; if (s.hasTrait(TR_GAMER)) m += 0.4f; break;
    }
    if (skill == SK_CREATIVITY && s.hasTrait(TR_MUSIC)) m += 0.4f;
    if (s.hasTrait(TR_PERFECTIONIST)) m += 0.15f;
    if (&s == &G.sims[G.mc]) m *= perkSkillMult();
    return m;
}
void skillGain(Sim& s, int skill, float amt) {
    if (skill < 0 || skill >= SK_COUNT) return;
    int before = skillLevel(s, skill);
    s.skills[skill] = std::fmin(100.f, s.skills[skill] + amt * skillMult(s, skill));
    int after = skillLevel(s, skill);
    if (after > before && &s == &G.sims[G.mc]) {
        char b[64];
        snprintf(b, sizeof b, "%s skill is now level %d!", SKILL_NAMES[skill], after);
        toast(b);
        addMoodlet(s, "Leveled Up", EM_CONFIDENT, 1, 120);
        goalNotify(GE_SKILLUP, skill);
    }
}
int skillLevel(const Sim& s, int skill) { return clampv((int)(s.skills[skill] / 10.f), 0, 10); }

int fameStars() {
    uint32_t f = G.followers;
    return f >= 5000000 ? 5 : f >= 500000 ? 4 : f >= 50000 ? 3 : f >= 5000 ? 2 : f >= 500 ? 1 : 0;
}
const char* fameTitle() {
    static const char* t[] = {"Unknown", "Rising Star", "Notable", "Popular", "Celebrity", "Global Icon"};
    return t[fameStars()];
}
std::string letterGrade(float p) { return p >= 90 ? "A" : p >= 80 ? "B" : p >= 70 ? "C" : p >= 60 ? "D" : "F"; }
int householdCount() { int n = 0; for (const Sim& s : G.sims) if (s.household) n++; return n; }

Pose stagePose(const Sim& s) { (void)s; return PO_IDLE; }

// ------------------------------------------------------------------ relationships
const Rel* findRel(int a, int b) {
    if (a > b) std::swap(a, b);
    for (const Rel& r : G.rels) if (r.a == a && r.b == b) return &r;
    return nullptr;
}
Rel& rel(int a, int b) {
    if (a > b) std::swap(a, b);
    for (Rel& r : G.rels) if (r.a == a && r.b == b) return r;
    Rel r; r.a = (uint16_t)a; r.b = (uint16_t)b;
    G.rels.push_back(r);
    return G.rels.back();
}
void changeRel(int a, int b, int dFr, int dRo) {
    { const int other = a == G.mc ? b : b == G.mc ? a : -1;   // the other sim reacts with an emotion bubble
      if (other >= 0) { if (dRo >= 5) emotePlay(other, EMO_LOVE); else if (dFr >= 6) emotePlay(other, EMO_HAPPY); else if (dFr <= -6) emotePlay(other, EMO_ANGRY); } }
    Rel& r = rel(a, b);
    r.met = true;
    if (G.sims[a].hasTrait(TR_ROMANTIC) && dRo > 0) dRo += dRo / 3 + 1;
    if (G.sims[a].hasTrait(TR_FAMILY) && r.status == RS_FAMILY && dFr > 0) dFr += 2;
    int frBefore = r.fr;
    r.fr = (int8_t)clampv(r.fr + dFr, -100, 100);
    if ((a == G.mc || b == G.mc) && frBefore < 30 && r.fr >= 30) goalNotify(GE_NEWFRIEND);
    if ((a == G.mc || b == G.mc) && frBefore < 80 && r.fr >= 80 && r.status != RS_FAMILY) { int o = a == G.mc ? b : a; addMemoryOnce(MEM_FRIEND, G.mc, o, "Best friends with " + G.sims[o].name + "!"); }
    r.ro = (int8_t)clampv(r.ro + dRo, -100, 100);
}
bool isFamily(int a, int b) { const Rel* r = findRel(a, b); return r && r->status == RS_FAMILY; }
static bool likesGender(const Sim& who, const Sim& other) {
    return who.interest == 2 || (who.interest == 0 && other.look.guy) || (who.interest == 1 && !other.look.guy);
}
bool canRomance(int a, int b) {
    if (a == b) return false;
    const Sim &A = G.sims[a], &B = G.sims[b];
    if (!A.isAdult() || !B.isAdult() || isFamily(a, b)) return false;
    return likesGender(A, B) && likesGender(B, A);
}
int partnerOf(int sim) {
    for (const Rel& r : G.rels)
        if ((r.a == sim || r.b == sim) && r.status >= RS_DATING && r.status <= RS_MARRIED) return r.a == sim ? r.b : r.a;
    return -1;
}
const char* relLabel(int a, int b) {
    const Rel* r = findRel(a, b);
    if (!r || !r->met) return "Stranger";
    if (r->status != RS_NONE) return REL_STATUS_NAMES[r->status];
    if (r->ro >= 40) return "Crush";
    if (r->fr >= 70) return "Best Friend";
    if (r->fr >= 40) return "Friend";
    if (r->fr >= 10) return "Acquaintance";
    if (r->fr <= -30) return "Enemy";
    if (r->fr < 0) return "Not a fan";
    return "Just met";
}

// ------------------------------------------------------------------ life events
static Actor* findActor(int simIdx, int* roomOut = nullptr) {
    for (int r = 0; r < R_COUNT; r++)
        for (Actor& a : G.rooms[G.house][r].actors)
            if (a.kind == 0 && a.id == simIdx) { if (roomOut) *roomOut = r; return &a; }
    return nullptr;
}

void moveIn(int idx) {
    Sim& s = G.sims[idx];
    if (s.household) return;
    s.household = true;
    if (!findActor(idx)) {
        Actor a; a.kind = 0; a.id = (uint16_t)idx; a.x = 120 + irand(160); a.y = 200 + irand(30); a.tx = a.x;
        G.rooms[G.house][R_LIVING].actors.push_back(a);
    }
    toast(s.name + " moved in!");
}

int makeBaby(int pa, int pb) {
    Sim b = randomSim(ST_INFANT, -1);
    const Sim* A = pa >= 0 ? &G.sims[pa] : nullptr;
    const Sim* B = pb >= 0 ? &G.sims[pb] : nullptr;
    if (A && B) {
        b.look.skin = irand(2) ? A->look.skin : B->look.skin;
        b.look.hairCol = irand(2) ? A->look.hairCol : B->look.hairCol;
    }
    b.look.hairStyle = b.look.guy ? 5 : 0;
    if (A) b.look.f.eyeCol = (irand(2) || !B) ? A->look.f.eyeCol : B->look.f.eyeCol;
    b.look.top = b.look.guy ? BABYBLUE : (irand(2) ? BLUSH : LILAC);
    b.household = true;
    b.stageDays = 0;
    for (float& sk : b.skills) sk = 0;
    for (float& n : b.needs) n = 90;
    G.sims.push_back(b);
    int idx = (int)G.sims.size() - 1;
    for (int i = 0; i < idx; i++)
        if (G.sims[i].household) { Rel& r = rel(i, idx); r.status = RS_FAMILY; r.fr = 40; r.met = true; }
    Actor a; a.kind = 0; a.id = (uint16_t)idx; a.x = 230; a.y = 214; a.tx = 230;
    G.rooms[G.house][R_NURSERY].actors.push_back(a);
    geneticsOnBirth(idx, pa, pb);
    addMemory(MEM_FAMILY, idx, pa >= 0 ? pa : -1, "Welcome to the family, " + G.sims[idx].name + "!");
    cutscenePlay(CS_BABY);
    return idx;
}

void ageUp(int idx) {
    Sim& s = G.sims[idx];
    if (s.stage >= ST_ELDER) return;
    s.stage = (Stage)(s.stage + 1);
    s.stageDays = 0;
    int slots = traitSlotsFor(s.stage);
    for (int i = 0; i < slots; i++)
        if (s.traits[i] == TR_NONE) {
            uint8_t t;
            do t = (uint8_t)irand(TR_COUNT); while (s.hasTrait(t) || !traitOk(t, s.stage));
            s.traits[i] = t;
            if (idx == G.mc) toast(std::string("New trait: ") + TRAIT_NAMES[t]);
        }
    if (s.stage == ST_ELDER) s.look.hairCol = 7;
    if (s.stage == ST_TODDLER || s.stage == ST_CHILD) { static const uint8_t KHS[] = {0, 1, 2, 3, 4, 6, 9, 31, 35, 12, 13}; static const uint8_t KHB[] = {5, 39, 6, 16}; s.look.hairStyle = s.look.guy ? KHB[irand(sizeof KHB)] : KHS[irand(sizeof KHS)]; }
    if (s.stage == ST_CHILD) { s.look.top = TOPS[irand(12)]; s.look.bottom = BOTS[irand(8)]; s.look.shoes = TOPS[irand(12)]; }
    geneticsOnAgeUp(idx);
    kidsOnAgeUp(idx);
    addMoodlet(s, "Birthday!", EM_HAPPY, 2, 360);
    char b[80];
    snprintf(b, sizeof b, "Happy birthday! %s is now %s %s!", s.name.c_str(),
             (s.stage == ST_INFANT || s.stage == ST_ELDER || s.stage == ST_ADULT) ? "an" : "a", STAGE_NAMES[s.stage]);
    toast(b);
    if (s.household) addMemory(MEM_LIFE, idx, -1, s.name + " grew up into " + ((s.stage == ST_INFANT || s.stage == ST_ELDER || s.stage == ST_ADULT) ? "an " : "a ") + STAGE_NAMES[s.stage] + "!");
    lifeStoryAgeUp(idx);
    // a little one who grew out of the nursery moves to the living room
    int room = -1;
    Actor* a = findActor(idx, &room);
    if (a && s.stage == ST_CHILD && room == R_NURSERY) {
        Actor copy = *a;
        auto& v = G.rooms[G.house][R_NURSERY].actors;
        for (size_t i = 0; i < v.size(); i++) if (v[i].kind == 0 && v[i].id == idx) { v.erase(v.begin() + i); break; }
        copy.y = 222;
        G.rooms[G.house][R_LIVING].actors.push_back(copy);
    }
}

// ------------------------------------------------------------------ activities
Activity makeAct(const char* name, float minutes, float speed) {
    Activity a;
    a.name = name; a.left = a.total = minutes; a.speed = speed;
    return a;
}
void startActivity(const Activity& a) {
    if (ACT.active()) cancelActivity();
    ACT = a;
}
void cancelActivity() {
    for (Actor& a : curRoom().actors) if (a.kind == 0 && a.id == G.mc) { a.sleeping = false; a.poseT = 0; }
    ACT = Activity();
}
float timeScale() {
    if (ACT.active()) return ACT.speed;
    return 2.f * worldSpeedMul();   // 2 game minutes per real second at Normal (a day is 12 real minutes)
}

static void completeActivity(const Activity& a);

// ------------------------------------------------------------------ careers
bool atWorkTime(const Sim& s) {
    if (s.career == CR_NONE || s.workedToday || !isWeekday(G.day)) return false;
    const CareerDef& c = CAREERS[s.career];
    int h = (int)(G.minute / 60);
    return h >= c.startHour - 1 && h < c.startHour + c.hours - 1;
}
void joinCareer(Sim& s, int career) {
    s.career = (uint8_t)career;
    s.careerLevel = (s.degree && career != CR_BARISTA && career != CR_DELIVERY && career != CR_HOSTESS) ? 1 : 0;
    s.performance = 0;
    toast(std::string("You got the job: ") + CAREERS[career].titles[s.careerLevel] + "!");
}
void finishShift(Sim& s, float hours, float bonus) {
    const CareerDef& c = CAREERS[s.career];
    int pay = (int)(c.pay[s.careerLevel] * hours);
    if (s.hasTrait(TR_GLAMOROUS) && fameStars() >= 2) pay += pay / 5;
    G.money += pay;
    if (&s == &MC()) celebShiftBonus();
    float perf = 12 + skillLevel(s, c.skill) * 2.f + bonus + (moodOf(s) > 20 ? 8 : moodOf(s) < -20 ? -8 : 0);
    if (s.hasTrait(TR_AMBITIOUS)) perf += 8;
    perf *= perkPerfMult();
    G.shifts++;
    if (s.hasTrait(TR_LAZY)) perf -= 4;
    s.performance += perf;
    s.workedToday = true;
    char b[96];
    snprintf(b, sizeof b, "Shift done! Earned $%d.", pay);
    toast(b);
    if (s.performance >= 100 && s.careerLevel < 4) {
        s.careerLevel++;
        s.performance = 0;
        toast(std::string("PROMOTED to ") + c.titles[s.careerLevel] + "!!"); if (&s == &MC()) rewardPopup("PROMOTION!", std::string("You are now ") + c.titles[s.careerLevel] + "!");
        if (&s == &MC()) addMemoryOnce(MEM_CAREER, G.mc, -1, std::string("Promoted to ") + c.titles[s.careerLevel] + "!");
        addMoodlet(s, "Promoted!", EM_CONFIDENT, 3, 480);
        fxBurst(200, 120, 30);
    }
}

static void completeActivity(const Activity& a) {
    {   // clumsy & perfectionist sims react to how things went
        Sim& me = MC();
        if (me.hasTrait(TR_CLUMSY) && a.skill >= 0 && irand(8) == 0) { toast("Oops! You fumbled it a little... (Clumsy)"); addMoodlet(me, "Oopsie", EM_PLAYFUL, 1, 90); }
        if (me.hasTrait(TR_PERFECTIONIST) && a.skill >= 0) { if (skillLevel(me, a.skill) >= 5) addMoodlet(me, "Nailed It", EM_CONFIDENT, 1, 120); else if (irand(3) == 0) addMoodlet(me, "Not Perfect Yet", EM_TENSE, 1, 90); }
    }
    Sim& s = MC();
    switch (a.kind) {
    case AK_WORK: finishShift(s, CAREERS[s.career].hours, 0); break;
    case AK_SCHOOL:
        s.schoolToday = true;
        s.gradePts = clampv(s.gradePts + (s.homework ? 6.f : 1.f) + (moodOf(s) > 0 ? 2.f : -1.f), 0.f, 100.f);
        toast("School's out! Current grade: " + letterGrade(s.gradePts));
        break;
    case AK_CLASS:
        s.credits += 1;
        if (!s.degree && s.credits >= 12) { s.degree = true; toast("You graduated college! Degree earned!"); addMemoryOnce(MEM_GOAL, G.mc, -1, "Graduated from Starlight College!"); addMoodlet(s, "Graduated!", EM_CONFIDENT, 3, 720); fxBurst(200, 100, 40); }
        else toast("Class done. Credits: " + std::to_string((int)s.credits) + "/12");
        break;
    case AK_HOMEWORK: s.homework = true; toast("Homework done!"); break;
    case AK_NOVEL: {
        int r = 20 + skillLevel(s, SK_CREATIVITY) * 25 + irand(40);
        G.money += r;
        toast("You sold a chapter of your novel for $" + std::to_string(r) + "!");
        break;
    }
    case AK_DINNER: if (G.date.active) G.date.score += 25; break;
    case AK_SLEEP: { int h = (int)(G.minute / 60); if (a.total >= 300 && h >= 5 && h < 12) seasonsGoodMorning(); break; }
    case AK_DANCE: if (G.date.active) G.date.score += 15; break;
    default: break;
    }
    for (Actor& ac : curRoom().actors) if (ac.kind == 0 && ac.id == G.mc) { ac.sleeping = false; ac.poseT = 0; }
}

// ------------------------------------------------------------------ world tick
static float decayRate(const Sim& s, int n) {
    static const float base[N_COUNT] = {6.f, 4.5f, 3.5f, 4.5f, 3.5f, 7.f};
    float r = base[n] * 0.6f;   // needs drain slower (player request)
    if (s.stage <= ST_TODDLER) r *= 1.3f;
    if (n == N_HUNGER && s.hasTrait(TR_FOODIE)) r *= 1.25f;
    if (n == N_ENERGY && s.hasTrait(TR_LAZY)) r *= 0.75f;
    if (n == N_HYGIENE && s.hasTrait(TR_NEAT)) r *= 0.6f;
    if (n == N_HYGIENE && s.hasTrait(TR_MESSY)) r *= 1.4f;
    if (n == N_SOCIAL && s.hasTrait(TR_OUTGOING)) r *= 1.3f;
    if (n == N_SOCIAL && s.hasTrait(TR_LONER)) r *= 0.5f;
    if (n == N_ENERGY && s.hasTrait(TR_NIGHTOWL)) { int h = (int)(G.minute / 60); r *= (h >= 20 || h < 3) ? 0.55f : (h >= 6 && h < 11) ? 1.35f : 1.f; }
    if (n == N_FUN && s.hasTrait(TR_GAMER) && (G.room == R_ARCADE)) r *= 0.5f;
    return r;
}

static void hourly(int hour);
static void daily();

static void stepMinutes(float m) {
    int prevHour = (int)(G.minute / 60);
    G.minute += m;
    if (G.minute >= 1440) { G.minute -= 1440; daily(); }
    int hour = (int)(G.minute / 60);
    if (hour != prevHour) hourly(hour);

    for (size_t i = 0; i < G.sims.size(); i++) {
        Sim& s = G.sims[i];
        if (!s.household) continue;
        bool isMC = (int)i == G.mc;
        for (int n = 0; n < N_COUNT; n++) {
            float d = -decayRate(s, n) * (isMC ? perkNeedMult(n) : 1.f);
            if (isMC && ACT.active()) {
                if (ACT.kind == AK_SLEEP && n == N_ENERGY) d = 0;
                if (ACT.kind == AK_SLEEP && n == N_HUNGER) d *= 0.4f;
                d += ACT.rate[n];
            }
            s.needs[n] = clampv(s.needs[n] + d * m / 60.f, 0.f, 100.f);
            if (!isMC && !s.isLittle()) {   // grown-ups look after themselves (per their autonomy setting)
                const int lv = l3AutoOf((int)i);
                if (lv == 2 && s.needs[n] < 45) s.needs[n] += 0.7f * m;
                else if (lv == 1 && s.needs[n] < 30) s.needs[n] += 0.5f * m;
                else if (lv == 0 && s.needs[n] < 10) s.needs[n] += 0.5f * m;   // critical needs still get handled
            }
        }
        for (size_t k = 0; k < s.moodlets.size();) {
            Moodlet& md = s.moodlets[k];
            if (md.minutes <= m) s.moodlets.erase(s.moodlets.begin() + k);
            else { md.minutes = (uint16_t)(md.minutes - m); k++; }
        }
    }
    if (ACT.active()) {
        if (ACT.skill >= 0) skillGain(MC(), ACT.skill, ACT.skillRate * m / 60.f);
        ACT.left -= m;
        bool done = ACT.left <= 0;
        if (ACT.kind == AK_SLEEP && MC().needs[N_ENERGY] >= 99 && hour >= 6 && hour < 12) done = true;
        if (ACT.kind == AK_SLEEP && !done) ACT.left = std::fmax(ACT.left, 5.f);
        if (done) {
            Activity fin = ACT;
            ACT = Activity();
            completeActivity(fin);
        }
    }
    if (G.postCooldown > 0) G.postCooldown -= m;
}

void worldTick(float dt) {
    float mins = std::fmin(dt * timeScale(), 30.f);
    while (mins > 0) {
        float st = std::fmin(mins, 1.f);
        stepMinutes(st);
        mins -= st;
    }
}

static int holidayOn(int day) {
    int d = day % 28;
    if (d == 18) return HOL_HALLOWEEN;   // Fall day 5: Spooky Day
    if (d == 25) return HOL_XMAS;        // Winter day 5: Winterfest
    return HOL_NONE;
}

static void hourly(int hour) {
    goalsHourly(hour);
    momentMaybeStart();
    dramaHourly(hour);
    romanceHourly(hour);
    lifeplusHourly(hour);
    lifeStoryHourly(hour);
    travelCourtHourly(hour);
    lockupHourly(hour);
    clubsHourly(hour); nightHourly(hour); hotdateHourly(); lxHourly(hour); dlHourly(hour); seHourly(hour); famHourly(hour); kidHourly(hour); guHourly(hour); dlyHourly(hour); choiceHourly(hour); lpHourly(hour); lvlHourly(hour); l3Hourly(hour);
    healthHourly(hour);
    economyHourly(hour);
    statusHourly(hour);
    lovebugHourly(hour);
    lifeHourly(hour);
    kidsHourly(hour);
    seasonsHourly(hour);
    familyHourly(hour);
    chatHourly(hour);
    Sim& me = MC();
    // reminders
    if (me.career != CR_NONE && isWeekday(G.day) && !me.workedToday && hour == CAREERS[me.career].startHour - 1)
        toast(std::string("Work at ") + ROOM_NAMES[CAREERS[me.career].room] + " soon! Open the Map.");
    if (hour == 8) eventMorningNotice();
    npcRelTick();
    if (me.stage >= ST_CHILD && me.stage <= ST_TEEN && isWeekday(G.day) && hour == 7 && !me.schoolToday)
        toast(std::string(schoolName(me.stage)) + " starts at 8 AM! Open the Map > School.");
    for (Event& e : G.events) {
        if (e.done || e.day != G.day) continue;
        if (e.hour == hour) {
            if (e.type == EV_DATE) toast("Date time! Meet " + G.sims[e.sim].name + " at " + ROOM_NAMES[e.loc] + ".");
            else toast("Planner: " + e.text + " now!");
        }
    }
    // babies need you
    for (size_t i = 0; i < G.sims.size(); i++) {
        Sim& s = G.sims[i];
        if (s.household && s.isLittle() && (s.needs[N_HUNGER] < 25 || s.needs[N_FUN] < 20) && hour % 2 == 0)
            toast(s.name + " is fussy! Feed or play with them.");
    }
    if (irand(100) < 45) townieChatter();
    dramaTick();
    // random texts from people you know
    if (hour >= 9 && hour <= 23 && irand(100) < 14) {
        std::vector<int> known;
        for (const Rel& r : G.rels) {
            int other = r.a == G.mc ? r.b : r.b == G.mc ? r.a : -1;
            if (other < 0 || G.sims[other].household) continue;
            if (r.met && (r.fr > 10 || r.ro > 10 || r.matched)) known.push_back(other);
        }
        if (!known.empty()) npcTexts(known[irand((int)known.size())]);
    }
    if (G.date.active && hour >= 23) endDate();
}

static void daily() {
    G.day++;
    goalsDaily();
    gardenDaily();
    crimeDaily();
    lockupDaily();
    galleryDaily();
    contentDaily();
    clubsDaily(); nightDaily(); spicyDaily(); lxDaily(); dlDaily(); famDaily(); kidDaily(); guDaily(); dlyDaily(); dyDaily(); choiceDaily(); tpDaily(); traitsDaily(); lpDaily(); farmDaily(); lvlDaily(); l3Daily();
    storyEventsDaily();
    dramaDaily();
    seasonsDaily();
    familyDaily();
    lifeDaily();
    // sleepover guests and visitors head home in the morning
    for (int h = 0; h < 2; h++) for (int r = 0; r < R_COUNT; r++) {
        if (!roomIsHome(r)) continue;
        auto& v = G.rooms[h][r].actors;
        v.erase(std::remove_if(v.begin(), v.end(), [](const Actor& a) { return a.kind == 0 && a.id < G.sims.size() && !G.sims[a.id].household && (int)a.id != G.mc; }), v.end());
    }
    // pregnancies
    for (size_t i = 0; i < G.sims.size(); i++) {
        Sim& s = G.sims[i];
        if (s.pregnant) {
            if (s.pregDays > 0) s.pregDays--;
            if (s.pregDays == 1 && s.household) { addMoodlet(s, "Due Tomorrow!", EM_TENSE, 1, 1440); toast(s.name.substr(0, s.name.find(' ')) + "'s baby is due tomorrow - hospital bag is packed!"); }
            if (s.pregDays == 0) {
                s.pregnant = false;
                int baby = makeBaby((int)i, partnerOf((int)i));
                if (s.household) lxBirth((int)i, baby); else toast("It's a baby! Welcome " + G.sims[baby].name + "! (Nursery)");
                addMoodlet(s, "New Baby!", EM_HAPPY, 3, 1440);
            }
        }
    }
    // aging + daily resets (household)
    for (size_t i = 0; i < G.sims.size(); i++) {
        Sim& s = G.sims[i];
        if (!s.household) continue;
        if (s.career != CR_NONE && isWeekday(G.day - 1) && !s.workedToday && (int)i == G.mc) {
            s.performance -= 15;
            toast("You skipped work yesterday... performance dropped.");
        }
        if ((int)i != G.mc && s.career != CR_NONE && isWeekday(G.day - 1)) { s.performance += 10; G.money += CAREERS[s.career].pay[s.careerLevel] * CAREERS[s.career].hours; }
        if (s.stage >= ST_CHILD && s.stage <= ST_TEEN && isWeekday(G.day - 1)) {
            if ((int)i == G.mc && !s.schoolToday) { s.gradePts -= 8; toast("You skipped school! Grade dropped."); }
            if ((int)i != G.mc) s.gradePts = clampv(s.gradePts + 1.f, 0.f, 100.f);
        }
        s.workedToday = s.schoolToday = s.homework = false;
        if (s.performance >= 100 && s.careerLevel < 4) { s.careerLevel++; s.performance = 0; }
        s.performance = clampv(s.performance, -50.f, 100.f);
        if (G.aging && STAGE_DAYS[s.stage] != 255 && ++s.stageDays >= STAGE_DAYS[s.stage]) ageUp((int)i);
    }
    economyDaily();
    lifeStoryDaily();
    worldSimDaily();
    // bills on Sundays
    if (weekday(G.day) == 6) {
        int items = 0;
        for (int r = 0; r < R_COUNT; r++) items += (int)G.rooms[G.house][r].items.size();
        economyIssueBills(items, householdCount());
    }
    // fame perks
    int stars = fameStars();
    if (stars > 0) {
        int sp = stars * stars * 40;
        G.money += sp;
        toast("Sponsorship money: $" + std::to_string(sp) + "!");
    }
    int hol = holidayOn(G.day);
    if (hol) {
        G.holiday = hol;
        toast(hol == HOL_HALLOWEEN ? "Happy Spooky Day! Costumes on!" : "Happy Winterfest! Decorate the tree!");
        addMoodlet(MC(), "Holiday Spirit", EM_HAPPY, 2, 1440);
    } else if (holidayOn(G.day - 1)) G.holiday = HOL_NONE;
    if (G.date.active) endDate();
    // remove stale events
    G.events.erase(std::remove_if(G.events.begin(), G.events.end(), [](const Event& e) { return e.day + 1 < G.day; }), G.events.end());
}

// ------------------------------------------------------------------ town
bool townieFits(int room, const Sim& s) {
    if (s.household) return false;
    if (room == R_SCHOOL) return s.stage >= ST_CHILD && s.stage <= ST_TEEN;
    if (room == R_COLLEGE) return s.stage == ST_YA;
    if (room == R_CLUB) return s.stage >= ST_YA && s.stage <= ST_MIDDLE;
    return s.stage >= ST_CHILD;
}
void spawnTownies(int room) {
    RoomState& rs = G.rooms[G.house][room];
    std::vector<Actor> keep;
    for (const Actor& a : rs.actors) if (a.kind == 0 && G.sims[a.id].household) keep.push_back(a);
    rs.actors = keep;
    if (room == R_NEIGHBOR) return;   // only your host is home (district.cpp)
    // whoever's routine has them here right now comes first (work shifts, school, lunch,
    // hobbies, date night...), then a couple of drop-ins so nowhere is ever a ghost town
    std::vector<int> pool, extra;
    const int hour = (int)(G.minute / 60);
    for (size_t i = 0; i < G.sims.size(); i++) {
        if (!townieFits(room, G.sims[i]) || lifeAway((int)i) || simDead((int)i) || simGone((int)i)) continue;
        int w = lifeWhere((int)i, hour);
        if (w == room) pool.push_back((int)i); else if (w != -2) extra.push_back((int)i);
    }
    int n = 3 + irand(2) + (fameStars() >= 3 ? 1 : 0);
    int drop = std::max(0, std::min(n - (int)pool.size(), 1 + irand(2)));
    if ((int)pool.size() < 2) drop = std::max(drop, 2);
    n = std::min(n, (int)pool.size() + drop);
    for (int k = 0; k < n && (!pool.empty() || !extra.empty()); k++) {
        std::vector<int>& src = pool.empty() ? extra : pool;
        int pick = irand((int)src.size());
        int idx = src[pick];
        src.erase(src.begin() + pick);
        Actor a; a.kind = 0; a.id = (uint16_t)idx;
        a.x = 40 + irand(320); a.tx = a.x; a.y = 186 + irand(48);
        a.stay = (float)(60 + irand(200));   // they'll head off eventually; new people wander in
        rs.actors.push_back(a);
        if (k == 0 && fameStars() >= 3) { rs.actors.back().say = "OMG is that " + MC().name + "?!"; rs.actors.back().sayT = 3; }
    }
}

void travelTo(int room) {
    { std::string why; if (lockupBlocks(room, &why)) { toast(why); sfxNo(); return; } }
    if (!(room == R_JAIL || room == R_BOARDING) && !familyTravelGate(room)) return;
    if (room == G.room) return;
    if (ACT.active()) cancelActivity();
    if (G.date.active && G.room == G.date.loc && room != G.room) endDate();
    // take the MC (and any toddler-free followers) out of the old room
    Actor me;
    bool found = false;
    for (int r = 0; r < R_COUNT && !found; r++) {
        auto& v = G.rooms[G.house][r].actors;
        for (size_t i = 0; i < v.size(); i++)
            if (v[i].kind == 0 && v[i].id == G.mc) { me = v[i]; v.erase(v.begin() + i); found = true; break; }
    }
    if (!found) { me.kind = 0; me.id = (uint16_t)G.mc; }
    bool longTrip = !(roomIsHome(room) && roomIsHome(G.room));
    if (!roomIsHome(G.room)) {   // leaving a town spot: the crowd disperses
        auto& v = G.rooms[G.house][G.room].actors;
        v.erase(std::remove_if(v.begin(), v.end(), [](const Actor& a) { return a.kind == 0 && !G.sims[a.id].household; }), v.end());
    }
    G.room = room;
    me.x = 200; me.tx = 200; me.y = 222; me.sleeping = false; me.poseT = 0; me.say.clear(); me.sayT = 0;
    me.seat = -1; me.useKind = 0; me.walking = false; me.goSeat = -1;
    G.rooms[G.house][room].actors.push_back(me);
    if (!roomIsHome(room)) spawnTownies(room);
    if (longTrip) worldTick(20.f / timeScale());   // ~20 minutes of travel
    startDateIfDue();
    eventArrive(room);
    weatherArrive(room);
    goalNotify(GE_TRAVEL, room);
    if (!roomIsHome(room)) for (const Actor& a : G.rooms[G.house][room].actors) if (a.kind == 0 && a.id != G.mc) squadGreet(a.id);
    // a friendly local might come say hi
    if (!roomIsHome(room) && irand(100) < 40) {
        for (const Actor& a : G.rooms[G.house][room].actors) {
            if (a.kind != 0 || G.sims[a.id].household) continue;
            const Rel* r = findRel(G.mc, a.id);
            if (r && r->met) continue;
            static const char* hi[] = {
                "Hey! You're new around here, right? I love your look. We should totally hang out sometime!",
                "OMG hiii! Is that a new outfit? You HAVE to tell me where you got it.",
                "How's it going? I'm always here, so come say hi whenever. Maybe we can grab a smoothie later?",
                "Hey there! Everybody's been talking about you. All good things... mostly. Ha!",
                "Hi! Don't be shy, come meet everyone. This place is way more fun with friends!"};
            dlgSay(a.id, hi[irand(5)]);
            Rel& rr = rel(G.mc, a.id); rr.met = true; rr.fr = (int8_t)std::max<int>(rr.fr, 5);
            break;
        }
    }
    crimeOnTravel(room);
    lifeOnTravel(room);
    familyOnArrive(room);
}
