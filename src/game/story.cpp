// Story mode: "From Intern to Icon".
// Original characters: Celestine Vane (creative director of the House of Vane and GLOSS
// magazine), Odette (her first assistant), Julian (style director and secret softie), and
// the Platinum Pink credit card, who has opinions. Eight chapters, each a list of beats.
#include "story.hpp"
#include "game.hpp"
#include "draw.hpp"
#include "sim.hpp"
#include "people.hpp"
#include "audio.hpp"
#include "oc.hpp"
#include "drama.hpp"
#include "status.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

namespace {
// ------------------------------------------------------------------ cast & places
enum Spk { SP_NARR, SP_BOSS, SP_ODETTE, SP_JULIAN, SP_YOU, SP_CARD, SP_BANK, SP_FRIEND, SP_CRITIC };
enum Bg { BG_OFFICE, BG_PENTHOUSE, BG_SALE, BG_BACKSTAGE, BG_STREET, BG_ATELIER, BG_APARTMENT, BG_LUMIERE, BG_FOUNTAIN };
enum BeatT { BT_SAY, BT_CHOICE, BT_GAME, BT_RES, BT_SHOW, BT_IF, BT_IFNOT, BT_IFDEBT, BT_IFBOSS, BT_SETTHEME, BT_ENDING, BT_END };
enum GameId { GM_COFFEE, GM_PHONES, GM_SALE, GM_STYLE1, GM_DESIGN3 };
enum ShowId { SH_VANE1, SH_SPRING, SH_LUMIERE, SH_DEBUT, SH_FINALE };
enum Flags : uint32_t { FL_SCARF = 1, FL_STAIRS = 2, FL_SKIPBDAY = 4, FL_REPLACED = 8, FL_TOLDODETTE = 16, FL_MAKEOVER = 32, FL_SKETCHES = 64, FL_HIDBAGS = 128, FL_HONEST = 256, FL_WARNED = 512 };
// option "set" codes
enum { SET_NONE = 0, SET_THEME = 100, SET_LABEL = 200, SET_ENDING = 300, SET_PAYALL = 400 };

struct Opt { const char* label = nullptr; const char* reply = nullptr; int spk = SP_NARR; int boss = 0, debt = 0, money = 0, fame = 0, fr = 0; uint32_t flag = 0; int set = 0; };
struct Beat { int type = BT_SAY, spk = SP_NARR, bg = -1; const char* text = nullptr; Opt o[3]; int nopt = 0; int arg = 0; const char* res[3] = {nullptr, nullptr, nullptr}; int rb[3] = {0, 0, 0}, rf[3] = {0, 0, 0}; };

Opt O(const char* label, const char* reply, int spk, int boss = 0, int debt = 0, int money = 0, int fame = 0, int fr = 0, uint32_t flag = 0, int set = 0) {
    Opt o; o.label = label; o.reply = reply; o.spk = spk; o.boss = boss; o.debt = debt; o.money = money; o.fame = fame; o.fr = fr; o.flag = flag; o.set = set; return o;
}
Beat SAY(int spk, int bg, const char* t) { Beat b; b.type = BT_SAY; b.spk = spk; b.bg = bg; b.text = t; return b; }
Beat CHO(int spk, int bg, const char* t, Opt a, Opt b2, Opt c = Opt()) { Beat b; b.type = BT_CHOICE; b.spk = spk; b.bg = bg; b.text = t; b.o[0] = a; b.o[1] = b2; b.o[2] = c; b.nopt = c.label ? 3 : 2; return b; }
Beat GAME(int g) { Beat b; b.type = BT_GAME; b.arg = g; return b; }
Beat SHOW(int s) { Beat b; b.type = BT_SHOW; b.arg = s; return b; }
Beat RES(int spk, const char* bad, const char* ok, const char* great, int b0 = 0, int b1 = 0, int b2 = 0, int f0 = 0, int f1 = 0, int f2 = 0) {
    Beat b; b.type = BT_RES; b.spk = spk; b.res[0] = bad; b.res[1] = ok; b.res[2] = great; b.rb[0] = b0; b.rb[1] = b1; b.rb[2] = b2; b.rf[0] = f0; b.rf[1] = f1; b.rf[2] = f2; return b;
}
Beat IF(uint32_t flag, int skip) { Beat b; b.type = BT_IF; b.arg = (int)flag; b.nopt = skip; return b; }        // skip the next N beats unless the flag is set
Beat IFNOT(uint32_t flag, int skip) { Beat b; b.type = BT_IFNOT; b.arg = (int)flag; b.nopt = skip; return b; }  // skip the next N beats if the flag is set
Beat IFDEBT(int amt, int skip) { Beat b; b.type = BT_IFDEBT; b.arg = amt; b.nopt = skip; return b; }           // skip unless debt >= amt
Beat IFBOSS(int lvl, int skip) { Beat b; b.type = BT_IFBOSS; b.arg = lvl; b.nopt = skip; return b; }           // skip unless approval >= lvl
Beat THEME(int th) { Beat b; b.type = BT_SETTHEME; b.arg = th; return b; }
Beat ENDING() { Beat b; b.type = BT_ENDING; b.bg = BG_ATELIER; return b; }
Beat END() { Beat b; b.type = BT_END; return b; }

const char* const RANKS[9] = {"Coffee Intern", "Junior Assistant", "Second Assistant", "Senior Assistant", "Assistant Stylist", "Stylist", "Emerging Designer", "Rising Star Designer", "Fashion Icon"};
const char* const CH_TITLES[8] = {"The Coffee Run", "The Book", "The Impossible Task", "Fashion Week", "Your First Look", "Lumiere City", "Your Debut", "Icon"};
const int CH_MONEY[8] = {150, 200, 300, 400, 500, 800, 2000, 5000};
const int CH_FANS[8] = {20, 40, 80, 200, 400, 900, 3000, 10000};

const char* const THEMES[6] = {"Celestial Noir", "Bubblegum Riot", "Ocean Glass", "Golden Hour", "Lavender Haze", "Cherry Cola"};
const RGB PAL[6][3] = {{{34, 30, 40}, {62, 60, 72}, {210, 210, 228}}, {{250, 120, 190}, {250, 240, 246}, {255, 60, 140}}, {{70, 180, 200}, {40, 80, 140}, {220, 250, 250}},
                       {{236, 190, 110}, {120, 76, 50}, {255, 236, 180}}, {{190, 160, 236}, {240, 232, 250}, {130, 90, 200}}, {{190, 30, 50}, {36, 24, 28}, {250, 220, 220}}};
const char* const LABELS[3] = {"{N}", "Maison Starlight", "Cherry Cola Club"};
const char* const TOPN[12] = {"Baby Tee", "Halter", "Slip Dress", "Striped Cami", "Tube Top", "Moto Jacket", "Polo", "Zip Hoodie", "Faux Fur Coat", "Babydoll Dress", "Bodycon Gown", "Crop Tee"};
const char* const BOTN[9] = {"Ruffle Mini", "Flares", "Cargos", "Wide-Leg", "Maxi Skirt", "Wrap Skirt", "Micro Shorts", "Denim Mini", "Capris"};
const char* const SHOEN[6] = {"Strappy Heels", "Platforms", "Knee Boots", "Combat Boots", "Studded Heels", "Chunky Sneakers"};
inline bool isDress(int top) { return top == 2 || top == 9 || top == 10; }
// what each theme loves (bitmasks over the menus above)
const uint16_t TH_TOPS[6] = {(1 << 2) | (1 << 5) | (1 << 8) | (1 << 10), (1 << 0) | (1 << 1) | (1 << 4) | (1 << 9) | (1 << 11), (1 << 1) | (1 << 2) | (1 << 3) | (1 << 11),
                             (1 << 3) | (1 << 6) | (1 << 9) | (1 << 10), (1 << 0) | (1 << 3) | (1 << 7) | (1 << 9), (1 << 1) | (1 << 4) | (1 << 5) | (1 << 7) | (1 << 8)};
const uint16_t TH_BOTS[6] = {(1 << 1) | (1 << 3) | (1 << 5), (1 << 0) | (1 << 6) | (1 << 7), (1 << 4) | (1 << 5) | (1 << 8), (1 << 3) | (1 << 4) | (1 << 5), (1 << 1) | (1 << 3) | (1 << 4), (1 << 1) | (1 << 2) | (1 << 6) | (1 << 7)};
const uint8_t TH_SHOES[6] = {(1 << 4) | (1 << 2), (1 << 1) | (1 << 5), (1 << 0) | (1 << 1), (1 << 0) | (1 << 2), (1 << 1) | (1 << 5), (1 << 3) | (1 << 2)};
const int DESIGN_MODELS[3] = {2, 9, 17};   // Destiny, Leila, Starla model your looks

// ------------------------------------------------------------------ the chapters
std::vector<Beat> g_ch[8];
void buildChapters() {
    if (!g_ch[0].empty()) return;
    // ---------- 1. The Coffee Run
    g_ch[0] = {
        SAY(SP_NARR, BG_OFFICE, "Monday, 6:58 AM. The House of Vane, 41st floor. Your first day as an intern at the most terrifying fashion house in Starlight Bay."),
        SAY(SP_ODETTE, BG_OFFICE, "You're the new intern? Hm. That outfit is... a choice. I'm Odette, first assistant. Celestine arrives in four minutes. Do not speak to her unless she speaks to you."),
        SAY(SP_JULIAN, BG_OFFICE, "Don't mind Odette, she hasn't slept since spring. I'm Julian, style director. Welcome to the zoo, {N}."),
        SAY(SP_ODETTE, BG_OFFICE, "She's EARLY. Everyone look busy. Look busier than that!"),
        SAY(SP_BOSS, BG_OFFICE, "Mm. Who is this."),
        SAY(SP_ODETTE, BG_OFFICE, "The new intern, Celestine. {N}."),
        SAY(SP_BOSS, BG_OFFICE, "I'd like a coffee. You know how I take it. ...You don't? How thrilling. Odette will tell you once. Once."),
        GAME(GM_COFFEE),
        RES(SP_BOSS, "It's cold. And it's wrong. Fascinating, both at once.", "Acceptable. Barely. Don't let it go to your head.", "...Mm. Correct. Don't look so surprised, it's unbecoming.", -8, 2, 8),
        CHO(SP_CARD, BG_STREET, "Lunch break. A boutique window: a cherry-red cashmere scarf, $480. Your brand-new Platinum Pink card is warm in your bag. It whispers: 'Hiiii.'",
            O("Buy it. It's an INVESTMENT.", "BEEP! Approved! See? That was easy. We're going to be such good friends.", SP_CARD, 0, 480, 0, 5, 0, FL_SCARF),
            O("Just touch it a little. Then leave.", "You leave. You think about the scarf for the rest of the afternoon. And possibly forever.", SP_YOU),
            O("Ask Julian if it's worth it", "Is it WORTH it? Darling, it's cashmere. It's always worth it. Can you AFFORD it? Completely different question.", SP_JULIAN)),
        SAY(SP_BOSS, BG_OFFICE, "Odette. Why is the intern wearing a lumpy sweater in my building."),
        CHO(SP_JULIAN, BG_BACKSTAGE, "Julian pulls you into the archive closet. Racks and racks of samples. 'Okay. We need to fix... all of this. Want a makeover?'",
            O("Yes. Do your worst.", "My BEST, thank you. Hold still. ...Oh, look at you. Celestine is going to hate how much she likes this.", SP_JULIAN, 4, 0, 0, 10, 0, FL_MAKEOVER),
            O("My style is fine, thanks.", "Brave. Wrong, but brave.", SP_JULIAN, -4)),
        SAY(SP_NARR, BG_APARTMENT, "Day one: over. Your feet hurt. Your phone has 41 messages from Odette. And somehow... you're kind of obsessed with this job."),
        END()};
    // ---------- 2. The Book
    g_ch[1] = {
        SAY(SP_ODETTE, BG_OFFICE, "Tonight you deliver The Book to Celestine's penthouse. It's the whole season's lookbook. Leave it on the table. Do not go upstairs. Do not breathe loudly."),
        SAY(SP_NARR, BG_PENTHOUSE, "Celestine's penthouse. Marble floors, a chandelier bigger than your apartment, and voices drifting down from upstairs..."),
        CHO(SP_YOU, BG_PENTHOUSE, "Leave it on the table, Odette said. But you don't see a table. Just a very grand staircase.",
            O("Leave it neatly by the door", "The next morning, Celestine says only: 'The floor. Interesting choice.' You'll be thinking about that for weeks.", SP_BOSS, -3),
            O("Tiptoe upstairs to find a table", "Celestine is on the landing, on the phone. She lowers it. 'Why are you on my stairs.' You've never left a building so fast.", SP_BOSS, -10, 0, 0, 0, 0, FL_STAIRS),
            O("Text Odette and wait", "Odette: 'THE CONSOLE BY THE ORCHIDS. ARE YOU BLIND.' ...Oh. There it is. Shaped like a swan.", SP_ODETTE, 3)),
        SAY(SP_NARR, BG_SALE, "Saturday. The Starlight Bay Designer Sample Sale. Up to 80% off. Two hundred shoppers. One of you."),
        SAY(SP_CARD, BG_SALE, "Listen. I'm not saying buy everything. I'm saying LOOK at everything. With intent. And then maybe buy everything."),
        GAME(GM_SALE),
        RES(SP_CARD, "You bought nothing! I'm so... proud? Sad? I'm feeling a lot right now.", "A few treasures! Very reasonable. Who even are we?", "WE CLEANED UP. Okay, the statement might be scary. Just don't open it. Ever.", 0, 0, 0, 0, 10, 25),
        CHO(SP_YOU, BG_APARTMENT, "Back home, the bags won't fit in your closet. And {F} is coming over in ten minutes.",
            O("Hide the bags under the bed", "{F}: '...Why is your bed six inches taller than last week?' You: 'Firm mattress.'", SP_FRIEND, 0, 0, 0, 0, 0, FL_HIDBAGS),
            O("Show {F} everything", "{F}: 'OKAY these are insane. Can I borrow the boots? Like, forever?'", SP_FRIEND, 0, 0, 0, 5, 4)),
        END()};
    // ---------- 3. The Impossible Task
    g_ch[2] = {
        SAY(SP_BOSS, BG_OFFICE, "The sketchbook of Auguste Laurent. His final collection, never shown. I want it on my desk tomorrow morning. That isn't a request. It's a forecast."),
        SAY(SP_ODETTE, BG_OFFICE, "That sketchbook doesn't exist. Nobody's seen it in twenty years. Good luck! Really. (Not really.) Oh, and you're on phones today."),
        GAME(GM_PHONES),
        RES(SP_NARR, "You dropped three calls, double-booked a fitting and called a duke 'dude'.", "You handled the phones. Mostly. Nobody cried. Mostly.", "Five calls, five perfect answers. Odette looks... threatened.", -5, 1, 6),
        IFDEBT(1, 2),
        SAY(SP_BANK, BG_APARTMENT, "Hello! Mr. Pennywhistle, Starlight Bank, about your Platinum Pink card? We've noticed some... enthusiasm. ${D} of enthusiasm."),
        CHO(SP_BANK, BG_APARTMENT, "We'd simply love to chat about a payment plan!",
            O("I'll pay it off this week, promise", "Wonderful! I'll write that down. In pen.", SP_BANK, 0, 0, 0, 0, 0, FL_HONEST),
            O("Sorry, wrong number! ...Bonjour?", "Ah. Of course. I'll call back. I always call back.", SP_BANK),
            O("Pay $200 right now", "Splendid! You're a credit to credit.", SP_BANK, 0, -200, -200)),
        CHO(SP_YOU, BG_STREET, "Twelve hours to find a sketchbook that might not exist. Who do you call?",
            O("Julian. He knows the archive.", "'Laurent's old assistant runs a flower shop on 9th. Bring peonies. Don't be weird.' ...She hands you the sketchbook. And a peony.", SP_JULIAN, 6, 0, 0, 0, 0, FL_SKETCHES),
            O("{F}. They know everybody.", "{F}: 'my cousin's roommate dated Laurent's grandson??' ...Somehow, by 3 AM, it's in your hands.", SP_FRIEND, 4, 0, 0, 0, 5, FL_SKETCHES),
            O("Give up and go to sleep", "'No sketchbook. Mm. I'll make a note of your limits, {N}.'", SP_BOSS, -12)),
        IF(FL_SKETCHES, 2),
        SAY(SP_BOSS, BG_OFFICE, "It's on my desk. And photocopied. Bound. Tabbed by season. ...Who taught you to do that."),
        SAY(SP_ODETTE, BG_OFFICE, "She has never said that many words to me. Ever. In two years."),
        END()};
    // ---------- 4. Fashion Week
    g_ch[3] = {
        SAY(SP_JULIAN, BG_BACKSTAGE, "Starlight Fashion Week. The House of Vane closes the night and Celestine wants you backstage with the run-of-show. Breathe. It's only the most important twelve minutes of the year."),
        CHO(SP_ODETTE, BG_BACKSTAGE, "Five minutes! Destiny's zipper just SNAPPED and the tailor is stuck in traffic!",
            O("Sew it yourself", "Julian: 'Since when can you SEW?' You: 'Since my grandma's living room.' Destiny hits her mark with one second to spare.", SP_JULIAN, 6, 0, 0, 10),
            O("Swap her into the backup gown", "The backup gown is better, honestly. Nobody will ever know. Except you. And now Odette.", SP_ODETTE, 3),
            O("Panic quietly in a corner", "Odette fixes it with a safety pin and a death glare. Mostly the glare.", SP_ODETTE, -6)),
        SAY(SP_NARR, BG_BACKSTAGE, "The lights drop. The music starts. Celestine takes her seat in the front row without looking at anyone."),
        SHOW(SH_VANE1),
        SAY(SP_BOSS, BG_BACKSTAGE, "Mm. ...Good."),
        SAY(SP_JULIAN, BG_BACKSTAGE, "She said GOOD. Out loud. In public. I'm going to frame this moment and hang it in my kitchen."),
        CHO(SP_FRIEND, BG_STREET, "Your phone buzzes. {F}: 'my birthday dinner is tonight!! you're still coming right??' ...Celestine expects you at the afterparty.",
            O("Go to the afterparty (it's work)", "{F}: 'it's fine. it's whatever. have fun with your fancy people.' Ouch.", SP_FRIEND, 8, 0, 0, 20, -12, FL_SKIPBDAY),
            O("Go to {F}'s birthday", "{F} screams when you walk in. The cake says 'happy bday' in purple icing. It's perfect.", SP_FRIEND, -6, 0, 0, 0, 10)),
        END()};
    // ---------- 5. Your First Look
    g_ch[4] = {
        THEME(1),
        SAY(SP_BOSS, BG_OFFICE, "Julian tells me you have 'an eye'. Prove it. One look in my Spring show. The mood is {T}. Disappoint me and you'll steam garments until you're forty."),
        SAY(SP_JULIAN, BG_ATELIER, "Pick a top, a bottom, shoes and a palette. Hint: Celestine has the memory of an elephant and the patience of a match. Make it {T}."),
        GAME(GM_STYLE1),
        SAY(SP_JULIAN, BG_BACKSTAGE, "Bold. I either love it or I need a doctor. Let's find out together."),
        SHOW(SH_SPRING),
        RES(SP_CRITIC, "GLOSS Online: 'Look four felt unfinished. Who styled it? Asking for Celestine.'", "GLOSS Online: 'A charming little surprise in look four.'", "GLOSS Online: 'Look four STOLE the show. Who is this stylist?!'", -6, 3, 10, 0, 50, 150),
        SAY(SP_ODETTE, BG_OFFICE, "Lumiere City Fashion Week is in two weeks. I've been planning this trip for a year. A whole YEAR. Nothing will ruin it. Nothing."),
        END()};
    // ---------- 6. Lumiere City
    g_ch[5] = {
        SAY(SP_BOSS, BG_OFFICE, "Odette has a cold. Odette always has a cold. You're coming to Lumiere City instead. You'll tell her."),
        CHO(SP_YOU, BG_OFFICE, "Odette has been dreaming about this trip for a year. And Celestine wants YOU.",
            O("Take the trip. This is your shot.", "'...She picked you. Of course she did. Enjoy it. Really.' Odette sneezes. It sounds like heartbreak.", SP_ODETTE, 10, 0, 0, 20, 0, FL_REPLACED),
            O("Tell Celestine to bring Odette", "'You're declining. Mm. Nobody declines.' ...She brings you anyway. AND Odette. Odette stares at you like you grew wings.", SP_BOSS, -6, 0, 0, 0, 0, FL_TOLDODETTE)),
        IFDEBT(700, 2),
        SAY(SP_BANK, BG_LUMIERE, "Mr. Pennywhistle again! Funny story: your card was just declined. In Lumiere City. At a croissant stand. ${D} is the number we're discussing."),
        CHO(SP_CARD, BG_LUMIERE, "The croissant man is waiting. The card is maxed. I'd like to say, for the record: this is not my fault. It's yours.",
            O("Sell sample sale finds to a boutique", "Noooo, not the boots! ...Fine. The boots. I'll think about them every day.", SP_CARD, 0, -600, 0, -5),
            O("Ask Celestine for an advance", "'An advance. On the salary I pay you. To work for me.' ...She signs it without looking up.", SP_BOSS, -8, -800),
            O("Pay it off with your savings", "Paid in full. I feel so... clean. Is this what being responsible feels like? Gross. Nice.", SP_CARD, 0, 0, 0, 0, 0, 0, SET_PAYALL)),
        SAY(SP_NARR, BG_LUMIERE, "Lumiere City. The fashion capital of the world. Even the pigeons look expensive."),
        SHOW(SH_LUMIERE),
        SAY(SP_JULIAN, BG_LUMIERE, "Margaux Moreau. Celestine's oldest rival. Word is the GLOSS board wants Moreau to REPLACE Celestine. Tonight."),
        CHO(SP_YOU, BG_LUMIERE, "You overhear the board in the hotel bar. Celestine is about to be blindsided.",
            O("Warn Celestine right now", "'I know, {N}. I've known for weeks.' She smiles. It's terrifying. 'But thank you. Truly.'", SP_BOSS, 12, 0, 0, 0, 0, FL_WARNED),
            O("Stay out of it", "Celestine handles it anyway. By morning, Moreau has a 'new opportunity' in another city. Celestine never mentions it.", SP_NARR, -2)),
        END()};
    // ---------- 7. Your Debut
    g_ch[6] = {
        SAY(SP_JULIAN, BG_ATELIER, "Word's out: Starlight Fashion Week has one empty slot. Celestine... recommended you. I've never seen her do that. Not once. Not even for me."),
        CHO(SP_JULIAN, BG_ATELIER, "Every designer needs a name on the door. What's your label called?",
            O("{N}", "Simple. Iconic. Like a perfume nobody can afford.", SP_JULIAN, 0, 0, 0, 0, 0, 0, SET_LABEL + 0),
            O("Maison Starlight", "Ooh. Very old money. Very new you.", SP_JULIAN, 0, 0, 0, 0, 0, 0, SET_LABEL + 1),
            O("Cherry Cola Club", "Fun! Sticky! I'd wear it to a funeral.", SP_JULIAN, 0, 0, 0, 0, 0, 0, SET_LABEL + 2)),
        CHO(SP_JULIAN, BG_ATELIER, "And the collection's mood?",
            O("Celestial Noir", "Dark, dramatic. Celestine will pretend she isn't flattered.", SP_JULIAN, 0, 0, 0, 0, 0, 0, SET_THEME + 0),
            O("Golden Hour", "Warm, soft, glowing. Very you.", SP_JULIAN, 0, 0, 0, 0, 0, 0, SET_THEME + 3),
            O("Cherry Cola", "Loud, red and a little bit dangerous. Yes.", SP_JULIAN, 0, 0, 0, 0, 0, 0, SET_THEME + 5)),
        GAME(GM_DESIGN3),
        IFNOT(FL_SKIPBDAY, 1),
        SAY(SP_FRIEND, BG_BACKSTAGE, "{F} texted you a row of hearts and 'go kill it!!!' You didn't even know they knew the date."),
        IF(FL_SKIPBDAY, 1),
        SAY(SP_FRIEND, BG_BACKSTAGE, "{F} is here. In the back row. You skipped their birthday... and they came anyway. You might cry before the show even starts."),
        SAY(SP_NARR, BG_BACKSTAGE, "Your models are dressed. Your hands are shaking. Julian squeezes your shoulder. 'Go.'"),
        SHOW(SH_DEBUT),
        RES(SP_CRITIC, "GLOSS: 'A promising start, if a little confused.' Starlight Times: 'Two stars, but watch this space.'", "GLOSS: 'A confident, delightful debut.' STATUS is buzzing.", "GLOSS: 'The debut of the decade.' Every buyer in the room is fighting over you.", -4, 4, 10, 200, 900, 2500),
        IFBOSS(0, 1),
        SAY(SP_BOSS, BG_BACKSTAGE, "I came. I saw. I won't say anything nice in public. ...It was very, very good, {N}."),
        END()};
    // ---------- 8. Icon
    g_ch[7] = {
        SAY(SP_BOSS, BG_OFFICE, "The board wants a new creative director when I step back. I want it to be you. Think carefully, {N}. This chair costs everything."),
        SAY(SP_JULIAN, BG_OFFICE, "You could have it all. Her office. Her chair. Her... permanent frown. Or you could build your own thing. Either way, I'm coming with you."),
        CHO(SP_YOU, BG_FOUNTAIN, "Outside, by the fountain, your phone won't stop buzzing. Celestine. GLOSS. The board. Everyone wants an answer.",
            O("Take the chair: the new Vane", "'Good. Now you'll understand why I never smile.' She almost smiles.", SP_BOSS, 10, 0, 0, 0, 0, 0, SET_ENDING + 1),
            O("Launch your own house", "'Your own house. ...I'd have done the same. Don't you dare tell anyone I said that.'", SP_BOSS, 0, 0, 0, 0, 0, 0, SET_ENDING + 2),
            O("Toss your phone in the fountain", "Splash. Silence. For the first time in a year you can hear yourself think. And you know exactly what you want to make.", SP_NARR, -5, 0, 0, 0, 0, 0, SET_ENDING + 3)),
        SAY(SP_NARR, BG_ATELIER, "One year later..."),
        SHOW(SH_FINALE),
        ENDING(),
        END()};
}

// ------------------------------------------------------------------ text
std::string friendName() {
    int f = dramaBff();
    if (f < 0) f = partnerOf(G.mc);
    if (f < 0) { int c = ocSim(0); if (c >= 0 && c != G.mc) f = c; }
    return f >= 0 && f < (int)G.sims.size() ? G.sims[f].name : std::string("Bambi");
}
int friendSim() {
    int f = dramaBff();
    if (f < 0) f = partnerOf(G.mc);
    if (f < 0) { int c = ocSim(0); if (c >= 0 && c != G.mc) f = c; }
    return f;
}
std::string labelName() { std::string l = LABELS[G.story.label % 3]; return l == "{N}" ? MC().name : l; }
std::string fillT(const char* t) {
    std::string o;
    if (!t) return o;
    for (const char* p = t; *p; p++) {
        if (*p == '{' && p[1] && p[2] == '}') {
            char k = p[1];
            if (k == 'N') o += MC().name; else if (k == 'F') o += friendName(); else if (k == 'T') o += THEMES[G.story.theme % 6];
            else if (k == 'L') o += labelName(); else if (k == 'D') o += std::to_string(std::max(0, (int)G.story.debt));
            p += 2; continue;
        }
        o += *p;
    }
    return o;
}
const char* spkName(int s) {
    static std::string f;
    switch (s) {
    case SP_BOSS: return "Celestine Vane"; case SP_ODETTE: return "Odette"; case SP_JULIAN: return "Julian";
    case SP_YOU: return MC().name.c_str(); case SP_CARD: return "Your Platinum Pink Card"; case SP_BANK: return "Mr. Pennywhistle (Starlight Bank)";
    case SP_FRIEND: f = friendName(); return f.c_str(); case SP_CRITIC: return "The Reviews";
    default: return "";
    }
}

// ------------------------------------------------------------------ state
enum Phase { PH_TITLE, PH_BEAT, PH_REPLY, PH_GAME, PH_SHOW, PH_DONE };
int g_phase = PH_TITLE;
int g_result = 1;          // last minigame / show grade 0..2
std::string g_reply; int g_replySpk = SP_NARR;
float g_t = 0;             // time in the current phase
float g_typed = 0;         // typewriter
// minigame state
struct MG { int order[4]; int pos = 0, correct = 0; float timer = 0; int stage = 0; int item = 0, picks = 0; int call[5]; int look = 0; int field = 0; bool dirty = true; } g_mg;
// show state
struct ShowLook { int model; int slot; const char* name; };
std::vector<ShowLook> g_show; std::string g_showTitle, g_showSub; bool g_showDesigner = false; int g_showBoss = 0;
float g_showT = 0;

const Beat* cur() { if (G.story.ch >= 8) return nullptr; auto& v = g_ch[G.story.ch]; return G.story.step < v.size() ? &v[G.story.step] : nullptr; }
void clampState() { G.story.boss = (int8_t)std::clamp((int)G.story.boss, -100, 100); if (G.story.debt < 0) G.story.debt = 0; }
void addBoss(int d) { G.story.boss = (int8_t)std::clamp((int)G.story.boss + d, -100, 100); }

// ------------------------------------------------------------------ design scoring
int lookScore(int k) {
    int th = G.story.theme % 6, top = G.story.looks[k * 3] % 12, bot = G.story.looks[k * 3 + 1] % 9, sh = G.story.looks[k * 3 + 2] % 6;
    int s = 0;
    if (TH_TOPS[th] >> top & 1) s += 3;
    if (isDress(top)) s += (TH_TOPS[th] >> top & 1) ? 2 : 0; else if (TH_BOTS[th] >> bot & 1) s += 2;
    if (TH_SHOES[th] >> sh & 1) s += 1;
    return s;
}
int designGrade(int looks) {
    int s = 0;
    for (int k = 0; k < looks; k++) s += lookScore(k);
    if (G.story.pal % 6 == G.story.theme % 6) s += 3;
    if (looks == 3) { int a = G.story.looks[0] % 12, b = G.story.looks[3] % 12, c = G.story.looks[6] % 12; if (a != b && b != c && a != c) s += 2; }
    s += skillLevel(MC(), SK_STYLE) / 2 + std::min<int>(G.story.bought, 4) / (looks == 3 ? 1 : 2) + G.story.boss / 25 + irand(3);
    G.story.score = (uint16_t)std::max(0, s);
    if (looks == 3) return s >= 24 ? 2 : s >= 15 ? 1 : 0;
    return s >= 10 ? 2 : s >= 6 ? 1 : 0;
}
void refreshDesign(int k) {
    const RGB* p = PAL[G.story.pal % 6];
    ocDesignSet(k, DESIGN_MODELS[k % 3], G.story.looks[k * 3], G.story.looks[k * 3 + 1], G.story.looks[k * 3 + 2], p[0], p[1], p[2]);
}

// ------------------------------------------------------------------ minigames
const char* const COFFEE[8] = {"Oat Latte", "Extra Hot", "No Foam", "Vanilla (sugar-free)", "Cinnamon Dust", "Triple Shot", "Almond Milk", "Over Ice"};
struct Call { const char* who; const char* ask; const char* a[3]; int right; };
const Call CALLS[8] = {
    {"Designer Lorenzo", "Is Celestine coming to my show tonight?", {"Front row, absolutely!", "She'll let you know.", "Who?"}, 1},
    {"GLOSS photo desk", "The cover shoot moved to 6 AM. Who tells Celestine?", {"I'll tell her myself.", "Not me.", "Let's not tell her."}, 0},
    {"Mr. Pennywhistle", "Just calling about a small matter of your card...", {"I'm in a meeting!", "Call my lawyer.", "Can I call you back? (no)"}, 0},
    {"Celestine's driver", "Traffic on 5th. Fifteen minutes late.", {"Take the bridge instead.", "Tell her it's my fault.", "Just honk a lot."}, 0},
    {"A very famous actress", "I NEED the red gown for the gala. Tonight!", {"It's yours. I'll courier it.", "I'll ask Celestine first.", "Sorry, it's taken."}, 1},
    {"Odette (from home, sick)", "Did you re-confirm the florist??", {"Yes, twice.", "What florist?", "I'll do it now."}, 0},
    {"The caterer", "We're out of microgreens. Is arugula okay?", {"Absolutely not.", "Sure, why not.", "What's arugula?"}, 0},
    {"Margaux Moreau's office", "Celestine's private number, please?", {"Of course! It's...", "I can take a message.", "Never call here again."}, 1}};
struct SaleItem { const char* name; int was, now; const char* card; };
const SaleItem SALE[10] = {{"Pink Satin Slip Dress", 900, 180, "It's basically FREE at this point. You'd be LOSING money not buying it."},
                           {"Studded Knee Boots", 1200, 300, "Think of the cost per wear! Wear them twice a day. To bed, even."},
                           {"Faux Fur Coat", 2000, 450, "Winter is coming. Probably. Eventually. Be ready."},
                           {"Logo Baguette Bag", 1500, 375, "This bag will appreciate in value. Like art. Like a house you can hold."},
                           {"Pearl Hair Clips (x6)", 120, 30, "Thirty dollars! That's like... four coffees for Celestine."},
                           {"Cherry Velour Tracksuit", 600, 140, "Comfort is a human right and so is velour."},
                           {"Crystal Platform Heels", 1100, 260, "You'll be taller. Taller people get promoted. Science."},
                           {"Lavender Babydoll Dress", 700, 150, "It goes with everything you own. And everything you're about to own."},
                           {"Silk Scarf Set", 400, 80, "Gifts! For other people! That's practically charity."},
                           {"Tiny Chrome Sunglasses", 350, 70, "They don't fit your face. Nobody's do. That's the point."}};

void settle();
void gameStart(int g) {
    g_mg = MG{};
    g_phase = PH_GAME; g_t = 0;
    switch (g) {
    case GM_COFFEE: { int pool[8] = {0, 1, 2, 3, 4, 5, 6, 7}; for (int i = 7; i > 0; i--) std::swap(pool[i], pool[irand(i + 1)]); for (int i = 0; i < 4; i++) g_mg.order[i] = pool[i]; g_mg.timer = 4.f; break; }
    case GM_PHONES: { int pool[8] = {0, 1, 2, 3, 4, 5, 6, 7}; for (int i = 7; i > 0; i--) std::swap(pool[i], pool[irand(i + 1)]); for (int i = 0; i < 5; i++) g_mg.call[i] = pool[i]; g_mg.timer = 7.f; break; }
    case GM_SALE: g_mg.timer = 4.5f; break;
    case GM_STYLE1: case GM_DESIGN3:
        for (int k = 0; k < 3; k++) { if (!G.story.looks[k * 3] && !G.story.looks[k * 3 + 1]) { G.story.looks[k * 3] = (uint8_t)irand(12); G.story.looks[k * 3 + 1] = (uint8_t)irand(9); G.story.looks[k * 3 + 2] = (uint8_t)irand(6); } refreshDesign(k); }
        break;
    }
}
int curGame() { const Beat* b = cur(); return b && b->type == BT_GAME ? b->arg : -1; }
void gameFinish(int grade) {
    g_result = std::clamp(grade, 0, 2);
    G.story.step++;
    g_phase = PH_BEAT; g_t = 0; g_typed = 0;
    sfxYay();
    settle();
}

// ------------------------------------------------------------------ shows
void showStart(int id) {
    g_show.clear(); g_showDesigner = false; g_showBoss = 0;
    switch (id) {
    case SH_VANE1:
        g_showTitle = "HOUSE OF VANE"; g_showSub = "Fall/Winter: Celestial Noir";
        g_show = {{2, -1, "Midnight Orchid"}, {4, -1, "Velvet Hour"}, {8, -1, "Silver Static"}, {24, -1, "Plum Eclipse"}, {28, -1, "Cold Front"}, {30, -1, "Black Swan"}};
        g_showBoss = 1; break;
    case SH_SPRING:
        g_showTitle = "HOUSE OF VANE"; g_showSub = "Spring: Bubblegum Riot";
        g_show = {{14, -1, "Sorbet Sunday"}, {16, -1, "Strawberry Milk"}, {25, -1, "Peony Bomb"}, {DESIGN_MODELS[0], 0, "Look 4 - styled by {N}"}, {27, -1, "Butter Lamp"}, {17, -1, "Pearl Tide"}};
        g_showBoss = 1; break;
    case SH_LUMIERE:
        g_showTitle = "ATELIER MOREAU"; g_showSub = "Lumiere City: Neon Baroque";
        g_show = {{1, -1, "Gilded Static"}, {33, -1, "Hot Pink Rococo"}, {22, -1, "Chandelier Girl"}, {15, -1, "Marble Swirl"}, {31, -1, "After Hours"}};
        break;
    case SH_DEBUT:
        g_showTitle = labelName(); g_showSub = std::string("Debut Collection: ") + THEMES[G.story.theme % 6];
        g_show = {{DESIGN_MODELS[0], 0, "Look 1"}, {DESIGN_MODELS[1], 1, "Look 2"}, {DESIGN_MODELS[2], 2, "Look 3"}};
        g_showDesigner = true; break;
    case SH_FINALE:
        g_showTitle = G.story.ending == 1 ? "HOUSE OF VANE by " + MC().name : labelName();
        g_showSub = "The Icon Collection";
        g_show = {{DESIGN_MODELS[0], 0, "Look 1"}, {32, -1, "Starlight Hoodie Couture"}, {DESIGN_MODELS[1], 1, "Look 2"}, {29, -1, "Golden Letter"}, {30, -1, "Midnight Fur"}, {DESIGN_MODELS[2], 2, "Look 3 (finale)"}};
        g_showDesigner = true; break;
    }
    for (int k = 0; k < 3; k++) refreshDesign(k);
    g_phase = PH_SHOW; g_showT = 0;
}
constexpr float LOOK_GAP = 3.4f, LOOK_DUR = 5.4f;
float showLength() { return g_show.size() * LOOK_GAP + LOOK_DUR - LOOK_GAP + 4.5f; }
void showFinish() {
    const Beat* b = cur();
    int id = b ? b->arg : 0;
    if (id == SH_SPRING) g_result = designGrade(1);
    else if (id == SH_DEBUT || id == SH_FINALE) g_result = designGrade(3);
    else g_result = 1;
    G.story.step++;
    g_phase = PH_BEAT; g_t = 0; g_typed = 0;
    settle();
}

// ------------------------------------------------------------------ advancing
void applyOpt(const Opt& o) {
    addBoss(o.boss);
    G.story.debt += o.debt;
    G.money += o.money;
    if (o.fame > 0) G.followers += (uint32_t)o.fame; else if (o.fame < 0) G.followers -= std::min<uint32_t>(G.followers, (uint32_t)-o.fame);
    int f = friendSim();
    if (o.fr && f >= 0) changeRel(G.mc, f, o.fr, 0);
    G.story.flags |= o.flag;
    if (o.set >= SET_PAYALL) { int pay = std::min<int>(G.money, G.story.debt); G.money -= pay; G.story.debt -= pay; }
    else if (o.set >= SET_ENDING) G.story.ending = (uint8_t)(o.set - SET_ENDING);
    else if (o.set >= SET_LABEL) G.story.label = (uint8_t)(o.set - SET_LABEL);
    else if (o.set >= SET_THEME) G.story.theme = (uint8_t)(o.set - SET_THEME);
    clampState();
}
void chapterDone() {
    int c = G.story.ch;
    G.money += CH_MONEY[c];
    G.followers += (uint32_t)CH_FANS[c];
    G.story.ch++; G.story.step = 0;
    addMoodlet(MC(), c == 7 ? "Fashion Icon!" : "Climbing the Ladder", EM_CONFIDENT, 2, 480);
    char b[96]; snprintf(b, sizeof b, "Promoted: %s  (+$%d)", RANKS[G.story.ch], CH_MONEY[c]);
    rewardPopup(c == 7 ? "STORY COMPLETE!" : "CHAPTER COMPLETE!", b);
    if (c >= 3) {
        static const char* const P[5] = {"saw {N} backstage at the House of Vane show. who IS that", "{N} styled look 4 at Vane and it's all anyone's talking about", "{N} spotted in Lumiere City with CELESTINE VANE??", "{N}'s debut collection just broke STATUS", "{N} is officially the most famous designer in Starlight Bay. we called it"};
        statusAnnounce(fillT(P[std::min(4, c - 3)]), G.mc);
    }
    g_phase = G.story.ch >= 8 ? PH_DONE : PH_TITLE; g_t = 0;
    sfxYay();
}
// run non-interactive beats until something needs the player
void settle() {
    for (int guard = 0; guard < 40; guard++) {
        const Beat* b = cur();
        if (!b) { if (G.story.ch < 8) chapterDone(); return; }
        switch (b->type) {
        case BT_IF: G.story.step += (G.story.flags & (uint32_t)b->arg) ? 1 : 1 + b->nopt; continue;
        case BT_IFNOT: G.story.step += (G.story.flags & (uint32_t)b->arg) ? 1 + b->nopt : 1; continue;
        case BT_IFDEBT: G.story.step += G.story.debt >= b->arg ? 1 : 1 + b->nopt; continue;
        case BT_IFBOSS: G.story.step += G.story.boss >= b->arg ? 1 : 1 + b->nopt; continue;
        case BT_SETTHEME: G.story.theme = (uint8_t)b->arg; G.story.step++; continue;
        case BT_END: chapterDone(); return;
        case BT_GAME: gameStart(b->arg); return;
        case BT_SHOW: showStart(b->arg); return;
        default: return;
        }
    }
}
void advance() {
    const Beat* b = cur();
    if (b && b->type == BT_RES) {
        addBoss(b->rb[g_result]);
        if (b->rf[g_result]) G.followers += (uint32_t)b->rf[g_result];
    }
    G.story.step++;
    g_typed = 0; g_t = 0; g_phase = PH_BEAT;
    settle();
}

// ------------------------------------------------------------------ layout
const Btn bNext{60, 150, 200, 40}, bExit{4, 208, 76, 28}, bPay{240, 208, 76, 28}, bSkip{220, 196, 94, 28};
Btn optB(int i) { return Btn{10, 30 + i * 46.f, 300, 40}; }
Btn gridB(int i) { return Btn{6 + (i % 2) * 156.f, 30 + (i / 2) * 40.f, 152, 36}; }
Btn rowL(int r) { return Btn{8, 30 + r * 34.f, 40, 30}; }
Btn rowR(int r) { return Btn{272, 30 + r * 34.f, 40, 30}; }
const Btn bDone{110, 200, 120, 32};
Btn lookTab(int k) { return Btn{50 + k * 74.f, 2, 70, 24}; }

// ------------------------------------------------------------------ drawing helpers
void bgDraw(int bg, float t) {
    switch (bg) {
    case BG_OFFICE: {
        grad(0, 0, TOP_W, TOP_H, C(236, 238, 244), C(206, 210, 222));
        for (int i = 0; i < 4; i++) { float x = 20 + i * 96; rect(x, 18, 84, 120, C(170, 196, 226)); for (int k = 0; k < 6; k++) rect(x + 4 + k * 13, 70 + (k * 37 % 40), 10, 68 - (k * 37 % 40), C(120, 140, 176)); outline(x, 18, 84, 120, 3, C(250, 250, 252)); }
        rect(0, 138, TOP_W, 4, C(250, 250, 252));
        rect(0, 142, TOP_W, 98, C(226, 226, 232));
        rect(0, 100, TOP_W, 26, C(30, 28, 34, 220));
        textOutline("HOUSE OF VANE", 200, 102, 0.6f, C(250, 250, 252), C(30, 28, 34), 1, 0.6f);
        break;
    }
    case BG_PENTHOUSE:
        grad(0, 0, TOP_W, TOP_H, C(250, 236, 220), C(220, 196, 170));
        rrect(140, 10, 120, 150, 60, C(60, 70, 110)); for (int i = 0; i < 20; i++) sparkle(150 + (i * 53 % 100), 20 + (i * 29 % 120), 1.5f, C(255, 250, 220));
        for (int i = 0; i < 9; i++) circle(200 + std::cos(i * 0.7f) * 30, 24 + std::sin(i * 0.7f) * 8, 3, C(255, 240, 200));
        for (int s = 0; s < 6; s++) rect(300 + s * 16, 150 - s * 14, 100, 14, C(196, 170, 140));
        rect(0, 168, TOP_W, 72, C(240, 232, 226));
        break;
    case BG_SALE:
        grad(0, 0, TOP_W, TOP_H, C(255, 210, 228), C(255, 160, 196));
        for (int r = 0; r < 3; r++) { float y = 40 + r * 50; line(10, y, 390, y, 3, C(180, 180, 190)); for (int i = 0; i < 18; i++) { RGB c = PAL[(i + r) % 6][i % 3]; rect(16 + i * 21, y + 2, 16, 34, C(c)); } }
        textOutline("SAMPLE SALE  -80%", 200, 4, 0.7f, C(255, 255, 255), C(255, 46, 140), 1, 1.0f);
        break;
    case BG_BACKSTAGE:
        grad(0, 0, TOP_W, TOP_H, C(46, 36, 52), C(24, 20, 30));
        for (int m = 0; m < 3; m++) { float x = 30 + m * 130; rect(x, 30, 100, 80, C(80, 70, 90)); for (int k = 0; k < 6; k++) { circle(x + 8 + k * 17, 26, 4, C(255, 240, 190)); circle(x + 8 + k * 17, 114, 4, C(255, 240, 190)); } }
        for (int i = 0; i < 14; i++) rect(i * 30.f, 140, 22, 60, C(PAL[i % 6][0]));
        break;
    case BG_STREET:
        grad(0, 0, TOP_W, TOP_H, C(60, 50, 110), C(240, 140, 160));
        for (int i = 0; i < 10; i++) { float h = 60 + (i * 47 % 90); rect(i * 42.f, 170 - h, 38, h, C(40, 34, 70)); for (int k = 0; k < 8; k++) if ((i + k) % 3) rect(i * 42.f + 6 + (k % 3) * 10, 180 - h + (k / 3) * 16, 5, 7, C(255, 230, 150)); }
        rect(0, 170, TOP_W, 70, C(70, 64, 90));
        break;
    case BG_ATELIER:
        grad(0, 0, TOP_W, TOP_H, C(252, 244, 236), C(236, 220, 210));
        for (int i = 0; i < 6; i++) { float x = 20 + i * 64; rect(x, 30 + (i % 2) * 10, 44, 56, C(255, 255, 255)); line(x + 8, 40 + (i % 2) * 10, x + 30, 74, 1.5f, C(120, 100, 110)); }
        for (int m = 0; m < 3; m++) { float x = 80 + m * 120; ellipse(x, 120, 14, 18, C(210, 190, 170)); rect(x - 2, 136, 4, 60, C(120, 90, 70)); }
        rect(0, 196, TOP_W, 44, C(200, 170, 150));
        break;
    case BG_APARTMENT:
        grad(0, 0, TOP_W, TOP_H, C(255, 226, 236), C(240, 200, 220));
        rect(30, 40, 110, 80, C(120, 160, 220)); outline(30, 40, 110, 80, 4, C(255, 255, 255));
        for (int i = 0; i < 7; i++) rect(260 + i * 18.f, 70 + (i % 2) * 6, 14, 30, C(PAL[i % 6][0]));
        rect(0, 170, TOP_W, 70, C(220, 180, 200));
        break;
    case BG_LUMIERE:
        grad(0, 0, TOP_W, TOP_H, C(30, 26, 70), C(150, 90, 150));
        for (int i = 0; i < 30; i++) sparkle(std::fmod(i * 97.f, 400.f), std::fmod(i * 31.f, 100.f), 1.5f, C(255, 250, 230, 200));
        tri(300, 30, 286, 170, 314, 170, C(60, 50, 90)); rect(292, 120, 16, 3, C(255, 220, 140));
        for (int i = 0; i < 8; i++) rect(i * 40.f, 150 - (i * 23 % 40), 36, 90, C(50, 42, 80));
        rect(0, 196, TOP_W, 44, C(60, 52, 84));
        break;
    case BG_FOUNTAIN:
        grad(0, 0, TOP_W, TOP_H, C(150, 200, 240), C(250, 230, 240));
        ellipse(200, 190, 160, 26, C(180, 190, 200)); ellipse(200, 186, 140, 18, C(120, 190, 230));
        rect(194, 110, 12, 76, C(200, 200, 210));
        for (int i = 0; i < 8; i++) { float a = t * 3 + i; circle(200 + std::cos(a) * 20, 110 + std::fabs(std::sin(a)) * 30, 2, C(200, 240, 255)); }
        break;
    }
}
void cardFace(float x, float y, float s, float t) {   // the Platinum Pink card, who is a character
    rrect(x, y, 90 * s, 56 * s, 8 * s, C(255, 120, 190)); rrect(x + 4 * s, y + 4 * s, 82 * s, 48 * s, 6 * s, C(255, 170, 214));
    rect(x + 10 * s, y + 14 * s, 16 * s, 12 * s, C(240, 210, 120));
    float bl = std::fmod(t, 3.f) < 0.12f ? 1.f : 6.f;
    ellipse(x + 50 * s, y + 24 * s, 5 * s, bl * s, C(UI_INK)); ellipse(x + 68 * s, y + 24 * s, 5 * s, bl * s, C(UI_INK));
    ellipse(x + 59 * s, y + 38 * s, 7 * s, 3 * s, C(200, 40, 100));
    text("PLATINUM PINK", x + 45 * s, y + 44 * s, 0.3f * s, C(255, 255, 255), 1);
}
void portrait(int spk, float t) {
    switch (spk) {
    case SP_BOSS: ocDraw(OC_CELESTINE, 6, 6, 1.f); break;
    case SP_ODETTE: ocDraw(OC_ODETTE, 6, 6, 1.f); break;
    case SP_JULIAN: ocDraw(OC_JULIAN, 298, 6, 1.f); break;
    case SP_YOU: circle(340, 90, 52, C(UI_INK)); circle(340, 90, 49, C(255, 236, 244)); drawSimPortrait(MC(), 340, 90, 48, t); break;
    case SP_FRIEND: { int f = friendSim(); if (f >= 0) { circle(340, 90, 52, C(UI_INK)); circle(340, 90, 49, C(255, 236, 244)); drawSimPortrait(G.sims[f], 340, 90, 48, t); } break; }
    case SP_CARD: cardFace(270, 40 + std::sin(t * 3) * 4, 1.3f, t); break;
    case SP_BANK: rrect(300, 40, 60, 100, 10, C(40, 40, 50)); rrect(306, 50, 48, 76, 4, C(150, 210, 250)); text("Starlight", 330, 70, 0.34f, C(UI_INK), 1); text("Bank", 330, 84, 0.34f, C(UI_INK), 1); text("calling...", 330, 104, 0.3f, C(UI_INK), 1); break;
    case SP_CRITIC: flashPanel(250, 40, 130, 80, RGB{30, 28, 34}, 6); textOutline("GLOSS", 315, 52, 0.9f, C(255, 255, 255), C(255, 46, 140), 1, 0.8f); text("REVIEWS", 315, 92, 0.4f, C(255, 236, 160), 1); break;
    default: break;
    }
}
void dialog(int spk, const std::string& s, float t) {
    flashPanel(6, 156, 388, 80, RGB{255, 255, 255}, 8, 240);
    const char* nmS = spkName(spk);
    if (nmS && *nmS) { float w = textW(nmS, 0.42f) + 16; flashPanel(14, 144, w, 20, spk == SP_BOSS ? RGB{30, 28, 34} : spk == SP_CARD ? RGB{255, 90, 170} : RGB{255, 46, 140}, 6); text(nmS, 22, 145, 0.42f, C(255, 255, 255)); }
    size_t n = std::min(s.size(), (size_t)g_typed);
    auto ls = wrapText(s.substr(0, n), 0.42f, 370);
    for (size_t k = 0; k < ls.size() && k < 5; k++) text(ls[k].c_str(), 16, 166 + k * 13.5f, 0.42f, C(spk == SP_NARR ? RGB{100, 80, 110} : UI_INK));
    if (n >= s.size()) sparkle(382, 226 + std::sin(t * 6) * 2, 4, C(255, 46, 140));
    (void)t;
}
int lastBg() {   // the most recent background in this chapter
    if (G.story.ch >= 8) return BG_ATELIER;
    auto& v = g_ch[G.story.ch];
    for (int i = std::min<int>(G.story.step, (int)v.size() - 1); i >= 0; i--) if (v[i].bg >= 0) return v[i].bg;
    return BG_OFFICE;
}

// ------------------------------------------------------------------ the runway
void drawRunway(float t) {
    grad(0, 0, TOP_W, TOP_H, C(18, 14, 26), C(40, 30, 52));
    // backdrop + sign
    rect(120, 0, 160, 96, C(30, 24, 40));
    for (int i = 0; i < 5; i++) { float a = std::sin(t * 0.8f + i) * 0.3f; tri(140 + i * 30, 0, 120 + i * 30 + a * 40, 100, 160 + i * 30 + a * 40, 100, C(255, 240, 220, 22)); }
    std::string ti = fillT(g_showTitle.c_str());
    textOutline(ti.c_str(), 200, 6, 0.6f, C(255, 255, 255), C(255, 46, 140), 1, 0.8f);
    text(g_showSub.c_str(), 200, 30, 0.38f, C(255, 220, 240), 1);
    // runway
    quad(176, 96, 224, 96, 300, 240, 100, 240, C(236, 232, 240));
    quad(184, 96, 216, 96, 270, 240, 130, 240, C(250, 248, 252));
    line(176, 96, 100, 240, 2, C(255, 120, 190)); line(224, 96, 300, 240, 2, C(255, 120, 190));
    // audience
    for (int r = 0; r < 5; r++) for (int i = 0; i < 7; i++) {
        float d = r / 4.f, y = 112 + r * 28, xl = 10 + i * 13 * (1 + d) - r * 4, xr = 390 - i * 13 * (1 + d) + r * 4;
        if (xl < 165 - r * 14) { circle(xl, y, 5 + r, C(60, 50, 70)); ellipse(xl, y + 12 + r, 7 + r, 6 + r, C(50, 42, 60)); }
        if (xr > 235 + r * 14) { circle(xr, y, 5 + r, C(60, 50, 70)); ellipse(xr, y + 12 + r, 7 + r, 6 + r, C(50, 42, 60)); }
    }
    for (int i = 0; i < 6; i++) { float ph = std::fmod(t * 1.7f + i * 0.37f, 1.f); if (ph < 0.08f) { float x = (i % 2) ? 30 + i * 22 : 370 - i * 20, y = 120 + (i * 37 % 100); sparkle(x, y, 10, C(255, 255, 255)); circle(x, y, 4, C(255, 255, 240)); } }
    // models (farther first)
    struct Dr { float u; int k; };
    std::vector<Dr> ds;
    for (size_t k = 0; k < g_show.size(); k++) {
        float lt = g_showT - k * LOOK_GAP;
        if (lt < 0 || lt > LOOK_DUR) continue;
        float u = lt < 2.4f ? lt / 2.4f : lt < 3.2f ? 1.f : 1.f - (lt - 3.2f) / 2.2f * 0.85f;
        ds.push_back({u, (int)k});
    }
    std::sort(ds.begin(), ds.end(), [](const Dr& a, const Dr& b) { return a.u < b.u; });
    for (auto& d : ds) {
        const ShowLook& L = g_show[d.k];
        float u = d.u, sc = 0.34f + u * 0.64f, foot = 104 + u * 128, x = 200 + std::sin(g_showT * 7 + d.k) * 1.6f * u;
        ellipse(x, foot, 24 * sc, 5 * sc, C(0, 0, 0, 60));
        float bob = std::fabs(std::sin(g_showT * 7 + d.k)) * 2.f * sc;
        if (L.slot >= 0) ocDesignDraw(L.slot, x - 48 * sc, foot - 188 * sc - bob, sc);
        else ocDraw(L.model, x - 48 * sc, foot - 188 * sc - bob, sc);
    }
    // finale: everyone + the bow
    float fin = g_show.size() * LOOK_GAP + LOOK_DUR - LOOK_GAP;
    if (g_showT > fin) {
        float k = std::min(1.f, (g_showT - fin) / 1.2f);
        const int nm = (int)std::min<size_t>(g_show.size(), 6);
        for (int i = 0; i < nm; i++) {
            float sc = 0.6f, foot = 236, x = (i < (nm + 1) / 2) ? 40 + i * 44.f : 360 - (nm - 1 - i) * 44.f;
            if (g_show[i].slot >= 0) ocDesignDraw(g_show[i].slot, x - 48 * sc, foot - 188 * sc + (1 - k) * 60, sc);
            else ocDraw(g_show[i].model, x - 48 * sc, foot - 188 * sc + (1 - k) * 60, sc);
        }
        if (g_showDesigner) {
            int me = -1; for (int i = 0; i < OC_COUNT; i++) if (ocSim(i) == G.mc) me = i;   // playing as a cast member? use their doll
            if (me >= 0) ocDraw(me, 200 - 48 * 0.7f, 236 - 188 * 0.7f, 0.7f); else drawSim(MC(), 200, 236, 0.85f, t, 0, PO_WAVE, false);
        }
        else if (g_showBoss) { ocDraw(OC_CELESTINE, 200 - 48 * 0.6f, 236 - 188 * 0.6f, 0.6f); }
        for (int i = 0; i < 20; i++) { float ph = std::fmod(t * 2.3f + i * 0.21f, 1.f); if (ph < 0.06f) sparkle(10 + (i * 71 % 380), 100 + (i * 43 % 120), 9, C(255, 255, 255)); }
        textOutline("STANDING OVATION", 200, 50, 0.5f, C(255, 236, 140), C(UI_INK), 1, 0.8f);
    }
    // caption
    int curK = std::clamp((int)(g_showT / LOOK_GAP), 0, (int)g_show.size() - 1);
    if (g_showT <= fin && !g_show.empty()) {
        const ShowLook& L = g_show[curK];
        std::string cap = fillT(L.name);
        char b[160]; snprintf(b, sizeof b, "Look %d/%d: %s  -  modeled by %s", curK + 1, (int)g_show.size(), cap.c_str(), ocDef(L.model).name);
        rect(0, 222, TOP_W, 18, C(0, 0, 0, 150)); text(b, 200, 223, 0.36f, C(255, 255, 255), 1);
    }
}
} // namespace

// ================================================================== API
const char* storyRankName() { return RANKS[std::min<int>(8, G.story.ch)]; }

void storyStart() {
    buildChapters();
#ifndef __3DS__
    if (getenv("STORY_CH")) { G.story.ch = (uint8_t)atoi(getenv("STORY_CH")); G.story.step = (uint8_t)(getenv("STORY_STEP") ? atoi(getenv("STORY_STEP")) : 0); }
#endif
    g_t = 0; g_typed = 0;
    if (G.story.ch >= 8) { g_phase = PH_DONE; return; }
    if (G.story.step == 0) g_phase = PH_TITLE;
    else { g_phase = PH_BEAT; settle(); if (g_phase == PH_BEAT) { const Beat* b = cur(); if (b && (b->type == BT_GAME)) gameStart(b->arg); } }
    for (int k = 0; k < 3; k++) refreshDesign(k);
}

bool storyUpdate(const Input& in) {
    g_t += in.dt;
    const touchPosition& tp = in.tp;
    bool tap = in.touch;
    if ((in.down & KEY_B) && g_phase != PH_GAME) { sfxTap(); return false; }
    if (tap && bExit.in(tp) && g_phase != PH_GAME) { sfxTap(); return false; }
    if (tap && bPay.in(tp) && G.story.debt > 0 && (g_phase == PH_BEAT || g_phase == PH_TITLE || g_phase == PH_DONE)) {
        int pay = std::min<int>(G.money, G.story.debt);
        if (pay <= 0) { toast("No cash to pay the card with!"); sfxNo(); }
        else { G.money -= pay; G.story.debt -= pay; toast("Paid $" + std::to_string(pay) + " off the Platinum Pink card."); sfxYay(); }
        return true;
    }
    switch (g_phase) {
    case PH_DONE:
        if (tap && bNext.in(tp)) { G.story = StoryState{}; g_phase = PH_TITLE; g_t = 0; toast("Story reset. From the top, intern!"); sfxTap(); }
        return true;
    case PH_TITLE:
        if ((in.down & KEY_A) || (tap && bNext.in(tp))) { g_phase = PH_BEAT; g_typed = 0; g_t = 0; settle(); sfxTap(); }
        return true;
    case PH_REPLY:
        g_typed += in.dt * 70;
        if ((in.down & KEY_A) || (tap && bNext.in(tp))) {
            if (g_typed < g_reply.size()) { g_typed = 9999; return true; }
            advance(); sfxTap();
        }
        return true;
    case PH_BEAT: {
        const Beat* b = cur();
        if (!b || !(b->type == BT_SAY || b->type == BT_CHOICE || b->type == BT_RES || b->type == BT_ENDING)) { settle(); return true; }
        g_typed += in.dt * 70;
        std::string s = b->type == BT_RES ? fillT(b->res[g_result]) : b->type == BT_ENDING ? std::string() : fillT(b->text);
        if (b->type == BT_CHOICE) {
            if (g_typed < s.size()) { if ((in.down & KEY_A) || tap) g_typed = 9999; return true; }
            for (int i = 0; i < b->nopt; i++) if (tap && optB(i).in(tp)) {
                applyOpt(b->o[i]);
                g_reply = fillT(b->o[i].reply); g_replySpk = b->o[i].spk;
                g_phase = PH_REPLY; g_typed = 0; sfxTap();
                return true;
            }
            return true;
        }
        if ((in.down & KEY_A) || (tap && bNext.in(tp))) {
            if (b->type != BT_ENDING && g_typed < s.size()) { g_typed = 9999; return true; }
            advance(); sfxTap();
        }
        return true;
    }
    case PH_SHOW:
        g_showT += in.dt;
        if (g_showT > showLength() || (tap && bSkip.in(tp)) || (in.down & KEY_START)) showFinish();
        return true;
    case PH_GAME: {
        int gm = curGame();
        if (gm < 0) { g_phase = PH_BEAT; return true; }
        g_mg.timer -= in.dt;
        switch (gm) {
        case GM_COFFEE:
            if (g_mg.stage == 0) { if (g_mg.timer <= 0) { g_mg.stage = 1; g_mg.timer = 12.f; } return true; }
            if (tap) for (int i = 0; i < 8; i++) if (gridB(i).in(tp)) { if (g_mg.order[g_mg.pos] == i) { g_mg.correct++; sfxTap(); } else sfxNo(); g_mg.pos++; break; }
            if (g_mg.pos >= 4 || g_mg.timer <= 0) gameFinish(g_mg.correct >= 4 ? 2 : g_mg.correct >= 2 ? 1 : 0);
            return true;
        case GM_PHONES: {
            const Call& c = CALLS[g_mg.call[g_mg.pos]];
            int pickI = -1;
            if (tap) for (int i = 0; i < 3; i++) if (optB(i).in(tp)) pickI = i;
            if (pickI >= 0 || g_mg.timer <= 0) {
                if (pickI == c.right) { g_mg.correct++; sfxYay(); } else sfxNo();
                g_mg.pos++; g_mg.timer = 7.f;
                if (g_mg.pos >= 5) gameFinish(g_mg.correct >= 5 ? 2 : g_mg.correct >= 3 ? 1 : 0);
            }
            return true;
        }
        case GM_SALE: {
            int choice = -1;
            if (tap && Btn{10, 120, 145, 60}.in(tp)) choice = 1;
            if (tap && Btn{165, 120, 145, 60}.in(tp)) choice = 0;
            if (in.down & KEY_A) choice = 1;
            if (in.down & KEY_Y) choice = 0;
            if (choice >= 0 || g_mg.timer <= 0) {
                if (choice == 1) { const SaleItem& it = SALE[g_mg.item]; G.story.debt += it.now; G.story.bought++; g_mg.picks++; sfxYay(); }
                g_mg.item++; g_mg.timer = 4.5f;
                if (g_mg.item >= 10) gameFinish(g_mg.picks == 0 ? 0 : g_mg.picks <= 3 ? 1 : 2);
            }
            return true;
        }
        case GM_STYLE1: case GM_DESIGN3: {
            int looks = gm == GM_STYLE1 ? 1 : 3;
            if (!tap) return true;
            if (looks == 3) for (int k = 0; k < 3; k++) if (lookTab(k).in(tp)) { g_mg.look = k; sfxTap(); return true; }
            int k = std::min(g_mg.look, looks - 1);
            uint8_t* L = &G.story.looks[k * 3];
            for (int r = 0; r < 4; r++) {
                int d = rowL(r).in(tp) ? -1 : rowR(r).in(tp) ? 1 : 0;
                if (!d) continue;
                if (r == 0) L[0] = (uint8_t)((L[0] + 12 + d) % 12);
                else if (r == 1) L[1] = (uint8_t)((L[1] + 9 + d) % 9);
                else if (r == 2) L[2] = (uint8_t)((L[2] + 6 + d) % 6);
                else G.story.pal = (uint8_t)((G.story.pal + 6 + d) % 6);
                if (r == 3) for (int j = 0; j < looks; j++) refreshDesign(j); else refreshDesign(k);
                sfxTap();
                return true;
            }
            if (bDone.in(tp)) gameFinish(1);
            return true;
        }
        }
        return true;
    }
    }
    return true;
}

void storyDrawTop(float t) {
    switch (g_phase) {
    case PH_DONE: {
        bgDraw(BG_ATELIER, t);
        flashPanel(40, 40, 320, 150, RGB{255, 255, 255}, 10, 235);
        textOutline("FROM INTERN TO ICON", 200, 50, 0.7f, C(255, 46, 140), C(UI_INK), 1, 0.9f);
        text("You did it. You're a Fashion Icon.", 200, 86, 0.48f, C(UI_INK), 1);
        static const char* E[4] = {"", "Ending: The New Vane", "Ending: Your Name in Lights", "Ending: Free Spirit"};
        text(E[G.story.ending % 4], 200, 110, 0.46f, C(150, 60, 160), 1);
        char b[64]; snprintf(b, sizeof b, "Celestine's approval: %d", (int)G.story.boss); text(b, 200, 134, 0.42f, C(UI_INK), 1);
        snprintf(b, sizeof b, "Label: %s", labelName().c_str()); text(b, 200, 154, 0.42f, C(UI_INK), 1);
        for (int k = 0; k < 3; k++) ocDesignDraw(k, 10 + k * 26, 150, 0.4f);
        return;
    }
    case PH_TITLE: {
        bgDraw(BG_STREET, t);
        rect(0, 0, TOP_W, TOP_H, C(20, 16, 26, 200));
        char b[48]; snprintf(b, sizeof b, "CHAPTER %d", G.story.ch + 1);
        textOutline(b, 200, 50, 0.6f, C(255, 236, 160), C(UI_INK), 1, 0.8f);
        textOutline(CH_TITLES[G.story.ch % 8], 200, 80, 1.0f, C(255, 255, 255), C(255, 46, 140), 1, 1.2f);
        text((std::string("You: ") + RANKS[G.story.ch % 9]).c_str(), 200, 130, 0.5f, C(255, 220, 240), 1);
        text("FROM INTERN TO ICON", 200, 200, 0.42f, C(255, 255, 255, 180), 1);
        ocDraw(OC_CELESTINE, 300, 30, 0.9f);
        return;
    }
    case PH_SHOW: drawRunway(t); return;
    case PH_GAME: {
        int gm = curGame();
        if (gm == GM_COFFEE) {
            bgDraw(BG_OFFICE, t);
            flashPanel(60, 20, 280, 130, RGB{255, 255, 255}, 10, 240);
            if (g_mg.stage == 0) {
                textOutline("Celestine's order. Memorize it!", 200, 26, 0.5f, C(255, 46, 140), C(255, 255, 255), 1);
                for (int i = 0; i < 4; i++) { char b[48]; snprintf(b, sizeof b, "%d. %s", i + 1, COFFEE[g_mg.order[i]]); text(b, 200, 54 + i * 22.f, 0.5f, C(UI_INK), 1); }
                char b[24]; snprintf(b, sizeof b, "%.1f", std::max(0.f, g_mg.timer)); text(b, 330, 26, 0.4f, C(150, 130, 150), 2);
            } else {
                textOutline("Tap the order, in order!", 200, 26, 0.5f, C(255, 46, 140), C(255, 255, 255), 1);
                char b[48]; snprintf(b, sizeof b, "Step %d of 4   Time: %.0f", std::min(4, g_mg.pos + 1), std::max(0.f, g_mg.timer)); text(b, 200, 70, 0.5f, C(UI_INK), 1);
                for (int i = 0; i < g_mg.pos; i++) sparkle(150 + i * 33, 110, 8, C(255, 200, 60));
            }
            ocDraw(OC_ODETTE, 0, 30, 0.8f);
        } else if (gm == GM_PHONES) {
            bgDraw(BG_OFFICE, t);
            const Call& c = CALLS[g_mg.call[std::min(4, g_mg.pos)]];
            flashPanel(40, 30, 320, 110, RGB{255, 255, 255}, 10, 240);
            char b[64]; snprintf(b, sizeof b, "Line %d of 5  -  %s", g_mg.pos + 1, c.who);
            textOutline(b, 200, 36, 0.48f, C(255, 46, 140), C(255, 255, 255), 1);
            auto ls = wrapText(std::string("\"") + c.ask + "\"", 0.48f, 290);
            for (size_t k = 0; k < ls.size() && k < 3; k++) text(ls[k].c_str(), 200, 66 + k * 18.f, 0.48f, C(UI_INK), 1);
            rect(60, 128, 280 * std::max(0.f, g_mg.timer) / 7.f, 6, C(255, 46, 140));
            for (int i = 0; i < 3; i++) { float ph = std::fmod(t * 4 + i, 2.f); if (ph < 1) circle(370, 60 + i * 20.f, 4, C(255, 200, 60)); }
        } else if (gm == GM_SALE) {
            bgDraw(BG_SALE, t);
            const SaleItem& it = SALE[std::min(9, g_mg.item)];
            flashPanel(30, 34, 240, 120, RGB{255, 255, 255}, 10, 245);
            text(it.name, 150, 42, 0.55f, C(UI_INK), 1);
            char b[64]; snprintf(b, sizeof b, "Was $%d", it.was); text(b, 150, 70, 0.44f, C(150, 130, 150), 1); line(110, 77, 190, 77, 1.5f, C(220, 40, 70));
            snprintf(b, sizeof b, "NOW $%d!", it.now); textOutline(b, 150, 88, 0.75f, C(255, 46, 140), C(UI_INK), 1, 0.8f);
            rect(50, 140, 200 * std::max(0.f, g_mg.timer) / 4.5f, 6, C(255, 46, 140));
            cardFace(286, 30 + std::sin(t * 4) * 3, 1.1f, t);
            auto ls = wrapText(it.card, 0.38f, 360);
            flashPanel(16, 166, 368, 50, RGB{255, 230, 244}, 8, 240);
            text("Your card:", 26, 168, 0.36f, C(255, 46, 140));
            for (size_t k = 0; k < ls.size() && k < 2; k++) text(ls[k].c_str(), 26, 182 + k * 13.f, 0.38f, C(UI_INK));
            snprintf(b, sizeof b, "Item %d/10   Card balance: $%d", g_mg.item + 1, (int)G.story.debt); text(b, 200, 222, 0.38f, C(255, 255, 255), 1);
        } else {   // design studio
            bgDraw(BG_ATELIER, t);
            int looks = gm == GM_STYLE1 ? 1 : 3, k = std::min(g_mg.look, looks - 1);
            for (int j = 0; j < looks; j++) { float sc = j == k ? 1.f : 0.6f, x = looks == 1 ? 152 : (j == 0 ? 40 : j == 1 ? 152 : 264), foot = 232; ocDesignDraw(j, x + (1 - sc) * 48, foot - 208 * sc, sc); }
            flashPanel(250, 6, 144, 40, RGB{255, 255, 255}, 8, 230);
            text("Theme:", 258, 8, 0.36f, C(150, 130, 150)); text(THEMES[G.story.theme % 6], 258, 22, 0.46f, C(255, 46, 140));
            int s = lookScore(k); char b[32]; snprintf(b, sizeof b, "Fit:"); text(b, 8, 8, 0.4f, C(UI_INK));
            for (int i = 0; i < 6; i++) heart(46 + i * 14, 16, 5, C(i < s ? RGB{255, 46, 140} : RGB{220, 200, 210}));
            if (G.story.pal % 6 == G.story.theme % 6) text("Palette matches the theme!", 8, 26, 0.34f, C(40, 150, 110));
        }
        return;
    }
    default: break;
    }
    // beats & replies
    const Beat* b = cur();
    bgDraw(lastBg(), t);
    if (g_phase == PH_REPLY) { portrait(g_replySpk, t); dialog(g_replySpk, g_reply, t); return; }
    if (!b) return;
    if (b->type == BT_ENDING) {
        static const char* const E[4] = {"", "The New Vane. You run the House of Vane now. Interns tremble when you walk past... and then you bring them coffee. Celestine sends one email a year. It just says: 'Mm. Good.'",
                                         "Your Name in Lights. {L} is the most talked-about label in fashion. Celestine sits in your front row every season. She never smiles. She always claps.",
                                         "Free Spirit. You design what you love, for the people you love. {L} sells out in four minutes flat. Your old phone is still at the bottom of that fountain."};
        std::string s = fillT(E[std::clamp<int>(G.story.ending, 1, 3)]);
        g_typed = 9999;
        textOutline("THE END", 200, 40, 1.0f, C(255, 236, 140), C(UI_INK), 1, 1.2f);
        dialog(SP_NARR, s, t);
        return;
    }
    if (b->type == BT_RES) { portrait(b->spk, t); dialog(b->spk, fillT(b->res[g_result]), t); return; }
    portrait(b->spk, t);
    dialog(b->spk, fillT(b->text), t);
}

void storyDrawBottom(float t) {
    bottomBackdrop(t);
    char b[96];
    // status bar (the design studio uses that strip for its look tabs)
    if (!(g_phase == PH_GAME && curGame() == GM_DESIGN3)) {
    flashPanel(4, 2, 312, 24, RGB{30, 28, 34}, 6, 230);
    snprintf(b, sizeof b, "%s", storyRankName()); text(b, 10, 6, 0.38f, C(255, 236, 160));
    snprintf(b, sizeof b, "Celestine %+d   Card $%d", (int)G.story.boss, (int)G.story.debt); text(b, 310, 6, 0.36f, C(255, 255, 255), 2);
    }
    switch (g_phase) {
    case PH_DONE:
        textOutline("Story complete!", 160, 60, 0.6f, C(255, 255, 255), C(UI_INK), 1);
        button(bNext, "Replay from Ch. 1", false, 0, 0.48f);
        break;
    case PH_TITLE: button(bNext, G.story.step ? "Continue" : "Begin Chapter", false, 0, 0.55f); break;
    case PH_REPLY: button(bNext, "Next >", false, 0, 0.55f); break;
    case PH_SHOW: {
        textOutline("NOW SHOWING", 160, 32, 0.5f, C(255, 255, 255), C(255, 46, 140), 1);
        int curK = std::clamp((int)(g_showT / LOOK_GAP), 0, std::max(0, (int)g_show.size() - 1));
        for (size_t k = 0; k < g_show.size(); k++) {
            std::string L = fillT(g_show[k].name);
            snprintf(b, sizeof b, "%d. %s", (int)k + 1, L.c_str());
            text(b, 30, 56 + k * 18.f, 0.42f, C((int)k == curK ? RGB{255, 236, 140} : RGB{255, 255, 255}));
        }
        static const char* const R[8] = {"The front row is gasping.", "Someone just dropped their phone.", "A photographer is crying a little.", "Celestine hasn't blinked in a minute.",
                                         "STATUS is losing its mind.", "The music just dropped. So did jaws.", "An influencer is filming upside down.", "A buyer is whispering numbers."};
        text(R[(int)(g_showT / 2.2f) % 8], 160, 174, 0.4f, C(255, 220, 240), 1);
        button(bSkip, "Skip", false, 1, 0.42f);
        return;
    }
    case PH_GAME: {
        int gm = curGame();
        if (gm == GM_COFFEE) {
            if (g_mg.stage == 0) textOutline("Memorize the order on the top screen...", 160, 100, 0.45f, C(255, 255, 255), C(UI_INK), 1);
            else for (int i = 0; i < 8; i++) button(gridB(i), COFFEE[i], false, i % 4, 0.42f);
        } else if (gm == GM_PHONES) {
            const Call& c = CALLS[g_mg.call[std::min(4, g_mg.pos)]];
            for (int i = 0; i < 3; i++) button(optB(i), c.a[i], false, i, 0.44f);
            snprintf(b, sizeof b, "Answered right: %d", g_mg.correct); text(b, 160, 176, 0.4f, C(255, 255, 255), 1);
        } else if (gm == GM_SALE) {
            button(Btn{10, 120, 145, 60}, "BUY IT (A)", false, 0, 0.55f);
            button(Btn{165, 120, 145, 60}, "Walk away (Y)", false, 1, 0.5f);
            snprintf(b, sizeof b, "Bags so far: %d", g_mg.picks); textOutline(b, 160, 60, 0.5f, C(255, 255, 255), C(UI_INK), 1);
            text("It's only 4 seconds. Decide with your heart.", 160, 90, 0.38f, C(255, 236, 244), 1);
        } else if (gm == GM_STYLE1 || gm == GM_DESIGN3) {
            int looks = gm == GM_STYLE1 ? 1 : 3, k = std::min(g_mg.look, looks - 1);
            if (looks == 3) for (int j = 0; j < 3; j++) { snprintf(b, sizeof b, "Look %d", j + 1); button(lookTab(j), b, j == k, 3, 0.4f); }
            const uint8_t* L = &G.story.looks[k * 3];
            const char* vals[4] = {TOPN[L[0] % 12], isDress(L[0] % 12) ? "(it's a dress!)" : BOTN[L[1] % 9], SHOEN[L[2] % 6], THEMES[G.story.pal % 6]};
            const char* names[4] = {"Top", "Bottom", "Shoes", "Palette"};
            for (int r = 0; r < 4; r++) {
                button(rowL(r), "<", false, 1, 0.5f); button(rowR(r), ">", false, 1, 0.5f);
                flashPanel(52, 30 + r * 34.f, 216, 30, RGB{255, 255, 255}, 6, 235);
                text(names[r], 60, 32 + r * 34.f, 0.32f, C(150, 130, 150));
                text(vals[r], 160, 42 + r * 34.f, 0.44f, C(UI_INK), 1);
            }
            text("Match the theme for a better show!", 160, 172, 0.38f, C(255, 236, 244), 1);
            button(bDone, looks == 3 ? "Send to Runway" : "Done!", false, 0, 0.46f);
        }
        return;
    }
    case PH_BEAT: {
        const Beat* bt = cur();
        if (bt && bt->type == BT_CHOICE) {
            std::string s = fillT(bt->text);
            if (g_typed >= s.size()) for (int i = 0; i < bt->nopt; i++) { std::string L = fillT(bt->o[i].label); button(optB(i), L.c_str(), false, i, 0.44f); }
            else text("...", 160, 80, 0.6f, C(255, 255, 255), 1);
        } else button(bNext, bt && bt->type == BT_ENDING ? "Finish" : "Next >", false, 0, 0.55f);
        break;
    }
    }
    button(bExit, "Exit", false, 1, 0.42f);
    if (G.story.debt > 0) button(bPay, "Pay Card", false, 2, 0.4f);
}
