---
--- 一个least recently cache实现， 使用哈希表和循环链表。
---
--- Created by colin.
--- DateTime: 2021/3/23 10:23
---

local sformat = string.format
local pairs = pairs
local mmin = math.min
local setmetatable = setmetatable

---@alias LruDelfunc fun(cache: LruCache, k: any, v: any) 删除函数回调原型

---@class LruNode 缓存结点
---@field next LruNode 下一个结点
---@field prev LruNode 上一个结点
---@field key any 缓存的键
---@field value any 缓存的值

---@class LruCache
---@field size number 缓存已使用的结点数量
---@field cap number 缓存可用的结点容量
---@field delfunc LruDelfunc 删除结点时的回调
---@field hash table<any, LruNode> 用于快速查找缓存的结点
---@field head LruNode 结点头
---@field forbidset boolean 禁止设置缓存，在删除回调时不允许设置缓存
local LruCache = {}

local LruCacheTM = {
	__index = LruCache,
	__pairs = function(t)
		local itr = function(cache, key)
			if not key then
				local node = cache.head
				if node.key and cache.hash[node.key] then
					return node.key, node.value
				end
			else
				local node = cache.hash[key]
				if node then
					node = node.next
					if node ~= cache.head then
						return node.key, node.value
					end
				end
			end
		end
		return itr, t, nil
	end,
	__len = function(t)
		return t.size
	end,
	__tostring = function(t)
		return sformat("LruCache: %s", t.hash)
	end,
}

---新建一个LruCache对象
---@param cap number 缓存的容量
---@param delfunc LruDelfunc 删除元素时的回调，也可不指定
---@return LruCache
function LruCache.new(cap, delfunc)
	---@type LruCache
	local cache = setmetatable({
		cap = 1,
		size = 0,
		delfunc = delfunc,
		hash = {},
		head = {},
		fobidset = false,
	}, LruCacheTM)
	cache.head.next = cache.head
	cache.head.prev = cache.head
	cache:setcap(cap)
	return cache
end

---触发删除回调
---@param cache LruCache
local function invoke_delfun(cache, k, v)
	if cache.delfunc then
		cache.forbidset = true
		cache:delfunc(k, v)
		cache.forbidset = false
	end
end

---设置缓存容量
---@param cap number
function LruCache:setcap(cap)
	assert(cap > 0)
	assert(not self.fobidset)
	---@type LruNode
	local node
	if cap > self.cap then
		for _ = 1, cap - self.cap do
			node = {}
			node.next = self.head
			node.prev = self.head.prev
			self.head.prev.next = node
			self.head.prev = node
		end
		self.cap = cap
	elseif cap < self.cap then
		for _ = 1, self.cap - cap do
			node = self.head.prev	-- tail
			self.head.prev = node.prev
			node.prev.next = self.head
			if self.hash[node.key] then
				self.hash[node.key] = nil
				invoke_delfun(self, node.key, node.value)
			end
		end
		self.cap = cap
		self.size = mmin(self.cap, self.size)
	end
end

---清除缓存内容
function LruCache:clear()
	assert(not self.fobidset)
	for _, n in pairs(self.hash) do
		n.key = nil
		n.value = nil
	end
	self.hash = {}
	self.size = 0
end

---缓存中是否包含该key
---@param key any
---@return boolean
function LruCache:contained(key)
	return not not self.hash[key]
end

---移动到链表尾，也就是头的前面
---@param cache LruCache
---@param node LruNode
local function move_to_tail(cache, node)
	node.prev.next = node.next
	node.next.prev = node.prev
	node.prev = cache.head.prev
	node.next = cache.head.prev.next
	node.next.prev = node
	node.prev.next = node
end

---从缓存取值，同时更新结点的使用
---@param key any 键
---@param def any 默认值
---@return any 返回值，如果找不到返回def
function LruCache:get(key, def)
	local node = self.hash[key]
	if node then
		move_to_tail(self, node)
		self.head = node
		return node.value
	end
	return def
end

---加进缓存
---@param key any 键
---@param value any 值
function LruCache:set(key, value)
	assert(not self.forbidset)
	local node = self.hash[key]
	if node then
		node.value = value
		move_to_tail(self, node)
		self.head = node
	else
		local dk, dv
		node = self.head.prev		-- 取尾结点
		if self.hash[node.key] then	-- 如果尾结点已用，要先删除它
			self.hash[node.key] = nil
			self.size = self.size - 1
			dk, dv = node.key, node.value
		end
		node.key = key
		node.value = value
		self.hash[key] = node
		self.head = node
		self.size = self.size + 1
		if dk ~= nil then
			invoke_delfun(self, dk, dv)
		end
	end
end

---从缓存删除值，并返回该值
function LruCache:delete(key)
	local node = self.hash[key]
	if node then
		move_to_tail(self, node)
		self.head = node.next
		self.size = self.size - 1
		local dk, dv = node.key, node.value
		node.key = nil
		node.value = nil
		self.hash[key] = nil
		invoke_delfun(self, dk, dv)
	end
end

-------------------------------------------------------------------
local function test()
	local cache = LruCache.new(4)
	for i = 1, 8 do
		cache:set(i, i)
	end
	print("=======================================", #cache)
	for k, v in pairs(cache) do print(k, v) end
	cache:set(9, 9)
	print("=======================================", #cache)
	for k, v in pairs(cache) do print(k, v) end

	print("=======================================", #cache)
	cache:get(7)
	cache:get(6)
	print("=======================================", #cache)
	for k, v in pairs(cache) do print(k, v) end

	cache:setcap(1)
	print("=======================================", #cache)
	for k, v in pairs(cache) do print(k, v) end

	cache:set(10, 10)
	print("=======================================", #cache)
	for k, v in pairs(cache) do print(k, v) end

	cache:setcap(4)
	cache:set(11, 11)
	cache:set(12, 12)
	cache:set(14, "hello")
	cache:set(14, 14)
	print("=======================================", #cache)
	for k, v in pairs(cache) do print(k, v) end

	cache:delete(11)
	cache:delete(12)
	cache:delete(14)
	print("=======================================", #cache)
	for k, v in pairs(cache) do print(k, v) end

	print(cache:contained(11))
	print(cache:contained(10))

	cache:clear()
	print("=======================================", #cache)
	for k, v in pairs(cache) do print(k, v) end

	cache:set(11, 11)
	cache:set(12, 12)
	cache:set(13, 13)
	print("=======================================", #cache)
	for k, v in pairs(cache) do print(k, v) end

	print("len", #cache)
	print(cache)

	local cache2 = LruCache.new(4, function(c, k, v)
		print("delete", k, v)
		c:get(1)
	end)
	cache2:set(1, 1)
	cache2:set(2, 2)
	cache2:set(3, 3)
	cache2:delete(2)
	print("=======================================", #cache)
	for k, v in pairs(cache2) do print(k, v) end
	cache2:set(4, 4)
	cache2:set(5, 5)
	cache2:set(6, 6)
	cache2:set(7, 7)
	print("=======================================", #cache)
	for k, v in pairs(cache2) do print(k, v) end
end
--test()
-------------------------------------------------------------------

return LruCache