#pragma once
#include "common.hpp"
// The pink flip-phone: Texts, Glam Date, InstaGlam, Map, Calendar, Planner,
// Family, People, Me (needs/skills), Jobs.
enum PhoneApp { AP_HOME, AP_TEXTS, AP_THREAD, AP_DATING, AP_SOCIAL, AP_MAP, AP_CAL, AP_PLANNER, AP_FAMILY, AP_PEOPLE, AP_ME, AP_JOBS, AP_DATELOC, AP_GOALS, AP_SCRAPBOOK, AP_TEA, AP_CAST, AP_STATUS, AP_SHOP, AP_LOT, AP_WALLET, AP_YUM, AP_HOTEL };
void phoneOpen(int app = AP_HOME);
void phoneOpenThread(int sim);   // jump to a text conversation
void phoneDatePick(int sim);     // the 'where should we go?' date picker
void phoneDatePickLater(int sim);   // they said yes by text: pick the place when you next open their thread
bool phoneUpdate(const Input& in);   // false = phone closed
void phoneDrawTop(float t);
void phoneDrawBottom(float t);
int phoneUnread();
