#include "State.h"
namespace Plugins::APC {
u32 Ctx::actor() const {
    const u32 tab = r32(ACTORCTX + ACTORTAB);
    const u32 ent = r32(tab + ACTORENT);
    const u32 act = r32(ent + ACTORPTR);
    return ram(tab, 8) && ram(ent, 0x24) && ram(act, ACTMODE + 1) ? act : 0;
}
void Ctx::release() {
    if (gate.act) {
        const u32 act = actor();
        if (act) w32(act + ACTFLAGS, r32(act + ACTFLAGS) & ~0x2000u);
        gate.act = false;
    }
}
void Ctx::resetGate() {
    release();
    gate.arm = false;
    gate.cancel = false;
    gate.age = 0;
    gate.panel = 0;
}
void Ctx::pollGate() {
    if (gate.goal) {
        if (gate.arm || gate.act) resetGate();
    }
    else if (r8(WORLD)) resetGate();
    else {
        const u32 panel = r32(PANELCTX);
        if (ram(panel, PANELSTATE + 4)) {
            if (r16(panel + PANELMODE) == 9 && r8(panel + PANELSTATE) == 4) {
                gate.panel = panel;
                gate.arm = true;
                gate.age = 0;
                gate.cancel = false;
            }
            if (gate.arm) {
                ++gate.age;
                const u32 act = actor();
                if (act) {
                    const u32 flags = r32(act + ACTFLAGS);
                    if (!(flags & 0x2000u) && w32(act + ACTFLAGS, flags | 0x2000u)) gate.act = true;
                    const u8 kind = r8(act + ACTKIND);
                    const u8 af = r8(act + ACTMODE);
                    const u16 pending = r16(act + ACTPEND);
                    const u16 btn = r16(act + ACTBTN);
                    if (kind == 9 || pending || (btn & 1) || (af & 4)) {
                        w16(act + ACTPEND, 0);
                        w16(act + ACTBTN, btn & ~0x0C03u);
                        w8(act + ACTKIND, 7);
                        w8(act + ACTMODE, af & ~4u);
                        gate.cancel = true;
                    }
                }
                const u8 state = r8(gate.panel + PANELSTATE);
                if (state >= 1 && state <= 4) w8(gate.panel + PANELSTATE, 0);
                if (gate.age > 60) resetGate();
            }
        }
    }
}
}
