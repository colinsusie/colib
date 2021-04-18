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

## 编译指南

### Linux/MacOS

如果你下载过Lua的源代码，并且已经编译和安装到系统中，那么只需要在colib根目录下执行`make`即可。

否则请按下面步骤执行：

- 下载和编译Lua：

```sh
curl -R -O http://www.lua.org/ftp/lua-5.4.3.tar.gz
tar zxf lua-5.4.3.tar.gz
cd lua-5.4.3
make all test
```

Lua版本视你的需求而定，也可到[Lua官网](https://www.lua.org/download.html)查看。

- 编译colib

假设你的Lua代码目录是`/home/colin/lua-5.4.3/src`；进入colib根目录执行：

```sh
make "INC=-I/home/colin/lua-5.4.3/src"
```

如无意外，应该可以编译成功，最后colib.so在colib子目录中。

### Windows

- 安装Visual Studio

首先需要先安装[Visual Studio](https://visualstudio.microsoft.com/zh-hans/vs/)，我的安装的是`Microsoft Visual Studio Community 2019`

- 下载和编译Lua

到[Lua官网](https://www.lua.org/download.html)下载Lua代码，解压后的目录假设是：`c:\Users\colin\lua-5.4.3`。

在开始菜单的`Visual Studio`文件夹中，找到`x64 Native Tools Command Prompt for VS 2019`或`x86 Native Tools Command Prompt for VS 2019`，一个用于编译x64程序，另一个用于编译x86程序，根据你的需求打开其中一个，弹出命令行。

在命令行中定位到`c:\Users\colin\lua-5.4.3\src`，然后执行下面命令：

```bat
cl /MD /O2 /c /DLUA_BUILD_AS_DLL *.c
ren lua.obj lua._obj
ren luac.obj luac._obj
link /DLL /IMPLIB:lua.lib /OUT:lua.dll *.obj
link /OUT:lua.exe lua._obj lua.lib
lib /OUT:lua-static.lib *.obj
link /OUT:luac.exe luac._obj lua-static.lib
```

执行完毕，将在`c:\Users\colin\lua-5.4.3\src`中看到`lua.exe, luac.exe, lua.dll`这些程序文件，以及`lua.lib, lua-static.lib`这些库文件。

- 编译colib

同样运行`x64 Native Tools Command Prompt for VS 2019`或`x86 Native Tools Command Prompt for VS 2019`，定位到colib根目录。

在命令行中执行`BuildWin.bat`，该批处理要求你输入Lua src的路径，也就是上面的`c:\Users\colin\lua-5.4.3\src`，输入并回车。

如无意外，应该可以编译成功，最后colib.dll文件在colib子目录中。

## 使用该库的注意点

- apis.lua 和其他lua文件都有详尽的接口说明，建议使用 IDEA + EmmyLua 插件，以获得方便的代码提示。
- test 目录中是各模块的测试代码，查看这些代码可以了解各模块的使用方法。
- 由于本人的精力有限，该库只支持 Lua5.3 以上的版本。

## 待开发的模块

- Skiplist：跳表
- filesys： 文件系统模块：查看文件属性，判断文件类型，增加目录，删除目录，遍历目录，文件路径操作 