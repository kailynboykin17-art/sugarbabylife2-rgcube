#include "vfs.hpp"
#include "sevents.hpp"
#include "deeplife.hpp"
#include "lifeext.hpp"
#include "farm.hpp"
#include "levels.hpp"
#include "life3.hpp"
#include "spicy.hpp"
#include "audio.hpp"
#include "hotdate.hpp"
#include "nightlife.hpp"
#include "villa.hpp"
#include "clubs.hpp"
#include "challenge.hpp"
#include "lifestory.hpp"
#include "kpop.hpp"
#include <algorithm>
// Binary save file: sdmc:/3ds/DreamDollhouse/config/save.dat
#include "game.hpp"
#include "catalog.hpp"
#include "extras.hpp"
#include "sim.hpp"
#include "chat.hpp"
#include "family.hpp"
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <ctime>
#include <string>

static const uint32_t MAGIC = 0x44445348;   // "DDSH"
static const int IL_COUNT_SAVE = 9;
static const uint32_t VERSION = 51;   // 12 = Stick Figure Ver. (wardrobe, crime & drama)

struct W {
    FILE* f;
    void b(const void* p, size_t n) { fwrite(p, 1, n, f); }
    template <class T> void v(const T& x) { b(&x, sizeof x); }
    void s(const std::string& x) { uint16_t n = (uint16_t)x.size(); v(n); b(x.data(), n); }
};
struct Rd {
    FILE* f;
    bool ok = true;
    void b(void* p, size_t n) { if (fread(p, 1, n, f) != n) ok = false; }
    template <class T> void v(T& x) { b(&x, sizeof x); }
    void s(std::string& x) { uint16_t n = 0; v(n); if (n > 4000) { ok = false; return; } x.resize(n); if (n) b(&x[0], n); }
    uint32_t count(uint32_t max) { uint32_t n = 0; v(n); if (n > max) { ok = false; n = 0; } return n; }
};

static void wLook(W& w, const PersonDef& p) {
    w.v(p.age); w.v(p.guy); w.v(p.skin); w.v(p.hairCol); w.v(p.hairStyle);
    w.v(p.top); w.v(p.bottom); w.v(p.bt); w.v(p.topPattern); w.v(p.shoes); w.v(p.lips);
    w.v(p.f);
    w.v(p.topStyle); w.v(p.shoeStyle); w.v(p.hat); w.v(p.acc); w.v(p.accCol);
}
static void rLook(Rd& r, PersonDef& p, uint32_t ver, uint32_t seed) {
    r.v(p.age); r.v(p.guy); r.v(p.skin); r.v(p.hairCol); r.v(p.hairStyle);
    r.v(p.top); r.v(p.bottom); r.v(p.bt); r.v(p.topPattern); r.v(p.shoes); r.v(p.lips);
    if (ver >= 8) r.v(p.f);
    else p.f = randomFace(p.guy, seed);   // older saves: give everyone a unique face
    if (ver >= 12) { r.v(p.topStyle); r.v(p.shoeStyle); r.v(p.hat); r.v(p.acc); r.v(p.accCol); }
    if (p.topStyle >= NUM_TOPS) p.topStyle = 0;
    if (p.shoeStyle >= NUM_SHOES) p.shoeStyle = 0;
    if (p.hat >= NUM_HATS) p.hat = 0;
    if (p.bt >= NUM_BOTTOMS) p.bt = BT_PANTS;
    p.name = nullptr;
    if (p.skin >= NUM_SKINS) p.skin = 0;
    if (p.hairCol >= NUM_HAIRCOLS) p.hairCol = 0;
    if (p.hairStyle >= NUM_HAIRSTYLES) p.hairStyle = 0;
    p.f.eyes %= 12; p.f.eyeCol %= 10; p.f.brows %= 10; p.f.nose %= 10; p.f.lipShape %= 10;   // (f.shape holds SBL2 makeup bits)
    p.f.shadow %= 10; p.f.liner %= 10; p.f.blush %= 10; p.f.marks %= 10; p.f.glasses %= 10; p.f.pierce %= 10; p.f.beard %= 10;
    if (p.bt > BT_GOWN) p.bt = BT_PANTS;
}

static void mkdirs() {
    mkdir("sdmc:/3ds", 0777);
    mkdir(APP_DIR, 0777);
    mkdir(CFG_DIR, 0777);
}

// ---------------------------------------------------------------- save slots
// Slot 1 keeps the original save.dat (old saves just show up there); slots 2..5 are save2..5.dat.
// Each save keeps a .bak of the previous version and a tiny .inf header for the slot list.
int g_saveSlot = 0; uint32_t g_playSecs = 0; std::string g_slotName;
static std::string slotPath(int k) { if (k <= 0) return SAVE_PATH; char b[16]; snprintf(b, sizeof b, "/save%d.dat", k + 1); return std::string(CFG_DIR) + b; }
static bool fileExists(const std::string& p) { FILE* f = fopen(p.c_str(), "rb"); if (!f) return false; fclose(f); return true; }
bool loadGame(Game& out);

// ---- rotating daily backups: P.day1 (newest) .. P.day3, each with its sidecar pack
static bool copyFile(const std::string& a, const std::string& b) {
    FILE* in = fopen(a.c_str(), "rb"); if (!in) return false;
    FILE* out = fopen((b + ".tmp").c_str(), "wb"); if (!out) { fclose(in); return false; }
    static char buf[16384]; size_t n; bool ok = true;
    while ((n = fread(buf, 1, sizeof buf, in)) > 0) if (fwrite(buf, 1, n, out) != n) { ok = false; break; }
    fclose(in); if (fclose(out) != 0) ok = false;
    if (!ok) { remove((b + ".tmp").c_str()); return false; }
    remove(b.c_str()); return rename((b + ".tmp").c_str(), b.c_str()) == 0;
}
static void readBkInfo(const std::string& P, uint16_t d[3]) { d[0] = d[1] = d[2] = 0xFFFF; FILE* f = fopen((P + ".bkinfo").c_str(), "rb"); if (!f) return; if (fread(d, 2, 3, f) != 3) d[0] = d[1] = d[2] = 0xFFFF; fclose(f); }
static void rotateBackups(const std::string& P, int day) {
    uint16_t d[3]; readBkInfo(P, d);
    if (d[0] == (uint16_t)day) return;
    for (int k = 2; k >= 1; k--) {
        const std::string from = P + ".day" + std::to_string(k), to = P + ".day" + std::to_string(k + 1);
        if (fileExists(from)) { copyFile(from, to); copyFile(from + ".pack", to + ".pack"); d[k] = d[k - 1]; }
    }
    if (copyFile(P, P + ".day1")) { copyFile(P + ".pack", P + ".day1.pack"); d[0] = (uint16_t)day; }
    FILE* f = fopen((P + ".bkinfo").c_str(), "wb"); if (f) { fwrite(d, 2, 3, f); fclose(f); }
}
int backupDay(int k) { uint16_t d[3]; readBkInfo(slotPath(g_saveSlot), d); const std::string b = slotPath(g_saveSlot) + ".day" + std::to_string(k + 1); return (k >= 0 && k < 3 && d[k] != 0xFFFF && fileExists(b)) ? d[k] : -1; }
bool restoreBackup(int k, Game& out) {
    if (backupDay(k) < 0) return false;
    const std::string P = slotPath(g_saveSlot), b = P + ".day" + std::to_string(k + 1);
    if (!copyFile(b, P)) return false;
    copyFile(b + ".pack", P + ".pack");
    return loadGame(out);
}
bool slotUsed(int k) { return fileExists(slotPath(k)) || fileExists(slotPath(k) + ".bak"); }
bool saveExists() { for (int k = 0; k < NUM_SLOTS; k++) if (slotUsed(k)) return true; return false; }
static const uint32_t INF_MAGIC = 0x534C4F54;   // "SLOT"
static void writeInfo(int k, const Game& g, const std::string& name, uint32_t play, int64_t when) {
    FILE* f = fopen((slotPath(k) + ".inf").c_str(), "wb"); if (!f) return;
    W w{f}; w.v(INF_MAGIC); w.v(VERSION); w.s(name);
    const Sim& me = g.sims[std::max(0, std::min((int)g.sims.size() - 1, g.mc))];
    w.s(me.name); w.v(g.day); w.v(g.minute); w.v(play); w.v(when); w.v((uint8_t)me.stage); w.v(me.outfit);
    int hh = 0; for (const Sim& s : g.sims) hh += s.household; w.v((uint8_t)hh);
    wLook(w, me.look); fclose(f);
}
bool readSlotInfo(int k, SlotInfo& out) {
    out = SlotInfo();
    if (!slotUsed(k)) return false;
    out.used = true;
    FILE* f = fopen((slotPath(k) + ".inf").c_str(), "rb");
    if (f) {
        Rd r{f}; uint32_t m = 0, ver = 0; r.v(m); r.v(ver);
        if (m == INF_MAGIC && ver <= VERSION) {
            r.s(out.name); r.s(out.portrait.name); r.v(out.day); r.v(out.minute); r.v(out.playSecs); r.v(out.savedAt);
            uint8_t st = 0; r.v(st); out.portrait.stage = (Stage)(st % ST_COUNT); r.v(out.portrait.outfit); out.portrait.outfit %= NUM_OUTFITS;
            uint8_t hh = 0; r.v(hh); out.household = hh;
            rLook(r, out.portrait.look, ver, 7); fclose(f);
            if (r.ok) { out.mc = out.portrait.name; size_t sp = out.mc.rfind(' '); out.family = sp == std::string::npos ? out.mc : out.mc.substr(sp + 1); out.hasInfo = true; return true; }
        } else fclose(f);
    }
    // an older save without a header: peek inside once and write one
    Game tmp; int keep = g_saveSlot; uint32_t kp = g_playSecs; std::string kn = g_slotName;
    g_saveSlot = k;
    if (loadGame(tmp)) { writeInfo(k, tmp, "My Life", 0, 0); g_saveSlot = keep; g_playSecs = kp; g_slotName = kn; return readSlotInfo(k, out) || true; }
    g_saveSlot = keep; g_playSecs = kp; g_slotName = kn;
    out.name = "Damaged save"; return true;
}
bool deleteSlot(int k) {
    std::string p = slotPath(k);
    remove(p.c_str()); remove((p + ".inf").c_str()); remove((p + ".bak").c_str()); remove((p + ".tmp").c_str()); remove((p + ".pack").c_str());
    for (const char* e : {".kpop", ".chal", ".club", ".vila", ".nite", ".hdat", ".spcy", ".lxe", ".lxeg", ".lxed", ".lxey", ".deep", ".deepi", ".sev"}) remove((p + e).c_str());
    return !slotUsed(k);
}
bool renameSlot(int k, const std::string& name) {
    SlotInfo si; if (!readSlotInfo(k, si)) return false;
    Game tmp; int keep = g_saveSlot; uint32_t kp = g_playSecs; std::string kn = g_slotName; g_saveSlot = k;
    bool ok = loadGame(tmp); g_saveSlot = keep; g_playSecs = kp; g_slotName = kn;
    if (!ok) return false;
    writeInfo(k, tmp, name, si.playSecs, si.savedAt); return true;
}

bool saveGame(const Game& g) {
    mkdirs();
    const std::string P = slotPath(g_saveSlot);
    FILE* f = fopen((P + ".tmp").c_str(), "wb");
    if (!f) return false;
    W w{f};
    w.v(MAGIC); w.v(VERSION);
    w.v(g.house); w.v(g.room); w.v(g.holiday); w.v(g.lightsOn); w.v(g.track);
    for (int h = 0; h < 2; h++)
        for (int r = 0; r < R_COUNT; r++) {
            const RoomState& rs = g.rooms[h][r];
            w.v(rs.wall); w.v(rs.wallPat); w.v(rs.floor); w.v(rs.floorPat);
            w.v((uint32_t)rs.items.size());
            for (const Placed& p : rs.items) w.v(p);
            uint32_t na = 0;   // only household members & pets are saved
            for (const Actor& a : rs.actors) if (a.kind == 1 || g.sims[a.id].household) na++;
            w.v(na);
            for (const Actor& a : rs.actors)
                if (a.kind == 1 || g.sims[a.id].household) { w.v(a.kind); w.v(a.id); w.v(a.x); w.v(a.y); }
        }
    w.v((uint32_t)g.diary.size());
    for (const DiaryEntry& d : g.diary) { w.s(d.date); w.s(d.text); }
    w.s(g.diaryCode);
    w.v(g.dishes);
    w.b(g.best, sizeof g.best);
    w.v((uint32_t)g.pets.size());
    for (uint8_t p : g.pets) w.v(p);
    w.v((uint32_t)g.sims.size());
    for (const Sim& s : g.sims) {
        w.s(s.name); w.s(s.bio); wLook(w, s.look);
        w.v(s.stage); w.v(s.stageDays); w.b(s.traits, 3); w.b(s.needs, sizeof s.needs); w.b(s.skills, sizeof s.skills);
        w.v(s.household); w.v(s.interest); w.v(s.outfit); w.v(s.career); w.v(s.careerLevel); w.v(s.performance);
        w.v(s.workedToday); w.v(s.enrolled); w.v(s.credits); w.v(s.degree); w.v(s.gradePts); w.v(s.homework); w.v(s.schoolToday);
        w.v(s.pregnant); w.v(s.pregDays);
        w.v((uint32_t)s.moodlets.size());
        for (const Moodlet& m : s.moodlets) { w.s(m.name); w.v(m.emotion); w.v(m.power); w.v(m.minutes); }
    }
    w.v(g.mc);
    w.v((uint32_t)g.rels.size());
    for (const Rel& r : g.rels) w.v(r);
    w.v((uint32_t)g.threads.size());
    for (const ChatThread& t : g.threads) {
        w.v(t.sim); w.v(t.prompt); w.v(t.unread); w.v(t.about);
        w.v((uint32_t)t.msgs.size());
        for (const Msg& m : t.msgs) { w.v(m.me); w.s(m.text); }
    }
    w.v((uint32_t)g.events.size());
    for (const Event& e : g.events) { w.v(e.day); w.v(e.hour); w.v(e.type); w.v(e.sim); w.v(e.loc); w.s(e.text); w.v(e.done); }
    w.v((uint32_t)g.posts.size());
    for (const Post& p : g.posts) { w.v(p.sim); w.s(p.text); w.v(p.likes); w.v(p.kind); }
    w.v(g.money); w.v(g.followers); w.v(g.day); w.v(g.minute); w.v(g.aging); w.v(g.ownedOutfits); w.v(g.artStyle); w.v(g.trophies); w.v(g.pregnancyOn); w.v(g.rep); w.b(g.cliqueStand, 5); w.v(g.clique); w.v(g.club);
    w.v(g.asp); w.b(g.aspLevel, 8); w.v(g.satisfaction); w.v(g.perks); w.b(g.wantType, 3); w.v(g.wantsDay); w.v(g.collected);
    w.v(g.weather); w.v(g.weatherDay); w.v(g.shifts); w.v(g.postsMade); w.v(g.datesHad); w.v(g.momentsSeen);
    for (const Plot& p : g.plots) { w.v(p.crop); w.v(p.stage); w.v(p.watered); w.v(p.health); }
    w.b(g.basket, sizeof g.basket);
    w.v((uint32_t)g.memories.size());
    for (const Memory& m : g.memories) { w.v(m.day); w.v(m.type); w.v(m.a); w.v(m.b); w.s(m.text); }
    // v12: crime & drama
    w.v(g.heat); w.v(g.rapSheet); w.v(g.crimes); w.v(g.heists); w.v(g.mischief); w.v(g.witness); w.v(g.witnessCrime); w.v(g.witnessDay);
    w.v((uint32_t)g.dirt.size()); for (const Dirt& d : g.dirt) { w.v(d.sim); w.v(d.secret); w.v(d.state); w.v(d.day); }
    w.v((uint32_t)g.tea.size()); for (const Scandal& t : g.tea) { w.v(t.sim); w.v(t.kind); w.v(t.extra); w.v(t.day); w.v(t.handled); }
    w.v(g.myScandal); w.v(g.myScandalKind); w.v(g.damageDay); w.v(g.heistDay); w.v(g.burgleDay); w.v(g.csSeen); w.v(g.alarm); w.v(g.cop);
    // v15: drama & social life
    w.v((uint32_t)g.grudges.size()); for (const Grudge& gr : g.grudges) { w.v(gr.sim); w.v(gr.days); }
    w.v((uint32_t)g.impressed.size()); for (uint16_t v : g.impressed) w.v(v);
    w.v((uint32_t)g.bffs.size()); for (uint16_t v : g.bffs) w.v(v);
    w.v(g.luck); w.v(g.allowanceDay); w.v(g.dramaDay); w.v(g.affair); w.v(g.affairBff); w.v(g.affairRisk);
    // v16: STATUS
    w.v((uint32_t)g.st.size());
    for (const StPost& p : g.st) {
        w.v(p.who); w.v(p.about); w.v(p.about2); w.v(p.kind); w.v(p.tag); w.v(p.cred); w.v(p.flags); w.v(p.heat); w.v(p.ttl);
        w.v(p.day); w.v(p.minute); w.v(p.reposts); w.v(p.likes); w.s(p.text);
        w.v((uint32_t)p.replies.size()); for (const StReply& r : p.replies) { w.v(r.who); w.v(r.tone); w.s(r.text); }
    }
    w.v((uint32_t)g.stFollow.size()); for (uint16_t v : g.stFollow) w.v(v);
    w.v((uint32_t)g.stBlock.size()); for (uint16_t v : g.stBlock) w.v(v);
    w.v(g.stPrivate); w.v(g.stClout); w.v(g.stPostsMade);
    w.v((uint32_t)g.lbLikes.size()); for (uint16_t v : g.lbLikes) w.v(v);
    w.v((uint32_t)g.lbBlock.size()); for (uint16_t v : g.lbBlock) w.v(v);
    w.v(g.lbIntent); w.v(g.lbFilter); w.v(g.lbTheme); w.v(g.lbStatus); w.v(g.lbMatches); w.v(g.lbDeleted);
    { const StoryState& q = g.story; w.v(q.ch); w.v(q.step); w.v(q.boss); w.v(q.debt); w.v(q.flags); w.v(q.theme); w.v(q.pal); w.v(q.label); w.v(q.ending); w.v(q.bought); for (int i = 0; i < 9; i++) w.v(q.looks[i]); w.v(q.score); }
    // v17: the life engine
    w.v((uint32_t)g.life.size());
    for (const NpcLife& l : g.life) {
        w.v(l.goalProg); w.v(l.goalsDone); w.v(l.hobbyLvl); w.v(l.away); w.v(l.kids); w.v(l.sinceDay);
        for (const LifeMemory& m : l.mem) { w.v(m.kind); w.v(m.other); w.v(m.day); }
    }
    w.v(g.lifeEv); w.v(g.lifeEvRoom); w.v(g.lifeEvA); w.v(g.lifeEvB); w.v(g.lifeEvDay);
    // v18: raising kids
    w.v((uint32_t)g.life.size());
    for (const NpcLife& l : g.life) { w.v(l.motor); w.v(l.talk); w.v(l.potty); w.v(l.imag); w.v(l.social); w.v(l.disc); w.v(l.miles); w.v(l.warm); w.v(l.strict); }
    w.v(g.festDone);
    { const OtomeState& o = g.oto; for (int i = 0; i < 7; i++) { w.v(o.aff[i]); w.v(o.ch[i]); w.v(o.ended[i]); } w.v(o.bonus); }
    // v19: family planner
    w.v(g.curfew); w.v(g.famEvents); w.v(g.allowance); w.v(g.mcGrounded); w.v(g.sneaker); w.v(g.mcSneaking);
    w.v((uint32_t)g.life.size());
    for (const NpcLife& l : g.life) { w.v(l.chore); w.v(l.choresWeek); w.v(l.grounded); }
    // v21: legacy challenges
    { const LegacyState& L = g.leg; w.v(L.active); w.v(L.gen); w.v(L.elim); w.v(L.view); w.v(L.done); w.v(L.cleared); w.v(L.startDay); w.v(L.lover); }
    w.v(g.groceries);   // v22
    // v23: love life + more outfits
    w.v(g.ownedOutfits2);
    { const LoveState& L = g.love; w.v(L.situ); w.v(L.rival); w.v(L.confront); w.v(L.situDay); w.v(L.lastRomDay); w.v(L.annivDay); w.v(L.evDay);
      w.v(L.trust); w.v(L.spark); w.v(L.situDodges);
      w.v((uint32_t)L.exes.size()); for (size_t i = 0; i < L.exes.size(); i++) { w.v(L.exes[i]); w.v(L.exDay[i]); }
      w.v((uint32_t)L.dated.size()); for (uint16_t d : L.dated) w.v(d); }
    w.v(g.cookTutDone); w.v(g.recipes);   // v24
    { const MovieState& M = g.movie; for (int i = 0; i < 16; i++) w.v(M.stars[i]); for (int i = 0; i < 4; i++) w.v(M.jobDay[i]); w.v(M.made); }   // v25
    // v26
    w.v((uint32_t)g.lineage.size()); for (const Lineage& l : g.lineage) { w.v(l.kid); w.v(l.pa); w.v(l.pb); }
    w.v((uint32_t)g.packages.size()); for (const Package& p : g.packages) { w.v(p.kind); w.v(p.day); w.v(p.hour); }
    w.v((uint32_t)g.sentiments.size()); for (const Sentiment& s : g.sentiments) { w.v(s.sim); w.v(s.kind); w.v(s.until); }
    w.v(g.lotTraits);
    { const Economy& E = g.eco;   // v27
      w.v(E.elecUse); w.v(E.waterUse); w.v(E.subs); w.v(E.autopay);
      w.v(E.bill.issued); w.v(E.bill.due); for (int i = 0; i < BI_COUNT; i++) w.v(E.bill.item[i]); w.v(E.bill.paid);
      for (int i = 0; i < 4; i++) w.v(E.history[i]);
      w.v(E.powerOff); w.v(E.waterOff); w.v(E.netOff);
      for (int i = 0; i < EC_COUNT; i++) { w.v(E.spend[i]); w.v(E.lastSpend[i]); } w.v(E.weekStartMoney);
      w.v(E.groceryQ); w.v(E.groceryDay); w.v(E.autoGrocery); w.v(E.ordersWeek);
      w.v((uint32_t)E.porch.size()); for (uint8_t p : E.porch) w.v(p);
      w.v(E.order.rest); for (int i = 0; i < 3; i++) w.v(E.order.qty[i]); w.v(E.order.placed); w.v(E.order.eta); w.v(E.order.tip); w.v(E.order.total); }
    // v28: favorites & home types
    for (const Sim& s : g.sims) { w.v(s.favFood); w.v(s.favColor); w.v(s.zodiac); }
    w.v(g.houseType[0]); w.v(g.houseType[1]);
    w.v(g.gadgets);   // v29
    // v31: life story + movie maker
    w.v((uint32_t)g.milestones.size()); for (const MilestoneRec& m : g.milestones) { w.v(m.sim); w.v(m.day); w.v(m.id); w.v(m.other); }
    w.v((uint32_t)g.adopted.size()); for (uint16_t a : g.adopted) w.v(a);
    for (size_t i = 0; i < g.sims.size(); i++) { w.v(i < g.deathType.size() ? g.deathType[i] : (uint8_t)0); w.v(i < g.deathDay.size() ? g.deathDay[i] : (uint16_t)0); w.v(i < g.sickDays.size() ? g.sickDays[i] : (uint8_t)0); }
    w.v(g.lastDeath); w.v(g.lastDeathDay); w.v(g.memorialDone);
    w.v((uint32_t)g.movies.size());
    for (const MovieSeries& ms : g.movies) { w.s(ms.title); w.v((uint32_t)ms.eps.size());
        for (const MovieEp& e : ms.eps) { w.s(e.title); w.v((uint32_t)e.scenes.size());
            for (const MovieScene& sc : e.scenes) { w.s(sc.title); w.v(sc.loc); w.v(sc.cam); w.v(sc.trans); w.v(sc.dur);
                for (const MovieActor& a : sc.cast) { w.v(a.sim); w.v(a.pos); w.v(a.act); w.v(a.outfit); }
                w.v((uint32_t)sc.lines.size()); for (const MovieLine& l : sc.lines) { w.v(l.who); w.v(l.emo); w.v(l.act); w.s(l.text); } } } }
    // v32: health & household
    for (size_t i = 0; i < g.sims.size(); i++) { w.v(i < g.illness.size() ? g.illness[i] : (uint8_t)0); w.v(i < g.diag.size() ? g.diag[i] : (uint8_t)0); }
    w.s(g.hhName); w.v(g.coinJarDay);
    w.v(g.vacDest); w.v(g.vacEnd); w.v(g.courtDay); w.v(g.courtCrime);   // v33
    w.v((uint32_t)g.ccSlots.size()); for (const std::string& s : g.ccSlots) w.s(s);   // v34
    w.v((uint32_t)g.ccWear.size()); for (const std::string& s : g.ccWear) w.s(s);
    w.v(g.speedIdx); w.v(g.popIdx); w.v(g.lifespan); w.v(g.storyProg);   // v35
    w.v(g.spriteAuto); w.v((uint32_t)g.spriteOf.size()); for (const std::string& s : g.spriteOf) w.s(s);   // v36
    w.v((uint32_t)g.movedAway.size()); for (uint8_t m : g.movedAway) w.v(m);
    w.v(g.rival); w.v(g.rivalFor); w.v(g.rivalScore);
    w.v(g.hotelOwned); w.v(g.hotelRate); w.v(g.hotelStayEnd); w.v(g.hotelTheme); w.v(g.hotelUpg); for (uint8_t s : g.hotelStaff) w.v(s); w.v(g.hotelGuests); w.v(g.hotelReview); w.v(g.hotelLastNet);
    w.v(g.jailUntil); w.v(g.boardEnd);   // v37
    w.v(g.homeId); w.v(g.rentMiss); w.v(g.homeFav); w.v(g.homeOwned);   // v39
    w.v((uint32_t)g.giftInv.size()); for (uint8_t c : g.giftInv) w.v(c);   // v40 content pack
    w.v((uint32_t)g.foodInv.size()); for (const FoodItem& f : g.foodInv) { w.v(f.recipe); w.v(f.quality); w.v(f.servings); w.v(f.day); }
    w.v((uint32_t)g.giftLog.size()); for (const GiftLog& l : g.giftLog) { w.v(l.sim); w.v(l.item); w.v(l.result); w.v(l.day); }
    w.v(g.tvWatched);
    w.v(g.recipesMade); w.v(g.trips); w.v(g.leg.cleared2); w.v(g.leg.homeAtStart); w.v(g.leg.tripsAtStart);   // v41
    w.v(g.filter); w.v(g.filterStr);   // v42 photo filter
    w.v(g.recipesMade2); w.v(g.storyEvDone);   // v43
    fclose(f);
    // keep the previous save as a backup, then swap the new one in
    if (fileExists(P)) { remove((P + ".bak").c_str()); rename(P.c_str(), (P + ".bak").c_str()); }
    bool ok = rename((P + ".tmp").c_str(), P.c_str()) == 0;
    vfsBeginSave();
    kpopSaveTo(P + ".kpop"); challengeSaveTo(P + ".chal"); clubsSaveTo(P + ".club"); villaSaveTo(P + ".vila"); nightSaveTo(P + ".nite"); hotdateSaveTo(P + ".hdat"); spicySaveTo(P + ".spcy"); lxSaveTo(P + ".lxe"); farmSaveTo(P + ".farm"); lvlSaveTo(P + ".lvl"); l3SaveTo(P + ".lf3"); dlSaveTo(P + ".deep"); seSaveTo(P + ".sev");
    vfsEndSave(P + ".pack", P);
    if (ok) rotateBackups(P, g.day);
    if (ok) writeInfo(g_saveSlot, g, g_slotName.empty() ? "My Life" : g_slotName, g_playSecs, (int64_t)time(nullptr));
    return ok;
}

static bool loadFrom(const std::string& path, Game& out);
bool loadGame(Game& out) {
    const std::string P = slotPath(g_saveSlot);
    bool ok = loadFrom(P, out);
    if (!ok && loadFrom(P + ".bak", out)) ok = true;   // recovery: fall back to the previous save
    vfsBeginLoad(P + ".pack", P);
    if (ok) { kpopLoadFrom(P + ".kpop"); challengeLoadFrom(P + ".chal"); clubsLoadFrom(P + ".club"); villaLoadFrom(P + ".vila"); nightLoadFrom(P + ".nite"); hotdateLoadFrom(P + ".hdat"); spicyLoadFrom(P + ".spcy"); lxLoadFrom(P + ".lxe"); farmLoadFrom(P + ".farm"); lvlLoadFrom(P + ".lvl"); l3LoadFrom(P + ".lf3"); dlLoadFrom(P + ".deep"); seLoadFrom(P + ".sev"); }
    vfsEndLoad();
    if (ok) {
        FILE* f = fopen((P + ".inf").c_str(), "rb");
        g_playSecs = 0; g_slotName = "My Life";
        if (f) { Rd q{f}; uint32_t m = 0, ver = 0; q.v(m); q.v(ver); if (m == INF_MAGIC) { std::string nm, mc; uint16_t d = 0; float mi = 0; q.s(nm); q.s(mc); q.v(d); q.v(mi); q.v(g_playSecs); if (!q.ok) g_playSecs = 0; if (q.ok && !nm.empty()) g_slotName = nm; } fclose(f); }
    }
    return ok;
}
static bool loadFrom(const std::string& path, Game& out) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    Rd r{f};
    uint32_t magic = 0, ver = 0;
    r.v(magic); r.v(ver);
    if (magic != MAGIC || ver < 4 || ver > VERSION) { fclose(f); return false; }
    Game g;
    r.v(g.house); r.v(g.room); r.v(g.holiday); r.v(g.lightsOn); r.v(g.track); if (g.track >= NUM_TRACKS && (g.track < 100 || g.track > 160)) g.track = 0;
    for (int h = 0; h < 2 && r.ok; h++)
        for (int rm = 0; rm < (ver >= 51 ? R_COUNT : ver >= 50 ? R_COUNT_V50 : ver >= 49 ? R_COUNT_V49 : ver >= 48 ? R_COUNT_V48 : ver >= 47 ? R_COUNT_V47 : ver >= 46 ? R_COUNT_V46 : ver >= 45 ? R_COUNT_V45 : ver >= 44 ? R_COUNT_V44 : ver >= 39 ? R_COUNT_V43 : ver >= 38 ? R_COUNT_V38 : ver >= 37 ? R_COUNT_V37 : ver >= 33 ? R_COUNT_V36 : ver >= 30 ? R_COUNT_V32 : ver >= 29 ? R_COUNT_V29 : ver >= 22 ? R_COUNT_V28 : ver >= 14 ? R_COUNT_V21 : ver >= 13 ? R_COUNT_V13 : ver >= 6 ? R_COUNT_V12 : R_COUNT_V5) && r.ok; rm++) {
            RoomState& rs = g.rooms[h][rm];
            r.v(rs.wall); r.v(rs.wallPat); r.v(rs.floor); r.v(rs.floorPat);
            rs.wall %= NUM_WALLS; rs.wallPat %= NUM_WALLPATS; rs.floor %= NUM_FLOORS; rs.floorPat %= NUM_FLOORPATS;
            uint32_t n = r.count(200);
            rs.items.resize(n);
            for (Placed& p : rs.items) r.v(p);
            rs.items.erase(std::remove_if(rs.items.begin(), rs.items.end(), [](const Placed& p) { return p.item >= itemCount(); }), rs.items.end());
            uint32_t na = r.count(100);
            for (uint32_t i = 0; i < na && r.ok; i++) {
                Actor a; r.v(a.kind); r.v(a.id); r.v(a.x); r.v(a.y); a.tx = a.x;
                rs.actors.push_back(a);
            }
        }
    uint32_t nd = r.count(500);
    for (uint32_t i = 0; i < nd && r.ok; i++) { DiaryEntry d; r.s(d.date); r.s(d.text); g.diary.push_back(d); }
    r.s(g.diaryCode);
    r.v(g.dishes);
    r.b(g.best, ver >= 6 ? sizeof g.best : sizeof(int32_t) * NUM_MINIGAMES_V5);
    uint32_t np = r.count(16);
    for (uint32_t i = 0; i < np && r.ok; i++) { uint8_t p; r.v(p); g.pets.push_back(p); }
    uint32_t ns = r.count(2000);
    for (uint32_t i = 0; i < ns && r.ok; i++) {
        Sim s;
        r.s(s.name); r.s(s.bio); rLook(r, s.look, ver, i * 7919u + (uint32_t)s.name.size() * 31u + (s.name.empty() ? 0u : (uint8_t)s.name[0]));
        r.v(s.stage); r.v(s.stageDays); r.b(s.traits, 3); r.b(s.needs, ver >= 7 ? sizeof s.needs : sizeof(float) * N_COUNT_V6); r.b(s.skills, sizeof s.skills);
        r.v(s.household); r.v(s.interest); r.v(s.outfit); r.v(s.career); r.v(s.careerLevel); r.v(s.performance);
        r.v(s.workedToday); r.v(s.enrolled); r.v(s.credits); r.v(s.degree); r.v(s.gradePts); r.v(s.homework); r.v(s.schoolToday);
        r.v(s.pregnant); r.v(s.pregDays);
        uint32_t nm = r.count(16);
        for (uint32_t k = 0; k < nm && r.ok; k++) { Moodlet m; r.s(m.name); r.v(m.emotion); r.v(m.power); r.v(m.minutes); s.moodlets.push_back(m); }
        if (s.stage >= ST_COUNT) s.stage = ST_YA;
        if (s.career >= CR_COUNT) s.career = CR_NONE;
        g.sims.push_back(s);
    }
    r.v(g.mc);
    uint32_t nr = r.count(20000);
    g.rels.resize(nr);
    for (Rel& x : g.rels) r.v(x);
    uint32_t nt = r.count(500);
    for (uint32_t i = 0; i < nt && r.ok; i++) {
        ChatThread t; r.v(t.sim); r.v(t.prompt); r.v(t.unread); if (ver >= 5) r.v(t.about);
        uint32_t nm = r.count(200);
        for (uint32_t k = 0; k < nm && r.ok; k++) { Msg m; r.v(m.me); r.s(m.text); t.msgs.push_back(m); }
        g.threads.push_back(t);
    }
    uint32_t ne = r.count(500);
    for (uint32_t i = 0; i < ne && r.ok; i++) { Event e; r.v(e.day); r.v(e.hour); r.v(e.type); r.v(e.sim); r.v(e.loc); r.s(e.text); r.v(e.done); g.events.push_back(e); }
    uint32_t npo = r.count(100);
    for (uint32_t i = 0; i < npo && r.ok; i++) { Post p; r.v(p.sim); r.s(p.text); r.v(p.likes); r.v(p.kind); g.posts.push_back(p); }
    r.v(g.money); r.v(g.followers); r.v(g.day); r.v(g.minute); r.v(g.aging);
    if (ver >= 20) r.v(g.ownedOutfits); else if (ver >= 5) { uint32_t o32 = 0; r.v(o32); g.ownedOutfits = o32; } else { uint16_t o16 = 0; r.v(o16); g.ownedOutfits = o16; }
    r.v(g.artStyle);
#ifdef DIVA_LIFE
    if (ver < 8 || g.artStyle >= NUM_ART_STYLES) g.artStyle = 4;
#else
    if (g.artStyle < FIRST_ART_CHOICE || g.artStyle >= NUM_ART_STYLES || g.artStyle == ART_FLIP) g.artStyle = DEFAULT_ART;   // pixel style retired
#endif
    if (ver >= 6) r.v(g.trophies);
    if (ver >= 7) r.v(g.pregnancyOn);
    if (ver >= 9) { r.v(g.rep); r.b(g.cliqueStand, 5); r.v(g.clique); r.v(g.club); if (g.clique >= 5) g.clique = -1; if (g.club >= 5) g.club = -1; }
    if (ver >= 10) {
        r.v(g.asp); r.b(g.aspLevel, 8); r.v(g.satisfaction); r.v(g.perks); r.b(g.wantType, 3); r.v(g.wantsDay); r.v(g.collected);
        r.v(g.weather); r.v(g.weatherDay); r.v(g.shifts); r.v(g.postsMade); r.v(g.datesHad); r.v(g.momentsSeen);
        if (g.asp >= 8) g.asp = -1;
        if (g.weather >= 6) g.weather = 0;
        for (uint8_t& lv : g.aspLevel) if (lv > 4) lv = 4;
    }
    if (ver >= 11) {
        for (Plot& p : g.plots) { r.v(p.crop); r.v(p.stage); r.v(p.watered); r.v(p.health); if (p.crop > NUM_CROPS) p = Plot(); }
        r.b(g.basket, sizeof g.basket);
        for (uint8_t& k : g.basket) if (k > 99) k = 99;
        uint32_t n = 0; r.v(n);
        if (n > 200) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) {
            Memory m; r.v(m.day); r.v(m.type); r.v(m.a); r.v(m.b); r.s(m.text);
            if (m.a >= (int)g.sims.size()) m.a = -1;
            if (m.b >= (int)g.sims.size()) m.b = -1;
            g.memories.push_back(m);
        }
    }
    if (ver >= 12 && r.ok) {
        r.v(g.heat); r.v(g.rapSheet); r.v(g.crimes); r.v(g.heists); r.v(g.mischief); r.v(g.witness); r.v(g.witnessCrime); r.v(g.witnessDay);
        uint32_t n = 0; r.v(n); if (n > 400) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) { Dirt d; r.v(d.sim); r.v(d.secret); r.v(d.state); r.v(d.day); if (d.sim < g.sims.size()) { d.secret %= 18; g.dirt.push_back(d); } }
        n = 0; r.v(n); if (n > 100) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) { Scandal t; r.v(t.sim); r.v(t.kind); r.v(t.extra); r.v(t.day); r.v(t.handled); if (t.sim < (int)g.sims.size()) g.tea.push_back(t); }
        r.v(g.myScandal); r.v(g.myScandalKind); r.v(g.damageDay); r.v(g.heistDay); r.v(g.burgleDay); r.v(g.csSeen); r.v(g.alarm); r.v(g.cop);
        if (g.cop >= (int)g.sims.size()) g.cop = -1;
        if (g.witness >= (int)g.sims.size()) g.witness = -1;
        if (g.myScandal > 100) g.myScandal = 100;
    }
    if (ver >= 15 && r.ok) {
        uint32_t n = 0; r.v(n); if (n > 500) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) { Grudge gr; r.v(gr.sim); r.v(gr.days); if (gr.sim < g.sims.size()) g.grudges.push_back(gr); }
        n = 0; r.v(n); if (n > 1000) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) { uint16_t v = 0; r.v(v); if (v < g.sims.size()) g.impressed.push_back(v); }
        n = 0; r.v(n); if (n > 500) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) { uint16_t v = 0; r.v(v); if (v < g.sims.size()) g.bffs.push_back(v); }
        r.v(g.luck); r.v(g.allowanceDay); r.v(g.dramaDay); r.v(g.affair); r.v(g.affairBff); r.v(g.affairRisk);
        if (g.affair >= (int)g.sims.size() || g.affairBff >= (int)g.sims.size() || g.affair < -1 || g.affairBff < -1) { g.affair = -1; g.affairBff = -1; }
        if (g.luck < -1 || g.luck > 1) g.luck = 0;
        if (g.heat > 100) g.heat = 100;
    }
    if (ver >= 16 && r.ok) {
        const int ns = (int)g.sims.size();
        auto okWho = [&](int16_t w) { return w >= -4 && w < ns; };
        uint32_t n = 0; r.v(n); if (n > 200) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) {
            StPost p;
            r.v(p.who); r.v(p.about); r.v(p.about2); r.v(p.kind); r.v(p.tag); r.v(p.cred); r.v(p.flags); r.v(p.heat); r.v(p.ttl);
            r.v(p.day); r.v(p.minute); r.v(p.reposts); r.v(p.likes); r.s(p.text);
            uint32_t nr = 0; r.v(nr); if (nr > 40) { r.ok = false; break; }
            for (uint32_t k = 0; k < nr && r.ok; k++) { StReply q; r.v(q.who); r.v(q.tone); r.s(q.text); if (okWho(q.who)) p.replies.push_back(q); }
            if (okWho(p.who) && p.about >= -1 && p.about < ns && p.about2 >= -1 && p.about2 < ns) g.st.push_back(p);
        }
        n = 0; r.v(n); if (n > 1000) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) { uint16_t v = 0; r.v(v); if (v < g.sims.size()) g.stFollow.push_back(v); }
        n = 0; r.v(n); if (n > 1000) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) { uint16_t v = 0; r.v(v); if (v < g.sims.size()) g.stBlock.push_back(v); }
        r.v(g.stPrivate); r.v(g.stClout); r.v(g.stPostsMade);
        g.stClout = (int16_t)std::max(-100, std::min(100, (int)g.stClout));
        n = 0; r.v(n); if (n > 1000) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) { uint16_t v = 0; r.v(v); if (v < g.sims.size()) g.lbLikes.push_back(v); }
        n = 0; r.v(n); if (n > 1000) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) { uint16_t v = 0; r.v(v); if (v < g.sims.size()) g.lbBlock.push_back(v); }
        r.v(g.lbIntent); r.v(g.lbFilter); r.v(g.lbTheme); r.v(g.lbStatus); r.v(g.lbMatches); r.v(g.lbDeleted);
        g.lbIntent %= 4; g.lbFilter %= 17; g.lbTheme %= 4; g.lbStatus %= 3;
        StoryState& q = g.story; r.v(q.ch); r.v(q.step); r.v(q.boss); r.v(q.debt); r.v(q.flags); r.v(q.theme); r.v(q.pal); r.v(q.label); r.v(q.ending); r.v(q.bought); for (int i = 0; i < 9; i++) r.v(q.looks[i]); r.v(q.score);
        if (q.ch > 8) q = StoryState{};
    }
    if (ver >= 17 && r.ok) {
        const int ns = (int)g.sims.size();
        uint32_t n = 0; r.v(n); if (n > 4000) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) {
            NpcLife l;
            r.v(l.goalProg); r.v(l.goalsDone); r.v(l.hobbyLvl); r.v(l.away); r.v(l.kids); r.v(l.sinceDay);
            for (LifeMemory& m : l.mem) { r.v(m.kind); r.v(m.other); r.v(m.day); if (m.other < -1 || m.other >= ns) m.other = -1; }
            if (l.goalProg > 100) l.goalProg = 0;
            if (l.away > 7) l.away = 0;
            if ((int)i < ns) g.life.push_back(l);
        }
        r.v(g.lifeEv); r.v(g.lifeEvRoom); r.v(g.lifeEvA); r.v(g.lifeEvB); r.v(g.lifeEvDay);
        if (g.lifeEv >= 15 || g.lifeEvRoom >= R_COUNT) g.lifeEv = 0, g.lifeEvRoom = 0;
        if (g.lifeEvA < -1 || g.lifeEvA >= ns) g.lifeEvA = -1;
        if (g.lifeEvB < -1 || g.lifeEvB >= ns) g.lifeEvB = -1;
    }
    if (ver >= 18 && r.ok) {
        uint32_t n = 0; r.v(n); if (n > 4000) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) {
            NpcLife tmp;
            r.v(tmp.motor); r.v(tmp.talk); r.v(tmp.potty); r.v(tmp.imag); r.v(tmp.social); r.v(tmp.disc); r.v(tmp.miles); r.v(tmp.warm); r.v(tmp.strict);
            if (i < g.life.size()) {
                NpcLife& l = g.life[i];
                l.motor = std::min<uint8_t>(tmp.motor, 100); l.talk = std::min<uint8_t>(tmp.talk, 100); l.potty = std::min<uint8_t>(tmp.potty, 100);
                l.imag = std::min<uint8_t>(tmp.imag, 100); l.social = std::min<uint8_t>(tmp.social, 100); l.disc = std::min<uint8_t>(tmp.disc, 100);
                l.miles = tmp.miles; l.warm = tmp.warm; l.strict = tmp.strict;
            }
        }
        r.v(g.festDone);
        OtomeState& o = g.oto;
        for (int i = 0; i < 7; i++) { r.v(o.aff[i]); r.v(o.ch[i]); r.v(o.ended[i]); if (o.ch[i] > 4) o.ch[i] = 0; if (o.ended[i] > 3) o.ended[i] = 0; }
        r.v(o.bonus);
    }
    if (ver >= 19 && r.ok) {
        r.v(g.curfew); r.v(g.famEvents); r.v(g.allowance); r.v(g.mcGrounded); r.v(g.sneaker); r.v(g.mcSneaking);
        if (g.curfew < 19 || g.curfew > 23) g.curfew = 22;
        if (g.allowance > 3) g.allowance = 0;
        if (g.sneaker < -1 || g.sneaker >= (int)g.sims.size()) g.sneaker = -1;
        uint32_t n = 0; r.v(n); if (n > 4000) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) {
            uint8_t c = 0, w2 = 0, gr = 0; r.v(c); r.v(w2); r.v(gr);
            if (i < g.life.size()) { g.life[i].chore = c < CHR_COUNT ? c : 0; g.life[i].choresWeek = w2; g.life[i].grounded = gr > 7 ? 0 : gr; }
        }
    }
    if (ver >= 21 && r.ok) {
        LegacyState& L = g.leg; r.v(L.active); r.v(L.gen); r.v(L.elim); r.v(L.view); r.v(L.done); r.v(L.cleared); r.v(L.startDay); r.v(L.lover);
        if (L.active < -1 || L.active >= 16) L.active = -1;
        if (L.view >= 16) L.view = 0;
        if (L.lover < -1 || L.lover >= (int)g.sims.size()) L.lover = -1;
        if (L.gen == 0 || L.gen > 99) L.gen = 1;
    }
    if (ver >= 22 && r.ok) { r.v(g.groceries); if (g.groceries > 60) g.groceries = 60; }
    if (ver >= 23 && r.ok) {
        r.v(g.ownedOutfits2);
        LoveState& L = g.love; const int ns = (int)g.sims.size();
        r.v(L.situ); r.v(L.rival); r.v(L.confront); r.v(L.situDay); r.v(L.lastRomDay); r.v(L.annivDay); r.v(L.evDay);
        r.v(L.trust); r.v(L.spark); r.v(L.situDodges);
        if (L.situ >= ns || L.situ < -1) L.situ = -1; if (L.rival >= ns || L.rival < -1) L.rival = -1; if (L.confront >= ns || L.confront < -1) L.confront = -1;
        if (L.trust > 100) L.trust = 70; if (L.spark > 100) L.spark = 60;
        uint32_t n = 0; r.v(n); if (n > 64) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) { uint16_t s = 0, d = 0; r.v(s); r.v(d); if (s < ns) { L.exes.push_back(s); L.exDay.push_back(d); } }
        n = 0; r.v(n); if (n > 64) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) { uint16_t s = 0; r.v(s); if (s < ns) L.dated.push_back(s); }
    }
    if (ver >= 24 && r.ok) { r.v(g.cookTutDone); r.v(g.recipes); }
    if (ver >= 25 && r.ok) { MovieState& M = g.movie; for (int i = 0; i < 16; i++) { r.v(M.stars[i]); if (M.stars[i] > 3) M.stars[i] = 0; } for (int i = 0; i < 4; i++) r.v(M.jobDay[i]); r.v(M.made); }
    if (ver >= 26 && r.ok) {
        const int ns = (int)g.sims.size(); uint32_t n = 0;
        r.v(n); if (n > 400) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) { Lineage l; r.v(l.kid); r.v(l.pa); r.v(l.pb); if (l.kid < ns && l.pa < ns && l.pb < ns) g.lineage.push_back(l); }
        n = 0; r.v(n); if (n > 64) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) { Package p; r.v(p.kind); r.v(p.day); r.v(p.hour); g.packages.push_back(p); }
        n = 0; r.v(n); if (n > 256) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) { Sentiment s; r.v(s.sim); r.v(s.kind); r.v(s.until); if (s.sim < ns) g.sentiments.push_back(s); }
        r.v(g.lotTraits);
    }
    if (ver >= 27 && r.ok) {
        Economy& E = g.eco;
        r.v(E.elecUse); r.v(E.waterUse); r.v(E.subs); r.v(E.autopay);
        r.v(E.bill.issued); r.v(E.bill.due); for (int i = 0; i < BI_COUNT; i++) r.v(E.bill.item[i]); r.v(E.bill.paid);
        for (int i = 0; i < 4; i++) r.v(E.history[i]);
        r.v(E.powerOff); r.v(E.waterOff); r.v(E.netOff);
        for (int i = 0; i < EC_COUNT; i++) { r.v(E.spend[i]); r.v(E.lastSpend[i]); } r.v(E.weekStartMoney);
        r.v(E.groceryQ); r.v(E.groceryDay); r.v(E.autoGrocery); r.v(E.ordersWeek);
        uint32_t n = 0; r.v(n); if (n > 64) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) { uint8_t p = 0; r.v(p); E.porch.push_back(p); }
        r.v(E.order.rest); for (int i = 0; i < 3; i++) r.v(E.order.qty[i]); r.v(E.order.placed); r.v(E.order.eta); r.v(E.order.tip); r.v(E.order.total);
        if (E.order.rest >= 10 || E.order.rest < -1) E.order.rest = -1;
        if (E.groceryQ > 3) E.groceryQ = 1;
    }
    if (ver >= 28 && r.ok) {
        for (Sim& s : g.sims) { r.v(s.favFood); r.v(s.favColor); r.v(s.zodiac); s.favFood %= 18; s.favColor %= 12; s.zodiac %= 12; }
        r.v(g.houseType[0]); r.v(g.houseType[1]); for (uint8_t& h : g.houseType) if (h > 2) h = 1;
        if (ver >= 29) r.v(g.gadgets);
    } else for (Sim& s : g.sims) { s.favFood = (uint8_t)irand(18); s.favColor = (uint8_t)irand(12); s.zodiac = (uint8_t)irand(12); }
    const size_t NS = g.sims.size();
    g.deathType.assign(NS, 0); g.deathDay.assign(NS, 0); g.sickDays.assign(NS, 0); g.milestones.clear(); g.adopted.clear(); g.movies.clear();
    if (ver >= 31 && r.ok) {
        uint32_t n = 0; r.v(n); if (n > 5000) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) { MilestoneRec m; r.v(m.sim); r.v(m.day); r.v(m.id); r.v(m.other); if (m.sim < NS) g.milestones.push_back(m); }
        n = 0; r.v(n); if (n > 500) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) { uint16_t a = 0; r.v(a); if (a < NS) g.adopted.push_back(a); }
        for (size_t i = 0; i < NS && r.ok; i++) { r.v(g.deathType[i]); r.v(g.deathDay[i]); r.v(g.sickDays[i]); if (g.deathType[i] >= DK_COUNT) g.deathType[i] = 0; }
        r.v(g.lastDeath); r.v(g.lastDeathDay); r.v(g.memorialDone); if (g.lastDeath >= (int)NS) g.lastDeath = -1;
        n = 0; r.v(n); if (n > 20) r.ok = false;
        for (uint32_t i = 0; i < n && r.ok; i++) { MovieSeries ms; r.s(ms.title); uint32_t ne = 0; r.v(ne); if (ne > 40) { r.ok = false; break; }
            for (uint32_t e = 0; e < ne && r.ok; e++) { MovieEp ep; r.s(ep.title); uint32_t nsc = 0; r.v(nsc); if (nsc > 40) { r.ok = false; break; }
                for (uint32_t k = 0; k < nsc && r.ok; k++) { MovieScene sc; r.s(sc.title); r.v(sc.loc); r.v(sc.cam); r.v(sc.trans); r.v(sc.dur);
                    for (MovieActor& a : sc.cast) { r.v(a.sim); r.v(a.pos); r.v(a.act); r.v(a.outfit); if (a.sim >= (int)NS) a.sim = -1; a.pos %= 3; }
                    uint32_t nl = 0; r.v(nl); if (nl > 60) { r.ok = false; break; }
                    for (uint32_t q = 0; q < nl && r.ok; q++) { MovieLine l; r.v(l.who); r.v(l.emo); r.v(l.act); r.s(l.text); if (l.who > 2) l.who = -1; l.emo %= EM_COUNT; sc.lines.push_back(l); }
                    if (sc.loc >= R_COUNT && sc.loc < 150) sc.loc = 0; ep.scenes.push_back(sc); }
                ms.eps.push_back(ep); }
            g.movies.push_back(ms); }
    }
    g.illness.assign(NS, 0); g.diag.assign(NS, 0);
    if (ver >= 32 && r.ok) {
        for (size_t i = 0; i < NS && r.ok; i++) { r.v(g.illness[i]); r.v(g.diag[i]); if (g.illness[i] >= IL_COUNT_SAVE) g.illness[i] = 0; }
        r.s(g.hhName); r.v(g.coinJarDay);
        if (ver >= 33) { r.v(g.vacDest); r.v(g.vacEnd); r.v(g.courtDay); r.v(g.courtCrime); if (g.vacDest > 3) g.vacDest = 0; }
        g.ccSlots.clear(); g.ccWear.clear();
        if (ver >= 34 && r.ok) {
            uint32_t n = 0; r.v(n); if (n > 64) r.ok = false;
            for (uint32_t i = 0; i < n && r.ok; i++) { std::string s; r.s(s); g.ccSlots.push_back(s); }
            n = 0; r.v(n); if (n > 3 * 2000) r.ok = false;
            for (uint32_t i = 0; i < n && r.ok; i++) { std::string s; r.s(s); g.ccWear.push_back(s); }
        }
        if (ver >= 35 && r.ok) {
            r.v(g.speedIdx); r.v(g.popIdx); r.v(g.lifespan); r.v(g.storyProg); g.speedIdx %= 5; g.popIdx %= 5; g.lifespan %= 3;
            g.spriteOf.clear();
            if (ver >= 36) { r.v(g.spriteAuto); uint32_t sn = 0; r.v(sn); if (sn > 4000) r.ok = false; for (uint32_t i = 0; i < sn && r.ok; i++) { std::string s; r.s(s); g.spriteOf.push_back(s); } }
            uint32_t n = 0; r.v(n); if (n > 4000) r.ok = false; g.movedAway.clear();
            for (uint32_t i = 0; i < n && r.ok; i++) { uint8_t m = 0; r.v(m); g.movedAway.push_back(m); }
            r.v(g.rival); r.v(g.rivalFor); r.v(g.rivalScore); if (g.rival >= (int)g.sims.size() || g.rivalFor >= (int)g.sims.size()) g.rival = -1;
            r.v(g.hotelOwned); r.v(g.hotelRate); r.v(g.hotelStayEnd); r.v(g.hotelTheme); r.v(g.hotelUpg); for (uint8_t& s : g.hotelStaff) { r.v(s); s %= 4; } r.v(g.hotelGuests); r.v(g.hotelReview); r.v(g.hotelLastNet);
        }
        if (ver >= 37 && r.ok) { r.v(g.jailUntil); r.v(g.boardEnd); }
        if (ver >= 39 && r.ok) { r.v(g.homeId); r.v(g.rentMiss); if (ver >= 42) { r.v(g.homeFav); r.v(g.homeOwned); } else { uint32_t a = 0, b = 0; r.v(a); r.v(b); g.homeFav = a; g.homeOwned = b; } if (g.homeId > 60) g.homeId = 0; }
        else g.homeId = (uint8_t)(g.house ? 1 : 0);
        if (ver >= 40 && r.ok) {
            uint32_t n = 0; r.v(n); if (n > 200) r.ok = false; g.giftInv.resize(r.ok ? n : 0); for (uint32_t i = 0; i < n && r.ok; i++) r.v(g.giftInv[i]);
            n = 0; r.v(n); if (n > 200) r.ok = false; g.foodInv.resize(r.ok ? n : 0); for (uint32_t i = 0; i < n && r.ok; i++) { FoodItem& f = g.foodInv[i]; r.v(f.recipe); r.v(f.quality); r.v(f.servings); r.v(f.day); if (f.recipe >= 64 && f.recipe < 100) f.recipe = (uint8_t)(f.recipe + 36); if (f.recipe >= 75 && (f.recipe < 100 || f.recipe >= 111)) f.recipe = 0; f.quality &= 3; }
            n = 0; r.v(n); if (n > 1000) r.ok = false; g.giftLog.resize(r.ok ? n : 0); for (uint32_t i = 0; i < n && r.ok; i++) { GiftLog& l = g.giftLog[i]; r.v(l.sim); r.v(l.item); r.v(l.result); r.v(l.day); }
            r.v(g.tvWatched);
        }
        if (ver >= 41 && r.ok) { r.v(g.recipesMade); r.v(g.trips); r.v(g.leg.cleared2); r.v(g.leg.homeAtStart); r.v(g.leg.tripsAtStart); }
        if (ver >= 42 && r.ok) { r.v(g.filter); r.v(g.filterStr); g.filter %= 8; }
        if (ver >= 43 && r.ok) { r.v(g.recipesMade2); r.v(g.storyEvDone); }
    }
    fclose(f);
    if (!r.ok || g.sims.empty() || g.mc < 0 || g.mc >= (int)g.sims.size() || g.house < 0 || g.house > 1 || g.room < 0 || g.room >= R_COUNT) return false;
    // validate actor references
    for (int h = 0; h < 2; h++)
        for (int rm = 0; rm < R_COUNT; rm++) {
            auto& v = g.rooms[h][rm].actors;
            for (size_t i = 0; i < v.size();) {
                bool bad = v[i].kind == 0 ? v[i].id >= g.sims.size() : v[i].id >= NUM_PETS;
                if (bad) v.erase(v.begin() + i); else i++;
            }
        }
    for (ChatThread& t : g.threads) if (t.sim >= g.sims.size()) t.sim = 0, t.prompt = -1;
    for (ChatThread& t : g.threads) { if (t.prompt >= NUM_PROMPTS && !chatLive(t.prompt)) t.prompt = -1; if (t.about >= (int)g.sims.size()) t.about = -1; }
    for (Sim& sm : g.sims) if (sm.outfit >= NUM_OUTFITS) sm.outfit = 0;
    for (Event& e : g.events) if (e.sim >= (int)g.sims.size()) e.sim = -1, e.done = true;
    for (Post& p : g.posts) if (p.sim >= (int)g.sims.size()) p.sim = -1;
    // 2.0 homes get a bathroom: add a toilet & shower to the closet if missing
    for (int h = 0; h < 2; h++) {
        bool has = false;
        for (int rm = 0; rm < R_COUNT; rm++) for (const Placed& p : g.rooms[h][rm].items) if (p.item / 12 == 33) has = true;
        if (!has) { g.rooms[h][R_CLOSET].items.push_back({(uint16_t)(33 * 12 + 4), 26, 230}); g.rooms[h][R_CLOSET].items.push_back({(uint16_t)(34 * 12 + 4), 380, 200}); }
    }
    out = g;
    return true;
}
