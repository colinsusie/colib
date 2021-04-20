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

print(cofilesys.exists("test_bitarray.lua"))
print(cofilesys.getsize("test_weightrand.lua"))
print(cofilesys.getsize("unknown.lua"))
print(cofilesys.getmtime("test_weightrand.lua"))
print(os.date("%c", cofilesys.getmtime("test_weightrand.lua")))

print(cofilesys.getmode("test_list.py"))
print(string.format("%X", cofilesys.getmode("..")))

print("isdir ..", cofilesys.isdir(".."))
print("isdir test_list.py", cofilesys.isdir("test_list.py"))
print("isfile test_list.py", cofilesys.isfile("test_list.py"))