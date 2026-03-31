set_project("jk")
set_version("1.0.0")
set_description("Joker's C++ Kit")

option("JK_BUILD_TESTING")
	set_default(true)
	set_showmenu(true)
	set_description("Build jk tests")

target("kit")
	set_kind("headeronly")
	add_headerfiles(
		"include/jk/meta/compiler.h",
		"include/jk/meta/type-list.h",
		"include/jk/meta/callables.h",
		"include/jk/wrap/string.h"
	)
	add_includedirs("include", {public = true})
	add_cxxflags(
		"cl::/Zc:preprocessor",
		"cl::/utf-8",
		"clang_cl::/utf-8",
		{public = true}
	)

target("module")
	set_kind("static")
	set_languages("cxx20")
	set_policy("build.c++.modules", true)
	add_deps("kit", {public = true})
	add_files("include/jk/jk.cppm", {public = true})

if has_config("JK_BUILD_TESTING") then
	includes("tests")
end
