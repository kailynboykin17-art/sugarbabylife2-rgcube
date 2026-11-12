#pragma once
#include "common.hpp"
// Household economy (economy.cpp): itemized bills & utilities with real consequences, a budget,
// the Yum Run food-delivery app, ShopNow online shopping (11 categories, grocery quality tiers,
// porch packages), shopping habits, and grocery freshness.
void economyIssueBills(int homeItems, int people);      // called Sundays
bool economyUtility(bool power, bool water);            // false = service cut off (blocks the action)
void economySpend(int cat, int amount);                 // track household spending
void economyHourly(int hour);
void economyDaily();
int  economyGroceryStars();                             // cooking bonus from grocery quality
void economyGroceriesStocked(int qty, int quality);     // fridge restocked
void walletAppUpdate(const Input& in); void walletAppDrawTop(float t); void walletAppDrawBottom(float t);
void yumAppUpdate(const Input& in); void yumAppDrawTop(float t); void yumAppDrawBottom(float t);
void shopAppUpdate(const Input& in); void shopAppDrawTop(float t); void shopAppDrawBottom(float t);
