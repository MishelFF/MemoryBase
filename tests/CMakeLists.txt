cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders TRUE)


project(SettingsTests LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

option(ENABLE_COVERAGE "Enable code coverage" ON)



qt_standard_project_setup()

include(GNUInstallDirs)


qt_add_executable(test_settingsmanager
    test_settingsmanager.cpp
    ../client/settingsmanager.cpp
    ../client/settingsmanager.h
)
set_source_files_properties(test_settingsmanager.cpp PROPERTIES SKIP_AUTOMOC OFF)
target_include_directories(test_settingsmanager  PRIVATE
    ../client
)

target_link_libraries(test_settingsmanager  PRIVATE
    Qt6::Core
    Qt6::Test
)
if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Building with LLVM coverage")

    target_compile_options(test_settingsmanager PRIVATE
        -fprofile-instr-generate
        -fcoverage-mapping
    )

    target_link_options(test_settingsmanager PRIVATE
        -fprofile-instr-generate
        -fcoverage-mapping
    )
endif()
enable_testing()

add_test(NAME SettingsManager COMMAND test_settingsmanager)
