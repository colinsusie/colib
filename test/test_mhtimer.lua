-- 先这样判断是不是Windows
if package.config:sub(1, 1) == "\\" then
	package.cpath = package.cpath .. ";../colib/?.dll"
else
	package.cpath = package.cpath .. ";../colib/?.so"
end
package.path = package.path ..";../colib/?.lua"


local Timer = require "mhtimer"
local currtime = 0

do
	local timer = Timer.new()
	for i = 1, 10 do
		timer:schedule(math.random(1, 10), 0, function(node)
			print(">>>>>>>", node.time)
		end)
	end
	print("time list = ", timer:get_times())

	while currtime < 20 do
		print("currtime", currtime)
		timer:tick(currtime)
		currtime = currtime + 1
	end
	print("time list = ", timer:get_times())
end


do
	print("============================================")
	local timer = Timer.new()
	timer:schedule(1, 2, function(node)
		print(">>>>>>>", node, node.time)
	end)
	timer:schedule(2, 2, function(node)
		print(">>>>>>>", node, node.time)
	end)
	print("time list = ", timer:get_times())

	currtime = 0
	while currtime < 10 do
		print("currtime", currtime)
		timer:tick(currtime)
		currtime = currtime + 1
	end
	print("time list = ", timer:get_times())
end


do
	print("============================================")
	local timer = Timer.new()
	for i = 1, 10 do
		timer:schedule(math.random(1, 2), math.random(1, 5), function(node)
			if currtime >= 10 then
				print("before unschedule = ", timer:get_times())
				timer:unschedule(node)
				print("after unschedule = ", timer:get_times())
			end
		end)
	end
	print("time list = ", timer:get_times())

	currtime = 0
	while currtime < 20 do
		print("currtime", currtime)
		timer:tick(currtime)
		currtime = currtime + 1
	end
	print("time list = ", timer:get_times())
end

do
	print("============================================")
	local timer = Timer.new()
	timer:schedule(1, 2, function(node)
		error("test error")
	end)
	print("time list = ", timer:get_times())

	currtime = 0
	while currtime < 5 do
		print("currtime", currtime)
		timer:tick(currtime)
		currtime = currtime + 1
	end
	print("time list = ", timer:get_times())
end