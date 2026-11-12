#pragma once
#include "common.hpp"
#include <string>
#include <vector>
// Spicy Mode (18+, OFF by default): presentation level for private scenes between consenting adults
// (Fade to Black / Censored / Uncensored), the eligibility + consent gate, the Velvet Room club
// (adults only) and the FanVault creator app. Art is original placeholder silhouettes - no explicit art.
enum SpicyLevel : uint8_t { SPL_OFF, SPL_FADE, SPL_CENSORED, SPL_UNCENSORED, SPL_COUNT };
extern const char* SPICY_LEVEL_NAMES[SPL_COUNT];
int spicyLevel();
bool spicyEligible(int a, int b, std::string* why);   // adults, not family, private place, no minors present
bool spicyBegin(int a, int b);        // true = a scene started (main switches to the scene mode)
bool spicyPending();
bool spicyUpdate(const Input& in);    // false = scene over
void spicyDrawTop(float t);
void spicyDrawBottom(float t);
std::vector<std::pair<std::string, int>> spicySettingsOptions();   // ids 9500+
std::vector<std::pair<std::string, int>> fanVaultOptions();        // ids 9600+
std::vector<std::pair<std::string, int>> velvetSpotOptions(int spot); // ids 9700+
void spicyOption(int id);
bool spicyBlocks(int room, std::string* why);
void spicyDaily();
void spicyDrawRoom(int room, float t, bool night);
void spicySaveTo(const std::string& path);
void spicyLoadFrom(const std::string& path);
