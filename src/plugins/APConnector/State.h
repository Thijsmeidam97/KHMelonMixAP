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

    enum CharIds {
        ROXAS,
        AXEL,
        SAIX,
        XALDIN,
        SORA,
        DEMYX,
        LARXENE,
        LEXAEUS,
        LUXORD,
        MARLUXIA,
        RIKU,
        VEXEN,
        XEMNAS,
        XIGBAR,
        ZEXION,
        MICKEY,
        GOOFY,
        DONALD
    }currentChar = ROXAS;

    const u16 soraFlashbackIds[5] = {39, 47, 53, 56, 61};
    const u16 xionDenyText[5] = {78, 111, 46, 46, 46};
    float hpScalePercent = 100;
    float statScalePercent = 100; 
    missionGoal missionResult = NOT_ACHIEVED; 
    using Clock = std::chrono::steady_clock;
    melonDS::NDS* nds = nullptr;
    MsgCb cb;
    u8 get4(u32 gs, u32 mid);
    APCSock srv = APCBadSock;
    APCSock cli = APCBadSock;
    std::string rx;
    std::deque<std::string> msgs;
    bool gameCompleted = false;
    Clock::time_point last = Clock::now();
    Clock::time_point sent = Clock::now();
    double gap = 0;
    void pollCurrentMission();
    u64 frm = 0;
    u32 magicKeys[15] = {0x05D, 0x05E, 0x05F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x065, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B};
    u16 currentMission = 0;
    void logmine(const char* fmt, ...);
    bool lock = false;
    void enemyScan();
    void itemGive(u32 item);
    void pollMissionLocking();
    void RowRelease(u32 rows);
    void pollCalculateScaling();
    void printItem(u8 itemKey);
    bool on = false;
    bool ws = false;
    void closeCon();
    void closeSrv();
    bool send(const std::string& text);
    void start();
    void pollDayLocking();
    bool wsOk() const;
    void accept();
    void pollGameStart();
    u32 missionDone = 0;
    void setMissionLocked(u16 mid, bool hidden);
    void showMsg();
    void giveMagic(u8 magicKey, u8 amount);
    void dayJump();
    void procLine(const std::string& line);
    void readCli();
    void wait();
    void net();
    void clear();
    void readOption(u32 ptr, u16* opts,u16 length);
    bool compareOption(u16* source, const u16* comp,u16 length);
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
    void pollRewardsBits();
    bool addKeys();
    bool patchName(u32 root);
    void pollPre();
    bool isChest(u32 obj) const;
    void pollOpen();
    bool clearItem();
    bool filterSrc();
    void pollSig();
    void p16(u32 ptr);
    void pollGate();
    void pollMision();
    void pollActiveFieldGoal();
    void run();
    JVal req(const JVal& val);
    JVal armDay();
    JVal armItem(const JVal& val);
    JVal armChar(const JVal& val);
    JVal armPre(const JVal& val);
    JVal armSig(const JVal& val);
    JVal fireSig(const JVal& val);
    JVal setGate(const JVal& val);
    JVal setDone(const JVal& val);
    bool goalAchieved = false;
    bool totalAchieved = false;
    bool finalMissionUnlocked = false;
};
}
