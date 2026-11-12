#pragma once
#include "social.hpp"
// Raising kids (kids.cpp): babies, toddlers and kids develop motor skills, talking, potty training,
// imagination, social skills and discipline through what you do with them; they hit milestones
// (first word, first steps, potty trained, reading, riding a bike...), throw tantrums when their
// needs are low, and the way you parent (warm vs strict) shapes the traits they grow up with.
void kidsOptions(int target, std::vector<SocialOpt>& v);   // adds parenting interactions
bool kidsDoSocial(int id, int target);                       // true if handled
void kidsHourly(int hour);
void kidsOnAgeUp(int sim);                                   // traits from how they were raised
std::string kidsSummary(int sim);                            // "3/8 milestones" etc. ("" for grown-ups)
