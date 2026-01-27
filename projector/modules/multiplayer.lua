local multiplayer = {}

multiplayer.sides = {
    SINGLEPLAYER = 0,
    SERVER = 1,
    CLIENT = 2
}

local sides_info = {
    [multiplayer.sides.SINGLEPLAYER] = { name = "SinglePlayer" },
    [multiplayer.sides.SERVER] = { name = "Server" },
    [multiplayer.sides.CLIENT] = { name = "Client" }
}

local side = multiplayer.sides.SINGLEPLAYER
local api = nil

function multiplayer.on_world_open()
    local m = _G["$Multiplayer"]
    if (m) then
        if (m.side == "server") then
            side = multiplayer.sides.SERVER
        elseif (m.side == "client") then
            side = multiplayer.sides.CLIENT
        end
    end
    if (multiplayer.is_multiplayer()) then
        if (side == multiplayer.sides.SERVER) then
            api = require(string.format("%s:api/%s/api", m.pack_id, m.api_references.Neutron.latest) )[m.side]

            api.entities.register("projector:projector_entity",{
                standard_fields = {
                    tsf_pos = {
                        maximum_deviation = 0.1,
                        evaluate_deviation = function (dist, cur_val, client_val)
                            if not client_val then
                                return 3
                            end

                            return math.euclidian3D(
                                cur_val[1], cur_val[2], cur_val[3],
                                client_val[1], client_val[2], client_val[3]
                            )
                        end
                    }
                }
	        })
        elseif (side == multiplayer.sides.CLIENT) then
            api = require(string.format("%s:api/%s/api", m.pack_id, m.api_references.Neutron[2]) )[m.side]
        end

        debug.log("Projector running in multiplayer mode. Side: " .. sides_info[side].name)
    end
end

function multiplayer.is_multiplayer()
    return side ~= multiplayer.sides.SINGLEPLAYER
end

function multiplayer.get_side()
    return side
end

function multiplayer.get_api()
    return api
end

return multiplayer