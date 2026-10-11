#include "State.h"
#include "../../NDS.h"
namespace Plugins::APC {
void Ctx::setNds(melonDS::NDS* value) {
    nds = value;
}
u8 Ctx::r8(u32 addr) const {
    return nds ? nds->ARM9Read8(addr) : 0;
}
u16 Ctx::r16(u32 addr) const {
    return nds ? nds->ARM9Read16(addr) : 0;
}
u32 Ctx::r32(u32 addr) const {
    return nds ? nds->ARM9Read32(addr) : 0;
}
bool Ctx::w8(u32 addr, u8 val) {
    bool ok = false;
    if (nds) {
        nds->ARM9Write8(addr, val);
        ok = r8(addr) == val;
    }
    return ok;
}
bool Ctx::w16(u32 addr, u16 val) {
    bool ok = false;
    if (nds) {
        nds->ARM9Write16(addr, val);
        ok = r16(addr) == val;
    }
    return ok;
}
bool Ctx::w32(u32 addr, u32 val) {
    bool ok = false;
    if (nds) {
        nds->ARM9Write32(addr, val);
        ok = r32(addr) == val;
    }
    return ok;
}
}
