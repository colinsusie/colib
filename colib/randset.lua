---
--- 随机集合对象
---
--- Created by colin.
--- DateTime: 2020/11/3 7:41
---

local tinsert = table.insert
local tremove = table.remove
local tmove = table.move
local tconcat = table.concat
local mrandom = math.random
local ipairs = ipairs
local pairs = pairs
local tostring = tostring
local sformat = string.format

---@class RandSet
---@field list table
---@field idxs table
local RandSet = {}

local RandSetMT = {
	__index = RandSet,
	__pairs = function(t)
		local itr = function(t, k)
			return next(t.idxs, k)
		end
		return itr, t, nil
	end,
	__len = function(t)
		return #t.list
	end,
	__tostring  = function(t)
		return sformat("RandSet: %s", t.idxs)
	end
}

---增加元素
---@return boolean 是否增加成功
function RandSet:add(e)
	if self.idxs[e] then
		return false
	else
		tinsert(self.list, e)
		self.idxs[e] = #self.list
		return true
	end
end

---删除元素
function RandSet:remove(e)
	local idx = self.idxs[e]
	if idx then
		local ln = #self.list
		if idx > 0 and idx <= ln then
			local re
			if idx == ln then
				re = tremove(self.list)
				self.idxs[re] = nil
			else
				re = self.list[idx]
				self.idxs[re] = nil
				local le = tremove(self.list)
				self.list[idx] = le
				self.idxs[le] = idx
			end
		end
	end
end

---判断集合中是否存在元素e
---@return boolean 是否存在
function RandSet:has(e)
	return self.idxs[e] ~= nil
end

---从集合随机选择n个元素返回，如果n为nil，则返回元素，否则返回元素列表
---@param n number
---@return any|any[]
function RandSet:choice(n)
	local ln = #self.list
	if not n then
		return self.list[mrandom(1, ln)]
	elseif n >= ln then
		return tmove(self.list, 1, ln, 1, {})
	else
		local s = 1
		local i, ei, es
		for _ = 1, n do
			i = mrandom(s, ln)
			ei = self.list[i]
			es = self.list[s]
			self.list[s] = ei
			self.idxs[ei] = s
			self.list[i] = es
			self.idxs[es] = i
			s = s + 1
		end
		return tmove(self.list, 1, s -1, 1, {})
	end
end

---将集合中的元素连接成字符串返回
---@param sep string 分隔符
---@param i number 起始索引，默认为1
---@param j number 结束索引，默认为#set
---@return string
function RandSet:concat(sep, i, j)
	i = i or 1
	j = j or #self.list
	local t = {}
	for _, e in ipairs(self.list) do
		t[#t+1] = tostring(e)
	end
	return tconcat(t, sep)
end

---新建一个集合对象返回
---@return RandSet
function RandSet.new()
	return setmetatable({
		list = {},
		idxs = {},
	}, RandSetMT)
end

-------------------------------------------------------------------
local function test()
	local set = RandSet.new()
	print("add---------------")
	for i = 1, 10 do
		set:add(i)
	end
	print(set)
	print("len: ", #set)
	print("remove---------------")
	set:remove(8)
	set:remove(7)
	set:remove(3)
	print(set)
	print("has---------------")
	print(set:has(8), set:has(1), set:has(10))
	print("choice---------------")
	print(set:choice())
	print(tconcat(set:choice(2), ", "))
	print(tconcat(set:choice(5), ", "))
	print(tconcat(set:choice(10), ", "))
	print(tconcat(set:choice(0), ", "))
	print("for---------------")
	for e in pairs(set) do
		print(e)
	end
end
--test()
-------------------------------------------------------------------

return RandSet