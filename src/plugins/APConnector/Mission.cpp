#include "State.h"
#include "missionDatabase.h"
namespace Plugins::APC {


    //skip cutscene is rough experiment, needs proper rewrite. skips when you just press start
#define PAUSECTX 0x0204BE0C
#define PAUSECALL 0x0204BD8C
#define PAUSEARG 0x0204BD98
#define PAUSEFRAME 0x02022D7D
#define SKIPFRAME 0x02022FE5
#define HANDOFF 0xE0
#define SKIPDEST 0x584
#define SKIPREQ 0x588
void Ctx::pollSkipCutscene(){
    u32 currentScene = r32(0x0208A788);
    static u8 delay = 0;
    bool paused = r8(0x0204BD84) != 0;
    u32 sceneState = currentScene + 0x4B88;
    u32 pause = r32(PAUSECTX);
    bool menu = r32(PAUSECALL) == PAUSEFRAME;
    bool skip = r32(sceneState + SKIPDEST) != 0;
    if (paused && pause && menu && skip) {
        w32(sceneState + SKIPREQ, 1);
        w32(pause + HANDOFF, 1);
        w32(PAUSEARG, 0);
        w32(PAUSECALL, SKIPFRAME);
    } 
}

void Ctx::pollMissionLocking(){
    u32 holoCTX = r32(HOLOCTX);

    if(holoCTX) {
        u32 missionPage = r32(holoCTX + PAGEA);
        if(missionPage) {
            u32 missionList = missionPage + LISTMGR + LISTPTR;

            u16 missionCount = r16(missionList + NODECNT);
            u32 mission = r32(missionList);
            u16 visibleCount = 0;
            if(mission && missionCount) {
                for (u16 i = 0; i < missionCount; i++) {
                    u16 missionID = r16(mission + NODEID);
                    //there are random floating ids of unused missions they've just hidden...
                    if(missionID >= 1 && missionID <= 91) {
                        for (HoloMissions &missiondb : missionDb) {
                            if (missiondb.missionID == missionID) {
                                u32 currentlyVisible = r32(mission + NODEFILTER);
                                if (missiondb.visible != currentlyVisible) {
                                    w32(mission + NODEFILTER, missiondb.visible);
                                }
                                if(missiondb.visible == 0) visibleCount++; 
                            }
                        }
                        //Update the visible mission list. otherwise it tries to scroll to a visible mission on the wraparound and crash
                    } else {
                        u32 currentlyVisible = r32(mission + NODEFILTER);
                        if(currentlyVisible != 1) w32(mission + NODEFILTER, 1); 
                    }
                    u32 nextMission = r32(mission + NODENEXT);
                    //if nextMission is 0 that's the end of the node list
                    if(nextMission == 0) break;
                    mission = nextMission;
                }
            }
            u32 listManager = missionPage + LISTMGR;
            u16 oldCount = r16(listManager + LISTVIS);
            if (oldCount != visibleCount) w16(listManager + LISTVIS, visibleCount);
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
                w16(day + 0x0C,hide);
                u16 daySize = r16(day);
                day += daySize;
            }
        }
    }

}

void Ctx::setMissionLocked(u16 missionID, bool hidden) {
    logmine("mission id %d, hidden %d\n", missionID, hidden);
    for (HoloMissions& missiondb : missionDb)
        if (missiondb.missionID == missionID) missiondb.visible = hidden;
}

}
