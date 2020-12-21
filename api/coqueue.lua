---
--- CoQueue API 说明
--- Created by colin.
--- DateTime: 2020/12/21 0:36
---

---@class queueobj 队列对象

---@class coqueue 队列模块
local coqueue = {}

---新建一个队伍对象
---@return queueobj
function coqueue.new() end

---将队伍转成table返回
---@param q queueobj
---@return table
function coqueue.totable(q) end

---向队伍压入值
---@param q queueobj
---@param v any
function coqueue.push(q, v) end

---从队伍弹出值
---@param q queueobj
---@return any
function coqueue.pop(q) end