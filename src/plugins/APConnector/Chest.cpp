#include "State.h"
namespace Plugins::APC {

    void Ctx::pollReplaceChest() {

        u32 objectTable = r32(0x0207FA24);
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

    void Ctx::pollDetectOpenChests(){
        u32 objectTable = r32(0x0207FA24);
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
                            this->chest.states.resize(chestCount,0);
                            //timed event found, now to loop through each chest
                            for(u32 slot = 0; slot < chestCount; slot++){
                                u32 chest = moduleBase + slot * moduleSize;
                                u8 chestState = r8(chest + 0x1B4);
                                u8 previousState = this->chest.states[slot];

                                if((previousState == 3) && (chestState == 5)) {
                                    logmine("open");
                                    u8 opcode = 0x08;
                                    u8 size = 0x02;
                                    u16 mission = r16(MISSIONID);
                                    u8 chestSlot = r8(chest + OBJSLOT);
                                    u8 buffer[2];
                                    
                                    buffer[0] = static_cast<u8>(mission);
                                    buffer[1] = chestSlot;

                                    returnToApClient(opcode, size, buffer);
                                }
                                this->chest.states[slot] = chestState;
                            }
                        }
                    }
                }
            }
        }
    }
}
