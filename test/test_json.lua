-- 先这样判断是不是Windows
if package.config:sub(1, 1) == "\\" then
	package.cpath = package.cpath .. ";../colib/?.dll"
else
	package.cpath = package.cpath .. ";../colib/?.so"
end
package.path = package.path ..";../colib/?.lua"

local cojson = require "json"
local cjson = require "cjson"
local codbg = require "dbg"
local ok, obj, str

-- -- 简单
str = [[{
	"name": "tom",
	"age": 30,
	"male": true,
	"false": false,
	"key": null,
	"list": [20, 30, 40],
	"str": "hello world",
	"float": 11.432323,
	"float2": 0,
	"float3": 1231.22342E-5,
	"float4": 0.22342e+5,
	"float4": 0.0,
	"float5": 9223372036854775807,
	"float6": -9223372036854775808,
	"float7": 92233720368547758423,
	"float8": 1e+307,
	"str": "未来\"是美好的"
}]]
obj = cojson.load(str)
print(codbg.str(obj))

-- 错误格式
str = "falss"
ok, obj = pcall(cojson.load, str)
print(ok, codbg.str(obj))

str = "truu"
ok, obj = pcall(cojson.load, str)
print(ok, codbg.str(obj))

str = "nulll"
ok, obj = pcall(cojson.load, str)
print(ok, codbg.str(obj))

str = "-0123"
ok, obj = pcall(cojson.load, str)
print(ok, codbg.str(obj))

str = "-0.e+20"
ok, obj = pcall(cojson.load, str)
print(ok, codbg.str(obj))

str = "12e.4"
ok, obj = pcall(cojson.load, str)
print(ok, codbg.str(obj))

str = "12e3.0"
ok, obj = pcall(cojson.load, str)
print(ok, codbg.str(obj))

str = "232424224244334343432243242"
ok, obj = pcall(cojson.load, str)
print(ok, codbg.str(obj))

str = "3.122E+3423424"
ok, obj = pcall(cojson.load, str)
print(ok, codbg.str(obj))

str = '"a\\"b\\/cxxxxx\\\\xxxx\\bxxxxx\\fxxxx\\nxxx\\txxx\\v"'
ok, obj = pcall(cojson.load, str)
print(ok, codbg.str(obj))

str = '"\\u4E2D\\u534E\\u7F8E"'
ok, obj = pcall(cojson.load, str)
print(ok, codbg.str(obj))

str = '"\\u4E2"'
ok, obj = pcall(cojson.load, str)
print(ok, codbg.str(obj))

str = '"\\uEF2H'
ok, obj = pcall(cojson.load, str)
print(ok, codbg.str(obj))

str = "\"Notice: I'll be updating the information in the next weeks. It'll be broken occasionally. \z
Notice: I'll be updating the information in the next weeks. It'll be broken occasionally. Notice: I'll\z
be updating the information in the next weeks. It'll be broken occasionally. Notice: I'll be updating\z
 the information in the next weeks. It'll be broken occasionally. Notice: I'll be updating the\z
 be updating the information in the next weeks. It'll be broken occasionally. Notice: I'll be updating\z
 be updating the information in the next weeks. It'll be broken occasionally. Notice: I'll be updating\z
 be updating the information in the next weeks. It'll be broken occasionally. Notice: I'll be updating\z
 be updating the information in the next weeks. It'll be broken occasionally. Notice: I'll be updating\z
 be updating the information in the next weeks. It'll be broken occasionally. Notice: I'll be updating\z
  information in the next weeks. It'll be broken occasionally.\""
ok, obj = pcall(cojson.load, str)
print(ok, codbg.str(obj))


str = [[
	{
		"a": {
			"b": {
				"c": {
				}
			}
		}
	}
]]
ok, obj = pcall(cojson.load, str, 2)
print(ok, codbg.str(obj))

local function loadfromefile()
	print("load test.json")
	cojson.loadf("./test.json")
end
loadfromefile()

local function benchmark()
	local stopwatch = codbg.stopwatch()

	local function dotest(file, func, loadmod, count)
		local f = io.open(file, 'rb')
		local jsontext = f:read('a')
		stopwatch:start()
		for i = 1, count do
			func(jsontext)
		end
		stopwatch:stop()
		print(string.format("%s: %s, seconds: %s", loadmod, file, stopwatch:elapsed()))
	end
	dotest("./test_float.json", cojson.load, "cojson", 30)
	dotest("./test_int.json", cojson.load, "cojson", 30)
	dotest("./test_string.json", cojson.load, "cojson", 90)
	dotest("./test_string2.json", cojson.load, "cojson", 50)
	dotest("./test_string3.json", cojson.load, "cojson", 50)
	dotest("./test_word.json", cojson.load, "cojson", 200)

	dotest("./test_float.json", cjson.decode, "cjson", 30)
	dotest("./test_int.json", cjson.decode, "cjson", 30)
	dotest("./test_string.json", cjson.decode, "cjson", 90)
	dotest("./test_string2.json", cjson.decode, "cjson", 50)
	dotest("./test_string3.json", cjson.decode, "cjson", 50)
	dotest("./test_word.json", cjson.decode, "cjson", 200)

	-- dotest("./canada.json", cjson.decode, "cjson", 20)
	-- dotest("./canada.json", cojson.load, "cojson", 20)
	-- dotest("./test_word.json", cjson.decode, "cjson", 300)
	-- dotest("./test_word.json", cojson.load, "cojson", 300)
	-- dotest("./twitter.json", cjson.decode, "cjson", 60)
	-- dotest("./twitter.json", cojson.load, "cojson", 60)
	-- dotest("./citm_catalog.json", cjson.decode, "cjson", 30)
	-- dotest("./citm_catalog.json", cojson.load, "cojson", 30)
	-- dotest("./player.json", cjson.decode, "cjson", 200)
	-- dotest("./player.json", cojson.load, "cojson", 200)
end
benchmark()