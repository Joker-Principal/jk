set_project("jk")
set_version("1.0.0")
set_description("Joker's C++ Kit")

option("header_only")
	set_default(false)
	set_description("build header-only library")

option("tests")
	set_default(true)
	set_description("enable tests")

target("jk")
	if has_config("header_only") then
		set_kind("headeronly")
	else
		set_kind("static")
		set_languages("cxx20")
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
