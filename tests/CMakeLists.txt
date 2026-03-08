project(tests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(add_ctest_task name)
    if(name IN_LIST JK_TESTS_NEED_BUILD)
        add_executable(${name} ${ARGN})
        add_test(
            NAME ${name} 
            COMMAND "$<TARGET_FILE:${name}>"
        )
    else()
        add_executable(${name} EXCLUDE_FROM_ALL ${ARGN})
        add_test(
            NAME ${name}
            COMMAND ${CMAKE_CTEST_COMMAND} 
                --build-and-test ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} 
                --build-target ${name}
                --build-generator ${CMAKE_GENERATOR}
                --build-nocmake
                --test-command "$<TARGET_FILE:${name}>"
        )
    endif()
    
    target_link_libraries(${name} PRIVATE joker::kit)
endfunction()

add_ctest_task(test_type_list type-list.cpp)
add_ctest_task(test_callables callables.cpp)
