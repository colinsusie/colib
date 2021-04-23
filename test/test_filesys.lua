-- 先这样判断是不是Windows
if package.config:sub(1, 1) == "\\" then
	package.cpath = package.cpath .. ";../colib/?.dll"
else
	package.cpath = package.cpath .. ";../colib/?.so"
end
package.path = package.path ..";../colib/?.lua"


local cofilesys = require "filesys"

print("scandir-----------------------")
for name, err in cofilesys.scandir(".") do
	print(name, err)
end

print("listdir-----------------------")
print(table.concat(cofilesys.listdir(".."), ", "))

print("exists", cofilesys.exists("test_bitarray.lua"))
print("getsize", cofilesys.getsize("test_weightrand.lua"))
print("getsize", cofilesys.getsize("unknown.lua"))
print("getmtime", cofilesys.getmtime("test_weightrand.lua"))
print(os.date("%c", math.floor(cofilesys.getmtime("test_weightrand.lua"))))
print("getatime", cofilesys.getatime("test_weightrand.lua"))
print("getctime", cofilesys.getctime("test_weightrand.lua"))

print(cofilesys.getmode("test_list.py"))
print(string.format("%X", cofilesys.getmode("..")))

print("isdir ..", cofilesys.isdir(".."))
print("isdir test_list.py", cofilesys.isdir("test_list.py"))
print("isfile test_list.py", cofilesys.isfile("test_list.py"))

-- print("mkdir", cofilesys.mkdir("testdir"))
-- print("rmdir", cofilesys.rmdir("testdir"))
-- print("rmdir", cofilesys.rmdir("testdir2"))

-- print("mkdir", cofilesys.mkdir("testdir"))
-- print("mkdir", cofilesys.mkdir("testdir/testdir"))
-- print("rmdir", cofilesys.rmdir("testdir"))

print("split", cofilesys.splitext("/home/colin/tsconfig.json"))
print("split", cofilesys.splitext("/home/colin/tsconfig..ext"))
print("split", cofilesys.splitext("/home/colin/tsconfig"))
print("split", cofilesys.splitext("."))

print("chgext", cofilesys.chgext("test.lua", ".luac"))
print("chgext", cofilesys.chgext("test", ".luac"))


 print("split", cofilesys.split("aaabbb/abc.a"))
 print("split", cofilesys.split("aaabbb///abc.a"))
 print("split", cofilesys.split("/abc.a"))

print("getcwd", cofilesys.getcwd())
print("chdir", cofilesys.chdir(".."))
print("getcwd", cofilesys.getcwd())

print("join", cofilesys.join("aaa", "bbb", "ccc.txt"))
print("join", cofilesys.join("aaa", "/bbb", "/ccc.txt"))
print("join", cofilesys.join("aaa/", "bbb/", "ccc.txt"))

print("isabs", cofilesys.isabs("/aa/bb"))
print("isabs", cofilesys.isabs("aa/bb"))

print("abspath", cofilesys.abspath("bbb.txt"))
print("abspath", cofilesys.abspath("bbb/ccc.txt"))
print("abspath", cofilesys.abspath("/aaa/bbb/ccc.txt"))

print("normpath", cofilesys.normpath("/aa/./bb//cc/../dd.txt"))
print("normpath", cofilesys.normpath("../../aa.txt"))
print("normpath", cofilesys.normpath("/../../aa.txt"))
print("normpath", cofilesys.normpath("/aa/../bb.txt"))
print("normpath", cofilesys.normpath("../../bb/../cc"))


print("splitdrive", cofilesys.splitdrive("C:\\aa\\bb\\cc"))
print("splitdrive", cofilesys.splitdrive("\\\\aa\\bb\\cc"))
print("isabs", cofilesys.isabs("C:\\aa\\b\\c"))
print("isabs", cofilesys.isabs("C:/aa\\b\\c"))
print("isabs", cofilesys.isabs("aa\\b\\c"))
print("split", cofilesys.split("C:\\aa\\bb\\cc"))
print("split", cofilesys.split("C:\\a\\\\/cc"))
print("normpath", cofilesys.normpath("C:\\a\\b/c"))
print("normpath", cofilesys.normpath("C:\\a/../b/c"))
print("normpath", cofilesys.normpath("C:\\../../b/c"))
print("normpath", cofilesys.normpath("../../b/c"))
print("normpath", cofilesys.normpath("../../b/../c"))
print("join", cofilesys.join("aa\\bb\\cc\\", "dd", "ee"))