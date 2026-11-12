#pragma once
#include "common.hpp"

enum Age : uint8_t { ADULT, TEEN, KID, BABY };
enum Bottom : uint8_t { BT_PANTS, BT_MINI, BT_SHORTS, BT_GOWN, BT_JEANS, BT_MAXI, BT_OVERALLS, BT_SWEATS, BT_PLEATED, BT_SWIM, NUM_BOTTOMS };
enum TopStyle : uint8_t { TS_TEE, TS_TANK, TS_CROP, TS_HOODIE, TS_JERSEY, TS_BUTTON, TS_SWEATER, TS_DRESS, TS_JACKET, TS_BLAZER, TS_TUBE, TS_CARDIGAN, TS_POLO, TS_RAINCOAT, TS_SHIRTLESS, NUM_TOPS };
enum ShoeStyle : uint8_t { SS_SNEAKERS, SS_SANDALS, SS_BOOTS, SS_HEELS, SS_PLATFORMS, SS_FLIPFLOPS, SS_LOAFERS, SS_SLIPPERS, SS_BAREFOOT, NUM_SHOES };
enum HatStyle : uint8_t { HT_NONE, HT_STRAW, HT_CAP, HT_BEANIE, HT_BUCKET, HT_HEADBAND, HT_BOW, HT_FLOWERS, HT_BANDANA, HT_HEADWRAP, HT_TIARA, HT_COWBOY, NUM_HATS };
extern const char* TOP_NAMES[NUM_TOPS];
extern const char* BOTTOM_NAMES[NUM_BOTTOMS];
extern const char* SHOE_NAMES[NUM_SHOES];
extern const char* HAT_NAMES[NUM_HATS];
enum Pose : uint8_t { PO_IDLE, PO_PHONE, PO_DANCE, PO_SLEEP, PO_WAVE, PO_WALK, PO_SIT, PO_LIE, PO_SITSLEEP };
extern float g_walkPhase;   // 0..1 walk cycle for PO_WALK

// Face & makeup for the Glam Doll style (all small enums; 0 = default)
struct FaceDef {
    uint8_t shape = 0;     // 0 oval, 1 round, 2 heart, 3 square, 4 long
    uint8_t eyes = 0;      // 0 almond, 1 round, 2 cat, 3 doe, 4 hooded, 5 monolid
    uint8_t eyeCol = 0;    // 0 brown, 1 dark, 2 hazel, 3 green, 4 blue, 5 gray, 6 violet
    uint8_t brows = 0;     // 0 soft arch, 1 straight, 2 bold, 3 high arch, 4 bushy
    uint8_t nose = 0;      // 0 button, 1 straight, 2 wide, 3 upturned
    uint8_t lipShape = 0;  // 0 full, 1 heart, 2 thin, 3 wide pout
    uint8_t shadow = 1;    // 0 none, 1 pink, 2 smoky, 3 gold, 4 lilac, 5 blue, 6 bronze
    uint8_t liner = 1;     // 0 none, 1 thin, 2 winged
    uint8_t blush = 1;     // 0 none, 1 pink, 2 peach, 3 berry, 4 bronzer
    uint8_t marks = 0;     // 0 none, 1 freckles, 2 beauty mark, 3 both
    uint8_t glasses = 0;   // 0 none, 1 round, 2 cat-eye, 3 square, 4 rimless
    uint8_t pierce = 0;    // 0 none, 1 studs, 2 hoops, 3 nose stud, 4 hoops + nose
    uint8_t beard = 0;     // 0 none, 1 stubble, 2 goatee, 3 full
    uint8_t pad = 0;
};
constexpr int NUM_SKINS = 10, NUM_HAIRCOLS = 10, NUM_HAIRSTYLES = 95;
RGB skinRGB(int i);
RGB hairRGB(int i);
RGB eyeRGB(int i);      // 10 eye colours
RGB shadowRGB(int i);   // 10 eyeshadows (0 none)
RGB blushRGB(int i);    // 10 blushes (0 none)
constexpr int NUM_FACE_OPTS = 10;
extern const char* SKIN_NAMES[NUM_SKINS];
extern const uint8_t SKIN_ORDER[NUM_SKINS];   // light -> deep, for Create-a-Sim
extern const char* HAIRCOL_NAMES[NUM_HAIRCOLS];
extern const char* HAIRSTYLE_NAMES[NUM_HAIRSTYLES];

struct PersonDef {
    const char* name;
    Age age;
    bool guy;
    uint8_t skin, hairCol, hairStyle;   // hairStyle: 0 sleek long, 1 waves, 2 ponytail, 3 bob, 4 bun, 5 short,
                                        // 6 curls, 7 box braids, 8 afro, 9 puffs, 10 pixie, 11 locs
    RGB top, bottom;
    Bottom bt;
    uint8_t topPattern;                 // 0 none, 1 zebra, 2 leopard, 3 stripes
    RGB shoes, lips;
    FaceDef f{};
    uint8_t topStyle = TS_TEE, shoeStyle = SS_SNEAKERS, hat = HT_NONE;   // Stick Figure Ver. wardrobe
    uint16_t acc = 0;                   // accessories for the person's own look (Accessory flags)
    RGB accCol = {255, 90, 160};
};
FaceDef randomFace(bool guy, uint32_t seed);   // varied faces for townies

// topPattern: 0 none, 1 zebra, 2 leopard, 3 stripes, 4 plaid, 5 checker, 6 hearts/lace, 7 rhinestones, 8 graphic tee
enum Accessory : uint16_t {
    ACC_BOW = 1, ACC_BEANIE = 2, ACC_SHADES = 4, ACC_TIARA = 8, ACC_CLIPS = 16, ACC_CHOKER = 32,
    ACC_FLANNEL = 64, ACC_STREAKS = 128, ACC_PEARLS = 256, ACC_HOOPS = 512, ACC_LEGWARM = 1024,
    ACC_BAG = 2048, ACC_NECKLACE = 4096, ACC_BANGLES = 8192, ACC_TIGHTS = 16384
};
struct OutfitDef {
    const char* name;
    RGB top, bottom;
    Bottom bt;
    uint8_t topPattern;
    RGB shoes;
    uint16_t acc = 0;
    RGB accCol = {255, 255, 255};
    uint8_t topStyle = TS_TEE, shoeStyle = SS_SNEAKERS, hat = HT_NONE;
};
constexpr int FIRST_AESTHETIC = 9;   // OUTFITS[9..16] are the aesthetic looks

constexpr int NUM_PEOPLE = 24;
constexpr int NUM_PETS = 6;
constexpr int NUM_OUTFITS = 163;
constexpr int FIRST_GIRL_BASIC = 151;   // 151+ = girls-only Closet Basics
constexpr int FIRST_MALE_OUTFIT = 145;   // 145+ = guys-only outfits
constexpr int FIRST_KID_OUTFIT = 135;   // 135+ = kids/preteen-only clothes   // 32..62 = style presets (Pinterest-board aesthetics); 63 max (64-bit owned mask)
constexpr int FIRST_NEW_OUTFIT = 17;   // 17..31 = Stick Figure Ver. looks
extern const PersonDef PEOPLE[NUM_PEOPLE];
extern const OutfitDef OUTFITS[NUM_OUTFITS];
extern const char* PET_NAMES[NUM_PETS];
extern const char* PET_KINDS[NUM_PETS];
extern const char* AGE_NAMES[4];

extern bool g_dollStyle;   // true = Y2K doll look, false = fashion-sketch look
extern int g_artStyle;     // 0 = Y2K doll, 1 = fashion sketch, 2 = pixel doll, 3 = Glam Doll (default)
constexpr int NUM_ART_STYLES = 12;   // 11 = Pocket City (mid-2000s mobile pixel art, default)
constexpr int ART_FLIP = 11;   // 3 Pixel Glam, 4 HD Glam, 5 Pastel Pixel, 6 Cozy Pixel, 7 Flash Stick (default), 8 Doodle, 9 Chibi, 10 Fashion Doll
constexpr int FIRST_STICK_STYLE = 7, ART_FASHION_DOLL = 10;
inline bool artIsGlam(int s) { return s >= 3 && s <= 6; }
inline bool artIsStick(int s) { return s >= FIRST_STICK_STYLE && s <= ART_FASHION_DOLL; }
inline bool artIsFlip(int s) { return s == ART_FLIP; }   // 11 = Pocket City (mid-2000s mobile pixel art, 2:1 dimetric rooms)
void drawFlipPerson(const PersonDef& d, int outfit, float x, float footY, float s, float t, int holiday, Pose pose, bool flip);
void drawFlipBust(const PersonDef& d, int outfit, float cx, float cy, float r, float t);
float flipHeight(Age a);   // the Stick Figure Ver. renderer family (incl. Fashion Doll)
inline bool artIsDoll(int s) { return s == ART_FASHION_DOLL; }
extern int g_stickMood;   // face for the next figure: 0 smile, 1 excited, 2 sad, 3 annoyed, 4 flirty, 5 surprised, 6 bored, 7 asleep, 8 confident, 9 embarrassed
void drawStickPerson(const PersonDef& d, int outfit, float x, float footY, float s, float t, int holiday, Pose pose, bool flip);
void drawStickBust(const PersonDef& d, int outfit, float cx, float cy, float r, float t);
float stickHeight(Age a, int style);
PersonDef legacyLook(const PersonDef& d, int& outfit);   // new wardrobe -> what the original art styles can draw
inline bool artIsPixel(int s) { return s == 3 || s == 5 || s == 6 || s == ART_FLIP; }
void setArtStyle(int s);
const char* artStyleName(int s);
void drawGlamPerson(const PersonDef& d, int outfit, float x, float footY, float s, float t, int holiday, Pose pose, bool flip);
void drawGlamBust(const PersonDef& d, int outfit, float cx, float cy, float s, float t, bool blink = true);   // head & shoulders close-up, (cx,cy) = chin
void drawMiniPerson(const PersonDef& d, int outfit, float x, float footY, float s, float t, int holiday, Pose pose, bool flip);
extern int g_walkFrame;
void drawPixelPerson(const PersonDef& d, int outfit, float x, float footY, float s, float t, int holiday, Pose pose, bool flip);
float personHeight(const PersonDef& p);   // in pixels at scale 1
// holiday: 0 none, 1 birthday, 2 halloween, 3 christmas
void drawPerson(const PersonDef& p, int outfit, float x, float footY, float s, float t, int holiday, Pose pose, bool flip);
void drawPet(int pet, float x, float footY, float s, float t, bool sleeping, bool flip);
