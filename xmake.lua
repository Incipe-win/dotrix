add_rules("mode.debug", "mode.release")
set_languages("c++20")

target("dotrix")
    set_kind("binary")
    add_files("src/**.cpp")
    add_includedirs("src", "src/vendor")
    set_targetdir("$(projectdir)")

    if is_plat("linux") then add_links("stdc++fs") end
    if is_mode("release") then set_optimize("fastest"); set_strip("all") end
    if is_mode("debug")   then set_symbols("debug") end
