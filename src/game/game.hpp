#pragma once
#include "common.hpp"
#include "people.hpp"

// ---------------- places ----------------
enum RoomType : int {
    R_LIVING, R_KITCHEN, R_BEDROOM, R_CLOSET, R_NURSERY, R_BASEMENT, R_YARD,      // home
    R_BEACH, R_SCHOOL, R_CLUB, R_RESTAURANT, R_BOUTIQUE, R_COLLEGE, R_CAFE, R_SPA, // town
    R_MALL, R_PARK, R_CINEMA, R_RINK,                                             // town (2.0)
    R_MALL_BEAUTY, R_ARCADE, R_BAKERY, R_PLAYGROUND, R_DINER, R_SUSHI, R_STREET, R_NEIGHBOR,   // districts (Stick Figure Ver.)
    R_HOSPITAL, R_PETSHOP, R_LIBRARY, R_GYM, R_HOTEL, R_CHAPEL,   // Uptown
    R_MARKET,                                                    // Sugar Market (save v22)
    R_BEDROOM2, R_BATHROOM, R_STUDY, R_TEENROOM,                 // extra home rooms (save v29)
    R_CORNER, R_TENNIS, R_THRIFT,                                // new places (save v29)
    R_HAUNTED, R_FRAT,                                           // Old Town extras (save v30)
    R_ISLAND, R_ISLANDTOWN, R_SLOPES, R_LODGE, R_BOULEVARD, R_TOWER,   // vacation worlds (save v33)
    R_JAIL, R_BOARDING,                                          // county jail & Rosewood Academy (save v37)
    R_COVESUITE, R_COVESIGHTS, R_PEAKSUITE, R_PEAKSIGHTS, R_BELLESUITE, R_BELLESIGHTS,   // hotel suites & sightseeing per trip (save v38)
    R_HOTELLOBBY, R_HOTELBATH, R_HOTELPOOL,
    R_GLOWBOWL, R_KARAOKE, R_PAINTSIP, R_DANCESTUDIO, R_NIGHTMKT, R_SERENITY,   // Neon Strip venues (save v44)
    R_DOWNTOWN, R_DTBOUTIQUE, R_DTBISTRO, R_VELVET,
    R_FUNPARK, R_FUNRIDES,
    R_SUBWAY, R_PIXELARCADE, R_LAUNDRO, R_AQUARIUM, R_MUSEUM, R_GARDENS, R_ROOFTOP,   // Metro lines (save v48)
    R_FOODCOURT, R_HALLWAY, R_CLASSROOM, R_CAFETERIA, R_SCHOOLGYM, R_DAYCARE, R_CHURCH, R_COMMUNITY, R_POOL, R_BLOCKPARTY, R_DORM,   // Town 2 (save v49)
    R_SALON, R_ROLLER, R_NIGHTMARKET, R_COURTHOUSE,   // Daily Life (save v50)
    R_AVSQUARE, R_AVFARM, R_AVBARN,                   // Apple Valley farm country (save v51)
                                         // Sparkle Land (save v47)                          // Hot Date downtown (save v45)                       // trip hotel rooms, themed per destination (save v39)
    R_COUNT
};
constexpr int R_COUNT_V47 = R_FUNRIDES + 1;
constexpr int R_COUNT_V48 = R_ROOFTOP + 1;
constexpr int R_COUNT_V49 = R_DORM + 1;
constexpr int R_COUNT_V50 = R_COURTHOUSE + 1;
constexpr int R_COUNT_V46 = R_VELVET + 1;
constexpr int R_COUNT_V45 = R_DTBISTRO + 1;
constexpr int R_COUNT_V44 = R_SERENITY + 1;
constexpr int R_COUNT_V43 = R_HOTELPOOL + 1;
constexpr int R_COUNT_V38 = R_BELLESIGHTS + 1;
constexpr int R_COUNT_V37 = R_BOARDING + 1;
constexpr int R_COUNT_V36 = R_TOWER + 1;   // rooms stored by saves v33..v36
constexpr int R_COUNT_V32 = R_FRAT + 1;   // rooms stored by saves v30..v32
constexpr int R_COUNT_V29 = 41;
constexpr int R_COUNT_V28 = 34;   // rooms stored by saves v22..v28
constexpr int R_COUNT_V21 = 33;   // rooms stored by saves v14..v21
constexpr int R_COUNT_V12 = 19;
constexpr int R_COUNT_V13 = 27;
constexpr int R_COUNT_V5 = 15;   // rooms stored by older saves
extern const char* ROOM_NAMES[R_COUNT];
extern const char* HOUSE_NAMES[2];
inline bool roomIsOuting(int r) { return r >= R_YARD && !(r >= R_BEDROOM2 && r <= R_TEENROOM); }   // uses a fixed backdrop
inline bool roomIsHome(int r) { return r <= R_YARD || (r >= R_BEDROOM2 && r <= R_TEENROOM); }

enum Holiday : int { HOL_NONE, HOL_BIRTHDAY, HOL_HALLOWEEN, HOL_XMAS };

// ---------------- sims ----------------
enum Stage : uint8_t { ST_INFANT, ST_TODDLER, ST_CHILD, ST_PRETEEN, ST_TEEN, ST_YA, ST_ADULT, ST_MIDDLE, ST_ELDER, ST_COUNT };
enum Need : int { N_HUNGER, N_ENERGY, N_HYGIENE, N_FUN, N_SOCIAL, N_BLADDER, N_COUNT };
constexpr int N_COUNT_V6 = 5;
enum Skill : int { SK_STYLE, SK_COOKING, SK_CHARISMA, SK_FITNESS, SK_CREATIVITY, SK_LOGIC, SK_COUNT };
enum Trait : uint8_t {
    TR_FASHIONISTA, TR_CHEERFUL, TR_ROMANTIC, TR_BOOKWORM, TR_FOODIE, TR_CREATIVE, TR_OUTGOING, TR_LONER,
    TR_AMBITIOUS, TR_LAZY, TR_ACTIVE, TR_NEAT, TR_GOOFBALL, TR_GLAMOROUS, TR_FAMILY, TR_HOTHEAD,
    TR_MUSIC, TR_CLUMSY, TR_SNEAKY, TR_JEALOUS, TR_SELFASSURED, TR_SNOB, TR_PETLOVER, TR_NIGHTOWL, TR_GAMER, TR_PERFECTIONIST, TR_KIND, TR_MEAN,
    TR_LOYAL, TR_GENEROUS, TR_FRUGAL, TR_OPTIMIST, TR_DRAMATIC, TR_MESSY, TR_SHOPAHOLIC, TR_INSECURE,
    TR_FLIRTY, TR_DIVA, TR_BLACKMAILER, TR_CHARMING, TR_BRAVE, TR_FARMER, TR_PICKME, TR_FUCKBOY, TR_SLUT, TR_GIRLYGIRL, TR_TOMBOY, TR_HOUSEWIFE, TR_PASTOR, TR_SCAMMER, TR_SHY, TR_SILVERSPOON, TR_BITCH, TR_VAIN, TR_VEGAN, TR_BOXER, TR_DANCER, TR_HOOPS, TR_FANGIRL, TR_FANBOY, TR_NERD, TR_MAKEUP, TR_TRAPSTAR, TR_BOUGIE, TR_SHADY, TR_PLUG, TR_QUICKTEMPER, TR_HOODRAT,   // page 3+ traits (traits2.cpp)
    TR_COUNT,
    TR_NONE = 255
};
enum Emotion : uint8_t { EM_FINE, EM_HAPPY, EM_FLIRTY, EM_CONFIDENT, EM_INSPIRED, EM_PLAYFUL, EM_ENERGIZED, EM_SAD, EM_ANGRY, EM_TENSE, EM_BORED, EM_FOCUSED, EM_EMBARRASSED, EM_UNCOMFORTABLE, EM_COUNT };
enum Career : uint8_t { CR_NONE, CR_STYLIST, CR_CHEF, CR_BARISTA, CR_HOSTESS, CR_DELIVERY, CR_TEACHER, CR_RIDESHARE, CR_MODEL, CR_DOCTOR, CR_WRITER, CR_ACTOR, CR_BUSINESS, CR_GARDENER, CR_INFLUENCER, CR_CELEBRITY, CR_COUNT };

extern const char* STAGE_NAMES[ST_COUNT];
extern const uint8_t STAGE_DAYS[ST_COUNT];
extern const char* NEED_NAMES[N_COUNT];
extern const char* SKILL_NAMES[SK_COUNT];
extern const char* TRAIT_NAMES[TR_COUNT];
extern const char* TRAIT_DESC[TR_COUNT];
extern const char* EMOTION_NAMES[EM_COUNT];
extern const RGB EMOTION_COLORS[EM_COUNT];

struct CareerDef {
    const char* name; int room; int startHour, hours; uint8_t skill; bool teenOk;
    const char* titles[5]; int pay[5];   // pay per hour
};
extern const CareerDef CAREERS[CR_COUNT];

struct Moodlet { std::string name; uint8_t emotion; int8_t power; uint16_t minutes; };

struct Sim {
    std::string name, bio;
    PersonDef look{};
    Stage stage = ST_YA;
    uint16_t stageDays = 0;
    uint8_t traits[3] = {TR_NONE, TR_NONE, TR_NONE};
    uint8_t favFood = 0, favColor = 0, zodiac = 0;   // Sims-3-style favorites (save v28)
    float needs[N_COUNT] = {80, 80, 80, 80, 80, 80};
    float skills[SK_COUNT] = {0, 0, 0, 0, 0, 0};
    bool household = false;
    uint8_t interest = 2;          // 0 guys, 1 girls, 2 everyone
    uint8_t outfit = 0;
    uint8_t career = CR_NONE, careerLevel = 0;
    float performance = 0;
    bool workedToday = false;
    bool enrolled = false;
    float credits = 0;
    bool degree = false;
    float gradePts = 70;           // 0..100 -> letter grade
    bool homework = false, schoolToday = false;
    bool pregnant = false;
    uint8_t pregDays = 0;
    std::vector<Moodlet> moodlets;
    bool hasTrait(uint8_t t) const { return traits[0] == t || traits[1] == t || traits[2] == t; }
    bool isAdult() const { return stage >= ST_YA; }
    bool isLittle() const { return stage <= ST_TODDLER; }
    bool isGirl() const { return !look.guy; }
};

enum RelStatus : uint8_t { RS_NONE, RS_DATING, RS_ENGAGED, RS_MARRIED, RS_FAMILY };
extern const char* REL_STATUS_NAMES[5];
struct Rel { uint16_t a = 0, b = 0; int8_t fr = 0, ro = 0; uint8_t status = RS_NONE; bool matched = false; bool met = false; };

struct Msg { bool me; std::string text; };
struct ChatThread { uint16_t sim = 0; std::vector<Msg> msgs; int16_t prompt = -1; bool unread = false; int16_t about = -1; };

enum EvType : uint8_t { EV_BIRTHDAY, EV_HOLIDAY, EV_DATE, EV_PLAN, EV_BILLS };
struct Event { uint16_t day = 0; uint8_t hour = 0; uint8_t type = EV_PLAN; int16_t sim = -1; uint8_t loc = 0; std::string text; bool done = false; };

struct Post { int16_t sim = -1; std::string text; uint32_t likes = 0; uint8_t kind = 0; };
struct Plot { uint8_t crop = 0, stage = 0, watered = 0, health = 3; };   // garden plot: crop 0 = empty, 1..6
struct Memory { uint16_t day = 0; uint8_t type = 0; int16_t a = -1, b = -1; std::string text; };   // scrapbook page
struct Grudge { uint16_t sim = 0; uint8_t days = 0; };
struct Dirt { uint16_t sim = 0; uint8_t secret = 0, state = 0; uint16_t day = 0; };   // a secret you know: 0 known, 1 paying you off, 2 exposed, 3 kept safe
// STATUS (in-game social network): who < 0 is a special account (see status.hpp)
struct StReply { int16_t who = -1; uint8_t tone = 0; std::string text; };
struct StPost {
    int16_t who = -1, about = -1, about2 = -1;
    uint8_t kind = 0, tag = 0, cred = 3, flags = 0, heat = 0, ttl = 0;
    uint16_t day = 0, minute = 0, reposts = 0;
    uint32_t likes = 0;
    std::string text;
    std::vector<StReply> replies;
};
// Story mode: "From Intern to Icon" (story.cpp)
struct StoryState {
    uint8_t ch = 0, step = 0;            // chapter 0..7, 8 = finished
    int8_t boss = 0;                     // Celestine's approval -100..100
    int32_t debt = 0;                    // the Platinum Pink card
    uint32_t flags = 0;
    uint8_t theme = 0, pal = 0, label = 0, ending = 0, bought = 0;
    uint8_t looks[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};   // 3 looks x (top, bottom, shoes)
    uint16_t score = 0;
};
// the life engine (life.cpp, save v17)
struct LifeMemory { uint8_t kind = 0; int16_t other = -1; uint16_t day = 0; };
struct NpcLife { uint8_t goalProg = 0, goalsDone = 0, hobbyLvl = 0, away = 0, kids = 0; uint16_t sinceDay = 0; LifeMemory mem[4];
                 // raising kids (kids.cpp, save v18): development 0..100, milestones bitmask, parenting style they grew up with
                 uint8_t motor = 0, talk = 0, potty = 0, imag = 0, social = 0, disc = 0; uint16_t miles = 0; int8_t warm = 0, strict = 0;
                 uint8_t chore = 0, choresWeek = 0, grounded = 0; };   // family planner (family.cpp, save v19)
// Sugar Hearts: Otome Ver. (otome.cpp, save v18)
struct OtomeState { int8_t aff[7] = {0, 0, 0, 0, 0, 0, 0}; uint8_t ch[7] = {0, 0, 0, 0, 0, 0, 0}; uint8_t ended[7] = {0, 0, 0, 0, 0, 0, 0}; uint8_t bonus = 0; };
// Legacy Challenges (legacy.cpp, save v21)
struct LegacyState { int8_t active = -1; uint8_t gen = 1, elim = 0, view = 0; uint32_t done = 0, cleared = 0; uint16_t startDay = 0; int16_t lover = -1;
                     uint64_t cleared2 = 0; uint8_t homeAtStart = 0, tripsAtStart = 0; };   // v41: cards 32+, gallery/trip goals
// your love life (romance.cpp, save v23)
struct LoveState {
    int16_t situ = -1, rival = -1, confront = -1;   // situationship, romantic rival, the partner who heard you cheated
    uint16_t situDay = 0, lastRomDay = 0, annivDay = 0, evDay = 0xFFFF;
    uint8_t trust = 70, spark = 60, situDodges = 0;
    std::vector<uint16_t> exes, exDay, dated;
};
// life story (lifestory.cpp, save v31): milestones, deaths, adoption
struct MilestoneRec { uint16_t sim = 0, day = 0; uint8_t id = 0; int16_t other = -1; };
// Movie Maker (moviemaker.cpp, save v31): series > episodes > scenes > lines
struct MovieLine { int8_t who = -1; uint8_t emo = 0, act = 0; std::string text; };   // who: -1 narrator, 0..2 cast slot
struct MovieActor { int16_t sim = -1; uint8_t pos = 1, act = 0, outfit = 0xFF; };
struct MovieScene { std::string title; uint8_t loc = 0, cam = 0, trans = 1, dur = 4; MovieActor cast[3]; std::vector<MovieLine> lines; };
struct MovieEp { std::string title; std::vector<MovieScene> scenes; };
struct MovieSeries { std::string title; std::vector<MovieEp> eps; };
// Starlight Studios director mode (director.cpp, save v25)
struct MovieState { uint8_t stars[16] = {0}; uint16_t jobDay[4] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}; uint16_t made = 0; };
// lifeplus.cpp (save v26): genetics, packages, sentiments, lot traits
struct Lineage { uint16_t kid = 0; int16_t pa = -1, pb = -1; };
struct Package { uint8_t kind = 0; uint16_t day = 0; uint8_t hour = 10; };
struct Sentiment { uint16_t sim = 0; uint8_t kind = 0; uint16_t until = 0; };
// economy.cpp (save v27): bills & utilities, budget, food delivery, groceries, porch packages
enum BillItem { BI_RENT, BI_ELEC, BI_WATER, BI_NET, BI_PHONE, BI_TRASH, BI_SUBS, BI_INSURE, BI_LATE, BI_COUNT };
enum SpendCat { EC_BILLS, EC_GROCERY, EC_TAKEOUT, EC_SHOPPING, EC_FUN, EC_INCOME, EC_COUNT };
struct Bill { uint16_t issued = 0, due = 0; int16_t item[BI_COUNT] = {0}; bool paid = true; };
struct FoodOrder { int8_t rest = -1; uint8_t qty[3] = {0, 0, 0}; uint32_t placed = 0; uint8_t eta = 40, tip = 0; int16_t total = 0; };
struct Economy {
    uint32_t elecUse = 0, waterUse = 0;
    uint8_t subs = 0; bool autopay = false;
    Bill bill; int16_t history[4] = {0, 0, 0, 0};
    bool powerOff = false, waterOff = false, netOff = false;
    int32_t spend[EC_COUNT] = {0}, lastSpend[EC_COUNT] = {0}; int32_t weekStartMoney = -1;
    uint8_t groceryQ = 1; uint16_t groceryDay = 0; bool autoGrocery = false; uint8_t ordersWeek = 0;
    std::vector<uint8_t> porch;
    FoodOrder order;
};
struct Scandal { int16_t sim = -1; uint8_t kind = 0, extra = 0; uint16_t day = 0; uint8_t handled = 0; };   // a headline in The Tea

// ---------------- rooms / world ----------------
struct Placed { uint16_t item; int16_t x, y; };

struct Actor {
    uint8_t kind = 0;          // 0 sim, 1 pet
    uint16_t id = 0;           // sim index or pet index
    float x = 200, y = 200, tx = 200;
    float wait = 0;
    float poseT = 0;
    uint8_t pose = 0;
    bool facingLeft = false;
    bool sleeping = false;
    std::string say;           // speech bubble
    float sayT = 0;
    float stay = -1;           // visitors: game minutes until they go home
    // movement & furniture use (not saved)
    float ty = 0;              // walk target y (0 = none)
    bool walking = false;
    float walkPhase = 0;
    int16_t seat = -1;         // item index being used (sitting / lying / bathing)
    uint8_t seatSlot = 0;
    uint8_t useKind = 0;       // 0 none, 1 sit, 2 lie, 3 bathe
    float useT = 0;            // NPC: seconds left on the furniture
    int16_t goSeat = -1;       // heading to this furniture
    uint8_t goSlot = 0;
    bool goSleep = false;
    uint8_t passer = 0;        // town foot traffic: 1 strolling past, 2 heading off
};

struct RoomState {
    std::vector<Placed> items;
    std::vector<Actor> actors;
    uint8_t wall = 0, wallPat = 0, floor = 0, floorPat = 0;
};

struct DiaryEntry { std::string date, text; };
struct FoodItem { uint8_t recipe = 0, quality = 1, servings = 1; uint16_t day = 0; };   // v40 cooked dishes (content pack)
struct GiftLog { int16_t sim = -1; uint8_t item = 0, result = 0; uint16_t day = 0; };

constexpr int NUM_MINIGAMES = 10;
constexpr int NUM_MINIGAMES_V5 = 6;

struct DateState { bool active = false; int16_t partner = -1; float score = 0; uint8_t loc = R_RESTAURANT; uint16_t endDay = 0; };

struct Game {
    // home
    int house = 0, room = R_LIVING;
    RoomState rooms[2][R_COUNT];
    int holiday = HOL_NONE;
    bool lightsOn = true;
    int track = 0;
    std::vector<DiaryEntry> diary;
    std::string diaryCode;
    uint32_t dishes = 0;
    int32_t best[NUM_MINIGAMES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint32_t eventsEntered = 0;          // bitmask: contests entered today
    uint16_t eventsDay = 0;
    uint16_t trophies = 0;
    int16_t rep = 0;                    // reputation -100 (mean girl) .. 100 (town darling)
    uint8_t cliqueStand[5] = {20, 20, 20, 20, 20};   // standing with each squad
    int8_t clique = -1;                 // squad you're in
    uint16_t squadAskDay = 0xFFFF;
    int8_t club = -1;                   // school club (kids & teens)
    std::vector<uint8_t> pets;          // owned pets (by pet index)
    // life sim
    std::vector<Sim> sims;
    int mc = 0;                         // the sim you are controlling
    std::vector<Rel> rels;
    std::vector<ChatThread> threads;
    std::vector<Event> events;
    std::vector<Post> posts;
    int32_t money = 2500;
    uint32_t followers = 0;
    uint16_t day = 0;
    float minute = 8 * 60;              // 0..1440
    bool aging = true;
    bool pregnancyOn = true;
    float postCooldown = 0;
    uint64_t ownedOutfits = 0x31;       // bitmask of OUTFITS 0..63
    uint64_t ownedOutfits2 = 0;         // OUTFITS 64.. (save v23)
    uint64_t ownedOutfits3 = 0, ownedOutfits4 = 0;
    uint8_t hidePurse = 0, crushBack = 0; uint16_t toys = 0; int16_t crush = -1;   // Kid Life (saved in .lxe)   // OUTFITS 128..255 (saved in the .lxe sidecar)
    DateState date;
    uint16_t lastBillsDay = 0;
    // goals, rewards, collections, weather (save v10)
    int8_t asp = -1;                    // chosen aspiration
    uint8_t aspLevel[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    uint32_t satisfaction = 0;          // reward points
    uint32_t perks = 0;
    uint8_t wantType[3] = {255, 255, 255};
    uint16_t wantsDay = 0xFFFF;
    uint32_t collected = 0;             // 4 sets x 6 bits
    uint8_t weather = 0;
    uint16_t weatherDay = 0xFFFF;
    uint16_t shifts = 0, postsMade = 0, datesHad = 0, momentsSeen = 0;
    // garden, gifts & scrapbook (save v11)
    Plot plots[6];
    uint8_t basket[7] = {0, 0, 0, 0, 0, 0, 0};
    std::vector<Memory> memories;
    // crime, scandal & drama (Stick Figure Ver., save v12)
    uint8_t heat = 0;                    // wanted level 0..100 (5 stars)
    uint16_t rapSheet = 0, crimes = 0, heists = 0;
    float mischief = 0;                  // sneakiness skill 0..100
    int16_t witness = -1;                // a townie who saw your last crime...
    uint8_t witnessCrime = 0;
    uint16_t witnessDay = 0;
    std::vector<Dirt> dirt;
    std::vector<Scandal> tea;
    uint8_t myScandal = 0, myScandalKind = 0;
    uint16_t damageDay = 0xFFFF, heistDay = 0xFFFF, burgleDay = 0xFFFF;
    uint16_t csSeen = 0;                 // cutscenes unlocked
    bool alarm = false;                  // home security
    int16_t cop = -1;                    // Officer Reyes
    // drama & social life (save v15)
    std::vector<Grudge> grudges;
    std::vector<uint16_t> impressed, bffs;
    int8_t luck = 0;                     // 1 lucky day, -1 bad hair day
    uint16_t allowanceDay = 0xFFFF, dramaDay = 0xFFFF;
    int16_t affair = -1, affairBff = -1; // secret romance with your BFF's boyfriend
    uint8_t affairRisk = 0;
    // STATUS social network (save v16)
    std::vector<StPost> st;
    std::vector<uint16_t> stFollow, stBlock;
    bool stPrivate = false;
    int16_t stClout = 0;                 // -100 (problematic) .. 100 (beloved)
    uint16_t stPostsMade = 0;
    StoryState story;
    // LOVEBUG dating app (save v16)
    std::vector<uint16_t> lbLikes, lbBlock;
    uint8_t lbIntent = 1, lbFilter = 0, lbTheme = 0, lbStatus = 0;
    uint16_t lbMatches = 0;
    bool lbDeleted = false;
    // life engine (save v17)
    std::vector<NpcLife> life;
    uint8_t lifeEv = 0, lifeEvRoom = 0;
    int16_t lifeEvA = -1, lifeEvB = -1;
    uint16_t lifeEvDay = 0xFFFF;
    // seasonal festivals (save v18): bit per festival activity done today
    uint32_t festDone = 0;
    OtomeState oto;
    // family planner (family.cpp, save v19)
    uint8_t curfew = 22, famEvents = 0, allowance = 0, mcGrounded = 0;
    int16_t sneaker = -1;   // a household teen who's out past curfew right now
    bool mcSneaking = false;
    LegacyState leg;
    uint8_t groceries = 0;
    LoveState love;
    bool cookTutDone = false;   // cooking tutorial (save v24)
    uint16_t recipes = 0;      // recipe combos discovered
    MovieState movie;
    std::vector<Lineage> lineage;
    std::vector<Package> packages;
    std::vector<Sentiment> sentiments;
    uint16_t lotTraits = 0;
    Economy eco;
    std::vector<MilestoneRec> milestones;
    std::vector<uint16_t> adopted;
    std::vector<uint8_t> deathType, sickDays;   // per sim (0 = alive / healthy)
    std::vector<uint8_t> illness, diag;          // health.cpp (save v32)
    std::string hhName; uint16_t coinJarDay = 0xFFFF;
    std::vector<std::string> ccSlots, ccWear;
    std::vector<std::string> spriteOf; bool spriteAuto = SPRITES_DEFAULT;   // v36 sprite art: per-sim PNG name ("-" = keep the doll), auto-assign for everyone   // v34 custom content: furniture slot names, per-sim worn CC (hair, clothing, accessory)
    uint8_t speedIdx = 1, popIdx = 2, lifespan = 1; bool storyProg = true;   // v35 World Settings
    std::vector<uint8_t> movedAway;
    int16_t rival = -1, rivalFor = -1; uint8_t rivalScore = 0;
    bool hotelOwned = false; uint16_t hotelRate = 150, hotelStayEnd = 0xFFFF; uint8_t hotelTheme = 0, hotelUpg = 0, hotelStaff[4] = {0, 0, 0, 0}, hotelGuests = 0, hotelReview = 3; int32_t hotelLastNet = 0;
    std::vector<uint8_t> giftInv; std::vector<FoodItem> foodInv; std::vector<GiftLog> giftLog; uint64_t tvWatched = 0;   // v40 content pack
    uint64_t recipesMade = 0, recipesMade2 = 0; uint8_t trips = 0; uint32_t storyEvDone = 0;   // v41 legacy goals
    uint8_t filter = 0, filterStr = 2;   // v42 photo filter (0 off) and strength 1..3
    uint8_t homeId = 0, rentMiss = 0; uint64_t homeFav = 0, homeOwned = 0;   // v39 Home Gallery
    uint16_t jailUntil = 0xFFFF, boardEnd = 0xFFFF;   // v37 jail release day, boarding-school term end
    uint8_t vacDest = 0, courtCrime = 0; uint16_t vacEnd = 0, courtDay = 0xFFFF;   // v33 vacations & courts
    std::vector<uint16_t> deathDay;
    int16_t lastDeath = -1; uint16_t lastDeathDay = 0; bool memorialDone = true;
    std::vector<MovieSeries> movies;
    uint8_t gadgets = 0;   // bit0 e-reader (save v29)
    uint8_t houseType[2] = {1, 1};   // 0 apartment, 1 house, 2 two-story home (save v28)   // home meals stocked from Sugar Market (save v22)
#ifdef DIVA_LIFE
    uint8_t artStyle = 4;               // HD Glam
#else
    uint8_t artStyle = DEFAULT_ART;               // 7 Flash Stick (default), 8 Doodle, 9 Chibi, 10 Fashion Doll
#endif
};

extern Game G;
inline RoomState& curRoom() { return G.rooms[G.house][G.room]; }
inline Sim& MC() { return G.sims[G.mc]; }
inline bool ownsOutfit(int k) { return k < 0 ? false : k < 64 ? (G.ownedOutfits >> k) & 1 : k < 128 ? (G.ownedOutfits2 >> (k - 64)) & 1 : k < 192 ? (G.ownedOutfits3 >> (k - 128)) & 1 : k < 256 ? (G.ownedOutfits4 >> (k - 192)) & 1 : false; }
inline void giveOutfit(int k) { if (k < 0) return; if (k < 64) G.ownedOutfits |= 1ull << k; else if (k < 128) G.ownedOutfits2 |= 1ull << (k - 64); else if (k < 192) G.ownedOutfits3 |= 1ull << (k - 128); else if (k < 256) G.ownedOutfits4 |= 1ull << (k - 192); }

constexpr int NUM_WALLS = 12, NUM_FLOORS = 12;
extern const RGB WALL_COLORS[NUM_WALLS];
extern const RGB FLOOR_COLORS[NUM_FLOORS];
constexpr int NUM_WALLPATS = 12, NUM_FLOORPATS = 7;
extern const char* WALL_PATTERNS[NUM_WALLPATS];
extern const char* FLOOR_PATTERNS[NUM_FLOORPATS];
extern const char* HOLIDAY_NAMES[4];

// save.cpp
bool saveGame(const Game& g);
bool loadGame(Game& g);
bool saveExists();
// save slots (save.cpp): 5 independent lives, each with a backup and a small header
constexpr int NUM_SLOTS = 8;
struct SlotInfo { bool used = false, hasInfo = false; std::string name, mc, family; uint16_t day = 0; float minute = 0; uint32_t playSecs = 0; int64_t savedAt = 0; int household = 0; Sim portrait; };
extern int g_saveSlot; extern uint32_t g_playSecs; extern std::string g_slotName;
bool slotUsed(int k);
bool readSlotInfo(int k, SlotInfo& out);
bool deleteSlot(int k);
bool renameSlot(int k, const std::string& name);

// main.cpp services
void toast(const std::string& msg);
void rewardPopup(const std::string& title, const std::string& line);
void thoughtPopup(int sim, const std::string& text, int mood);   // Sims-style 'what they think' bubble (mood: -1 bad, 0 meh, 1 good, 2 romantic)   // the rainbow 'LIFE GOAL REWARD!' card
void fxBurst(float x, float y, int n);
void say(int simIdx, const std::string& line);

// dialogue.cpp: DS-style speech bubbles with a portrait (speaker -1 = narrator)
void dlgSay(int speaker, const std::string& text);
bool dlgActive();
void dlgAdvance();
void dlgDrawTop(float t);

// events.cpp: weekly town events & contests
struct TownEvent { const char* name; int room; int startH, endH; int game; const char* host; const char* intro; };
int eventsOn(int day, const TownEvent** out, int max);   // events on that day
const TownEvent* eventHere(int room);                    // running now at this place
void eventMorningNotice();
void eventContestResult(const TownEvent* ev, int score);
void eventArrive(int room);
void eventCrowning(const TownEvent* e);
bool eventEntered(const TownEvent* e);
void eventMarkEntered(const TownEvent* e);

// room.cpp
void drawRoom(const Game& g, float t, int selItem, int selActor, bool party);
void drawSim(const Sim& s, float x, float footY, float scale, float t, int holiday, Pose pose, bool flip);
float simHeight(const Sim& s);
// head-and-shoulders portrait inside a circle of radius r centred on (cx, cy)
void drawRoomOverlay(const Game& g, float t);
void styleGrade(float t);
void drawSimPortrait(const Sim& s, float cx, float cy, float r, float t);
