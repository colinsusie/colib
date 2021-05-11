-- 先这样判断是不是Windows
if package.config:sub(1, 1) == "\\" then
	package.cpath = package.cpath .. ";../colib/?.dll"
else
	package.cpath = package.cpath .. ";../colib/?.so"
end
package.path = package.path ..";../?.lua"

local rand = require "colib.rand"



if true then
	print("=============================")
	local randobj = rand.new()
	for i = 1, 20 do
		print("randobj:nextfloat()", randobj:nextfloat())
	end
	for i = 1, 20 do
		print("randobj:nextfloat(1, 10)", randobj:nextfloat(1, 10))
	end
	for i = 1, 20 do
		print("randobj:nextint(-100, 100)", randobj:nextint(-100, 100))
	end
	for i = 1, 20 do
		print("randobj:nextint(0, 1000)", randobj:nextint(0, 1000))
	end
end

if true then
	print("same seed=============================")
	local randobj1 = rand.new(1234567, 7654321)
	for i = 1, 20 do
		print("randobj1:nextint(0, 10000)", randobj1:nextint(0, 10000))
	end
	local randobj2 = rand.new(1234567, 7654321)
	for i = 1, 20 do
		print("randobj2:nextint(0, 10000)", randobj2:nextint(0, 10000))
	end
	randobj2:setseed(5636925, 97532147)
	for i = 1, 20 do
		print("randobj2:nextint(0, 1000)", randobj2:nextint(0, 1000))
	end
end