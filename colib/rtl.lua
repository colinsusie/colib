---
---实现一些最基础的功能：类型和面向对象
---@author colin
---
local type = type
local pairs = pairs
local setmetatable = setmetatable
local getmetatable = getmetatable
local rawget = rawget

local rtl = {}

---取类名，外部须确保o是class或object
---@return string
function rtl.classname(o)
    return o._classname
end

---取对象的类表，外部须确保o是class或object
function rtl.getclass(o)
	return o._class
end

---尝试复制一个对象
---@return any 返回新的对象
function rtl.clone(o)
    local lookup = {}
    local function _copy(o)
        if type(o) ~= 'table' then
            return o
        elseif lookup[o] then
            return lookup[o]
        else
            local tbl = {}
            lookup[o] = tbl
            for k, v in pairs(o) do
                tbl[_copy(k)] = _copy(v)
            end
            return setmetatable(tbl, getmetatable(o))
        end
    end
    return _copy(o)
end

---将一个对象输出为字符串: 一般是用于调试
--key仅支持string,number,boolean，value可以是任意类型. 对于不支持的类型key只打印出其类型
--但对于userdata,thread,function,class,object这些，只会打印出格式化的名字
--@param o 对象
--@param f 是否格式化，默认为False
--@param depth 打印的层级深度, 默认是20级
--@return 返回字符串
function rtl.tostring(o, f, depth)
	local inspect = require "inspect"
	local option = {}
	if f then
		option.newline = '\n'
		option.indent= '  '
	else
		option.newline = ' '
		option.indent= ''
	end
	if not depth then
		depth = 10
	end
	option.depth = depth

	return inspect.inspect(o, option)
end
-- 别名简化
rtl.str = rtl.tostring

--[[
    --实现OO机制，类定义如下, 切记类名在正确：
    MyClass = class("MyClass", BaseClass)
    
    --构造函数：在里面定义实例成员，构造函数的参数是可变的
    function MyClass._init(self, m1, m2)
        --这里调用父类的构造函数
        rtl.super(MyClass)._init(self, m1, m2)
        self.mem1 = m1
        self.mem2 = m2
    end

    -- 定义静态成员
    MyClass.classmem1 = 1
    MyClass.classmem2 = "Hello"

    -- 定义方法
    MyClass.func(self, a, b)
        return self.mem1 * a + self.mem2 * b
    end

    -- 使用如下：
    local mycls = MyClass(10, 20)
    mycls:func(1, 2)

]]
rtl._class_metatable = nil

---定义类：
---@param classname string 类名
---@param super table 基类，可选，省略表示不从任何类继承
---@return table 返回代表类的表
function rtl.class(classname, super)
    local klass = {}
    klass._classname = classname
    klass._class = klass
    klass._super = super
    klass._type = 'class'
    klass.__index = klass
    klass.__name = "Instance: " .. classname       -- tostring能打印类名

    -- 把类的元表保存到全局变量
    local _class_metatable = rtl._class_metatable
    if not _class_metatable then
        _class_metatable = {
            __index = function (t, k)
                local super = rawget(t, '_super')
                if super then
                    return super[k]
                else
                    return nil
                end
            end,

            __call = function (cls, ...)
                local obj = {}
                obj._type = 'object'
                setmetatable(obj, cls)
                local _init = cls._init
                if _init then
                    _init(obj, ...)
                end
                return obj
            end,

            __tostring = function (t)
                return "Class: " .. t._classname
            end
        }
        rtl._class_metatable = _class_metatable
    end
    setmetatable(klass, _class_metatable) 
    return klass
end

---返回一个类的父类
function rtl.super(klass)
    return klass._super
end

---判断一个对象是不是某个类的实例
--@param obj 对象
--@param klass 类
--@return boolean
function rtl.is_instance(obj, klass)
    if not obj then 
        return false 
    end
    local c = obj._class
    while c do
        if klass ~= c then 
            c = c._super
        else 
            return true 
        end
    end
    return false
end

function rtl.traceback(msg, level)
    local gcore = require "gcore"
    level = level or 3
    return gcore.traceback(tostring(msg), level)
end


-------------------------------------------------------------------------------
--测试
local function _test()
    -- 打印和复制对象
    local t = {
        k1 = "hello",
        k2 = 2323,
        k3 = true,
        k4 = {1, 2, 4, 6},
        k5 = {
            s1 = "one",
            s2 = "two",
            s3 = "three",
        },
        k6 = function() end,
        [true] = false,
        [{}] = "world",
    }
    print(rtl.tostring(t, true))
    local tc = rtl.clone(t)
    print(rtl.tostring(tc, false))


    -- 类和对象
    local AClass = rtl.class("AClass")
    function AClass._init(self, a, b)
        print('AClass._init')
        self.a = a
        self.b = b
    end
    function AClass.func_a(self)
        print("AClass:" .. rtl.classname(self))
    end

    local BClass = rtl.class("BClass", AClass)
--    function BClass._init(self, a, b)
--        rtl.super(BClass)._init(self, a, b)
--        print('BClass._init')
--    end
    function BClass.func_b(self)
        print("BClass:" .. rtl.classname(self))
    end

    local b = BClass(1, 2)
    b:func_a()
    b:func_b()
end
-- _test()


return rtl
