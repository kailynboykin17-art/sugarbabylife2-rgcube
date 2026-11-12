#pragma once
#include "common.hpp"
#include <string>
// STATUS: Starlight Bay's chaotic little social network (status.cpp).
// Townies post on their own every hour (moods, fits, couples, subtweets, shade, rumors,
// confessions, leaked group chats, fake accounts, deleted posts + receipts), reply to
// each other, remember what was said about them and hold it against the poster.
enum : int16_t { ST_ACC_TEA = -2, ST_ACC_CONFESS = -3, ST_ACC_FAKE = -4 };
void statusHourly(int hour);
void statusSeed();                       // fills an empty feed (new games / old saves)
void statusOnTalk(int target);           // they bring up a post when you talk to them
void statusPostAs(int who, const std::string& text, int about);   // a post from a townie / account (other systems)
// post kinds other systems can use (match status.cpp's Kind enum)
enum : int { SP_MOOD = 0, SP_OUTFIT = 1, SP_COUPLE = 2, SP_SUBTWEET = 3, SP_RUMOR = 5, SP_HYPE = 12, SP_EVENT = 17, SP_BREAKUP = 19, SP_BREAKING = 20 };
void statusPostKind(int who, int kind, const std::string& text, int about, int about2 = -1);
void statusAnnounce(const std::string& text, int about);   // @baybaytea posts about a big moment
std::string statusHandle(int who, int fakeOf = -1);
void statusAppDrawTop(float t);
void statusAppDrawBottom(float t);
void statusAppUpdate(const Input& in);
