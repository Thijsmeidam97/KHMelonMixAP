#include "State.h"
#include <unordered_set>
namespace Plugins::APC {
u32 frameCount = 0;
bool ran = true;
void Ctx::dayJump() {

    u32 gamestate = r32(GAMESTATE);
    if (gamestate) {
        u32 dayBitMask = 357 << 23;
        u32 gamestateBitField = gamestate + 0x10;
        u32 currentGamestateBitField = r32(gamestateBitField);
        u32 mask = ~(511 << 23);
        currentGamestateBitField &= mask;
        currentGamestateBitField |= 357 << 23;
        w32(gamestateBitField, (currentGamestateBitField));
        //transition wizardry, prevents xion cutscene
        w8(0x0204C241,0);
        w16(0x0204C242, 0x2711);
        w16(0x0204C244,0);
        w32(PENDINGID, 2);              // pending scene ID
        w32(PENDINGARG, 0);              // pending argument
        u32 scenePointer = r32(SCENEREC);
        if(scenePointer){
            w32(scenePointer + SCENEOBJ, 0xFFFFFFFE);
        }     // scene object state
    }

}
void Ctx::pollGameStart() {
    bool day7 = false;
    u32 gamestate = r32(GAMESTATE);
    if (gamestate) {
        u32 dayBitMask = 0xFF1u <<23;
        u32 gamestateBitField = gamestate + 0x10;
        u32 day = (r32(gamestateBitField));
        day = day >> 23;
        day7 = day == 7;
    }
    u32 fieldContext = r32(FIELDCONTEXT);
    if(fieldContext) {
        u16 fieldIsActive = r16(fieldContext + FIELDISACTIVEOFFSET);
        if (fieldIsActive == 0){
            frameCount++;
            if(day7) {
                dayJump();
            }
        }
    }
}
}