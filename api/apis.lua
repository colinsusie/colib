---
--- API说明，配合EmmyLua
--- Created by colin.
--- DateTime: 2020/12/24 23:13
---

----------------------------------------------------------------------------------------------------
---@class listobj 数组对象

---@class colist 数组模块
local colist = {}

---新建一个列表对象
---@overload fun(): listobj
---@param cap number 可选，数组的初始容量
---@return listobj
function colist.new(cap) end

---向列表对象增加一个元素，如果没有指定pos则插入到最后，否则插入到pos的位置
---@overload fun(list: listobj, value: any)
---@param list listobj
---@param pos number
---@param value any
function colist.insert(list, pos, value) end

---删除一个值，并返回这个值，如果没有指定pos则删除最后的值，否则删除pos位置的值
---@overload fun(list: listobj): any
---@param list listobj
---@param pos number
---@return any
function colist.remove(list, pos) end

---将list保存转换为table返回， i, j为保存的范围，默认i=1, j=#ls
---@overload fun(list: listobj): table
---@overload fun(list: listobj, i: number): table
---@param list listobj
---@param i number 开始索引，如果不指定则为1
---@param j number 结束索引，如果不指定则默认#list
---@return table
function colist.totable(list, i, j) end

---从table加载值到list， i, j为加载的范围，默认i=1, j=#tb
---@overload fun(list: listobj, tb: table)
---@overload fun(list: listobj, tb: table, i: number)
---@param list listobj
---@param tb table
---@param i number 开始索引，如果不指定则为1
---@param j number 结束索引，如果不指定则默认#tb
---@return listobj
function colist.fromtable(list, tb, i, j) end

---将list2的值追加到list尾
---@param list listobj
---@param list2 listobj|table
function colist.extend(list, list2) end

---返回v在list中的索引，如果找不到返回nil
---@param list listobj
---@param v any
---@return number
function colist.indexof(list, v) end

---将list的内容串连成字符串返回
---@overload fun(list: listobj): string
---@overload fun(list: listobj, sep): string
---@overload fun(list: listobj, sep, i): string
---@param list listobj
---@param sep string 分隔符
---@param i number 起始索引，默认为1
---@param j number 结束索引，默认为#list
---@return string
function colist.concat(list, sep, i, j) end

---清空list的内容，shink指定是否收缩内存
---@overload fun(list: listobj)
---@param list listobj
---@param shink boolean
function colist.clear(list, shink) end

---交换两个值的位置
---@param list listobj
---@param idx1 number
---@param idx2 number
function colist.exchange(list, idx1, idx2) end

---对list原地排序：如果key函数指定，则优先用key；如果cmp函数指定，则用cmp；否则用默认的a < b比较
---@overload fun(list:table):number
---@param list listobj
---@param cmp fun(a, b): boolean  比较函数： a < b 返回true，a排在b前面
---@param key fun(v): number key函数，返回元素的整型值用于排序
function colist.sort(list, cmp, key) end


----------------------------------------------------------------------------------------------------
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


----------------------------------------------------------------------------------------------------

---@class codbg 调试模块
local codbg = {}

---打印traceback，作用类似debug.traceback，但会显示每一层的变量信息
---@param msg string 附加消息
---@param level number 从第几层级开始，默认为1
---@param max number 打印的最大层次，默认是20
---@return string 返回堆栈字符串
function codbg.traceback(msg, level, max) end

------------------------------------------------------------------------------------------------------
---@class coseri Lua序列化模块，限制：不能有函数对象，忽略元表
local coseri = {}

---序列化对象
---@vararg any 对象列表，可以打包多个对象
---@return string 返回打包后的数据
function coseri.pack(...) end

---反序列化对象
---@data string 序列化数据
---@return any 反序列化出来的对象列表
function coseri.unpack(data) end

------------------------------------------------------------------------------------------------------

---@class costr 字符串模块，不支持unicode字符
local costr = {}

---判断str是否从prefix开始
---@param str string 字符串
---@param prefix string 后缀字符串
---@param s number 开始索引，默认等于1；支持负索引，-1表示#str
---@param e number 结束索引，默认等于#str；支持负索引，-1表示#str
---@return boolean
function costr.startswith(str, prefix, s, e) end


---判断str是否以suffix结尾
---@param str string 字符串
---@param suffix string 后缀字符串
---@param s number 开始索引，默认等于1；支持负索引，-1表示#str
---@param e number 结束索引，默认等于#str；支持负索引，-1表示#str
---@return boolean
function costr.endswith(str, suffix, s, e) end

---返回长度为width的左对齐字符串，右边空余处用fillchar填充
---如果str的长度大于width，则直接返回str。
---@param str string 原字符串
---@param width number 对齐后的宽度
---@param fillchar string 填充字符，默认为空格
---@return string
function costr.ljust(str, width, fillchar) end

---返回长度为width的右对齐字符串，左边空余处用fillchar填充
---如果str的长度大于width，则直接返回str。
---@param str string 原字符串
---@param width number 对齐后的宽度
---@param fillchar string 填充字符，默认为空格
---@return string
function costr.rjust(str, width, fillchar) end

---返回长度为width的居中对齐字符串，两边空余处用fillchar填充
---如果str的长度大于width，则直接返回str。
---@param str string 原字符串
---@param width number 对齐后的宽度
---@param fillchar string 填充字符，默认为空格
---@return string
function costr.center(str, width, fillchar) end

---删除前导字符串
---@param str string 源字符串
---@param chars string 要删除的字符串集合，默认为空格(\t\n\v\f\r )
---@return string 返回结果字符串
function costr.lstrip(str, chars) end

---删除尾随字符
---@param str string 源字符串
---@param chars string 要删除的字符串集合，默认为空格(\t\n\v\f\r )
---@return string 返回结果字符串
function costr.rstrip(str, chars) end

---删除前导和尾随字符
---@param str string 源字符串
---@param chars string 要删除的字符串集合，默认为空格(\t\n\v\f\r )
---@return string 返回结果字符串
function costr.strip(str, chars) end

---将字符串分隔为多个子字符串，注意该函数只支持0结尾的字符串
---@param str string 源字符串
---@param sep string 分隔串，比如, = <>等等
---@param matchchar boolean 如果为true，sep表示字符集合，只要匹配到集合中的任意字符都算命中；
---	否则为一个字符串，只有匹配到整个字符串才算命中。
---@return string[] 返回分隔后的字符串列表
function costr.split(str, sep, matchchar) end

