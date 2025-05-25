project(tests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(add_ctest_task name)
    add_executable(${name} EXCLUDE_FROM_ALL ${ARGN})
    target_link_libraries(${name} PRIVATE joker::kit)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_CTEST_COMMAND} --build-and-test ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} 
            --build-target ${name}
            --build-generator ${CMAKE_GENERATOR}
            --build-nocmake
            --build-noclean
            --test-command ${name}
    )
endfunction()

add_ctest_task(test_type_list type-list.cpp)
add_ctest_task(test_callables callables.cpp)
