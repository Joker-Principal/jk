cmake_minimum_required(VERSION 3.28)
project(jk 
    VERSION 1.0.0 
    LANGUAGES CXX
    DESCRIPTION "Joker's C++ Kit"
)

include(GNUInstallDirs)
include(CTest)
include(FeatureSummary)

### dependencies
find_package(Qt6 QUIET COMPONENTS Core Widgets Sql Xml)
find_package(fmt CONFIG QUIET)
find_package(nlohmann_json CONFIG QUIET)
find_package(Protobuf CONFIG QUIET)
find_package(asio CONFIG QUIET)
find_package(range-v3 CONFIG QUIET)
find_package(fftw3 CONFIG QUIET)

### create target
set(sources)
set(link_libs)
set(definitions)
set(compile_options)

## meta
list(APPEND sources 
    meta/compiler.h
    meta/type-list.h
    meta/callables.h
    wrap/string.h
)

list(APPEND compile_options
    # make MSVC preprocessor conformant to C++ standard
    # https://devblogs.microsoft.com/cppblog/announcing-full-support-for-a-c-c-conformant-preprocessor-in-msvc/
    # https://learn.microsoft.com/en-us/cpp/build/reference/zc-preprocessor?view=msvc-170
    "$<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>"
)

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

add_library(module)
add_library(joker::module ALIAS module)

set_target_properties(module PROPERTIES
    CXX_EXTENSIONS OFF
)

if(NOT CMAKE_CXX_STANDARD)
    target_compile_features(module PUBLIC cxx_std_20)
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
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILE_SET jk_header_set
    FILE_SET jk_module_set DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

### tests
if(DEFINED JK_BUILD_TESTING)
    set(BUILD_TESTING ${JK_BUILD_TESTING})
endif()

option(JK_BUILD_TESTING "Build jk tests" ${BUILD_TESTING})
add_feature_info(Testing JK_BUILD_TESTING "Build jk tests")

set(JK_TESTS_NEED_BUILD "" CACHE STRING 
    "List of tests that need to be built. If a test is not in this list, it will be built on demand when running ctest."
)

if(BUILD_TESTING)
	add_subdirectory(tests)
endif()

### package
include(CMakePackageConfigHelpers)

install(EXPORT jkTargets
    FILE jkTargets.cmake
    NAMESPACE joker::
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/jk/cmake"
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
