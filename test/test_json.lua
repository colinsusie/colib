-- 先这样判断是不是Windows
if package.config:sub(1, 1) == "\\" then
	package.cpath = package.cpath .. ";../colib/?.dll"
else
	package.cpath = package.cpath .. ";../colib/?.so"
end
package.path = package.path ..";../colib/?.lua"

local cojson = require "json"
local cjson = require "cjson"
local moonjson = require "moonjson"
local codbg = require "dbg"
local ok, obj, str

if false then
	-- 简单
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
		"float4": 0.0,
		"float5": 9223372036854775807,
		"float6": -9223372036854775808,
		"float7": 92233720368547758423,
		"float8": 1e+307,
		"float9": -23424,
		"float10": -111.222e+4,
		"float11": -1.222e-5,
		"float12": 1.2e+5,
		"str": "未来\"是美好的",
		"emoji": "\ud83c\udf09"
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

	str = '"\\uEF'
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
end


if false then
	local function load_benchmark()
		local stopwatch = codbg.stopwatch()

		local function doload(file, func, loadmod, count)
			local f = io.open(file, 'rb')
			local jsontext = f:read('a')
			stopwatch:start()
			for i = 1, count do
				func(jsontext)
			end
			stopwatch:stop()
			print(string.format("%s\t%s: %s", loadmod, file, stopwatch:elapsed()))
		end
		print("load============================================================")
		-- doload("test_float.json", cojson.load, "cojson", 30)
		-- doload("test_float.json", cjson.decode, "cjson", 30)
		-- doload("test_float.json", moonjson.decode, "moonjson", 30)

		-- doload("test_int.json", cojson.load, "cojson", 30)
		-- doload("test_int.json", cjson.decode, "cjson", 30)
		-- doload("test_int.json", moonjson.decode, "moonjson", 30)
		

		-- doload("test_string.json", cojson.load, "cojson", 90)
		-- doload("test_string.json", cjson.decode, "cjson", 90)
		-- doload("test_string.json", moonjson.decode, "moonjson", 90)

		-- doload("test_string2.json", cojson.load, "cojson", 200)
		-- doload("test_string2.json", cjson.decode, "cjson", 200)
		-- doload("test_string2.json", moonjson.decode, "moonjson", 200)

		-- doload("twitter.json", cojson.load, "cojson", 60)
		-- doload("twitter.json", cjson.decode, "cjson", 60)
		-- doload("twitter.json", moonjson.decode, "moonjson", 60)

		-- doload("citm_catalog.json", cojson.load, "cojson", 30)
		-- doload("citm_catalog.json", cjson.decode, "cjson", 30)
		-- doload("citm_catalog.json", moonjson.decode, "moonjson", 30)

		doload("test_word.json", cojson.load, "cojson", 100)
		doload("test_word.json", cjson.decode, "cjson", 100)
		doload("test_word.json", moonjson.decode, "moonjson", 100)

		-- doload("test_space.json", cojson.load, "cojson", 200)
		-- doload("test_space.json", cjson.decode, "cjson", 200)
		-- doload("test_space.json", moonjson.decode, "moonjson", 200)

		-- doload("test_utf8escape.json", cojson.load, "cojson", 100)
		-- doload("test_utf8escape.json", cjson.decode, "cjson", 100)
		-- doload("test_utf8escape.json", moonjson.decode, "moonjson", 100)
	end
	load_benchmark()
end
------------------------------------------------------------------------------
if false then
	obj = {
		-- name = "tom",
		-- age = 30,
		-- male = true,
		-- key = cojson.null,
		-- list = {20, 30, 40},
		-- array = {30, nil, 40, 50, 60},
		-- array2 = {30, nil, 40, 50, 60},
		-- str = "hello world",
		-- float = 11.432323,
		-- float2 = 0,
		-- float3 = 1231.22342E-5,
		-- float4 = 0.0,
		-- float5 = 9223372036854775807,
		-- float6 =   -9223372036854775807,
		-- float7 = 92233720368547758423,
		-- float8 = 1e+307,
		-- float9 = -23424,
		-- float10 = -111.222e+4,
		-- float11 = -1.222e-5,
		-- float12 = 1.2e+5,
		int = 10000,
		int2 = -2123,
		int3 = 0,
		int4 = 13412412412412,
		int5 = -9223372036854775803,
		-- str = "未来\"是美\t\f\b\\\r\n好的",
		-- dict = {
		-- 	{
		-- 		name = "xxxx",
		-- 		skills = {1, 2, 3, 4, "wwwww"},
		-- 	},
		-- 	{
		-- 		name = "xxxx",
		-- 		skills = {1, 2, 3, 4, "wwwww"},
		-- 	},
		-- 	{
		-- 		name = "xxxx",
		-- 		skills = {1, 2, 3, 4, "wwwww"},
		-- 	},
		-- 	{}, {}
		-- }
	}
	str = cojson.dump(obj, true)
	print(str)

	obj = {}
	str = cojson.dump(obj, true, true)
	print(str)

	-- do return end

	obj = {[20] = "hi"}
	str = cojson.dump(obj, false, true, true)
	print(str)

	obj = {[20] = "hi"}
	ok, str = pcall(cojson.dump, obj)
	print(ok, str)

	obj = {
		a = {
			b = {
				c = 30
			}
		}
	}
	ok, str = pcall(cojson.dump, obj, false, false, false, 3)
	print(ok, str)
end

if true then
	local function dump_benchmark()
		local stopwatch = codbg.stopwatch()
		local function dodump(file, load, dump, mod, count)
			local f = io.open(file, 'rb')
			local jsontext = f:read('a')
			local obj = load(jsontext)

			stopwatch:start()
			for i = 1, count do
				dump(obj)
			end
			stopwatch:stop()
			print(string.format("%s\t%s: %s", mod, file, stopwatch:elapsed()))
		end

		print("dump============================================================")
		-- dodump("test_float.json", cojson.load, cojson.dump, "cojson", 10)
		-- dodump("test_float.json", cjson.decode, cjson.encode, "cjson", 10)
		-- dodump("test_float.json", moonjson.decode, moonjson.encode, "moonjson", 10)

		dodump("test_int.json", cojson.load, cojson.dump, "cojson", 30)
		dodump("test_int.json", cjson.decode, cjson.encode, "cjson", 30)
		dodump("test_int.json", moonjson.decode, moonjson.encode, "moonjson", 30)
		
		-- dodump("test_string.json", cojson.load, cojson.dump, "cojson", 90)
		-- dodump("test_string.json", cjson.decode, cjson.encode, "cjson", 90)
		-- dodump("test_string.json", moonjson.decode, moonjson.encode, "moonjson", 90)

		-- dodump("test_string2.json", cojson.load, cojson.dump, "cojson", 50)
		-- dodump("test_string2.json", cjson.decode, cjson.encode, "cjson", 50)
		-- dodump("test_string2.json", moonjson.decode, moonjson.encode, "moonjson", 50)

		-- dodump("twitter.json", cojson.load, cojson.dump, "cojson", 60)
		-- dodump("twitter.json", cjson.decode, cjson.encode, "cjson", 60)
		-- dodump("twitter.json", moonjson.decode, moonjson.encode, "moonjson", 60)

		-- dodump("citm_catalog.json", cojson.load, cojson.dump, "cojson", 30)
		-- dodump("citm_catalog.json", cjson.decode, cjson.encode, "cjson", 30)
		-- dodump("citm_catalog.json", moonjson.decode, moonjson.encode, "moonjson", 30)

		-- dodump("test_word.json", cojson.load, cojson.dump, "cojson", 100)
		-- dodump("test_word.json", cjson.decode, cjson.encode, "cjson", 100)
		-- dodump("test_word.json", moonjson.decode, moonjson.encode, "moonjson", 100)

		-- dodump("test_space.json", cojson.load, cojson.dump, "cojson", 200)
		-- dodump("test_space.json", cjson.decode, cjson.encode, "cjson", 200)
		-- dodump("test_space.json", moonjson.decode, moonjson.encode, "moonjson", 200)

		-- dodump("test_utf8escape.json", cojson.load, cojson.dump, "cojson", 100)
		-- dodump("test_utf8escape.json", cjson.decode, cjson.encode, "cjson", 100)
		-- dodump("test_utf8escape.json", moonjson.decode, moonjson.encode, "moonjson", 100)
	end
	dump_benchmark()
end