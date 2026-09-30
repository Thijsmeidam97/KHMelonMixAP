#include "State.h"
#include <unordered_set>
namespace Plugins::APC {
bool Ctx::isChest(u32 obj) const {
    bool ok = ram(obj, 0x1BC);
    if (ok) {
        const u32 klass = r32(obj + OBJCLASS);
        ok = ram(klass, 0x50) && r32(obj + OBJTYPE) == PICKTICK && r16(klass + CLASSID) == 0x000B && r16(klass + CLASSSIZE) == 0x01BC;
    }
    return ok;
}
void Ctx::pollOpen() {
    const u32 mish = r16(MISSIONID);
    const u32 root = r32(ROOTCTX);
    const u32 list = r32(SCENEROOT + SCENELIST);
    if (!ram(root) || !ram(list, LISTBUCKET * 4)) {
        open = {};
    }
    else {
        if (mish != open.mish || root != open.root || list != open.list) {
            open.sts.clear();
            open.init = false;
            open.mish = mish;
            open.root = root;
            open.list = list;
        }
        const auto old = open.sts;
        std::unordered_map<u32, u8> next;
        std::unordered_set<u32> seen;
        u32 n = 0;
        for (u32 bucket = 0; bucket < LISTBUCKET; ++bucket) {
            u32 obj = r32(list + bucket * 4);
            for (u32 depth = 0; ram(obj, 8) && seen.insert(obj).second && depth < 512 && n < 4096; ++depth) {
                const u32 after = r32(obj + OBJLINK);
                if (isChest(obj)) {
                    const u8 state = r8(obj + OBJSTATE);
                    const u8 slot = r8(obj + OBJSLOT);
                    if (open.sup && state == 3 && r16(obj + OBJREWARD)) w16(obj + OBJREWARD, 0);
                    const auto it = old.find(obj);
                    if (open.init && it != old.end() && it->second == 3 && state != 3) {
                        JVal ev = JVal::object();
                        ev["mission"] = JVal::number(mish);
                        ev["slot"] = JVal::number(slot);
                        ev["frame"] = JVal::number((double)frm);
                        if (open.evs.size() >= 64) open.evs.erase(open.evs.begin());
                        open.evs.push_back(std::move(ev));
                    }
                    next[obj] = state;
                }
                obj = after;
                ++n;
            }
        }
        open.sts = std::move(next);
        open.init = true;
    }
}
}
