#include "vfs.hpp"
#include <map>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cstdint>

namespace {
struct Mem { char* buf = nullptr; size_t len = 0; };
bool saving = false, loading = false;
std::map<std::string, Mem*> out;                    // path -> memstream (filled at fclose)
std::map<std::string, std::vector<char>> in;        // path -> packed bytes
const uint32_t MAGIC = 0x4B504253;                  // "SBPK"
}

FILE* vfopen(const char* path, const char* mode) {
    if (saving && mode[0] == 'w') {
        auto it = out.find(path);
        if (it != out.end()) { free(it->second->buf); delete it->second; out.erase(it); }
        Mem* m = new Mem; FILE* f = open_memstream(&m->buf, &m->len);
        if (!f) { delete m; return fopen(path, mode); }
        out[path] = m; return f;
    }
    if (loading && mode[0] == 'r') {
        auto it = in.find(path);
        if (it != in.end()) { if (it->second.empty()) return nullptr; return fmemopen(it->second.data(), it->second.size(), "rb"); }
    }
    return fopen(path, mode);
}

void vfsBeginSave() { saving = true; for (auto& e : out) { free(e.second->buf); delete e.second; } out.clear(); }

bool vfsEndSave(const std::string& pack, const std::string& base) {
    saving = false;
    const std::string tmp = pack + ".tmp";
    FILE* f = fopen(tmp.c_str(), "wb"); bool ok = f != nullptr;
    if (f) {
        uint32_t n = (uint32_t)out.size(); ok &= fwrite(&MAGIC, 4, 1, f) == 1; ok &= fwrite(&n, 4, 1, f) == 1;
        for (auto& e : out) {
            const std::string key = e.first.compare(0, base.size(), base) == 0 ? e.first.substr(base.size()) : e.first;
            uint32_t kl = (uint32_t)key.size(), dl = (uint32_t)e.second->len;
            ok &= fwrite(&kl, 4, 1, f) == 1; ok &= fwrite(key.data(), 1, kl, f) == kl; ok &= fwrite(&dl, 4, 1, f) == 1;
            if (dl) ok &= fwrite(e.second->buf, 1, dl, f) == dl;
        }
        ok &= fclose(f) == 0;
    }
    if (ok) {
        remove(pack.c_str()); ok = rename(tmp.c_str(), pack.c_str()) == 0;
        if (ok) for (auto& e : out) remove(e.first.c_str());   // tidy up the old one-file-per-system saves
    } else remove(tmp.c_str());
    if (!ok) for (auto& e : out) { FILE* g = fopen(e.first.c_str(), "wb"); if (g) { if (e.second->len) fwrite(e.second->buf, 1, e.second->len, g); fclose(g); } }   // fallback: loose files
    for (auto& e : out) { free(e.second->buf); delete e.second; } out.clear();
    return ok;
}

void vfsBeginLoad(const std::string& pack, const std::string& base) {
    in.clear(); loading = true;
    FILE* f = fopen(pack.c_str(), "rb"); if (!f) return;
    uint32_t m = 0, n = 0;
    if (fread(&m, 4, 1, f) == 1 && m == MAGIC && fread(&n, 4, 1, f) == 1 && n < 256) {
        for (uint32_t i = 0; i < n; i++) {
            uint32_t kl = 0, dl = 0; if (fread(&kl, 4, 1, f) != 1 || kl > 256) break;
            std::string key(kl, '\0'); if (fread(&key[0], 1, kl, f) != kl) break;
            if (fread(&dl, 4, 1, f) != 1 || dl > (8u << 20)) break;
            std::vector<char> d(dl); if (dl && fread(d.data(), 1, dl, f) != dl) break;
            in[base + key] = std::move(d);
        }
    }
    fclose(f);
}
void vfsEndLoad() { loading = false; in.clear(); }
