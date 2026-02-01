local multiplayer = require("projector:multiplayer")
local util = require("projector:util")

local rules = {}

local current_rules = {
    resolution_max = { 255, 255 },
    resolution_min = { 1, 1 },
    offset_max = { 255, 255, 255 },
    offset_min = { 1, 0, 0 },
    fps_max = 60,
    allowed_orientations = { util.orientation.VERTICAL, util.orientation.HORIZONTAL },
    allowed_axes = { util.axis.X, util.axis.Z },
    allow_rgb_mode = true,
    allow_use = true
}

local role_rules = {
    --["role_name"] = {
    --  rules = {}
    --}
}

local ROLE_GLOBAL_STR = "__global__"

local function parse(dst, src)
    if (type(src.resolution_max) == type(current_rules.resolution_max) and #src.resolution_max == #current_rules.resolution_max) then
        for k, v in pairs(src.resolution_max) do
            if (type(v) == type(dst.resolution_max[k])) then
                dst.resolution_max[k] = math.clamp(v, current_rules.resolution_min[k], current_rules.resolution_max[k])
            end
        end
    end
    if (type(src.offset_max) == type(current_rules.offset_max) and #src.offset_max == #current_rules.offset_max) then
        for k, v in pairs(src.offset_max) do
            if (type(v) == type(dst.offset_max[k])) then
                dst.offset_max[k] = math.clamp(v, current_rules.offset_min[k], current_rules.offset_max[k])
            end
        end
    end
    if (type(src.fps_max) == type(current_rules.fps_max)) then
        dst.fps_max = math.clamp(src.fps_max, 1, current_rules.fps_max)
    end
    if (type(src.allowed_orientations) == type(current_rules.allowed_orientations)) then
        local temp = {}
        for k, v in pairs(src.allowed_orientations) do
            if (v == "vertical") then
                table.insert(temp, util.orientation.VERTICAL)
            elseif (v == "horizontal") then
                table.insert(temp, util.orientation.HORIZONTAL)
            end
        end
        if (#temp > 0) then
            dst.allowed_orientations = temp
        end
    end
    if (type(src.allowed_axes) == type(current_rules.allowed_axes)) then
        local temp = {}
        for k, v in pairs(src.allowed_axes) do
            if (v == "X") then
                table.insert(temp, util.axis.X)
            elseif (v == "Z") then
                table.insert(temp, util.axis.Z)
            end
        end
        if (#temp > 0) then
            dst.allowed_axes = temp
        end
    end
    if (type(src.allow_rgb_mode) == type(current_rules.allow_rgb_mode)) then
        dst.allow_rgb_mode = src.allow_rgb_mode
    end
    if (type(src.allow_use) == type(current_rules.allow_use)) then
        dst.allow_use = src.allow_use
    end
end

function rules.on_world_open()
    role_rules[ROLE_GLOBAL_STR] = table.deep_copy(current_rules)

    local file_path = pack.shared_file("projector", "rules.json")
    if (file.isfile(file_path)) then
        local rule_set = json.parse(file.read(file_path))
        if (rule_set[ROLE_GLOBAL_STR] ~= nil) then
            parse(role_rules[ROLE_GLOBAL_STR], rule_set[ROLE_GLOBAL_STR])
        end
        for role_name, _rules in pairs(rule_set) do
            if (role_name ~=  ROLE_GLOBAL_STR) then
                role_rules[role_name] = table.deep_copy(role_rules[ROLE_GLOBAL_STR])
                parse(role_rules[role_name], _rules)
            end
        end
    end
end

function rules.get_rules(player_id)
    if (multiplayer.get_side() == multiplayer.sides.SERVER) then
        local api = multiplayer.get_api()
        local role = api.accounts.by_identity.get_account(api.sandbox.players.get_by_pid(player_id).identity)
        return (role_rules[role] and role_rules[role] or role_rules[ROLE_GLOBAL_STR])
    else
        return current_rules
    end
end

function rules.apply_rules(new_rules)
    for k, v in pairs(new_rules) do
        current_rules[k] = v
    end
end

return rules