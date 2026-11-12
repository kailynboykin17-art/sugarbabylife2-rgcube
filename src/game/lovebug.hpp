#pragma once
#include "common.hpp"
// LOVEBUG: the dating app (lovebug.cpp)
void lovebugTick(float dt);        // every frame: delayed replies arrive
void lovebugHourly(int hour);      // new likes, texting personalities, townie dating drama
void lovebugUpdate(const Input& in);
void lovebugDrawTop(float t);
void lovebugDrawBottom(float t);
