#pragma once
#include "common.hpp"
#include <string>
#include <vector>
// Sprite art (sprites.cpp): full-body character PNGs from the SD card (sdmc:/LifeSim/Sprites/*.png)
// replace the drawn doll for any Sim that has one. Files starting with "m_" are guys, anything else girls.
// Any size works: transparent PNGs, or a plain light background that gets cut away automatically.
#define SPRITE_DIR "sdmc:/LifeSim/Sprites"
void spriteScan();                                   // list the PNGs (boot + CC Manager reload)
const std::vector<std::string>& spriteNames();
bool spriteIsGuy(const std::string& name);
std::string spriteFor(const struct Sim& s);          // assigned or auto-picked sprite name ("" = use the doll)
uint32_t spriteKey(const struct Sim& s);             // changes when the Sim's sprite changes
bool spriteCast(uint32_t seed, bool guy, uint8_t* out);
bool spriteCast(uint32_t seed, bool guy, uint8_t* rgba96x208);  // story cast models in Maple Valley
bool spriteBig(const struct Sim& s, uint8_t* rgba96x208);       // false = no sprite, draw the doll
bool spriteWorld(const struct Sim& s, uint8_t* rgba3x48x104);   // 3 world frames (idle, step, step)
bool spriteDecode(const std::string& path, std::vector<uint8_t>& rgba, int& w, int& h, std::string* err);
bool spriteCanvas(const std::string& name, uint8_t* rgba96x208);  // for previews and tools

int spriteEnsureCast();   // SBL2: add the official pixel-doll cast as town NPCs (once per save)
