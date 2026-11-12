#pragma once
#include "game.hpp"

// ---------- clock & calendar ----------
// 1 year = 4 seasons x 7 days. Each day: Mon..Sun.
extern const char* WEEKDAY_NAMES[7];
extern const char* SEASON_NAMES[4];
inline int weekday(int day) { return day % 7; }
inline int seasonOf(int day) { return day < 0 ? 0 : (day / 7) % 4; }
inline bool isWeekday(int day) { return weekday(day) < 5; }
std::string clockStr(float minute);
std::string dateStr(int day);

// ---------- sims ----------
void buildWorld(Game& g, const Sim& player);   // household + townies + starter home
Sim randomSim(Stage st, int gender /*0 girl 1 guy -1 any*/);
std::string randomName(bool guy);
Emotion emotionOf(const Sim& s);
Emotion emotionOf(const Sim& s, int& strength);   // strength = summed moodlet power
int moodOf(const Sim& s);                       // -100..100
void addMoodlet(Sim& s, const char* name, Emotion e, int power, int minutes);
void skillGain(Sim& s, int skill, float amt);
int skillLevel(const Sim& s, int skill);
int fameStars();
const char* fameTitle();
std::string letterGrade(float pts);
int householdCount();
Pose stagePose(const Sim& s);

// ---------- relationships ----------
Rel& rel(int a, int b);
const Rel* findRel(int a, int b);
const char* relLabel(int a, int b);
bool canRomance(int a, int b);
bool isFamily(int a, int b);
int partnerOf(int sim);
void changeRel(int a, int b, int dFr, int dRo);

// ---------- life events ----------
void ageUp(int idx);
int makeBaby(int parentA, int parentB);
int traitSlotsFor(Stage st);
void furnishExtraRooms(RoomState* rooms);         // returns new sim index or -1
void moveIn(int simIdx);

// ---------- activities ----------
enum ActKind : uint8_t { AK_GENERIC, AK_SLEEP, AK_WORK, AK_SCHOOL, AK_CLASS, AK_HOMEWORK, AK_NOVEL, AK_DINNER, AK_DANCE, AK_SWIM };
struct Activity {
    std::string name;
    float left = 0, total = 0;
    float rate[N_COUNT] = {0, 0, 0, 0, 0};      // need change per game hour
    int skill = -1;
    float skillRate = 0;                         // skill points per game hour
    float speed = 8;                             // time multiplier while doing it
    int item = -1;                               // furniture index, -1 none
    uint8_t pose = PO_IDLE;
    uint8_t kind = AK_GENERIC;
    bool active() const { return left > 0; }
};
extern Activity ACT;
void startActivity(const Activity& a);
void cancelActivity();
Activity makeAct(const char* name, float minutes, float speed);

// ---------- world tick ----------
void worldTick(float realDt);                    // advances time, needs, events
float timeScale();

// ---------- texting / dating / social media ----------
ChatThread& threadFor(int sim);
void npcTexts(int sim, int prompt = -1);
struct TextChoice { const char* text; int8_t fr, ro; const char* reply; int8_t aboutFr = 0; };
// kind: 0 casual, 1 romantic, 2 drama from a rival, 3 drama about someone else ({x} = their name)
struct TextPrompt { const char* line; uint8_t kind; TextChoice c[3]; };
std::string fillName(const char* s, int about);
void npcDrama(int sim, int about);   // about < 0: rival drama, else gossip about that sim
void dramaTick();                    // random drama events (called hourly)
extern const TextPrompt PROMPTS[];
extern const int NUM_PROMPTS;
void answerPrompt(ChatThread& th, int choice);
void sendText(int sim, int kind);                // 0 hi, 1 selfie, 2 invite over, 3 ask date
std::vector<int> swipeCandidates();
bool swipeRight(int sim);
void makePost(int kind);                         // social media post
void townieChatter();
bool askOnDate(int sim, int loc);
void startDateIfDue();
void endDate(bool silent = false);

// ---------- careers ----------
bool atWorkTime(const Sim& s);
void finishShift(Sim& s, float hours, float bonusPerf);
void joinCareer(Sim& s, int career);

// ---------- town ----------
void travelTo(int room);
void spawnTownies(int room);
bool townieFits(int room, const Sim& s);   // who'd plausibly be out at this place
void randomWardrobe(PersonDef& d, Stage st);   // age/gender-appropriate mix & match clothes
