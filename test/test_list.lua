package.cpath = package.cpath .. ";./?.lua"
local list = require "colua.list"

-- simple random
local seed = 123456789
local function srandom()
	seed = (1103515245 * seed + 12345) % 65536
	return seed
end

local function full_test()
	local ls = list.new()
	print("ls = ", ls)

	print("-- add value")
	for i = 1, 10 do
		ls[#ls+1] = i
	end
	print(list.concat(ls, ", "))
	print("len =", #ls)

	print("-- access value")
	print("ls[3] =", ls[3])
	ls[2] = "hello"
	print("ls[2] =", ls[2])
	print("ls[-2] =", ls[-2])
	ls[-2] = "world"
	print("ls[-2] =", ls[-2])
	print("ls[100] =", ls[100])
	print(list.concat(ls, ", "))

	print("-- insert value")
	list.insert(ls, 20)
	list.insert(ls, 30.5)
	print(list.concat(ls, ", "))
	list.insert(ls, 1, "one")
	list.insert(ls, 3, "tow")
	list.insert(ls, #ls, "three")
	print(list.concat(ls, ", "))
	-- list.insert(ls, 100, "three")  -- error
	-- list.insert(ls, 0, "three") -- error

	print("-- remove value")
	list.remove(ls)
	print(list.concat(ls, ", "))
	print(list.remove(ls, 1))
	print(list.concat(ls, ", "))
	print(list.remove(ls, 3))
	print(list.concat(ls, ", "))
	-- list.remove(ls, 0) -- error

	print("-- totable")
	local t = list.totable(ls)
	print(table.concat(t, "; "))
	t = list.totable(ls, 1, 3)
	print(table.concat(t, "; "))
	t = list.totable(ls, 4, 8)
	print(table.concat(t, "; "))
	t = list.totable(ls, -1, 30)
	print(table.concat(t, "; "))
	t = list.totable(ls, 10, 5)
	print(table.concat(t, "; "))

	print("-- fromtable")
	list.fromtable(ls, {"a", "b", "c"})
	print(list.concat(ls, ", "))
	list.fromtable(ls, {"a", "b", "c"}, 1, 2)
	print(list.concat(ls, ", "))
	list.fromtable(ls, {"a", "b", "c"}, 0, 7)
	print(list.concat(ls, ", "))
	list.fromtable(ls, {"a", "b", "c"}, 4, 2)
	print(list.concat(ls, ", "))

	print("-- extend")
	local ls2 = list.new()
	list.fromtable(ls2, {1, 2, 3, 4})
	list.fromtable(ls, {4, 5, 6, 7})
	list.extend(ls, ls2)
	print(list.concat(ls, ", "))
	list.extend(ls, {33, 44, 55})
	print(list.concat(ls, ", "))
	list.extend(ls, {})
	print(list.concat(ls, ", "))

	print("-- indexof")
	print(list.indexof(ls, 55))
	print(list.indexof(ls, 6))
	print(list.indexof(ls, "hello"))

	print("-- clear")
	list.clear(ls, true)
	print(list.concat(ls, ", "))

	print("-- exchange")
	list.fromtable(ls, {1, 2, 3, 4})
	list.exchange(ls, 2, 3)
	print(list.concat(ls, ", "))
	list.exchange(ls, 4, 1)
	print(list.concat(ls, ", "))
	list.exchange(ls, 4, 4)
	print(list.concat(ls, ", "))

	print("-- error ")
	print(pcall(function()  ls[1] = nil end))
	print(pcall(function()  ls[6] = 8 end))

	print("-- sort")
	list.fromtable(ls, {100, 3, 79, 5, 1, 9, 10, 10, 10, 11})
	-- list.sort(ls, function(a, b) return a < b end)
	list.sort(ls, nil, function(a) return a end)
	print(list.concat(ls, ", "))


	print("-- gc")
	ls = nil
	ls2 = nil
	collectgarbage()
end


local function perf_test()
	local function test_list1()
		local linsert = list.insert
		local ls = list.new()
		local c = 1000000
		local tm = os.clock()
		for i = 1, c do
			linsert(ls, i)
		end
		print("lua.list - insert after(1000000): ", os.clock() - tm)
	end

	local function test_list2()
		local linsert = list.insert
		local ls = list.new()
		local c = 100000
		local tm = os.clock()
		for i = 1, c do
			linsert(ls, 1, i)
		end
		print("lua.list - insert before(100000)", os.clock() - tm)
	end

	local function test_list3()
		local ls = list.new()
		local c = 1000000
		local linsert = list.insert
		local lremove = list.remove
		for i = 1, c do
			linsert(ls, i)
		end

		local tm = os.clock()
		for i = 1, c do
			lremove(ls)
		end
		print("lua.list - remove after(1000000)", os.clock() - tm)
	end

	local function test_list4()
		local ls = list.new()
		local c = 100000
		local linsert = list.insert
		local lremove = list.remove
		for i = 1, c do
			linsert(ls, i)
		end

		local tm = os.clock()
		for i = 1, c do
			lremove(ls, 1)
		end
		print("lua.list - remove before(100000)", os.clock() - tm)
	end

	local function test_list5()
		local ls = list.new()
		local c = 1000000
		local linsert = list.insert
		for i = 1, c do
			linsert(ls, i)
		end

		local tm = os.clock()
		for i = 1, c do
			ls[i] = ls[i] - ls[i]
		end
		print("lua.list - get/set(1000000)", os.clock() - tm)
	end

	local function test_list6()
		seed = 123456789
		local ls = list.new()
		local c = 1000000
		local linsert = list.insert
		for i = 1, c do
			linsert(ls, srandom())
		end

		local tm = os.clock()
		-- list.sort(ls, function(a, b) return a < b end)
		-- list.sort(ls)
		list.sort(ls, nil, function(a, b) return a end)
		print("lua.list - sort(1000000)", os.clock() - tm)
		-- print(list.concat(ls, ", "))
	end

	test_list1()
	test_list2()
	test_list3()
	test_list4()
	test_list5()
	test_list6()

	local function test_table1()
		local linsert = table.insert
		local ls = {}
		local c = 1000000
		local tm = os.clock()
		for i = 1, c do
			linsert(ls, i)
		end
		print("lua.table - insert after(1000000): ", os.clock() - tm)
	end

	local function test_table2()
		local linsert = table.insert
		local ls = {}
		local c = 100000
		local tm = os.clock()
		for i = 1, c do
			linsert(ls, 1, i)
		end
		print("lua.table - insert before(100000)", os.clock() - tm)
	end

	local function test_table3()
		local ls = {}
		local c = 1000000
		local linsert = table.insert
		local lremove = table.remove
		for i = 1, c do
			linsert(ls, i)
		end

		local tm = os.clock()
		for i = 1, c do
			lremove(ls)
		end
		print("lua.table - remove after(1000000)", os.clock() - tm)
	end

	local function test_table4()
		local ls = {}
		local c = 100000
		local linsert = table.insert
		local lremove = table.remove
		for i = 1, c do
			linsert(ls, i)
		end

		local tm = os.clock()
		for i = 1, c do
			lremove(ls, 1)
		end
		print("lua.table - remove before(100000)", os.clock() - tm)
	end

	local function test_table5()
		local ls = {}
		local c = 1000000
		local linsert = table.insert
		for i = 1, c do
			linsert(ls, i)
		end

		local tm = os.clock()
		for i = 1, c do
			ls[i] = ls[i] - ls[i]
		end
		print("lua.table - get/set(1000000)", os.clock() - tm)
	end

	local function test_table6()
		seed = 123456789
		local ls = {}
		local c = 1000000
		local linsert = table.insert
		for i = 1, c do
			linsert(ls, srandom())
		end

		local tm = os.clock()
		table.sort(ls, function(a, b) return a < b end)
		print("lua.table - sort(1000000)", os.clock() - tm)
		-- print(table.concat(ls, ", "))
	end

	test_table1()
	-- test_table2()	-- 太慢
	test_table3()
	-- test_table4()	-- 太慢
	test_table5()
	test_table6()
end

full_test()
perf_test()