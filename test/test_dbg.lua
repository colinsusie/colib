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

--func3()

local st = {
	one = "tttt",
	two = function() end,
	three = 1111,
}
local t = {
	12, 14, 15,
	[4.0] = 16,
	[4.1] = 16,
	one = true,
	two = 23.44,
	three = "h\ael\blo world",
	four = {
		name = "jim",
		age = 24,
	},
	five = {true, false, nil, 122},
	siv = {1, 2, 3, 4},
	st1 = st,
	st2 = st,
	["aa\009cc"] = true,
	ee = "ffff\020uuu",
	ff = "ffff\x1Fuuu",
	gg = "ffff\139uuu",
	ii = "cccc\z
	dddddddd",
	jj = [[sdfasf
	asdfasfsafas
	asdfasfsafsafas]],
}
st.t = t
setmetatable(t, {
	__tostring = function()
		return "this is t"
	end
})

--print(inspect.inspect(t, {indent="    "}))
print(codbg.str(t, "\n", "    "))
print(codbg.str(nil))
print(codbg.str("aabbcc"))

print(codbg.getclock())
print(codbg.getclock())