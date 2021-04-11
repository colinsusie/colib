local tinsert = table.insert
local mrand = math.random
local mfloor = math.floor

local weightrand = {}


---基于权重的随机选择算法
---@param values any[] 值列表
---@param weight number[] 值对应的权重列表
---@return fun() 返回一个闭包，调用该闭包才得到最终的值；闭包返回随机到的值和索引
function weightrand.choice(values, weights)
	assert(#values == #weights)

	local count = #weights
	local sum = 0		-- 计算总和
	for _, w in ipairs(weights) do
		sum = sum + w
	end
	local avg = sum / count		-- 平均权重

	local aliases = {}		-- 别名表
	for _, _ in ipairs(weights) do
		tinsert(aliases, {1, false})
	end

	local sidx = 1	-- 找到第1个小于平均值的索引
	while sidx <= count and weights[sidx] >= avg do
		sidx = sidx + 1
	end
	if sidx <= count then	-- 如果 small_i > count 表示所有权重值相等，什么也不用处理
		local small = {sidx, weights[sidx] / avg}
		local bidx = 1		-- 找到第1个大于等于平均值的索引
		while bidx <= count and weights[bidx] < avg do
			bidx = bidx + 1
		end
		local big = {bidx, weights[bidx] / avg}

		while true do
			aliases[small[1]] = {small[2], big[1]}	-- 桶的索引即是小权重的索引，从中去掉小权重的比例，剩余的放大权重
			big = {big[1], big[2] - (1-small[2])}	-- 大权重去掉已放入的比例
			if big[2] < 1 then	-- 如果大权重剩余的比例已小于1，表示小于平均权重
				small = big		-- 大权重变成小权重
				bidx = bidx + 1	-- 找下一个大权重的索引
				while bidx <= count and weights[bidx] < avg do
					bidx = bidx + 1
				end
				if bidx > count then
					break
				end
				big = {bidx, weights[bidx] / avg} -- 得到下一个大权重
			else	-- 大权重的比例大于等于1，表示不比平均权重小，继续找小权重
				sidx = sidx + 1		-- 找下一个小权重索引
				while sidx <= count and weights[sidx] >= avg do
					sidx = sidx + 1
				end
				if sidx > count then
					break
				end
				small = {sidx, weights[sidx] / avg}
			end
		end
	end

	return function()
		local n = mrand() * count
		local i = mfloor(n)
		local odds, alias = aliases[i+1][1], aliases[i+1][2]	-- 小权重比例，大权重索引
		local idx
		if n - i > odds then
			idx =  alias
		else
			idx = i + 1
		end
		return values[idx], idx
	end
end

return weightrand
