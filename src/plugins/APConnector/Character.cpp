#include "State.h"
#include "../../Platform.h"
#include "missionDatabase.h"
namespace Plugins::APC {
void Ctx::pollChar() {
        const u32 scene = r32(SCENEID);
        if (scene == 2 || scene == 19) {
            const u32 liveSlot0Kind = r32(SESSIONTAB + 4);
            const u8 missionRecordKind = r8(MISSIONREC + MEMBERKIND);
            if (r32(SESSIONTAB)) {
                bool soraFlag = false;
                //check missions, if roxas is not present during sora flashbacks it black screen softlocks
                u16 missionId = r16(MISSIONID);
                for(const u16 id: soraFlashbackIds){
                    if (missionId == id) soraFlag = true;
                }
                if (!soraFlag) {
                    if(!setCharacterRandoFlag) {
                        for (HoloMissions &missiondb : missionDb) {
                            if(missiondb.missionID == missionId) { 
                                if (liveSlot0Kind != missiondb.setChar) w32((SESSIONTAB + 4), missiondb.setChar);
                                if (missionRecordKind != missiondb.setChar) w8((MISSIONREC + MEMBERKIND), missiondb.setChar);
                            }
                        }
                    } else {
                        //check both session and mission character to change
                        if (liveSlot0Kind != int(currentChar)) w32((SESSIONTAB + 4), int(currentChar));
                        if (missionRecordKind != int(currentChar)) w8((MISSIONREC + MEMBERKIND), int(currentChar));
                    }
                } else {
                    if (liveSlot0Kind != int(ROXAS)) w32((SESSIONTAB + 4), int(ROXAS));
                    if (missionRecordKind != int(ROXAS)) w8((MISSIONREC + MEMBERKIND), int(ROXAS));
                }
            }
        }
}
}
