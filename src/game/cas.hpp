#pragma once
#include "game.hpp"
// Create-a-Sim
void casStart(bool forBaby = false);
int casUpdate(const Input& in);   // 0 = still editing, 1 = done (world built), -1 = back to title, 2/3 = glam station done/cancel
void casStartEdit(int sim, int price);   // Glam Station: restyle an existing sim
bool casEditing();
void casDrawTop(float t);
void casDrawBottom(float t);
bool askText(const char* hint, std::string& out, int maxLen, bool numeric = false);
