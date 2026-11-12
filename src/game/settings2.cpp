#include "levels.hpp"
#include "settings2.hpp"
#include "game.hpp"
#include "extras.hpp"
#include "nightlife.hpp"
#include "dynasty.hpp"
#include <cstdio>
#include "vfs.hpp"
#define fopen vfopen

const char* GAME_VERSION = "v3.0";
namespace {
struct GS { uint32_t magic = 0x47533031; uint8_t on[GS_COUNT] = {1, 1, 1, 1, 1, 1, 1, 1}; uint8_t guideSeen = 0, pad[3] = {}; };
GS S; bool wantGuide = false;
const char* NAMES[GS_COUNT] = {"Random Life Events", "House Fires", "Family Drama & Sibling Fights", "Choice Pop-ups", "Little Random Pop-ups", "Town Pulse (NPC lives)", "Emotion Bubbles", "First-time App Tips"};
}
bool gsOn(int k) { return k < 0 || k >= GS_COUNT || S.on[k]; }

LxMenu gsApp() {
    LxMenu m;
    m.push_back({std::string("Sugar Bunny Life 2 ") + GAME_VERSION, 23999});
    m.push_back({std::string("Game Mode & Levels: ") + (lvlLevels() ? "Levels" : "Ultimate Sandbox"), 28000});
    m.push_back({"How to Play", 23100});
    m.push_back({"What's New", 23200});
    for (int k = 0; k < GS_COUNT; k++) m.push_back({std::string(S.on[k] ? "[ON]  " : "[OFF] ") + NAMES[k], 23001 + k});
    m.push_back({std::string(nlAfterHours() ? "[ON]  " : "[OFF] ") + "After Hours drug content (18+)", 23020});
    m.push_back({"Spicy 18+ settings...", 23021});
    return m;
}

LxMenu gsGuide() {
    LxMenu m;
    static const char* G2[] = {
        "Tap anything in a room (people, furniture, spots) to see what you can do.",
        "Open the PHONE for everything else. Page 1 = main apps, page 2 = folders.",
        "MAP: travel anywhere. Tap Next > for more pages (Metro, Town, Main Street).",
        "At home, tap Home on the Map for the DOLLHOUSE view of every room.",
        "Needs bars: keep your sim fed, rested, clean and social.",
        "Grow Up app: family, kids, teens, school, story mode & difficulty.",
        "Daily Life app: fridge, recipes, BFFs, exes, business, loans, notifications.",
        "Dynasty app: 10-generation legacy, random events, rotating households.",
        "Build mode: Buy furniture, Walls, Floors, and Themes > Furnished Rooms.",
        "Drama pops up on its own - pick how to react. Choices come back later!",
        "Game Settings app: switch any system off if it's too much.",
        "Your game autosaves. Back up the LifeSim folder on your SD card sometimes!"};
    for (const char* s : G2) m.push_back({s, 23999});
    return m;
}

LxMenu gsOption(int id) {
    LxMenu m;
    if (id >= 23001 && id < 23001 + GS_COUNT) { S.on[id - 23001] ^= 1; return gsApp(); }
    if (id == 23020) { nlSetAfterHours(!nlAfterHours()); return gsApp(); }
    if (id == 23100) return gsGuide();
    if (id == 23200) {
        static const char* W[] = {
            "v3.0 What's New:",
            "Dollhouse view, 12 furnished room themes, 7 new beds + 18 furniture",
            "32 new traits (4 trait pages), family dynamics",
            "Emotion bubbles, 112 choice events, 50 random pop-ups, Town Pulse",
            "Sugar Rivals 3v3 hero arena (17 heroes), Photo Mode",
            "Dynasty: 10 generations, random life events, rotational play",
            "Daily Life: recipes, sick days, likes & dislikes, business, loans, holidays",
            "Salon, Roller Rink, Night Market, Courthouse, school rooms, daycare & more",
            "Kid Life & Grow Up: toys, first crush, school, teens, graduations",
            "One-file saves, 8 save slots, wills, real moving out",
            "Tap-fast freeze fix, buttons that never overflow, notifications log"};
        for (const char* s : W) m.push_back({s, 23999});
        return m;
    }
    return m;
}

bool gsWantGuide() { if (!wantGuide) return false; wantGuide = false; return true; }
void gsSaveTo(const std::string& p) { S.guideSeen = 1; FILE* f = fopen(p.c_str(), "wb"); if (!f) return; fwrite(&S, sizeof S, 1, f); fclose(f); }
void gsLoadFrom(const std::string& p) {
    S = GS{}; FILE* f = fopen(p.c_str(), "rb");
    if (f) { GS g; if (fread(&g, 1, sizeof g, f) >= 8 && g.magic == S.magic) S = g; fclose(f); }
    wantGuide = !S.guideSeen && G.day <= 1;   // new saves: show the guide once
}
