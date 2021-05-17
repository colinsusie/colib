add_rules("mode.debug", "mode.release")

local function handle_plat(target)
	if is_plat("windows") then
		target:add("defines", "LUA_USE_WINDOWS")
		if target:kind() ~= "binary" then
			target:add("defines", "LUA_BUILD_AS_DLL")
		end
	elseif is_plat("macosx") then
		target:add("defines", "LUA_USE_MACOSX")
		target:add("links", "m")
		if target:kind() == "binary" then
			target:add("defines", "LUA_USE_READLINE")
			target:add("links", "readline")
		end
	elseif is_plat("linux") then
		target:add("defines", "LUA_USE_LINUX")
		target:add("links", "m", "dl")
		if target:kind() == "binary" then
			target:add("defines", "LUA_USE_READLINE")
			target:add("links", "readline")
		end
		target:add("ldflags", "-Wl,-E")
	end
	if is_mode("debug") then
		target:set("symbols", "debug")
	else
		target:set("symbols", "")
	end
end

target("luast") do
	set_kind("static")
	set_basename("lua-static")
	add_files("src/*.c|luac.c|lua.c")
	set_warnings("allextra")
	set_optimize("faster")
	on_load(handle_plat)
end

target("luash") do
	set_kind("shared")
	set_basename("lua")
	add_files("src/*.c|luac.c|lua.c")
	set_warnings("allextra")
	set_optimize("faster")
	on_load(handle_plat)
end

target("lua") do
	set_kind("binary")
	add_deps("luast")
	add_files("src/lua.c")
	set_warnings("allextra")
	set_optimize("faster")
	on_load(handle_plat)
end

target("luac") do
	set_kind("binary")
	add_files("src/luac.c")
	add_deps("luast")
	set_warnings("allextra")
	set_optimize("faster")
	on_load(handle_plat)
end