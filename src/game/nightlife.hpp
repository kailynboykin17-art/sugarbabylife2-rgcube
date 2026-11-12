#pragma once
#include "common.hpp"
#include <string>
#include <vector>
// Neon Strip venues (Glow Bowl, Karaoke Lounge, Paint & Sip, Dance Studio, Night Market, Serenity Center)
// plus the opt-in, adults-only "After Hours" system: made-up party substances with come-up / peak /
// comedown / hangover moodlets, tolerance & addiction, withdrawal, rehab, and dealing with a skill,
// client list, heat (arrests) and a crew. All names are fictional; it is game mechanics only.
std::vector<std::pair<std::string, int>> nightSpotOptions(int spotKind);   // DA ids
bool nightAction(int act);
void nightDrawRoom(int room, float t, bool night);
void nightHourly(int hour);
void nightDaily();
void nightSaveTo(const std::string& path);
void nightLoadFrom(const std::string& path);
bool nlAfterHours();            // After Hours (fictional substances) switch
void nlSetAfterHours(bool on);
