#include "State.h"
#include "../../Platform.h"
namespace Plugins::APC {
bool withdrawn = false;
void Ctx::pollMision() {
    //Setting the mission to skip giving fixed and weighted table rewards. specifically the first bit of nOption64
    u8 flags = r8(RESULTREC + RESULTFLAGS);
    bool set = (flags & 1) == 0;
    if (set && (!withdrawn)) {
        flags = flags | 1;
        w8(RESULTREC + RESULTFLAGS, flags);
        u16 mish = r16(MISSIONID);
        bool mine = (mish == 37) || (mish == 91) || (mish == 88);
        if (missionResult == OVERACHIEVED || missionResult == ACHIEVED && !mine) {
            returnToApClient(0x07, int(missionResult));
            missionResult = NOT_ACHIEVED;
            withdrawn = false;
        }
        //some missions don't activated the field goal
        if(mine) {
            returnToApClient(0x07, int(missionResult));
            missionResult = NOT_ACHIEVED;
            withdrawn = false;
    }
    }
    if(set && withdrawn) {
        withdrawn = false;
    }
}

void Ctx::pollCurrentMission(){
    currentMission = r16(MISSIONID);
}
void Ctx::pollActiveFieldGoal(){
    bool result = false;
    u32 fieldContext = r32(FIELDCONTEXT);

    if (fieldContext != 0) {
        u32 total = 0;
        u32 done = 0;
        u32 goal = 0;
        total = r32(fieldContext + FIELDMISSIONTOTALOFFSET);
        goal = r32(fieldContext + FIELDMISSIONGOALOFFSET);
        done = r32(fieldContext + FIELDMISSIONDONEOFFSET);
        if (total !=0){
            bool goalAchieved = done >= goal;
            bool totalAchieved = done == total;
            if (goalAchieved) missionResult = totalAchieved ? OVERACHIEVED : ACHIEVED;

        } else missionResult = NOT_ACHIEVED;

        u32 gameState = r32(GAMESTATE);
        bool withdrew = false;
        if (gameState) {
            u8 flags = r8(gameState + 0x423);
            withdrew = (flags & 1) != 0;
            if(withdrew) {
                withdrawn = true;
                missionResult = NOT_ACHIEVED;
            }
         }
    }
}
}


