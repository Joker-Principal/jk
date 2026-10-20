local function add_ctest_task(name, src)
	for _, std in ipairs({"20", "23"}) do
		local target_name = "test_" .. std .. "_" .. name
		target(target_name)
			set_kind("binary")
			set_default(false)
			set_languages("cxx" .. std)
			add_deps("jk")
			add_files(src)
			add_tests("default")
	end
end

add_ctest_task("type_list", "type-list.cpp")
add_ctest_task("callables", "callables.cpp")
add_ctest_task("string", "string.cpp")
add_ctest_task("location", "location.cpp")
add_ctest_task("type_var", "type-var.cpp")
add_ctest_task("expected", "expected.cpp")

if has_config("log") then
	add_ctest_task("log", "log.cpp")
end

if has_config("sqlite") then
	add_ctest_task("sqlite", "sqlite.cpp")
end
