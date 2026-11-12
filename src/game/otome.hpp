#pragma once
#include "common.hpp"
// Sugar Hearts: Otome Ver. - a portrait story mode inside Sugar Bunny Life.
// Turn the 3DS sideways like a book: the top screen is the left page (portraits & scenes),
// the touch screen is the right page (dialogue & choices). Seven romance routes for Kailyn.
void otomeOpen();
bool otomeUpdate(const Input& in);   // false = back to town
void otomeDrawTop(float t);
void otomeDrawBottom(float t);
