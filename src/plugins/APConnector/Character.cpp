#include "State.h"
namespace Plugins::APC {
void Ctx::pollChar() {
    if (chr.arm && chr.err.empty()) {
        const u32 scene = r32(SCENEID);
        if (scene == 2 || scene == 19) {
            const u32 sa = SESSIONTAB + 4;
            const u32 ma = MISSIONREC + MEMBERKIND;
            const u32 sk = r32(sa);
            const u8 mk = r8(ma);
            if (r32(SESSIONTAB)) {
                if (sk != chr.kind && !w32(sa, chr.kind)) chr.err = "session member-kind write did not verify";
                if (chr.err.empty() && mk != chr.kind && !w8(ma, (u8)chr.kind)) chr.err = "mission member-kind write did not verify";
                chr.appl = chr.err.empty() && r32(sa) == chr.kind && r8(ma) == chr.kind;
            }
        }
    }
}
}
