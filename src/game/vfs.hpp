#pragma once
#include <cstdio>
#include <string>
// Save pack: while vfsBeginSave()/vfsEndSave() is active, every "wb" fopen goes to memory and the whole set is
// written as ONE file (<slot>.pack). While a pack is loaded, "rb" fopens of packed paths read from memory.
// Paths that aren't in the pack fall back to the real SD file (old saves with separate sidecars still load).
FILE* vfopen(const char* path, const char* mode);
void vfsBeginSave();
bool vfsEndSave(const std::string& packPath, const std::string& base);   // writes the pack, deletes old sidecar files
void vfsBeginLoad(const std::string& packPath, const std::string& base);
void vfsEndLoad();
