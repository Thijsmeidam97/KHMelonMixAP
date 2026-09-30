#include "State.h"
#include <algorithm>
#include <set>
namespace Plugins::APC {
int Ctx::itemCnt(u32 key) const {
    const u32 st = r32(GAMESTATE);
    return ram(st, ITEMCOUNT + key + 1) ? r8(st + ITEMCOUNT + key) : -1;
}
bool Ctx::seenAddr(u32 st, u32 key, u32& addr, u8& mask) const {
    const u32 id = ITEMFLAG + key;
    const u32 word = id / 32;
    const u32 byte = 3 - (id % 32) / 8;
    addr = st + STATEWORD + word * 4 + byte;
    mask = (u8)(1u << (7 - id % 8));
    return ram(st, ITEMCOUNT + key + 1) && ram(addr);
}
void Ctx::pollItem() {
    if (item.arm && !item.fire && item.err.empty()) {
        const u32 scene = r32(SCENEID);
        const u32 st = r32(GAMESTATE);
        const u32 addr = st + ITEMCOUNT + item.key;
        bool ok = (scene == 2 || scene == 19) && ram(st, ITEMCOUNT + item.key + 1);
        if (ok) {
            const u8 cnt = r8(addr);
            if (cnt <= 99) {
                item.oldc = cnt;
                item.newc = cnt;
                if (cnt == 99)
                    item.fire = true;
                else {
                    u32 faddr = 0;
                    u8 fmask = 0;
                    ok = seenAddr(st, item.key, faddr, fmask);
                    if (ok) {
                        const u8 flags = r8(faddr);
                        const u8 next = (u8)(cnt + 1);
                        ok = w8(addr, next);
                        if (!ok) item.err = "count write did not verify";
                        if (ok && !(flags & fmask)) {
                            ok = w8(faddr, (u8)(flags | fmask));
                            if (!ok) item.err = "owned-flag write did not verify";
                        }
                        if (ok && r8(addr) != next) {
                            ok = false;
                            item.err = "count verification failed";
                        }
                        if (ok) {
                            item.newc = next;
                            item.fire = true;
                        }
                    }
                }
            }
        }
    }
}
bool Ctx::addKeys() {
    bool ok = pre.arm && !pre.ids.empty() && r16(MISSIONID) == pre.mish;
    if (ok) {
        const u32 root = r32(ROOTCTX);
        ok = ram(root, ROOTREC + ROOTSPAN);
        if (ok) {
            const u32 rec = root + ROOTREC;
            const u8 cnt = r8(rec + RECNAME);
            ok = cnt <= RECCAP;
            std::set<u32> have;
            for (u32 i = 0; i < cnt && ok; ++i) have.insert(r16(rec + RECIDS + i * 2));
            std::vector<u32> add;
            for (u32 key : pre.ids)
                if (have.insert(key).second) add.push_back(key);
            ok = cnt + add.size() <= RECCAP;
            for (size_t j = 0; j < add.size() && ok; ++j)
                ok = w16(rec + RECIDS + (cnt + j) * 2, (u16)add[j]);
            ok = ok && r16(MISSIONID) == pre.mish && r32(ROOTCTX) == root;
            if (ok && !add.empty()) ok = w8(rec + RECNAME, (u8)(cnt + add.size()));
            pre.root = root;
        }
    }
    return ok;
}
bool Ctx::patchName(u32 root) {
    bool ok = std::find(pre.ids.begin(), pre.ids.end(), CUSTOMKEY) != pre.ids.end() && ram(root, ROOTREC + ROOTSPAN);
    if (ok) {
        const u32 rec = root + ROOTREC;
        const u32 tab = r32(rec);
        const u8 cnt = r8(rec + RECLOAD);
        ok = ram(tab) && cnt && cnt <= 0x80 && ram(tab, cnt * RECSTEP);
        bool found = false;
        const std::string name = "Custom Item";
        for (u32 i = 0; i < cnt && ok; ++i) {
            const u32 ent = tab + i * RECSTEP;
            if (r16(ent + ITEMID) == CUSTOMKEY) {
                found = true;
                for (u32 j = 0; j < ITEMCHARS && ok; ++j)
                    ok = w16(ent + j * 2, j < name.size() ? (u8)name[j] : 0);
            }
        }
        ok = ok && found;
    }
    return ok;
}
void Ctx::pollPre() {
    if (pre.arm && pre.mish) {
        const u32 root = r32(ROOTCTX);
        if (r16(MISSIONID) == pre.mish && ram(root, ROOTREC + ROOTSPAN) && pre.root != root) {
            addKeys();
            patchName(root);
        }
    }
}
}
