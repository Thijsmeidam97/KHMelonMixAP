#pragma once
#include "Addr.h"
#include "Json.h"
#include "Net.h"
#include <chrono>
#include <deque>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
namespace melonDS { class NDS; }
namespace Plugins::APC {
using MsgCb = std::function<void(const std::string&)>;
class Ctx {
public:
    explicit Ctx(MsgCb fn);
    ~Ctx();
    void setNds(melonDS::NDS* value);
    void setOn(bool value);
    void loadRom();
    void loadSt();
    void poll();
private:
    struct Item {
        bool arm, fire;
        u32 key, oldc, newc;
        std::string err;
    } item{};
    struct Char {
        bool arm, appl;
        u32 kind;
        std::string err;
    } chr{};
    struct Pre {
        bool arm;
        u32 mish, root;
        std::vector<u32> ids;
    } pre{};
    struct Sig {
        bool arm, fire, src;
        u32 key, day, mish, slot;
        int base;
        u32 fkey, fday, fmish, fslot;
        u64 ffrm;
    } sig{};
    struct Open {
        bool init, sup;
        u32 mish, root, list;
        std::unordered_map<u32, u8> sts;
        std::vector<JVal> evs;
    } open{};
    std::vector<JVal> revs;
    struct Gate {
        bool goal, arm, act, cancel;
        u32 done, need, total, panel, age;
    } gate{};
    struct Holo {
        std::vector<u32> mish, unlock, days;
        bool dayOn;
        std::string cache;
        u32 vis, ctx, page, rows, cnt;
    } holo{};
    struct Day {
        bool arm, appl;
        u32 stable, old;
        std::string err;
    } day{};
    typedef enum{
        NOT_ACHIEVED = 0,
        ACHIEVED = 1,
        OVERACHIEVED = 2
    } missionGoal;

    missionGoal missionResult = NOT_ACHIEVED; 
    using Clock = std::chrono::steady_clock;
    melonDS::NDS* nds = nullptr;
    MsgCb cb;
    APCSock srv = APCBadSock;
    APCSock cli = APCBadSock;
    std::string rx;
    std::deque<std::string> msgs;
    Clock::time_point last = Clock::now();
    Clock::time_point sent = Clock::now();
    double gap = 0;
    u64 frm = 0;
    bool lock = false;
    bool on = false;
    bool ws = false;
    void closeCon();
    void closeSrv();
    bool send(const std::string& text);
    void start();
    bool wsOk() const;
    void accept();
    void showMsg();
    void procLine(const std::string& line);
    void readCli();
    void wait();
    void net();
    void clear();
    bool readMem(const std::string& dom, u32 addr, u32 size, std::vector<u8>& out, std::string& err) const;
    bool writeMem(const std::string& dom, u32 addr, const std::vector<u8>& data, std::string& err);
    u8 r8(u32 addr) const;
    u16 r16(u32 addr) const;
    u32 r32(u32 addr) const;
    bool w8(u32 addr, u8 val);
    bool w16(u32 addr, u16 val);
    bool w32(u32 addr, u32 val);
    void pollDay();
    int itemCnt(u32 key) const;
    bool seenAddr(u32 st, u32 key, u32& addr, u8& mask) const;
    void pollItem();
    void pollChar();
    bool addKeys();
    bool patchName(u32 root);
    void pollPre();
    bool isChest(u32 obj) const;
    void pollOpen();
    bool clearItem();
    bool filterSrc();
    void pollSig();
    u32 actor() const;
    void release();
    void resetGate();
    void pollGate();
    void pollMish();
    void pollMision();
    void pollActiveFieldGoal();
    void pollDays();
    void run();
    JVal req(const JVal& val);
    JVal armDay();
    JVal armItem(const JVal& val);
    JVal armChar(const JVal& val);
    JVal armPre(const JVal& val);
    JVal armSig(const JVal& val);
    JVal fireSig(const JVal& val);
    JVal armHolo(const JVal& val);
    JVal setGate(const JVal& val);
    bool goalAchieved = false;
    bool totalAchieved = false;
};
}
