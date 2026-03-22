add_defines(
    "_WIN32_WINNT=0x0A00",
    "WINVER=0x0A00"
)

local commonlib_root = os.getenv("COMMONLIBSSE_NG_ROOT")
if commonlib_root == nil or commonlib_root == "" then
    commonlib_root = "lib/commonlibsse-ng"
end
commonlib_root = path.absolute(commonlib_root)

if not os.isdir(commonlib_root) then
    raise("CommonLibSSE-NG root not found at '" .. commonlib_root .. "'. Initialize lib/commonlibsse-ng or set COMMONLIBSSE_NG_ROOT.")
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
        description = "Skyrim console observability, display, and command submission debug helper",
        options = {
            address_library = true,
            struct_dependent = false
        }
    })

    apply_project_warnings()

    add_files("src/**.cpp")
    add_headerfiles("src/**.h", "src/**.hpp")
    add_includedirs("src")

target("ConsoleLinkTests")
    set_kind("binary")
    set_default(false)

    add_deps("commonlibsse-ng")
    apply_project_warnings()

    add_files(
        "src/config/Settings.cpp",
        "src/command/CommandSubmissionState.cpp",
        "src/command/InboxPath.cpp",
        "src/display/DisplayState.cpp",
        "src/display/OutboxPath.cpp",
        "tests/**.cpp"
    )

    add_includedirs("src")

