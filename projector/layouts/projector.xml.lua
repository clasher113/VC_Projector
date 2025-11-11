local config = require("projector:config")
local display = require("projector:display")
local rgb_addon = require("projector:rgb_addon")
local synchronizer = require("projector:synchronizer")
local util = require("projector:util")
local highlight = require("projector:highlight")

local orientations = { 
	[util.orientation.VERTICAL] = "Vertical",
	[util.orientation.HORIZONTAL] = "Horizontal"
}
local axes = {
	[util.axis.X] = "X", 
	[util.axis.Z] = "Z"
}

local show_additional_settings = true
local logs_num = 1
local single_time_init = false
local gui_enabled = true

local LOGS_MAX_COUNT = 20

function on_gui_render()
	for k, v in pairs(synchronizer.messages) do
		log_message(v)
		synchronizer.messages[k] = nil
	end
	set_gui_enabled(util.status_info[synchronizer.get_status()].gui_enabled)

	local anim_speed = 2000 -- pixels per second
	local size = document["root"].size
	if (show_additional_settings == true and size[1] < 810) then
		size[1] = math.min(810, size[1] + anim_speed * time.delta())
	end
	if (show_additional_settings == false and size[1] > 540) then
		size[1] = math.max(540, size[1] - anim_speed * time.delta())
	end
	document["root"].size = size
	if (rgb_addon.is_loaded() == false and config.rgb_mode and display.rgb_initialized == false) then
		rgb_consumer(true)
	end
end

function on_open()
	if (single_time_init == false) then
		single_time_init = true

		document["orientation"].text = "Orientation: " .. orientations[config.orientation]
		document["axis"].text = "Axis: " .. axes[config.axis]
		if (not rgb_addon.is_loaded()) then
			document["rgb_mode"].tooltip = "RGB addon not installed"
			document["rgb_mode"].tooltipDelay = 0
		end
		document["rgb_mode"].checked = config.rgb_mode
		document["same_size"].checked = config.same_size
		document["use_bytearray"].checked = config.use_bytearray
		document["highlight_area_checkbox"].checked = config.highlight_area
		toggle_additional_settings()
		same_size_consumer(config.same_size)
		stop_on_lag_consumer(config.stop_on_lag_duration)

		document["settings_1"]:setInterval(1, on_gui_render)
		synchronizer.on_disconnect_callback = function()
			if (synchronizer.get_status() == util.synchronizer_status.CAPTURING) then
				stop()
			end
		end
		synchronizer.on_lag_callback = function()
			local enabled = config.stop_on_lag_duration ~= document["stop_on_lag_trackbar"].max
			local delta = time.delta() * 1000
			if (enabled and delta > config.stop_on_lag_duration) then
				stop()
				log_message("Stopped due to lag " .. tostring(math.round(delta, 0)) .. "ms, limit " .. 
					tostring(config.stop_on_lag_duration) .. "ms")
				return true
			end
			return false
		end
	end
end

function set_gui_enabled(flag)
	if (flag == gui_enabled) then return end
	gui_enabled = flag
	document["settings_1"].enabled = flag
	document["settings_2"].enabled = flag
	document["sync"].enabled = flag
end

function log_message(string)
	local color = (logs_num % 2 == 0 and "#ffffff10" or "#ffffff00")
	document["logs"]:add("<textbox id='log" .. tostring(logs_num) .. "' color='" .. color .. "' editable='false' multiline='true' text-wrap='true' autoresize='true'>" .. string .. "</textbox>")
	if (logs_num > LOGS_MAX_COUNT) then
		document["log" .. tostring(logs_num - LOGS_MAX_COUNT)]:destruct()
	end
	for i=logs_num,math.max(1, logs_num - LOGS_MAX_COUNT),-1 do 
		document["log" .. tostring(i)]:moveInto(document["logs"])
	end
	logs_num = logs_num + 1
end

function status_supplier(string)
	return "Status: " .. util.status_info[synchronizer.get_status()].string
end

function stop()
	synchronizer.set_status(util.synchronizer_status.READY)
	document["main_button"].text = "Start"
	display.current_framerate = 0
	if (config.clear_on_stop) then
		display.clear()
	end
	log_message("Capturing stopped")
	highlight.refresh()
end

function start()
	synchronizer.set_status(util.synchronizer_status.CAPTURING)
	document["main_button"].text = "Stop"
	log_message("Capturing started")
	config.write()
	highlight.stop()
end

function main_button_func()
	if (synchronizer.get_status() == util.synchronizer_status.READY) then
		start()
	elseif (synchronizer.get_status() == util.synchronizer_status.CAPTURING) then
		stop()
	else
		log_message("You must synchronize first")
	end
end

function synchronize()
	if (synchronizer.get_status() == util.synchronizer_status.NOT_CONNECTED) then
		log_message("Not connected")
		return
	end
	log_message("Synchronization...")
	synchronizer.set_status(util.synchronizer_status.SYNCING)
	config.write()
end

function toggle_orientation()
	config.orientation = config.orientation + 1
	if (config.orientation > #(orientations)) then 
		config.orientation = 1
	end
	document["orientation"].text = "Orientation: " .. orientations[config.orientation]
	highlight.refresh()
end

function toggle_axis()
	config.axis = config.axis + 1
	if (config.axis > #(axes)) then 
		config.axis = 1
	end
	document["axis"].text = "Axis: " .. axes[config.axis]
	highlight.refresh()
end

function fps_consumer(string)
	local new_refresh_rate = tonumber(string)	
	if (config.refresh_rate == new_refresh_rate) then return end
	config.refresh_rate = new_refresh_rate
	if (synchronizer.get_status() == util.synchronizer_status.READY) then synchronizer.set_status(util.synchronizer_status.CONNECTED) end
end

function fps_supplier()
	document["refresh_rate_label"].text = "Projection refresh rate: " .. tostring(config.refresh_rate)
	return config.refresh_rate
end

function rgb_consumer(checked)
	config.rgb_mode = checked
	if (rgb_addon.is_loaded() == false and (synchronizer.get_status() == util.synchronizer_status.CONNECTED or 
		synchronizer.get_status() == util.synchronizer_status.READY) and display.rgb_initialized == false) then
		log_message("Initializing")
		synchronizer.set_status(util.synchronizer_status.INIT)
	end
end

function rgb_supplier()
	return config.rgb_mode
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
	return validate_textbox(string, 1, 255, document["projection_size_x"])
end

function projection_size_x_consumer(string)
	if (not projection_size_x_validator(string)) then return end
	config.resolution[1] = tonumber(string)
	highlight.refresh()
	if (config.same_size == true) then
		capture_size_x_consumer(string)
	end
	if (synchronizer.get_status() == util.synchronizer_status.READY) then synchronizer.set_status(util.synchronizer_status.CONNECTED) end
end

function projection_size_x_supplier()
	local temp = document["projection_size_x"].valid
	return tostring(config.resolution[1])
end

function projection_size_y_validator(string)
	return validate_textbox(string, 1, 255, document["projection_size_y"])
end

function projection_size_y_consumer(string)
	if (not projection_size_y_validator(string)) then return end
	config.resolution[2] = tonumber(string)
	highlight.refresh()
	if (config.same_size == true) then
		capture_size_y_consumer(string)
	end
	if (synchronizer.get_status() == util.synchronizer_status.READY) then synchronizer.set_status(util.synchronizer_status.CONNECTED) end
end

function projection_size_y_supplier()
	local temp = document["projection_size_y"].valid
	return tostring(config.resolution[2])
end

function capture_size_x_validator(string)
	return validate_textbox(string, 1, 1920, document["capture_size_x"])
end

function capture_size_x_consumer(string)
	if (not capture_size_x_validator(string)) then return end
	config.capture_size[1] = tonumber(string)
	if (synchronizer.get_status() == util.synchronizer_status.READY) then synchronizer.set_status(util.synchronizer_status.CONNECTED) end
end

function capture_size_x_supplier()
	local temp = document["capture_size_x"].valid
	return tostring(config.capture_size[1])
end

function capture_size_y_validator(string)
	return validate_textbox(string, 1, 1080, document["capture_size_y"])
end

function capture_size_y_consumer(string)
	if (not capture_size_y_validator(string)) then return end
	config.capture_size[2] = tonumber(string)
	if (synchronizer.get_status() == util.synchronizer_status.READY) then synchronizer.set_status(util.synchronizer_status.CONNECTED) end
end

function capture_size_y_supplier()
	local temp = document["capture_size_y"].valid
	return tostring(config.capture_size[2])
end

function projection_offset_x_validator(string)
	return validate_textbox(string, 1, 255, document["projection_offset_x"])
end

function projection_offset_x_consumer(string)
	if (not projection_offset_x_validator(string)) then return end
	config.offset[1] = tonumber(string)
	highlight.refresh()
end

function projection_offset_x_supplier()
	local temp = document["projection_offset_x"].valid
	return tostring(config.offset[1])
end

function projection_offset_y_validator(string)
	return validate_textbox(string, 0, 255, document["projection_offset_y"])
end

function projection_offset_y_consumer(string)
	if (not projection_offset_y_validator(string)) then return end
	config.offset[2] = tonumber(string)
	highlight.refresh()
end

function projection_offset_y_supplier()
	local temp = document["projection_offset_y"].valid
	return tostring(config.offset[2])
end

function projection_offset_z_validator(string)
	return validate_textbox(string, 0, 255, document["projection_offset_z"])
end

function projection_offset_z_consumer(string)
	if (not projection_offset_z_validator(string)) then return end
	config.offset[3] = tonumber(string)
	highlight.refresh()
end

function projection_offset_z_supplier()
	local temp = document["projection_offset_z"].valid
	return tostring(config.offset[3])
end

function same_size_consumer(checked)
	config.same_size = checked
	document["capture_size_x"].enabled = not checked
	document["capture_size_y"].enabled = not checked
	if (checked) then
		capture_size_x_consumer(document["projection_size_x"].text)
		capture_size_y_consumer(document["projection_size_y"].text)
		highlight.refresh()
	end
end

function clear_on_stop_consumer(checked)
	config.clear_on_stop = checked
end

function clear_on_stop_supplier()
	return config.clear_on_stop
end

function framerate_supplier()
	return "Current framerate: " .. tostring(display.current_framerate)
end

function toggle_additional_settings()
	show_additional_settings = not show_additional_settings
	document["additional_settings"].text = "Additional settings " .. (show_additional_settings and "<<" or ">>")
end

function use_bytearray_consumer(checked)
	config.use_bytearray = checked
end

function highlight_area_consumer(checked)
	config.highlight_area = checked
	highlight.refresh()
end

function stop_on_lag_consumer(value)
	local is_max_value = value == document["stop_on_lag_trackbar"].max
	document["stop_on_lag_label"].text = "Stop on lag: " .. (is_max_value and "Disabled" or tostring(value) .. "ms")
	document["stop_on_lag_trackbar"].value = value
	config.stop_on_lag_duration = value
end

function clear_display()
	if (synchronizer.get_status() == util.synchronizer_status.CAPTURING) then
		log_message("Does it make sense while the projector is running?")
	else
		display.clear()
		log_message("Display cleaned")
	end
end