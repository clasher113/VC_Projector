local logs_panel
local status_label
local settings_container_1
local settings_container_2
local main_button
local sync_button
local orientation_button
local axis_button
local optimization_checkbox
local refresh_rate_textbox
local projection_size_x_textbox
local projection_size_y_textbox
local projection_offset_x_textbox
local projection_offset_y_textbox
local capture_size_x_textbox
local capture_size_y_textbox

local default_projection_size_x = "64"
local default_projection_size_y = "64"
local default_projection_offset_x = "0"
local default_projection_offset_y = "0"
local default_refresh_rate = "15"

local orientations = {"Vertical", "Horizontal"}
local axes = {"X", "Z"}
local logs_num = 0

function on_open()
	-- retreive elements
	logs_panel = document["logs"]
	status_label = document["status"]
	settings_container_1 = document["settings_1"]
	settings_container_2 = document["settings_2"]
	main_button = document["button"]
	sync_button = document["sync"]
	orientation_button = document["orientation"]
	axis_button = document["axis"]
	optimization_checkbox = document["optimization"]
	refresh_rate_textbox = document["refresh_rate"]
	projection_size_x_textbox = document["projection_size_x"]
	projection_size_y_textbox = document["projection_size_y"]
	projection_offset_x_textbox = document["projection_offset_x"]
	projection_offset_y_textbox = document["projection_offset_y"]
	capture_size_x_textbox = document["capture_size_x"]
	capture_size_y_textbox = document["capture_size_y"]

	-- set default values
	refresh_rate_textbox.text = default_refresh_rate
	projection_size_x_textbox.text = default_projection_size_x
	projection_size_y_textbox.text = default_projection_size_y
	projection_offset_x_textbox.text = default_projection_offset_x
	projection_offset_y_textbox.text = default_projection_offset_x
	capture_size_x_textbox.text = projection_size_x_textbox.text
	capture_size_y_textbox.text = projection_size_y_textbox.text
	orientation_button.text = "Orientation: " .. orientations[1]
	axis_button.text = "Axis: " .. axes[1]

	set_status("Idle")
end

function log_message(string)
	local size = logs_panel.size
	logs_panel:add("<label id='" .. tostring(logs_num) .. "' multiline='true' text-wrap='true'>" .. string .. "</label>")
	logs_panel.size = size
end

function set_status(string)
	status_label.text = "Status: " .. string
end

function test_func()
   log_message("test") 
   set_status("new status")
   settings_container_1.enabled = false
   settings_container_2.enabled = false
end

function synchronize()
	log_message("Synchronization...")
end

function index_of(array, value)
    for i, v in ipairs(array) do
        if v == value then
            return i
        end
    end
    return nil
end

function toggle_orientation()
	local strings = string.split(orientation_button.text, ": ")
	local index = index_of(orientations, strings[2])
	index = index + 1
	if (index > #(orientations)) then 
		index = 1
	end
	orientation_button.text = "Orientation: " .. orientations[index]
	axis_button.visible = (index ~= 2)
end

function toggle_axis()
	local strings = string.split(axis_button.text, ": ")
	local index = index_of(axes, strings[2])
	index = index + 1
	if (index > #(axes)) then 
		index = 1
	end
	axis_button.text = "Axis: " .. axes[index]
end

function handle_textbox(string, min, max, textbox_id)
	local number = tonumber(string)
	if (number == nil) then
		log_message(textbox_id .. ": input must be a number")
		return false
	end
	if (number > max or number < min) then
		log_message(textbox_id .. ": the number must be less than " .. tostring(max) .. " and greater than " .. tostring(min))
		return false
	end
	return true
end

function fps_consumer(string)
	if (handle_textbox(string, 1, 60, "Refresh rate") == false) then
		refresh_rate_textbox.text = default_refresh_rate
	end
end

function projection_size_x_consumer(string)
	if (handle_textbox(string, 1, 255, "Projection size X") == false) then
		projection_size_x_textbox.text = default_projection_size_x
	end
	capture_size_x_textbox.text = projection_size_x_textbox.text
end

function projection_size_y_consumer(string)
	if (handle_textbox(string, 1, 255, "Projection size Y") == false) then
		projection_size_y_textbox.text = default_projection_size_y
	end
	capture_size_y_textbox.text = projection_size_y_textbox.text
end

function projection_offset_x_consumer(string)
	if (handle_textbox(string, 1, 255, "Projection offset X") == false) then
		projection_offset_x_textbox.text = default_projection_offset_x
	end
end

function projection_offset_y_consumer(string)
	if (handle_textbox(string, 0, 255, "Projection offset Y") == false) then
		projection_offset_y_textbox.text = default_projection_offset_y
	end
end