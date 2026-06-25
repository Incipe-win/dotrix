add_rules("mode.debug", "mode.release")
set_languages("c++20")

target("dotrix")
    set_kind("binary")
    add_files("src/**.cpp")
    add_includedirs("src", "src/vendor", "../fuibase/include")
    set_targetdir("$(projectdir)")
    before_build(function (target)
        import("lib.detect.find_tool")
        local git = find_tool("git")
        local v = "unknown"
        if git then
            local out = try { function() return os.iorunv(git.program, {"describe", "--tags", "--always"}) end }
            if out and out:trim() ~= "" then v = out:trim() end
        end
        target:add("defines", 'DOTRIX_VERSION="' .. v .. '"')
    end)

    if is_plat("linux") then
        add_links("stdc++fs", "pthread")
        if is_mode("release") then
            add_ldflags("-static")
        end
    end
    if is_mode("release") then set_optimize("fastest"); set_strip("all") end
    if is_mode("debug")   then set_symbols("debug") end
