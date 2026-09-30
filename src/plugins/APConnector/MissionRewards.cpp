#include "State.h"
#include "../../Platform.h"
namespace Plugins::APC {

int count = 0;
void Ctx::pollMision() {
    //Setting the mission to skip giving fixed and weighted table rewards. specifically the first bit of nOption64
    u8 flags = r8(RESULTREC + RESULTFLAGS);
    bool set = (flags & 1) == 0;
    if (set) {
        flags = flags | 1;
        w8(RESULTREC + RESULTFLAGS, flags);
        if (missionResult == OVERACHIEVED || missionResult == ACHIEVED) {
            u16 mish = r16(MISSIONID);
            JVal ev = JVal::object();
            ev["mission"] = JVal::number(mish);
            ev["result"] = JVal::number((int)missionResult);
            if (revs.size() >= 64) revs.erase(revs.begin());
            revs.push_back(std::move(ev));
            missionResult = NOT_ACHIEVED;
        }
    }
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
            if(count >= 240) {
                melonDS::Platform::Log(melonDS::Platform::LogLevel::Debug, "goal= %d\n", goal);
                melonDS::Platform::Log(melonDS::Platform::LogLevel::Debug, "total= %d\n", total);
                melonDS::Platform::Log(melonDS::Platform::LogLevel::Debug, "done= %d\n", done);
                fflush(stdout);
                count = 0;
            }

        } else missionResult = NOT_ACHIEVED;
    }
count++;
}
}


