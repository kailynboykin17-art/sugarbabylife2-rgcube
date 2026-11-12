#pragma once
#include "game.hpp"
// Page 3+ traits (traits2.cpp): age gating & daily trait behaviours.
bool traitOk(int trait, int stage);   // adult-only traits never land on kids
void traitsDaily();
