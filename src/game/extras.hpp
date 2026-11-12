#pragma once
#include "common.hpp"
// Garden, gifts and the scrapbook.

// ---------------- backyard garden
constexpr int NUM_PLOTS = 6, NUM_CROPS = 6, NUM_GIFTS = 7;   // gifts = 6 crops + chocolates
extern const char* CROP_NAMES[NUM_CROPS];
extern const char* GIFT_NAMES[NUM_GIFTS];
int cropSeedPrice(int crop);       // crop 0..5
int cropDays(int crop);
int gardenReady();                 // plots ready to harvest
int gardenPlanted();
bool gardenNeedsWater();
bool gardenPlant(int crop);        // pays for seeds, fills the first empty plot
void gardenWater();
int gardenHarvest();               // returns items picked
int basketCount(int k);
int basketTotal();
int basketSellValue();
int sellHarvest();
void gardenDaily();
void gardenDraw(float t);          // the garden beds in the backyard
void giveGift(int item, int target);

// ---------------- scrapbook of memories
enum MemType : uint8_t { MEM_LOVE, MEM_FAMILY, MEM_CAREER, MEM_GOAL, MEM_FRIEND, MEM_FUN, MEM_LIFE, MEM_HOME };
void addMemory(int type, int a, int b, const std::string& text);
bool hasMemory(int type, int a, int b);
void addMemoryOnce(int type, int a, int b, const std::string& text);   // skips exact repeats
void scrapbookUpdate(const Input& in);
void scrapbookDrawTop(float t);
void scrapbookDrawBottom(float t);
