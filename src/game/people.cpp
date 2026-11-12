#include "people.hpp"
#include "draw.hpp"
#include <cmath>
#include <cstdio>

static const RGB SKINS[6] = {{255, 226, 204}, {242, 196, 162}, {226, 174, 130}, {198, 136, 94}, {158, 104, 72}, {108, 70, 50}};
static const RGB HAIRS[8] = {{26, 18, 22}, {74, 42, 30}, {156, 98, 52}, {232, 192, 112},
                             {246, 234, 204}, {152, 56, 36}, {236, 116, 168}, {204, 198, 198}};

using namespace col;
static const RGB BERRY_LIP{196, 40, 96}, RED_LIP{200, 30, 50}, GLOSS_LIP{240, 120, 150}, NUDE_LIP{190, 110, 100};

// name, age, guy, skin, hairCol, hairStyle, top, bottom, bottomType, pattern, shoes, lips
const PersonDef PEOPLE[NUM_PEOPLE] = {
    {"Raven",     TEEN,  false, 1, 0, 0, CREAM, BLACK, BT_PANTS, 1, HOT, BERRY_LIP},
    {"Bella",     TEEN,  false, 0, 3, 1, HOT, BLUSH, BT_MINI, 0, GOLD, GLOSS_LIP},
    {"Sienna",    TEEN,  false, 3, 1, 2, PINK, CHOC, BT_PANTS, 0, CREAM, GLOSS_LIP},
    {"Tiara",     TEEN,  false, 5, 0, 1, GOLD, BLACK, BT_MINI, 2, BLACK, RED_LIP},
    {"Gigi",      TEEN,  false, 2, 5, 3, LILAC, CREAM, BT_MINI, 0, LILAC, GLOSS_LIP},
    {"Crystal",   ADULT, false, 0, 4, 0, BLACK, HOT, BT_MINI, 0, HOT, RED_LIP},
    {"Amber",     ADULT, false, 2, 2, 1, CREAM, MOCHA, BT_PANTS, 3, GOLD, NUDE_LIP},
    {"Kayla",     TEEN,  false, 4, 6, 2, BLACK, BLUSH, BT_SHORTS, 0, BLUSH, GLOSS_LIP},
    {"Star",      TEEN,  false, 1, 1, 0, BERRY, BLACK, BT_PANTS, 0, BERRY, BERRY_LIP},
    {"Mom Mimi",  ADULT, false, 3, 1, 4, CHOC, CREAM, BT_GOWN, 0, GOLD, RED_LIP},
    {"Nana Rose", ADULT, false, 0, 7, 4, LILAC, LILAC, BT_GOWN, 0, LILAC, GLOSS_LIP},
    {"Tyler",     TEEN,  true,  0, 2, 5, BABYBLUE, LATTE, BT_PANTS, 0, WHITE, NUDE_LIP},
    {"Jordan",    TEEN,  true,  4, 0, 5, BLACK, BABYBLUE, BT_PANTS, 1, WHITE, NUDE_LIP},
    {"Chase",     TEEN,  true,  2, 1, 5, PINK, CHOC, BT_PANTS, 0, BLACK, NUDE_LIP},
    {"Dad Marcus", ADULT, true, 5, 0, 5, CREAM, CHOC, BT_PANTS, 0, CHOC, NUDE_LIP},
    {"Pops Ray",  ADULT, true,  1, 7, 5, MINT, LATTE, BT_PANTS, 0, MOCHA, NUDE_LIP},
    {"Lexi",      KID,   false, 0, 3, 2, HOT, CREAM, BT_MINI, 0, HOT, GLOSS_LIP},
    {"Kiki",      KID,   false, 4, 0, 4, LILAC, HOT, BT_MINI, 2, GOLD, GLOSS_LIP},
    {"Ollie",     KID,   true,  1, 5, 5, BABYBLUE, CHOC, BT_SHORTS, 0, WHITE, NUDE_LIP},
    {"Bean",      KID,   true,  3, 1, 5, GOLD, LATTE, BT_PANTS, 3, BLUSH, NUDE_LIP},
    {"Baby Lulu", BABY,  false, 0, 4, 0, BLUSH, BLUSH, BT_PANTS, 0, BLUSH, GLOSS_LIP},
    {"Baby Pip",  BABY,  true,  2, 2, 0, BABYBLUE, BABYBLUE, BT_PANTS, 0, BABYBLUE, GLOSS_LIP},
    {"Baby Rosie", BABY, false, 4, 0, 0, LILAC, LILAC, BT_PANTS, 2, LILAC, GLOSS_LIP},
    {"Baby Teddy", BABY, true,  5, 0, 0, MINT, MINT, BT_PANTS, 0, MINT, GLOSS_LIP},
};

// name, top, bottom, bottom style, print, shoes, accessories, accessory colour, top style, shoe style, hat
static const RGB W{255, 255, 255};
const OutfitDef OUTFITS[NUM_OUTFITS] = {
    {"Signature Look", {0, 0, 0}, {0, 0, 0}, BT_PANTS, 0, {0, 0, 0}},   // uses the person's own
    {"Zebra Night Out", CREAM, BLACK, BT_PANTS, 1, HOT, 0, W, TS_TEE, SS_HEELS},
    {"Pink Mini", HOT, BLUSH, BT_MINI, 0, GOLD, 0, W, TS_CROP, SS_PLATFORMS},
    {"Leopard Glam", BLACK, BLACK, BT_MINI, 2, BLACK, 0, W, TS_TUBE, SS_HEELS},
    {"Velour Tracksuit", PINK, PINK, BT_SWEATS, 0, WHITE, 0, W, TS_HOODIE, SS_SNEAKERS},
    {"Pajama Party", BLUSH, BLUSH, BT_PANTS, 3, BLUSH, 0, W, TS_BUTTON, SS_SLIPPERS},
    {"Beach Babe", HOT, CREAM, BT_SHORTS, 0, GOLD, 0, W, TS_TANK, SS_SANDALS, HT_STRAW},
    {"Red Carpet Gown", BERRY, BERRY, BT_GOWN, 7, GOLD, 0, W, TS_TUBE, SS_HEELS},
    {"Halloween Kitty", BLACK, BLACK, BT_MINI, 0, BLACK, 0, W, TS_TEE, SS_BOOTS},
    // ---- aesthetics ----
    {"Y2K Butterfly", LILAC, BABYBLUE, BT_MINI, 8, WHITE, ACC_CLIPS | ACC_LEGWARM, {255, 120, 200}, TS_CROP, SS_PLATFORMS},
    {"McBling Princess", HOT, PINK, BT_MINI, 7, GOLD, ACC_TIARA | ACC_HOOPS, {230, 230, 240}, TS_TUBE, SS_PLATFORMS},
    {"Soft Girl", BLUSH, CREAM, BT_PLEATED, 6, WHITE, ACC_CLIPS | ACC_LEGWARM, {255, 236, 240}, TS_CARDIGAN, SS_SNEAKERS},
    {"Downtown Girl", BLACK, {120, 150, 190}, BT_JEANS, 0, CHOC, ACC_SHADES | ACC_PEARLS, {40, 30, 34}, TS_BLAZER, SS_LOAFERS},
    {"Coquette", CREAM, BLUSH, BT_MINI, 6, BLUSH, ACC_BOW | ACC_PEARLS, {255, 160, 190}, TS_DRESS, SS_LOAFERS},
    {"90s Grunge", {70, 66, 76}, BLACK, BT_JEANS, 0, BLACK, ACC_BEANIE | ACC_FLANNEL, {150, 40, 50}, TS_TEE, SS_BOOTS},
    {"Scene Goth", BLACK, BLACK, BT_MINI, 5, HOT, ACC_STREAKS | ACC_CHOKER | ACC_BOW | ACC_LEGWARM, {255, 40, 160}, TS_TEE, SS_BOOTS},
    {"Baddie", CHOC, LATTE, BT_PANTS, 0, WHITE, ACC_HOOPS | ACC_SHADES, {224, 184, 104}, TS_CROP, SS_SNEAKERS},
    // ---- Stick Figure Ver. wardrobe: for every age & gender ----
    {"Beach Day", WHITE, {70, 110, 190}, BT_SHORTS, 0, {176, 120, 80}, 0, W, TS_TANK, SS_SANDALS, HT_STRAW},
    {"Varsity Jock", {206, 40, 64}, {64, 64, 76}, BT_SWEATS, 0, WHITE, 0, W, TS_JERSEY, SS_SNEAKERS, HT_CAP},
    {"Sunday Best", {130, 180, 236}, {50, 54, 92}, BT_PANTS, 0, {90, 60, 50}, 0, W, TS_DRESS, SS_LOAFERS},
    {"Skater Kid", {96, 168, 96}, {160, 140, 104}, BT_PANTS, 0, {40, 40, 44}, 0, W, TS_HOODIE, SS_SNEAKERS, HT_BEANIE},
    {"Overalls Cutie", {255, 222, 120}, {84, 132, 204}, BT_OVERALLS, 3, WHITE, 0, W, TS_TEE, SS_SNEAKERS, HT_BOW},
    {"Fresh Kicks", {40, 40, 46}, {84, 112, 172}, BT_JEANS, 8, WHITE, ACC_HOOPS, {224, 184, 104}, TS_TEE, SS_SNEAKERS, HT_BUCKET},
    {"Office Boss", {64, 64, 84}, {64, 64, 84}, BT_PANTS, 0, BLACK, 0, W, TS_BLAZER, SS_HEELS},
    {"Cozy Knit", {214, 184, 150}, {124, 92, 72}, BT_MAXI, 0, {214, 130, 150}, 0, W, TS_CARDIGAN, SS_SLIPPERS},
    {"Tracksuit Drip", {60, 172, 222}, {60, 172, 222}, BT_SWEATS, 0, WHITE, 0, W, TS_JACKET, SS_SNEAKERS},
    {"Boho Sundress", {250, 204, 124}, {204, 92, 64}, BT_PANTS, 6, {176, 120, 80}, 0, W, TS_DRESS, SS_SANDALS, HT_FLOWERS},
    {"Emo Kid", BLACK, {60, 60, 72}, BT_JEANS, 5, BLACK, ACC_STREAKS | ACC_CHOKER, {206, 40, 64}, TS_TEE, SS_BOOTS},
    {"Prom Night", {124, 64, 204}, {124, 64, 204}, BT_GOWN, 7, GOLD, 0, W, TS_TUBE, SS_HEELS},
    {"Cheer Squad", {232, 40, 92}, WHITE, BT_PLEATED, 3, WHITE, 0, W, TS_TANK, SS_SNEAKERS, HT_HEADBAND},
    {"Rainy Day", {255, 214, 60}, {84, 112, 172}, BT_JEANS, 0, {70, 140, 90}, 0, W, TS_RAINCOAT, SS_BOOTS, HT_BUCKET},
    {"Midnight Stealth", {34, 30, 40}, {34, 30, 40}, BT_PANTS, 0, {34, 30, 40}, 0, W, TS_HOODIE, SS_SNEAKERS, HT_BEANIE},
    // ---- style presets: aesthetics boards ----
    {"Stockholm Style", CREAM, {150, 176, 206}, BT_JEANS, 0, {60, 70, 60}, ACC_BAG | ACC_HOOPS, {212, 176, 96}, TS_SWEATER, SS_SNEAKERS},
    {"Coconut Girl", {255, 150, 80}, {255, 200, 90}, BT_MINI, 6, WHITE, ACC_NECKLACE | ACC_BANGLES, {255, 110, 160}, TS_TANK, SS_SANDALS, HT_FLOWERS},
    {"Sade Girl", {120, 30, 40}, {150, 100, 60}, BT_MAXI, 2, {120, 30, 40}, ACC_HOOPS | ACC_BANGLES | ACC_NECKLACE, {224, 184, 104}, TS_TUBE, SS_BOOTS},
    {"Off-Duty Model", BLACK, {150, 160, 176}, BT_JEANS, 0, {150, 150, 156}, ACC_SHADES | ACC_BAG | ACC_BANGLES, {40, 34, 34}, TS_JACKET, SS_BOOTS},
    {"Rockstar GF", BLACK, BLACK, BT_MINI, 8, {150, 20, 30}, ACC_CHOKER | ACC_SHADES | ACC_NECKLACE, {150, 20, 30}, TS_JACKET, SS_BOOTS},
    {"Streetwear", {60, 140, 90}, {90, 120, 170}, BT_JEANS, 8, WHITE, ACC_NECKLACE, {224, 184, 104}, TS_JERSEY, SS_SNEAKERS, HT_CAP},
    {"Skater Teen '03", {236, 236, 236}, {70, 100, 160}, BT_JEANS, 8, WHITE, ACC_BANGLES, {200, 30, 40}, TS_TEE, SS_SNEAKERS},
    {"Shabby Chic", BLUSH, {170, 196, 220}, BT_PLEATED, 6, CREAM, ACC_BOW | ACC_PEARLS, {255, 190, 200}, TS_CARDIGAN, SS_LOAFERS},
    {"Clean Girl", WHITE, {214, 196, 170}, BT_PANTS, 0, WHITE, ACC_HOOPS | ACC_NECKLACE, {224, 184, 104}, TS_TANK, SS_SANDALS},
    {"Cottagecore", {250, 232, 214}, {250, 232, 214}, BT_MAXI, 6, {176, 120, 80}, ACC_BOW, {120, 160, 100}, TS_DRESS, SS_LOAFERS, HT_FLOWERS},
    {"Dark Academia", {92, 64, 48}, {70, 60, 56}, BT_PLEATED, 4, CHOC, ACC_NECKLACE, {200, 170, 110}, TS_BLAZER, SS_LOAFERS},
    {"Light Academia", CREAM, {214, 196, 170}, BT_PLEATED, 4, {176, 140, 100}, ACC_PEARLS, {250, 246, 236}, TS_SWEATER, SS_LOAFERS},
    {"Old Money", {40, 50, 90}, CREAM, BT_PANTS, 0, {120, 80, 60}, ACC_PEARLS | ACC_BAG, {250, 246, 236}, TS_POLO, SS_LOAFERS},
    {"Fairycore", {200, 230, 210}, {220, 200, 240}, BT_MAXI, 7, {220, 200, 240}, ACC_TIARA | ACC_CLIPS, {180, 255, 220}, TS_DRESS, SS_BAREFOOT, HT_FLOWERS},
    {"Boho", {236, 220, 196}, {190, 110, 70}, BT_MAXI, 6, {176, 120, 80}, ACC_BANGLES | ACC_NECKLACE, {224, 184, 104}, TS_BUTTON, SS_SANDALS, HT_HEADBAND},
    {"Sporty", WHITE, BLACK, BT_SWEATS, 0, WHITE, ACC_HOOPS, {224, 184, 104}, TS_TANK, SS_SNEAKERS, HT_CAP},
    {"Preppy", {30, 40, 70}, {30, 40, 70}, BT_PLEATED, 4, BLACK, ACC_BOW | ACC_PEARLS, {250, 246, 236}, TS_SWEATER, SS_LOAFERS, HT_HEADBAND},
    {"Punk", BLACK, {170, 30, 40}, BT_MINI, 4, BLACK, ACC_CHOKER | ACC_STREAKS | ACC_BANGLES, {200, 200, 210}, TS_JACKET, SS_BOOTS},
    {"Minimalist", {214, 200, 180}, {214, 200, 180}, BT_PANTS, 0, {120, 90, 70}, ACC_BAG, {120, 90, 70}, TS_BLAZER, SS_LOAFERS},
    {"Royalcore", {130, 40, 90}, {130, 40, 90}, BT_GOWN, 7, GOLD, ACC_TIARA | ACC_PEARLS, {240, 210, 120}, TS_DRESS, SS_HEELS},
    {"Witchy", {50, 30, 60}, BLACK, BT_MAXI, 0, BLACK, ACC_NECKLACE | ACC_CHOKER, {170, 120, 220}, TS_CARDIGAN, SS_BOOTS},
    {"Pastel Goth", BLACK, LILAC, BT_MINI, 5, LILAC, ACC_CHOKER | ACC_BOW | ACC_LEGWARM, {200, 170, 255}, TS_CROP, SS_PLATFORMS},
    {"E-Girl", BLACK, {70, 66, 76}, BT_PLEATED, 3, BLACK, ACC_STREAKS | ACC_CHOKER | ACC_CLIPS, {255, 120, 200}, TS_TEE, SS_PLATFORMS},
    {"VSCO Girl", {255, 214, 120}, {70, 100, 160}, BT_SHORTS, 0, WHITE, ACC_BANGLES | ACC_CLIPS, {120, 200, 190}, TS_TEE, SS_SANDALS},
    {"Country", {170, 40, 50}, {70, 100, 160}, BT_JEANS, 4, {150, 100, 60}, ACC_NECKLACE, {224, 184, 104}, TS_BUTTON, SS_BOOTS, HT_COWBOY},
    {"Star Girl", {30, 30, 40}, {150, 170, 200}, BT_JEANS, 7, {200, 200, 210}, ACC_SHADES | ACC_NECKLACE, {255, 230, 120}, TS_CROP, SS_BOOTS},
    {"Siren", {40, 80, 90}, {40, 80, 90}, BT_GOWN, 7, {200, 200, 210}, ACC_PEARLS | ACC_HOOPS, {200, 230, 240}, TS_TUBE, SS_HEELS},
    {"Costa Girl", WHITE, {255, 230, 200}, BT_MAXI, 0, {176, 120, 80}, ACC_HOOPS | ACC_BANGLES, {224, 184, 104}, TS_BUTTON, SS_SANDALS, HT_STRAW},
    {"Race Day", {220, 40, 40}, WHITE, BT_PANTS, 3, WHITE, ACC_SHADES, {30, 30, 30}, TS_JACKET, SS_SNEAKERS, HT_CAP},
    {"Princess", BLUSH, BLUSH, BT_GOWN, 7, {255, 220, 230}, ACC_TIARA | ACC_PEARLS, {255, 240, 250}, TS_DRESS, SS_HEELS},
    {"Vaporwave", {120, 220, 230}, {220, 140, 230}, BT_SHORTS, 5, WHITE, ACC_SHADES, {255, 120, 220}, TS_TEE, SS_PLATFORMS, HT_BUCKET},
    // ---- 2000s it-girl fits (save v23)
    {"Camo Motel Era", {120, 128, 92}, {96, 116, 150}, BT_MINI, 8, {242, 188, 206}, ACC_SHADES | ACC_BAG | ACC_HOOPS, {242, 196, 214}, TS_HOODIE, SS_SNEAKERS},
    {"Geek of the Week", {222, 60, 140}, {160, 172, 136}, BT_PANTS, 8, {242, 96, 166}, ACC_SHADES | ACC_BAG | ACC_HOOPS, {30, 30, 34}, TS_TEE, SS_BOOTS},
    {"Cargo Cap Cutie", {246, 244, 240}, {140, 128, 100}, BT_SHORTS, 8, {196, 146, 96}, ACC_SHADES | ACC_BAG, {120, 128, 88}, TS_TEE, SS_BOOTS, HT_CAP},
    {"Cocoa Peacoat", {112, 72, 52}, {66, 76, 104}, BT_PLEATED, 0, {44, 34, 34}, ACC_TIGHTS | ACC_BAG | ACC_HOOPS, {60, 46, 50}, TS_JACKET, SS_BOOTS},
    {"Mia", {28, 28, 32}, {36, 34, 40}, BT_JEANS, 0, {206, 176, 136}, ACC_NECKLACE | ACC_SHADES | ACC_BAG, {120, 210, 200}, TS_TANK, SS_HEELS},
    {"Eli", {250, 214, 60}, {200, 200, 206}, BT_SHORTS, 8, {226, 220, 204}, ACC_SHADES | ACC_NECKLACE | ACC_BAG, {30, 30, 34}, TS_TEE, SS_SNEAKERS},
    {"Sunday", {28, 28, 32}, {104, 112, 72}, BT_MINI, 0, {204, 142, 62}, ACC_NECKLACE | ACC_BANGLES | ACC_BAG | ACC_SHADES | ACC_LEGWARM, {246, 246, 246}, TS_TANK, SS_BOOTS},
    // ---- 2000s mall-girl collages (save v28)
    {"Lace Cami Lounge", {246, 244, 236}, {112, 86, 66}, BT_SHORTS, 0, {110, 90, 74}, ACC_NECKLACE, {200, 170, 130}, TS_TANK, SS_BOOTS},
    {"Henley & Mini", {244, 240, 232}, {150, 172, 204}, BT_MINI, 0, {204, 164, 112}, ACC_HOOPS, {240, 236, 228}, TS_BUTTON, SS_BOOTS},
    {"Pink Zip Hoodie", {236, 70, 120}, {80, 102, 152}, BT_SHORTS, 8, {246, 246, 246}, ACC_HOOPS | ACC_NECKLACE, {170, 230, 120}, TS_HOODIE, SS_SNEAKERS},
    {"Babydoll Flare", {246, 200, 212}, {80, 96, 132}, BT_JEANS, 0, {104, 82, 66}, ACC_BAG | ACC_HOOPS, {236, 160, 190}, TS_CARDIGAN, SS_BOOTS},
    // ---- menswear
    {"Varsity Jacket", {30, 42, 92}, {50, 62, 92}, BT_JEANS, 0, {246, 246, 246}, ACC_NECKLACE, {246, 246, 246}, TS_JACKET, SS_SNEAKERS},
    {"Hoodie & Joggers", {150, 150, 158}, {34, 34, 40}, BT_SWEATS, 0, {246, 246, 246}, 0, {246, 246, 246}, TS_HOODIE, SS_SNEAKERS},
    {"Button-Up & Chinos", {170, 200, 236}, {200, 176, 136}, BT_PANTS, 0, {110, 76, 56}, 0, {110, 76, 56}, TS_BUTTON, SS_LOAFERS},
    {"Sharp Suit", {26, 26, 32}, {26, 26, 32}, BT_PANTS, 0, {20, 20, 24}, ACC_NECKLACE, {200, 200, 210}, TS_BLAZER, SS_LOAFERS},
    {"Jersey Fit", {200, 36, 46}, {70, 90, 140}, BT_JEANS, 8, {246, 246, 246}, ACC_NECKLACE, {30, 30, 34}, TS_JERSEY, SS_SNEAKERS, HT_CAP},
    {"Denim Jacket & Tee", {92, 122, 172}, {30, 30, 36}, BT_PANTS, 0, {60, 46, 40}, 0, {246, 246, 246}, TS_JACKET, SS_BOOTS},
    {"Blush Sweater Mini", {236, 200, 206}, {96, 116, 146}, BT_MINI, 0, {150, 122, 100}, ACC_HOOPS | ACC_BAG | ACC_BANGLES, {220, 180, 90}, TS_SWEATER, SS_BOOTS},
    {"Pinstripe Prep", {150, 200, 210}, {226, 220, 206}, BT_PLEATED, 3, {204, 192, 172}, ACC_HOOPS | ACC_BAG | ACC_NECKLACE | ACC_BANGLES, {220, 180, 90}, TS_BUTTON, SS_BOOTS},
    {"Rhinestone Princess", {24, 22, 26}, {24, 22, 26}, BT_MINI, 7, {24, 22, 26}, ACC_NECKLACE | ACC_BAG, {230, 230, 240}, TS_DRESS, SS_HEELS, HT_TIARA},
    {"Cherry Babe Tank", {26, 24, 28}, {80, 104, 140}, BT_JEANS, 0, {30, 28, 30}, ACC_HOOPS | ACC_NECKLACE, {210, 30, 44}, TS_TANK, SS_HEELS},
    {"Priceless Tee", {255, 60, 150}, {240, 140, 180}, BT_SHORTS, 8, {196, 152, 110}, ACC_HOOPS | ACC_SHADES | ACC_BAG, {220, 180, 90}, TS_TEE, SS_BOOTS},
    {"Peasant Top & Flares", {212, 120, 142}, {80, 100, 140}, BT_JEANS, 0, {196, 164, 112}, ACC_SHADES | ACC_BAG, {140, 92, 60}, TS_CROP, SS_PLATFORMS},
    {"Lime Henley Flares", {160, 210, 110}, {90, 120, 160}, BT_JEANS, 0, {90, 60, 40}, ACC_HOOPS | ACC_NECKLACE | ACC_BANGLES, {220, 180, 90}, TS_TEE, SS_FLIPFLOPS},
    {"Lace Babydoll", {248, 246, 240}, {120, 140, 166}, BT_MINI, 6, {150, 122, 100}, ACC_HOOPS | ACC_NECKLACE | ACC_BAG | ACC_BANGLES, {220, 180, 90}, TS_TEE, SS_BOOTS},
    {"Suede Jacket Layers", {140, 100, 70}, {120, 140, 166}, BT_MINI, 0, {150, 122, 100}, ACC_HOOPS | ACC_NECKLACE | ACC_BAG, {220, 180, 90}, TS_JACKET, SS_BOOTS},
    {"Navy Halter & Ripple Skirt", {40, 56, 100}, {240, 238, 236}, BT_MAXI, 3, {30, 40, 80}, ACC_HOOPS | ACC_BANGLES, {215,170,70}, TS_TANK, SS_HEELS},
    {"Golden Hour Ruffles", {250, 226, 140}, {250, 226, 140}, BT_MINI, 6, {250, 236, 200}, ACC_BAG | ACC_NECKLACE, {215,170,70}, TS_TUBE, SS_PLATFORMS, HT_FLOWERS},
    {"Tube Top & Baggy Capris", {245, 245, 245}, {36, 36, 40}, BT_PANTS, 0, {240, 240, 240}, ACC_SHADES | ACC_NECKLACE | ACC_BAG | ACC_BANGLES, {215,170,70}, TS_TUBE, SS_FLIPFLOPS},
    {"Teal Tee & Flares", {70, 170, 170}, {140, 160, 190}, BT_JEANS, 0, {30, 30, 30}, ACC_HOOPS | ACC_NECKLACE | ACC_BAG | ACC_BANGLES, {215,170,70}, TS_TEE, SS_SNEAKERS, HT_HEADBAND},
    {"Denim Mini & Fur Boots", {24, 24, 26}, {80, 110, 150}, BT_SHORTS, 0, {170, 140, 100}, ACC_NECKLACE | ACC_BANGLES, {215,170,70}, TS_TUBE, SS_BOOTS},
    {"White Tube & Brown Mini", {245, 245, 245}, {120, 80, 50}, BT_MINI, 0, {30, 30, 30}, ACC_BAG | ACC_BANGLES, {200,200,210}, TS_TUBE, SS_HEELS},
    {"Butter Tank & Denim Maxi", {250, 236, 170}, {170, 190, 210}, BT_MAXI, 0, {240, 240, 240}, ACC_HOOPS | ACC_NECKLACE | ACC_BAG, {215,170,70}, TS_TANK, SS_SANDALS},
    {"Aqua Bikini & Sarong", {120, 210, 230}, {120, 210, 230}, BT_SWIM, 0, {150, 190, 255}, ACC_HOOPS | ACC_NECKLACE | ACC_BANGLES, {230,230,240}, TS_TUBE, SS_SLIPPERS, HT_FLOWERS},
    {"Baby Blue Tee & Ruffle Mini", {160, 210, 225}, {70, 80, 100}, BT_MINI, 6, {150, 130, 100}, ACC_NECKLACE | ACC_BANGLES | ACC_LEGWARM, {215,170,70}, TS_TEE, SS_BOOTS},
    {"Floral Bikini & White Ruffles", {250, 200, 220}, {250, 250, 250}, BT_MINI, 6, {200, 160, 110}, ACC_BAG | ACC_NECKLACE, {120,190,240}, TS_TUBE, SS_SANDALS, HT_FLOWERS},
    {"Cowgirl Corset", {245, 240, 232}, {140, 170, 200}, BT_SHORTS, 0, {60, 40, 30}, ACC_TIGHTS | ACC_NECKLACE, {200,190,180}, TS_BUTTON, SS_BOOTS, HT_COWBOY},
    {"Cozy Butter Shorts", {245, 245, 245}, {250, 226, 140}, BT_SHORTS, 0, {180, 130, 90}, ACC_NECKLACE | ACC_LEGWARM | ACC_HOOPS, {215,170,70}, TS_CARDIGAN, SS_BOOTS},
    {"White Babydoll & Grey Shorts", {245, 243, 236}, {140, 140, 150}, BT_SHORTS, 0, {240, 240, 240}, ACC_NECKLACE | ACC_BANGLES, {215,170,70}, TS_TUBE, SS_SANDALS},
    {"Leopard Bralette & Fur Mini", {200, 150, 100}, {90, 110, 160}, BT_MINI, 7, {200, 170, 120}, ACC_BAG | ACC_NECKLACE | ACC_BANGLES | ACC_HOOPS, {215,170,70}, TS_CROP, SS_BOOTS, HT_CAP},
    {"Rocker Fur Jacket", {245, 245, 245}, {120, 150, 190}, BT_SHORTS, 0, {30, 30, 30}, ACC_SHADES | ACC_NECKLACE | ACC_BAG, {200,200,210}, TS_JACKET, SS_BOOTS},
    {"Olive Cowl & Tiered Skirt", {170, 160, 70}, {245, 240, 232}, BT_MAXI, 6, {210, 170, 80}, ACC_NECKLACE | ACC_BANGLES | ACC_BAG | ACC_HOOPS, {215,170,70}, TS_TANK, SS_SANDALS},
    {"Pink Shirred Top & Maxi", {250, 190, 210}, {250, 250, 250}, BT_MAXI, 6, {30, 30, 30}, ACC_BAG | ACC_NECKLACE | ACC_BANGLES, {215,170,70}, TS_TUBE, SS_PLATFORMS, HT_FLOWERS},
    {"Midnight Hibiscus Maxi", {24, 24, 28}, {24, 24, 28}, BT_MAXI, 8, {24, 24, 28}, ACC_BAG | ACC_NECKLACE, {90,160,240}, TS_TANK, SS_PLATFORMS, HT_FLOWERS},
    {"Cowgirl Ruffle Dress", {245, 240, 225}, {245, 240, 225}, BT_MINI, 6, {190, 150, 110}, ACC_BAG | ACC_NECKLACE | ACC_BANGLES, {215,170,70}, TS_DRESS, SS_BOOTS, HT_COWBOY},
    {"Rosebud Zip Hoodie", {240, 225, 210}, {250, 190, 210}, BT_SHORTS, 4, {180, 140, 100}, ACC_BAG | ACC_LEGWARM, {120,80,60}, TS_HOODIE, SS_BOOTS},
    {"Olive Lace Cami & Peasant Skirt", {100, 110, 70}, {250, 250, 250}, BT_MAXI, 6, {140, 140, 80}, ACC_HOOPS | ACC_NECKLACE | ACC_BAG | ACC_BANGLES, {215,170,70}, TS_TANK, SS_SLIPPERS, HT_BANDANA},
    {"Snow Bunny Cape", {245, 245, 245}, {245, 245, 245}, BT_MINI, 0, {30, 30, 30}, ACC_BAG | ACC_LEGWARM, {120,80,60}, TS_RAINCOAT, SS_BOOTS},
    {"Lace Slip & Cowboy Boots", {240, 236, 226}, {240, 236, 226}, BT_MINI, 6, {140, 90, 50}, ACC_BAG | ACC_BANGLES | ACC_HOOPS, {215,170,70}, TS_TANK, SS_BOOTS},
    {"Leopard Bustier & Denim Maxi", {110, 70, 45}, {40, 40, 44}, BT_MAXI, 7, {24, 24, 26}, ACC_BAG | ACC_NECKLACE, {230,230,240}, TS_TUBE, SS_PLATFORMS},
    {"Camo Mesh & Baggy Jeans", {130, 140, 100}, {170, 195, 220}, BT_JEANS, 4, {120, 80, 50}, ACC_BAG | ACC_NECKLACE | ACC_HOOPS, {215,170,70}, TS_CROP, SS_SNEAKERS},
    {"Red Tee & Denim Mini", {170, 30, 40}, {130, 160, 190}, BT_MINI, 0, {140, 110, 80}, ACC_BAG | ACC_NECKLACE | ACC_BANGLES, {215,170,70}, TS_TEE, SS_BOOTS},
    {"Wine Zip-Up & Dark Flares", {120, 40, 60}, {50, 60, 90}, BT_JEANS, 0, {150, 60, 60}, ACC_BAG | ACC_HOOPS | ACC_NECKLACE, {220,220,230}, TS_HOODIE, SS_HEELS},
    {"Baby Tee & Track Pants", {150, 200, 240}, {50, 50, 56}, BT_SWEATS, 8, {240, 240, 240}, ACC_NECKLACE | ACC_BANGLES, {215,170,70}, TS_TEE, SS_SNEAKERS},
    {"Pink Zip-Up & Light Flares", {240, 170, 190}, {170, 195, 225}, BT_JEANS, 0, {40, 40, 44}, ACC_NECKLACE | ACC_HOOPS, {230,230,240}, TS_HOODIE, SS_SNEAKERS},
    {"Plum Halter & Fur Legwarmers", {70, 40, 50}, {70, 40, 50}, BT_MINI, 6, {240, 236, 230}, ACC_BAG | ACC_LEGWARM | ACC_TIGHTS | ACC_BANGLES, {215,170,70}, TS_TANK, SS_HEELS, HT_BEANIE},
    {"Blush Top & Pleated Mini", {250, 225, 225}, {200, 215, 235}, BT_PLEATED, 0, {30, 30, 30}, ACC_HOOPS | ACC_LEGWARM, {220,220,230}, TS_SWEATER, SS_PLATFORMS},
    {"Black Halter & Bow Belt Jeans", {30, 30, 32}, {60, 80, 120}, BT_JEANS, 0, {220, 215, 240}, ACC_BAG, {240,200,210}, TS_TANK, SS_SNEAKERS},
    {"Fur Collar Moto & Shorts", {30, 30, 32}, {110, 140, 180}, BT_SHORTS, 0, {30, 30, 32}, ACC_BAG | ACC_NECKLACE, {215,170,70}, TS_JACKET, SS_BOOTS, HT_HEADBAND},
    {"Shearling Parka & Mini", {170, 150, 110}, {50, 50, 60}, BT_SHORTS, 0, {100, 70, 50}, ACC_BAG | ACC_NECKLACE | ACC_LEGWARM, {120,200,180}, TS_RAINCOAT, SS_BOOTS, HT_HEADBAND},
    {"Script Tee & Grey Joggers", {240, 240, 240}, {170, 170, 176}, BT_SWEATS, 8, {150, 100, 70}, ACC_BAG | ACC_HOOPS | ACC_BANGLES, {215,170,70}, TS_TEE, SS_BOOTS},
    {"Peasant Crop & Satin Maxi", {245, 243, 236}, {236, 228, 210}, BT_MAXI, 6, {120, 80, 50}, ACC_BAG | ACC_BANGLES, {215,170,70}, TS_CROP, SS_SANDALS},
    {"Mauve Vest & Denim Mini", {190, 120, 130}, {120, 150, 190}, BT_MINI, 0, {200, 150, 150}, ACC_BAG | ACC_NECKLACE | ACC_BANGLES, {215,170,70}, TS_BUTTON, SS_BOOTS, HT_BANDANA},
    {"Purple Off-Shoulder & Socks", {130, 70, 160}, {120, 150, 190}, BT_SHORTS, 0, {110, 70, 40}, ACC_BAG | ACC_NECKLACE | ACC_CHOKER | ACC_LEGWARM, {215,170,70}, TS_TUBE, SS_LOAFERS},
    {"Butter Stripe Tee & Shorts", {250, 230, 160}, {220, 220, 225}, BT_SHORTS, 3, {210, 170, 120}, ACC_BAG | ACC_NECKLACE | ACC_BANGLES, {215,170,70}, TS_TEE, SS_SANDALS, HT_FLOWERS},
    {"Peasant Blouse & Faded Flares", {245, 243, 238}, {110, 140, 160}, BT_JEANS, 6, {30, 30, 30}, ACC_NECKLACE, {230,230,240}, TS_SWEATER, SS_PLATFORMS},
    {"Boho Lace & Wide Jeans", {240, 232, 220}, {70, 90, 130}, BT_JEANS, 6, {90, 50, 30}, ACC_BAG | ACC_HOOPS | ACC_CHOKER, {215,170,70}, TS_BUTTON, SS_LOAFERS},
    {"Beige Zip Jacket & Brown Mini", {225, 205, 180}, {110, 70, 45}, BT_MINI, 0, {120, 80, 50}, ACC_BAG, {200,60,60}, TS_JACKET, SS_BOOTS},
    {"Grey Off-Shoulder & Fur Boots", {150, 150, 156}, {170, 195, 225}, BT_JEANS, 0, {240, 240, 240}, ACC_NECKLACE, {220,220,230}, TS_SWEATER, SS_BOOTS},
    {"Black Tee & Vintage Flares", {50, 50, 54}, {140, 150, 140}, BT_JEANS, 0, {30, 30, 30}, ACC_NECKLACE | ACC_BANGLES | ACC_HOOPS, {215,170,70}, TS_TEE, SS_HEELS},
    {"Hibiscus Baby Tee & Pink Joggers", {100, 70, 55}, {245, 190, 210}, BT_SWEATS, 8, {190, 150, 100}, ACC_NECKLACE | ACC_HOOPS, {215,170,70}, TS_TEE, SS_BOOTS},
    {"Moto Crop & Bootcut Jeans", {30, 30, 32}, {60, 80, 120}, BT_JEANS, 0, {150, 110, 70}, ACC_BAG, {200,200,210}, TS_JACKET, SS_BOOTS},
    {"Kid: Lilac Plaid Top & Swirl Jeans", {170, 165, 230}, {60, 95, 160}, BT_JEANS, 0, {120, 80, 190}, 0, {255,255,255}, TS_BUTTON, SS_SNEAKERS},
    {"Kid: Pink Tee & Heart Patch Flares", {245, 175, 205}, {60, 90, 150}, BT_JEANS, 0, {240, 235, 225}, 0, {255,255,255}, TS_TEE, SS_LOAFERS},
    {"Kid: Striped Tee & Overalls", {240, 150, 160}, {70, 105, 170}, BT_OVERALLS, 3, {240, 240, 240}, 0, {255,255,255}, TS_TEE, SS_SNEAKERS},
    {"Kid: Rosette Knit Pinafore & Flares", {245, 240, 220}, {150, 180, 215}, BT_JEANS, 0, {245, 160, 190}, 0, {255,255,255}, TS_SWEATER, SS_LOAFERS, HT_HEADBAND},
    {"Kid: Yellow Heart Ruffle Tee", {250, 235, 140}, {60, 95, 165}, BT_JEANS, 8, {30, 30, 34}, ACC_NECKLACE, {215,200,170}, TS_TEE, SS_BOOTS},
    {"Kid: Star Graphic Layer Tee", {30, 30, 34}, {40, 50, 90}, BT_JEANS, 8, {190, 150, 100}, 0, {255,255,255}, TS_TEE, SS_BOOTS, HT_BOW},
    {"Kid: Butter Stripe Bow Set", {248, 235, 170}, {248, 235, 170}, BT_PANTS, 3, {150, 95, 55}, ACC_BAG | ACC_PEARLS, {215,185,130}, TS_TANK, SS_SANDALS, HT_FLOWERS},
    {"Kid: Pumpkin Halloween Costume", {245, 130, 40}, {30, 30, 34}, BT_PANTS, 0, {30, 30, 34}, 0, {255,255,255}, TS_HOODIE, SS_SNEAKERS, HT_BEANIE},
    {"Kid: Christmas Pajama Set", {200, 40, 50}, {200, 40, 50}, BT_SWEATS, 3, {245, 245, 245}, 0, {255,255,255}, TS_SWEATER, SS_SLIPPERS},
    {"Kid: Easter Pastel Dress", {250, 205, 225}, {250, 205, 225}, BT_PANTS, 0, {245, 245, 245}, 0, {255,255,255}, TS_DRESS, SS_LOAFERS, HT_BOW},
    {"Male Outfit 1", {101, 62, 48}, {120, 150, 185}, BT_JEANS, 0, {90, 50, 30}, ACC_NECKLACE, {215, 175, 70}, TS_TEE, SS_LOAFERS},
    {"Male Outfit 2", {240, 120, 40}, {40, 55, 85}, BT_JEANS, 8, {60, 60, 60}, ACC_NECKLACE, {200, 200, 210}, TS_POLO, SS_SNEAKERS},
    {"Male Outfit 3", {220, 50, 50}, {120, 135, 150}, BT_JEANS, 8, {30, 30, 30}, ACC_BANGLES, {120, 200, 255}, TS_TEE, SS_SNEAKERS},
    {"Male Outfit 4", {245, 245, 240}, {80, 120, 180}, BT_JEANS, 0, {40, 50, 110}, ACC_NECKLACE | ACC_BANGLES, {200, 200, 210}, TS_POLO, SS_SNEAKERS},
    {"Male Outfit 5", {120, 95, 70}, {45, 55, 75}, BT_JEANS, 0, {20, 20, 20}, ACC_NECKLACE, {200, 200, 210}, TS_JACKET, SS_LOAFERS},
    {"Male Outfit 6", {85, 60, 50}, {35, 40, 60}, BT_JEANS, 0, {60, 40, 30}, ACC_NECKLACE, {215, 175, 70}, TS_JACKET, SS_BOOTS},
    {"Basics: Leather Jacket & Low-Rise Flares", {60, 45, 40}, {55, 70, 110}, BT_JEANS, 0, {20, 20, 20}, 0, {255,255,255}, TS_JACKET, SS_BOOTS},
    {"Basics: Lace Tank & Low-Rise Jeans", {250, 248, 245}, {55, 70, 110}, BT_JEANS, 6, {20, 20, 20}, ACC_NECKLACE, {215,175,70}, TS_TANK, SS_BOOTS},
    {"Basics: Striped Long Sleeve & Flares", {215, 190, 150}, {70, 90, 130}, BT_JEANS, 3, {20, 20, 20}, 0, {255,255,255}, TS_SWEATER, SS_BOOTS},
    {"Basics: Babydoll Top & Leg Warmers", {250, 248, 245}, {60, 60, 70}, BT_MINI, 6, {20, 20, 20}, ACC_LEGWARM, {240,240,235}, TS_CROP, SS_BOOTS},
    {"Basics: Black Long Sleeve & Jeans", {25, 25, 30}, {90, 120, 170}, BT_JEANS, 0, {245, 245, 245}, 0, {255,255,255}, TS_SWEATER, SS_SNEAKERS},
    {"Basics: Graphic Tee & Jean Shorts", {250, 250, 250}, {100, 140, 190}, BT_SHORTS, 8, {245, 245, 245}, 0, {255,255,255}, TS_TEE, SS_SNEAKERS},
    {"Basics: Grey Hoodie & Sweats", {150, 150, 155}, {210, 210, 215}, BT_SWEATS, 0, {245, 245, 245}, 0, {255,255,255}, TS_HOODIE, SS_SNEAKERS},
    {"Basics: Bodysuit & Pleated Skirt", {40, 40, 45}, {120, 125, 140}, BT_PLEATED, 0, {20, 20, 20}, ACC_NECKLACE, {215,175,70}, TS_TANK, SS_BOOTS},
    {"Basics: White Tee & Bike Shorts", {250, 250, 250}, {30, 30, 30}, BT_SHORTS, 0, {245, 245, 245}, 0, {255,255,255}, TS_TEE, SS_SNEAKERS},
    {"Basics: Puffer & Leggings", {35, 35, 40}, {25, 25, 28}, BT_PANTS, 0, {190, 140, 90}, 0, {255,255,255}, TS_JACKET, SS_BOOTS},
    {"Basics: Cardigan & Denim Mini", {245, 240, 232}, {150, 180, 210}, BT_MINI, 0, {245, 245, 245}, ACC_NECKLACE, {215,175,70}, TS_CARDIGAN, SS_SNEAKERS},
    {"Basics: Tube Top & Cargo Pants", {230, 230, 235}, {130, 130, 130}, BT_PANTS, 0, {245, 245, 245}, ACC_HOOPS, {215,175,70}, TS_TUBE, SS_SNEAKERS},
};

const char* PET_NAMES[NUM_PETS] = {"Coco", "Duchess", "Clover", "Peanut", "Goldie", "Bijou"};
const char* PET_KINDS[NUM_PETS] = {"Chihuahua", "Persian Cat", "Bunny", "Hamster", "Goldfish", "Poodle"};
const char* AGE_NAMES[4] = {"Adult", "Teen", "Kid", "Baby"};

static float ageScale(Age a) { return a == ADULT ? 1.0f : a == TEEN ? 0.94f : a == KID ? 0.7f : 0.62f; }
float personHeight(const PersonDef& p) { return p.age == BABY ? 40 * ageScale(p.age) : 120 * ageScale(p.age); }

void topPatternOn(const Pen& p, float yTop, float yBot, float lTop, float rTop, float lBot, float rBot, int pat, float seed) {
    if (pat == 0) return;
    if (pat >= 4) {
        auto at = [&](float u, float v, float& ox, float& oy) {
            float y = yTop + (yBot - yTop) * v;
            float l = lTop + (lBot - lTop) * v, r = rTop + (rBot - rTop) * v;
            ox = l + (r - l) * u; oy = y;
        };
        float x0, y0, x1, y1;
        if (pat == 4) {   // plaid
            for (int i = 1; i < 5; i++) { at(0, i / 5.f, x0, y0); at(1, i / 5.f, x1, y1); p.L(x0, y0, x1, y1, 0.9f, C(BLACK, 90)); }
            for (int i = 1; i < 3; i++) { at(i / 3.f, 0, x0, y0); at(i / 3.f, 1, x1, y1); p.L(x0, y0, x1, y1, 0.9f, C(200, 40, 50, 150)); }
        } else if (pat == 5) {   // checkerboard
            for (int r = 0; r < 4; r++) for (int k = 0; k < 3; k++) if ((r + k) % 2) {
                float xa, ya, xb, yb, xc, yc, xd, yd;
                at(k / 3.f, r / 4.f, xa, ya); at((k + 1) / 3.f, r / 4.f, xb, yb); at((k + 1) / 3.f, (r + 1) / 4.f, xc, yc); at(k / 3.f, (r + 1) / 4.f, xd, yd);
                p.Q(xa, ya, xb, yb, xc, yc, xd, yd, C(HOT));
            }
        } else if (pat == 6) {   // little hearts / lace
            for (int i = 0; i < 4; i++) { at(0.25f + 0.5f * (i % 2), 0.2f + i * 0.2f, x0, y0); p.H(x0, y0, 1.2f, C(255, 255, 255, 220)); }
            at(0, 0.05f, x0, y0); at(1, 0.05f, x1, y1); p.L(x0, y0, x1, y1, 0.8f, C(255, 255, 255, 200));
        } else if (pat == 8) {   // graphic tee: a big heart with a sparkle
            at(0.5f, 0.42f, x0, y0);
            p.H(x0, y0, 3.2f, C(HOT)); p.H(x0, y0 - 0.4f, 1.6f, C(255, 160, 210));
            at(0.78f, 0.22f, x1, y1); p.Sp(x1, y1, 1.6f, C(GOLD));
        } else {   // rhinestones
            for (int i = 0; i < 6; i++) { at(0.2f + 0.3f * (i % 3), 0.2f + 0.3f * (i / 3), x0, y0); p.Ci(x0, y0, 0.7f, C(255, 255, 255, 230)); }
            at(0.5f, 0.5f, x0, y0); p.Sp(x0, y0, 2.f, C(255, 255, 255, 230));
        }
        (void)seed;
        return;
    }
    u32 dk = C(BLACK);
    int n = 6;
    for (int i = 0; i < n; i++) {
        float k0 = (i + 0.25f) / n, k1 = k0 + (pat == 3 ? 0.07f : 0.09f);
        float y0 = yTop + (yBot - yTop) * k0, y1 = yTop + (yBot - yTop) * k1;
        float l0 = lTop + (lBot - lTop) * k0, r0 = rTop + (rBot - rTop) * k0;
        float l1 = lTop + (lBot - lTop) * k1, r1 = rTop + (rBot - rTop) * k1;
        if (pat == 1) {          // zebra: wavy, uneven
            float w = std::sin(i * 1.7f + seed) * 1.2f;
            p.Q(l0, y0 + w, r0, y0 - w, r1, y1 - w * 0.5f, l1, y1 + w * 0.5f, dk);
        } else if (pat == 3) {   // candy stripes
            p.Q(l0, y0, r0, y0, r1, y1, l1, y1, C(255, 255, 255, 150));
        } else {                 // leopard spots
            for (int k = 0; k < 2; k++) {
                float fx = l0 + (r0 - l0) * (0.25f + 0.5f * ((i + k) % 2)) + k;
                p.Ci(fx, y0 + 1, 1.3f, C(70, 42, 28));
                p.Ci(fx + 0.3f, y0 + 0.8f, 0.6f, C(176, 118, 60));
            }
        }
    }
}

void drawHat(const Pen& p, int holiday, float t) {
    if (holiday == 1) {   // party hat
        p.T(14, 1, 26, 1, 21, -14, C(HOT));
        p.L(17, -4, 24, -6, 1.2f, C(GOLD)); p.L(15.5f, 0, 25, -1, 1.2f, C(CREAM));
        p.Ci(21, -14, 2, C(GOLD));
    } else if (holiday == 2) {   // witch hat
        p.E(20, 2, 26, 4, C(BLACK));
        p.T(13, 2, 27, 2, 24, -18, C(BLACK));
        p.R(14, 0, 12, 2, C(PURPLE));
        p.Sp(24, -18, 2.5f, C(GOLD));
    } else if (holiday == 3) {   // santa hat
        p.Q(12, 1, 28, 1, 30, -6, 19, -12, C(RED));
        p.Ci(31, -6, 2.6f, C(WHITE));
        p.R(11, -1, 18, 4, C(WHITE));
    }
    (void)t;
}

static void drawBaby(const PersonDef& d, int outfit, float x, float footY, float s, float t, int holiday, Pose pose, bool flip) {
    s *= ageScale(BABY);
    float bob = std::sin(t * 3.f) * 0.8f;
    Pen p(x, footY + bob, 40, 40, s * 1.0f, flip);
    RGB skin = SKINS[d.skin], hair = HAIRS[d.hairCol];
    RGB on = outfit == 0 ? d.top : OUTFITS[outfit].top;
    if (outfit == 8) on = BLACK;
    p.E(13, 36, 11, 7, C(shade(on, 0.85f))); p.E(27, 36, 11, 7, C(shade(on, 0.85f)));
    p.E(20, 29, 22, 19, C(on));
    if (d.topPattern == 2 && outfit == 0) topPatternOn(p, 22, 36, 12, 28, 10, 30, 2, 0);
    heart(p.X(20), p.Y(28), 2.5f * s, C(255, 255, 255, 150));
    float wave = pose == PO_WAVE ? std::sin(t * 9) * 3 : 0;
    p.Ci(9, 27 + wave, 3.4f, C(skin)); p.Ci(31, 27, 3.4f, C(skin));
    p.Ci(20, 13, 10.5f, C(skin));
    // hair tuft + bow
    p.E(20, 5, 12, 5, C(hair));
    p.L(20, 3, 23, 0, 1.6f, C(hair));
    if (!d.guy) { p.T(24, 3, 30, 0, 30, 6, C(HOT)); p.T(24, 3, 18, 0, 18, 6, C(HOT)); p.Ci(24, 3, 1.6f, C(BERRY)); }
    if (pose == PO_SLEEP) {
        p.L(14, 13, 18, 13, 1.f, C(BLACK)); p.L(22, 13, 26, 13, 1.f, C(BLACK));
    } else {
        bool blink = std::fmod(t + d.skin, 4.f) < 0.12f;
        if (blink) { p.L(14, 13, 18, 13, 1.f, C(BLACK)); p.L(22, 13, 26, 13, 1.f, C(BLACK)); }
        else { p.Ci(16, 13, 1.9f, C(BLACK)); p.Ci(24, 13, 1.9f, C(BLACK)); p.Ci(16.6f, 12.4f, 0.6f, C(WHITE)); p.Ci(24.6f, 12.4f, 0.6f, C(WHITE)); }
    }
    p.Ci(13, 17, 2, C(255, 140, 170, 90)); p.Ci(27, 17, 2, C(255, 140, 170, 90));
    p.Ci(20, 19, 2.2f, C(HOT)); p.Ci(20, 19, 1, C(BLUSH));   // pacifier
    if (holiday) { Pen hp(x, footY + bob - 1 * s, 40, 40, s, flip); drawHat(hp, holiday, t); }
}


// ============================================================ Y2K doll style
// Big heads, huge lashy eyes, glossy pouty lips and chunky platforms.
bool g_dollStyle = false;   // default: fashion-sketch look
#ifdef DIVA_LIFE
int g_artStyle = 4;         // Dream Dollhouse: HD Glam by default, every style available
#else
int g_artStyle = DEFAULT_ART;         // Stick Figure Ver.: 7 Flash Stick (default), 8 Doodle, 9 Chibi, 10 Fashion Doll
#endif
float g_walkPhase = 0;
#ifdef DIVA_LIFE
void setArtStyle(int s) { g_artStyle = (s % NUM_ART_STYLES + NUM_ART_STYLES) % NUM_ART_STYLES; g_dollStyle = g_artStyle == 0; }
#else
void setArtStyle(int s) { const int n = ART_FLIP - FIRST_ART_CHOICE; /* the pixel style (11) is retired */ g_artStyle = FIRST_ART_CHOICE + (((s - FIRST_ART_CHOICE) % n) + n) % n; g_dollStyle = g_artStyle == 0; }
#endif
const char* artStyleName(int s) {
    static const char* n[NUM_ART_STYLES] = {"Y2K Doll", "Sketch", "Mini Pixel", "Pixel Glam", "HD Glam", "Pastel Pixel", "Cozy Pixel", "Flash Stick", "Doodle", "Chibi", "Fashion Doll", "Pixel Diva"};
    return n[(s % NUM_ART_STYLES + NUM_ART_STYLES) % NUM_ART_STYLES];
}

static void dollBackHair(const Pen& p, const PersonDef& d, u32 HR, float t) {
    if (d.guy) return;
    switch (d.hairStyle) {
    case 0:   // sleek & long
        p.E(20, 3, 31, 24, HR);
        p.Q(5, 4, 35, 4, 37, 58, 3, 56, HR);
        p.T(3, 56, 12, 56, 6, 64, HR); p.T(37, 58, 28, 58, 34, 65, HR);
        break;
    case 1:   // big volume waves
        p.E(20, 4, 36, 28, HR);
        p.Q(4, 8, 36, 8, 38, 46, 2, 46, HR);
        for (int i = 0; i < 5; i++) { p.Ci(3.5f, 14 + i * 8.f, 4.6f, HR); p.Ci(36.5f, 14 + i * 8.f, 4.6f, HR); }
        break;
    case 2: {   // high ponytail
        float sw = std::sin(t * 2.f) * 2;
        p.E(20, 3, 28, 22, HR);
        p.Ci(30, -6, 5.2f, HR);
        p.Q(29, -5, 36, -3, 38 + sw, 34, 30 + sw, 32, HR);
        break;
    }
    case 3:   // chunky bob
        p.E(20, 6, 33, 28, HR);
        p.Q(4, 6, 36, 6, 36, 24, 4, 24, HR);
        break;
    case 4:   // top bun
        p.E(20, 3, 28, 22, HR);
        p.Ci(20, -10, 7, HR);
        break;
    }
}

static void hoop(const Pen& p, float cx, float cy, float r, u32 c) {
    const int N = 10;
    for (int i = 0; i < N; i++) {
        float a0 = i * 6.2831853f / N, a1 = (i + 1) * 6.2831853f / N;
        p.L(cx + std::cos(a0) * r, cy + std::sin(a0) * r, cx + std::cos(a1) * r, cy + std::sin(a1) * r, 0.7f, c);
    }
}

static void dollHead(const Pen& p, const PersonDef& d, Pose pose, float t) {
    const RGB skin = SKINS[d.skin], hair = HAIRS[d.hairCol];
    const u32 SK = C(skin), SKD = C(shade(skin, 0.86f)), HR = C(hair), HRL = C(shade(hair, 1.55f), 170), DK = C(BLACK);
    // neck + face (oval with a soft pointed chin)
    p.R(18.2f, 19, 3.6f, 12, SKD);
    p.E(7.6f, 11.5f, 2.8f, 4.4f, SK); p.E(32.4f, 11.5f, 2.8f, 4.4f, SK);   // ears
    p.E(20, 8.5f, 24, 23, SK);
    p.Q(9, 10, 31, 10, 25, 21, 15, 21, SK);
    p.E(20, 20.6f, 10, 4.4f, SK);
    // front hair
    if (d.guy) {
        p.E(20, -1, 26, 13, HR);
        p.T(11, 0, 16, -9, 20, -1, HR); p.T(16, -1, 22, -10, 26, -1, HR); p.T(22, 0, 28, -7, 30, 2, HR);   // spiky quiff
        p.R(7.6f, 1, 2.6f, 9, HR); p.R(29.8f, 1, 2.6f, 9, HR);
        p.L(14, -4, 20, -6, 1.f, HRL);
    } else {
        p.E(20, -1, 27, 13, HR);
        switch (d.hairStyle) {
        case 0:   // side-swept fringe framing the face
            p.Q(7, -1, 31, -2, 27, 3, 8, 8, HR);
            p.Q(6, 2, 10.5f, 2, 9.5f, 27, 4, 29, HR); p.Q(29.5f, 2, 34, 2, 36, 29, 30.5f, 27, HR);
            break;
        case 1:   // middle part
            p.Q(20, -5, 8, 0, 6, 16, 12, 2, HR); p.Q(20, -5, 32, 0, 34, 16, 28, 2, HR);
            p.Q(5, 6, 10, 6, 10, 26, 4, 28, HR); p.Q(30, 6, 35, 6, 36, 28, 30, 26, HR);
            break;
        case 3:   // blunt bangs bob
            p.R(8, -2, 24, 6.5f, HR);
            p.Q(5, 2, 10, 2, 10, 20, 5, 21, HR); p.Q(30, 2, 35, 2, 35, 21, 30, 20, HR);
            break;
        default:  // sleek pulled back
            p.Q(8, 0, 32, 0, 30, 3, 10, 3, HR);
            break;
        }
        p.L(13, -3, 16, 4, 1.3f, HRL); p.L(25, -4, 27, 1, 1.f, HRL);   // shine streaks
    }
    // eyes
    static const RGB IRIS[4] = {{120, 80, 50}, {70, 140, 90}, {80, 120, 190}, {150, 100, 170}};
    static const RGB SHADOW[4] = {{255, 120, 190}, {190, 120, 230}, {230, 190, 110}, {200, 120, 90}};
    const RGB iris = IRIS[(d.skin + d.hairCol) % 4];
    const u32 BROW = C(shade(hair, d.hairCol >= 3 ? 0.55f : 0.85f));
    const bool closed = pose == PO_SLEEP || std::fmod(t * 0.9f + d.skin * 0.37f, 3.7f) < 0.12f;
    const float ey = 10.f;
    for (int side = 0; side < 2; side++) {
        float ex = side ? 25.4f : 14.6f, dir = side ? 1.f : -1.f;
        float w = d.guy ? 6.6f : 7.8f, h = d.guy ? 4.f : 5.f;
        if (!d.guy) p.E(ex + dir * 0.6f, ey - 1.8f, 8.8f, 4.6f, C(SHADOW[d.hairCol % 4], 150));
        if (closed) {
            p.L(ex - w / 2, ey, ex + w / 2, ey + 0.3f, 1.1f, DK);
            if (!d.guy) p.L(ex + dir * w / 2, ey + 0.2f, ex + dir * (w / 2 + 1.6f), ey - 1.4f, 0.7f, DK);
        } else {
            p.E(ex, ey, w, h, C(WHITE));
            p.Ci(ex + 0.2f, ey + 0.3f, h * 0.46f, C(iris));
            p.Ci(ex + 0.2f, ey + 0.3f, h * 0.22f, DK);
            p.Ci(ex + 0.9f, ey - 0.6f, 0.75f, C(WHITE));
            p.Ci(ex - 0.5f, ey + 1.1f, 0.35f, C(WHITE));
            // thick upper lid
            p.L(ex - w / 2, ey + 0.2f, ex - 1.2f, ey - h / 2 + 0.1f, 1.2f, DK);
            p.L(ex - 1.2f, ey - h / 2 + 0.1f, ex + 1.8f, ey - h / 2 + 0.2f, 1.2f, DK);
            p.L(ex + 1.8f, ey - h / 2 + 0.2f, ex + w / 2, ey + 0.1f, 1.2f, DK);
            if (!d.guy) {   // flirty winged lashes
                float ox = ex + dir * w / 2;
                p.L(ox, ey - 0.3f, ox + dir * 2.0f, ey - 2.4f, 0.8f, DK);
                p.L(ox - dir * 1.2f, ey - 1.8f, ox + dir * 0.6f, ey - 3.8f, 0.7f, DK);
                p.L(ex - w / 2 + 0.6f, ey + h / 2 - 0.4f, ex + w / 2 - 0.6f, ey + h / 2 - 0.4f, 0.4f, C(BLACK, 110));
            }
        }
        if (d.guy) p.L(ex - 3.5f, ey - 4.2f, ex + 3.5f, ey - 4.6f, 1.3f, BROW);
        else { p.L(ex - dir * 3.4f, ey - 4.4f, ex + dir * 0.4f, ey - 6.0f, 0.75f, BROW); p.L(ex + dir * 0.4f, ey - 6.0f, ex + dir * 4.2f, ey - 5.0f, 0.75f, BROW); }
    }
    // tiny nose
    p.L(19.4f, 14.6f, 20.8f, 15.4f, 0.6f, C(shade(skin, 0.72f)));
    // lips
    if (!d.guy) {
        const RGB L = d.lips;
        p.E(18.4f, 18.0f, 4.8f, 2.6f, C(L)); p.E(21.6f, 18.0f, 4.8f, 2.6f, C(L));
        p.E(20, 19.5f, 7.4f, 3.4f, C(shade(L, 1.08f)));
        p.L(16.6f, 18.7f, 23.4f, 18.7f, 0.5f, C(shade(L, 0.6f)));
        p.E(21.8f, 19.1f, 2.0f, 0.8f, C(255, 255, 255, 160));   // gloss
        p.Ci(11.2f, 15.4f, 2.6f, C(255, 120, 150, 60)); p.Ci(28.8f, 15.4f, 2.6f, C(255, 120, 150, 60));
        if (d.age != KID) { hoop(p, 8.2f, 17.6f, 2.8f, C(GOLD)); hoop(p, 31.8f, 17.6f, 2.8f, C(GOLD)); }
    } else {
        p.E(20, 18.8f, 5.4f, 1.8f, C(shade(skin, 0.7f)));
    }
}

// Aesthetic accessories for the sketch & doll styles
static void drawAccessories(const Pen& p, const PersonDef& d, int outfit, bool doll, float waistY) {
    if (outfit < FIRST_AESTHETIC || outfit >= NUM_OUTFITS) return;
    const OutfitDef& o = OUTFITS[outfit];
    const uint16_t a = o.acc;
    const u32 AC = C(o.accCol), ACD = C(shade(o.accCol, 0.7f));
    const float cx = 20, top = doll ? -3.5f : 3.0f, hw = doll ? 24.f : 14.5f, ey = doll ? 10.f : 13.f;
    const float exL = doll ? 14.6f : 16.3f, exR = doll ? 25.4f : 23.6f, neckY = doll ? 22.f : 23.5f;
    if (a & ACC_FLANNEL) {   // flannel tied around the hips
        p.R(13, waistY - 1.5f, 14, 3.4f, AC);
        p.L(14, waistY - 0.5f, 26, waistY - 0.5f, 0.6f, C(BLACK, 110)); p.L(20, waistY - 1.5f, 20, waistY + 1.9f, 0.6f, C(BLACK, 110));
        p.Q(13, waistY + 1, 16, waistY + 1, 15, waistY + 12, 12, waistY + 11, AC); p.Q(24, waistY + 1, 27, waistY + 1, 28, waistY + 11, 25, waistY + 12, ACD);
    }
    if (a & ACC_CHOKER) { p.R(17.6f, neckY, 4.8f, 1.5f, C(BLACK)); p.Ci(20, neckY + 2.2f, 0.8f, C(HOT)); }
    if (a & ACC_PEARLS) for (int i = 0; i < 7; i++) { float u = i / 6.f - 0.5f; p.Ci(20 + u * 7.f, neckY + 6 + (0.25f - u * u) * 5.f, 0.7f, C(255, 252, 240)); }
    if (a & ACC_STREAKS) {   // neon hair streaks
        p.L(cx - hw * 0.32f, top + 3, cx - hw * 0.42f, top + hw * 1.1f, 1.4f, AC);
        p.L(cx + hw * 0.36f, top + 2, cx + hw * 0.48f, top + hw * 1.0f, 1.2f, C(80, 200, 255));
    }
    if (a & ACC_BEANIE) {
        p.E(cx, top + hw * 0.18f, hw * 1.12f, hw * 0.62f, AC);
        p.R(cx - hw * 0.56f, top + hw * 0.22f, hw * 1.12f, hw * 0.16f, ACD);
        p.Ci(cx + hw * 0.1f, top - hw * 0.12f, hw * 0.08f, ACD);
    }
    if (a & ACC_TIARA) {
        p.L(cx - hw * 0.42f, top + hw * 0.16f, cx + hw * 0.42f, top + hw * 0.16f, 1.0f, C(230, 230, 245));
        p.T(cx - 2, top + hw * 0.16f, cx + 2, top + hw * 0.16f, cx, top - hw * 0.08f, C(230, 230, 245));
        p.Ci(cx, top + hw * 0.06f, 0.9f, C(HOT));
        p.Sp(cx - hw * 0.25f, top + hw * 0.1f, 1.6f, C(255, 255, 255, 230)); p.Sp(cx + hw * 0.25f, top + hw * 0.1f, 1.6f, C(255, 255, 255, 230));
    }
    if (a & ACC_CLIPS) for (int i = 0; i < 2; i++) {   // butterfly clips
        float bx = cx + (i ? 1 : -1) * hw * 0.36f, by = top + hw * 0.32f + i * 2;
        p.T(bx, by, bx - 2.2f, by - 1.6f, bx - 2.0f, by + 1.2f, AC); p.T(bx, by, bx + 2.2f, by - 1.6f, bx + 2.0f, by + 1.2f, AC);
        p.Ci(bx, by, 0.5f, ACD);
    }
    if (a & ACC_BOW) {
        float bx = cx + hw * 0.32f, by = top + hw * 0.1f, s = doll ? 4.5f : 3.2f;
        p.T(bx, by, bx - s, by - s * 0.7f, bx - s, by + s * 0.7f, AC); p.T(bx, by, bx + s, by - s * 0.7f, bx + s, by + s * 0.7f, AC);
        p.L(bx - 0.4f, by, bx - 1.6f, by + s * 1.3f, 0.9f, AC); p.L(bx + 0.4f, by, bx + 1.6f, by + s * 1.3f, 0.9f, AC);
        p.Ci(bx, by, s * 0.3f, ACD);
    }
    if ((a & ACC_HOOPS) && !doll) { hoop(p, 12.6f, 20.0f, 2.4f, C(GOLD)); }
    if (a & ACC_SHADES) {
        float w = doll ? 8.6f : 5.6f, h = doll ? 5.0f : 3.4f;
        p.E(exL, ey, w, h, C(30, 22, 30, 240)); p.E(exR, ey, w, h, C(30, 22, 30, 240));
        p.L(exL + w / 2, ey - 0.6f, exR - w / 2, ey - 0.6f, 0.7f, C(30, 22, 30));
        p.L(exL - w * 0.25f, ey - h * 0.15f, exL - w * 0.05f, ey - h * 0.3f, 0.6f, C(255, 255, 255, 170));
        p.L(exR - w * 0.25f, ey - h * 0.15f, exR - w * 0.05f, ey - h * 0.3f, 0.6f, C(255, 255, 255, 170));
    }
    (void)d;
}

// Map the Stick Figure Ver. wardrobe onto what the original art styles can draw
static Bottom legacyBt(int b) {
    switch (b) { case BT_JEANS: case BT_OVERALLS: case BT_SWEATS: return BT_PANTS; case BT_MAXI: return BT_GOWN; case BT_PLEATED: return BT_MINI; case BT_SWIM: return BT_SHORTS; default: return (Bottom)(b < 4 ? b : 0); }
}
PersonDef legacyLook(const PersonDef& d0, int& outfit) {
    PersonDef d = d0;
    static const uint8_t HS[16] = {2, 9, 7, 11, 5, 5, 6, 5, 5, 3, 4, 9, 5, 1, 0, 5};   // styles 12..27
    if (d.hairStyle >= 12 && d.hairStyle < 28) d.hairStyle = HS[d.hairStyle - 12];
    if (d.guy && d.hairStyle != 5 && d.hairStyle != 6 && d.hairStyle != 7 && d.hairStyle != 8 && d.hairStyle != 10 && d.hairStyle != 11 && d.hairStyle >= 12) d.hairStyle = 5;
    d.bt = legacyBt(d.bt);
    if (outfit > 0 && outfit < NUM_OUTFITS && (OUTFITS[outfit].bt >= 4 || outfit >= FIRST_NEW_OUTFIT)) {
        const OutfitDef& o = OUTFITS[outfit];
        d.top = o.top; d.bottom = o.bottom; d.bt = legacyBt(o.bt); d.topPattern = o.topPattern; d.shoes = o.shoes;
        outfit = 0;
    }
    return d;
}

void drawPerson(const PersonDef& d0, int outfit, float x, float footY, float s, float t, int holiday, Pose pose, bool flip) {
    if (d0.hairStyle >= 28) {   // the newest hairstyles are doll-only: other art styles draw their closest match
        static const uint8_t NEAR[NUM_HAIRSTYLES - 28] = {3, 0, 4, 9, 2, 1, 25, 12, 0, 4, 20, 15,
                                                       7, 7, 7, 1, 2, 5, 9, 1, 2, 1, 1, 9, 4, 1, 6, 7, 7, 5,
                                                       26, 26, 7, 6, 12, 4, 1, 6, 0, 7, 6, 8, 6, 3, 12, 5, 5, 6, 1, 6, 12, 8, 1, 0, 12, 8, 12, 0, 1, 0, 12, 6, 1, 1, 1, 1};
        PersonDef d1 = d0; d1.hairStyle = NEAR[(d0.hairStyle - 28) % (NUM_HAIRSTYLES - 28)];
        drawPerson(d1, outfit, x, footY, s, t, holiday, pose, flip); return;
    }
    if (artIsFlip(g_artStyle)) { drawFlipPerson(d0, outfit, x, footY, s, t, holiday, pose, flip); return; }
    if (artIsStick(g_artStyle)) { drawStickPerson(d0, outfit, x, footY, s, t, holiday, pose, flip); return; }
    if (d0.hairStyle >= 12 || d0.bt >= 4 || (outfit > 0 && outfit < NUM_OUTFITS && OUTFITS[outfit].bt >= 4)) {   // older styles only know the original wardrobe
        int o2 = outfit;
        PersonDef d1 = legacyLook(d0, o2);
        drawPerson(d1, o2, x, footY, s, t, holiday, pose, flip); return;
    }
    if (artIsGlam(g_artStyle) && d0.age != BABY) { drawGlamPerson(d0, outfit, x, footY, s * ageScale(d0.age), t, holiday, pose, flip); return; }
    // the older styles only know the original 6 tones / 8 colours / 6 hairstyles
    PersonDef d = d0;
    static const uint8_t SKMAP[4] = {0, 2, 4, 5}, HSMAP[6] = {1, 0, 1, 4, 3, 1};
    if (d.skin > 5) d.skin = SKMAP[(d.skin - 6) % 4];
    if (d.hairCol > 7) d.hairCol = 6;
    if (d.hairStyle > 5) d.hairStyle = d.guy ? 5 : HSMAP[(d.hairStyle - 6) % 6];
    if (d.age == BABY) { drawBaby(d, outfit, x, footY, s, t, holiday, pose, flip); return; }
    if (g_artStyle == 2) { drawPixelPerson(d, outfit, x, footY, s * ageScale(d.age) / (d.age == KID ? 0.85f : d.age == TEEN ? 0.97f : 1.f), t, holiday, pose, flip); return; }
    s *= ageScale(d.age);
    const bool sitSleep = pose == PO_SITSLEEP;
    if (sitSleep) pose = PO_SLEEP;
    if (pose == PO_LIE) pose = PO_IDLE;
    const bool sitting = pose == PO_SIT || sitSleep;
    const bool walking = pose == PO_WALK;
    const float wdx = walking ? std::sin(g_walkPhase * 6.2832f) * 3.2f : 0.f;
    float bob = pose == PO_DANCE ? -std::fabs(std::sin(t * 7.f)) * 4.f : walking ? -std::fabs(std::sin(g_walkPhase * 6.2832f)) * 1.5f : std::sin(t * 1.6f + d.skin) * 0.6f;
    if (sitting) bob = 0;
    Pen p(x, (sitting ? footY + 58 * s : footY) + bob, 40, 120, s, flip);

    const RGB skin = SKINS[d.skin], hair = HAIRS[d.hairCol];
    const RGB skinD = shade(skin, 0.85f);
    RGB top = d.top, bot = d.bottom, shoes = d.shoes;
    Bottom bt = d.bt;
    int pat = d.topPattern;
    if (outfit > 0) { const OutfitDef& o = OUTFITS[outfit]; top = o.top; bot = o.bottom; shoes = o.shoes; bt = o.bt; pat = o.topPattern; }
    if (d.guy && (bt == BT_MINI || bt == BT_GOWN)) bt = BT_PANTS;
    const bool heels = !d.guy && d.age != KID;
    const bool hasBang = !d.guy && d.age != KID && d.hairStyle != 4;
    const u32 SK = C(skin), SKD = C(skinD), HR = C(hair), HRL = C(shade(hair, 1.5f), 170);

    // ---- back hair ----
    if (g_dollStyle) dollBackHair(p, d, HR, t);
    else switch (d.guy ? 5 : d.hairStyle) {
    case 0: p.Q(8.5f, 6, 31.5f, 6, 33, 54, 7, 52, HR); p.T(7, 52, 14, 52, 9, 60, HR); p.T(33, 54, 27, 54, 31, 61, HR); break;
    case 1:
        p.Q(8, 6, 32, 6, 34, 50, 6, 50, HR);
        for (int i = 0; i < 4; i++) { p.Ci(6.5f, 22 + i * 9.5f, 3.6f, HR); p.Ci(33.5f, 22 + i * 9.5f, 3.6f, HR); }
        break;
    case 2: {
        float sw = std::sin(t * 2.f) * 2;
        p.Ci(29, 3, 4.2f, HR); p.Q(28, 4, 33, 5, 33 + sw, 34, 28 + sw, 32, HR); p.R(27, 2, 4, 2.5f, C(HOT));
        break;
    }
    case 3: p.Q(8.5f, 6, 31.5f, 6, 33, 24, 7, 24, HR); break;
    case 4: p.Ci(20, -1.5f, 5.5f, HR); p.L(15, -1, 25, -2, 1.f, HRL); break;
    default: break;
    }

    // ---- legs & shoes ----
    const u32 LEG = (bt == BT_PANTS) ? C(bot) : SK;
    const u32 LEGD = (bt == BT_PANTS) ? C(shade(bot, 0.85f)) : SKD;
    float lw = d.guy ? 1.3f : 0.f;
    const float footTop = sitting ? 94.f : 114.f;
    if (sitting) {   // thighs come toward us (foreshortened), shins hang down
        p.Q(14.5f - lw, 62, 19.6f, 62, 19.9f, 70, 13.2f - lw, 70, LEGD);
        p.Q(20.4f, 62, 25.5f + lw, 62, 26.8f + lw, 70, 20.1f, 70, LEG);
        p.Q(13.6f - lw, 69, 19.2f, 69, 18.4f, footTop, 15.8f - lw * 0.5f, footTop, LEGD);
        p.Q(20.8f, 69, 26.4f + lw, 69, 24.2f + lw * 0.5f, footTop, 21.6f, footTop, LEG);
    } else {
        p.Q(14.5f - lw, 62, 19.6f, 62, 18.6f + wdx, 114, 16 + wdx, 114, LEGD);
        p.Q(20.4f, 62, 25.5f + lw, 62, 24.2f - wdx, 114, 21.6f - wdx, 114, LEG);
    }
    const u32 SH = C(shoes);
    Pen ps = p;
    ps.oy += (footTop - 114.f) * s;
    for (int f = 0; f < 2; f++) {
        bool left = f == 0;
        float cx = left ? 16.4f + wdx : 23.6f - wdx;
        if (g_dollStyle && !d.guy) {   // chunky platforms
            const u32 SOLE = C(shade(shoes, 0.62f));
            ps.R(cx - 3.4f, 112.5f, 7.2f, 3.6f, SH); ps.R(cx - 4.0f, 116, 8.2f, 4, SOLE);
            if (left) ps.R(cx - 3.0f, 113.2f, 2.4f, 1.2f, C(255, 255, 255, 140));
        } else if (g_dollStyle) {
            ps.E(cx - 0.2f, 117, 9.5f, 6, SH); ps.R(cx - 4.6f, 118.5f, 9, 1.8f, C(WHITE));
        } else if (heels) {
            if (left) { ps.T(cx - 1.2f, 112.5f, cx + 3.0f, 113, cx - 3.2f, 119.6f, SH); ps.L(cx + 2.5f, 114, cx + 2.8f, 120, 1.3f, SH); ps.Ci(cx - 0.8f, 115.5f, 0.6f, C(255, 255, 255, 200)); }
            else { ps.T(cx - 2.4f, 112.5f, cx + 1.6f, 113, cx + 3.4f, 119.6f, SH); ps.L(cx - 1.9f, 114, cx - 2.2f, 120, 1.3f, SH); }
        } else {
            ps.E(cx, 117, 8, 5, SH);
            if (!d.guy && left) ps.L(cx - 1.9f, 115, cx + 2.1f, 115, 0.8f, C(shade(shoes, 0.7f)));
        }
    }

    // ---- bottoms ----
    const u32 BT = C(bot), BTD = C(shade(bot, 0.8f));
    switch (bt) {
    case BT_PANTS: p.Q(14.5f - lw, 50, 25.5f + lw, 50, 26 + lw, 64, 14 - lw, 64, BT); p.L(20, 56, 20, 64, 0.7f, BTD); break;
    case BT_MINI: p.Q(14.6f, 50, 25.4f, 50, 29.5f, 72, 10.5f, 72, BT); p.L(17, 54, 14, 71, 0.7f, BTD); p.L(23, 54, 26, 71, 0.7f, BTD); break;
    case BT_SHORTS: p.Q(14.2f, 50, 25.8f, 50, 27, 68, 13, 68, BT); p.L(20, 58, 20, 68, 0.8f, BTD); break;
    case BT_GOWN:
        p.Q(14.6f, 50, 25.4f, 50, 34, footTop + 4, 6, footTop + 4, BT);
        p.Q(20, 60, 25.4f, 52, 33, footTop + 4, 27, footTop + 4, BTD);
        for (int i = 0; i < 4; i++) p.Sp(12 + i * 6.f, 80 + (i * 13 % 30), 1.6f, C(255, 255, 255, (uint8_t)(120 + 100 * std::sin(t * 3 + i))));
        break;
    }

    // ---- torso ----
    float sh = d.guy ? 2.f : 0.f;
    const u32 TP = C(top);
    p.Q(12 - sh, 30, 28 + sh, 30, 25.5f, 51, 14.5f, 51, TP);
    if (!d.guy) p.Q(15, 30, 25, 30, 23, 34, 17, 34, SK);   // neckline
    topPatternOn(p, 31, 50, 12.5f - sh, 27.5f + sh, 14.8f, 25.2f, pat, (float)d.skin);
    if (bt != BT_GOWN && !d.guy) p.R(14.6f, 48.8f, 10.8f, 2.4f, C(GOLD));
    if (bt == BT_GOWN) { p.Q(14.6f, 44, 25.4f, 44, 25.4f, 51, 14.6f, 51, BT); }

    // ---- arms ----
    float ath = d.guy ? 3.8f : 3.1f;
    float lx = 12.5f - sh, rx = 27.5f + sh;
    float wave = std::sin(t * 9.f) * 4.f;
    // left arm
    if (pose == PO_DANCE) { p.L(lx, 31.5f, 7, 21 + wave * 0.3f, ath, SKD); p.L(7, 21 + wave * 0.3f, 6, 9, ath * 0.9f, SKD); }
    else if (sitting) { p.L(lx, 31.5f, 10, 46, ath, SKD); p.L(10, 46, 15, 60, ath * 0.9f, SKD); p.Ci(15.5f, 60.5f, 1.8f, SKD); }
    else { p.L(lx, 31.5f, 9 - wdx * 0.4f, 47, ath, SKD); p.L(9 - wdx * 0.4f, 47, 11 - wdx, 61, ath * 0.9f, SKD); p.Ci(11 - wdx, 61.5f, 1.8f, SKD); }
    // right arm
    if (pose == PO_PHONE && g_dollStyle) {   // phone held up by the ear, clear of the big doll face
        p.L(rx, 31.5f, 35.5f, 25, ath, SK); p.L(35.5f, 25, 34, 18, ath * 0.9f, SK);
        p.R(32.2f, 9.5f, 3.6f, 8, C(col::LIGHTGRAY)); p.R(32.7f, 10.2f, 2.6f, 3.2f, C(HOT));
    } else if (pose == PO_PHONE) {
        p.L(rx, 31.5f, 32, 23, ath, SK); p.L(32, 23, 27, 15.5f, ath * 0.9f, SK);
        p.R(25, 10.5f, 3.2f, 7, C(col::LIGHTGRAY)); p.R(25.4f, 11, 2.4f, 3, C(HOT));   // flip phone
    } else if (pose == PO_DANCE || pose == PO_WAVE) {
        p.L(rx, 31.5f, 33, 21 - wave * 0.3f, ath, SK); p.L(33, 21 - wave * 0.3f, 35 + (pose == PO_WAVE ? wave * 0.5f : 0), 9, ath * 0.9f, SK);
    } else if (sitting) {
        p.L(rx, 31.5f, 30, 46, ath, SK); p.L(30, 46, 25, 60, ath * 0.9f, SK); p.Ci(24.5f, 60.5f, 1.8f, SK);
    } else {
        p.L(rx, 31.5f, 31 + wdx * 0.4f, 47, ath, SK); p.L(31 + wdx * 0.4f, 47, 29.5f + wdx, 61, ath * 0.9f, SK); p.Ci(29.5f + wdx, 61.5f, 1.8f, SK);
        if (!d.guy && d.age != KID) {   // handbag
            p.Q(28, 56, 35, 56, 36, 64, 27, 64, C(d.topPattern == 2 ? GOLD : HOT));
            p.L(29, 56, 31.5f, 51, 0.8f, C(GOLD)); p.L(34, 56, 31.5f, 51, 0.8f, C(GOLD));
        }
    }
    if (!d.guy) { p.R(8.3f, 55, 3.4f, 1.2f, C(GOLD)); }   // bangle

    if (g_dollStyle) dollHead(p, d, pose, t);
    else {
    // ---- neck & head ----
    p.R(18.3f, 20, 3.6f, 11, SKD);
    p.E(20, 13, 14.5f, 19, SK);
    // front hair
    if (d.guy) {
        p.E(20, 5.5f, 16, 9, HR); p.R(12.5f, 5, 2.2f, 7, HR); p.R(25.3f, 5, 2.2f, 7, HR);
        p.L(14, 3, 23, 1.5f, 1.f, HRL);
    } else {
        p.E(20, 6, 16.5f, 11, HR);
        if (hasBang) {
            p.T(11.5f, 4, 29, 3, 13.5f, 21, HR);   // sweeping side bang over one eye
            p.T(11.5f, 4, 18, 4, 11.8f, 24, HR);
            p.L(15, 4.5f, 24, 3, 1.2f, HRL); p.L(14, 8, 16.5f, 16, 0.8f, HRL);
        } else {
            p.T(12, 4, 28, 4, 13, 12, HR);
        }
    }

    // ---- face ----
    const u32 DK = C(BLACK);
    const bool closed = pose == PO_SLEEP || std::fmod(t * 0.9f + d.skin * 0.37f, 3.7f) < 0.12f;
    auto eye = [&](float ex) {
        if (closed) { p.L(ex - 2.2f, 13.4f, ex + 2.2f, 13.4f, 0.8f, DK); return; }
        p.E(ex, 13, 4.4f, 2.5f, C(WHITE));
        p.Ci(ex + 0.4f, 13.1f, 1.15f, C(d.skin > 3 ? RGB{60, 36, 26} : RGB{90, 60, 110}));
        p.Ci(ex + 0.7f, 12.7f, 0.35f, C(WHITE));
        if (!d.guy) { p.L(ex - 2.4f, 12, ex + 2.6f, 11.6f, 0.9f, DK); p.L(ex + 2.6f, 11.6f, ex + 3.8f, 10.5f, 0.7f, DK); }
    };
    if (!(hasBang)) eye(16.3f);
    eye(23.6f);
    p.L(21.5f, 9.6f, 26, 9.1f, 0.7f, C(shade(hair, 0.8f)));
    if (!d.guy) {
        p.E(20.6f, 18.4f, 4.6f, 2.1f, C(d.lips));
        p.Ci(21.4f, 18.0f, 0.5f, C(255, 255, 255, 200));   // lip gloss
        p.Ci(25.2f, 16, 1.8f, C(255, 120, 150, 70));       // blush
        if (d.age != KID) { p.Ci(12.8f, 17.4f, 0.8f, C(GOLD)); p.Ci(12.8f, 19.2f, 1.f, C(WHITE)); p.Ci(12.8f, 21.4f, 1.1f, C(WHITE)); }
    } else {
        p.L(19, 18.6f, 22.4f, 18.4f, 0.7f, C(shade(skin, 0.6f)));
    }
    }
    if (pose == PO_SLEEP) {
        float zt = std::fmod(t, 2.f);
        text("z", p.X(28) + zt * 4, p.Y(-4) - zt * 6, 0.45f * s + 0.1f, C(255, 255, 255, (uint8_t)(220 - zt * 100)));
    }
    // occasional diva sparkle near the hair
    float sp = std::fmod(t * 0.5f + d.skin * 0.21f + d.hairCol * 0.13f, 3.f);
    if (sp < 0.5f) p.Sp(9, 6, 2.5f + sp * 3, C(255, 255, 255, (uint8_t)(220 - sp * 400)));

    drawAccessories(p, d, outfit, g_dollStyle, 50.f);
    Pen hp = p;
    if (g_dollStyle) hp.oy -= 6 * hp.s;
    if (holiday) drawHat(hp, holiday, t);
    if (outfit == 8 && !d.guy) {   // kitty ears for the Halloween costume
        hp.T(12, 2, 16, -5, 18, 1, C(BLACK)); hp.T(22, 1, 25, -5, 28, 2, C(BLACK));
        hp.T(13.5f, 1, 16, -3, 17, 1, C(HOT)); hp.T(23, 1, 25, -3, 26.5f, 1.5f, C(HOT));
    }
}

void drawPet(int pet, float x, float footY, float s, float t, bool sleeping, bool flip) {
    Pen p(x, footY, 44, 34, s, flip);
    const u32 BOW = C(HOT), BOWD = C(BERRY), DK = C(BLACK);
    float wag = std::sin(t * 10.f) * 4.f;
    auto petEye = [&](float ex, float ey) {
        if (sleeping) p.L(ex - 1.4f, ey, ex + 1.4f, ey, 0.8f, DK);
        else { p.Ci(ex, ey, 1.6f, DK); p.Ci(ex + 0.5f, ey - 0.5f, 0.5f, C(WHITE)); }
    };
    auto bow = [&](float bx, float by) {
        p.T(bx, by, bx - 4, by - 3, bx - 4, by + 3, BOW); p.T(bx, by, bx + 4, by - 3, bx + 4, by + 3, BOW); p.Ci(bx, by, 1.2f, BOWD);
    };
    switch (pet) {
    case 0: {   // chihuahua in rhinestone collar
        u32 f = C(222, 178, 128), fd = C(190, 140, 96);
        p.L(9, 18, 3, 12 - wag * 0.6f, 2.f, fd);
        p.R(12, 24, 3, 10, fd); p.R(25, 24, 3, 10, fd);
        p.E(20, 20, 22, 12, f);
        p.R(16, 24, 3, 10, f); p.R(29, 24, 3, 10, f);
        p.Ci(32, 12, 7.5f, f); p.E(37, 15, 7, 5, C(240, 210, 170));
        p.T(26, 8, 28, -4, 32, 6, f); p.T(34, 6, 40, -3, 38, 9, f);
        p.T(27.5f, 6, 28.5f, -1, 31, 6, C(BLUSH)); p.T(35, 6, 38.5f, 0, 37, 8, C(BLUSH));
        petEye(33, 11); p.Ci(40, 14, 1.3f, DK);
        for (int i = 0; i < 4; i++) p.Ci(27 + i * 1.8f, 18.5f + (i % 2) * 0.4f, 0.8f, C(255, 255, 255, 230));
        p.Sp(29, 18, 2, C(255, 255, 255, (uint8_t)(150 + 100 * std::sin(t * 5))));
        break;
    }
    case 1: {   // fluffy persian cat
        u32 f = C(248, 244, 240), fd = C(220, 212, 212);
        float tsw = std::sin(t * 2.f) * 3;
        p.L(7, 22, 2, 8 + tsw, 4.5f, fd);
        p.E(20, 22, 26, 16, f); p.Ci(12, 22, 6, fd);
        p.E(16, 31, 6, 5, f); p.E(26, 31, 6, 5, f);
        p.Ci(31, 13, 8, f);
        p.T(25, 8, 26, 0, 30, 6, f); p.T(33, 6, 37, 0, 37, 8, f);
        p.T(26, 6, 26.5f, 2, 29, 6, C(BLUSH)); p.T(34, 6, 36, 2.5f, 36, 7, C(BLUSH));
        if (sleeping) { p.L(27.5f, 13, 30.5f, 13, 0.8f, DK); p.L(32.5f, 13, 35.5f, 13, 0.8f, DK); }
        else { p.E(29, 12.5f, 3, 3.4f, C(90, 170, 220)); p.E(34.5f, 12.5f, 3, 3.4f, C(90, 170, 220)); p.R(28.6f, 11, 0.8f, 3, DK); p.R(34.1f, 11, 0.8f, 3, DK); }
        p.T(31, 16, 33, 16, 32, 17.5f, C(HOT));
        bow(26, 4);
        break;
    }
    case 2: {   // bunny
        u32 f = C(250, 236, 236), fd = C(228, 210, 212);
        float hop = sleeping ? 0 : std::fabs(std::sin(t * 2.5f)) * 2;
        Pen q(x, footY - hop * s, 44, 34, s, flip);
        q.Ci(8, 24, 4, C(WHITE));
        q.E(19, 24, 24, 18, f); q.E(24, 32, 10, 5, fd);
        q.Ci(30, 16, 7.5f, f);
        q.E(27, 2, 4.5f, 16, f); q.E(33, 2, 4.5f, 16, f); q.E(27, 3, 2, 11, C(BLUSH)); q.E(33, 3, 2, 11, C(BLUSH));
        if (sleeping) q.L(31, 15, 34, 15, 0.8f, DK); else { q.Ci(32, 15, 1.6f, DK); q.Ci(32.5f, 14.4f, 0.5f, C(WHITE)); }
        q.Ci(37, 18, 1.2f, C(HOT));
        q.T(29, 9, 25, 6, 25, 12, BOW); q.T(29, 9, 33, 6, 33, 12, BOW);   // bow between ears
        break;
    }
    case 3: {   // hamster
        u32 f = C(236, 186, 120), fw = C(255, 244, 228);
        p.E(22, 25, 24, 18, f); p.E(24, 28, 16, 10, fw);
        p.Ci(16, 17, 3, f); p.Ci(28, 17, 3, f); p.Ci(16, 17, 1.6f, C(BLUSH)); p.Ci(28, 17, 1.6f, C(BLUSH));
        petEye(19, 23); petEye(25, 23); p.Ci(22, 26, 0.9f, C(HOT));
        p.Ci(16, 28, 2, C(255, 150, 170, 120)); p.Ci(28, 28, 2, C(255, 150, 170, 120));
        p.E(18, 33, 4, 2.5f, C(BLUSH)); p.E(26, 33, 4, 2.5f, C(BLUSH));
        break;
    }
    case 4: {   // goldfish in a heart-trimmed bowl on a little stand
        p.R(12, 30, 20, 4, C(GOLD));
        p.Ci(22, 17, 14, C(190, 225, 245, 170));
        p.E(22, 20, 24, 14, C(120, 190, 235, 170));
        float fx = 22 + std::sin(t * 0.9f) * 6;
        bool dir = std::cos(t * 0.9f) > 0;
        float k = dir ? 1.f : -1.f;
        p.E(fx, 18, 9, 6, C(255, 150, 40));
        p.T(fx - 4 * k, 18, fx - 8 * k, 15 + std::sin(t * 8), fx - 8 * k, 21 - std::sin(t * 8), C(255, 120, 40));
        p.Ci(fx + 2.5f * k, 17, 0.8f, DK);
        float b = std::fmod(t, 2.f);
        p.Ci(fx + 4 * k, 14 - b * 5, 1 + b * 0.4f, C(255, 255, 255, 160));
        heart(p.X(22), p.Y(5), 3.f * s, C(HOT));
        p.E(16, 9, 5, 3, C(255, 255, 255, 120));
        break;
    }
    default: {   // pink poodle
        u32 f = C(255, 196, 220), fd = C(240, 160, 196);
        p.Ci(5, 13 - wag * 0.2f, 4, f); p.L(9, 20, 6, 14, 2, fd);
        p.R(13, 22, 3, 10, fd); p.R(26, 22, 3, 10, fd);
        p.Ci(14.5f, 32, 3, fd); p.Ci(27.5f, 32, 3, fd);
        p.E(20, 19, 20, 12, f); p.Ci(14, 18, 6, f); p.Ci(24, 18, 6, f);
        p.R(17, 22, 3, 10, f); p.R(30, 22, 3, 10, f);
        p.Ci(18.5f, 32, 3, f); p.Ci(31.5f, 32, 3, f);
        p.Ci(33, 10, 6.5f, f); p.Ci(33, 3, 5, f);   // pouf top
        p.E(28, 13, 5, 10, fd);
        p.E(39, 12, 7, 5, f); petEye(35, 9); p.Ci(42, 12, 1.2f, DK);
        bow(33, -1);
        break;
    }
    }
}
