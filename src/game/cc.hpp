#pragma once
#include "common.hpp"
#include <string>
// Custom Content (cc.cpp): plain-text CC files on the SD card (APP_DIR/cc/...), loaded at startup.
// Types: furniture, hair, clothing, accessory, character. See CC_GUIDE.txt on the SD card.
#define CC_ROOT "sdmc:/LifeSim"
#define CC_DIR CC_ROOT "/Mods"   // Hair, Clothing, Accessories, Furniture, Characters, Animations, Backgrounds, Stories, UI
constexpr int CC_SLOTS = 32;           // build-mode slots for custom furniture
extern int g_ccBase[CC_SLOTS];
extern float g_ccTime;                  // drawSim time, for dance sway        // what each slot "works like" (bed, sofa...)
void ccLoadAll();                      // scan the SD card (call at boot and from the CC Manager)
void ccAssignSlots();                  // map custom furniture to build slots (after load / new game)
bool ccSlotUsed(int slot);
void ccDrawFurniture(int slot, float x, float footY, float s, int colorIdx, float t);
void ccDrawWear(const struct Sim& s, float x, float footY, float scale, bool flip, bool front, int pose);
int ccCount();
int ccBackgroundCount();                         // custom backgrounds (Movie Maker locations 200+)
const char* ccBackgroundName(int k);
void ccDrawBackground(int k, float t);
void ccOpen();
bool ccUpdate(const Input& in);        // false = back to menu
void ccDrawTop(float t);
void ccDrawBottom(float t);

uint32_t ccWearKey(const struct Sim& s);
void ccBakeWear(const struct Sim& s, uint8_t* rgba, int W, int H, int frames, float scale);
