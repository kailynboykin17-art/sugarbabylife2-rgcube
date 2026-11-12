// Sugarbaby Life 2 - RG Cube / Android port: citro2d/citro3d stand-in, implemented on SDL2 in platform.cpp.
#pragma once
#include "3ds.h"
#include <string>
struct TexImpl;
struct C3D_Tex { int w, h; void* data; u32 size; TexImpl* impl; };
Result GSPGPU_FlushDataCache(const void*, u32);
extern u32* gpuCmdBuf; extern u32 gpuCmdBufOffset, gpuCmdBufSize;
struct C3D_RenderTarget { TexImpl* impl; int screen; };
struct Tex3DS_SubTexture { u16 width, height; float left, top, right, bottom; };
struct C2D_Image { C3D_Tex* tex; const Tex3DS_SubTexture* subtex; };
struct C2D_ImageTint { u32 c; };
struct C2D_TextBuf_s { int x; }; typedef C2D_TextBuf_s* C2D_TextBuf;
struct C2D_Text { std::string s; };
struct C3D_Mtx { float m[6]; };
enum { GPU_RGBA8, GPU_NEAREST, GPU_LINEAR, GPU_TEXFACE_2D, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ONE, GPU_ZERO, GPU_ALWAYS, GPU_GEQUAL, GPU_WRITE_COLOR, GPU_WRITE_ALL, C3D_FRAME_SYNCDRAW };
typedef int GPU_DEPTHBUF;
#define C3D_DEFAULT_CMDBUF_SIZE 0x40000
#define C2D_DEFAULT_MAX_OBJECTS 4096
enum { C2D_AlignLeft=0, C2D_AlignRight=1<<2, C2D_AlignCenter=2<<2, C2D_WithColor=1<<0, C2D_AtBaseline=1<<1 };
inline u32 C2D_Color32(u8 r, u8 g, u8 b, u8 a) { return r | (g<<8) | (b<<16) | ((u32)a<<24); }
inline bool C3D_Init(int){return true;} inline void C3D_Fini(){} inline bool C2D_Init(int){return true;} inline void C2D_Fini(){} inline void C2D_Prepare(){}
bool C3D_TexInit(C3D_Tex*, int, int, int); bool C3D_TexInitVRAM(C3D_Tex*, int, int, int);
void C3D_TexSetFilter(C3D_Tex*, int, int); void C3D_TexDelete(C3D_Tex*);
C3D_RenderTarget* C3D_RenderTargetCreateFromTex(C3D_Tex*, int, int, GPU_DEPTHBUF);
C3D_RenderTarget* C2D_CreateScreenTarget(int, int);
void C3D_FrameBegin(int); void C3D_FrameEnd(int);
void C3D_AlphaBlend(int,int,int,int,int,int); inline void C3D_DepthTest(bool,int,int){}
void C2D_TargetClear(C3D_RenderTarget*, u32); void C2D_SceneBegin(C3D_RenderTarget*); void C2D_Flush();
void C2D_ViewReset(); void C2D_ViewSave(C3D_Mtx*); void C2D_ViewRestore(const C3D_Mtx*); void C2D_ViewTranslate(float,float); void C2D_ViewScale(float,float); void C2D_ViewRotate(float);
bool C2D_DrawRectSolid(float,float,float,float,float,u32);
bool C2D_DrawRectangle(float,float,float,float,float,u32,u32,u32,u32);
bool C2D_DrawTriangle(float,float,u32,float,float,u32,float,float,u32,float);
bool C2D_DrawLine(float,float,u32,float,float,u32,float,float);
bool C2D_DrawCircleSolid(float,float,float,float,u32);
bool C2D_DrawEllipseSolid(float,float,float,float,float,u32);
bool C2D_DrawImageAt(C2D_Image, float, float, float, const C2D_ImageTint*, float, float);
inline void C2D_PlainImageTint(C2D_ImageTint* t, u32 c, float){ t->c=c; }
inline C2D_TextBuf C2D_TextBufNew(size_t){ return new C2D_TextBuf_s; } inline void C2D_TextBufDelete(C2D_TextBuf b){ delete b; } inline void C2D_TextBufClear(C2D_TextBuf){}
inline const char* C2D_TextParse(C2D_Text* t, C2D_TextBuf, const char* s){ t->s = s; return s + t->s.size(); }
inline void C2D_TextOptimize(C2D_Text*){}
void C2D_TextGetDimensions(const C2D_Text*, float, float, float*, float*);
void C2D_DrawText(const C2D_Text*, u32, float, float, float, float, float, ...);
