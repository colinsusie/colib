-- 先这样判断是不是Windows
if package.config:sub(1, 1) == "\\" then
	package.cpath = package.cpath .. ";../colib/?.dll"
else
	package.cpath = package.cpath .. ";../colib/?.so"
end
package.path = package.path ..";../?.lua"

local bitarray = require "colib.bitarray"

local a = bitarray.new(2, 48)

print("len", #a)


local t = {}
for i, v in ipairs(a) do
	t[i] = tostring(v)
end
print("array: ", #t, table.concat(t, ","))

t = bitarray.tointegers(a)
print("tointegers: ", table.concat(t, ","))

for i = 1, 40 do
	a[i] = true
end
t = bitarray.tointegers(a)
print("tointegers: ", table.concat(t, ","))

print("concat", #a, bitarray.concat(a, ","))

bitarray.fromintegers(a, {1023})
print("fromintegers: ", table.concat(bitarray.tointegers(a), ","))
bitarray.fromintegers(a, {1023, 511, 511})
print("fromintegers: ", table.concat(bitarray.tointegers(a), ","))
bitarray.frombooleans(a, {true, true, false, false})
print("frombooleans: ", table.concat(bitarray.tointegers(a), ","))

bitarray.clear(a)
print("clear: ", table.concat(bitarray.tointegers(a), ","))

bitarray.resize(a, 2)
a[1] = true
bitarray.exchange(a, 1, 32)
print("exchange", a[1], a[32])
print("exchange: ", table.concat(bitarray.tointegers(a), ","))

bitarray.set(a, 49, true)
print("set: ", table.concat(bitarray.tointegers(a), ","))
bitarray.set(a, 128, true)
print("set: ", table.concat(bitarray.tointegers(a), ","))
bitarray.set(a, 1, true)
print("set: ", table.concat(bitarray.tointegers(a), ","))

print(a[49], a[128], a[48])
print(a[32], a[1], a[200], a[2])
print(bitarray.wordsize(a))
print(bitarray.wordbits(a))

bitarray.resize(a, 4)
print("resize: ", table.concat(bitarray.tointegers(a), ","))

do -- 测试哈希函数
	local hashf = require "colib.mashf"
	local s = "Licensed under the Apache License, Version 2.0 (the \"License\");"
	for _, fun in pairs(hashf) do
		print(fun(s))
	end
end

do -- 用bitarray 和 hash函数 实现一个简单的bloom filter
	local hashf = require "colib.mashf"

	local BloomFilter = {}
	function BloomFilter:add(str)
		assert(type(str) == "string")
		local idx
		for _, fun in pairs(hashf) do
			idx = (fun(str) % #self.ba) + 1
			self.ba[idx] = true
		end
	end
	function BloomFilter:contains(str)
		assert(type(str) == "string")
		local idx
		for _, fun in pairs(hashf) do
			idx = (fun(str) % #self.ba) + 1
			if not self.ba[idx] then
				return false
			end
		end
		return true
	end
	local function new_bloomfilter()
		return setmetatable({
			ba = bitarray.new(4096, 64),
		}, { __index = BloomFilter })
	end


	local words = {
		"above",
		"abuse",
		"accident",
		"accord",
		"account",
		"accounting",
		"acoustic",
		"action",
		"activex",
		"adams",
		"adderss",
		"adolescent",
		"adult",
		"adults",
	}
	local bf = new_bloomfilter()
	for _, w in ipairs(words) do
		bf:add(w)
	end
	print("action", bf:contains("action"))
	print("activex", bf:contains("activex"))
	print("biubiu", bf:contains("biubiu"))
	print("hello", bf:contains("hello"))
end

