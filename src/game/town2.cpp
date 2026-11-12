#include "town2.hpp"
#include "growup.hpp"
#include "kidlife.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "district.hpp"
#include "extras.hpp"
#include "tween.hpp"
#include <cmath>
#include <cstring>
#include "people.hpp"
#include "dailylife.hpp"
void lifeMarry(int a, int b);
#include <algorithm>
using namespace col;

bool town2Room(int r) { return r >= R_FOODCOURT && r <= R_COURTHOUSE; }
static Sim& me() { return MC(); }
static void need(int n, float d) { Sim& s = me(); s.needs[n] = std::max(0.f, std::min(100.f, s.needs[n] + d)); }
static bool pay(int c) { if (G.money < c) { toast("Not enough money ($" + std::to_string(c) + ")."); return false; } G.money -= c; return true; }
static bool kidOrTeen() { return me().stage >= ST_CHILD && me().stage <= ST_TEEN; }
static bool hasToddler(int* who = nullptr) { for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household && G.sims[i].stage == ST_TODDLER) { if (who) *who = i; return true; } return false; }
static bool atDaycare = false;

std::vector<std::pair<std::string, int>> town2SpotOptions(int k) {
    std::vector<std::pair<std::string, int>> v; const int st = me().stage;
    switch (k) {
    case SP_FCFOOD: v.push_back({"Pizza Slice ($6)", 18001}); v.push_back({"Boba Tea ($5)", 18002}); v.push_back({"Pretzel Bites ($4)", 18006}); break;
    case SP_FCTABLE: v.push_back({"Hang Out with Friends", 18003}); v.push_back({"People-Watch", 18007}); break;
    case SP_T2PHOTOBOOTH: v.push_back({"Photo Booth ($5)", 18004}); break;
    case SP_PIERCE: v.push_back({"Ear Studs ($25)", 18005}); v.push_back({"Hoops ($30)", 18008}); v.push_back({"Triple Studs ($35)", 18009}); if (st >= ST_TEEN) v.push_back({"Nose Stud ($40)", 18017}); break;
    case SP_LOCKER: v.push_back({"Open Your Locker", 18010}); v.push_back({"Decorate Your Locker ($5)", 18011}); break;
    case SP_BULLETIN: v.push_back({"Read the Bulletin Board", 18012}); break;
    case SP_DESK: if (kidOrTeen()) { v.push_back({"Pay Attention", 18020}); v.push_back({"Pass Notes", 18021}); v.push_back({"Raise Your Hand", 18022}); v.push_back({"Cheat on the Test", 18025}); } else v.push_back({"Help the Teacher (volunteer)", 18024}); break;
    case SP_CHALK: v.push_back({"Solve a Problem on the Board", 18023}); break;
    case SP_LUNCHLINE: v.push_back({"Get School Lunch ($3)", 18030}); break;
    case SP_LUNCHTABLE: v.push_back({"Sit with Friends", 18031}); v.push_back({"Trade Snacks", 18032}); v.push_back({"Start a Food Fight!", 18033}); break;
    case SP_T2BLEACHERS: v.push_back({"Watch the Big Game", 18040}); v.push_back({"Pep Rally!", 18041}); break;
    case SP_T2COURT: v.push_back({"Gym Class Dodgeball", 18042}); v.push_back({"Cheer Practice", 18043}); break;
    case SP_CRIB: if (hasToddler()) { v.push_back({atDaycare ? "Pick Up Your Toddler" : "Drop Off Your Toddler ($20)", atDaycare ? 18051 : 18050}); } else v.push_back({"(No toddler to drop off)", 18899}); break;
    case SP_PLAYMAT: if (st >= ST_TEEN) v.push_back({"Work a Daycare Shift", 18052}); v.push_back({"Story Time Circle", 18053}); v.push_back({"Finger Painting", 18054}); break;
    case SP_PEW: v.push_back({"Attend Service", 18060}); v.push_back({"Sing in the Choir", 18061}); break;
    case SP_BAKESALE: v.push_back({"Bake Sale Treat ($3)", 18062}); v.push_back({"Volunteer: Food Drive", 18063}); break;
    case SP_CCDESK: v.push_back({"Volunteer", 18070}); v.push_back({"Take a Pottery Class ($15)", 18071}); v.push_back({"Bingo Night", 18073}); break;
    case SP_T2STORYRUG: v.push_back({"Library Story Time", 18072}); v.push_back({"Read to the Little Kids", 18074}); break;
    case SP_POOLWATER: v.push_back({"Swim Laps", 18080}); v.push_back({"Cannonball Contest", 18081}); v.push_back({"Marco Polo", 18082}); break;
    case SP_SNACKBAR: v.push_back({"Snow Cone ($4)", 18083}); if (st >= ST_TEEN) v.push_back({"Lifeguard Shift", 18084}); break;
    case SP_GRILL: v.push_back({"Grab a Burger", 18090}); v.push_back({"Man the Grill", 18091}); break;
    case SP_BOUNCE: if (st <= ST_PRETEEN) v.push_back({"Bounce House!", 18092}); else v.push_back({"Supervise the Bounce House", 18095}); v.push_back({"Sidewalk Chalk", 18096}); break;
    case SP_DJ: v.push_back({"Dance in the Street", 18093}); v.push_back({"Meet the Neighbors", 18094}); break;
    case SP_DORMDESK: v.push_back({"Study for Finals", 18100}); v.push_back({"Pull an All-Nighter", 18101}); break;
    case SP_DORMBUNK: v.push_back({"Power Nap", 18102}); v.push_back({"Gossip with Your Roommate", 18105}); break;
    case SP_SALONCHAIR: v.push_back({"Trim ($20)", 18200}); v.push_back({"Silk Press ($60)", 18201}); v.push_back({"Knotless Braids ($150)", 18202}); v.push_back({"Box Braids ($120)", 18203}); v.push_back({"Blowout Curls ($50)", 18204}); v.push_back({"Salon Gossip", 18205}); break;
    case SP_NAILBAR: v.push_back({"Full Set - Y2K Glitter ($40)", 18210}); v.push_back({"French Tips ($35)", 18211}); v.push_back({"Fill ($20)", 18212}); v.push_back({"Polish: Hot Pink ($15)", 18213}); v.push_back({"Polish: Cherry Red ($15)", 18214}); v.push_back({"Polish: Black ($15)", 18215}); v.push_back({"Polish: Lilac ($15)", 18216}); break;
    case SP_HOSPDESK: v.push_back({"Checkup ($30)", 18220}); v.push_back({"Visit the Newborns", 18221}); break;
    case SP_HOSPBED: v.push_back({"ER Visit ($150, cures colds fast)", 18222}); if (st >= ST_YA) v.push_back({"Ultrasound Appointment", 18223}); break;
    case SP_RINKFLOOR: v.push_back({"Skate to the Music", 18230}); v.push_back({"Couples Skate", 18231}); v.push_back({"Limbo Contest", 18232}); break;
    case SP_SKATERENT: v.push_back({"Rent Skates ($5)", 18233}); v.push_back({"Arcade Claw Machine ($2)", 18234}); break;
    case SP_MKSTALL: v.push_back({"Funnel Cake ($6)", 18240}); v.push_back({"Street Tacos ($8)", 18241}); v.push_back({"Buy Handmade Jewelry ($15)", 18242}); break;
    case SP_MKGAMES: v.push_back({"Ring Toss ($3)", 18243}); v.push_back({"Ferris Wheel at Night ($4)", 18244}); break;
    case SP_CLERK: if (st >= ST_YA) { v.push_back({"Get a Marriage License ($50)", 18250}); v.push_back({"Change Your Last Name", 18251}); } v.push_back({"Pay a Parking Ticket ($40)", 18252}); break;
    case SP_JUDGE: if (st >= ST_YA) v.push_back({"Custody Hearing", 18253}); v.push_back({"Jury Duty", 18254}); v.push_back({"Watch a Trial", 18255}); break;
    case SP_GREEK: if (me().isAdult() || st == ST_YA) { v.push_back({"Frat / Sorority Party", 18103}); v.push_back({"Rush a House", 18104}); } else v.push_back({"(College students only)", 18899}); break;
    }
    return v;
}

LxMenu town2Option(int id) {
    Sim& s = me();
    auto say = [](const std::string& t) { toast(t); };
    switch (id) {
    case 18001: if (pay(6)) { need(N_HUNGER, 40); say("Cheesy pizza, food court style."); } break;
    case 18002: if (pay(5)) { need(N_HUNGER, 10); need(N_FUN, 10); say("Brown sugar boba. Elite."); } break;
    case 18006: if (pay(4)) { need(N_HUNGER, 25); say("Warm pretzel bites with cheese dip."); } break;
    case 18003: need(N_SOCIAL, 30); need(N_FUN, 10); say("You and your friends take over a whole table."); break;
    case 18007: need(N_FUN, 8); say(irand(2) ? "A couple is breaking up at the Cinnabon. Riveting." : "Someone's mom is power-walking laps."); break;
    case 18004: if (pay(5)) { guPhoto("Food court photo booth"); tweenPlay(G.mc, TW_HOP); say("Four poses! Saved to the Photo Album."); } break;
    case 18005: case 18008: case 18009: case 18017: { const int price = id == 18005 ? 25 : id == 18008 ? 30 : id == 18009 ? 35 : 40;
        if (s.stage <= ST_PRETEEN && irand(2)) { say("\"I'll need a parent's signature, sweetie.\""); break; }
        if (!pay(price)) break; s.look.f.pierce = (uint8_t)(id == 18005 ? 1 : id == 18008 ? 2 : id == 18009 ? 8 : (s.look.f.pierce == 2 ? 4 : 3));
        addMoodlet(s, "New Piercings!", EM_CONFIDENT, 1, 1440); say("OW. ...okay, they look SO cute."); break; }
    case 18010: { static const char* L[5] = {"A folded note falls out: \"u r cool\" - no name!", "Your gym clothes. Smelly.", "Someone left you a candy!", "You forgot your homework IN here.", "Your locker mirror says you look great."};
                  say(L[irand(5)]); break; }
    case 18011: if (pay(5)) { addMoodlet(s, "Cute Locker", EM_HAPPY, 1, 1440); say("Glitter, photos and a tiny chandelier. Iconic."); } break;
    case 18012: { static const char* B[4] = {"CLUBS FAIR this Friday!", "Talent show sign-ups open!", "Lost: one (1) retainer. Please return.", "School dance - tickets $5!"}; say(B[irand(4)]); break; }
    case 18020: s.gradePts = std::min(100.f, s.gradePts + 3); s.skills[SK_LOGIC] = std::min(10.f, s.skills[SK_LOGIC] + 0.1f); need(N_FUN, -5); say("You actually learned something!"); break;
    case 18021: need(N_SOCIAL, 12); if (irand(3) == 0) { s.gradePts -= 2; say("BUSTED. The teacher reads your note out loud."); } else say("Note passed successfully. Giggles."); break;
    case 18022: if (irand(100) < 50 + (int)s.skills[SK_LOGIC] * 5) { s.gradePts = std::min(100.f, s.gradePts + 2); addMoodlet(s, "Got It Right", EM_CONFIDENT, 1, 240); say("Correct! Teacher's pet."); } else say("Wrong answer. Someone snickers."); break;
    case 18025: if (irand(100) < 35) { s.gradePts = std::max(0.f, s.gradePts - 15); G.mcGrounded = std::max<uint8_t>(G.mcGrounded, 3); addMoodlet(s, "Caught Cheating", EM_SAD, 2, 1440); lxRepAdd(CI_SCHOLARS, -5, "caught cheating"); say("CAUGHT! Zero on the test, a call home, and you're grounded."); }
        else { s.gradePts = std::min(100.f, s.gradePts + 6); addMoodlet(s, "Got Away With It", EM_TENSE, 1, 600); say("You copied the answers... and got away with it. This time."); } break;
    case 18023: s.skills[SK_LOGIC] = std::min(10.f, s.skills[SK_LOGIC] + 0.2f); say("You solve it at the board. The class claps (sarcastically?)."); break;
    case 18024: need(N_SOCIAL, 10); say("You help hand out worksheets. The kids are adorable chaos."); break;
    case 18030: if (pay(3)) { need(N_HUNGER, 35); say(irand(2) ? "Rectangle pizza day!" : "Mystery meat Monday..."); } break;
    case 18031: need(N_SOCIAL, 25); say("Lunch table gossip: who likes who, who's fighting, who's moving."); break;
    case 18032: need(N_HUNGER, 10); need(N_SOCIAL, 10); say("You trade your carrots for a fruit snack. Business genius."); break;
    case 18033: need(N_FUN, 25); need(N_HYGIENE, -30); if (kidOrTeen() && irand(2)) { G.mcGrounded = std::max<uint8_t>(G.mcGrounded, 2); say("FOOD FIGHT! ...detention AND grounded."); } else say("FOOD FIGHT!! Legendary."); break;
    case 18040: need(N_FUN, 20); need(N_SOCIAL, 15); say(irand(2) ? "We WON! The bleachers go wild!" : "We lost... but the snack bar was good."); break;
    case 18041: need(N_FUN, 20); tweenPlay(G.mc, TW_CHEER); say("WE GOT SPIRIT, YES WE DO!"); break;
    case 18042: dlyActivity(LK_SPORTS); need(N_FUN, 15); need(N_ENERGY, -15); s.skills[SK_FITNESS] = std::min(10.f, s.skills[SK_FITNESS] + 0.15f); say(irand(2) ? "You're the last one standing in dodgeball!" : "Hit in the face first round. Classic."); break;
    case 18043: need(N_ENERGY, -15); s.skills[SK_FITNESS] = std::min(10.f, s.skills[SK_FITNESS] + 0.2f); tweenPlay(G.mc, TW_ARMSUP); say("5, 6, 7, 8! You nail the toe touch."); break;
    case 18050: { int t; if (!hasToddler(&t) || !pay(20)) break; atDaycare = true; addMoodlet(G.sims[t], "Daycare Friends", EM_HAPPY, 1, 600); say(G.sims[t].name.substr(0, G.sims[t].name.find(' ')) + " runs straight to the block corner. No tears!"); break; }
    case 18051: atDaycare = false; say("Pickup! They made you a macaroni necklace."); break;
    case 18052: { const int pay2 = 30 + irand(15); G.money += pay2; need(N_ENERGY, -20); say("Daycare shift: naps, snacks, 4 diaper emergencies. +$" + std::to_string(pay2)); break; }
    case 18053: case 18072: need(N_FUN, 15); need(N_SOCIAL, 10); say("Story time! Everyone sits criss-cross applesauce."); break;
    case 18054: need(N_FUN, 15); s.skills[SK_CREATIVITY] = std::min(10.f, s.skills[SK_CREATIVITY] + 0.1f); say("A masterpiece. It's going on the fridge."); break;
    case 18060: need(N_SOCIAL, 15); addMoodlet(s, "Peaceful", EM_HAPPY, 1, 720); say("A calm, uplifting service. The church ladies love your outfit."); break;
    case 18061: s.skills[SK_CHARISMA] = std::min(10.f, s.skills[SK_CHARISMA] + 0.15f); need(N_FUN, 10); say("The choir hits the high note together. Chills."); break;
    case 18062: if (pay(3)) { need(N_HUNGER, 20); say("Mrs. Henderson's lemon bars. Worth every cent."); } break;
    case 18063: case 18070: need(N_SOCIAL, 15); need(N_ENERGY, -10); addMoodlet(s, "Gave Back", EM_HAPPY, 2, 1440); lxRepAdd(CI_FAMILY, 2, "volunteering"); say("You volunteered - the whole neighborhood noticed."); break;
    case 18071: dlyActivity(LK_ART); if (pay(15)) { s.skills[SK_CREATIVITY] = std::min(10.f, s.skills[SK_CREATIVITY] + 0.3f); say("Your pottery bowl is... lopsided. You love it."); } break;
    case 18073: { need(N_FUN, 15); if (irand(4) == 0) { G.money += 25; say("B-I-N-G-O! You win $25!"); } else say("So close. The grandmas are ruthless."); break; }
    case 18074: need(N_SOCIAL, 10); s.skills[SK_CHARISMA] = std::min(10.f, s.skills[SK_CHARISMA] + 0.1f); say("You do all the voices. The little kids are obsessed."); break;
    case 18080: dlyActivity(LK_SPORTS); need(N_ENERGY, -15); need(N_HYGIENE, 15); s.skills[SK_FITNESS] = std::min(10.f, s.skills[SK_FITNESS] + 0.2f); say("20 laps! Your arms are noodles."); break;
    case 18081: need(N_FUN, 25); say(irand(2) ? "MASSIVE splash! You win the cannonball contest!" : "Belly flop. Everyone felt that."); break;
    case 18082: need(N_FUN, 20); need(N_SOCIAL, 15); say("MARCO! ...POLO! ...fish out of water!"); break;
    case 18083: if (pay(4)) { need(N_HUNGER, 10); need(N_FUN, 5); say("Blue raspberry snow cone. Blue tongue."); } break;
    case 18084: { const int p = 35; G.money += p; need(N_ENERGY, -15); say("Lifeguard shift: one whistle, zero emergencies. +$35"); break; }
    case 18090: need(N_HUNGER, 40); need(N_SOCIAL, 10); say("Free block party burgers!"); break;
    case 18091: dlyActivity(LK_COOKING); s.skills[SK_COOKING] = std::min(10.f, s.skills[SK_COOKING] + 0.2f); need(N_SOCIAL, 15); say("You run the grill like a pro. Neighbors line up."); break;
    case 18092: need(N_FUN, 35); need(N_ENERGY, -10); tweenPlay(G.mc, TW_HOP); say("BOUNCE BOUNCE BOUNCE!"); break;
    case 18095: need(N_SOCIAL, 10); say("You count heads in the bounce house. Someone lost a shoe."); break;
    case 18096: s.skills[SK_CREATIVITY] = std::min(10.f, s.skills[SK_CREATIVITY] + 0.1f); need(N_FUN, 15); say("Hopscotch, rainbows and a giant cat. Sidewalk art!"); break;
    case 18093: dlyActivity(LK_DANCING); need(N_FUN, 25); tweenPlay(G.mc, TW_WAVE); say("The Cha-Cha Slide comes on. EVERYONE dances."); break;
    case 18094: { need(N_SOCIAL, 25); int n = 0; for (int i = 0; i < (int)G.sims.size() && n < 3; i++) if (i != G.mc && !G.sims[i].household && irand(6) == 0) { changeRel(G.mc, i, 4, 0); n++; }
                  say("You meet " + std::to_string(std::max(1, n)) + " neighbors. One of them is DEFINITELY nosy."); break; }
    case 18100: s.gradePts = std::min(100.f, s.gradePts + 4); s.skills[SK_LOGIC] = std::min(10.f, s.skills[SK_LOGIC] + 0.2f); need(N_FUN, -10); say("Flashcards, highlighters, iced coffee. Finals-ready."); break;
    case 18101: s.gradePts = std::min(100.f, s.gradePts + 7); need(N_ENERGY, -45); addMoodlet(s, "All-Nighter", EM_UNCOMFORTABLE, 1, 720); say("3 AM. You've read the same page nine times."); break;
    case 18102: need(N_ENERGY, 30); say("The best 20 minutes of your life."); break;
    case 18105: need(N_SOCIAL, 20); say("Your roommate spills ALL the tea about the RA."); break;
    case 18103: dlyActivity(LK_PARTIES); need(N_FUN, 35); need(N_SOCIAL, 25); need(N_ENERGY, -20); say(irand(2) ? "Themed party: Y2K night! Your outfit wins." : "Someone fell into the pool. Again."); break;
    case 18104: if (irand(100) < 45 + (int)s.skills[SK_CHARISMA] * 5) { addMoodlet(s, "Got a Bid!", EM_CONFIDENT, 2, 2880); say("BID DAY! You're in! New sisters/brothers forever."); } else say("No bid this time. Their loss."); break;
    case 18200: if (pay(20)) { addMoodlet(s, "Fresh Trim", EM_CONFIDENT, 1, 720); say("Just the ends, she said. (She took 3 inches.)"); } break;
    case 18201: case 18202: case 18203: case 18204: { static const int P[4] = {60, 150, 120, 50}; static const char* H[4] = {"Sleek Long", "Knotless Braids", "Box Braids", "Blowout Curls"};
        if (!pay(P[id - 18201])) break; extern const char* HAIRSTYLE_NAMES[]; for (int i = 0; i < NUM_HAIRSTYLES; i++) if (!strcmp(HAIRSTYLE_NAMES[i], H[id - 18201])) { s.look.hairStyle = (uint8_t)i; break; }
        addMoodlet(s, "Hair Laid", EM_CONFIDENT, 2, 2880); tweenPlay(G.mc, TW_HOP); say(std::string("New hair: ") + H[id - 18201] + ". You are UNSTOPPABLE."); break; }
    case 18205: need(N_SOCIAL, 20); say("The salon knows EVERYTHING. Who's pregnant, who's broke, who's cheating."); break;
    case 18213: case 18214: case 18215: case 18216: { static const uint8_t NC[4] = {1, 2, 3, 6}; if (!pay(15)) break; s.look.f.pad = (uint8_t)((s.look.f.pad & 0xE3) | (NC[id - 18213] << 2)); dlyNailsDone(); addMoodlet(s, "Nails Done", EM_CONFIDENT, 1, 2880); say("Fresh polish! Look at your hands."); break; }
    case 18210: case 18211: case 18212: { static const int P[3] = {40, 35, 20}; if (!pay(P[id - 18210])) break; if (id == 18210) s.look.f.pad = (uint8_t)((s.look.f.pad & 0xE3) | (7 << 2)); if (id == 18211) s.look.f.pad = (uint8_t)((s.look.f.pad & 0xE3) | (4 << 2)); addMoodlet(s, "Nails Done", EM_CONFIDENT, 1, 2880); dlyNailsDone(); say(id == 18210 ? "Rhinestones, chrome & tiny butterflies. Iconic." : id == 18211 ? "Classic French tips. Clean girl approved." : "Fill done. Fresh again."); break; }
    case 18220: if (pay(30)) { need(N_ENERGY, 5); say("Checkup: healthy! The doctor gives you a lollipop anyway."); } break;
    case 18221: need(N_FUN, 10); say("Rows of tiny babies in tiny hats. Your heart melts."); break;
    case 18222: if (pay(150)) { dlyCure(G.mc); say("ER: fluids and meds. You're feeling way better."); } break;
    case 18223: { bool preg = false; for (auto& x : G.sims) (void)x; preg = s.pregnant; say(preg ? "Ultrasound: a healthy little heartbeat! You cry a little." : "Ultrasound: nothing to see - you're not expecting."); if (preg) guPhoto("First ultrasound"); break; }
    case 18230: dlyActivity(LK_DANCING); need(N_FUN, 25); s.skills[SK_FITNESS] = std::min(10.f, s.skills[SK_FITNESS] + 0.1f); say(irand(3) ? "Gliding under the disco ball to a Y2K bop." : "You ate it in front of everyone. Get back up!"); break;
    case 18231: { bool partner = false; for (const Rel& r : G.rels) if ((r.a == G.mc || r.b == G.mc) && r.status >= RS_DATING && r.status <= RS_MARRIED) partner = true;
        need(N_FUN, 20); say(partner ? "Couples skate! Holding hands under the pink lights." : "Couples skate... you hold the wall instead."); break; }
    case 18232: need(N_FUN, 20); say(irand(2) ? "You won the limbo! Flexible queen." : "Out in round one. Your back hurts."); break;
    case 18233: if (pay(5)) { need(N_FUN, 5); say("Pink skates with pom-poms. Yes."); } break;
    case 18234: if (pay(2)) say(irand(5) == 0 ? "YOU WON A PLUSHIE!" : "The claw dropped it. Rigged."); break;
    case 18240: if (pay(6)) { need(N_HUNGER, 30); say("Powdered sugar EVERYWHERE."); } break;
    case 18241: if (pay(8)) { need(N_HUNGER, 40); say("Best tacos on the boardwalk."); } break;
    case 18242: dlyActivity(LK_SHOPPING); if (pay(15)) { addMoodlet(s, "New Jewelry", EM_HAPPY, 1, 1440); say("A handmade charm bracelet. Cute."); } break;
    case 18243: if (pay(3)) say(irand(4) == 0 ? "RINGER! You win a giant teddy bear!" : "So close. Try again?"); break;
    case 18244: if (pay(4)) { need(N_FUN, 20); guPhoto("Ferris wheel at night"); say("The whole boardwalk lit up below you."); } break;
    case 18250: { if (!pay(50)) break; int p = -1; for (const Rel& r : G.rels) if ((r.a == G.mc || r.b == G.mc) && r.status == RS_ENGAGED) p = r.a == G.mc ? r.b : r.a;
        if (p < 0) { say("You need a fiance first! (Refund.)"); G.money += 50; break; }
        lifeMarry(G.mc, p); addMoodlet(s, "Just Married!", EM_HAPPY, 3, 2880); guPhoto("Courthouse wedding"); say("Courthouse wedding! Quick, cute, legally married."); break; }
    case 18251: { int p = -1; for (const Rel& r : G.rels) if ((r.a == G.mc || r.b == G.mc) && r.status == RS_MARRIED) p = r.a == G.mc ? r.b : r.a;
        if (p < 0) { say("Name changes here are for newlyweds."); break; }
        const std::string& pn = G.sims[p].name; const size_t sp = pn.find(' '); if (sp == std::string::npos) break;
        const std::string fn = s.name.substr(0, s.name.find(' ')); s.name = fn + pn.substr(sp); say("New name, who dis? You're now " + s.name + "."); break; }
    case 18252: if (pay(40)) say("Ticket paid. Never parking there again."); break;
    case 18253: say(irand(2) ? "Custody hearing: the judge orders shared weekends. Everyone's tense." : "Custody hearing postponed. More waiting, more lawyer bills."); G.money = G.money > 100 ? G.money - 100 : 0; addMoodlet(s, "Court Stress", EM_UNCOMFORTABLE, 1, 1440); break;
    case 18254: { const int p = 40; G.money += p; need(N_FUN, -10); say("Jury duty. 6 hours, 1 sad sandwich, $40."); break; }
    case 18255: need(N_FUN, 15); say("The defendant's mom yelled at the judge. Better than TV."); break;
    }
    return {};
}

void town2DrawRoom(int room, float t, bool night) {
    static const RGB WALL[15] = {{250, 230, 210}, {120, 170, 220}, {240, 230, 200}, {230, 220, 210}, {200, 170, 130}, {255, 230, 240}, {235, 225, 210}, {210, 230, 220}, {140, 210, 240}, {150, 200, 250}, {220, 200, 230}, {255, 220, 235}, {60, 30, 90}, {30, 30, 70}, {225, 215, 195}};
    static const char* TITLE[15] = {"FOOD COURT", "HALLWAY & LOCKERS", "CLASSROOM", "CAFETERIA", "SCHOOL GYM", "LITTLE SPROUTS DAYCARE", "MAPLE VALLEY CHURCH", "COMMUNITY CENTER", "COMMUNITY POOL", "BLOCK PARTY", "COLLEGE DORM", "GLAM SALON", "ROLLER RINK", "BOARDWALK NIGHT MARKET", "COURTHOUSE"};
    const int k = room - R_FOODCOURT;
    grad(0, 0, TOP_W, 158, C(WALL[k]), C(WALL[k].r * 4 / 5, WALL[k].g * 4 / 5, WALL[k].b * 4 / 5));
    switch (room) {
    case R_FOODCOURT: { static const RGB S[4] = {{230, 60, 60}, {255, 170, 40}, {80, 180, 120}, {200, 90, 200}}; static const char* N[4] = {"PIZZA", "PRETZEL", "BOBA", "TACOS"};
        for (int i = 0; i < 4; i++) { rect(20 + i * 95.f, 40, 80, 60, C(S[i])); rect(20 + i * 95.f, 40, 80, 14, C(255, 255, 255)); text(N[i], 60 + i * 95.f, 42, 0.38f, C(S[i]), 1); } rect(0, 140, TOP_W, 18, C(220, 200, 190)); break; }
    case R_HALLWAY: for (int i = 0; i < 14; i++) { rect(10 + i * 28.f, 50, 24, 100, C(i % 2 ? RGB{60, 110, 200} : RGB{70, 120, 210})); rect(14 + i * 28.f, 60, 16, 3, C(30, 60, 120)); rect(14 + i * 28.f, 66, 16, 3, C(30, 60, 120)); } break;
    case R_CLASSROOM: rect(60, 20, 280, 70, C(40, 80, 60)); rect(56, 16, 288, 4, C(150, 110, 70)); text("2 + 2 = ?   SPELLING TEST FRIDAY", 200, 50, 0.4f, C(240, 240, 240), 1);
        for (int i = 0; i < 6; i++) rect(30 + i * 60.f, 120, 44, 8, C(190, 150, 100)); circle(360, 40, 14, C(255, 255, 255)); break;
    case R_CAFETERIA: rect(20, 30, 360, 40, C(200, 200, 205)); text("LUNCH", 200, 40, 0.6f, C(UI_INK), 1); for (int i = 0; i < 3; i++) rect(30 + i * 125.f, 120, 100, 10, C(240, 120, 80)); break;
    case R_SCHOOLGYM: rect(0, 120, TOP_W, 38, C(210, 160, 90)); circle(200, 138, 22, C(255, 255, 255, 0)); rect(198, 120, 4, 38, C(255, 255, 255)); for (int r = 0; r < 4; r++) rect(10, 40 + r * 18.f, 120, 14, C(150, 110, 70));
        rect(300, 30, 70, 50, C(255, 255, 255)); rect(325, 72, 20, 4, C(255, 90, 40)); break;
    case R_DAYCARE: for (int i = 0; i < 12; i++) rect(i * 34.f, 100 + (i % 3) * 6, 30, 30, C(i % 3 == 0 ? RGB{255, 120, 120} : i % 3 == 1 ? RGB{120, 200, 255} : RGB{255, 220, 90})); text("ABC  123", 200, 30, 0.8f, C(255, 90, 160), 1); break;
    case R_CHURCH: for (int i = 0; i < 3; i++) { rect(80 + i * 100.f, 20, 40, 80, C(120, 160, 230)); rect(84 + i * 100.f, 24, 32, 36, C(255, 210, 120, 200)); } rect(196, 10, 8, 30, C(220, 190, 120)); rect(188, 18, 24, 6, C(220, 190, 120));
        for (int i = 0; i < 4; i++) rect(30 + i * 90.f, 128, 70, 12, C(140, 90, 60)); break;
    case R_COMMUNITY: rect(20, 30, 160, 60, C(255, 250, 230)); text("BINGO 7PM", 100, 50, 0.5f, C(200, 60, 80), 1); rect(220, 30, 160, 60, C(255, 250, 230)); text("STORY TIME 10AM", 300, 50, 0.4f, C(60, 120, 200), 1); break;
    case R_POOL: rect(0, 100, TOP_W, 58, C(60, 170, 230)); for (int i = 0; i < 8; i++) rect(i * 50.f + std::fmod(t * 20, 50.f), 110 + (i % 2) * 20, 30, 2, C(255, 255, 255, 140)); rect(0, 96, TOP_W, 6, C(240, 240, 240)); break;
    case R_BLOCKPARTY: rect(0, 120, TOP_W, 38, C(90, 90, 100)); for (int i = 0; i < 10; i++) { circle(20 + i * 40.f, 30 + std::sin(t * 2 + i) * 4, 10, C(i % 2 ? RGB{255, 90, 160} : RGB{90, 200, 255})); rect(19 + i * 40.f, 40, 1, 30, C(80, 80, 80)); }
        rect(0, 20, TOP_W, 2, C(255, 255, 255)); break;
    case R_SALON: for (int i = 0; i < 3; i++) { rect(30 + i * 70.f, 30, 50, 60, C(255, 255, 255)); rect(34 + i * 70.f, 34, 42, 52, C(200, 230, 255)); } rect(260, 60, 120, 30, C(255, 150, 200)); text("NAILS", 320, 68, 0.45f, C(255, 255, 255), 1); break;
    case R_ROLLER: rect(0, 110, TOP_W, 48, C(120, 80, 160)); for (int i = 0; i < 8; i++) circle(50 + i * 45.f, 125 + (i % 2) * 15, 3, C(i % 2 ? RGB{255, 120, 220} : RGB{120, 220, 255}));
        circle(200, 30, 16, C(220, 220, 240)); for (int i = 0; i < 6; i++) circle(200 + std::cos(t + i) * 30, 30 + std::sin(t + i) * 12, 2, C(255, 255, 255)); break;
    case R_NIGHTMARKET: for (int i = 0; i < 4; i++) { rect(20 + i * 95.f, 60, 80, 50, C(i % 2 ? RGB{230, 90, 90} : RGB{240, 200, 80})); rect(16 + i * 95.f, 54, 88, 8, C(255, 255, 255)); }
        for (int i = 0; i < 20; i++) circle(10 + i * 20.f, 30 + std::sin(i * 0.8f) * 6, 3, C(255, 230, 140)); rect(0, 140, TOP_W, 18, C(150, 110, 80)); break;
    case R_COURTHOUSE: for (int i = 0; i < 6; i++) rect(50 + i * 60.f, 30, 14, 100, C(245, 240, 230)); rect(40, 20, 330, 12, C(230, 225, 215)); text("JUSTICE", 205, 6 + 16, 0.4f, C(90, 80, 70), 1); break;
    case R_DORM: rect(20, 40, 100, 60, C(255, 230, 240)); text("STUDY!!", 70, 60, 0.45f, C(200, 60, 120), 1); rect(260, 20, 120, 80, C(80, 60, 120)); for (int i = 0; i < 6; i++) circle(270 + i * 20.f, 30, 3, C(255, 230, 120)); break;
    }
    if (night) rect(0, 0, TOP_W, TOP_H, C(10, 10, 40, 70));
    textOutline(TITLE[k], 200, 4, 0.55f, C(255, 255, 255), C(UI_INK), 1);
}
