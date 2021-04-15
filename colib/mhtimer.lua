---
--- 一个基于最小堆的定时器实现
--- Created by colin.
--- DateTime: 2021/4/14 20:35
---
local setmetatable = setmetatable
local tunpack = table.unpack
local xpcall = xpcall
local traceback = debug.traceback

---@class TimerNode 时间结点
---@field time number 超时的时间点
---@field period number 周期触发的间隔，为0表示没有周期触发
---@field callback fun(node: TimerNode, ...) 超时回调
---@field args any[] 回调函数的参数，在schedule可以传入自定义参数
---@field index number 跟踪当前结点的堆索引

---@class Timer 定时器
---@field heap TimerNode[]
local MHTimer = {}

local TimerMT = {
	__index = MHTimer,
	__len = function(t)
		return #t.heap
	end,
}

---向上移动结点
---@param heap TimerNode[] 最小堆
---@param index number 开始比较的索引
---@param node TimerNode 将占坑的结点
local function _heap_shift_up(heap, index, node)
	local parent = index // 2
	while parent >= 1 and heap[parent].time > node.time do
		heap[index] = heap[parent]
		heap[index].index = index
		index = parent
		parent = index // 2
	end
	heap[index] = node
	heap[index].index = index
end

---向下移动结点
---@param heap TimerNode[] 最小堆
---@param index number 开始比较的索引
---@param node TimerNode 将占坑的结点
local function _heap_shift_down(heap, index, node)
	local size = #heap + 1
	local child = index * 2 + 1
	while child <= size do
		if child == size or heap[child].time > heap[child-1].time then
			child = child - 1
		end  -- 取值较小的孩子比较
		if node.time > heap[child].time then
			heap[index] = heap[child]
			heap[index].index = index
			index = child
			child = index * 2 + 1
		else
			break
		end
	end
	heap[index] = node
	heap[index].index = index
end

---增加最小堆元素
---@param heap TimerNode[]
---@param node TimerNode
local function _heap_add(heap, node)
	_heap_shift_up(heap, #heap + 1, node)
end

---删除最小堆元素
---@param heap TimerNode[]
---@param node TimerNode
local function _heap_remove(heap, node)
	if node.index > 0 then
		local sz = #heap
		local lastnode = heap[sz]
		heap[sz] = nil
		if node ~= lastnode then
			local parent = node.index // 2
			if parent >= 1 and heap[parent].time > lastnode.time then
				_heap_shift_up(heap, node.index, lastnode)
			else
				_heap_shift_down(heap, node.index, lastnode)
			end
		end
		node.index = 0
	end
end

---调整元素的位置， 由于元素的时间点变化，要将其调整至合适的位置
---@param heap TimerNode[]
---@param node TimerNode
local function _head_adjust(heap, node)
	if node.index > 0 then
		local parent = node.index // 2
		if parent >= 1 and heap[parent].time > node.time then
			_heap_shift_up(heap, node.index, node)
		else
			_heap_shift_down(heap, node.index, node)
		end
	end
end

---创建一个Timer
function MHTimer.new()
	return setmetatable({
		heap = {},
	}, TimerMT)
end

---更新时间点，触发超时事件，外部要不断调用该函数，并传入当前时间点
---@param time number 时间点
function MHTimer:tick(time)
	local heap = self.heap
	---@type TimerNode
	local node
	while #heap > 0 do
		node = heap[1]
		if time >= node.time then	-- timeout
			if node.callback then
				local ok, err = xpcall(node.callback, traceback, node, tunpack(node.args))
				if not ok then
					print(err)
				end
			end
			if node.period > 0 then
				node.time = node.time + node.period
				_head_adjust(heap, node)
			else
				_heap_remove(heap, node)
			end
		else
			break
		end
	end
end

---增加调度事件
---@param first number 首次发生的时间点，注意不是间隔
---@param period number 周期性触发的间隔，如果为0表示没有周期性触发
---@param callback fun(node: TimerNode, ...) 触发时的回调函数，以及附加的任意参数
---@return TimerNode 返回结点
function MHTimer:schedule(first, period, callback, ...)
	---@type TimerNode
	local node = {
		time = first,
		period = period or 0,
		callback = callback,
		args = {...},
		index = 0,
	}
	_heap_add(self.heap, node)
	return node
end

---删除调试事件
---@param node TimerNode
function MHTimer:unschedule(node)
	_heap_remove(self.heap, node)
end

---返回触发事件的时间点，调试用
function MHTimer:get_times()
	local t = {}
	for i, node in ipairs(self.heap) do
		t[i] = node.time .. ":" .. node.period
	end
	return table.concat(t, ", ")
end

return MHTimer