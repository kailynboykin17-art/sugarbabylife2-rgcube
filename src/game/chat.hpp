#pragma once
#include "common.hpp"
#include <string>
#include <vector>
// "Alive" texting (chat.cpp): branching conversations that react to what's going on.
// A live conversation is stored on the thread as prompt = CHAT_BASE + topic * 4 + stage.
constexpr int CHAT_BASE = 1000;
struct ChatThread;
struct ChatOpt { std::string label; int tone; int style; };   // style: button colour
inline bool chatLive(int prompt) { return prompt >= CHAT_BASE && prompt < CHAT_BASE + 400; }
std::vector<ChatOpt> chatStarters(int sim);         // things you can text them about right now
void chatStart(int sim, int starter);               // you open a conversation
std::vector<ChatOpt> chatChoices(const ChatThread& th);
void chatChoose(ChatThread& th, int i);
void chatTyped(int sim, const std::string& msg);    // you typed your own message
bool chatTyping(int sim);                           // they're typing...
bool chatWaiting(int sim);                          // your message is delivered, not answered yet
void chatTick(float dt);
void chatHourly(int hour);
void chatNews(int sim, int lifeKind, int about);    // a townie texts you their big news (LifeMem kind)
