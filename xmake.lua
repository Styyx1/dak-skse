-- include subprojects
includes("lib/commonlibsse", "extern/styyx-utils", "extern/clib-util")

local MOD_NAME = "DynamicActivationKey"
local MOD_VERSION = "2.0.0"
local MOD_DESC = "Press a key to enable alternative actions"

-- set project constants
set_project(MOD_NAME)
set_version(MOD_VERSION)
set_license("GPL-3.0")
set_languages("c++23")
set_warnings("allextra")

set_config("commonlib_toml", true)
set_encodings("utf-8")


-- add common rules
add_rules("mode.debug", "mode.releasedbg")

-- define targets
target(MOD_NAME)
    add_deps("styyx-util")
    add_deps("clib-util")
    add_rules("commonlibsse.plugin", {
        name = MOD_NAME,
        author = "styyx",
        description = MOD_DESC
    })

    -- add src files
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")
