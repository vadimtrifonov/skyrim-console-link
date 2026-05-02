add_defines(
    "_WIN32_WINNT=0x0A00",
    "WINVER=0x0A00"
)

local commonlib_root = path.absolute("lib/commonlibsse-ng")
if not os.isdir(commonlib_root) then
    raise("CommonLibSSE-NG is missing at '" .. commonlib_root .. "'. This repo expects that directory to be populated by Git submodules. Run 'git submodule update --init --recursive'.")
end

local commonlib_xmake = path.join(commonlib_root, "xmake.lua")
if not os.isfile(commonlib_xmake) then
    raise("CommonLibSSE-NG at '" .. commonlib_root .. "' is incomplete. Expected xmake project at '" .. commonlib_xmake .. "'. Re-run 'git submodule update --init --recursive'.")
end

includes(commonlib_root)

set_project("ConsoleLink")
set_version("0.0.0")
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
        name = "ConsoleLink",
        author = "ConsoleLink contributors",
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
