#pragma once
#include "game.hpp"
#include <cstdio>
#include <string>
// Starlight Cast: original characters with detailed dress-up-doll pixel portraits (static)
constexpr int OC_COUNT = 52;
struct OcDef { const char* name; const char* title; const char* bio; bool guy; Stage stage; };
const OcDef& ocDef(int i);
void ocPaint(int i, uint8_t* rgba, int& w, int& h);   // software render (96 x 208)
void ocDraw(int i, float x, float y, float scale = 1.f);   // stamp the portrait (top-left at x, y)
int ocSim(int i);              // the townie who is this character (-1 if not in town yet)
void ocEnsureInTown();
Sim ocMakeSim(int i);          // a ready-made Sim for this cast member (play as them!)         // adds the cast to the world (new games and old saves)
constexpr int OC_CELESTINE = 35, OC_ODETTE = 36, OC_JULIAN = 37;   // House of Vane (story mode)
void ocPaintDesign(int model, int top, int bottom, int shoes, RGB c1, RGB c2, RGB c3, uint8_t* rgba);
void ocDesignSet(int slot, int model, int top, int bottom, int shoes, RGB c1, RGB c2, RGB c3);   // 4 slots
void ocDesignDraw(int slot, float x, float y, float scale = 1.f);
int ocIndexOfSim(const std::string& name, const std::string& bio);   // cast index for a sim (-1 if not cast)
// everyone as a doll (townies, you, the cast): built from each sim's own look, cached
void ocBuildWorldSim(const Sim& s, uint8_t* out);   // 3 frames (stand, walk A, walk B), 48x104 each
void ocPaintSim(const Sim& s, uint8_t* rgba);       // full-size 96x208 doll of any sim (software)
void ocFrameTick();                                  // once per frame (paces doll building)
float ocDollHeight(const Sim& s);                    // feet to top of head, in pixels at scale 1
int ocDrawSim(const Sim& s, float x, float footY, float scale, int pose, bool flip, float t, float walkPhase);   // 0 n/a, 1 drawn, 2 building
bool ocDrawSimBust(const Sim& s, float cx, float cy, float r);
void ocOtomePaint(int i, uint8_t* rgba);                                  // Sugar Hearts: 0 Kailyn, 1..7 the guys
void ocOtomeDraw(int i, float x, float y, float scale, int rows = 208, int col0 = 0, int cols = 96);   // stamp (top-left), cropped
void ocSbhPaint(int i, uint8_t* rgba);                                     // Starlight Bay High cast (1..6)
void ocSbhDraw(int i, float x, float y, float scale, int rows = 208, int col0 = 0, int cols = 96);
void ocSimPortraitDraw(const Sim& s, float x, float y, float scale, int rows = 208, int col0 = 0, int cols = 96);   // any sim, big (one cached slot)
void ocAppDrawTop(float t);
void ocAppDrawBottom(float t);
void ocAppUpdate(const Input& in);

// Custom Heads (converted Sims 1 heads in romfs:/Heads or sdmc:/LifeSim/Heads), picked in Create-a-Sim > Pretty
int ocHeadCount();
std::string ocHeadName(int id);   // 1-based; 0 = "Doll Head"
