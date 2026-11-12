#pragma once
#include "lifeext.hpp"
// Game Settings (settings2.cpp): one page to switch whole systems on/off, a How to Play guide and What's New.
enum { GS_EVENTS, GS_FIRES, GS_DRAMA, GS_CHOICES, GS_POPUPS, GS_PULSE, GS_EMOTES, GS_TIPS, GS_COUNT };
bool gsOn(int k);
LxMenu gsApp();
LxMenu gsOption(int id);   // ids 23000-23999
bool gsWantGuide();        // a brand-new save opens the guide once
LxMenu gsGuide();
extern const char* GAME_VERSION;
void gsSaveTo(const std::string& path);
void gsLoadFrom(const std::string& path);
