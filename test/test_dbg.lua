package.cpath = package.cpath .. ";../colib/?.so"
package.path = package.path ..";../colib/?.lua"

local codbg = require "dbg"


local function func1(a, b, c)
	local d = a + b * c
	print(codbg.traceback("hello"))
	return d
end

local function func2(a, b, c)
	local d = func1(a, b, c) * 2
	return d
end

local function func3()
	func2(1, 2, 10)
end

func3()