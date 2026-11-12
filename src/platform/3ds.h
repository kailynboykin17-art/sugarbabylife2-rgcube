// Sugarbaby Life 2 - RG Cube / Android port.
// Stand-in for libctru: the same names the game uses, implemented on SDL2 in platform.cpp.
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdlib>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
typedef int8_t s8; typedef int16_t s16; typedef int32_t s32; typedef int64_t s64; typedef s32 Result;
struct touchPosition { u16 px, py; };
struct circlePosition { s16 dx, dy; };
enum { KEY_A=1, KEY_B=2, KEY_SELECT=4, KEY_START=8, KEY_DRIGHT=16, KEY_DLEFT=32, KEY_DUP=64, KEY_DDOWN=128, KEY_R=256, KEY_L=512, KEY_X=1024, KEY_Y=2048,
 KEY_ZL=1<<14, KEY_ZR=1<<15, KEY_TOUCH=1<<20, KEY_CSTICK_RIGHT=1<<24, KEY_CSTICK_LEFT=1<<25, KEY_CSTICK_UP=1<<26, KEY_CSTICK_DOWN=1<<27,
 KEY_CPAD_RIGHT=1<<28, KEY_CPAD_LEFT=1<<29, KEY_CPAD_UP=1<<30, KEY_CPAD_DOWN=(int)(1u<<31),
 KEY_UP=KEY_DUP|KEY_CPAD_UP, KEY_DOWN=KEY_DDOWN|KEY_CPAD_DOWN, KEY_LEFT=KEY_DLEFT|KEY_CPAD_LEFT, KEY_RIGHT=KEY_DRIGHT|KEY_CPAD_RIGHT };
void hidScanInput(); u32 hidKeysDown(); u32 hidKeysHeld(); void hidTouchRead(touchPosition*); void hidCircleRead(circlePosition*);
bool aptMainLoop(); u64 osGetTime(); void gfxInitDefault(); void gfxExit();
inline Result romfsInit(){return 0;} inline Result romfsExit(){return 0;}
enum gfxScreen_t { GFX_TOP, GFX_BOTTOM }; enum gfx3dSide_t { GFX_LEFT, GFX_RIGHT };

// ---- software keyboard (shows Android's keyboard over the game)
struct SwkbdState { int type; int maxLen; char hint[96]; char initial[512]; };
enum { SWKBD_TYPE_NORMAL=0, SWKBD_TYPE_WESTERN=1, SWKBD_TYPE_NUMPAD=2, SWKBD_BUTTON_NONE=-1, SWKBD_BUTTON_LEFT=0, SWKBD_BUTTON_MIDDLE=1, SWKBD_BUTTON_RIGHT=2,
 SWKBD_BUTTON_CONFIRM=2, SWKBD_DARKEN_TOP_SCREEN=1, SWKBD_VALID_ANYTHING=0, SWKBD_VALID_NOTEMPTY_NOTBLANK=0, SWKBD_PREDICTIVE_INPUT=4 };
typedef int SwkbdButton;
void swkbdInit(SwkbdState*, int type, int buttons, int maxLen);
void swkbdSetHintText(SwkbdState*, const char*);
void swkbdSetInitialText(SwkbdState*, const char*);
inline void swkbdSetFeatures(SwkbdState*, u32){}
SwkbdButton swkbdInputText(SwkbdState*, char* buf, size_t size);

// ---- sound (mixed by SDL audio)
struct ndspWaveBuf { void* data_vaddr; s16* data_pcm16; u32 nsamples; bool looping; volatile int status; u32 pos_; };
enum { NDSP_WBUF_FREE=0, NDSP_WBUF_QUEUED=1, NDSP_WBUF_PLAYING=2, NDSP_WBUF_DONE=3, NDSP_OUTPUT_STEREO=1, NDSP_INTERP_LINEAR=1, NDSP_FORMAT_MONO_PCM16=5 };
Result ndspInit(); void ndspExit(); inline void ndspSetOutputMode(int){}
void ndspChnReset(int); inline void ndspChnSetInterp(int,int){} void ndspChnSetRate(int,float); inline void ndspChnSetFormat(int,int){}
void ndspChnSetMix(int, float*); void ndspChnWaveBufAdd(int, ndspWaveBuf*); void ndspChnWaveBufClear(int);
inline void* linearAlloc(size_t n){ return calloc(1,n);} inline void linearFree(void* p){ free(p);} inline Result DSP_FlushDataCache(const void*, u32){return 0;}
#define R_FAILED(r) ((r)<0)
#define R_SUCCEEDED(r) ((r)>=0)
void svcSleepThread(long long ns);
