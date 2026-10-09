set_xmakever("3.0.0")
set_project("StarfieldVehicleSpeedometer")
set_version("0.2.2")
set_arch("x64")
set_languages("c++23")
add_rules("mode.debug", "mode.release")

-- This repository has no dependency on SAD or Map Cursor Meter.
-- Supply an external CommonLibSF checkout through COMMONLIBSF_DIR, or put
-- its source in external/CommonLibSF next to this xmake.lua file.
local commonlib = os.getenv("COMMONLIBSF_DIR")
if not commonlib or commonlib == "" then
    commonlib = "external/CommonLibSF"
end
if not os.isdir(commonlib) then
    raise("CommonLibSF not found: " .. commonlib .. ". Set COMMONLIBSF_DIR or clone into external/CommonLibSF.")
end
includes(commonlib)

target("StarfieldVehicleSpeedometer", function()
    add_rules("commonlibsf.plugin", {
        name = "StarfieldVehicleSpeedometer",
        author = "VexenDeimos",
        description = "Customizable Starfield land-vehicle speedometer"
    })
    set_license("GPL-3.0-or-later")
    add_includedirs("include")
    add_defines("NOMINMAX", "WIN32_LEAN_AND_MEAN")
    add_files("src/VehicleSpeedometer.cpp")
end)
