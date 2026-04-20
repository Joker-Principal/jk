set_project("jk")
set_version("1.0.0")
set_description("Joker's C++ Kit")

option("header_only")
	set_description("build header-only library")
	set_showmenu(true)
	set_default(false)

option("module_std")
	set_description("set C++ standard version")
	set_showmenu(true)
	set_default("23")
	set_values("20", "23")

option("tests")
	set_description("enable tests")
	set_showmenu(true)
	set_default(true)

target("jk")
	if has_config("header_only") then
		set_kind("headeronly")
	else
		set_kind("static")
		set_languages("cxx$(module_std)")
		set_policy("build.c++.modules", true)
		add_files("include/jk/jk.cppm", {public = true})
	end
	add_includedirs("include", {public = true})
	add_headerfiles("include/(jk/**.h)")
	add_cxxflags(
		"cl::/Zc:preprocessor",
		"cl::/utf-8",
		"clang_cl::/utf-8",
		{public = true}
	)

if has_config("tests") then
	includes("tests")
end
