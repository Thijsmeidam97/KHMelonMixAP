-- Kingdom Hearts 358/2 Days (Europe, YKGP)
--
-- Supporting script for:
--   Kingdom Hearts - 358-2 Days (Europe) (En,Fr,De,Es,It).nds
--
-- This is the EU scene-request experiment based only on the EU decomp.
-- It does not use gameplay input, CPU-register redirection, or an execution
-- callback.  It updates the live Day 357 descriptor and the two pending-scene
-- words identified by the EU decomp; the normal game dispatcher then consumes
-- the request.
--
-- This script does not modify the ROM or SaveRAM.

local ARM9 = "ARM9 System Bus"
local LOG_PATH = "E:\\khdays ap\\bizhawk\\Lua\\NDS\\KH358_eu_day357_support.log"

local TARGET_DAY = 357
local STORY_TRANSITION = 10001 -- 0x2711, the Day 357 special transition

local ADDR = {
    -- EU decomp SceneCtl at data_0204bda4.
    scene_object = 0x0204BDA8,
    scene_entry = 0x0204BDAC,
    current_scene = 0x0204BDB0,
    pending_scene = 0x0204BDB4,
    pending_argument = 0x0204BDB8,

    -- EU decomp data_0204be18 is the GameState pointer.  The bit array
    -- passed to func_020235d0/func_020235e8 starts at pointer + 0x10.
    game_state_ptr = 0x0204BE18,

    -- SceneTransition data_0204c240, as used by ov005's normal story path.
    story_descriptor = 0x0204C240,
}

local log_file = io.open(LOG_PATH, "w")
local frame = 0
local stable_frames = 0
local applied = false
local last_state = ""

local function log(message)
    local line = "KH358 EU Day 357 support: " .. tostring(message)
    print(line)
    if log_file then
        log_file:write(line .. "\n")
        log_file:flush()
    end
end

local function safe(fn, fallback)
    local ok, value = pcall(fn)
    if ok then return value end
    return fallback
end

local function r8(address, domain)
    return safe(function() return memory.read_u8(address, domain) end, 0) or 0
end

local function r16(address, domain)
    return safe(function() return memory.read_u16_le(address, domain) end, 0) or 0
end

local function r32(address, domain)
    return safe(function() return memory.read_u32_le(address, domain) end, 0) or 0
end

local function w8(address, value, domain)
    local ok, err = pcall(memory.write_u8, address, value, domain)
    if not ok then log(string.format("write_u8 failed at %08X: %s", address, tostring(err))) end
    return ok
end

local function w16(address, value, domain)
    local ok, err = pcall(memory.write_u16_le, address, value, domain)
    if not ok then log(string.format("write_u16 failed at %08X: %s", address, tostring(err))) end
    return ok
end

local function w32(address, value, domain)
    local ok, err = pcall(memory.write_u32_le, address, value, domain)
    if not ok then log(string.format("write_u32 failed at %08X: %s", address, tostring(err))) end
    return ok
end

local function state_text()
    local obj = r32(ADDR.scene_object, ARM9)
    local task_status = obj ~= 0 and r32(obj + 0x14, ARM9) or 0
    return string.format(
        "obj=%08X task=%08X entry=%08X scene=%08X stable=%d pending=%08X/%08X gs=%08X",
        obj,
        task_status,
        r32(ADDR.scene_entry, ARM9),
        r32(ADDR.current_scene, ARM9),
        stable_frames,
        r32(ADDR.pending_scene, ARM9),
        r32(ADDR.pending_argument, ARM9),
        r32(ADDR.game_state_ptr, ARM9))
end

local function set_story_day()
    local store = r32(ADDR.game_state_ptr, ARM9)
    if store == 0 or store < 0x02000000 then
        log("GameState store pointer is null")
        return false
    end

    -- Exact equivalent of func_020235e8(0, 9, 357):
    -- func_02025754 writes a 9-bit MSB-first field at bit offset 0.
    local base = store + 0x10
    local old_word = r32(base, ARM9)
    local old_value = (old_word >> 23) & 0x1ff
    local mask = 0x1ff << 23
    local new_word = (old_word & (~mask)) | ((TARGET_DAY & 0x1ff) << 23)
    w32(base, new_word, ARM9)

    log(string.format("GameState day %d -> %d; ptr=%08X base=%08X word=%08X",
        old_value, TARGET_DAY, store, base, r32(base, ARM9)))
    return true
end

local function set_story_descriptor()
    local base = ADDR.story_descriptor
    w8(base + 0, 0x00, ARM9)
    w8(base + 1, 0x00, ARM9)
    w16(base + 2, STORY_TRANSITION, ARM9)
    w16(base + 4, 0x0000, ARM9)
    log(string.format("descriptor transition=%d parameter=%04X",
        r16(base + 2, ARM9), r16(base + 4, ARM9)))
end

local function write_scene_request()
    w32(ADDR.pending_scene, 0x00000002, ARM9)
    w32(ADDR.pending_argument, 0x00000000, ARM9)

    -- EU decomp func_02023bbc(obj) returns ended only when obj[5] == -2.
    -- Normal scene callbacks produce this by returning -2.  Since this Lua
    -- path cannot call that callback, reproduce only the documented task
    -- return field so func_0202099c can tear down the current scene.
    local obj = r32(ADDR.scene_object, ARM9)
    if obj == 0 then
        log("scene request written, but scene object is null")
        return
    end
    local status_before = r32(obj + 0x14, ARM9)
    w32(obj + 0x14, 0xFFFFFFFE, ARM9)
    log(string.format("EU scene request written: %08X/%08X; task[%08X+14] %08X -> %08X",
        r32(ADDR.pending_scene, ARM9), r32(ADDR.pending_argument, ARM9),
        obj, status_before, r32(obj + 0x14, ARM9)))
end

local function ready()
    return r32(ADDR.scene_object, ARM9) ~= 0
        and r32(ADDR.current_scene, ARM9) ~= 0
        and r32(ADDR.pending_scene, ARM9) == 0
end

pcall(function() client.SetSoundOn(false) end)
log("loaded for EU YKGP; using only EU decomp addresses; no input or CPU redirection")

event.onframestart(function()
    frame = frame + 1

    if ready() then
        stable_frames = stable_frames + 1
    else
        stable_frames = 0
    end

    local current = state_text()
    if current ~= last_state then
        log("state frame=" .. tostring(frame) .. " " .. current)
        last_state = current
    end

    if not applied and stable_frames >= 120 then
        applied = true
        if set_story_day() then
            set_story_descriptor()
            write_scene_request()
            log("one-shot EU request complete; waiting for the normal loader")
        else
            log("request cancelled because GameState was not ready")
        end
    end

    if frame % 180 == 0 then
        log(string.format("heartbeat frame=%d applied=%s state={%s}",
            frame, tostring(applied), state_text()))
    end
end, "KH358 EU Day 357 support")

event.onexit(function()
    if log_file then
        log_file:close()
        log_file = nil
    end
end)
