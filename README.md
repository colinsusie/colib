## colib

这是一个Lua扩展库，提供了一些常用的扩展功能，该库还在不断完善中。。。

## 功能列表

已提供的模块有：

- rtl模块：
	- 提供基础的面向对象实现。
- dbg模块：
	- 扩展的traceback函数，可以打印每一调用层级的变量值。
	- 打印Lua对象为字符串形式。
	- 高精度时钟
- list对象：
	- 数组对象，接口类似于table模块；
	- 提供更快的插入和删除操作
	- 提供更快的排序功能。
	- 禁止将元素值设置为nil。
- queue对象：
	- 提供比table更快的入队和出队操作。
- str模块：
	- 一些常用的字符串操作函数，参考了Python的字符串接口。
- LruCache对象：
	- 基于循环链表的Lru Cache实现。
- seri模块：
	- 参考自云风的序列化库
	- 代码重写编写，格式上有一点微小的变化。
- randset对象：
	- 可随机的集合对象
- weightrand模块：
	- 一个极速的基于权重的随机选择算法
- MHTimer对象：
    - 一个基于最小堆的定时器实现
- bitarray对象：
	- 位数组
- filesys模块
	- 常用的文件和目录接口
- oset有序集合
	- 基于skiplist实现
- rand对象
	- 基于xoshiro256**的伪随机数生成器(PRNG)

## 编译指南

### 1. 使用xmake编译

先[安装xmake](https://xmake.io/#/zh-cn/getting_started)；然后在根目录下执行：

```sh
xmake
```

如果提示找不到lua.h文件，则需要配置lua的头文件路径，在Windows下：

```sh
xmake f --includedirs=<luaincdir> --linkdirs=<lualibdir> --links=<lualib>
# <luaincdir> 替换为Lua代码文件所在的路径，比如：e:\lua\lua-5.4.2\src
# <lualibdir> 替换为编译出来的Lua库文件所在的路径，比如：e:\lua\lua-5.4.2\build\windows\x64\release
# <lualib> 替换为Lua库文件，比如：lua；实际上找的是lua.lib
```

在其他系统下：

```sh
xmake f --includedirs=<luaincdir>
# <luaincdir> 替换为Lua代码文件所在的路径，比如/home/colin/lua-5.4.2/src
```

上面命令只需要执行一次，以后就不用再执行。接下来仍然执行`xmake`来编译工程，看看能否成功。

### 2. 使用make编译

make只支持在非Window系统下编译，同样在根目录下执行(在freebsd下用gmake代替make)：

```sh
make
```

如果提示找不到lua.h文件，同样需要指定Lua的头文件路径：

```sh
make "INC=-I<luaincdir>"
# <luaincdir> 替换为Lua代码文件所在的路径，比如/home/colin/lua-5.4.2/src
```

如无意外应该可以编译成功，最后生成colibc.so在colib子目录中。

### 3. 使用xmake在Windows下编译Lua

根目录下有一个`lua-xmake.lua`文件，将其拷贝到Lua目录下，比如`/home/colin/lua-5.4.2`，然后改名为`xmake.lua`。

接着执行：

```sh
xmake
```

如果成功会在`build\windows\x64\release`生成下面文件：

```sh
lua-static.lib		# 静态库
lua.exe				# 执行程序
lua.dll				# 动态库
luac.exe			# 编译程序
lua.lib				# 导入库
```


## 使用该库的注意点

- apis.lua 和其他lua文件都有详尽的接口说明，建议使用 IDEA + EmmyLua 插件，以获得方便的代码提示。
- test 目录中是各模块的测试代码，查看这些代码可以了解各模块的使用方法。进入test, 然后执行`lua test_xxx`。
- 由于本人的精力有限，该库只支持 Lua5.3 以上的版本。

## 系统兼容性

该库只在下面的系统编译通过，其他系统不能保证可以编译

- Windows 10
- macOS Big Sur 11.2.3
- Debian GNU/Linux 10 (buster)
- Ubuntu 20.04.1 LTS
- FreeBSD 12
- CentOS 8

## 待开发的模块