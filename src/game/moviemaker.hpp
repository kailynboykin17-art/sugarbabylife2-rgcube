#pragma once
#include "common.hpp"
// Movie Maker (moviemaker.cpp): write your own Sims-style series. Series > Episodes > Scenes,
// each scene has a location, up to 3 cast members (position, outfit, acting), a camera angle,
// a transition and a script of dialogue / narration lines with expressions and actions.
// Movies are saved as lightweight scene scripts and replayed with the real characters & rooms.
void movieOpen();
bool movieUpdate(const Input& in);   // false = back to the menu
void movieDrawTop(float t);
void movieDrawBottom(float t);
