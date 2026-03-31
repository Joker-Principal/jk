local function add_ctest_task(name, src)
	for _, std in ipairs({"20", "23"}) do
		local target_name = "test_" .. std .. "_" .. name
		target(target_name)
			set_kind("binary")
			set_default(false)
			set_languages("cxx" .. std)
			add_deps("kit")
			add_files(src)
			add_tests("default")
	end
end

add_ctest_task("type_list", "type-list.cpp")
add_ctest_task("callables", "callables.cpp")
add_ctest_task("string", "string.cpp")

target("test_module")
	set_kind("binary")
	set_default(false)
	add_deps("module")
	set_languages("cxx20")
	add_files("module.cpp")
	add_tests("default")
