#include "State.h"
namespace Plugins::APC {
bool Ctx::clearItem() {
    const u32 st = r32(GAMESTATE);
    const u32 addr = st + ITEMCOUNT + CUSTOMKEY;
    bool ok = ram(st, ITEMCOUNT + CUSTOMKEY + 1) && r8(addr);
    if (ok) ok = w8(addr, 0);
    return ok;
}
bool Ctx::filterSrc() {
    const u32 tab = r32(ITEMCFG + QTYOFF);
    bool ok = ram(tab, QTYNUM * 4);
    bool out = false;
    for (u32 i = 0; i < QTYNUM && ok; ++i) {
        if (r16(tab + i * 4) == CUSTOMKEY)
            out = (w16(tab + i * 4, 0) && w16(tab + i * 4 + 2, 0)) || out;
    }
    return ok && out;
}
void Ctx::pollSig() {
    if (sig.arm) {
        if (!sig.fire && sig.key != CUSTOMKEY) {
            const int cnt = itemCnt(sig.key);
            if (cnt >= 0) {
                if (sig.base < 0) sig.base = cnt;
                else if (cnt > sig.base) {
                    sig.fire = true;
                    sig.fkey = sig.key;
                    sig.fday = sig.day;
                    sig.fmish = sig.mish;
                    sig.fslot = sig.slot;
                    sig.ffrm = frm;
                }
                else if (cnt < sig.base) sig.base = cnt;
            }
        }
        if (sig.fire && sig.fkey == CUSTOMKEY) {
            if (!sig.src) sig.src = filterSrc();
            clearItem();
            const u32 ctx = r32(ITEMCTX);
            if (ram(ctx, ITEMLIST + ITEMNUM * ITEMSTEP)) sig.src = true;
        }
    }
}
}
