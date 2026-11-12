#pragma once
#include "lifeext.hpp"
// Town Pulse (townpulse.cpp): the neighborhood lives on without you. Every day townies make friends, feud,
// date, break up, get engaged, marry, have babies, get promoted, quit jobs and throw parties - reported
// through the news, gossip and pop-ups when it's someone you know. A log lives in the Dynasty app.
void tpDaily();
LxMenu tpLog();
void tpSaveTo(const std::string& path);
void tpLoadFrom(const std::string& path);
