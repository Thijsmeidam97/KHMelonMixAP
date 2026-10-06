#include "State.h"
#include "../../Platform.h"
namespace Plugins::APC {

//test code not in use, needs cleanup as well to be readable
void Ctx::pollRewardsBits(){
    u16 mid = r16(0x0204C23C);
    u32 gs = r32(0x0204BE18);
    if(gs && mid) {
        u8 flags = get4(gs,mid);
        bool r1 = (flags & (1<<0)) != 0;
        bool r2 = (flags & (1<<1)) != 0;
        bool r3 = (flags & (1<<2)) != 0;
        bool r4 = (flags & (1<<3)) != 0;
        bool r5 = (flags & (1<<4)) != 0;
        bool r6 = (flags & (1<<5)) != 0;
        bool r7 = (flags & (1<<6)) != 0;
        bool r8 = (flags & (1<<7)) != 0;
}
}
void Ctx::logmine(const char *fmt,...){
    if (fmt == nullptr)
        return;

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    fflush(stdout);
}
u8 Ctx::get4(u32 gs, u32 mid)
{
    u32 off = 0x092B + mid * 4;
    u32 adr = gs + 0x10 + (off / 32) * 4;
    u32 bit = off & 31;
    u8 flags = 0;

    if (bit + 4 <= 32) {
        flags = (r32(adr) >> (28 - bit)) & 0x0F;
    }
    if (bit + 4 > 32) {
        u32 n = 32 - bit;
        u32 a = r32(adr) & ((1u << n) - 1);
        u32 b = r32(adr + 4) >> (32 - (4 - n));
        flags = (a << (4 - n)) | b;
    }
    return flags;
}
}
