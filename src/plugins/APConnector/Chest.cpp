#include "State.h"
#include <unordered_set>
namespace Plugins::APC {

    void Ctx::pollReplaceChest() {

        u32 objectTable = r32(0x0207FA24);
        int32_t slot = 0;
        if (objectTable) {
            for (u32 mod = 0; mod < 32; mod++) {
                //first identify which module the timed chest event of the map is
                u32 modulePool = r32(objectTable + 0x17C + mod * 4);
                if (modulePool) {
                    u16 moduleKind = r16(modulePool + 0x4C);
                    u16 moduleSize = r16(modulePool + 0x4E);
                    if(moduleKind == 0x0B && moduleSize == 0x1BC) {
                        u32 moduleBase = r32(modulePool + 0x54);
                        u16 chestCount = r16(modulePool + 0x50);
                        if(moduleBase) {
                            //timed event found, now to loop through each chest
                            for(u32 slot = 0; slot < chestCount; slot++){
                                u32 chest = moduleBase + slot * moduleSize;
                                u16 flags = r16(chest + 0x12);
                                u32 handler = r32(chest + 0x0C);
                                //is the handler the reward chest pointer
                                if(flags && (handler == 0x0207A0C8)) {
                                    u8 chestState = r8(chest + 0x1B4);
                                    u16 chestKey = r16(chest + 0x1B6 );
                                    if(chestState == 3 && chestKey != 0x00) {
                                        w16(chest + 0x1B6, 0);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
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
