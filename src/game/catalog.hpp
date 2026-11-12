#pragma once
int itemPrice(int id);   // main.cpp
#include "common.hpp"

enum Shape : int {
    SH_BED, SH_SOFA, SH_CHAIR, SH_TABLE, SH_LAMP, SH_PLANT, SH_RUG, SH_TV, SH_BOOKSHELF, SH_DRESSER,
    SH_VANITY, SH_SHOERACK, SH_FRIDGE, SH_OVEN, SH_SINK, SH_COUNTER, SH_CRIB, SH_TOYBOX,
    SH_PAINTING, SH_CURTAINS, SH_CLOCK, SH_MIRROR, SH_BEANBAG, SH_DESK, SH_SPEAKER, SH_LOUNGER,
    SH_GRILL, SH_UMBRELLA, SH_ARCADE, SH_DISCOBALL, SH_PERFUMESHELF, SH_FLOWERS, SH_BATHTUB,
    SH_TOILET, SH_SHOWER, SH_BENCH,
    // cute room pack
    SH_BRASSBED, SH_PLUSHBED, SH_OVALVANITY, SH_FLUFFYRUG, SH_HEARTRUG, SH_FLOWERCUSHION, SH_EGGCHAIR, SH_GAMERCHAIR,
    SH_CLOTHESRACK, SH_RECORDPLAYER, SH_RETROPC, SH_TEATABLE, SH_WARDROBE, SH_FLOORMIRROR, SH_NIGHTSTAND,
    SH_STARLIGHTS, SH_VINES, SH_POSTERWALL,
    SH_BATHVANITY, SH_SOAKTUB, SH_RETROFRIDGE, SH_PLUSHSOFA, SH_BOOKCUBBY, SH_MONSTERA, SH_DRUMLAMP,
    SH_GAMINGPC,
    // dream bedroom pack
    SH_CANOPYBED, SH_DAYBED, SH_HEARTSHELF, SH_PLUSHSHELF, SH_MANNEQUIN, SH_GUITAR, SH_SKATEBOARD, SH_BOOMBOX,
    SH_CORKBOARD, SH_VINYLWALL, SH_POUF, SH_STILETTOCHAIR, SH_HOLLYVANITY, SH_ROUNDRUG, SH_LANTERN, SH_HEXSHELF,
    SH_FLOORLAMP, SH_PLAIDSOFA, SH_WORLDMAP, SH_LIPSPILLOW,
    // lived-in room pack
    SH_CLOTHESPILE, SH_BOOKSTACK, SH_PURSEHOOKS, SH_IRONBED, SH_MUSICSTAND, SH_WINDOWSEAT, SH_PLANTSHELF, SH_DESKLAMP,
    SH_POLAROIDS, SH_CANDLES, SH_FLOORHEADPHONES, SH_MAGAZINES, SH_PATCHWORKBED, SH_FLATSHOES,
    SH_CC0, SH_CC_LAST = SH_CC0 + 31,   // 32 custom-content furniture slots (cc.cpp)
    // aesthetic room pack (after the CC slots so old saves keep their item ids)
    SH_CLOUDBED, SH_EMOBED, SH_GAMERBED, SH_BRATBED, SH_DAISYBED, SH_ZEBRABED, SH_BOWBED, SH_FAIRYCURTAIN, SH_NEONHEART, SH_FIGSHELF, SH_GAMERDESK, SH_CRTTV, SH_LAVALAMP, SH_MOONCHAIR, SH_CHECKRUG, SH_SUNRUG, SH_LEORUG, SH_HEARTMIRROR, SH_ISLAND, SH_STEELFRIDGE, SH_FLOATSHELF, SH_FIREPLACE, SH_SECTIONAL, SH_RECORDCONSOLE, SH_BEADCURTAIN,
    SH_COUNT
};

struct ShapeInfo { const char* name; bool wall; float w, h; };

constexpr int NUM_COLORS = 12;   // 10 solids + Zebra + Leopard
extern ShapeInfo SHAPES[SH_COUNT];   // custom-content slots are filled in at runtime
extern int g_ccBase[32];
extern const RGB PALETTE[NUM_COLORS];
extern const char* COLOR_NAMES[NUM_COLORS];

inline int itemCount() { return SH_COUNT * NUM_COLORS; }
// the cute-pack pieces behave like the classic ones (sleep, sit, makeup, outfits, music...)
inline int baseShape(int sh) {
    if (sh >= SH_CC0 && sh <= SH_CC_LAST) return g_ccBase[sh - SH_CC0];
    switch (sh) {
    case SH_BRASSBED: case SH_PLUSHBED: return SH_BED;
    case SH_FLOWERCUSHION: case SH_EGGCHAIR: case SH_GAMERCHAIR: return SH_BEANBAG;
    case SH_OVALVANITY: case SH_FLOORMIRROR: return SH_VANITY;
    case SH_CLOTHESRACK: case SH_WARDROBE: return SH_DRESSER;
    case SH_RECORDPLAYER: return SH_SPEAKER;
    case SH_RETROPC: case SH_GAMINGPC: return SH_DESK;
    case SH_NIGHTSTAND: case SH_STARLIGHTS: return SH_LAMP;
    case SH_VINES: return SH_PLANT;
    case SH_POSTERWALL: return SH_PAINTING;
    case SH_BATHVANITY: return SH_SINK;
    case SH_SOAKTUB: return SH_BATHTUB;
    case SH_RETROFRIDGE: return SH_FRIDGE;
    case SH_PLUSHSOFA: return SH_SOFA;
    case SH_BOOKCUBBY: return SH_BOOKSHELF;
    case SH_MONSTERA: return SH_PLANT;
    case SH_DRUMLAMP: return SH_LAMP;
    case SH_CANOPYBED: case SH_DAYBED: return SH_BED;
    case SH_HEARTSHELF: case SH_PLUSHSHELF: case SH_CORKBOARD: case SH_VINYLWALL: case SH_HEXSHELF: case SH_WORLDMAP: return SH_PAINTING;
    case SH_MANNEQUIN: return SH_DRESSER;
    case SH_GUITAR: case SH_BOOMBOX: return SH_SPEAKER;
    case SH_POUF: case SH_STILETTOCHAIR: case SH_LIPSPILLOW: return SH_BEANBAG;
    case SH_HOLLYVANITY: return SH_VANITY;
    case SH_LANTERN: case SH_FLOORLAMP: return SH_LAMP;
    case SH_PLAIDSOFA: return SH_SOFA;
    case SH_IRONBED: case SH_PATCHWORKBED: return SH_BED;
    case SH_WINDOWSEAT: return SH_BEANBAG;
    case SH_PURSEHOOKS: case SH_CLOTHESPILE: return SH_DRESSER;
    case SH_BOOKSTACK: case SH_MAGAZINES: return SH_BOOKSHELF;
    case SH_PLANTSHELF: return SH_PLANT;
    case SH_CLOUDBED: case SH_EMOBED: case SH_GAMERBED: case SH_BRATBED: case SH_DAISYBED: case SH_ZEBRABED: case SH_BOWBED: return SH_BED;
    case SH_FAIRYCURTAIN: return SH_STARLIGHTS;
    case SH_NEONHEART: case SH_FIGSHELF: case SH_FLOATSHELF: case SH_BEADCURTAIN: return SH_PAINTING;
    case SH_GAMERDESK: return SH_DESK;
    case SH_CRTTV: case SH_FIREPLACE: return SH_TV;
    case SH_LAVALAMP: return SH_LAMP;
    case SH_MOONCHAIR: return SH_BEANBAG;
    case SH_HEARTMIRROR: return SH_MIRROR;
    case SH_ISLAND: return SH_COUNTER;
    case SH_STEELFRIDGE: return SH_FRIDGE;
    case SH_SECTIONAL: return SH_SOFA;
    case SH_RECORDCONSOLE: return SH_SPEAKER;
    case SH_DESKLAMP: case SH_CANDLES: return SH_LAMP;
    case SH_POLAROIDS: return SH_PAINTING;
    case SH_FLOORHEADPHONES: case SH_MUSICSTAND: return SH_SPEAKER;
    default: return sh;
    }
}
inline bool isRugShape(int sh) { return sh == SH_RUG || sh == SH_FLUFFYRUG || sh == SH_HEARTRUG || sh == SH_ROUNDRUG || sh == SH_CHECKRUG || sh == SH_SUNRUG || sh == SH_LEORUG; }   // drawn flat under everything
inline int itemShape(int id) { return id / NUM_COLORS; }
inline int itemColor(int id) { return id % NUM_COLORS; }
inline int itemId(int shape, int color) { return shape * NUM_COLORS + color; }
std::string itemName(int id);

// x = center, y = bottom edge. t = time (for animation). music = something is playing.
void drawItem(int id, float x, float y, float s, float t, bool music);
void drawItemFront(int id, float x, float y, float s);   // part drawn over a sim using it (blanket, tub rim...)

// ---- furniture use data (seats & standing spots) ----
struct UseSpot { int slots; float seatY; float slotDX[2]; uint8_t kind; };   // kind: 1 sit, 2 lie, 3 bathe, 0 stand beside
UseSpot useSpot(int shape);
