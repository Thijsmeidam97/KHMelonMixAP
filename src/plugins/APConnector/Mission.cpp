#include "State.h"
#include <algorithm>
#include <sstream>
#include <unordered_set>

namespace Plugins::APC {
void Ctx::pollMish() {
    bool run = !holo.mish.empty() || !holo.unlock.empty();
    const u32 ctx = r32(HOLOCTX);
    run = run && ram(ctx, PAGEA + 4);
    if (run) {
        const u32 page = r32(ctx + PAGEA);
        run = ram(page, LISTMGR + MGRSIZE);
        if (run) {
            const u32 mgr = page + LISTMGR;
            const u32 list = mgr + LISTPTR;
            const u32 head = r32(list);
            const u16 vis = r16(mgr + LISTVIS);
            const u16 cnt = r16(list + NODECNT);
            const u16 link = r16(list + NODELINK);
            run = cnt && cnt <= 512 && vis <= cnt && link == NODELINKOFF && ram(head, NODESIZE);
            if (run) {
                std::ostringstream sig;
                sig << ctx << ':' << page << ':' << mgr << ':' << head << ':' << cnt << ':' << link << ':' << r32(mgr + MGRRES);
                const std::string key = sig.str();
                run = key != holo.cache || vis != holo.vis;
                if (run) {
                    struct T { u32 addr; u32 id; u32 mark; u32 want; };
                    std::vector<T> all;
                    std::unordered_set<u32> seen;
                    u32 node = head;
                    u32 cur = 0;
                    bool good = true;
                    for (u32 i = 0; i < cnt && good; ++i) {
                        good = ram(node, NODESIZE) && seen.insert(node).second;
                        if (good) {
                            const u32 id = r16(node + NODEID);
                            const u32 mark = r32(node + NODEFILTER);
                            const bool locked = std::find(holo.mish.begin(), holo.mish.end(), id) != holo.mish.end();
                            const bool unlocked = std::find(holo.unlock.begin(), holo.unlock.end(), id) != holo.unlock.end();
                            if (!mark) ++cur;
                            if (locked || unlocked) all.push_back({node, id, mark, locked ? 1u : 0u});
                            if (i + 1 < cnt) {
                                node = r32(node + NODENEXT);
                                good = node != 0;
                            }
                        }
                    }
                    good = good && cur == vis;
                    if (!holo.unlock.empty())
                        good = good && std::any_of(all.begin(), all.end(), [](const T& t) { return !t.want; });
                    std::vector<T> changed;
                    for (const T& t : all)
                        if (t.mark != t.want) changed.push_back(t);
                    if (good && changed.empty()) {
                        holo.cache = key;
                        holo.vis = vis;
                    }
                    if (good && !changed.empty()) {
                        const u32 hidden = (u32)std::count_if(changed.begin(), changed.end(), [](const T& t) { return !t.mark; });
                        const u32 shown = (u32)std::count_if(changed.begin(), changed.end(), [](const T& t) { return !t.want; });
                        const u32 newVis = vis - hidden + shown;
                        good = newVis > 0 && newVis <= cnt && r32(HOLOCTX) == ctx && r32(ctx + PAGEA) == page && r32(list) == head && r16(list + NODECNT) == cnt && r16(mgr + LISTVIS) == vis;
                        std::vector<T> done;
                        for (const T& t : changed) {
                            if (good) {
                                const bool set = r16(t.addr + NODEID) == t.id && r32(t.addr + NODEFILTER) == t.mark && w32(t.addr + NODEFILTER, t.want);
                                if (!set) good = false;
                                if (set) done.push_back(t);
                            }
                        }
                        if (good) good = w16(mgr + LISTVIS, (u16)newVis);
                        for (const T& t : done)
                            good = good && r16(t.addr + NODEID) == t.id && r32(t.addr + NODEFILTER) == t.want;
                        if (!good) {
                            for (const T& t : done) w32(t.addr + NODEFILTER, t.mark);
                            w16(mgr + LISTVIS, vis);
                            holo.cache.clear();
                        }
                        else {
                            holo.cache = key;
                            holo.vis = newVis;
                        }
                    }
                }
            }
        }
    }
}
void Ctx::pollDays() {
    bool run = !holo.days.empty();
    const u32 ctx = r32(HOLOCTX);
    run = run && ram(ctx, PAGEB + 4);
    if (run) {
        const u32 page = r32(ctx + PAGEB);
        run = ram(page, ROWCNT + 4);
        if (run) {
            const u32 rows = r32(page + ROWPTR);
            const u32 cnt = r32(page + ROWCNT);
            run = cnt >= 2 && cnt <= 56 && ram(rows, cnt * ROWSIZE);
            run = run && !(ctx == holo.ctx && page == holo.page && rows == holo.rows && cnt == holo.cnt);
            std::vector<u8> data;
            std::string err;
            run = run && readMem("ARM9 System Bus", rows, cnt * ROWSIZE, data, err);
            std::vector<u8> packed;
            bool hidden = false;
            for (u32 i = 0; i < cnt && run; ++i) {
                const size_t at = i * ROWSIZE;
                const u32 first = data[at] | data[at + 1] << 8 | data[at + 2] << 16 | data[at + 3] << 24;
                const u32 last = data[at + 4] | data[at + 5] << 8 | data[at + 6] << 16 | data[at + 7] << 24;
                run = first >= 1 && first <= last && last <= 358;
                bool hide = false;
                for (u32 day : holo.days)
                    if (first <= day && day <= last) hide = true;
                hidden = hidden || hide;
                if (!hide) packed.insert(packed.end(), data.begin() + at, data.begin() + at + ROWSIZE);
            }
            run = run && hidden && !packed.empty() && r32(page + ROWPTR) == rows && r32(page + ROWCNT) == cnt;
            if (run) {
                const u32 newCnt = (u32)(packed.size() / ROWSIZE);
                run = writeMem("ARM9 System Bus", rows, packed, err) && w32(page + ROWCNT, newCnt);
                if (!run) {
                    writeMem("ARM9 System Bus", rows, data, err);
                    w32(page + ROWCNT, cnt);
                }
                std::vector<u8> verify;
                if (run)
                    run = readMem("ARM9 System Bus", rows, (u32)packed.size(), verify, err) && verify == packed && r32(page + ROWCNT) == newCnt;
                if (!run && !verify.empty()) {
                    writeMem("ARM9 System Bus", rows, data, err);
                    w32(page + ROWCNT, cnt);
                }
                if (run) {
                    holo.ctx = ctx;
                    holo.page = page;
                    holo.rows = rows;
                    holo.cnt = newCnt;
                }
            }
        }
    }
}
}
