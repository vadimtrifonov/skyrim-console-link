set_xmakever("3.0.0")

add_defines(
    "_WIN32_WINNT=0x0A00",
    "WINVER=0x0A00"
)

local commonlibvr_root = path.absolute("lib/commonlibvr")
if not os.isdir(commonlibvr_root) then
    raise("CommonLibVR is missing at '" .. commonlibvr_root .. "'. Run 'git submodule update --init --recursive'.")
end

local commonlibvr_xmake = path.join(commonlibvr_root, "xmake.lua")
if not os.isfile(commonlibvr_xmake) then
    raise("CommonLibVR at '" .. commonlibvr_root .. "' is incomplete. Expected xmake project at '" .. commonlibvr_xmake .. "'.")
end

local openvr_header = path.join(commonlibvr_root, "extern/openvr/headers/openvr.h")
if not os.isfile(openvr_header) then
    raise("CommonLibVR at '" .. commonlibvr_root .. "' is incomplete. Expected nested OpenVR headers at '" .. openvr_header .. "'. Run 'git submodule update --init --recursive'.")
end

includes(commonlibvr_root)

set_project("Console Link")
set_version("1.0.0")
set_license("Apache-2.0")
set_languages("c++23")
set_policy("package.requires_lock", true)

add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")
add_rules("plugin.compile_commands.autoupdate", { outputdir = ".", lsp = "clangd" })

option("msvc_analyze")
    set_default(false)
    set_showmenu(true)
    set_description("Enable MSVC /analyze for project sources")
option_end()

local function apply_project_warnings()
    set_warnings("allextra", "error")
    add_cxxflags(
        "cl::/permissive-",
        "cl::/W4",
        "cl::/WX",
        "cl::/wd4201"
    )

    add_cxxflags(
        "/external:anglebrackets",
        "/external:W0",
        { tools = "cl", force = true }
    )

    add_cxxflags(
        "clang_cl::/W4",
        "clang_cl::/WX"
    )

    add_cxxflags(
        "clang::-Wall",
        "clang::-Wextra",
        "clang::-Werror"
    )
end

target("ConsoleLink")
    add_deps("commonlibsse-ng")

    add_rules("commonlibsse-ng.plugin", {
        name = "Console Link",
        author = "Console Link contributors",
        description = "Skyrim console observability and command submission debug helper",
        options = {
            address_library = true,
            struct_dependent = false
        }
    })

    apply_project_warnings()

    add_files("src/**.cpp")

    if has_config("msvc_analyze") then
        add_cxxflags(
            "/analyze",
            "/wd6294",
            { tools = "cl", files = "src/**.cpp", force = true }
        )
    end

    add_headerfiles("src/**.h", "src/**.hpp")
    add_includedirs("src")

target("ConsoleLinkTests")
    set_kind("binary")
    set_default(false)

    add_deps("commonlibsse-ng")
    apply_project_warnings()

    add_files(
        "src/settings/Settings.cpp",
        "src/local_commands/support/FormKey.cpp",
        "src/local_commands/lookup/LookupCommand.cpp",
        "src/local_commands/lookup/LookupFormCommand.cpp",
        "src/local_commands/lookup_lvli/LookupLvliCommand.cpp",
        "src/local_commands/lookup_lvli/LookupLvliFormat.cpp",
        "src/local_commands/lookup_lvli/LookupLvliFormCommand.cpp",
        "src/local_commands/lookup_lvli/LookupLvliInspect.cpp",
        "src/local_commands/lookup_prefix/LookupPrefixCommand.cpp",
        "src/inbox/InboxFileReader.cpp",
        "src/inbox/InboxState.cpp",
        "src/inbox/InboxPath.cpp",
        "src/inbox/StartupInboxClear.cpp",
        "tests/**.cpp"
    )

    if has_config("msvc_analyze") then
        add_cxxflags(
            "/analyze",
            "/wd6294",
            {
                tools = "cl",
                files = "tests/**.cpp",
                force = true
            }
        )
    end

    add_includedirs("src")
