cmake_minimum_required(VERSION 3.28)
project(jk 
    VERSION 1.0.0 
    LANGUAGES CXX
    DESCRIPTION "Joker's C++ Kit"
)

include(GNUInstallDirs)
include(CTest)
include(FeatureSummary)

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

## add library
set(source_paths)
foreach(source IN LISTS sources)
	list(APPEND source_paths "include/jk/${source}")
endforeach()

add_library(kit INTERFACE)
add_library(joker::kit ALIAS kit)

target_sources(kit INTERFACE
	FILE_SET jk_file_set
	TYPE HEADERS
	BASE_DIRS include
	FILES ${source_paths}
)

target_include_directories(kit INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_link_libraries(kit INTERFACE ${link_libs})

target_compile_definitions(kit INTERFACE ${definitions})

### tests
if(DEFINED JK_BUILD_TESTING)
    set(BUILD_TESTING ${JK_BUILD_TESTING})
endif()

option(JK_BUILD_TESTING "Build jk tests" ${BUILD_TESTING})
add_feature_info(Testing JK_BUILD_TESTING "Build jk tests")

if(BUILD_TESTING)
	add_subdirectory(tests)
endif()
