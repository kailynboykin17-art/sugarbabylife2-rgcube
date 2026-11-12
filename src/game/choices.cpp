#include "life3.hpp"
#include "settings2.hpp"
#include "choices.hpp"
#include "game.hpp"
#include "sim.hpp"
#include "extras.hpp"
#include "emote.hpp"
#include "growup.hpp"
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include "vfs.hpp"
#define fopen vfopen

bool menuStillOpen();
namespace {
struct Opt { const char* label; int rel, mood, money, rep; const char* now; const char* later; int laterRel; };
struct Scene { const char* prompt; bool kidOk; Opt o[3]; };
// {X} = the other person. mood: 1 happy, -1 sad/angry, 0 none
const Scene SC[] = {
    {"{X} posted an unflattering pic of you!", true, {
        {"Confront her", -4, -1, 0, 0, "You DM'd {X}: \"Take it down.\" She did... with an attitude.", "{X} reposted it on her close friends story. Petty.", -3},
        {"Let it go", 2, 0, 0, 1, "You let it slide. Mature queen.", "{X} apologized and posted a cute pic of you instead!", 6},
        {"Spill her tea back", -8, 1, 0, -1, "You posted her worst throwback. The comments are a WAR ZONE.", "{X} found out it was you. You two are NOT okay.", -8}}},
    {"{X} borrowed $50 and \"forgot\" to pay you back.", false, {
        {"Ask for it back", -2, 0, 50, 0, "Awkward, but {X} Venmo'd you. +$50", "", 0},
        {"Forget about it", 3, -1, 0, 0, "You let it go. $50 is $50 though...", "{X} surprised you with lunch to say thanks!", 5},
        {"Tell everyone", -9, 1, 0, -2, "Now the whole town knows {X} doesn't pay people back.", "{X} paid you back... and blocked you.", -5}}},
    {"You saw {X}'s partner flirting with someone else!", false, {
        {"Tell {X}", 6, -1, 0, 1, "{X} cried, then thanked you. Real friend.", "{X} and their partner broke up. {X} says you saved them.", 6},
        {"Stay out of it", 0, 0, 0, 0, "Not your business... right?", "{X} found out you KNEW. \"How could you not tell me?!\"", -10},
        {"Confront the partner", 2, 1, 0, -1, "You made a SCENE. Everyone's talking.", "{X} heard what you did. Ride or die!", 5}}},
    {"{X} wants to copy your homework.", true, {
        {"Let them", 5, 0, 0, 0, "{X} owes you one.", "You BOTH got caught copying. Detention!", -2},
        {"Say no", -3, 0, 0, 1, "{X} rolled their eyes. Rules are rules.", "{X} got a C and blames you.", -2},
        {"Charge $10", -1, 1, 10, 0, "Business is business. +$10", "Now half the class wants to buy your homework.", 0}}},
    {"{X} wore the SAME outfit as you today.", true, {
        {"Laugh it off", 5, 1, 0, 1, "Twinning! You took a selfie together.", "", 0},
        {"Go change", 0, -1, 0, 0, "You ran home and changed. Crisis averted.", "", 0},
        {"Throw shade", -7, 1, 0, -1, "\"It looks better on me.\" Ouch.", "{X} started copying your style ON PURPOSE now.", -3}}},
    {"{X} is spreading a rumor about you!", true, {
        {"Confront them", -3, 1, 0, 1, "\"Say it to my face.\" {X} backed down.", "The rumor died out. Respect.", 2},
        {"Ignore it", 0, -1, 0, -1, "You ignored it. It's still out there...", "Nobody even remembers the rumor now.", 0},
        {"Start a counter-rumor", -6, 1, 0, -2, "Two can play that game.", "The rumor war got SO messy. You both look bad.", -4}}},
    {"{X} invites you out on a school/work night.", true, {
        {"Go!", 5, 1, -20, 0, "Best night ever. Worst morning ever.", "", 0},
        {"Stay home", -2, 0, 0, 0, "Responsible you. Boring you?", "{X} posted pics from the night. FOMO.", 0},
        {"Go, but leave early", 3, 1, -10, 0, "The perfect balance.", "", 0}}},
    {"You found {X}'s diary...", true, {
        {"Read it", -2, 1, 0, 0, "You know EVERYTHING now. Including about you.", "{X} realized someone read it. Paranoid much?", -3},
        {"Give it back", 7, 0, 0, 1, "{X} is so grateful. Trust level: max.", "{X} trusted you with a BIG secret.", 4},
        {"Tell their crush what's in it", -12, 1, 0, -2, "Chaos. Absolute chaos.", "{X} will never forgive you for this one.", -10}}},
    {"{X} asks your honest opinion on their awful new haircut.", true, {
        {"Be honest", -3, 0, 0, 1, "\"It's... a lot.\" {X} is wearing a hat now.", "{X} fixed it and thanked you for being real.", 5},
        {"Lie: \"It's cute!\"", 4, 0, 0, 0, "{X} believes you. For now.", "{X} found out everyone hated it. \"You LIED?\"", -5},
        {"Change the subject", 0, 0, 0, 0, "\"Sooo... did you see the game?\" Smooth.", "", 0}}},
    {"Something broke at home and {X} is blaming YOU.", true, {
        {"Tell the truth", 2, 0, 0, 1, "You told the truth. {X} got busted.", "{X} is mad you snitched.", -4},
        {"Take the blame", 6, -1, 0, 0, "You took the fall. {X} owes you BIG.", "{X} covered for you when you needed it.", 5},
        {"Blame the dog", 0, 1, 0, 0, "The dog did it. Obviously.", "Nobody believed it was the dog.", -1}}},
    {"A cute stranger asks for your number.", false, {
        {"Give it", 0, 1, 0, 0, "You're blushing. They texted 2 minutes later!", "Your date with them went... surprisingly well!", 0},
        {"Politely no", 0, 0, 0, 0, "\"I'm flattered, but no.\"", "", 0},
        {"Give a fake number", 0, 1, 0, 0, "You gave them the pizza place's number. Iconic.", "The pizza place called to ask if you're okay.", 0}}},
    {"{X} wants to be in your group project... and does nothing.", true, {
        {"Call them out", -5, 1, 0, 1, "{X} finally did their part.", "You got an A! {X} is weirdly proud.", 3},
        {"Do it all yourself", 2, -1, 0, 0, "You did everything. Again.", "{X} took credit in front of the whole class!", -6},
        {"Tell the teacher/boss", -8, 0, 0, 0, "{X} got a lecture. Petty but fair.", "", 0}}},
    {"{X} keeps texting you at 3 AM.", true, {{"Answer", 4, -1, 0, 0, "You talked till sunrise. Tired but bonded.", "", 0}, {"Mute them", -2, 0, 0, 0, "Sweet, sweet sleep.", "{X} asked why you're ignoring them.", -2}, {"Reply \"go to SLEEP\"", 1, 1, 0, 0, "{X} sent 14 crying emojis.", "", 0}}},
    {"{X} wants to borrow your favorite outfit.", true, {{"Lend it", 5, 0, 0, 0, "{X} looked amazing in it. Ugh.", "{X} returned it with a stain!", -4}, {"Say no", -3, 0, 0, 0, "\"It's sentimental, okay?\"", "", 0}, {"Rent it for $15", -1, 1, 15, 0, "Small business owner. +$15", "", 0}}},
    {"You caught {X} talking about you behind your back.", true, {{"Confront them", -4, 1, 0, 1, "\"I heard EVERYTHING.\" {X} went pale.", "{X} sent a long apology text.", 4}, {"Pretend you didn't hear", 0, -1, 0, 0, "You smiled. It hurt.", "", 0}, {"Talk about THEM louder", -8, 1, 0, -1, "Two can play this game.", "The whole friend group picked sides.", -5}}},
    {"{X} got the role/promotion you wanted.", false, {{"Congratulate them", 6, -1, 0, 1, "Classy. You hugged it out.", "{X} put in a good word for you!", 4}, {"Sulk", -2, -1, 0, 0, "You ate ice cream in the car.", "", 0}, {"Say it was rigged", -7, 0, 0, -2, "Bitter much?", "{X} heard what you said.", -4}}},
    {"Your phone dies right before plans with {X}.", true, {{"Show up anyway", 4, 0, 0, 0, "You found them! Old-school.", "", 0}, {"Stay home", -5, -1, 0, 0, "{X} waited an hour. Oops.", "{X} still brings it up.", -2}, {"Borrow a stranger's phone", 2, 1, 0, 0, "A random grandma helped you out.", "", 0}}},
    {"{X} wants to start a business with you.", false, {{"Go for it!", 5, 1, -100, 0, "You're officially partners. -$100 startup costs.", "Your business made $400!", 3}, {"Pass", -2, 0, 0, 0, "Friends and money don't mix.", "{X} started it alone and it's doing great.", 0}, {"Ask for a contract", 1, 0, 0, 1, "Smart AND supportive.", "", 0}}},
    {"{X} is crying in the bathroom.", true, {{"Comfort them", 8, 0, 0, 1, "You sat with {X} till they felt better.", "{X} told everyone you're the sweetest.", 5}, {"Get a teacher/boss", 2, 0, 0, 0, "Help is on the way.", "", 0}, {"Walk away awkwardly", -3, -1, 0, 0, "Too much emotion. You panicked.", "", 0}}},
    {"You accidentally liked {X}'s post from 3 years ago.", false, {{"Unlike it fast", 0, -1, 0, 0, "They definitely saw. They DEFINITELY saw.", "{X} liked one of yours from 2019. War.", 1}, {"Own it, comment too", 3, 1, 0, 0, "\"Throwback queen!\" Bold.", "", 0}, {"Delete the app", 0, -1, 0, 0, "You are now living off-grid.", "", 0}}},
    {"{X} invites you to their family dinner.", true, {{"Go and be charming", 6, 1, 0, 1, "Their grandma LOVES you.", "{X}'s family invited you to their vacation!", 4}, {"Make an excuse", -3, 0, 0, 0, "\"I have... a thing.\"", "", 0}, {"Go, but be weird", -1, 0, 0, -1, "You talked about conspiracy theories. For an hour.", "", 0}}},
    {"{X} copied your whole look.", true, {{"Take it as a compliment", 4, 1, 0, 1, "Imitation is flattery!", "", 0}, {"Change your style", 0, 0, -30, 0, "New look! -$30", "{X} copied the NEW look too.", -2}, {"Call it out online", -6, 0, 0, -1, "The comments are... mixed.", "", 0}}},
    {"You find $100 on the sidewalk.", true, {{"Keep it", 0, 1, 100, 0, "Finders keepers! +$100", "", 0}, {"Turn it in", 0, 1, 0, 2, "Good karma. Everyone's impressed.", "The owner found you and gave you a $50 reward!", 0}, {"Split it with {X}", 6, 1, 50, 0, "Shared luck! +$50 each.", "", 0}}},
    {"{X} asks you to keep a BIG secret.", true, {{"Take it to the grave", 7, 0, 0, 0, "Your lips are sealed.", "{X} trusts you more than anyone now.", 4}, {"Tell one person", -2, 1, 0, 0, "Just ONE person. That's fine. Right?", "The secret got out. {X} knows it was you.", -10}, {"Say you can't keep secrets", 0, 0, 0, 0, "Honest, at least.", "", 0}}},
    {"Your ex texts \"hey stranger\".", false, {{"Reply", 0, 1, 0, 0, "Here we go again...", "Your ex asked to hang out. Hmm.", 0}, {"Leave on read", 0, 1, 0, 0, "Growth. Healing. Power.", "", 0}, {"Screenshot & send to the group chat", 0, 1, 0, -1, "The group chat is SCREAMING.", "", 0}}},
    {"{X} challenges you to a dance battle.", true, {{"Accept!", 3, 1, 0, 1, "You went OFF. The crowd went wild.", "", 0}, {"Decline", -1, 0, 0, -1, "Chicken!", "", 0}, {"Bring backup dancers", 5, 1, 0, 2, "Full choreography. Legendary.", "", 0}}},
    {"Your neighbor's dog keeps barking at night.", false, {{"Knock and ask nicely", 0, 0, 0, 1, "They bought a bark collar. Peace!", "", 0}, {"Complain to the HOA", 0, 0, 0, -1, "Bureaucracy wins.", "Your neighbor is giving you the cold shoulder.", 0}, {"Buy earplugs", 0, 0, -10, 0, "Problem solved. -$10", "", 0}}},
    {"{X} wants you to be their wingman.", false, {{"Hype them up", 6, 1, 0, 0, "{X} got the number! You're a legend.", "{X} and their crush are dating now!", 4}, {"Steal the crush", -12, 1, 0, -2, "Oops. You got the number instead.", "{X} is NOT speaking to you.", -6}, {"Make it awkward", -3, 0, 0, 0, "You said something weird. Mission failed.", "", 0}}},
    {"Someone ate your leftovers from the fridge.", true, {{"Find the culprit", -3, 1, 0, 0, "It was {X}. Obviously.", "", 0}, {"Label everything", 0, 0, 0, 0, "Your name is on EVERYTHING now.", "", 0}, {"Eat THEIR leftovers", -4, 1, 0, 0, "Revenge tastes like lasagna.", "{X} started a fridge war.", -3}}},
    {"{X} wants to go on a road trip with you.", false, {{"Yes! Road trip!", 7, 1, -60, 0, "Gas station snacks & singing. -$60", "You and {X} made a scrapbook of the trip.", 3}, {"Too busy", -2, 0, 0, 0, "Maybe next time.", "", 0}, {"Only if you pick the music", 3, 1, -60, 0, "Your playlist ruled. -$60", "", 0}}},
    {"Your teacher/boss calls you out in front of everyone.", true, {{"Apologize", 0, -1, 0, 1, "Embarrassing, but handled.", "", 0}, {"Talk back", 0, 1, 0, -2, "\"With all due respect...\" The room gasped.", "You got written up / detention.", 0}, {"Laugh it off", 0, 0, 0, 0, "Cool under pressure.", "", 0}}},
    {"{X} forgot your birthday.", true, {{"Say nothing", -2, -1, 0, 0, "You're fine. Totally fine.", "{X} realized and threw you a late party!", 6}, {"Remind them", 2, 0, 0, 0, "{X} was SO sorry. Cake incoming.", "", 0}, {"Forget THEIR birthday", -5, 1, 0, 0, "Petty revenge, scheduled.", "", 0}}},
    {"You're stuck in an elevator with {X}.", false, {{"Deep talk", 7, 0, 0, 0, "You learned SO much about {X}.", "", 0}, {"Panic", -1, -1, 0, 0, "You hit every button. Twice.", "", 0}, {"Make it a party", 4, 1, 0, 0, "Elevator karaoke!", "", 0}}},
    {"{X} asks to copy your notes before the test.", true, {{"Share", 5, 0, 0, 0, "Study buddies!", "You both aced it!", 3}, {"Say no", -3, 0, 0, 1, "Earn it yourself.", "", 0}, {"Give them fake notes", -10, 1, 0, -2, "Evil. Iconic. Evil.", "{X} failed and knows it was you.", -8}}},
    {"A brand wants to sponsor you for $200.", false, {{"Take the deal", 0, 1, 200, -1, "Cha-ching! #ad +$200", "Followers say you sold out.", 0}, {"Decline", 0, 0, 0, 1, "Authentic queen.", "", 0}, {"Negotiate for more", 0, 1, 350, 0, "You got $350! Business brain.", "", 0}}},
    {"{X} wants to set you up on a blind date.", false, {{"Sure!", 3, 1, 0, 0, "Your date was... actually cute?", "You've been texting your blind date nonstop.", 0}, {"Absolutely not", -1, 0, 0, 0, "You'd rather stay home with snacks.", "", 0}, {"Only a double date", 4, 1, 0, 0, "Safety in numbers!", "", 0}}},
    {"Your sibling/cousin borrowed your car/bike without asking.", true, {{"Yell at them", -5, 1, 0, 0, "LOUD family moment.", "", 0}, {"Make them pay gas", -1, 0, 20, 0, "+$20. Fair.", "", 0}, {"Borrow their stuff back", -3, 1, 0, 0, "Eye for an eye.", "", 0}}},
    {"{X} says your cooking is \"interesting\".", true, {{"Ask what they meant", 1, 0, 0, 0, "\"Too much salt.\" Noted.", "", 0}, {"Never cook for them again", -3, 1, 0, 0, "Their loss.", "", 0}, {"Cook something better", 4, 1, 0, 0, "Your second dish SLAPPED.", "", 0}}},
    {"You see {X} at the store in their pajamas.", true, {{"Wave enthusiastically", 2, 1, 0, 0, "{X} pretended not to see you.", "", 0}, {"Hide behind the cereal", 0, 0, 0, 0, "Mission: unseen. Success.", "", 0}, {"Take a picture", -4, 1, 0, 0, "Blackmail material acquired.", "{X} found the pic in your phone.", -3}}},
    {"{X} wants to move in as your roommate.", false, {{"Yes!", 6, 1, 200, 0, "Rent split! +$200 this month.", "{X} never does the dishes.", -3}, {"No", -3, 0, 0, 0, "You need your space.", "", 0}, {"Trial week", 2, 0, 0, 0, "Sounds fair.", "The trial week went great!", 3}}},
    {"{X} is getting bullied.", true, {{"Step in", 10, 1, 0, 2, "You shut it DOWN. Hero.", "{X} says you changed their life.", 6}, {"Report it", 5, 0, 0, 1, "The adults handled it.", "", 0}, {"Look away", -6, -1, 0, -1, "That didn't feel right.", "", 0}}},
    {"Your favorite store has a huge sale.", false, {{"Shop till you drop", 0, 1, -150, 0, "Bags everywhere. -$150", "", 0}, {"Buy one thing", 0, 1, -30, 0, "Restraint! -$30", "", 0}, {"Window shop only", 0, -1, 0, 0, "Your wallet thanks you.", "", 0}}},
    {"{X} wants to start a band with you.", true, {{"Join!", 6, 1, 0, 1, "Your band name: The Glitter Riots.", "Your band played its first gig!", 3}, {"You can't sing...", -1, 0, 0, 0, "Fair.", "", 0}, {"Be the manager", 4, 1, 0, 0, "You're the boss now.", "", 0}}},
    {"{X} spilled coffee all over your outfit.", true, {{"Laugh it off", 4, 0, 0, 1, "Accidents happen!", "", 0}, {"Make them pay", -3, 0, 40, 0, "Dry cleaning: +$40", "", 0}, {"Spill yours on them", -7, 1, 0, -1, "Coffee war. Everyone is sticky.", "", 0}}},
    {"Your parents want to meet {X}.", true, {{"Set it up", 5, 0, 0, 1, "Your parents adore {X}.", "", 0}, {"Stall forever", -2, 0, 0, 0, "\"Next week. Probably.\"", "", 0}, {"Warn {X} about your family", 3, 1, 0, 0, "{X} came prepared. Smart.", "", 0}}},
    {"{X} gave you an ugly gift.", true, {{"Fake-love it", 5, 0, 0, 0, "\"OMG I LOVE IT.\" Oscar-worthy.", "{X} got you a matching one. Help.", 1}, {"Be honest", -4, 0, 0, 0, "{X} is hurt.", "", 0}, {"Regift it", 0, 1, 0, 0, "Recycling!", "{X} saw it at someone else's house.", -6}}},
    {"You get a text: \"we need to talk\" from {X}.", true, {{"Call right away", 3, -1, 0, 0, "It was nothing! They just wanted to chat.", "", 0}, {"Panic for 3 hours", 0, -1, 0, 0, "Your heart can't take this.", "", 0}, {"Reply \"about what??\"", 1, 0, 0, 0, "\"About the party!\" Phew.", "", 0}}},
    {"{X} wants to try a TikTak trend with you.", true, {{"Do it!", 5, 1, 0, 1, "It went viral! +1,000 views.", "", 0}, {"Too embarrassing", -1, 0, 0, 0, "Your dignity remains.", "", 0}, {"Do it badly on purpose", 3, 1, 0, 0, "Comedy gold.", "", 0}}},
    {"Someone keyed {X}'s car and they think it was you.", false, {{"Prove your innocence", 3, -1, 0, 1, "Camera footage cleared you!", "", 0}, {"Get offended", -6, 1, 0, 0, "\"How DARE you.\"", "", 0}, {"Find the real culprit", 7, 1, 0, 2, "Detective mode. Case closed.", "", 0}}},
    {"{X} brags about their new phone nonstop.", true, {{"Hype them up", 3, 0, 0, 0, "Supportive friend.", "", 0}, {"Brag about yours", -2, 1, 0, 0, "Phone-off!", "", 0}, {"Hide their phone", -4, 1, 0, 0, "Chaos agent.", "{X} found it in the freezer.", -2}}},
    {"A fortune teller says your life will change soon.", true, {{"Believe it", 0, 1, -10, 0, "You're SO ready. -$10 for the reading", "Something good happened... coincidence?", 0}, {"Laugh", 0, 0, 0, 0, "Sure, lady.", "", 0}, {"Ask for lottery numbers", 0, 1, -10, 0, "\"7, 14, 22...\" -$10", "", 0}}},
    {"{X} keeps one-upping every story you tell.", true, {{"Call it out", -3, 1, 0, 0, "\"Not everything is about you!\"", "", 0}, {"Stop telling stories", -1, -1, 0, 0, "Fine. Silence.", "", 0}, {"Make up a wild story", 2, 1, 0, 0, "You \"met a celebrity\". They can't top that.", "", 0}}},
    {"{X} wants to throw you a surprise party.", true, {{"Act surprised", 6, 1, 0, 0, "\"OMG I had NO idea!\" (you knew)", "", 0}, {"Say you hate parties", -4, -1, 0, 0, "Party canceled. Mood killed.", "", 0}, {"Help plan your own party", 3, 1, 0, 0, "Control freak, but cute.", "", 0}}},
    {"You hear a weird noise in the house at night.", true, {{"Investigate", 0, 1, 0, 0, "It was the cat. Obviously.", "", 0}, {"Hide under the covers", 0, -1, 0, 0, "Nope. Not today.", "", 0}, {"Wake everyone up", 0, 0, 0, 0, "Family meeting at 3 AM: it was the fridge.", "", 0}}},
    {"{X} wants your honest opinion on their singing.", true, {{"Be honest", -3, 0, 0, 1, "\"...Maybe lessons?\"", "{X} took lessons. They're GOOD now.", 4}, {"Lie", 3, 0, 0, 0, "\"Beyoncé who?\"", "{X} auditioned for a show and bombed.", -3}, {"Sing with them", 5, 1, 0, 0, "Duet! Both terrible, both happy.", "", 0}}},
    {"Your crush is at the same party as you.", false, {{"Go talk to them", 0, 1, 0, 1, "You were smooth. SO smooth.", "Your crush followed you back!", 0}, {"Stare from across the room", 0, -1, 0, 0, "Mysterious... or creepy?", "", 0}, {"Get {X} to introduce you", 4, 1, 0, 0, "{X} is the best wingman.", "", 0}}},
    {"{X} is always late.", true, {{"Lie about the time", 0, 1, 0, 0, "You said 6. It was 7. They were on time!", "", 0}, {"Leave without them", -4, 0, 0, 0, "Punctuality matters.", "", 0}, {"Accept it", 2, 0, 0, 0, "They're worth the wait.", "", 0}}},
    {"You win a radio contest!", true, {{"Take the cash ($300)", 0, 1, 300, 0, "+$300!", "", 0}, {"Take the concert tickets", 0, 1, 0, 0, "Front row seats!", "The concert was the best night of your life.", 0}, {"Give the prize to {X}", 8, 1, 0, 1, "{X} SCREAMED.", "", 0}}},
    {"{X} needs help moving this weekend.", false, {{"Help out", 7, -1, 0, 0, "Your back hurts. Pizza was worth it.", "{X} helped YOU when you needed it.", 3}, {"\"Busy\"", -3, 0, 0, 0, "Very busy. Doing nothing.", "", 0}, {"Send pizza instead", 3, 0, -25, 0, "Supportive from a distance. -$25", "", 0}}},
    {"{X} won't stop talking about their ex.", false, {{"Listen", 4, -1, 0, 0, "Two hours. You know EVERYTHING.", "", 0}, {"Change the subject", 0, 0, 0, 0, "\"Anyway, did you eat?\"", "", 0}, {"Set them up with someone new", 5, 1, 0, 0, "Moving on era!", "", 0}}},
    {"A celebrity is filming in town!", true, {{"Try to get in the background", 0, 1, 0, 1, "You're an extra! Famous-ish.", "You can be seen for 0.5 seconds in the movie!", 0}, {"Ask for an autograph", 0, 1, 0, 0, "Got it! Framing it.", "", 0}, {"Act unbothered", 0, 0, 0, 1, "Too cool for celebrities.", "", 0}}},
    {"{X} wants to switch outfits for a day.", true, {{"Swap!", 6, 1, 0, 0, "Style swap! You both looked iconic.", "", 0}, {"No thanks", -1, 0, 0, 0, "Your outfit, your rules.", "", 0}, {"Swap but keep the shoes", 3, 1, 0, 0, "Compromise queen.", "", 0}}},
    {"{X} wants to film a prank on you for their channel.", true, {{"Play along", 5, 1, 0, 1, "The video got 10K views. You're a star!", "", 0}, {"Refuse", -2, 0, 0, 0, "Not your circus.", "", 0}, {"Prank them first", 3, 1, 0, 1, "Reverse uno! The comments love it.", "{X} wants a rematch. Prank war!", 2}}},
    {"You overhear your parents arguing about money.", true, {{"Ask what's wrong", 0, -1, 0, 0, "They promise it'll be okay.", "Your family made a budget together. Closer now.", 0}, {"Pretend you didn't hear", 0, -1, 0, 0, "You turned your music up.", "", 0}, {"Offer your savings", 0, 1, -40, 1, "They hugged you SO tight. -$40", "", 0}}},
    {"{X} dares you to jump in the pool with your clothes on.", true, {{"Do it!", 5, 1, 0, 1, "SPLASH! Legend status.", "", 0}, {"No way", -1, 0, 0, 0, "Your outfit thanks you.", "", 0}, {"Push {X} in instead", 2, 1, 0, 0, "{X} is soaked and screaming. Iconic.", "{X} got you back. You're soaked now.", 0}}},
    {"Someone left an anonymous love note for you.", true, {{"Try to find out who", 0, 1, 0, 0, "Detective mode activated.", "It was {X}!! Your heart is racing.", 5}, {"Keep it as a mystery", 0, 1, 0, 0, "Some secrets are cuter unsolved.", "", 0}, {"Post it online", 0, 0, 0, -1, "The whole town is guessing...", "Your secret admirer is embarrassed you posted it.", 0}}},
    {"{X} wants your help planning a surprise for their partner.", false, {{"Help plan it", 6, 1, 0, 0, "Rose petals, candles, playlist. Perfect.", "{X}'s partner cried happy tears!", 3}, {"Too busy", -2, 0, 0, 0, "Good luck!", "", 0}, {"Suggest something chaotic", 1, 1, 0, 0, "Flash mob? Bold.", "It was... a lot. But they loved it?", 0}}},
    {"The new kid/coworker sits alone at lunch.", true, {{"Invite them over", 0, 1, 0, 2, "They seem SO relieved. New friend!", "Your new friend saved you a seat every day since.", 0}, {"Wave from afar", 0, 0, 0, 0, "Baby steps.", "", 0}, {"Ignore them", 0, -1, 0, -1, "That felt kinda cold.", "", 0}}},
    {"{X} wants to dye your hair.", true, {{"Let them!", 4, 1, 0, 0, "Pink streaks! Bold!", "Your new hair is trending at school/work.", 0}, {"Absolutely not", -1, 0, 0, 0, "Your hair, your rules.", "", 0}, {"Only the tips", 2, 1, 0, 0, "Subtle but cute.", "", 0}}},
    {"You accidentally sent a screenshot of {X}'s text TO {X}.", true, {{"Apologize", 0, -1, 0, 0, "Mortifying. They forgave you... mostly.", "", 0}, {"\"Wrong chat lol\"", -4, 0, 0, 0, "That made it worse.", "{X} is still salty about the screenshot.", -2}, {"Throw your phone in a lake", 0, -1, -150, 0, "Problem solved? -$150 for a new phone.", "", 0}}},
    {"{X} offers you a free concert ticket.", true, {{"Go!", 6, 1, 0, 0, "Front row! You lost your voice.", "", 0}, {"Can't, homework/work", -1, -1, 0, 1, "Responsible queen.", "", 0}, {"Sell the ticket", -4, 1, 80, 0, "+$80. {X} is NOT happy.", "", 0}}},
    {"Your sibling/roommate keeps using your makeup.", true, {{"Confront them", -3, 1, 0, 0, "\"BUY. YOUR. OWN.\"", "", 0}, {"Hide your makeup", 0, 0, 0, 0, "It's in the freezer now.", "", 0}, {"Give them a makeover lesson", 5, 1, 0, 0, "Bonding! They look great.", "", 0}}},
    {"{X} asks if you've been ghosting them.", true, {{"Be honest", 3, 0, 0, 1, "You talked it out. Feels better.", "", 0}, {"Make an excuse", -1, 0, 0, 0, "\"My phone was broken.\"", "{X} saw you online during the 'broken phone' era.", -4}, {"Keep ghosting", -6, 0, 0, 0, "...", "", 0}}},
    {"You get a scholarship/bonus offer - but you'd have to move.", false, {{"Take it", 0, 1, 500, 1, "Big moves! +$500", "", 0}, {"Stay for your people", 3, 0, 0, 0, "Home is where your heart is.", "", 0}, {"Negotiate to stay remote", 0, 1, 250, 0, "Best of both worlds. +$250", "", 0}}},
    {"{X} brings their annoying cousin to every hangout.", true, {{"Include the cousin", 3, -1, 0, 1, "The cousin is... growing on you.", "The cousin is now your friend too!", 2}, {"Say something", -3, 0, 0, 0, "Awkward, but honest.", "", 0}, {"Stop going to hangouts", -4, -1, 0, 0, "FOMO, but peace.", "", 0}}},
    {"A stray dog follows you home.", true, {{"Keep it!", 0, 1, -30, 0, "New best friend! -$30 for supplies", "Your stray is now the neighborhood's favorite dog.", 0}, {"Find its owner", 0, 1, 0, 2, "The owner cried and gave you $50!", "", 0}, {"Give it to a shelter", 0, 0, 0, 1, "It'll find a good home.", "", 0}}},
    {"{X} wants you to lie to their parents/partner for them.", true, {{"Cover for them", 5, 0, 0, 0, "Your poker face is elite.", "The lie unraveled. Now you're both in trouble.", -3}, {"Refuse", -4, 0, 0, 1, "Not getting involved.", "", 0}, {"Tell the truth", -9, 0, 0, 0, "You snitched. Oof.", "{X} isn't talking to you.", -5}}},
    {"You failed a test/presentation.", true, {{"Ask for a redo", 0, 0, 0, 1, "Your teacher/boss said yes!", "You aced the redo!", 0}, {"Cry about it", 0, -1, 0, 0, "Let it out.", "", 0}, {"Blame the Wi-Fi", 0, 0, 0, -1, "Classic excuse.", "", 0}}},
    {"{X} gets a puppy and asks you to puppy-sit.", true, {{"Say yes!", 6, 1, 0, 0, "The puppy chewed your shoe. Worth it.", "", 0}, {"You're allergic", -1, 0, 0, 0, "Achoo. Rain check.", "", 0}, {"Charge them $30", 0, 1, 30, 0, "Business is booming. +$30", "", 0}}},
    {"Your favorite influencer is doing a meet & greet.", true, {{"Line up for hours", 0, 1, 0, 0, "Got a selfie! Worth every minute.", "", 0}, {"Skip it", 0, -1, 0, 0, "You'll watch the vlog instead.", "", 0}, {"Sneak to the front", 0, 1, 0, -1, "You got a hug! People are mad.", "", 0}}},
    {"{X} wants to have a sleepover at your place.", true, {{"Host it!", 7, 1, -20, 0, "Pizza, face masks, gossip till 4 AM.", "", 0}, {"Your place is a mess", -2, 0, 0, 0, "Maybe next week.", "", 0}, {"Make it a movie marathon", 5, 1, -10, 0, "Seven movies. Zero sleep.", "", 0}}},
    {"Someone is wearing a fake designer bag... and it's {X}.", false, {{"Say nothing", 2, 0, 0, 1, "Kindness wins.", "", 0}, {"Ask where they got it", -2, 1, 0, 0, "\"Oh, it's... vintage.\" Sure.", "", 0}, {"Expose it", -8, 1, 0, -2, "Brutal. The whole group gasped.", "{X} is telling everyone YOU wear fakes.", -4}}},
    {"{X} says they're moving away next month.", true, {{"Make every day count", 8, -1, 0, 0, "You planned a bucket list together.", "{X} visits and says they miss you SO much.", 4}, {"Pretend you're fine", 0, -1, 0, 0, "You cried in the shower.", "", 0}, {"Help them pack", 5, 0, 0, 0, "Boxes, tears, and old photos.", "", 0}}},
    {"You win a free makeover!", true, {{"Go glam", 0, 1, 0, 1, "Full beat. You're unrecognizable (in a good way).", "", 0}, {"Natural look", 0, 1, 0, 0, "Clean girl aesthetic achieved.", "", 0}, {"Give it to {X}", 6, 1, 0, 1, "{X} looks stunning. They owe you.", "", 0}}},
    {"{X} keeps asking to borrow your charger.", true, {{"Lend it again", 2, 0, 0, 0, "Your charger is basically theirs now.", "{X} bought you a new charger as thanks!", 3}, {"Say no", -2, 0, 0, 0, "Charge your own phone!", "", 0}, {"Buy them their own", 4, 1, -15, 0, "Problem solved. -$15", "", 0}}},
    {"A neighbor complains your music is too loud.", true, {{"Turn it down", 0, 0, 0, 1, "Peace restored.", "", 0}, {"Invite them in", 0, 1, 0, 1, "Now they're dancing too!", "Your neighbor brought cookies to say thanks.", 0}, {"Turn it UP", 0, 1, 0, -2, "Party mode.", "HOA letter incoming...", 0}}},
    {"{X} wants to start a TikTak dance trend with you.", true, {{"Learn it!", 5, 1, 0, 1, "You nailed it. 50K views!", "", 0}, {"Two left feet", -1, 0, 0, 0, "Maybe you'll film instead.", "", 0}, {"Make your own trend", 3, 1, 0, 2, "Your trend went VIRAL.", "Strangers are doing your dance at the mall!", 0}}},
};
constexpr int NSC = sizeof SC / sizeof SC[0];
struct Pop { const char* text; int money, mood, luck; };
const Pop POPS[] = {
    {"A bird pooped on your shoulder. Good luck, apparently!", 0, 0, 1},
    {"You found your old Tamagotchi. It's... still alive?!", 0, 1, 0},
    {"Free samples at the mall! You ate 9 of them.", 0, 1, 0},
    {"Your celebrity crush liked your comment!!", 0, 1, 0},
    {"You tripped in public. Nobody saw. (Somebody saw.)", 0, -1, 0},
    {"Ice cream truck! You chased it down the block.", -3, 1, 0},
    {"Your horoscope says today is YOUR day.", 0, 1, 0},
    {"You got a parking ticket. Ugh.", -35, -1, 0},
    {"A stranger complimented your outfit!", 0, 1, 0},
    {"Your favorite song came on the radio!", 0, 1, 0},
    {"You found $20 in the couch cushions!", 20, 1, 0},
    {"Spam call #47 today. You answered in an accent.", 0, 1, 0},
    {"The Wi-Fi went down for an hour. Chaos.", 0, -1, 0},
    {"Someone left cookies on your doorstep!", 0, 1, 0},
    {"You stubbed your toe on the coffee table.", 0, -1, 0},
    {"Your plant is thriving. You're a plant mom now.", 0, 1, 0},
    {"A raccoon got into the trash. Again.", 0, -1, 0},
    {"You won a free drink at the coffee shop!", 0, 1, 0},
    {"Your hair is having the BEST day.", 0, 1, 0},
    {"You sent a text to the wrong person. Mortifying.", 0, -1, 0},
    {"Rainbow outside! Everyone's posting it.", 0, 1, 0},
    {"Your old friend from 2nd grade found you online!", 0, 1, 0},
    {"You burnt the toast. The smoke alarm agrees.", 0, -1, 0},
    {"The mail came: it's a fashion catalog!", 0, 1, 0},
    {"You got a refund check you forgot about. +$45", 45, 1, 0},
    {"A neighbor's cat adopted you for the afternoon.", 0, 1, 0},
    {"You finally finished that show. Now you're empty inside.", 0, -1, 0},
    {"Someone stole your parking spot. The audacity.", 0, -1, 0},
    {"Your nails look PERFECT today.", 0, 1, 0},
    {"You found a four-leaf clover!", 0, 1, 1},
    {"Lunch was free today - someone's birthday at work/school!", 0, 1, 0},
    {"You fell asleep and missed the group call.", 0, -1, 0},
    {"A street musician played your request!", -2, 1, 0},
    {"You heard a rumor about the new neighbors...", 0, 0, 0},
    {"Your selfie got 200 likes!", 0, 1, 0},
    {"Coupon code worked! You saved $15.", 15, 1, 0},
    {"A pigeon stole your fries.", 0, -1, 0},
    {"You had a dream you were famous. It felt SO real.", 0, 1, 0},
    {"Your favorite store restocked your size!", 0, 1, 0},
    {"You stepped in a puddle with new shoes.", 0, -1, 0},
    {"Someone in the group chat finally said what everyone's thinking.", 0, 1, 0},
    {"You found your lost earring!", 0, 1, 0},
    {"The vending machine gave you TWO snacks!", 0, 1, 0},
    {"Your crush walked past and smelled amazing.", 0, 1, 0},
    {"Power nap: successful. You feel brand new.", 0, 1, 0},
    {"Someone left a sweet note on your locker/desk.", 0, 1, 0},
    {"You're on a 7-day streak with your bestie!", 0, 1, 0},
    {"Your phone screen cracked. Noooo.", -60, -1, 0},
    {"A kid on the street said you look like a princess!", 0, 1, 0},
    {"Sunset was GORGEOUS tonight. You took 40 photos.", 0, 1, 0},
};
constexpr int NPOP = sizeof POPS / sizeof POPS[0];
struct Later { int day, scene, opt, who; };
std::vector<Later> later;
int cur = -1, curWho = -1; bool want = false;

std::string fill(const char* s, int who) {
    std::string r = s; const std::string n = who >= 0 && who < (int)G.sims.size() ? G.sims[who].name.substr(0, G.sims[who].name.find(' ')) : "Someone";
    size_t p; while ((p = r.find("{X}")) != std::string::npos) r.replace(p, 3, n); return r;
}
int pickPerson() {
    std::vector<int> v; for (int i = 0; i < (int)G.sims.size(); i++) if (i != G.mc && rel(G.mc, i).met && G.sims[i].stage >= ST_CHILD && std::abs((int)G.sims[i].stage - (int)MC().stage) <= 2) v.push_back(i);
    return v.empty() ? -1 : v[irand((int)v.size())];
}
}

bool choiceWanted() { return want; }
LxMenu choiceMenu() {
    want = false; LxMenu m; if (cur < 0) return m;
    m.push_back({fill(SC[cur].prompt, curWho), 21999});
    for (int k = 0; k < 3; k++) m.push_back({fill(SC[cur].o[k].label, curWho), 21001 + k});
    return m;
}
LxMenu choiceOption(int id) {
    if (id < 21001 || id > 21003 || cur < 0) return {};
    const Opt& o = SC[cur].o[id - 21001]; Sim& me = MC();
    if (curWho >= 0) changeRel(G.mc, curWho, o.rel, 0);
    if (o.money) { if (o.money < 0 && G.money < -o.money) {} else G.money += o.money; }
    if (o.rep) lxRepAdd(me.stage <= ST_TEEN ? CI_SCHOLARS : CI_PARTY, o.rep * 2, "a choice you made");
    if (o.mood > 0) addMoodlet(me, "Did It My Way", EM_CONFIDENT, 1, 360); else if (o.mood < 0) addMoodlet(me, "Bothered", EM_SAD, 1, 360);
    emotePlay(G.mc, o.mood > 0 ? EMO_HAPPY : o.mood < 0 ? EMO_SAD : o.rel < -5 ? EMO_SHOCK : EMO_IDEA);
    toast(fill(o.now, curWho));
    if (o.later[0] && irand(100) < 70) later.push_back({(int)G.day + 2 + irand(4), cur, id - 21001, curWho});
    cur = -1; return {};
}
void choicePopup();
void choiceHourly(int hour) {
    if (gsOn(GS_POPUPS) && hour >= 8 && hour <= 22 && irand(100) < 7) choicePopup();
    if (!want && cur >= 0 && !menuStillOpen()) cur = -1;   // dismissed with Back: that drama passes
    if (!gsOn(GS_CHOICES)) return;
    if (want || cur >= 0 || hour < 9 || hour > 21 || irand(100) >= l3PopPct(7)) return;
    const bool kid = MC().stage <= ST_PRETEEN;
    for (int tries = 0; tries < 6; tries++) { const int s = irand(NSC); if (kid && !SC[s].kidOk) continue; const int w = pickPerson(); if (w < 0) return; cur = s; curWho = w; want = true; return; }
}
void choicePopup() {
    const Pop& p = POPS[irand(NPOP)]; Sim& me = MC();
    if (p.money) G.money = p.money > 0 ? G.money + p.money : (G.money > -p.money ? G.money + p.money : 0);
    if (p.mood > 0) addMoodlet(me, "Little Win", EM_HAPPY, 1, 180); else if (p.mood < 0) addMoodlet(me, "Little Annoyance", EM_UNCOMFORTABLE, 1, 180);
    if (p.luck) addMoodlet(me, "Feeling Lucky", EM_CONFIDENT, 1, 720);
    toast(p.text);
}
void choiceDaily() {
    for (size_t i = 0; i < later.size();) {
        const Later& L = later[i];
        if (L.day <= (int)G.day) { const Opt& o = SC[L.scene].o[L.opt]; if (L.who < (int)G.sims.size()) { if (o.laterRel) changeRel(G.mc, L.who, o.laterRel, 0); toast(fill(o.later, L.who)); } later.erase(later.begin() + i); }
        else i++;
    }
}

void choiceSaveTo(const std::string& p) { FILE* f = fopen(p.c_str(), "wb"); if (!f) return; uint32_t n = (uint32_t)later.size(); fwrite(&n, 4, 1, f); if (n) fwrite(later.data(), sizeof(Later), n, f); fclose(f); }
void choiceLoadFrom(const std::string& p) { later.clear(); FILE* f = fopen(p.c_str(), "rb"); if (!f) return; uint32_t n = 0; if (fread(&n, 4, 1, f) == 1 && n < 500) { later.resize(n); if (n && fread(later.data(), sizeof(Later), n, f) != n) later.clear(); } fclose(f);
    for (size_t i = 0; i < later.size();) { if (later[i].scene < 0 || later[i].scene >= NSC || later[i].opt < 0 || later[i].opt > 2) later.erase(later.begin() + i); else i++; } }
