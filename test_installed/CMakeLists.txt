cmake_minimum_required(VERSION 3.14)
project(DiGraphXFastInstalledTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find dependencies first - the installed config references fmt, Py2Cpp and MyWheel. spdlog is
# needed by the compiled logger.
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(Py2Cpp REQUIRED)
find_package(MyWheel REQUIRED)
find_package(DiGraphXFast REQUIRED)

add_executable(digraphxfast_installed_test main.cpp)
target_link_libraries(digraphxfast_installed_test PRIVATE DiGraphXFast::DiGraphXFast)

enable_testing()
add_test(NAME digraphxfast_installed_test COMMAND digraphxfast_installed_test)
