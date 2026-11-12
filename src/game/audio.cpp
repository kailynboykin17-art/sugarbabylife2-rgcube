#include "audio.hpp"
#include <3ds.h>
#include <cmath>
#include <cstdio>
#include <cstring>

const char* TRACK_NAMES[NUM_TRACKS] = {"Glitter Morning", "Party Girl", "Lullaby", "Spooky Glam", "Snow Sparkle", "Beach Daze"};

static const int RATE = 22050;
static const int BUF = 1024;
static bool ok = false;
static ndspWaveBuf wb[3];
static s16* mbuf = nullptr;
static ndspWaveBuf sw, swr[4];   // sw unused; swr = sound-effect ring (never rewrite a buffer the DSP still owns)
static s16* sbuf = nullptr;
static const int SMAX = RATE / 2;

struct Track { int bpm; const int8_t* mel; int melLen; int melSteps; const int8_t* bass; int bassLen; int bassSteps; bool tri; };

static const int8_t M0[] = {72, 76, 79, 76, 77, 74, 71, 74, 72, 76, 79, 84, 83, 79, 74, 0, 76, 77, 79, 81, 79, 77, 76, 74, 72, 74, 76, 72, 71, 67, 72, 0};
static const int8_t B0[] = {48, 48, 53, 53, 55, 55, 48, 48};
static const int8_t M1[] = {69, 69, 72, 69, 74, 72, 69, 67, 69, 69, 72, 76, 74, 72, 69, 0, 64, 67, 69, 72, 74, 76, 74, 72, 69, 67, 64, 67, 69, 0, 69, 0};
static const int8_t B1[] = {45, 57, 45, 57, 41, 53, 41, 53, 43, 55, 43, 55, 40, 52, 40, 52};
static const int8_t M2[] = {67, 72, 71, 69, 67, 64, 65, 67, 69, 67, 65, 64, 62, 64, 60, 0};
static const int8_t B2[] = {48, 43, 41, 43, 45, 43, 41, 48};
static const int8_t M3[] = {62, 65, 69, 68, 67, 66, 65, 0, 62, 65, 69, 74, 73, 72, 71, 0};
static const int8_t B3[] = {38, 38, 41, 41, 37, 37, 38, 0};
static const int8_t M4[] = {76, 79, 81, 79, 76, 74, 76, 0, 74, 77, 79, 77, 74, 72, 74, 0, 72, 76, 79, 84, 83, 81, 79, 0, 77, 76, 74, 72, 71, 72, 72, 0};
static const int8_t B4[] = {48, 55, 48, 55, 50, 57, 50, 57, 53, 60, 55, 62, 48, 55, 48, 55};
static const int8_t M5[] = {67, 69, 71, 74, 71, 69, 67, 64, 62, 64, 67, 69, 67, 64, 62, 0, 67, 69, 71, 74, 76, 74, 71, 69, 67, 69, 71, 67, 64, 62, 67, 0};
static const int8_t B5[] = {43, 50, 43, 50, 48, 55, 48, 55, 40, 47, 40, 47, 38, 45, 43, 50};
#define N(a) (int)(sizeof(a) / sizeof(a[0]))
static const Track TRACKS[NUM_TRACKS] = {
    {120, M0, N(M0), 2, B0, N(B0), 8, false}, {132, M1, N(M1), 2, B1, N(B1), 4, false},
    {70, M2, N(M2), 4, B2, N(B2), 8, true},   {100, M3, N(M3), 2, B3, N(B3), 8, true},
    {116, M4, N(M4), 2, B4, N(B4), 4, false}, {96, M5, N(M5), 2, B5, N(B5), 4, true},
};

static int track = -1;
static int melIdx = 0, bassIdx = 0, melLeft = 0, bassLeft = 0, melTotal = 1;
static int melNote = 0, bassNote = 0;
static float ph1 = 0, ph2 = 0;

static float mtof(int n) { return 440.f * std::pow(2.f, (n - 69) / 12.f); }
static float triw(float p) { return p < 0.5f ? 4 * p - 1 : 3 - 4 * p; }

// ---- streamed songs: raw s16 mono 22050 Hz files in romfs:/music or sdmc:/LifeSim/music (track ids 100+k)
#include <dirent.h>
#include <string>
#include <vector>
#include <algorithm>
static std::vector<std::string> songPaths, songNames;
static FILE* songF = nullptr;
static void songScan() {
    if (!songPaths.empty()) return;
    const char* dirs[2] = {"romfs:/music", "sdmc:/LifeSim/music"};
    for (const char* d : dirs) {
        DIR* dp = opendir(d); if (!dp) continue;
        while (dirent* e = readdir(dp)) {
            std::string n = e->d_name; if (n.size() < 5 || n.substr(n.size() - 4) != ".raw") continue;
            std::string nm = n.substr(0, n.size() - 4); for (auto& c : nm) if (c == '_') c = ' ';
            while (nm.find("  ") != std::string::npos) nm.erase(nm.find("  "), 1);
            if (std::find(songNames.begin(), songNames.end(), nm) != songNames.end()) continue;
            songPaths.push_back(std::string(d) + "/" + n); songNames.push_back(nm);
        }
        closedir(dp);
    }
}
int songCount() { songScan(); return (int)songNames.size(); }
const char* songName(int k) { songScan(); return k >= 0 && k < (int)songNames.size() ? songNames[k].c_str() : ""; }
static void fill(s16* out, int n);
static void fillSong(s16* out, int n) {
    size_t got = fread(out, sizeof(s16), n, songF);
    if ((int)got < n) { fseek(songF, 0, SEEK_SET); size_t more = fread(out + got, sizeof(s16), n - got, songF); got += more; }
    for (int i = (int)got; i < n; i++) out[i] = 0;
    for (int i = 0; i < n; i++) out[i] = (s16)(out[i] * 3 / 5);
}
static void fill(s16* out, int n) {
    if (songF) { fillSong(out, n); return; }
    for (int i = 0; i < n; i++) {
        float v = 0;
        if (track >= 0) {
            const Track& tr = TRACKS[track];
            int spb = RATE * 60 / (tr.bpm * 4);
            if (melLeft <= 0) { melNote = tr.mel[melIdx]; melIdx = (melIdx + 1) % tr.melLen; melLeft = melTotal = spb * tr.melSteps; }
            if (bassLeft <= 0) { bassNote = tr.bass[bassIdx]; bassIdx = (bassIdx + 1) % tr.bassLen; bassLeft = spb * tr.bassSteps; }
            melLeft--; bassLeft--;
            if (melNote > 0) {
                ph1 += mtof(melNote) / RATE; if (ph1 >= 1) ph1 -= 1;
                float env = 1.f - 0.75f * (1.f - (float)melLeft / melTotal);
                float w = tr.tri ? triw(ph1) : (ph1 < 0.5f ? 0.6f : -0.6f);
                v += w * 0.2f * env;
            }
            if (bassNote > 0) {
                ph2 += mtof(bassNote) / RATE; if (ph2 >= 1) ph2 -= 1;
                v += triw(ph2) * 0.22f;
            }
        }
        if (v > 1) v = 1; if (v < -1) v = -1;
        out[i] = (s16)(v * 24000);
    }
}

bool audioInit() {
    if (R_FAILED(ndspInit())) return false;
    ok = true;
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    float mix[12];
    memset(mix, 0, sizeof mix);
    mix[0] = mix[1] = 0.8f;
    for (int ch = 0; ch < 2; ch++) {
        ndspChnReset(ch);
        ndspChnSetInterp(ch, NDSP_INTERP_LINEAR);
        ndspChnSetRate(ch, RATE);
        ndspChnSetFormat(ch, NDSP_FORMAT_MONO_PCM16);
        ndspChnSetMix(ch, mix);
    }
    mbuf = (s16*)linearAlloc(BUF * 3 * sizeof(s16));
    sbuf = (s16*)linearAlloc(SMAX * 4 * sizeof(s16));
    memset(wb, 0, sizeof wb);
    memset(&sw, 0, sizeof sw); memset(swr, 0, sizeof swr);
    for (int i = 0; i < 3; i++) { wb[i].data_pcm16 = mbuf + i * BUF; wb[i].nsamples = BUF; wb[i].status = NDSP_WBUF_DONE; }
    return true;
}

void audioExit() {
    if (songF) { fclose(songF); songF = nullptr; }
    if (!ok) return;
    ndspChnReset(0); ndspChnReset(1);
    for (int i = 0; i < 3; i++) for (int w = 0; w < 50 && wb[i].status == NDSP_WBUF_PLAYING; w++) svcSleepThread(2000000);
    ndspChnWaveBufClear(0);
    ndspChnWaveBufClear(1);
    ndspExit();
    if (mbuf) linearFree(mbuf);
    if (sbuf) linearFree(sbuf);
    ok = false;
}

void audioUpdate() {
    if (!ok) return;
    for (int i = 0; i < 3; i++) {
        if (wb[i].status == NDSP_WBUF_DONE || wb[i].status == NDSP_WBUF_FREE) {
            fill(wb[i].data_pcm16, BUF);
            DSP_FlushDataCache(wb[i].data_pcm16, BUF * sizeof(s16));
            ndspChnWaveBufAdd(0, &wb[i]);
        }
    }
}

void musicPlay(int t) {
    if (songF) { fclose(songF); songF = nullptr; }
    if (t >= 100) {
        songScan(); const int k = t - 100;
        if (k < (int)songPaths.size()) songF = fopen(songPaths[k].c_str(), "rb");
        if (songF) { track = t; return; }
        t = 0;
    }
    track = t;
    melIdx = bassIdx = melLeft = bassLeft = 0;
}
int musicTrack() { return track; }
bool audioIsPlaying() { return track >= 0 || songF; }
bool audioOk() { return ok; }

void sfx(float f0, float f1, int ms) {
    if (!ok || !sbuf) return;
    static uint32_t lastMs = 0; const uint32_t now = (uint32_t)osGetTime();
    if (now - lastMs < 35) return;   // rapid tapping: one click per ~2 frames is plenty
    int slot = -1;
    for (int i = 0; i < 4; i++) if (swr[i].status == NDSP_WBUF_FREE || swr[i].status == NDSP_WBUF_DONE) { slot = i; break; }
    if (slot < 0) return;   // all 4 still playing - skip this sound instead of corrupting the DSP queue
    lastMs = now;
    int n = RATE * ms / 1000;
    if (n > SMAX) n = SMAX;
    s16* buf = sbuf + slot * SMAX;
    float ph = 0;
    for (int i = 0; i < n; i++) {
        float k = (float)i / n;
        float f = f0 + (f1 - f0) * k;
        ph += f / RATE; if (ph >= 1) ph -= 1;
        float env = 1.f - k;
        buf[i] = (s16)((ph < 0.5f ? 1 : -1) * 9000 * env);
    }
    DSP_FlushDataCache(buf, n * sizeof(s16));
    ndspWaveBuf& w = swr[slot];
    memset(&w, 0, sizeof w);
    w.data_pcm16 = buf;
    w.nsamples = n;
    ndspChnWaveBufAdd(1, &w);
}
bool g_tapSound = false;   // tap clicks are OFF by default (Grow Up app > Tap Sounds)
void sfxTap() { if (g_tapSound) sfx(1200, 1600, 40); }
void sfxYay() { sfx(800, 1800, 220); }
void sfxNo() { sfx(400, 180, 200); }
