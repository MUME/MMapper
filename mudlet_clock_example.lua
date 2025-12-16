--[[
    MMapper GMCP Clock Integration for Mudlet

    This script receives clock data from MMapper via GMCP and can be used
    as a foundation for day/night cycles, moon tracking, and other time-based
    features in your Mudlet GUI.

    Installation:
    1. Save this file or copy its contents
    2. In Mudlet, create a new Script
    3. Paste this code into the script
    4. Save and the script will auto-register

    Usage:
    The clock data will automatically update when MMapper sends it.
    You can access the data via: mmapper.clock
]]

-- Initialize namespace
mmapper = mmapper or {}
mmapper.clock = mmapper.clock or {}

-- Month name mappings (0-indexed)
mmapper.clock.WESTRON_MONTHS = {
    [0] = "Afteryule",
    [1] = "Solmath",
    [2] = "Rethe",
    [3] = "Astron",
    [4] = "Thrimidge",
    [5] = "Forelithe",
    [6] = "Afterlithe",
    [7] = "Wedmath",
    [8] = "Halimath",
    [9] = "Winterfilth",
    [10] = "Blotmath",
    [11] = "Foreyule"
}

mmapper.clock.SINDARIN_MONTHS = {
    [0] = "Narwain",
    [1] = "Ninui",
    [2] = "Gwaeron",
    [3] = "Gwirith",
    [4] = "Lothron",
    [5] = "Norui",
    [6] = "Cerveth",
    [7] = "Urui",
    [8] = "Ivanneth",
    [9] = "Narbeleth",
    [10] = "Hithui",
    [11] = "Girithron"
}

-- Weekday names (0-indexed, 0 = Sunday)
mmapper.clock.WESTRON_WEEKDAYS = {
    [0] = "Sunday",
    [1] = "Monday",
    [2] = "Trewsday",
    [3] = "Hevensday",
    [4] = "Mersday",
    [5] = "Highday",
    [6] = "Sterday"
}

mmapper.clock.SINDARIN_WEEKDAYS = {
    [0] = "Oranor",
    [1] = "Orithil",
    [2] = "Orgaladhad",
    [3] = "Ormenel",
    [4] = "Orbelain",
    [5] = "Oraearon",
    [6] = "Orgilion"
}

-- State variables
mmapper.clock.data = nil
mmapper.clock.lastUpdate = nil
mmapper.clock.previousTimeOfDay = nil

-- Helper function to calculate weekday
function mmapper.clock.getWeekday(year, month, day)
    -- MUME calendar: 360 days per year, 30 days per month
    local totalDays = (year - 2850) * 360 + month * 30 + day
    return totalDays % 7
end

-- Helper function to format time as string
function mmapper.clock.formatTime(data, useWestron)
    if not data then return "Unknown time" end

    local monthNames = useWestron and mmapper.clock.WESTRON_MONTHS or mmapper.clock.SINDARIN_MONTHS
    local weekdayNames = useWestron and mmapper.clock.WESTRON_WEEKDAYS or mmapper.clock.SINDARIN_WEEKDAYS

    local weekday = mmapper.clock.getWeekday(data.year, data.month, data.day)
    local monthName = monthNames[data.month] or "Unknown"
    local weekdayName = weekdayNames[weekday] or "Unknown"

    local hour = data.hour
    local minute = data.minute
    local ampm = "am"

    if hour == 0 then
        hour = 12
    elseif hour == 12 then
        ampm = "pm"
    elseif hour > 12 then
        hour = hour - 12
        ampm = "pm"
    end

    return string.format("%d:%02d%s on %s, the %d%s of %s, year %d of the Third Age",
        hour, minute, ampm,
        weekdayName,
        data.day + 1, -- Display as 1-30 instead of 0-29
        mmapper.clock.getDaySuffix(data.day + 1),
        monthName,
        data.year
    )
end

-- Helper for day suffix (1st, 2nd, 3rd, etc.)
function mmapper.clock.getDaySuffix(day)
    if day == 1 or day == 21 then return "st"
    elseif day == 2 or day == 22 then return "nd"
    elseif day == 3 or day == 23 then return "rd"
    else return "th" end
end

-- Main GMCP handler
function mmapper.clock.handleTimeInfo(event, ...)
    -- Get the GMCP data
    local data = gmcp.MUME and gmcp.MUME.Time and gmcp.MUME.Time.Info

    if not data then
        cecho("\n<red>Error: MUME.Time.Info data not found in GMCP\n")
        return
    end

    -- Store the data
    mmapper.clock.data = data
    mmapper.clock.lastUpdate = os.time()

    -- Detect time of day transitions
    if mmapper.clock.previousTimeOfDay and mmapper.clock.previousTimeOfDay ~= data.timeOfDay then
        mmapper.clock.onTimeOfDayChange(data.timeOfDay)
    end
    mmapper.clock.previousTimeOfDay = data.timeOfDay

    -- Raise custom events that your scripts can listen for
    raiseEvent("MMapperClockUpdate", data)

    -- Optional: Debug output (comment out if you don't want it)
    -- mmapper.clock.debug(data)
end

-- Called when time of day changes (dawn, day, dusk, night)
function mmapper.clock.onTimeOfDayChange(newTimeOfDay)
    -- You can customize these messages or add actions
    if newTimeOfDay == "dawn" then
        cecho("\n<yellow>The sun rises in the east, casting long shadows across the land.\n")
        -- Example: Update your GUI, change colors, etc.

    elseif newTimeOfDay == "day" then
        cecho("\n<white>The sun climbs higher into the sky.\n")

    elseif newTimeOfDay == "dusk" then
        cecho("\n<orange>The sun sets in the west, painting the sky in shades of orange and red.\n")

    elseif newTimeOfDay == "night" then
        cecho("\n<blue>Darkness falls as the sun disappears below the horizon.\n")
    end

    -- Raise event for other scripts
    raiseEvent("MMapperTimeOfDayChanged", newTimeOfDay)
end

-- Debug output function
function mmapper.clock.debug(data)
    local output = {
        "\n<cyan>===== MMapper Clock Update =====",
        string.format("<white>Time: <yellow>%s", mmapper.clock.formatTime(data, true)),
        string.format("<white>Season: <green>%s", data.season or "unknown"),
        string.format("<white>Time of Day: <orange>%s", data.timeOfDay or "unknown"),
        string.format("<white>Moon Phase: <magenta>%s <white>(<yellow>%d<white>/12 illumination)",
            data.moonPhase or "unknown", data.moonLevel or 0),
        string.format("<white>Moon Visibility: <cyan>%s", data.moonVisibility or "unknown"),
        string.format("<white>Dawn: <yellow>%d:00<white>, Dusk: <orange>%d:00",
            data.dawnHour or 0, data.duskHour or 0),
        string.format("<white>Precision: <green>%s", data.precision or "unknown"),
        "<cyan>==============================="
    }

    for _, line in ipairs(output) do
        cecho(line .. "\n")
    end
end

-- Command to display current time
function mmapper.clock.showTime()
    if not mmapper.clock.data then
        cecho("\n<red>No clock data available yet. Make sure MMapper is connected and GMCP is enabled.\n")
        return
    end

    local data = mmapper.clock.data

    cecho("\n<cyan>╔══════════════════════════════════════════════╗\n")
    cecho("<cyan>║ <white>           MUME Time & Weather           <cyan>║\n")
    cecho("<cyan>╠══════════════════════════════════════════════╣\n")
    cecho(string.format("<cyan>║ <white>Time:   <yellow>%-33s<cyan>║\n",
        mmapper.clock.formatTime(data, true)))
    cecho(string.format("<cyan>║ <white>Season: <green>%-33s<cyan>║\n",
        data.season or "unknown"))
    cecho(string.format("<cyan>║ <white>Period: <orange>%-33s<cyan>║\n",
        data.timeOfDay or "unknown"))
    cecho("<cyan>╠══════════════════════════════════════════════╣\n")
    cecho(string.format("<cyan>║ <white>Moon:   <magenta>%-33s<cyan>║\n",
        data.moonPhase or "unknown"))
    cecho(string.format("<cyan>║ <white>Light:  <yellow>%d/12 <white>%-26s<cyan>║\n",
        data.moonLevel or 0, "(" .. (data.moonVisibility or "unknown") .. ")"))
    cecho("<cyan>╠══════════════════════════════════════════════╣\n")
    cecho(string.format("<cyan>║ <white>Dawn at <yellow>%02d:00<white>, Dusk at <orange>%02d:00          <cyan>║\n",
        data.dawnHour or 0, data.duskHour or 0))
    cecho("<cyan>╚══════════════════════════════════════════════╝\n")
end

-- Enable GMCP module (run once on profile load)
function mmapper.clock.enableGMCP()
    if not gmod then
        cecho("\n<red>Error: gmod not available. Make sure GMCP is enabled in Mudlet.\n")
        return false
    end

    gmod.enableModule("Client", "MUME.Time 1")
    cecho("\n<green>MMapper GMCP Clock module enabled!\n")
    cecho("<white>Type '<yellow>mmtime<white>' to see the current time.\n")
    return true
end

-- Register the event handler
if mmapper.clock.eventHandler then
    killAnonymousEventHandler(mmapper.clock.eventHandler)
end
mmapper.clock.eventHandler = registerAnonymousEventHandler("gmcp.MUME.Time.Info", "mmapper.clock.handleTimeInfo")

-- Create alias to show time
if mmapper.clock.timeAlias then
    killAlias(mmapper.clock.timeAlias)
end
mmapper.clock.timeAlias = tempAlias("^mmtime$", [[mmapper.clock.showTime()]])

-- Enable GMCP support
tempTimer(1, function() mmapper.clock.enableGMCP() end)

-- Notify user
cecho("\n<green>MMapper Clock Integration loaded successfully!\n")
cecho("<white>Waiting for clock data from MMapper...\n")
cecho("<white>Type '<yellow>mmtime<white>' to display the current time once data is received.\n")

--[[
    INTEGRATION WITH YOUR IMP GUI:

    To integrate with your existing day/night script, you can:

    1. Listen for the MMapperClockUpdate event:
       registerAnonymousEventHandler("MMapperClockUpdate", "yourFunctionName")

    2. Access clock data anytime via:
       mmapper.clock.data.timeOfDay  -- "dawn", "day", "dusk", or "night"
       mmapper.clock.data.season     -- "winter", "spring", "summer", "autumn"
       mmapper.clock.data.moonPhase  -- Moon phase name
       mmapper.clock.data.moonLevel  -- Moon illumination (0-12)

    3. Listen for time of day changes:
       registerAnonymousEventHandler("MMapperTimeOfDayChanged", "yourTransitionHandler")

    4. Example integration:

       function updateDayNightCycle()
           if not mmapper.clock.data then return end

           local tod = mmapper.clock.data.timeOfDay

           if tod == "night" then
               -- Apply night theme
               setYourGUITheme("dark")
           else
               -- Apply day theme
               setYourGUITheme("light")
           end
       end

       registerAnonymousEventHandler("MMapperClockUpdate", "updateDayNightCycle")
]]
