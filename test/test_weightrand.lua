-- 先这样判断是不是Windows
if package.config:sub(1, 1) == "\\" then
	package.cpath = package.cpath .. ";../colib/?.dll"
else
	package.cpath = package.cpath .. ";../colib/?.so"
end
package.path = package.path ..";../colib/?.lua"


-- 线性扫描
local function prepare_weighted_random1(values, weights)
    assert(#values == #weights)
    local sum = 0       -- 计算总权重
    for _, wt in ipairs(weights) do
        sum = sum + wt
    end
    return function()
        local n = math.random(1, sum)       -- 线性扫描
        for idx, wt in ipairs(weights) do
            if n <= wt then
                return values[idx], weights[idx]
            end
            n = n - wt
        end
    end
end

-- 二叉查找
local function prepare_weighted_random2(values, weights)
    local totals = {}       -- 总和列表
    local sum = 0
    for i, w in ipairs(weights) do
        sum = sum + w
        totals[i] = sum
    end

    -- 返回选择器函数
    return function()
        local n = math.random() * sum
        local mid, distance
        local low, high = 0, #weights
        while low < high do
            mid = (low + high) // 2
            distance = totals[mid+1]
            if distance < n then
                low = mid + 1
            elseif distance > n then
                high = mid
            else
                low = mid
                break
            end
        end
        return values[low+1], weights[low+1]
    end
end

-- 跳房子
local function prepare_weighted_random3(values, weights)
    assert(#values == #weights)
    local tinsert = table.insert
    local ipairs = ipairs

    local sorted_indices  = {}      -- 排序的权重索引
    for i, _ in ipairs(weights) do
        tinsert(sorted_indices, i)
    end
    table.sort(sorted_indices, function(a, b)
        return weights[a] > weights[b]
    end)

    local sorted_weights = {}   -- 排序的权重列表
    for _, i in ipairs(sorted_indices) do
        tinsert(sorted_weights, weights[i])
    end

    local totals = {}       -- 总和列表
    local sum = 0
    for i, w in ipairs(sorted_weights) do
        sum = sum + w
        totals[i] = sum
    end

    -- 返回选择器函数
    return function()
        local n = math.random() * sum
        local idx = 1
        local distance, weight, sidx
        while true do
            if totals[idx] > n then     -- 找到
                sidx = sorted_indices[idx]
                return values[sidx], weights[sidx]
            end
            weight = sorted_weights[idx]
            distance = n - totals[idx]
            idx = idx + (1 + distance // weight)
        end
    end
end

local function benchmarks()
	local weightrand = require "weightrand"
    local values = {}
    local weights = {}
    for i = 1, 10000 do
        values[i] = i
        weights[i] = math.random(1, 1000)
    end

	local choice = weightrand.choice(values, weights)
    local randomizers = {
        {"Linear Scan", prepare_weighted_random1(values, weights)},
        {"Binary Search", prepare_weighted_random2(values, weights)},
        {"Hopscotch Selection", prepare_weighted_random3(values, weights)},
        {"Alias Method ", choice},
    }

    for _, randomizer in ipairs(randomizers) do
        local tm = os.clock()
        for i = 0, 10000 do
            randomizer[2]()
        end
        print(string.format("%s time = %s", randomizer[1], os.clock() - tm))
    end

	for i = 1, 10 do
		local v, idx = choice()
		print(v)
	end
end
benchmarks()