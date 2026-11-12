#pragma once
#include "lifeext.hpp"
// Design Studio (designer.cpp): Tomodachi/New Leaf-style clothing designer. Draw a 32x32 design on the touch
// screen (16 colours, pen, fill, mirror, templates), save up to 8, and dress any sim in it. A sim wearing a
// design keeps look.topPattern = 100 + slot; the doll painter maps the pixels onto the top with fabric shading.
constexpr int DS_SLOTS = 8, DS_N = 32;
bool dsPixel(int slot, float u, float v, uint8_t& r, uint8_t& g, uint8_t& b);   // false = "keep outfit colour" pixel
uint32_t dsVersion(int slot);   // bumps on every edit (doll cache key)
LxMenu dsApp();
LxMenu dsOption(int id);        // ids 25000-25999
bool dsEditing();
bool dsUpdate(const Input& in); // false = leave the editor
void dsDrawTop(float t);
void dsDrawBottom(float t);
void dsSaveTo(const std::string& path);
void dsLoadFrom(const std::string& path);
