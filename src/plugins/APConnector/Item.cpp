#include "State.h"
namespace Plugins::APC {



void Ctx::itemGive(u32 itemKey) {
    u32 gameState = r32(GAMESTATE);

    if (gameState) {
        u8 currentItemCount = r8(gameState + ITEMCOUNT + itemKey);
        u8 newItemCount = currentItemCount + 1;
        w8(gameState + ITEMCOUNT + itemKey, newItemCount);


    }
}

void Ctx::RowRelease(u32 rows) {
    for (u8 i = 0; i < (rows* 5); i++) {
        itemGive(0x01);
    }
}

void Ctx::giveMagic(u8 magicKey, u8 amount) {
    for (u8 i = 0; i < amount; i++) {
        itemGive(magicKey);
    }
}

}
