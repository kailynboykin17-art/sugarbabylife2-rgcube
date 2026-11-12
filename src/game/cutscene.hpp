#pragma once
#include "common.hpp"
// Six story cutscenes (letterboxed, skippable with START, replayable from the Scrapbook)
enum CsId { CS_INTRO, CS_DATE, CS_WEDDING, CS_BABY, CS_BUSTED, CS_SCANDAL, CS_COUNT };
extern const char* CS_NAMES[CS_COUNT];
void cutscenePlay(int id, bool replay = false);   // queues it (first time only unless replay)
bool cutscenePending();
void cutsceneBegin();                              // start the queued scene
bool cutsceneUpdate(const Input& in);              // false = finished
void cutsceneDrawTop(float t);
void cutsceneDrawBottom(float t);
