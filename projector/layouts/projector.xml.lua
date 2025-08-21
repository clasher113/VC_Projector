local config = require("projector:config")
local display = require("projector:display")
local rgb_addon = require("projector:rgb_addon")
local synchronizer = require("projector:synchronizer")

local logs_panel
local status_label
local settings_container_1
local settings_container_2
local main_button
local sync_button
local orientation_button
local axis_button
local clear_on_stop_checkbox
local refresh_rate_trackbar
local refresh_rate_label
local projection_size_x_textbox
local projection_size_y_textbox
local projection_offset_x_textbox
local projection_offset_y_textbox
local projection_offset_z_textbox
local capture_size_x_textbox
local capture_size_y_textbox
local same_size_checkbox
local rgb_mode_checkbox

local default_refresh_rate = config.refresh_rate
local default_projection_size_x = tostring(config.resolution[1])
local default_projection_size_y = tostring(config.resolution[2])
local default_capture_size_x = tostring(config.capture_size[1])
local default_capture_size_y = tostring(config.capture_size[2])
local default_projection_offset_x = tostring(config.offset[1])
local default_projection_offset_y = tostring(config.offset[2])
local default_projection_offset_z = tostring(config.offset[3])

local orientations = {"Vertical", "Horizontal"}
local axes = {"X", "Z"}

local logs_num = 1
local single_time_init = false

function on_game_update()
	if (synchronizer.is_connected()) then
		if (synchronizer.is_capturing) then
			set_status("Projecting")
		elseif (synchronizer.is_synchronized) then
			set_status("Ready")
		else
			set_status("Connected")
		end
	else
		set_status("Waiting for connection")
	end
	for k,v in pairs(synchronizer.statuses) do
		log_message(v)
		synchronizer.statuses[k] = nil
	end
end

function on_open()
	if (single_time_init == false) then

		-- retreive elements
		logs_panel = document["logs"]
		status_label = document["status"]
		settings_container_1 = document["settings_1"]
		settings_container_2 = document["settings_2"]
		main_button = document["main_button"]
		sync_button = document["sync"]
		orientation_button = document["orientation"]
		axis_button = document["axis"]
		clear_on_stop_checkbox = document["clear_on_stop"]
		refresh_rate_trackbar = document["refresh_rate"]
		refresh_rate_label = document["refresh_rate_label"]
		projection_size_x_textbox = document["projection_size_x"]
		projection_size_y_textbox = document["projection_size_y"]
		projection_offset_x_textbox = document["projection_offset_x"]
		projection_offset_y_textbox = document["projection_offset_y"]
		projection_offset_z_textbox = document["projection_offset_z"]
		capture_size_x_textbox = document["capture_size_x"]
		capture_size_y_textbox = document["capture_size_y"]
		same_size_checkbox = document["same_size"]
		rgb_mode_checkbox = document["rgb_mode"]

		single_time_init = true
		settings_container_1:setInterval(1, on_game_update)

		default_refresh_rate = config.refresh_rate
		default_projection_size_x = tostring(config.resolution[1])
		default_projection_size_y = tostring(config.resolution[2])
		default_capture_size_x = tostring(config.capture_size[1])
		default_capture_size_y = tostring(config.capture_size[2])
		default_projection_offset_x = tostring(config.offset[1])
		default_projection_offset_y = tostring(config.offset[2])
		default_projection_offset_z = tostring(config.offset[3])
		clear_on_stop_checkbox.checked = config.clear_on_stop
		config.orientation = 1
		config.axis = 1

		-- set default values
		refresh_rate_trackbar.value = default_refresh_rate
		projection_size_x_textbox.text = default_projection_size_x
		projection_size_y_textbox.text = default_projection_size_y
		projection_offset_x_textbox.text = default_projection_offset_x
		projection_offset_y_textbox.text = default_projection_offset_y
		projection_offset_z_textbox.text = default_projection_offset_z
		capture_size_x_textbox.text = default_capture_size_x
		capture_size_y_textbox.text = default_capture_size_y
		orientation_button.text = "Orientation: " .. orientations[config.orientation]
		axis_button.text = "Axis: " .. axes[config.axis]
		if (rgb_addon.is_loaded() ~= true) then
			rgb_mode_checkbox.enabled = false
			rgb_mode_checkbox.checked = false
			settings_container_2:add("<container id='tooltip' color='#00000000' size='" .. rgb_mode_checkbox.size[1] .. "," .. rgb_mode_checkbox.size[2] .."' pos='".. rgb_mode_checkbox.pos[1] .. "," .. rgb_mode_checkbox.pos[2] .. "'></container>")
			document["tooltip"].tooltip = "RGB addon require"
			document["tooltip"].tooltipDelay = 0
		else
			rgb_mode_checkbox.checked = config.rgb_mode
		end
		fps_consumer("")
		same_size_consumer(config.same_size)
		init_display()

		synchronizer.on_disconnect_callback = function()
			if (synchronizer.is_capturing == true) then
				stop()
			end
		end
	end
end

function set_gui_enabled(flag)
	settings_container_1.enabled = flag
	settings_container_2.enabled = flag
	sync_button.enabled = flag
end

function log_message(string)
	local size = logs_panel.size
	local color = (logs_num % 2 == 0 and "#ffffff10" or "#ffffff00")
	logs_panel:add("<textbox id='log" .. tostring(logs_num) .. "' color='" .. color .. "' editable='false' multiline='true' text-wrap='true' autoresize='true'>" .. string .. "</textbox>")
	for i=logs_num,1,-1 do 
		document["log" .. tostring(i)]:moveInto(logs_panel)
	end
	logs_panel.size = size
	logs_num = logs_num + 1
end

function set_status(string)
	status_label.text = "Status: " .. string
end

function stop()
	set_gui_enabled(true)
	synchronizer.is_capturing = false
	main_button.text = "Start"
	if (clear_on_stop_checkbox.checked == true) then
		display.clear()
	end
	log_message("Capturing stopped")
end

function start()
	set_gui_enabled(false)
	synchronizer.is_capturing = true
	main_button.text = "Stop"
	init_display()
	log_message("Capturing started")
end

function main_button_func()
	if (synchronizer.is_synchronized == true and synchronizer.is_capturing == false) then
		start()
	elseif (synchronizer.is_capturing  == true) then
		stop()
	elseif (synchronizer.is_synchronized == false) then
		log_message("You must synchronize first")
	end
end

function get_orientation_index()
	return index_of(orientations, string.split(orientation_button.text, ": ")[2])
end

function get_axis_index()
	return index_of(axes, string.split(axis_button.text, ": ")[2])
end

function init_display()
	config.refresh_rate = refresh_rate_trackbar.value
	config.resolution[1] = tonumber(projection_size_x_textbox.text)
	config.resolution[2] = tonumber(projection_size_y_textbox.text)
	config.capture_size[1] = tonumber(capture_size_x_textbox.text)
	config.capture_size[2] = tonumber(capture_size_y_textbox.text)
	config.offset[1] = tonumber(projection_offset_x_textbox.text)
	config.offset[2] = tonumber(projection_offset_y_textbox.text)
	config.offset[3] = tonumber(projection_offset_z_textbox.text)
	config.axis = get_axis_index()
	config.orientation = get_orientation_index()
	config.same_size = same_size_checkbox.checked
	config.clear_on_stop = clear_on_stop_checkbox.checked
	config.rgb_mode = rgb_mode_checkbox.checked
	config.write()
end

function synchronize()
	if (synchronizer.is_connected() == false) then
		log_message("Not connected")
		return
	end
	log_message("Synchronization...")
	synchronizer.is_syncing = true
	init_display()
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
	local index = get_orientation_index()
	index = index + 1
	if (index > #(orientations)) then 
		index = 1
	end
	orientation_button.text = "Orientation: " .. orientations[index]
end

function toggle_axis()
	local index = get_axis_index()
	index = index + 1
	if (index > #(axes)) then 
		index = 1
	end
	axis_button.text = "Axis: " .. axes[index]
end

function fps_consumer(string)
	refresh_rate_label.text = "Projection refresh rate: " .. tostring(refresh_rate_trackbar.value)
	synchronizer.is_synchronized = false
end

local function validate_textbox(input, min, max, textbox)
	local number = tonumber(input)
	textbox.tooltipDelay = 0
	if (number == nil) then
		textbox.tooltip = "Input must be a number"
		return false
	end
	if (number > max) then
		textbox.tooltip = "Number too big. Possible maximum - " .. tostring(max)
		return false
	elseif (number < min) then
		textbox.tooltip = "Number too low. Required minimum - " .. tostring(min)
		return false
	end
	textbox.tooltip = ""
	return true
end

function projection_size_x_validator(string)
	return validate_textbox(string, 1, 255, projection_size_x_textbox)
end

function projection_size_x_consumer(string)
	if (projection_size_x_validator(string)) then
		config.resolution[1] = tonumber(string)
		if (same_size_checkbox.checked == true) then
			capture_size_x_consumer(projection_size_x_textbox.text)
		end
	end
	synchronizer.is_synchronized = false
end

function projection_size_x_supplier()
	local temp = projection_size_x_textbox.valid
	return tostring(config.resolution[1])
end

function projection_size_y_validator(string)
	return validate_textbox(string, 1, 255, projection_size_y_textbox)
end

function projection_size_y_consumer(string)
	if (projection_size_y_validator(string)) then
		config.resolution[2] = tonumber(string)
		if (same_size_checkbox.checked == true) then
			capture_size_y_consumer(projection_size_y_textbox.text)
		end
	end
	synchronizer.is_synchronized = false
end

function projection_size_y_supplier()
	local temp = projection_size_y_textbox.valid
	return tostring(config.resolution[2])
end

function capture_size_x_validator(string)
	return validate_textbox(string, 1, 1920, capture_size_x_textbox)
end

function capture_size_x_consumer(string)
	if (capture_size_x_validator(string)) then
		config.capture_size[1] = tonumber(string)
	end
	synchronizer.is_synchronized = false
end

function capture_size_x_supplier()
	local temp = capture_size_x_textbox.valid
	return tostring(config.capture_size[1])
end

function capture_size_y_validator(string)
	return validate_textbox(string, 1, 1080, capture_size_y_textbox)
end

function capture_size_y_consumer(string)
	if (capture_size_y_validator(string)) then
		config.capture_size[2] = tonumber(string)
	end
	synchronizer.is_synchronized = false
end

function capture_size_y_supplier()
	local temp = capture_size_y_textbox.valid
	return tostring(config.capture_size[2])
end

function projection_offset_x_validator(string)
	return validate_textbox(string, 1, 255, projection_offset_x_textbox)
end

function projection_offset_x_consumer(string)
	if (projection_offset_x_validator(string)) then
		config.offset[1] = tonumber(string)
	end
end

function projection_offset_x_supplier()
	local temp = projection_offset_x_textbox.valid
	return tostring(config.offset[1])
end

function projection_offset_y_validator(string)
	return validate_textbox(string, 0, 255, projection_offset_y_textbox)
end

function projection_offset_y_consumer(string)
	if (projection_offset_y_validator(string)) then
		config.offset[2] = tonumber(string)
	end
end

function projection_offset_y_supplier()
	local temp = projection_offset_y_textbox.valid
	return tostring(config.offset[2])
end

function projection_offset_z_validator(string)
	return validate_textbox(string, 0, 255, projection_offset_z_textbox)
end

function projection_offset_z_consumer(string)
	if (projection_offset_z_validator(string)) then
		config.offset[3] = tonumber(string)
	end
end

function projection_offset_z_supplier()
	local temp = projection_offset_z_textbox.valid
	return tostring(config.offset[3])
end

function same_size_consumer(checked)
	same_size_checkbox.checked = checked
	capture_size_x_textbox.enabled = (checked == false)
	capture_size_y_textbox.enabled = (checked == false)
	if (checked == true) then
		capture_size_x_consumer(projection_size_x_textbox.text)
		capture_size_y_consumer(projection_size_y_textbox.text)
	end
end

function clear_display()
	if (synchronizer.is_capturing == true) then
		log_message("Does it make sense while the projector is running?")
	else
		init_display()
		display.clear()
		log_message("Display cleaned")
	end
end
