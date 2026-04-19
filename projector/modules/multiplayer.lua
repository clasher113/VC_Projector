local multiplayer = {}

multiplayer.logged_in = false
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
        api = require(string.format("%s:api/%s/api", m.pack_id, m.api_references.Neutron.latest) )[m.side]

        if (m.side == "server") then
            side = multiplayer.sides.SERVER

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
                },
                custom_fields = {
                    owner_pid = {
                        maximum_deviation = 1,
                        evaluate_deviation = api.entities.eval.NotEquals,
                        provider = function(uid, field_name)
                            return entities.get(uid):get_component("projector:projector").get_owner_pid()
                        end
                    }
                }
	        })
        elseif (m.side == "client") then
            side = multiplayer.sides.CLIENT
        end
        debug.log("Projector running in multiplayer mode. Side: " .. sides_info[side].name)
    end

    return api, side
end

function multiplayer.get_side()
    return side
end

function multiplayer.get_api()
    return api
end

return multiplayer