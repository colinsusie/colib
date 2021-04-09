package.cpath = package.cpath .. ";../colib/?.so"
package.path = package.path ..";../colib/?.lua"

local coseri = require "seri"
local dbg = require "dbg"


local data = coseri.pack({
	0, false, true, 127, 0x7FFF, 0xFFFF, 0x7FFFFFFF, 0xFFFFFFFF, 0x0011FFFFFFFFFFFF, -212, -0xFF00FF0022, 0.12432, 32321.0333,
	sstr = "hello world",
	lstr = "Lua对象序列化，参考自skynet的序列化模块(https://github.com/cloudwu/skynet)，格式总体上相同，但有一些细微的差别，Lua对象序列化，参考自skynet的序列化模块(https://github.com/cloudwu/skynet)，格式总体上相同，但有一些细微的差别",
	lstr2 = "While The Python Language Reference describes the exact syntax",
	tb = {
		name = "tom",
		age = 24,
		male = true,
		power = 13123213.222,
		time = -2342423.3333,
		eeeeee = -32432342232342332,
	}
})
print("#data=", #data)
local obj = coseri.unpack(data)
print(dbg.str(obj))

data = coseri.pack("hello.....", true, nil, {})
print(coseri.unpack(data))

do
	local t = {}
	for i = 1, 10000 do
		t[i] = 0x0011FFFFFFFFFFFF
	end
	--print("t=", dbg.str(t))

	print("t size = ", #t)
	local ti = os.clock()
	for i = 1, 1000 do
		local td = coseri.pack(t)
		coseri.unpack(td)
	end
	print("time = ", os.clock() - ti)
end

--do
--	local robot_data = require "robot_data"
--	local ti = os.clock()
--	local seridata = coseri.pack(robot_data)
--	local obj = coseri.unpack(seridata)
--	print("time = ", os.clock() - ti)
--
--	local f = io.open("pack.log", "wb")
--	f:write(seridata)
--	f:close()
--
--	local f2 = io.open("unpack.log", "wb")
--	f2:write(dbg.str(obj))
--	f2:close()
--end

