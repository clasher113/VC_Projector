local data_buffer = require("core:data_buffer")

local instance_limit = {}

local position = nil
local block_index = block.index("projector:projector")
local file_path = pack.data_file("projector", "limit") 

function instance_limit.set_position(pos)
	if (pos ~= nil and position ~= nil) then
		if (position[1] == pos[1] and position[2] == pos[2] and position[3] == pos[3]) then
			return
		end
	end
	instance_limit.remove()
	position = pos
end

function instance_limit.remove()
	if (instance_limit.exist()) then
		block.destruct(position[1], position[2], position[3])
	end
	position = nil
end

function instance_limit.exist()
	if (position ~= nil) then
		local index = block.get(position[1], position[2], position[3])
		if (index > 0 and index == block_index) then
			return true
		end
	end	
	return false
end

function instance_limit.load()
	if (file.isfile(file_path)) then
		local buffer = data_buffer()
		buffer:put_bytes(file.read_bytes(file_path))
		buffer:set_position(1)
		position = {buffer:get_int64(), buffer:get_int64(), buffer:get_int64()}
	end
end

function instance_limit.save()
	if (position ~= nil) then
		local buffer = data_buffer()
		for i=1,3 do
			buffer:put_int64(position[i])
		end
		file.write_bytes(file_path, buffer:get_bytes())
	end
end

return instance_limit