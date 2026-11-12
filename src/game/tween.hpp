#pragma once
// Tweening: eased, time-based transforms layered on top of the sprite poses (hops, spins, kneels,
// leans, shakes, swings, pop-ins, breathing). Everything is procedural - no new sprite frames.
enum TweenKind { TW_HOP, TW_SPIN, TW_KNEEL, TW_LEANL, TW_LEANR, TW_SHAKE, TW_CHEER, TW_SWING, TW_SWAY, TW_POP, TW_BOW, TW_WAVE, TW_CLAP, TW_ARMSUP, TW_STOMP, TW_COUNT };
void tweenPlay(int sim, int kind, float dur = 0.8f, int loops = 1);
void tweenLoop(int sim, int kind);   // keep playing while this is called each frame (dancer, waiter...)
struct TweenX { float dx = 0, dy = 0, rot = 0, sx = 1, sy = 1; float armL = 0, armR = 0, legL = 0, legR = 0, head = 0; bool any = false; };
extern bool g_rigOn; extern float g_rig[5];   // oc.cpp cut-out rig inputs
TweenX tweenGet(int sim, float t);
void tweenTick(float dt);
float easeOutBack(float x); float easeInOutSine(float x); float easeOutElastic(float x); float easeOutBounce(float x);
