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

set(sources)
set(link_libs)
set(definitions)

if(DEFINED JK_BUILD_TESTING)
    set(BUILD_TESTING ${JK_BUILD_TESTING})
endif()

option(JK_BUILD_TESTING "Build jk tests" ${BUILD_TESTING})
add_feature_info(Testing JK_BUILD_TESTING "Build jk tests")

if(BUILD_TESTING)
	add_subdirectory(tests)
endif()
