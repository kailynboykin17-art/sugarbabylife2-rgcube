#pragma once
#include <string>
// Emotion bubbles (emote.cpp): Sims-style icons that pop over a sim's head (heart, sparkle, rain cloud, anger,
// shock, money, music, sweat, idea, sick) with a random reaction word, plus a matching tween reaction.
enum { EMO_LOVE, EMO_HAPPY, EMO_SAD, EMO_ANGRY, EMO_SHOCK, EMO_MONEY, EMO_MUSIC, EMO_SWEAT, EMO_IDEA, EMO_SICK, EMO_COUNT };
void emotePlay(int sim, int kind, const std::string& word = "");
void emoteFromText(int sim, const std::string& msg);   // reads a message and reacts (no-op if nothing fits)
void emoteDraw(int sim, float x, float headY, float t);
