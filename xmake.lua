add_rules("mode.debug", "mode.release", "mode.coverage")
add_requires("doctest", {alias = "doctest"})
set_languages("c++20")

if is_plat("windows") then
    add_cxflags("/EHsc /W4 /WX /wd5285 /wd4459 /wd4819", { force = true })
end

target("test_digraphx_fast")
    set_kind("binary")
    add_includedirs("include", {public = true})
    add_files("test/source/*.cpp")
    add_packages("doctest")
    add_tests("default")
