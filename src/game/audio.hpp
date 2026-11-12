#pragma once
// Tiny chiptune synth on the 3DS DSP (needs dspfirm.cdc dumped on the SD card;
// if it's missing the game still runs, just silently).
constexpr int NUM_TRACKS = 6;
extern const char* TRACK_NAMES[NUM_TRACKS];
bool audioInit();
void audioExit();
void audioUpdate();
void musicPlay(int track);    // -1 = stop
int musicTrack();
bool audioIsPlaying();
bool audioOk();
void sfx(float f0, float f1, int ms);
void sfxTap();
void sfxYay();
void sfxNo();

int songCount();                 // streamed songs found in romfs:/music + sdmc:/LifeSim/music
const char* songName(int k);     // musicPlay(100 + k) plays one
