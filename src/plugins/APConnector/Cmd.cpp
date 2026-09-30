#include "State.h"
#include "../../NDS.h"
#include "../../NDSCart.h"
#include <cmath>
#include <iomanip>
#include <set>
#include <sstream>
namespace Plugins::APC {
void Ctx::run() {
    ++frm;
    pollDay();
    pollPre();
    pollActiveFieldGoal();
    pollOpen();
    pollSig();
    pollItem();
    pollChar();
    pollGate();
    pollMish();
    pollMision();
    pollDays();
    showMsg();
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
    const bool busy = item.arm && !item.fire && item.err.empty();
    JVal out = fail(ok ? "A KH358 item grant is already pending" : "Invalid real KH358 item key");
    ok = ok && !busy;
    if (ok) {
        item = {};
        item.arm = true;
        item.key = key;
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
JVal Ctx::armHolo(const JVal& v) {
    const auto read = [](const JVal* val, u32 max, std::vector<u32>& out) {
        bool ok = val && val->kind == JVal::Kind::Arr && val->arr.size() <= 512;
        std::set<u32> seen;
        if (ok) {
            for (const JVal& x : val->arr) {
                u32 n = 0;
                if (!jInt(&x, n) || n < 1 || n > max || !seen.insert(n).second) ok = false;
                if (ok) out.push_back(n);
            }
        }
        return ok;
    };
    std::vector<u32> mish;
    std::vector<u32> unlock;
    std::vector<u32> days;
    bool ok = read(v.get("mission_ids"), 0xFFFF, mish);
    const JVal* u = v.get("unlock_mission_ids");
    if (ok && u) ok = read(u, 0xFFFF, unlock);
    if (!u) unlock.clear();
    std::set<u32> locked(mish.begin(), mish.end());
    for (u32 n : unlock)
        if (locked.count(n)) ok = false;
    if (ok) ok = read(v.get("days"), 358, days);
    JVal out = fail("invalid mission ID array");
    if (ok) {
        holo.mish = std::move(mish);
        holo.unlock = std::move(unlock);
        holo.days = std::move(days);
        holo.dayOn = !holo.days.empty();
        holo.cache.clear();
        holo.ctx = 0;
        out = reply("KH358_HOLO_FILTER_ARMED");
        out["mission_count"] = JVal::number(holo.mish.size());
        out["unlock_mission_count"] = JVal::number(holo.unlock.size());
        out["day_count"] = JVal::number(holo.days.size());
        out["mission_hook_armed"] = JVal::boolean(!holo.mish.empty() || !holo.unlock.empty());
        out["day_hook_armed"] = JVal::boolean(holo.dayOn);
    }
    return out;
}
JVal Ctx::setGate(const JVal& v) {
    bool goal = false;
    u32 done = 0;
    u32 need = 0;
    u32 total = 0;
    bool ok = jBool(v.get("unlocked"), goal) && jInt(v.get("completed_missions"), done) && jInt(v.get("required_missions"), need) && jInt(v.get("total_missions"), total) && need <= total && done <= total;
    JVal out = fail("Invalid Clocktower Examine AP goal state");
    if (ok) {
        gate.goal = goal;
        gate.done = done;
        gate.need = need;
        gate.total = total;
        if (goal) resetGate();
        out = reply("KH358_CLOCKTOWER_EXAMINE_GATE_SET");
        out["unlocked"] = JVal::boolean(goal);
        out["completed_missions"] = JVal::number(done);
        out["required_missions"] = JVal::number(need);
        out["total_missions"] = JVal::number(total);
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
        out["events"] = JVal::array();
        out["events"].arr = std::move(revs);
        revs.clear();
    }
    else if (ok && type == "KH358_HOLO_FILTER") out = armHolo(v);
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
        bool ok = p.parse(reqs, err) && reqs.kind == JVal::Kind::Arr;
        if (!ok)
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
