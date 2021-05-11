-- 先这样判断是不是Windows
if package.config:sub(1, 1) == "\\" then
	package.cpath = package.cpath .. ";../colib/?.dll"
else
	package.cpath = package.cpath .. ";../colib/?.so"
end
package.path = package.path ..";../colib/?.lua"

local oset = require "oset"
local dbg = require "dbg"

----------------------------------------------------------------------
if false then
	local set = oset.new()
	print("set: ", set)
	for i = 1, 10 do
		set:add("id"..i, math.random(1, 50))
	end
	set:dump()
end

if false then
	print("delete value======================================")
	local set = oset.new()
	set:add("id1", 1)
	set:add("id2", 2)
	set:add("id3", 3)
	set:add("id4", 4)
	set:add("id5", 5)
	set:add("id6", 6)
	set:add("id7", 7)
	set:add("id8", 8)
	set:add("id9", 9)
	set:dump()

	print('set:remove("id2")', set:remove("id2"))
	print('set:remove("id4")', set:remove("id4"))
	print('set:remove("id7")', set:remove("id7"))
	print('set:remove("id6")', set:remove("id6"))
	print('set:remove("id5")', set:remove("id5"))
	set:dump()

	print('set:remove("id1")', set:remove("id1"))
	print('set:remove("id3")', set:remove("id3"))
	print('set:remove("id5")', set:remove("id5"))
	print('set:remove("id8")', set:remove("id8"))
	print('set:remove("id9")', set:remove("id9"))
	set:dump()
end

if false then
	print("update score======================================")
	local set = oset.new()
	set:add("id1", 1)
	set:add("id2", 2)
	set:add("id3", 3)
	set:add("id4", 4)
	set:add("id5", 5)
	set:add("id6", 6)
	set:add("id7", 7)
	set:add("id8", 8)
	set:add("id9", 9)
	set:dump()

	print('set:update("id4", 11)', set:update("id4", 11))
	print('set:update("id2", 6)', set:update("id2", 6))
	print('set:update("id1", 0)', set:update("id1", 0))
	set:dump()
end

if false then
	print("query======================================")
	local set = oset.new()
	set:add("id1", 21)
	set:add("id2", 23)
	set:add("id3", 1)
	set:add("id4", 37)
	set:add("id5", 23)
	set:add("id6", 52)
	set:add("id7", 78)
	set:add("id8", 23)
	set:add("id9", 53)
	set:dump()

	print('set:getbyvalue("id7")', set:getbyvalue("id7"))
	print('set:getbyvalue("id9")', set:getbyvalue("id9"))
	print('set:getbyvalue("idxxxx")', set:getbyvalue("idxxxx"))
	print('set:getbyrank(3)', set:getbyrank(3))
	print('set:getbyrank(200)', set:getbyrank(200))
	print('set:getbyrank(0)', set:getbyrank(0))
	print('set:getbyrank(1)', set:getbyrank(1))
	print('set:getbyrank(#set)', set:getbyrank(#set))
	print('set:getbyscore(23)', set:getbyscore(23))
	print('set:getbyscore(78)', set:getbyscore(78))
	print('set:getbyscore(1000)', set:getbyscore(1000))
	print('set:getbyscore(0)', set:getbyscore(0))
end

if true then
	print("iterate======================================")
	local set = oset.new()
	set:add("id1", 21)
	set:add("id2", 23)
	set:add("id3", 1)
	set:add("id4", 37)
	set:add("id5", 23)
	set:add("id6", 52)
	set:add("id7", 78)
	set:add("id8", 23)
	set:add("id9", 53)
	set:dump()

	print("set:itrbyrank(1)-------------")
	for rank, value, score in set:itrbyrank(1) do
		print(rank, value, score)
	end
	print("set:itrbyrank(4, true)-------------")
	for rank, value, score in set:itrbyrank(4, true) do
		print(rank, value, score)
	end

	print("set:itrbyrank(10)-------------")
	for rank, value, score in set:itrbyrank(10) do
		print(rank, value, score)
	end
	print("set:itrbyrank(0)-------------")
	for rank, value, score in set:itrbyrank(0) do
		print(rank, value, score)
	end

	print("set:itrbyscore(53)-------------")
	for rank, value, score in set:itrbyscore(53) do
		print(rank, value, score)
	end
	print("set:itrbyscore(23, true)-------------")
	for rank, value, score in set:itrbyscore(23, true) do
		print(rank, value, score)
	end
	print("set:itrbyscore(1000)-------------")
	for rank, value, score in set:itrbyscore(1000) do
		print(rank, value, score)
	end
	print("set:itrbyscore(0)-------------")
	for rank, value, score in set:itrbyscore(0) do
		print(rank, value, score)
	end

	print("set:itrbvalue('id6')-------------")
	for rank, value, score in set:itrbyvalue('id6') do
		print(rank, value, score)
	end
	print("set:itrbyvalue('id1')-------------")
	for rank, value, score in set:itrbyvalue('id1') do
		print(rank, value, score)
	end
	print("set:itrbyvalue('id0')-------------")
	for rank, value, score in set:itrbyvalue('id0') do
		print(rank, value, score)
	end
	print("set:itrbyvalue('id8', true)-------------")
	for rank, value, score in set:itrbyvalue('id8', true) do
		print(rank, value, score)
	end
	print("set:itrbyvalue('id2')-------------")
	for rank, value, score in set:itrbyvalue('id2') do
		print(rank, value, score)
	end

	print("oset.getranklist(set, 5, 4)------------")
	---@type cosetrank
	local t = oset.getranklist(set, 5, 4);
	for _, i in ipairs(t) do
		print(i.rank, i.value, i.score)
	end
end

if false then
	print("level======================================")
	local set = oset.new()
	for i = 1, 10000 do
		set:add("id"..i, math.random(1000))
	end
	set:dump(1)
end

if false then
	print("benchmark======================================")
	local set = oset.new()
	for i = 1, 10000 do
		set:add("id"..i, math.random(1, 3000))
	end

	local mrand = math.random
	local w = dbg.stopwatch()
	w:start()
	for i = 1, 1000000 do
		set:update("id"..mrand(1, 10000), mrand(1, 3000))
	end
	w:stop()
	print("set:update 10000: ", w:elapsed())

	w:start()
	for i = 1, 1000000 do
		set:getbyrank(mrand(1, 10000))
	end
	w:stop()
	print("set:getrank 10000: ", w:elapsed())
end

