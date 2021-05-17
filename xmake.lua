
add_rules("mode.debug", "mode.release")

target("colibc") do
	set_kind("shared")						-- 动态库
	add_files("src/*.c")					-- 源代码

	set_targetdir("$(projectdir)/colib")	-- 生成路径
	set_warnings("allextra")				-- 全部警告

	on_load(function (target)
		if is_plat("windows") then
			target:set("filename", "colibc.dll")
			target:add("cflags", "/utf-8")				-- 支持utf-8代码文件
			target:add("defines", "LUA_BUILD_AS_DLL")
		else
			target:set("filename", "colibc.so")
			if is_plat("macosx") then
				target:add("shflags", "-undefined dynamic_lookup")
			end

			-- 尝试找到lua头文件的搜索目录
			import("lib.detect.find_path")
			local p = find_path("lua.h", {"/usr/local/include"})
			if p then
				target:add("includedirs", p)
			end
		end
	end)
end
