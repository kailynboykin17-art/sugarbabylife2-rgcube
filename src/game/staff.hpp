#pragma once
#include "common.hpp"
// Visible venue staff (clerks, host, waiter, chef, club dancer) and party guests who move around and
// "do" things. Called every frame from the actor update.
void staffFrame(float dt, bool partyOn);
