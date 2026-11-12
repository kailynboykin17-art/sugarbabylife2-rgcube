#pragma once
#include "common.hpp"

enum Food : int { F_CHICKEN, F_PIZZA, F_COOKIES, F_EGGS, F_PANCAKES, F_CUPCAKES, F_BURGER, F_PASTA, F_TACOS, F_MAC, F_LASAGNA, F_BROWNIES, F_FRIEDRICE, F_RAMEN, F_GRILLCHEESE, F_STEAK, F_CINNAROLL, F_SMOOTHIEBOWL, F_COUNT };
enum FoodState : int { FS_RAW, FS_DONE, FS_BURNT };
extern const char* FOOD_NAMES[F_COUNT];

void drawFood(int food, int state, float x, float y, float s, float t);
void drawDrink(int drink, float x, float y, float s, float fill);
constexpr int NUM_DRINKS = 6;
extern const char* DRINK_NAMES[NUM_DRINKS];

void cookStart();
bool cookUpdate(const Input& in);   // false = leave the kitchen
void cookDrawTop(float t);
void cookDrawBottom(float t);
