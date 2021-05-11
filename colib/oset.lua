---@type cooset
local oset = require "colibc.oset"

---@class cosetrank 排行信息
---@field rank number
---@field value any
---@field score number

---取一段排名，返回一个列表
---@param setobj coosetobj
---@param rank number 起始排行
---@param count number 排行数量
---@return cosetrank[]
function oset.getranklist(setobj, rank, count)
	local t = {}
	for r, v, s in setobj:itrbyrank(rank) do
		if count > 0 then
			count = count - 1
			t[#t+1] = {
				rank = r,
				value = v,
				score = s,
			}
		else
			break
		end
	end
	return t
end

return oset