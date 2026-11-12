// Lively town dialogue: ambient lines by place, age, trait, weather & time, little
// two-person conversations between townies, and variety for social interactions.
#include "chatter.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "goals.hpp"
#include "crime.hpp"
#include "life.hpp"
#include <cstring>
#include <cmath>
#include <string>

static const char* PLACE[R_COUNT][6] = {
    {"Who left the TV on?", "This couch is SO comfy.", "We need more throw pillows.", "Snack break?", "I love what you did with the place!", "Is it movie night yet?"},
    {"Something smells AMAZING.", "Who ate my leftovers?!", "I'm a chef now. Sort of.", "Is there more coffee?", "Dibs on the last cookie!", "We're out of milk. Again."},
    {"Five more minutes...", "My closet is a disaster.", "Is that my sweater?", "Nap time is sacred.", "Okay, outfit check!", "Where's my charger?"},
    {"So many shoes, so little time.", "Does this match?", "I have NOTHING to wear.", "Ooh, is this new?", "Mirror, mirror...", "Help me pick earrings!"},
    {"Shhh, the baby's sleeping!", "Who's a cutie? YOU are!", "Diaper duty again...", "Look at those tiny socks!", "Lullaby time.", "Aww, a yawn!"},
    {"Turn it UP!", "Rematch! Right now!", "This is the best hangout spot.", "Who's got snacks?", "Game night forever.", "Down here is a vibe."},
    {"Fresh air!", "The garden looks great.", "Who wants to grill?", "Pool day?!", "Look, a butterfly!", "I could stay out here all day."},
    {"Surf's up!", "Who brought the sunscreen?", "Race you to the water!", "My sandcastle is ICONIC.", "Sand. Everywhere. Again.", "Best beach day ever!"},
    {"Did you do the homework?", "Pop quiz?! Nooo.", "Lunch is the best class.", "Hall pass, anyone?", "I'm late, I'm late!", "Who's going to the dance?"},
    {"This song is EVERYTHING.", "Dance floor, now!", "Mocktails on me!", "The DJ is on fire tonight.", "Is that who I think it is?", "I'm not leaving till they close."},
    {"The pasta here is unreal.", "Table for two?", "Save room for dessert!", "Ooh, fancy.", "I'll have what they're having.", "Check, please!"},
    {"That bag is a whole MOOD.", "Does this come in pink?", "I'm just looking... I swear.", "Sale rack, here I come.", "Fitting room selfie!", "My wallet is crying."},
    {"Finals are coming...", "Library date?", "Professor said it's on the test.", "I need SO much coffee.", "Study group at seven!", "Graduation can't come soon enough."},
    {"Extra foam, please!", "This latte is art.", "Wi-Fi password?", "One more muffin...", "Best spot in town.", "Iced, always."},
    {"Cucumber water? Fancy.", "I'm SO relaxed right now.", "Face mask o'clock.", "Treat yourself!", "My nails look perfect.", "Ahhh, bliss."},
    {"The food court calls to me.", "Window shopping is cardio.", "Arcade after this?", "That store is SO overpriced.", "Smoothie run!", "I lost my friends again."},
    {"The ducks are judging me.", "Perfect day for a picnic.", "Did you see that dog?!", "Jog with me!", "Swings are for everyone!", "Fresh air hits different."},
    {"No spoilers!", "Popcorn first.", "Did you cry? I cried.", "Front row or back row?", "Phones off, people!", "That twist though!"},
    {"Don't let me fall!", "Hot cocoa break?", "I'm basically a pro.", "Look, a spin!", "My toes are frozen.", "Twirl contest!"},
    {"This lip gloss is LIFE.", "Do I need contour? Don't answer.", "Swatching everything. Everything.", "The sample tester is free, right?", "My nails are so cute today.", "Is this shade too much? Never."},
    {"High score or bust!", "Who keeps beating my record?!", "One more quarter...", "I'm SO good at this game.", "The claw machine is rigged.", "Tickets! So many tickets!"},
    {"That croissant is calling my name.", "Fresh out of the oven!", "I'll take one of everything.", "Sprinkles make it healthier.", "This smells like heaven.", "Is the cake for me? Please?"},
    {"Push me higher!", "Tag, you're it!", "The slide is so fast!", "Monkey bars champion!", "Sandbox castle time.", "My turn on the swings!"},
    {"Pancakes for dinner? Yes.", "Extra syrup, please.", "This milkshake is huge.", "Best fries in Starlight Bay.", "Refill on that coffee?", "Breakfast all day is a lifestyle."},
    {"I can't use chopsticks. Help.", "One more roll. Okay two.", "This wasabi is ATTACKING me.", "Spicy tuna forever.", "So fresh!", "The conveyor belt is mesmerizing."},
    {"This street has the best vibes.", "Did you see that car?!", "Street style is everything.", "Let's grab a bubble tea.", "Look at that mural!", "Window shopping is cardio."},
    {"Nice place you've got.", "Is that a new couch?", "Your neighbor waved at me.", "Love the garden!", "Got any snacks?", "Cute curtains!"},
    {"I hate waiting rooms.", "Is it my turn yet?", "The nurse is SO nice.", "Flu shot? I'm brave. Mostly.", "Hand sanitizer, anyone?", "Apple a day, they said."},
    {"Look at that puppy!", "I want ALL the kittens.", "Can we adopt one? Please?", "That bunny is judging me.", "Fish are so calming.", "Who's a good boy?!"},
    {"Shhh, it's a library.", "I'm reading three books at once.", "Found the romance section.", "This chair is perfect for naps.", "Overdue fines? Never heard of them.", "Study group at 4?"},
    {"Leg day is the worst day.", "One more rep!", "Who's hogging the treadmill?", "My smoothie is my reward.", "Gym selfie, obviously.", "I'm sore in places I didn't know existed."},
    {"This lobby is SO fancy.", "Room service? Yes, please.", "The pool here is gorgeous.", "I could live in this hotel.", "Do you think celebrities stay here?", "The bathrobes are so fluffy."},
    {"Weddings make me cry.", "Did you see the dress?!", "Save me a seat!", "So many flowers!", "Who caught the bouquet?", "I'm not crying, you're crying."},
    {"Ooh, strawberries are on sale!", "Who needs a cart this big?", "I came for milk. I'm leaving with a whole haul.", "This lip gloss is only $8?!", "Self-checkout hates me.", "Free samples aisle five!"},
    {"This bed is SO comfy.", "Who left their socks here?", "Guest room vibes."}, {"Who used all the hot water?!", "Knock first!!", "Ugh, the mirror's all foggy."},
    {"One more round!", "Lag! It was LAG!", "GG, easy."}, {"Posters on every wall.", "Do NOT tell Mom about this mess.", "Door's closed for a reason."},
    {"Two-for-one on chips!", "Can I get a lotto ticket?", "Is the slushie machine working?", "Just grabbing gas station snacks."},
    {"Nice serve!", "Love-fifteen!", "That was OUT!", "Anyone up for doubles?"},
    {"Five dollars for vintage?!", "This was someone's prom dress.", "I found the CUTEST jacket.", "Thrifting is a sport."},
    {"Did that door just close by itself?!", "I am NOT going in the basement.", "Okay who touched my shoulder.", "This place gives me chills.", "Ghost hunting era!", "I heard a whisper..."},
    {"This party is SO loud!", "Who even lives here?", "Frat boys are a whole species.", "The playlist is actually fire.", "Is that a couch on the lawn?", "Rush week chaos!"},
    {"This water is SO clear!", "I'm never leaving.", "Sunscreen check!", "Is that a sea turtle?!", "Island time, baby.", "Best. Trip. Ever."},
    {"Mango smoothies everywhere!", "Let's buy matching shell necklaces.", "The music here is amazing.", "I need a hammock.", "Tiki torches at night are magic.", "Postcard time!"},
    {"Fresh powder!", "I'm gonna wipe out, watch.", "My cheeks are frozen.", "Race you to the bottom!", "Snowball fight?!", "Ski bunny era."},
    {"Hot cocoa with extra marshmallows.", "This fireplace is everything.", "Cozy sweater weather.", "My legs are SO sore.", "Board games by the fire?", "Snowed in. Perfect."},
    {"Oh la la, the boutiques!", "A crepe on every corner.", "Everyone here is so chic.", "I'm buying a beret.", "The cafes are dreamy.", "City of lights!"},
    {"The view up here!", "You can see the whole city.", "Take my picture!", "It sparkles at night.", "I'm scared of heights. Ish.", "Bucket list: done."},
};
static const char* KIDL[] = {"Tag, you're it!", "I lost a tooth!", "Can we get ice cream?", "I'm a dinosaur! RAWR!", "Watch this cartwheel!", "Are we there yet?"};
static const char* TEENL[] = {"That's so cringe.", "Literally obsessed.", "My mom took my phone.", "No cap, that's iconic.", "Main character energy.", "Is anyone else bored?"};
static const char* ELDER[] = {"Back in my day...", "These young folks and their phones.", "My knees can tell it'll rain.", "Have you eaten? You look thin.", "I've still got moves!", "Bingo night, anyone?"};
static const char* TRAIT[TR_COUNT][2] = {
    {"Is that vintage? Serving.", "My outfit took two hours."}, {"What a beautiful day!", "Everything's gonna be great!"},
    {"Love is in the air~", "Have you ever been in love?"}, {"I just finished the best book.", "Did you know otters hold hands?"},
    {"I could eat a whole pizza.", "Snack recommendations?"}, {"I need to paint this sky.", "Everything's an art project."},
    {"Hiiii! Let's be friends!", "Party at mine later?"}, {"...I'd rather be at home.", "People are a lot today."},
    {"I'm getting promoted. Watch.", "Time is money, babe."}, {"Is it nap time yet?", "Walking is SO much effort."},
    {"Ten laps before breakfast!", "Let's race!"}, {"Ew, is that a stain?", "Everything in its place."},
    {"Knock knock!", "I'm hilarious, admit it."}, {"Paparazzi, find my good side.", "Diamonds are a girl's best friend."},
    {"Family first, always.", "I miss my grandma's cooking."}, {"Don't. Test. Me.", "Who touched my stuff?!"},
    {"This song is stuck in my head. Happily.", "Name a better album. You can't."}, {"Oops! Did anyone see that?", "Why is the floor so... floor-y?"},
    {"I was never here.", "Nothing to see here, officer."}, {"Who was that you were talking to?", "I'm not jealous. I'm just... aware."},
    {"I look good and I know it.", "Confidence is my best accessory."}, {"Is this... polyester?", "I only shop designer, sweetie."},
    {"Look at that puppy!!", "I want to adopt EVERY pet in town."}, {"Night time is MY time.", "Mornings are a crime."},
    {"One more level. Then sleep.", "GG, easy."}, {"It has to be PERFECT.", "Hmm. Almost right."},
    {"Want half my snack?", "You're doing amazing, sweetie!"}, {"Ugh. You again.", "Nice try. Not really."},
    {"I'd never let my people down.", "Loyalty is everything."}, {"I got you a little something!", "Lunch is on me today."},
    {"Is this on sale? It should be.", "I made coffee at home. Saved $6."}, {"Tomorrow's gonna be amazing.", "Every cloud has a silver lining!"},
    {"I literally cannot with today.", "Did you SEE what she posted?!"}, {"Have you seen my other shoe?", "It's organized chaos."},
    {"I just ordered something... again.", "Add to cart is my cardio."}, {"Do I look okay? Be honest.", "Wait, are you mad at me?"},
    {"Hey you~ come here often?", "Is it hot in here or is it me?"}, {"Where is my spotlight?", "I don't do lines. Or waiting."}, {"I know what you did.", "Everyone has a price, babe."}, {"You look amazing, by the way.", "I could talk to you forever."}, {"Haunted house? Let's go.", "I'll go first. Obviously."}, {"My tomatoes are THRIVING.", "Up at dawn with the chickens."}, {"I'm not like other girls...", "I only have guy friends, honestly."}, {"U up?", "I'm not looking for anything serious."}, {"No strings, no problem.", "Life's too short to be shy."}, {"Is this pink enough? No.", "Bows on everything, always."}, {"Skirts? Hard pass.", "Race you to the court!"}, {"Dinner's at six. Sharp.", "My house, my rules, my casserole."}, {"Bless your heart.", "See you Sunday, sweetie."}, {"Wanna make $500 fast? Trust me.", "It's not a scam, it's an opportunity."}, {"...um, hi.", "I'll just stand over here."}, {"Daddy's card has no limit.", "What do you mean 'budget'?"}, {"And? Your point?", "I'm not mean, I'm honest."}, {"Have you SEEN my face today?", "Hold on, let me fix my hair."}, {"Is this vegan? I'll check.", "Plants are the future, babe."}, {"Wanna spar?", "Float like a butterfly."}, {"Five, six, seven, eight!", "I can't stop dancing."}, {"Ball is life.", "Did you see that game last night?!"}, {"I will DIE for my faves.", "Did you see the new album?!"}, {"Actually, in the lore...", "Day one fan, don't test me."}, {"Fun fact: did you know...", "I finished the homework early."}, {"Full beat, every day.", "Lashes are non-negotiable."}, {"Grind don't stop.", "Loyalty over everything."}, {"It looks expensive. It wasn't.", "Champagne taste, coupon budget."}, {"Cute... for you.", "I'm not saying anything. *side-eye*"}, {"Need something? I know a guy.", "I got you. Always."}, {"Don't. Start.", "I'm calm. I'm CALM."}, {"This block raised me.", "Keep it real or keep it moving."}
};
static const char* RAIN[] = {"I forgot my umbrella!", "My hair is ruined.", "Rainy days are cozy days."};
static const char* SNOW[] = {"Snowball fight?!", "It's FREEZING.", "Hot cocoa weather!"};
static const char* HEAT[] = {"It's SO hot.", "I'm melting...", "Pool. Now."};
static const char* NIGHT[] = {"Why am I still awake?", "The stars are so pretty.", "Late night snack run?"};
static const char* COP[] = {"Keep it moving, folks.", "I've got my eye on you.", "Nice and orderly, people.", "Anyone seen a shoplifter?"};
static const char* ABOUT_YOU[] = {"Did you see what {MC} did?!", "OMG, it's {MC}...", "*whispers* that's the one from The Tea!", "I can't believe {MC}, honestly."};
static const char* WANTED[] = {"Isn't {MC} the one the cops are after?", "Hold onto your purse - {MC} is here."};

static std::string fillMC(const char* s) {
    std::string o = s;
    size_t k;
    while ((k = o.find("{MC}")) != std::string::npos) o.replace(k, 4, MC().name);
    return o;
}

std::string ambientLine(int sim) {
    if (sim < 0 || sim >= (int)G.sims.size()) return "";
    const Sim& s = G.sims[sim];
    if (sim == G.cop) return COP[irand(4)];
    const int h = (int)(G.minute / 60);
    int r = irand(100);
    if (r < 40) { std::string l = lifeLine(sim); if (!l.empty()) return l; }   // their job, news, memories, hobby, dream
    r = irand(100);
    if (G.myScandal > 20 && r < 22) return fillMC(ABOUT_YOU[irand(4)]);
    if (G.heat >= 60 && r < 30) return fillMC(WANTED[irand(2)]);
    if (s.stage <= ST_PRETEEN && s.stage >= ST_CHILD && r < 55) return KIDL[irand(6)];
    if (s.stage == ST_TEEN && r < 40) return TEENL[irand(6)];
    if (s.stage == ST_ELDER && r < 45) return ELDER[irand(6)];
    if (r < 60 && roomOutdoors(G.room)) {
        if (G.weather == WX_RAIN || G.weather == WX_STORM) return RAIN[irand(3)];
        if (G.weather == WX_SNOW) return SNOW[irand(3)];
        if (G.weather == WX_HEAT) return HEAT[irand(3)];
    }
    if ((h >= 22 || h < 5) && r < 50) return NIGHT[irand(3)];
    if (r < 75) { uint8_t t = s.traits[irand(3)]; if (t < TR_COUNT) return TRAIT[t][irand(2)]; }
    { const char* l = PLACE[G.room % R_COUNT][irand(6)]; return l ? l : "What a day!"; }
}

// ---------------------------------------------------------------- townie conversations
struct ConvoDef { const char* l[4]; };
static const ConvoDef CONVOS[] = {
    {{"Did you hear about {X}?", "No WAY. Spill!", "My lips are sealed... okay fine.", nullptr}},
    {{"Cute shoes!", "Thanks! They were on sale!", "Stop, I need them.", nullptr}},
    {{"Are you going to the party?", "Only if you're going!", "Then we're going!", nullptr}},
    {{"I think {X} likes you.", "WHAT. Since when?!", "Since forever, obviously.", "*screams internally*"}},
    {{"Ugh, Mondays.", "Tell me about it.", nullptr, nullptr}},
    {{"Want to grab a latte later?", "Always. Extra foam.", nullptr, nullptr}},
    {{"Is that a new haircut?", "You noticed!", "It's giving main character.", nullptr}},
    {{"I'm starting a band.", "You can't play anything.", "Details!", nullptr}},
    {{"Did you see The Tea today?", "{MC} is ALL over it.", "Yikes...", nullptr}},
    {{"Where'd you get that bag?", "Secret.", "...it's fake, isn't it.", "HOW DARE YOU."}},
    {{"I'm SO tired.", "Same. Nap pact?", "Nap pact.", nullptr}},
    {{"Have you met {MC}?", "Yeah! Total sweetheart.", nullptr, nullptr}},
};
static const int NUM_CONVOS = sizeof(CONVOS) / sizeof(CONVOS[0]);
static int cA = -1, cB = -1, cDef = -1, cLine = 0;
static float cT = 0, cCool = 6;
static std::string cX;
static std::string cLife[4]; static int cLifeN = 0;   // a conversation about their actual lives (life.cpp)
static const int LIFE_CONVO = 9999;

static Actor* actorFor(int sim) { for (Actor& a : curRoom().actors) if (a.kind == 0 && a.id == sim) return &a; return nullptr; }
void chatterTick(float dt) {
    if (G.sims.empty()) return;
    RoomState& rs = curRoom();
    if (cDef >= 0) {
        Actor* a = actorFor(cA); Actor* b = actorFor(cB);
        if (!a || !b) { cDef = -1; return; }
        cT -= dt;
        if (cT > 0) return;
        const char* ln = cDef == LIFE_CONVO ? (cLine < cLifeN ? cLife[cLine].c_str() : nullptr) : cLine < 4 ? CONVOS[cDef].l[cLine] : nullptr;
        if (!ln) { cDef = -1; cCool = 8 + frand() * 10; return; }
        std::string s = fillMC(ln);
        size_t k; while ((k = s.find("{X}")) != std::string::npos) s.replace(k, 3, cX);
        Actor* sp = cLine % 2 == 0 ? a : b; Actor* li = cLine % 2 == 0 ? b : a;
        sp->say = s; sp->sayT = 2.4f; sp->facingLeft = li->x < sp->x; li->facingLeft = sp->x < li->x;
        sp->walking = false; li->walking = false; li->wait = std::fmax(li->wait, 3.f); sp->wait = std::fmax(sp->wait, 3.f);
        cLine++; cT = 2.5f;
        return;
    }
    cCool -= dt;
    if (cCool > 0) return;
    cCool = 3;
    // two townies standing near each other strike up a conversation
    for (size_t i = 0; i < rs.actors.size(); i++) {
        const Actor& a = rs.actors[i];
        if (a.kind != 0 || a.id == G.mc || a.id >= G.sims.size() || G.sims[a.id].stage < ST_CHILD || a.sleeping) continue;
        for (size_t j = i + 1; j < rs.actors.size(); j++) {
            const Actor& b = rs.actors[j];
            if (b.kind != 0 || b.id == G.mc || b.id >= G.sims.size() || G.sims[b.id].stage < ST_CHILD || b.sleeping) continue;
            if (std::fabs(a.x - b.x) > 110 || irand(3)) continue;
            cA = a.id; cB = b.id; cDef = irand(NUM_CONVOS); cLine = 0; cT = 0.2f;
            if (irand(100) < 55 && (cLifeN = lifeConvo(cA, cB, cLife)) > 0) { cDef = LIFE_CONVO; return; }
            if (cDef == 8 && !G.myScandal) cDef = 0;
            int x = irand((int)G.sims.size());
            cX = (x != cA && x != cB) ? G.sims[x].name : std::string("the new barista");
            return;
        }
    }
}

// ---------------------------------------------------------------- social line variety
struct Vary { const char* base; const char* alt[3]; };
static const Vary VARY[] = {
    {"So how's life?", {"What's the tea today?", "How've you BEEN?", "Tell me everything."}},
    {"Pretty good!", {"Honestly? Living my best life.", "Can't complain!", "Busy, but cute busy."}},
    {"Love your look!!", {"Okay, that outfit?! Obsessed.", "You look SO good today!", "Who's your stylist?!"}},
    {"Aww stop! <3", {"You're too sweet!", "Stoppp, I'm blushing.", "Thank youuu!"}},
    {"Why did the purse go to therapy?", {"What do you call a fancy fish? So-fish-ticated.", "I told my mirror a joke... it cracked up.", "Why are shoes so loyal? They stick to your sole."}},
    {"HAHAHA", {"LMAO stop!", "I'm DEAD.", "Okay that was good."}},
    {"Did you hear about...", {"Okay so you didn't hear this from me...", "You will NOT believe this.", "Promise not to tell?"}},
    {"No WAY!", {"Shut UP.", "I KNEW it!", "Wait, WHAT?!"}},
    {"Come here!", {"Bring it in!", "Hug time!", "Squeeze!"}},
    {"Let's dance!", {"This is my SONG!", "Dance with me!", "Show me your moves!"}},
    {"Woo!", {"Yesss!", "Let's GO!", "Ayyy!"}},
    {"Nice outfit... from 1995?", {"Bold of you to wear that.", "Did you get dressed in the dark?", "Interesting... choice."}},
    {"EXCUSE me?!", {"Oh no you DIDN'T.", "Wow. Okay.", "Say that again. I dare you."}},
    {"Is it hot in here, or is it you?", {"Are you a parking ticket? 'Cause you've got FINE written all over you.", "You must be tired, you've been running through my mind.", "Do you believe in love at first sight?"}},
    {"Hehe ;)", {"Smooth...", "*giggles*", "You're trouble."}},
    {"Ew, no.", {"...pass.", "Wow, no.", "Was that a line?"}},
    {"You make me smile, you know?", {"You're my favorite person.", "I think about you a lot.", "You make everything better."}},
};
const char* varyLine(const char* line, int sim) {
    if (!line) return line;
    (void)sim;
    for (const Vary& v : VARY) if (!std::strcmp(v.base, line)) { int k = irand(4); return k == 0 ? v.base : v.alt[k - 1]; }
    return line;
}
