target("colibc") do
	set_kind("shared")
	add_files("src/*.c")
	set_targetdir("$(projectdir)/colib")	-- 生成路径
	set_objectdir("$(buildir)/objs")		-- object目标生成的路径
	set_dependir("$(buildir)/deps")			-- deps文件生成的路径
	set_warnings("allextra")				-- 全部警告
	if is_mode("release") then				-- release开启优化
		set_optimize("faster")
	end

	before_build(function (target)			-- 目标加载时
		local function input(msg)
			io.write(msg)
			io.flush()
			return io.read()
		end

		if is_plat("windows") then			-- windows
			target:set("filename", "colibc.dll")
			target:add("cflags", "/utf-8")
			target:add("defines", "LUA_BUILD_AS_DLL")

			local luadir = os.getenv("LUA_DIR")
			if not luadir then
				luadir = input("input lua include dir: ")
			end
			target:add("includedirs", luadir)
			target:add("linkdirs", luadir)
			target:add("links", "lua")
		else		-- other
			target:set("filename", "colibc.so")

			local luadir = os.getenv("LUA_DIR")
			if luadir then
				target:add("includedirs", luadir)
			elseif not os.exists("/usr/local/include/lua.h") then
				luadir = input("input lua include dir: ")
				target:add("includedirs", luadir)
			end
		end
	end)
end
