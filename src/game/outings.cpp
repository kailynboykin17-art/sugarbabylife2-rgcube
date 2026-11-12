#include "outings.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "growup.hpp"
#include "tween.hpp"
#include "levels.hpp"
#include <algorithm>
#include <string>

void toast(const std::string& s);

namespace {
struct Choice { const char* label; const char* result; int rel; int fun; int money; };
struct Outing {
    const char* name; const char* flavor[3]; int cost; int hrs; int seasons; bool home;   // cost is per person; seasons bitmask (1 spring 2 summer 4 fall 8 winter)
    const char* moment; Choice ch[3];
};
#define ALL 15
const Outing O[] = {
    // ------------------------------------------------ hangouts at home
    {"Baking Day", {"Flour EVERYWHERE. The dog is white now.", "The cookies came out shaped like... something.", "Someone ate half the dough before it hit the oven."}, 3, 2, ALL, true,
        "The cake is lopsided and leaning.", {{"Frost it anyway", "More frosting = more structure. Delicious chaos!", 4, 10, 0}, {"Start over", "Round two is perfect, but everyone's tired.", 2, 4, -6}, {"Call it 'modern art'", "Everyone dies laughing.", 5, 14, 0}}},
    {"Spa Day", {"Cucumber eyes and face masks all around.", "Dad got talked into a face mask. He likes it.", "Somebody fell asleep mid-pedicure."}, 4, 2, ALL, true,
        "Your little one painted their nails AND the couch.", {{"Laugh it off", "It's a cute stain now. Memories.", 5, 8, 0}, {"Clean it together", "Teamwork! The couch survives.", 3, 2, 0}, {"Big lecture", "The vibe is ruined.", -4, -8, 0}}},
    {"Craft Night", {"Glitter got into places glitter should never be.", "A macaroni masterpiece is born.", "Somebody hot-glued their fingers together."}, 2, 2, ALL, true,
        "Two siblings want the same glitter pen.", {{"Take turns", "Fair and square. Everybody creates.", 3, 6, 0}, {"Buy another pen ($5)", "Crisis averted with cash.", 2, 8, -5}, {"Let them figure it out", "...they did not figure it out.", -3, -4, 0}}},
    {"Pajama Day", {"Nobody changes out of PJs. Cereal for every meal.", "Matching onesies. Iconic.", "A pillow fort the size of the living room."}, 0, 3, ALL, true,
        "The pillow fort is collapsing!", {{"Hold the roof!", "Everyone dives in to save it. Heroes.", 5, 12, 0}, {"Let it fall", "Pillow avalanche! Everyone's screaming (happily).", 4, 14, 0}, {"Make everyone clean up", "Fort's gone. Fun's gone.", -2, -6, 0}}},
    {"Backyard BBQ", {"Somebody's burgers are charcoal.", "The neighbors smelled it and invited themselves.", "The sprinkler turned on mid-meal."}, 6, 3, 3, true,
        "The grill flares up!", {{"Stay calm, flip it", "Grill master moment. Everyone claps.", 4, 10, 0}, {"Order pizza ($30)", "Pizza saves the day.", 2, 8, -30}, {"Scream and run", "Legendary family story forever.", 3, 12, 0}}},
    {"Karaoke Night", {"Someone did a full power ballad with a hairbrush mic.", "A duet nobody asked for... and it SLAPPED.", "The toddler sang the alphabet song 9 times."}, 1, 2, ALL, true,
        "Your teen refuses to sing.", {{"Sing something embarrassing", "They cave just to stop you. Duet!", 5, 10, 0}, {"Let them DJ", "They pick the music - and secretly love it.", 4, 8, 0}, {"Force them", "Eye roll heard around the world.", -4, -4, 0}}},
    {"Puzzle Night", {"1,000 pieces. 3 missing. One is in the dog.", "The edges are done! Progress!", "Grandma would've finished this in an hour."}, 2, 2, ALL, true,
        "The last piece is missing.", {{"Search the whole house", "Found it in a sock. VICTORY.", 4, 10, 0}, {"Draw the missing piece", "The puzzle is canon now.", 3, 8, 0}, {"Blame someone", "Accusations fly.", -5, -6, 0}}},
    {"Family Photo Shoot", {"Matching denim outfits. Very 2004.", "The baby wouldn't look at the camera.", "We got ONE good shot out of 200."}, 3, 2, ALL, true,
        "Someone blinked in every photo.", {{"Keep the funny ones", "The blooper reel is the best part.", 5, 10, 0}, {"Retake all of them", "Perfect photo, cranky family.", 1, -2, 0}, {"Photoshop it", "Nobody will ever know.", 2, 4, 0}}},
    {"Garden Day", {"The tomatoes are finally red!", "Someone found a worm and named it Gerald.", "Mud fight. Everyone lost."}, 2, 2, 7, true,
        "The kids want to plant candy.", {{"Let them try", "It won't grow, but the joy did.", 5, 10, 0}, {"Teach real gardening", "Little green thumbs!", 3, 4, 0}, {"Say no", "Pouting in the flower bed.", -2, -4, 0}}},
    {"Family Car Wash", {"Everyone got wetter than the car.", "The hose is undefeated.", "The car is... cleaner? Kind of?"}, 1, 1, 3, true,
        "Someone sprays you on purpose.", {{"WATER WAR", "Total chaos. Best day ever.", 6, 14, 0}, {"Stay focused", "The car is spotless. Sort of fun.", 2, 4, 0}, {"Take the hose away", "Party pooper alert.", -3, -6, 0}}},
    {"Indoor Camping", {"A tent in the living room and flashlight stories.", "S'mores in the microwave. They exploded.", "Someone 'saw a bear'. It was the coat rack."}, 2, 3, 12, true,
        "Ghost stories are getting too scary.", {{"Switch to funny stories", "Giggles until midnight.", 4, 8, 0}, {"Keep going", "Everyone sleeps in your bed tonight.", 2, 10, 0}, {"Lights on, bedtime", "Fine, but boring.", 0, -2, 0}}},
    {"Cooking Contest", {"Chopped: Family Edition. Mystery basket = leftovers.", "Dad made something called 'surprise loaf'.", "Ketchup was used as a sauce. And a garnish."}, 5, 2, ALL, true,
        "You have to pick a winner.", {{"Pick the youngest", "Diplomatic. Everyone gets it.", 3, 6, 0}, {"Pick the real best", "Honest - and someone sulks.", 1, 4, 0}, {"Everyone wins!", "Participation trophies for all.", 4, 6, 0}}},
    {"Dance Party", {"Lights off, disco ball on.", "The toddler has MOVES.", "Somebody did the worm and pulled something."}, 0, 1, ALL, true,
        "Dance battle! Your kid challenges you.", {{"Go all out", "Iconic. The family will talk about this forever.", 5, 14, 0}, {"Let them win", "They're SO proud.", 5, 10, 0}, {"Sit it out", "Lame.", -2, -4, 0}}},
    {"Holiday Decorating", {"Tangled lights for 45 minutes.", "The tree is leaning. It's fine.", "Someone ate the popcorn garland."}, 4, 2, 12, true,
        "One ornament breaks - it was Grandma's.", {{"Glue it back together", "It's even more special now.", 4, 6, 0}, {"Tell everyone the story behind it", "Tears and hugs.", 6, 6, 0}, {"Hide it", "...for now.", -1, 0, 0}}},
    // ------------------------------------------------ outings around town
    {"Zoo Trip", {"The monkeys threw something. We don't talk about it.", "A giraffe licked the stroller.", "Everyone wanted the $15 plush."}, 18, 4, 7, false,
        "Your kid wants the giant stuffed animal ($25).", {{"Buy it", "Best day of their life.", 4, 10, -25}, {"Win one at the game booth", "Took 6 tries. Worth it.", 5, 12, -12}, {"Not today", "Meltdown at the gift shop.", -3, -6, 0}}},
    {"Beach Day", {"Sandcastle kingdom complete.", "Someone got a sunburn shaped like their sunglasses.", "A seagull stole a whole sandwich."}, 6, 4, 2, false,
        "The waves are big today.", {{"Boogie board together", "Everyone wipes out. Everyone laughs.", 5, 14, 0}, {"Stay on the sand", "Safe, sunny and chill.", 2, 6, 0}, {"Bury Dad in the sand", "A classic for a reason.", 5, 12, 0}}},
    {"Bowling", {"Bumpers up. No shame.", "Someone bowled backwards through their legs.", "A STRIKE! The whole alley cheered."}, 8, 2, ALL, false,
        "Your kid is losing badly.", {{"Secretly bowl badly", "They win! Their face!!", 5, 8, 0}, {"Teach them a trick", "They get a spare - pure joy.", 4, 8, 0}, {"Play to win", "You won. Nobody's talking to you.", -3, 2, 0}}},
    {"Mini Golf", {"Hole 7 has a windmill and it is EVIL.", "Hole-in-one on the dinosaur!", "Someone 'accidentally' kicked their ball in."}, 9, 2, 7, false,
        "Someone's cheating.", {{"Call it out with love", "Busted. Everyone laughs.", 3, 6, 0}, {"Let it slide", "Peace kept. Integrity questionable.", 2, 6, 0}, {"Cheat back", "A chaotic cheating war.", 4, 10, 0}}},
    {"Sparkle Land Day", {"Three roller coasters and a funnel cake.", "Someone threw up on the teacups.", "Character photo with a mascot!"}, 30, 5, 3, false,
        "Your kid is too short for the big ride.", {{"Ride the kiddie coaster together", "They feel SO grown up.", 5, 10, 0}, {"Win them a prize instead", "Huge stuffed unicorn acquired.", 4, 8, -15}, {"Leave them with a sibling", "Sibling is NOT happy.", -3, 6, 0}}},
    {"Aquarium", {"The jellyfish room was magical.", "A kid pressed their face on the shark tank.", "Penguin feeding time!"}, 15, 3, ALL, false,
        "Touch tank! Your little one is scared.", {{"Touch it first", "Brave together. They loved it.", 5, 8, 0}, {"Just watch", "They point at everything instead.", 2, 4, 0}, {"Push them to try", "Tears at the starfish.", -3, -4, 0}}},
    {"Movie Theater", {"Way too much popcorn.", "Someone cried at the cartoon.", "The little one asked questions the WHOLE movie."}, 12, 3, ALL, false,
        "Nobody can agree on a movie.", {{"Pick the kids' movie", "Surprisingly great.", 4, 8, 0}, {"Split up and meet after", "Everyone's happy-ish.", 1, 8, 0}, {"Rock paper scissors", "Fair! Chaotic! Fun!", 3, 8, 0}}},
    {"Sunday Brunch", {"Pancakes taller than the toddler.", "Mimosas for the grown-ups, juice for the kids.", "The waiter sang happy birthday to the wrong table."}, 14, 2, ALL, false,
        "Your teen is on their phone the whole time.", {{"Phones in the basket", "Real conversation! Who knew!", 4, 4, 0}, {"Ask what they're watching", "You learn a new dance. Bonding!", 5, 6, 0}, {"Let it go", "Peaceful but distant.", 0, 2, 0}}},
    {"Family Shopping Trip", {"Matching outfits for the holiday card.", "Someone hid inside the clothing rack.", "The teen wants EVERYTHING."}, 20, 3, ALL, false,
        "Your teen wants a $60 jacket.", {{"Buy it", "You're the coolest parent alive.", 5, 6, -60}, {"Split the cost", "Responsibility AND style.", 4, 6, -30}, {"Say no", "Silent car ride home.", -4, -4, 0}}},
    {"Camping Trip", {"Tent set up after only 2 arguments.", "Real s'mores by the fire.", "Something rustled. Everyone screamed."}, 10, 6, 7, false,
        "It starts raining.", {{"Sing in the tent", "The coziest night ever.", 6, 10, 0}, {"Pack up and go home", "Pizza and dry clothes.", 1, 2, -20}, {"Dance in the rain", "Muddy. Freezing. Perfect.", 5, 14, 0}}},
    {"Ice Skating", {"Everyone held the wall the whole time.", "Someone did a spin! Then fell.", "Hot cocoa after. Elite."}, 10, 2, 8, false,
        "Your kid keeps falling.", {{"Hold their hands", "They glide! Movie moment.", 6, 8, 0}, {"Get them a skate helper", "Little penguin walker. So cute.", 3, 6, -5}, {"Skate off with the teen", "Little one feels left behind.", -3, 4, 0}}},
    {"Farmers Market", {"Free samples are the real attraction.", "Someone bought a whole pumpkin pie.", "Live music and a puppy petting zone."}, 8, 2, 7, false,
        "Your kid wants honey sticks AND kettle corn.", {{"Get both", "Sugar rush in the car.", 3, 8, -8}, {"Pick one", "Decisions! They chose kettle corn.", 2, 4, -4}, {"Bring fruit instead", "Groans... then they eat it.", 1, 0, -2}}},
    {"Museum Day", {"Dinosaur bones! The kids lost it.", "Someone touched the art. Alarms.", "The gift shop had a crystal geode."}, 12, 3, ALL, false,
        "Your kid asks a big question about space.", {{"Explain it", "Little scientist unlocked.", 4, 4, 0}, {"Find the answer together", "Learning date! Super cute.", 5, 6, 0}, {"'Ask your teacher'", "Missed moment.", -1, 0, 0}}},
    {"Pumpkin Patch", {"Hayride and apple cider.", "The corn maze took 40 minutes.", "Someone picked a pumpkin bigger than the toddler."}, 8, 3, 4, false,
        "The corn maze! Everyone wants to lead.", {{"Let the kids lead", "Lost for ages. Hilarious.", 5, 10, 0}, {"Use the map", "Out in 10 minutes. Efficient.", 1, 4, 0}, {"Race in teams", "Competitive chaos!", 4, 12, 0}}},
    {"Holiday Lights Drive", {"Cocoa in the car and the whole town lit up.", "One house had a dancing light show.", "Someone fell asleep before the best house."}, 3, 2, 8, false,
        "Pick the best house!", {{"The tacky one", "Tacky is a lifestyle.", 4, 8, 0}, {"The fancy one", "Elegance wins.", 3, 6, 0}, {"Ours next year!", "A family project is born.", 5, 8, 0}}},
    {"Water Park", {"The lazy river is undefeated.", "The big slide was SCARY.", "Wedgie count: 4."}, 20, 4, 2, false,
        "The big slide - who's brave?", {{"Go down together", "Screaming the whole way. Iconic.", 5, 14, 0}, {"Cheer from the bottom", "Proud parent moment.", 3, 6, 0}, {"Lazy river only", "Relaxing. Kids are bored.", 0, 4, 0}}},
    {"Arcade Night", {"Tickets for days. Prize: one eraser.", "Air hockey got intense.", "Someone held the claw machine hostage."}, 10, 2, ALL, false,
        "The kids want to pool tickets for the big prize.", {{"Pool them", "Teamwork! Giant teddy bear!", 5, 8, 0}, {"Everyone gets their own prize", "Many tiny erasers.", 2, 6, 0}, {"Buy more tokens ($15)", "Mega prize achieved.", 3, 10, -15}}},
    {"Picnic in the Park", {"Sandwiches, ants, and a frisbee.", "The ducks were VERY interested in the bread.", "Cloud-shaped animals contest."}, 4, 3, 7, false,
        "A duck steals the sandwich.", {{"Chase the duck", "You did not win. Everyone else did.", 5, 12, 0}, {"Share the rest", "Peaceful picnic. Sweet.", 3, 6, 0}, {"Go get tacos", "Plan B is delicious.", 2, 8, -15}}},
    {"Roller Rink", {"Couples skate - you skated with your kid.", "The disco ball was ON.", "Someone fell and took 3 people down."}, 9, 2, ALL, false,
        "The limbo contest!", {{"Enter it", "You made it to round 4!", 5, 12, 0}, {"Coach the kids", "Your kid WON.", 5, 8, 0}, {"Snack bar break", "Nachos > limbo.", 1, 6, -6}}},
    {"Fishing Trip", {"Three hours. One tiny fish. Huge celebration.", "Someone caught a boot.", "Grandpa's lucky hat came along."}, 5, 4, 7, false,
        "Your kid catches a fish and wants to keep it.", {{"Name it, then release it", "Goodbye, Bubbles. A beautiful lesson.", 5, 6, 0}, {"Cook it for dinner", "Fresh! Kid feels proud.", 3, 6, 0}, {"Keep it in a bucket", "...the bucket smells now.", 1, 4, 0}}},
    {"Church & Lunch", {"Everybody dressed up, little ones squirmed.", "The choir was beautiful.", "Lunch after at the family's favorite spot."}, 10, 3, ALL, false,
        "The toddler is getting loud during service.", {{"Step out with them", "Calmed down. Quiet bonding.", 3, 2, 0}, {"Snacks and crayons", "Crisis averted.", 2, 2, 0}, {"Pretend it's not your kid", "The whole row knows.", -1, 2, 0}}},
    {"Library Story Time", {"The kids picked 14 books each.", "Puppet show!", "Someone shushed YOU."}, 0, 2, ALL, false,
        "Your kid wants to read the story to everyone.", {{"Let them!", "They read it SO proudly.", 5, 6, 0}, {"Read it together", "Sweet little team.", 4, 4, 0}, {"Maybe next time", "Disappointed little face.", -2, -2, 0}}},
};
const int NO = (int)(sizeof O / sizeof O[0]);
int pending = -1;

std::vector<int> crew() { std::vector<int> v; for (int i = 0; i < (int)G.sims.size(); i++) if (G.sims[i].household) v.push_back(i); return v; }
bool any(int lo, int hi) { for (int i : crew()) if (G.sims[i].stage >= lo && G.sims[i].stage <= hi) return true; return false; }
const char* ageLine() {   // how the different ages react
    if (any(ST_TEEN, ST_TEEN) && irand(2)) { static const char* T[3] = {"Your teen said it was 'so cringe' - then took 40 selfies.", "Your teen pretended not to know you. For 10 minutes.", "Your teen actually smiled. Screenshot it."}; return T[irand(3)]; }
    if (any(ST_TODDLER, ST_TODDLER) && irand(2)) { static const char* T[3] = {"The toddler napped through the best part.", "The toddler had a meltdown, then fell asleep on you.", "The toddler said a new word: 'MORE!'"}; return T[irand(3)]; }
    if (any(ST_CHILD, ST_PRETEEN)) { static const char* T[3] = {"The kids said it was 'the best day EVER'.", "The kids asked 'can we come back tomorrow?' 50 times.", "The kids fell asleep in the car home."}; return T[irand(3)]; }
    if (any(ST_ELDER, ST_ELDER)) return "Grandma had more energy than everyone.";
    return "Just the grown-ups - honestly kind of nice.";
}
}

LxMenu outMenu() {
    LxMenu m; const int s = seasonOf(G.day); const int n = (int)crew().size();
    m.push_back({std::string("Family Outings - ") + SEASON_NAMES[s] + ", " + std::to_string(n) + " in the family", 26999});
    m.push_back({"-- Hangouts at home --", 26999});
    for (int i = 0; i < NO; i++) if (O[i].home && (O[i].seasons >> s & 1)) m.push_back({std::string(O[i].name) + (O[i].cost ? " ($" + std::to_string(O[i].cost * n) + ")" : " (free)"), 26100 + i});
    m.push_back({"-- Outings around town --", 26999});
    for (int i = 0; i < NO; i++) if (!O[i].home && (O[i].seasons >> s & 1)) m.push_back({std::string(O[i].name) + " ($" + std::to_string(O[i].cost * n) + ", " + std::to_string(O[i].hrs) + "h)", 26100 + i});
    return m;
}

LxMenu outOption(int id) {
    LxMenu m;
    if (id == 26000) return outMenu();
    if (id >= 26100 && id < 26100 + NO) {
        const Outing& o = O[id - 26100]; const std::vector<int> c = crew();
        if (c.size() < 2) { toast("Outings are a family thing - you need someone else in your household!"); return {}; }
        if (!o.home && !lvlAllows(LF_COMMUNITY)) { toast("Outings around town unlock at Level 25 (home hangouts are open now)."); return {}; }
        const int cost = o.cost * (int)c.size();
        if (G.money < cost) { toast(std::string(o.name) + " costs $" + std::to_string(cost) + " for the whole family."); return {}; }
        if (G.minute + o.hrs * 60 > 23 * 60) { toast("It's too late for that today - try tomorrow!"); return {}; }
        if (MC().needs[N_ENERGY] < 15) { toast("You're too tired to herd everyone out the door."); return {}; }
        G.money -= cost; G.minute += o.hrs * 60;
        for (int i : c) { Sim& s = G.sims[i];
            s.needs[N_FUN] = std::min(100.f, s.needs[N_FUN] + 30); s.needs[N_SOCIAL] = std::min(100.f, s.needs[N_SOCIAL] + 20);
            s.needs[N_ENERGY] = std::max(0.f, s.needs[N_ENERGY] - o.hrs * 4.f); s.needs[N_HUNGER] = std::max(0.f, s.needs[N_HUNGER] - o.hrs * 5.f);
            addMoodlet(s, "Family Fun Day", EM_HAPPY, 2, 480); tweenPlay(i, TW_CHEER);
            for (int j : c) if (j > i) changeRel(i, j, 4, 0); }
        guPhoto(std::string(o.name) + " with the family");
        toast(std::string(o.name) + "! " + o.flavor[irand(3)]);
        toast(ageLine());
        pending = id - 26100; lvlEvent(LEV_OUTING); lvlPrestige(1, "family time");
        m.push_back({std::string("Moment: ") + o.moment, 26999});
        for (int k = 0; k < 3; k++) m.push_back({o.ch[k].label, 26500 + k});
        return m;
    }
    if (id >= 26500 && id < 26503) {
        if (pending < 0) return {};
        const Choice& ch = O[pending].ch[id - 26500]; pending = -1;
        const std::vector<int> c = crew();
        for (int i : c) { if (i != G.mc) changeRel(G.mc, i, ch.rel, 0); G.sims[i].needs[N_FUN] = std::max(0.f, std::min(100.f, G.sims[i].needs[N_FUN] + ch.fun)); }
        if (ch.money < 0 && G.money < -ch.money) { toast(std::string(ch.result) + " (you were a little short - someone chipped in)"); return {}; }
        G.money += ch.money;
        if (ch.rel >= 5) addMoodlet(MC(), "Core Memory", EM_HAPPY, 2, 720);
        else if (ch.rel < 0) addMoodlet(MC(), "Parenting Is Hard", EM_TENSE, 1, 240);
        toast(ch.result);
        return {};
    }
    return {};
}
