add_rules("mode.debug", "mode.release", "mode.coverage")
add_requires("doctest", {alias = "doctest"})
add_requires("fmt", {alias = "fmt"})
add_requires("nanobench", {alias = "nanobench"})
set_languages("c++20")

if is_mode("release") then
    set_optimize("fastest")
end

if is_plat("windows") then
    add_cxflags("/EHsc /utf-8 /W4 /WX /wd4702", { force = true })
end

target("test_digraphx_fast")
    set_kind("binary")
    add_includedirs("include", {public = true})
    add_files("test/source/*.cpp")
    add_packages("doctest")
    add_tests("default")

target("BM_neg_cycle")
    set_kind("binary")
    add_includedirs("include", {public = true})
    add_files("bench/BM_neg_cycle.cpp")
    add_packages("nanobench", "fmt")
