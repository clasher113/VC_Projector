local config = require("projector:config")
local display = require("projector:display")
local synchronizer = require("projector:synchronizer")
local util = require("projector:util")

local highlight = {}

local particle_emitters = {}
local particle_preset = {
	max_distance = 1000000,
	spawn_interval = 0.05,
	frames = {
		"blocks:highlight4",
		"blocks:highlight6",
		"blocks:highlight8",
		"blocks:highlight10",
		"blocks:highlight10",
		"blocks:highlight10",
		"blocks:highlight10",
		"blocks:highlight8",
		"blocks:highlight6",
		"blocks:highlight4",
	},
	lighting = false,
	collision = false,
	acceleration = { 0.0, 0.0, 0.0 },
	explosion = { 0.0, 0.0, 0.0 },
	size = { 1.2, 1.2, 1.2 },
	lifetime = 0.5,
	lifetime_spread = 0.0,
	size_spread = 0.0
}
local position = nil
local size = nil
local speed = 1 -- blocks per second
local distance = 0
local EMITTERS_INTERVAL = 10 -- blocks
local PARTICLES_INTERVAL = 0 -- blocks

function highlight.refresh(self)
	highlight.stop()

	if (not config.highlight_area or synchronizer.get_status() == util.synchronizer_status.CAPTURING) then return end

	position = vec3.add(vec3.add(display.get_position(hud.get_player()), 0.5), config.offset)
	size = vec2.sub(config.resolution, 1)

	distance = (size[1] + size[2]) * 2
	local particle_size = (EMITTERS_INTERVAL + PARTICLES_INTERVAL + (particle_preset.lifetime / particle_preset.spawn_interval)) * speed
	local interval = particle_size + (distance % particle_size) / (distance / particle_size) 
	local current_position = 0

	for i=1, math.max(1, math.floor(distance / interval)) do
		local emitter = {
			id = gfx.particles.emit(position, -1, particle_preset),
			current_pos = current_position,
			update = function(self)
				self.current_pos = self.current_pos + speed + PARTICLES_INTERVAL
				if (self.current_pos > distance) then
					self.current_pos = self.current_pos - distance
				end
	
				local offset = { 0.0, 0.0 }
				offset[1] = offset[1] + math.min(self.current_pos, size[1])
				if (self.current_pos > size[1]) then
					offset[2] = offset[2] + math.min(size[2], self.current_pos - size[1])
				end
				if (self.current_pos > size[1] + size[2]) then
					offset[1] = offset[1] - math.min(size[1], self.current_pos - size[1] - size[2])
				end
				if (self.current_pos > size[1] * 2 + size[2]) then
					offset[2] = offset[2] - (self.current_pos - (size[1] * 2 + size[2]))
				end

				local final_pos = table.copy(position)
				if (config.orientation == util.orientation.VERTICAL) then
					if (config.axis == util.axis.X) then
						final_pos[1] = final_pos[1] + offset[1]
						final_pos[2] = final_pos[2] + offset[2]
					elseif (config.axis == util.axis.Z) then
						final_pos[3] = final_pos[3] + offset[1]
						final_pos[2] = final_pos[2] + offset[2]
					end
				elseif (config.orientation == util.orientation.HORIZONTAL) then
					if (config.axis == util.axis.X) then
						final_pos[1] = final_pos[1] - offset[1] + size[1]
						final_pos[3] = final_pos[3] + offset[2]
					elseif (config.axis == util.axis.Z) then
						final_pos[1] = final_pos[1] + offset[2]
						final_pos[3] = final_pos[3] + offset[1]
					end
				end

				gfx.particles.set_origin(self.id, final_pos)
			end
		}
		table.insert(particle_emitters, emitter)
		current_position = current_position + interval
	end
	highlight.update()
end

function highlight.update()
	if (not config.highlight_area) then return end

	for _, particle in pairs(particle_emitters) do
		particle:update()
	end
end

function highlight.stop()
	for k, v in pairs(particle_emitters) do
		gfx.particles.stop(v.id)
		particle_emitters[k] = nil
	end
end

return highlight