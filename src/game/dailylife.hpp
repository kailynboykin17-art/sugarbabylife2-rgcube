#pragma once
#include "lifeext.hpp"
#include "social.hpp"
#include <vector>
// Daily Life (dailylife.cpp): recipes & the fridge, grocery delivery, sick days, hair growth & salon, nails, weather outfits,
// group chats, exes, best friends, celebrity path, HOA drama, own business, side hustles, credit & loans, performance
// reviews, holidays (Halloween, Christmas, Valentine's, adult birthdays). Menu ids 19000-19999.
LxMenu dlyApp();
LxMenu dlyOption(int id);
void dlyHourly(int hour);
void dlyDaily();
bool dlySick(int sim);
void dlyCure(int sim);
void dlyNailsDone();
// likes & dislikes
enum { LK_FASHION, LK_MUSIC, LK_SPORTS, LK_COOKING, LK_GAMING, LK_BOOKS, LK_DOGS, LK_CATS, LK_PARTIES, LK_NATURE, LK_BEACH, LK_SHOPPING, LK_DANCING, LK_ART, LK_HORROR, LK_CARS, LK_COUNT };
extern const char* LIKE_NAMES[LK_COUNT];
int dlyLikes(int sim, int topic);   // +1 likes, -1 dislikes, 0 neutral
void dlyActivity(int topic);        // the player did something about this topic: moodlet if they love/hate it
void dlyLikeOptions(int target, std::vector<SocialOpt>& v);
bool dlyLikeSocial(int id, int target);
void dlySaveTo(const std::string& path);
void dlyLoadFrom(const std::string& path);
