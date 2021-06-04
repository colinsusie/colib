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

---获取高精度时钟
---@return number 返回秒，是一个浮点数
function codbg.hrclock() end

---新建一个stopwatch对象
---@return costopwatch
function codbg.stopwatch() end

---@class costopwatch 用于计时
local costopwatch = {}

---开始计时
function costopwatch:start() end

---结束计时
function costopwatch:stop() end

---取得start/stop的计时间隔
---@return number 返回秒数，是一个浮点数
function costopwatch:elapsed() end

---取得start/stop的计时间隔
---@return number 返回毫秒，是一个整数
function costopwatch:elapsed_ms() end

---取得start/stop的计时间隔
---@return number 返回微秒，是一个整数
function costopwatch:elapsed_us() end

---取得start/stop的计时间隔
---@return number 返回纳秒，是一个整数
function costopwatch:elapsed_ns() end

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
---@return boolean
function costr.startswith(str, prefix) end


---判断str是否以suffix结尾
---@param str string 字符串
---@param suffix string 后缀字符串
---@return boolean
function costr.endswith(str, suffix) end

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
---		否则为一个字符串，只有匹配到整个字符串才算命中。
---@return string[] 返回分隔后的字符串列表
function costr.split(str, sep, matchchar) end

---从后往前查找子字符串
---@param str string 源字符串
---@param sub string 子字符串
---@param matchchar boolean 如果为true，sub表示字符集合，只要匹配到集合中的任意字符都算命中；
---		否则为一个字符串，只有匹配到整个字符串才算命中。
---@param start number 开始查找的位置，默认是#str
---@return number, number 如果找到：
---		找不到返回 nil
---		matchchar=false：返回sub在str中的起始和结束索引。
---		matchchar=true: 返回找到的索引，以及匹配到的字符。
function costr.rfind(str, sub, matchchar, start) end


------------------------------------------------------------------------------------------------------

---@class cobitobj 位图对象

---@class cobitarray 位图数组
local cobitarray = {}


---创建一个位图数组： wordsize * bitsize = 能存放的位数量
---@param wordsize number 整型元素的长度
---@param bitsize number 一个整型的位大小
---@param cobaobj
function cobitarray.new(wordsize, bitsize) end

---返回整型元素列表
---@param obj cobitobj
---@return number[]
function cobitarray.tointegers(obj) end

---返回位列表
---@param obj cobitobj
---@return boolean[]
function cobitarray.tobooleans(obj) end

---从整型数组加载位数据
---@param obj cobitobj
---@param ints number[]
function cobitarray.fromintegers(obj, ints) end

---从布尔数组加载位数据
---@param obj cobitobj
---@param bools boolean[]
function cobitarray.frombooleans(obj, bools) end

---将位图数组的内容串连成字符串返回
---@param obj cobitobj
---@param sep string 分隔符
---@param i number 起始索引，默认为1
---@param j number 结束索引，默认为#list
---@return string
function cobitarray.concat(obj, sep, i, j) end

---清除位图数组的内容
function cobitarray.clear(obj) end

---交换位图数组的两个值
---@param obj cobitobj
---@param i number
---@param j number
function cobitarray.exchange(obj, i, j) end

---调整位图数组的大小，注意wordsize是整型元素的数量
---@param obj cobitobj
---@param wordsize number 整型元素的数量
function cobitarray.resize(obj, wordsize) end

---设置位图数组的值，作用类似于 cobitobj[i] = v，不同的是如果i超出数组边界，会自动扩充数组
---@param obj cobitobj
---@param i number
---@param v boolean
function cobitarray.set(obj, i, v) end

---返回位图数组的整型元素的数量
---@param obj cobitobj
---@return number
function cobitarray.wordsize(obj) end

---返回位图数组的整型元素的位大小
---@param obj cobitobj
function cobitarray.wordbits(obj) end


------------------------------------------------------------------------------------------------------

---@class cofilesys 文件系统模块
---@field sep string 路径分隔符，Windows下为 \, 其他为 /
---@field allseps string 所有有效的分隔符，Windows下为 \/，其他系统只有 /
---@field iswindows boolean  是否在Windows平台，Windows的路径和其他系统不一样
local cofilesys = {}

---扫描目录，返回一个迭代器，用在for循环中，不会包含 . 和 .. 两个特殊目录
---@param path string 目录路径
function cofilesys.scandir(path) end

---判断文件或目录是否存在
---@param path string 路径
---@return boolean 是否存在
function cofilesys.exists(path) end

---获得文件的大小
---@param path string 路径
---@return number 如果成功返回文件大小，否则返回nil, 和错误消息
function cofilesys.getsize(path) end

---取文件的修改时间，返回的时间是一个浮点数
---@param path string 路径
---@return number 如果成功返回时间戳，否则返回nil, 和错误消息
function cofilesys.getmtime(path) end

---取文件的访问时间，返回的时间是一个浮点数
---@param path string 路径
---@return number 如果成功返回时间戳，否则返回nil, 和错误消息
function cofilesys.getatime(path) end

---取文件的创建或状态改变的时间，返回的时间是一个浮点数
---@param path string 路径
---@return number 如果成功返回时间戳，否则返回nil, 和错误消息
function cofilesys.getctime(path) end

---取文件的模式
---@param path string 路径
---@return number 如果成功返回文件模式，否则返回nil, 和错误消息
function cofilesys.getmode(path) end

---创建目录
---@param path string 路径
---@return boolean 是否成功，如果失败，后面带有错误消息
function cofilesys.mkdir(path) end

---删除目录
---@param path string 路径
---@return boolean 是否成功，如果失败，后面带有错误消息
function cofilesys.rmdir(path) end

---修改工作目录
---@param path string 路径
---@return boolean 是否修改成功，如果失败，后面带有错误消息
function cofilesys.chdir(path) end

---取当前工作目录
---@return string 返回工作目录，如果失败返回nil和一个错误消息
function cofilesys.getcwd() end


------------------------------------------------------------------------------------------------------

---@class cooset 有序集合模块
local cocoset = {}

---创建一个有序集合
---@return coosetobj
function cocoset.new() end

---@class coosetobj 有序集合对象
local coosetobj = {}

---取集合长度
---@return number
function coosetobj:getlen() end

---增加元素
---@param value any 除了nil外的其他值
---@param score number 分数，是一个整型，分数越大越在前面
---@return boolean 是否增加成功，值重复时失败
function coosetobj:add(value, score) end

---删除元素
---@param value any 除了nil外的其他值
---@return boolean 是否成功，值没有在集中时失败
function coosetobj:remove(value) end

---更新元素的分数
---@param value any 除了nil外的其他值
---@param newscore number 新的分数
---@return boolean 是否成功，值没有在集中时失败
function coosetobj:update(value, newscore) end

---通过值取信息，排名从1开始算
---@param value any 除了nil外的其他值
---@return number, number, number 找不到返回nil，成功返回三元组：排名，值，分数
function coosetobj:getbyvalue(value) end

---通过排名取信息，排名从1开始算
---@param rank number 排名值
---@return number, number, number 找不到返回nil，成功返回三元组：排名，值，分数
function coosetobj:getbyrank(rank) end

---通过分数取信息，排名从1开始算，总是找到排名最前的分数
---@param score number 分数
---@return number, number, number 找不到返回nil，成功返回三元组：排名，值，分数
function coosetobj:getbyscore(score) end

---返回排名在rank处的迭代器，用在for循环中： for rank, value, score in obj:itrbyrank(1) ... end
---如果不指定rank，则从第1名开始迭代
---reverse 是否前向迭代
function coosetobj:itrbyrank(rank, reverse) end

---返回分数在score处的迭代器，用在for循环中： for rank, value, score in obj:itrbscore(1) ... end
---如果不指定score，则从第1名开始迭代
---reverse 是否前向迭代
function coosetobj:itrbyscore(score) end

---返回value处的迭代器，用在for循环中： for rank, value, score in obj:itrbvalue(1) ... end
---如果不指定value，则从第1名开始迭代
---reverse 是否前向迭代
function coosetobj:itrbyvalue(value) end

---打印集合的内容
function coosetobj:dump() end


------------------------------------------------------------------------------------------------------

---@class corand 随机数生成器
local corand = {}

---新建一个生成器
---@param seed1 number 随机种子，如果这两个不指定，则内部用默认的种子
---@param seed2 number
---@return corandobj
function corand.new(seed1, seed2) end

---@class corandobj 随机数生成器
local corandobj = {}

---重新设置随机种子
---@param seed1 number 随机种子
---@param seed2 number 随机种子
function corandobj:setseed(seed1, seed) end

---生成下一个整数，如果 a, b指定，则生成[a, b]；如果不指定，则生成int64范围的整数
---@param a number 范围
---@param b number 范围
---@return number
function corandobj:nextint(a, b) end

---生成下一个浮点数，如果 a, b指定，则生成[a, b)；如果不指定，则生成[0, 1)范围的浮点数
---@param a number 范围
---@param b number 范围
---@return number
function corandobj:nextfloat(a, b) end

----------------------------------------------------------------------------------------------------
---@class cocodec 编码模块
local cocodec = {}

---base64编码
---@field str string 源字符串
---@return string 返回编码后的base64字符串
function cocodec.b64encode(str) end

---base64解码
---@field str string 源字符串
---@return string 返回解码后的字符串
function cocodec.b64decode(str) end


----------------------------------------------------------------------------------------------------
---@class cohashf 非加密哈希函数
local cohashf = {}

function cohashf.fnv1a64(str) end
function cohashf.fnv1a32(str) end
function cohashf.djb2(str) end
function cohashf.sdbm(str) end
function cohashf.fnv1a32(str) end
function cohashf.rs(str) end
function cohashf.js(str) end
function cohashf.bkdr(str) end
function cohashf.dek(str) end
function cohashf.ap(str) end