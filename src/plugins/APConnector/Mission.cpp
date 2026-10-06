#include "State.h"
#include "missionDatabase.h"
namespace Plugins::APC {


void Ctx::pollMissionLocking(){
    u32 holoCTX = r32(HOLOCTX);

    if(holoCTX) {
        u32 missionPage = r32(holoCTX + PAGEA);
        if(missionPage) {
            u32 missionList = missionPage + LISTMGR + LISTPTR;

            u16 missionCount = r16(missionList + NODECNT);
            u32 mission = r32(missionList);

            if(mission && missionCount) {
                for (u16 i = 0; i < missionCount; i++) {
                    u16 missionID = r16(mission + NODEID);
                    //there are random floating ids of unused missions they've just hidden...
                    if(missionID < 95) {
                        for (HoloMissions &missiondb : missionDb) {
                            if (missiondb.missionID == missionID) {
                                u32 currentlyVisible = r32(mission + NODEFILTER);
                                if (missiondb.visible != currentlyVisible) {
                                    w32(mission + NODEFILTER, missiondb.visible);
                                }
                            }
                        }
                    }
                    u32 nextMission = r32(mission + NODENEXT);
                    //if nextMission is 0 that's the end of the node list
                    if(nextMission == 0) break;
                    mission = nextMission;
                }
            }
        }
    }

}
void Ctx::pollDayLocking(){
    u32 holoCTX = r32(HOLOCTX);

    if(holoCTX) {
        u32 dayPage = r32(holoCTX + PAGEA);
        if(dayPage) {
            u32 dayList = r32(dayPage + LISTMGR + MGRRES);
            u16 dayCount = r16(dayList);
            u32 day = dayList + 2;
            for (u16 i = 0; i < dayCount; i++) {
                u16 dayNumber = r16(day + 0x06);
                bool hide = true;
                for (HoloMissions &missiondb : missionDb){ 
                    if (missiondb.day == dayNumber) {
                        hide &= missiondb.visible;
                    }
                }
                if (hide) {
                    w16(day + 0x0C,hide);
                }
                u16 daySize = r16(day);
                day += daySize;
            }
        }
    }

}

void Ctx::setMissionLocked(u16 missionID, bool hidden) {
    for (HoloMissions& missiondb : missionDb)
        if (missiondb.missionID == missionID) missiondb.visible = hidden;
}
}