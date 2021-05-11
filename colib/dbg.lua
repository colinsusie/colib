local type = type
local tconcat = table.concat
local sformat = string.format
local pairs = pairs
local getmetatable = getmetatable
local srep = string.rep
local tointeger = math.tointeger
local mfloor = math.floor

---@type codbg
local dbg = require "colibc.dbg"

---@class DbgStrArg 打印对象的参数
---@field newline string 换行符，默认为\n
---@field indent string 缩进字符串，默认为"  "
---@field maxlevel number 最大层级，默认为32层
---@field level number 当前层级
---@field buffer string[] 写缓冲
---@field bufidx number 写缓冲索引
---@field tableids table<table, number> 表对象的ID映射
---@field tablecounts table<table, number> 表对象的计数
---@field genid number 表对象的最大分配ID


---判断字符串是否是标识符
---@param str string
local function isident(str)
  return str:match( "^[_%a][_%a%d]*$" )
end

---写字符串
---@param str string
local function writestr(str, arg)
	arg.buffer[arg.bufidx] = str
	arg.bufidx = arg.bufidx + 1
end

---生成对象ID
local function maketableid(t, arg)
	local id = arg.tableids[t]
	if not id then
		id = arg.genid
		arg.genid = arg.genid + 1
		arg.tableids[t] = id
	end
	return id
end

---写缩进
local function writeindent(arg)
	writestr(sformat("%s%s", arg.newline, srep(arg.indent, arg.level)), arg)
end

local writeobject

---写表键
local function writekey(k, arg)
	if type(k) == "string" and isident(k) then
		writestr(k, arg)
	else
		writestr("[", arg)
		writeobject(k, arg)
		writestr("]", arg)
	end
end

---判断K是不是数组的索引
local function isarrayindex(k, len)
	if tointeger then	-- >= 5.3
		k = tointeger(k)
		return k and 1 <= k and k <= len
	else
		return type(k) == "number" and mfloor(k) == k
			and 1 <= k and k <= len
	end
end

---写表
local function writetable(t, arg)
	if arg.tableids[t] then
		writestr(sformat("<table %s>", arg.tableids[t]), arg)
	elseif arg.level >= arg.maxlevel then
		writestr("{...}", arg)
	else
		local needindent
		writestr("{", arg)
		arg.level = arg.level + 1
		do
			if arg.tablecounts[t] > 1 then
				writestr(sformat("--<table %s>", maketableid(t, arg)), arg)
				needindent = true
			end

			local count = 0
			local len = #t
			for i = 1, len do
				if needindent then
					writeindent(arg)
					needindent = false
				end
				if count > 0 then
					writestr(", ", arg)
				end
				writeobject(t[i], arg)
				count = count + 1
			end

			needindent = false
			for k, v in pairs(t) do
				if not isarrayindex(k, len) then
					if count > 0 then
						writestr(",", arg)
					end
					writeindent(arg)
					writekey(k, arg)
					writestr(" = ", arg)
					writeobject(v, arg)
					count = count + 1
					needindent = true
				end
			end

			local mt = getmetatable(t)
			if mt then
				if count > 0 then
					writestr(",", arg)
				end
				writeindent(arg)
				writestr("<metatable> = ", arg)
				writeobject(mt, arg)
				needindent = true
			end
		end
		arg.level = arg.level - 1
		if needindent then
			writeindent(arg)
		end
		writestr("}", arg)
	end
end

---写对象
writeobject = function(obj, arg)
	local tp = type(obj)
	if tp == "string" then
		writestr(sformat("%q", obj), arg)
	elseif tp == "table" then
		writetable(obj, arg)
	elseif tp == "number" or tp == "boolean" or tp == "nil" then
		writestr(sformat("%s", obj), arg)
	else
		writestr(sformat("<%s>", obj), arg)
	end
end

---计算相同表的数量
local function count_table(obj, counts)
	counts = counts or {}
	if type(obj) == "table" then
		if counts[obj] then
			counts[obj] = counts[obj] + 1
		else
			counts[obj] = 1
			for k, v in pairs(obj) do
				count_table(k, counts)
				count_table(v, counts)
			end
			count_table(getmetatable(obj), counts)
		end
	end
	return counts
end

---打印Lua对象
---@param obj any 对象
---@param newline string 换行符，默认为\n
---@param indent string 缩进字符串，默认为"  "
---@param maxlevel number 最大层级，默认为32层
function dbg.str(obj, newline, indent, maxlevel, verbose)
	---@type DbgStrArg
	local arg = {
		newline = newline or "\n",
		indent = indent or "  ",
		verbose = verbose,
		maxlevel = maxlevel or 32,
		level = 0,
		buffer = {},
		bufidx = 1,
		tableids = {},
		genid = 1,
		tablecounts = count_table(obj)
	}

	writeobject(obj, arg)
	return tconcat(arg.buffer)
end

return dbg