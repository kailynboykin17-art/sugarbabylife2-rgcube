#pragma once
#include "common.hpp"
// Photo Mode (photomode.cpp): hides the HUD, pose any sim in the room with tween poses, move them, add a filter,
// frame & stickers, then SNAP - saves a real 400x240 .bmp to sdmc:/LifeSim/photos and logs it in the Photo Album.
extern bool g_photoClean;
void photoOpen();
bool photoUpdate(const Input& in);   // false = back to play
void photoDrawOverlay(float t);      // after the world on the top screen
void photoDrawBottom(float t);
void photoAfterFrame();              // call right after C3D_FrameEnd: writes the pending capture
