#include "State.h"
#include "../../NDS.h"
#include "../../NDSCart.h"
#include <cmath>
#include <iomanip>
#include <set>
#include <sstream>
#include "tcpServer.h"
namespace Plugins::APC {
void Ctx::run() {
    ++frm;
    pollPre();
    pollActiveFieldGoal();
    // pollOpen();
    pollSig();
    pollChar();
    pollGameStart();
    pollGate();
    pollMision();
    pollCalculateScaling();
    enemyScan();
    pollRewardsBits();
    showMsg();
    pollMissionLocking();
    pollDayLocking();
    pollReplaceChest();
    pollDetectOpenChests();
    server.pollTCP();

}

void Ctx::commandHandling(u8 opcode, u8 size, u8 *buffer) {
    logmine("opcode %02X",opcode);
    switch(opcode) {
        case 0x01: {
            if (size == 2) {
                u16 item = buffer[2] | (buffer[3] << 8);
                giveItemType(item);
            }
            break;
        }
        case 0x02: setMissionLocked(buffer[2],buffer[3]); break;
        case 0x03: currentChar = static_cast<CharIds>(buffer[2]); break;
        case 0x04: missionDone = buffer[2]; break;
        case 0x05: finalMissionUnlocked = buffer[2]; break;
        case 0x06: setMultipleMissionLocked(size,buffer);

    }
}
void Ctx::returnToApClient(u8 opcode, u8 size, u8 * buffer) {
    std::vector<u8> packet{opcode, size};
    packet.insert(packet.end(), buffer, buffer + size);
    server.send(packet);

}

void Ctx::setMultipleMissionLocked(u8 amount, u8 *buffer) {
    for (u8 i = 0; i < (amount/2); i++) {
        setMissionLocked(buffer[2+(i*2)],buffer[3+(i*2)]);
    }
}


void Ctx::giveItemType(u16 item){
        bool magic = false;
        for (u8 i = 0; i < 15; i++) {
            magic |= magicKeys[i] == item;
        }
        if ((item != 1) && !magic) {
            itemGive(item);

        } 
        if( item == 1) {
            RowRelease(1);
        }
        if(magic) {
            giveMagic(item, 3);
        }
}
JVal Ctx::armDay() {
    const bool busy = day.arm && !day.appl && day.err.empty();
    const bool done = day.appl;
    JVal out = fail(busy ? "Day 357 support is already waiting for a stable scene" : "Day 357 support was already applied; load a state or ROM to reset it");
    if (!busy && !done) {
        day = {};
        day.arm = true;
        out = reply("KH358_DAY357_SUPPORT_ARMED");
        out["target_day"] = JVal::number(DAYNUM);
        out["stable_frames"] = JVal::number(STABLE);
    }
    return out;
}
JVal Ctx::armItem(const JVal& v) {
    u32 key = 0;
    bool ok = jInt(v.get("item_key"), key) && key >= 1 && key <= 0x276;
    // const bool busy = item.arm && !item.fire && item.err.empty();
    JVal out;
    // ok = ok && !busy;
    // if (ok) {
    //     item = {};
    //     item.arm = true;
    //     item.key = key;
    //     out = reply("KH358_ITEM_GRANT_ARMED");
    //     out["item_key"] = JVal::number(key);
    // }
    if (true ) {
        bool magic = false;
        for (u8 i = 0; i < 15; i++) {
            magic |= magicKeys[i] == key;
            logmine("%d %d\n", magicKeys[i], key);
        }
        if ((key != 1) && !magic) {
            itemGive(key);

        } 
        if( key == 1) {
            RowRelease(1);
        }
        if(magic) {
            giveMagic(key, 3);
        }
        printItem(key);
        out = reply("KH358_ITEM_GRANT_ARMED");
        out["item_key"] = JVal::number(key);
    }
    return out;
}
JVal Ctx::armChar(const JVal& v) {
    u32 kind = 0;
    const bool ok = jInt(v.get("member_kind"), kind) && kind <= 19;
    JVal out = fail("Invalid KH358 character member kind");
    if (ok) {
        chr = {};
        chr.arm = true;
        currentChar = static_cast<CharIds>(kind);
        chr.kind = kind;
        out = reply("KH358_CHARACTER_ARMED");
        out["member_kind"] = JVal::number(kind);
        out["applied"] = JVal::boolean(false);
    }
    return out;
}
JVal Ctx::armPre(const JVal& v) {
    u32 mish = 0;
    const JVal* ids = v.get("item_ids");
    bool ok = jInt(v.get("mission"), mish) && mish >= 1 && mish <= 0xFFFF && ids && ids->kind == JVal::Kind::Arr && !ids->arr.empty() && ids->arr.size() <= RECCAP;
    std::vector<u32> vals;
    std::set<u32> seen;
    if (ok) {
        for (const JVal& id : ids->arr) {
            u32 key = 0;
            if (!jInt(&id, key) || key < 1 || key > 0xFFFF || !seen.insert(key).second) ok = false;
            if (ok) vals.push_back(key);
        }
    }
    JVal out = fail("Invalid KH358 chest preload mission or item_ids");
    if (ok) {
        if (mish != pre.mish || vals != pre.ids) pre = {};
        pre.arm = true;
        pre.mish = mish;
        pre.ids = std::move(vals);
        const bool pool = addKeys();
        const u32 root = r32(ROOTCTX);
        if (root) patchName(root);
        out = reply("KH358_CHEST_PRELOAD_ARMED");
        out["mission"] = JVal::number(mish);
        out["item_count"] = JVal::number(pre.ids.size());
        out["pool_poll_armed"] = JVal::boolean(pool || pre.arm);
    }
    return out;
}
JVal Ctx::armSig(const JVal& v) {
    u32 key = 0;
    u32 dayv = 0;
    u32 mish = 0;
    u32 slot = 0;
    bool ok = jInt(v.get("item_key"), key) && key >= 1 && key <= 0xFFFF && jInt(v.get("day"), dayv) && dayv <= 358 && jInt(v.get("mission"), mish) && mish >= 1 && mish <= 0xFFFF && jInt(v.get("slot"), slot) && slot <= 0xFF;
    if (ok && (!sig.arm || key != sig.key || dayv != sig.day || mish != sig.mish || slot != sig.slot)) {
        sig = {};
        sig.arm = true;
        sig.key = key;
        sig.day = dayv;
        sig.mish = mish;
        sig.slot = slot;
        sig.base = itemCnt(key);
    }
    JVal out = fail("Invalid KH358 chest item signal key, mission, or slot");
    if (ok) {
        out = reply("KH358_CHEST_ITEM_SIGNAL_ARMED");
        out["armed"] = JVal::boolean(true);
        out["item_key"] = JVal::number(key);
        out["day"] = JVal::number(dayv);
        out["mission"] = JVal::number(mish);
        out["slot"] = JVal::number(slot);
        out["fired"] = JVal::boolean(sig.fire);
        out["virtual_item_safe"] = JVal::boolean(true);
    }
    return out;
}
JVal Ctx::fireSig(const JVal& v) {
    u32 key = 0;
    u32 mish = 0;
    u32 slot = 0;
    bool ok = jInt(v.get("item_key"), key) && jInt(v.get("mission"), mish) && jInt(v.get("slot"), slot) && key == CUSTOMKEY && sig.arm && sig.key == key && sig.mish == mish && sig.slot == slot;
    JVal out = fail("Custom chest item signal is not armed for this mission/slot");
    if (ok && !sig.fire) {
        sig.fire = true;
        sig.fkey = key;
        sig.fday = 0;
        sig.fmish = mish;
        sig.fslot = slot;
        sig.ffrm = frm;
        sig.src = filterSrc();
        patchName(r32(ROOTCTX));
    }
    if (ok) {
        out = reply("KH358_CHEST_ITEM_SIGNAL_FIRED");
        out["item_key"] = JVal::number(key);
        out["day"] = JVal::number(0);
        out["mission"] = JVal::number(mish);
        out["slot"] = JVal::number(slot);
        out["fired"] = JVal::boolean(true);
        out["frame"] = JVal::number((double)sig.ffrm);
    }
    return out;
}
JVal Ctx::setGate(const JVal&) {
    finalMissionUnlocked = true;
    return reply("KH358_CLOCKTOWER_EXAMINE_GATE_SET");
}
JVal Ctx::setDone(const JVal& v) {
    u32 done = 0;
    bool ok = jInt(v.get("missionDone"), done) && done <= 91;
    JVal out = fail("Invalid KH358 mission completed count");
    if (ok) {
        missionDone = done;
        out = reply("KH358_MISSION_DONE_SET");
        out["missionDone"] = JVal::number(missionDone);
    }
    return out;
}
JVal Ctx::req(const JVal& v) {
    JVal out = fail("request is not an object");
    bool ok = v.kind == JVal::Kind::Obj;
    std::string type;
    if (ok) ok = jStr(v.get("type"), type);
    if (ok && type == "PING") out = reply("PONG");
    else if (ok && type == "SYSTEM") {
        out = reply("SYSTEM_RESPONSE");
        out["value"] = JVal::string("NDS");
    }
    else if (ok && type == "PREFERRED_CORES") {
        out = reply("PREFERRED_CORES_RESPONSE");
        out["value"] = JVal::object();
    }
    else if (ok && type == "HASH") {
        out = reply("HASH_RESPONSE");
        const auto* cart = nds ? nds->GetNDSCart() : nullptr;
        if (!cart)
            out["value"] = JVal::nul();
        else {
            std::ostringstream h;
            h << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << cart->Checksum();
            out["value"] = JVal::string(h.str());
        }
    }
    else if (ok && type == "MEMORY_SIZE") {
        std::string dom;
        ok = jStr(v.get("domain"), dom);
        if (!ok)
            out = fail("missing memory domain");
        else {
            out = reply("MEMORY_SIZE_RESPONSE");
            if (dom == "ROM") out["value"] = JVal::number(nds && nds->GetNDSCart() ? nds->GetNDSCart()->GetROMLength() : 0);
            else if (dom == "ARM9 System Bus" || dom == "System Bus") out["value"] = JVal::number(4294967296.0);
            else out = fail("unsupported memory domain: " + dom);
        }
    }
    else if (ok && type == "GUARD") {
        u32 addr = 0;
        std::string enc;
        std::string dom;
        ok = jInt(v.get("address"), addr) && jStr(v.get("expected_data"), enc) && jStr(v.get("domain"), dom);
        if (!ok) out = fail("invalid GUARD request");
        else {
            std::vector<u8> want;
            std::vector<u8> got;
            std::string err;
            ok = b64dec(enc, want) && readMem(dom, addr, (u32)want.size(), got, err);
            if (!ok) out = fail(err.empty() ? "invalid GUARD data" : err);
            else {
                out = reply("GUARD_RESPONSE");
                out["address"] = JVal::number(addr);
                out["value"] = JVal::boolean(want == got);
            }
        }
    }
    else if (ok && type == "LOCK") {
        lock = true;
        out = reply("LOCKED");
    }
    else if (ok && type == "UNLOCK") {
        lock = false;
        out = reply("UNLOCKED");
    }
    else if (ok && type == "READ") {
        u32 addr = 0;
        u32 size = 0;
        std::string dom;
        ok = jInt(v.get("address"), addr) && jInt(v.get("size"), size) && jStr(v.get("domain"), dom);
        if (!ok) out = fail("invalid READ request");
        else {
            std::vector<u8> data;
            std::string err;
            ok = readMem(dom, addr, size, data, err);
            if (!ok) out = fail(err);
            else {
                out = reply("READ_RESPONSE");
                out["value"] = JVal::string(b64enc(data));
            }
        }
    }
    else if (ok && type == "WRITE") {
        u32 addr = 0;
        std::string dom;
        std::string enc;
        ok = jInt(v.get("address"), addr) && jStr(v.get("value"), enc) && jStr(v.get("domain"), dom);
        if (!ok) out = fail("invalid WRITE request");
        else {
            std::vector<u8> data;
            std::string err;
            ok = b64dec(enc, data) && writeMem(dom, addr, data, err);
            if (!ok) out = fail(err.empty() ? "invalid WRITE data" : err);
            else out = reply("WRITE_RESPONSE");
        }
    }
    else if (ok && type == "DISPLAY_MESSAGE") {
        std::string msg;
        ok = jStr(v.get("message"), msg);
        if (!ok) out = fail("invalid display message");
        else {
            msgs.push_back(std::move(msg));
            out = reply("DISPLAY_MESSAGE_RESPONSE");
        }
    }
    else if (ok && type == "SET_MESSAGE_INTERVAL") {
        const JVal* x = v.get("value");
        ok = x && x->kind == JVal::Kind::Num && std::isfinite(x->num) && x->num >= 0;
        if (!ok) out = fail("invalid message interval");
        else {
            gap = x->num;
            out = reply("SET_MESSAGE_INTERVAL_RESPONSE");
        }
    }
    else if (ok && type == "KH358_READ_MISSION_ID") {
        out = reply("KH358_MISSION_ID_RESPONSE");
        out["mission_id"] = JVal::number(r16(MISSIONID));
    }
    else if (ok && type == "KH358_DAY357_SUPPORT") out = armDay();
    else if (ok && type == "KH358_DAY357_SUPPORT_STATUS") {
        out = reply("KH358_DAY357_SUPPORT_STATUS");
        out["state"] = JVal::string(!day.err.empty() ? "error" : day.appl ? "applied" : day.arm ? "waiting" : "idle");
        out["stable_frames"] = JVal::number(day.stable);
        out["target_day"] = JVal::number(DAYNUM);
        out["old_day"] = day.appl ? JVal::number(day.old) : JVal::nul();
        out["err"] = day.err.empty() ? JVal::nul() : JVal::string(day.err);
    }
    else if (ok && type == "KH358_GIVE_ITEM") out = armItem(v);
    else if (ok && type == "KH358_ITEM_GRANT_STATUS") {
        out = reply("KH358_ITEM_GRANT_STATUS");
        out["state"] = JVal::string(!item.err.empty() ? "error" : item.fire ? "granted" : "waiting");
        out["item_key"] = item.arm ? JVal::number(item.key) : JVal::nul();
        out["old_count"] = item.fire ? JVal::number(item.oldc) : JVal::nul();
        out["new_count"] = item.fire ? JVal::number(item.newc) : JVal::nul();
        out["err"] = item.err.empty() ? JVal::nul() : JVal::string(item.err);
    }
    else if (ok && type == "KH358_SET_CHARACTER") out = armChar(v);
    else if (ok && type == "KH358_CHARACTER_STATUS") {
        out = reply("KH358_CHARACTER_STATUS");
        out["state"] = JVal::string(!chr.err.empty() ? "error" : chr.appl ? "applied" : "waiting");
        out["member_kind"] = chr.arm ? JVal::number(chr.kind) : JVal::nul();
        out["err"] = chr.err.empty() ? JVal::nul() : JVal::string(chr.err);
    }
    else if (ok && type == "KH358_CHEST_PRELOAD") out = armPre(v);
    else if (ok && type == "KH358_CHEST_ITEM_SIGNAL_ARM") out = armSig(v);
    else if (ok && type == "KH358_CHEST_ITEM_SIGNAL_FIRE") out = fireSig(v);
    else if (ok && type == "KH358_CHEST_ITEM_SIGNAL_STATUS") {
        out = reply("KH358_CHEST_ITEM_SIGNAL_STATUS");
        out["armed"] = JVal::boolean(sig.arm);
        out["fired"] = JVal::boolean(sig.fire);
        out["item_key"] = JVal::number(sig.fire ? sig.fkey : sig.key);
        out["day"] = JVal::number(sig.fire ? sig.fday : sig.day);
        out["mission"] = JVal::number(sig.fire ? sig.fmish : sig.mish);
        out["slot"] = JVal::number(sig.fire ? sig.fslot : sig.slot);
        out["caller"] = JVal::nul();
        out["frame"] = sig.fire ? JVal::number((double)sig.ffrm) : JVal::nul();
    }
    else if (ok && type == "KH358_CHEST_OPEN_STATUS") {
        bool sup = false;
        if (v.get("suppress_rewards") && jBool(v.get("suppress_rewards"), sup)) open.sup = sup;
        out = reply("KH358_CHEST_OPEN_STATUS");
        out["events"] = JVal::array();
        for (auto& ev : open.evs) out["events"].arr.push_back(std::move(ev));
        open.evs.clear();
        out["suppress_rewards"] = JVal::boolean(open.sup);
    }
    else if (ok && type == "KH358_MISSION_RESULT_STATUS") {
        out = reply("KH358_MISSION_RESULT_STATUS");
        out["gameCompleted"] = JVal::boolean(gameCompleted);
        out["events"] = JVal::array();
        out["events"].arr = std::move(revs);
        revs.clear();
    }
    else if (ok && type == "KH358_MISSION_DONE") out = setDone(v);
    else if (ok && type == "KH358_CLOCKTOWER_EXAMINE_GATE") out = setGate(v);
    else if (!ok) out = fail(type.empty() ? "request has no type" : "Unknown command: " + type);
    else out = fail("Unknown command: " + type);
    return out;
}
void Ctx::procLine(const std::string& line) {
    if (line == "VERSION")
        send("1\n");
    else {
        JVal reqs;
        std::string err;
        JParse p(line);
        bool ok = p.parse(reqs, err);
        if (ok && reqs.kind == JVal::Kind::Obj) {
            std::string type;
            u32 mid = 0;
            bool hidden = false;
            bool set = jStr(reqs.get("type"), type) && type == "setMissionLocked";
            set = set && jInt(reqs.get("missionID"), mid) && mid >= 1 && mid <= 91;
            set = set && jBool(reqs.get("hidden"), hidden);
            if (set) setMissionLocked((u16)mid, hidden);
        }
        else if (!ok || reqs.kind != JVal::Kind::Arr)
            send("[]\n");
        else {
            JVal out = JVal::array();
            JVal guard;
            bool failed = false;
            for (const JVal& val : reqs.arr) {
                JVal cur = failed ? guard : req(val);
                if (!failed && cur.kind == JVal::Kind::Obj) {
                    std::string type;
                    const JVal* t = cur.get("type");
                    const JVal* value = cur.get("value");
                    jStr(t, type);
                    if (type == "GUARD_RESPONSE" && value && value->kind == JVal::Kind::Bool && !value->b) {
                        failed = true;
                        guard = cur;
                    }
                }
                out.arr.push_back(std::move(cur));
            }
            send(dump(out) + "\n");
        }
    }
}
}
