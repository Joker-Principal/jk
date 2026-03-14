project(tests)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(add_ctest_lazy_task name std)
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
	target_compile_features(${name} PUBLIC "cxx_std_${std}")
	target_link_libraries(${name} PRIVATE joker::kit)
endfunction()

function(add_ctest_build_task name std)
	add_executable(${name} ${ARGN})
	add_test(
		NAME ${name}
		COMMAND "$<TARGET_FILE:${name}>"
	)
	target_compile_features(${name} PUBLIC "cxx_std_${std}")
	target_link_libraries(${name} PRIVATE joker::kit)
endfunction()

function(add_ctest_task name)
	foreach(std IN ITEMS 20 23)
		set(target_name "${name}_${std}")
		if(name IN_LIST JK_TESTS_NEED_BUILD)
			add_ctest_build_task(${target_name} ${std} ${ARGN})
		else()
			add_ctest_lazy_task(${target_name} ${std} ${ARGN})
		endif()
	endforeach()
endfunction()

add_ctest_task(test_type_list type-list.cpp)
add_ctest_task(test_callables callables.cpp)
