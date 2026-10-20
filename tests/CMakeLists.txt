project(tests)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(add_ctest_lazy_task name std)
	add_executable(${name} EXCLUDE_FROM_ALL ${ARGN})
	target_compile_features(${name} PUBLIC "cxx_std_${std}")
	target_link_libraries(${name} PRIVATE joker::kit)

	if(CMAKE_CONFIGURATION_TYPES)
		set(target_dir "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>")
		set(build_config "--build-config" "$<CONFIG>")
	else()
		set(target_dir "${CMAKE_CURRENT_BINARY_DIR}")
		set(build_config)
	endif()

	add_test(
		NAME "c${name}"
		COMMAND ${CMAKE_CTEST_COMMAND}
			--build-and-test "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}"
			--build-target "${name}"
			--build-generator "${CMAKE_GENERATOR}"
			${build_config}
			--build-nocmake
			--build-noclean
			--test-command "${target_dir}/${name}${CMAKE_EXECUTABLE_SUFFIX}"
	)
endfunction()

function(add_ctest_build_task name std)
	add_executable(${name} ${ARGN})
	target_compile_features(${name} PUBLIC "cxx_std_${std}")
	target_link_libraries(${name} PRIVATE joker::kit)
	add_test(
		NAME ${name}
		COMMAND "$<TARGET_FILE:${name}>"
	)
endfunction()

function(add_ctest_task name)
	foreach(std IN ITEMS 20 23)
		set(target_name "test_${std}_${name}")
		if(name IN_LIST JK_TESTS_NEED_BUILD)
			add_ctest_build_task(${target_name} ${std} ${ARGN})
		else()
			add_ctest_lazy_task(${target_name} ${std} ${ARGN})
		endif()
	endforeach()
endfunction()

add_ctest_task(type_list type-list.cpp)
add_ctest_task(callables callables.cpp)
add_ctest_task(string string.cpp)
add_ctest_task(location location.cpp)
add_ctest_task(type_var type-var.cpp)
add_ctest_task(expected expected.cpp)

if(JK_HAS_SPDLOG)
	add_ctest_task(log log.cpp)
endif()
