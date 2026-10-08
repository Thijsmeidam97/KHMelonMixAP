#include "State.h"
#include "../../Platform.h"
namespace Plugins::APC {
u16 count = 0;
void Ctx::p16(u32 ptr)
{
    char out[64];
    u32 i;
    u16 c;

    i = 0;
    while (i < 63) {
        c = r16(ptr + i * 2);
        if (c == 0) break;

        if (c < 0x80) {
            out[i] = (char)c;
        } else {
            out[i] = '?';
        }

        i++;
    }

    out[i] = 0;
}

void Ctx::pollGate() {
    // Look at option 1 and press A if it matches a known Xion option.
    if (!finalMissionUnlocked) {
        u32 ctx = r32(HUDCTX);
        if (ctx) {
            u32 opt1 = r32(ctx + HUDCTXSECONDENTRY);
            if (opt1) {
                u16 option1[5];
                bool xionCheck;
                readOption(opt1, option1, 5);
                xionCheck = compareOption(option1, xionDenyText, 5);
                if (xionCheck) {
                    u32 state = r32(ctx);
                    u32 count = r32(ctx + HUDCTXCOUNT);
                    u32 cursor = r32(ctx + HUDCTXCURSOR);
                    if (cursor == 1 && count > 1 && state == 4) {
                        w16(KEYINPUTMASK, 0x0001);
                    }
                }
            }
        }
    }
    // Check if scene 8 is activated, this activates the roxas kh2 cutscene after riku and the credits
    u32 scene = r32(0x0204BDB0);
    if (scene == 8) {
        gameCompleted = true;
        u8 opcode = 0x09;
        u8 size = 0x01;
        u8 completed[1] = {0x01} ;
        returnToApClient(opcode, size, completed);
    }
}
void Ctx::readOption(u32 ptr, u16* opts,u16 length)
{
    for (u16 i = 0; i < length; i++)
    {
        opts[i] = r16(ptr + i * 2);
    }
}


bool Ctx::compareOption(u16* source, const u16* comp,u16 length){
    bool result = true;
    for(u16 i = 0; i < length; i++) {
        result &= source[i] == comp[i];
    }
    return result;
}



}//end