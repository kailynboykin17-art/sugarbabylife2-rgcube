#pragma once
#include "common.hpp"
#include <string>
#include <vector>
// City content pack (content.cpp): 70 gifts with taste-based NPC reactions, 60 recipes cooked through the
// existing cooking minigame into real food, and 12 original TV shows (60 episodes) watchable offline.
enum ContentTab { CT_SHOP, CT_BAG, CT_RECIPES, CT_TV };
void contentOpen(int tab, int giveTarget = -1);
int contentUpdate(const Input& in);   // 0 stay, 1 leave, 2 start the cooking minigame
void contentDrawTop(float t);
void contentDrawBottom(float t);
// social: "Give a Gift >" lists these after the classic basket gifts
void contentGiftOptions(std::vector<std::pair<std::string, int>>& out);   // ids 1700+gift, 1800+food index
bool contentHasGiftables();
void contentGive(int optId, int sim);
// cooking minigame hook: turns the finished plates into a real dish when a recipe was chosen
bool contentCookDone(int plates, int good, int stars, int burnt);
bool contentCooking();
void contentDaily();
int contentToGoFor(int da);   // store snack you can take to go (-1 none)
const char* contentToGoName(int k);
int contentToGoPrice(int k);
void contentBuyToGo(int k);
std::string contentLine(const char* ctx, uint32_t seed);   // a line from the original dialogue library
