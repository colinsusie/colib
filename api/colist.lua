---
--- colist API说明
--- Created by colin.
--- DateTime: 2020/12/20 17:22
---

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