cmake_minimum_required(VERSION 3.28)
project(jk 
    VERSION 2.5.1
    LANGUAGES CXX
    DESCRIPTION "Joker's C++ Kit"
)

include(GNUInstallDirs)
include(CTest)
include(FeatureSummary)

set(JK_CXX_STANDARD "20" CACHE STRING "C++ standard used to build jk (20 or 23)")
set_property(CACHE JK_CXX_STANDARD PROPERTY STRINGS 20 23)

if(NOT JK_CXX_STANDARD MATCHES "^(20|23)$")
    message(FATAL_ERROR "JK_CXX_STANDARD must be 20 or 23, got '${JK_CXX_STANDARD}'")
endif()

### create target
set(sources)
set(link_libs)
set(definitions)
set(compile_options)

list(APPEND sources 
    meta/compiler.h
    meta/type-var.h
    meta/type-list.h
    meta/callables.h
    wrap/string.h
    wrap/location.h
    wrap/expected.h
)

list(APPEND compile_options
    # make MSVC preprocessor conformant to C++ standard
    # https://devblogs.microsoft.com/cppblog/announcing-full-support-for-a-c-c-conformant-preprocessor-in-msvc/
    # https://learn.microsoft.com/en-us/cpp/build/reference/zc-preprocessor?view=msvc-170
    "$<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>"

    # make MSVC source files be treated as UTF-8 by default
    "$<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:/utf-8>"

    "$<$<CXX_COMPILER_ID:MSVC>:/permissive->"
)

## log
find_package(spdlog CONFIG QUIET)
if(spdlog_FOUND)
    set(JK_HAS_SPDLOG ON)
    list(APPEND link_libs spdlog::spdlog)
    list(APPEND definitions "JK_HAS_SPDLOG=1")
    list(APPEND sources wrap/log.h)
endif()

## sqlite
set(JK_SQLITE_PACKAGE "SQLite3" CACHE STRING "SQLite package to find")
if(CMAKE_VERSION VERSION_LESS "4.3")
    set(JK_SQLITE_LIBRARY_DEFAULT "SQLite::SQLite3")
else()
    set(JK_SQLITE_LIBRARY_DEFAULT "SQLite3::SQLite3")
endif()
set(JK_SQLITE_LIBRARY "${JK_SQLITE_LIBRARY_DEFAULT}" CACHE STRING "SQLite library to link")
find_package("${JK_SQLITE_PACKAGE}" QUIET)
if(${JK_SQLITE_PACKAGE}_FOUND)
    set(JK_HAS_SQLITE ON)
    list(APPEND link_libs "${JK_SQLITE_LIBRARY}")
    list(APPEND definitions "JK_HAS_SQLITE=1")
    list(APPEND sources wrap/sqlite.h)
endif()

## add library
set(source_paths)
foreach(source IN LISTS sources)
	list(APPEND source_paths "include/jk/${source}")
endforeach()

add_library(kit INTERFACE)
add_library(joker::kit ALIAS kit)

target_sources(kit INTERFACE
	FILE_SET jk_header_set
	TYPE HEADERS
	BASE_DIRS include
	FILES ${source_paths}
)

target_include_directories(kit INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_link_libraries(kit INTERFACE ${link_libs})

target_compile_definitions(kit INTERFACE ${definitions})

target_compile_options(kit INTERFACE ${compile_options})

add_library(module STATIC)
add_library(joker::module ALIAS module)

set_target_properties(module PROPERTIES
    CXX_EXTENSIONS OFF
    OUTPUT_NAME jk
)
set_target_properties(module PROPERTIES DEBUG_POSTFIX d)

if(NOT CMAKE_CXX_STANDARD)
    target_compile_features(module PUBLIC "cxx_std_${JK_CXX_STANDARD}")
endif()

target_sources(module PUBLIC
    FILE_SET jk_module_set
    TYPE CXX_MODULES
    BASE_DIRS "include"
    FILES "include/jk/jk.cppm"
)

target_link_libraries(module PUBLIC joker::kit)

install(TARGETS kit module
    EXPORT jkTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILE_SET jk_header_set
    FILE_SET jk_module_set DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

### tests
option(JK_BUILD_TESTING "Build jk tests" ${BUILD_TESTING})
add_feature_info(Testing JK_BUILD_TESTING "Build jk tests")

set(JK_TESTS_NEED_BUILD "" CACHE STRING 
    "List of tests that need to be built. If a test is not in this list, it will be built on demand when running ctest."
)

if(JK_BUILD_TESTING)
	add_subdirectory(tests)
endif()

### package
include(CMakePackageConfigHelpers)

install(EXPORT jkTargets
    FILE jkTargets.cmake
    NAMESPACE joker::
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/jk/cmake"
    EXPORT_PACKAGE_DEPENDENCIES
)

write_basic_package_version_file(
    "jkConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/buildsystem/jkConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/jk/cmake"
)

install(
    FILES 
        "${PROJECT_BINARY_DIR}/jkConfig.cmake"
        "${PROJECT_BINARY_DIR}/jkConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/jk/cmake"
)
