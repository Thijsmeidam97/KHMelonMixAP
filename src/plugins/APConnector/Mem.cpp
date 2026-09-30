#include "State.h"
#include "../../NDS.h"
#include "../../NDSCart.h"
#include <cstring>
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
bool Ctx::readMem(const std::string& dom, u32 addr, u32 size, std::vector<u8>& out, std::string& err) const {
    bool ok = size <= RAMSIZE;
    if (!ok) {
        err = "memory request is too large";
    }
    if (ok) {
        out.resize(size);
        if (dom == "ROM") {
            const auto* cart = nds ? nds->GetNDSCart() : nullptr;
            ok = cart && addr <= cart->GetROMLength() && size <= cart->GetROMLength() - addr;
            if (ok)
                std::memcpy(out.data(), cart->GetROM() + addr, size);
            else
                err = "ROM range is unavailable";
        }
        else {
            ok = dom == "ARM9 System Bus" || dom == "System Bus";
            if (!ok)
                err = "unsupported memory domain: " + dom;
            else if (!nds) {
                err = "no NDS is loaded";
                ok = false;
            }
            else {
                for (u32 i = 0; i < size; ++i)
                    out[i] = nds->ARM9Read8(addr + i);
            }
        }
    }
    return ok;
}
bool Ctx::writeMem(const std::string& dom, u32 addr, const std::vector<u8>& data, std::string& err) {
    bool ok = dom != "ROM";
    if (!ok)
        err = "ROM is read-only";
    if (ok) {
        ok = dom == "ARM9 System Bus" || dom == "System Bus";
        if (!ok)
            err = "unsupported memory domain: " + dom;
        else if (!nds) {
            err = "no NDS is loaded";
            ok = false;
        }
        else {
            for (u32 i = 0; i < data.size(); ++i)
                nds->ARM9Write8(addr + i, data[i]);
        }
    }
    return ok;
}
void Ctx::pollDay() {
    if (day.arm && !day.appl && day.err.empty()) {
        const u32 obj = r32(SCENEREC);
        const bool ready = obj && r32(SCENEID) && !r32(PENDINGID);
        day.stable = ready ? day.stable + 1 : 0;
        if (day.stable >= STABLE) {
            if (!ram(obj, SCENEOBJ + 4))
                day.err = "scene object pointer is outside main RAM";
            const u32 st = r32(GAMESTATE);
            if (day.err.empty() && !ram(st, SCENEOBJ))
                day.err = "GameState store pointer is null or outside main RAM";
            if (day.err.empty()) {
                const u32 word = st + STATEWORD;
                const u32 old = r32(word);
                const u32 mask = 0x1FFu << 23;
                day.old = (old >> 23) & 0x1FFu;
                if (!w32(word, (old & ~mask) | ((DAYNUM & 0x1FFu) << 23)))
                    day.err = "GameState day write did not verify";
                if (day.err.empty())
                {
                    bool ok = w32(PENDINGID, 2);
                    if (ok) ok = w32(PENDINGARG, 0);
                    if (ok) ok = w32(obj + SCENEOBJ, 0xFFFFFFFE);
                    if (!ok) day.err = "Day 357 scene request write did not verify";
                }
                if (day.err.empty())
                    day.appl = true;
            }
        }
    }
}
}
