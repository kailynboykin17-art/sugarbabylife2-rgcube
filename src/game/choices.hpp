#pragma once
#include "lifeext.hpp"
// Choice events (choices.cpp): drama pop-ups where you pick how to react ("Confront her / Let it go /
// Spill the tea"). Choices change relationships, reputation, mood & money right away, and some come
// back days later as follow-ups. Option ids 21000-21999.
bool choiceWanted();      // main opens choiceMenu() when this turns true
LxMenu choiceMenu();
LxMenu choiceOption(int id);
void choiceDaily();
void choiceHourly(int hour);
void choiceSaveTo(const std::string& path);
void choiceLoadFrom(const std::string& path);
