set_project("jk")
set_description("Joker's C++ Kit")
set_version("2.6.1")

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

option("log")
	set_description("enable wrap/log")
	set_showmenu(true)
	set_default(true)
option_end()

option("sqlite")
	set_description("enable wrap/sqlite")
	set_showmenu(true)
	set_default(true)
option_end()

if has_config("log") then
	add_requires("spdlog")
end

if has_config("sqlite") then
	add_requires("sqlite3")
end

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
	if has_config("log") then
		add_packages("spdlog", {public = true})
		add_defines("JK_HAS_SPDLOG=1", {public = true})
	else
		remove_headerfiles("include/jk/wrap/log.h")
	end
	if has_config("sqlite") then
		add_packages("sqlite3", {public = true})
		add_defines("JK_HAS_SQLITE=1", {public = true})
	else
		remove_headerfiles("include/jk/wrap/sqlite.h")
	end
	add_cxxflags(
		"cl::/Zc:preprocessor",
		"cl::/utf-8",
		"clang_cl::/utf-8",
		{public = true}
	)

if has_config("tests") then
	includes("tests")
end
