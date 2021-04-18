-- 先这样判断是不是Windows
if package.config:sub(1, 1) == "\\" then
	package.cpath = package.cpath .. ";../colib/?.dll"
else
	package.cpath = package.cpath .. ";../colib/?.so"
end
package.path = package.path ..";../colib/?.lua"

local costr = require "str"

print("startswith-----------------------")
print(costr.startswith("abc", "a"))
print(costr.startswith("abc", ""))
print(costr.startswith("abc", "abcd"))
print(costr.startswith("abc", "ab", 1))
print(costr.startswith("abc", "ab", 2))
print(costr.startswith("abc", "ab", 0, 2))
print(costr.startswith("abc", "ab", -3))
print(costr.startswith("abc", "ab", 3, 6))

print("endswith-----------------------")
print(costr.endswith("abc", "c"))
print(costr.endswith("abc", ""))
print(costr.endswith("abc", "abcd", 2))
print(costr.endswith("abc", "bc", 2, 2))
print(costr.endswith("abc", "bc", 2, 3))
print(costr.endswith("abc", "bc", -2, 3))
print(costr.endswith("abc", "cbc"))

print("ljust-----------------------")
print(costr.ljust("abc", 5))
print(costr.ljust("abc", 5, '.'))
print(costr.ljust("abc", 2, '.'))
print(costr.ljust("abc", 3, '.'))
print(costr.ljust("abc", 6))

print("rjust-----------------------")
print(costr.rjust("abc", 5))
print(costr.rjust("abc", 5, '.'))
print(costr.rjust("abc", 2, '.'))
print(costr.rjust("abc", 3, '.'))
print(costr.rjust("abc", 6, "x"))

print("center-----------------------")
print(costr.center("abc", 5))
print(costr.center("abc", 5, '.'))
print(costr.center("abc", 2, '.'))
print(costr.center("abc", 3, '.'))
print(costr.center("abc", 6, "x"))
print(costr.center("abc", 7, "x"))

print("lstrip-----------------------")
print(costr.lstrip("lstrip"))
print(costr.lstrip("\t \n   lstrip"))
print(costr.lstrip("xxuueelstrip", 'xeu'))
print(costr.lstrip("", 'xeu'))

print("rstrip-----------------------")
print(costr.rstrip("rstrip"))
print(costr.rstrip("rstrip\t \n   "))
print(costr.rstrip("rstripxxuuee", 'xeu'))
print(costr.rstrip("", 'xeu'))

print("strip-----------------------")
print(costr.strip("strip"))
print(costr.strip("  \vstrip\t \n   "))
print(costr.strip("uueestripxxuuee", 'xeu'))
print(costr.strip("", 'xeu'))
print(costr.strip("xxxxuuuu", 'xeu'))

print("split-----------------------")
local t = costr.split("1|2|3", "|", true)
print(string.format("len=%s, data=%s", #t, table.concat(t, ", ")))
t = costr.split(",.1.,.2.,.3,.", ",.", true)
print(string.format("len=%s, data=%s", #t, table.concat(t, ", ")))
t = costr.split("", ",.", true)
print(string.format("len=%s, data=%s", #t, table.concat(t, ", ")))
t = costr.split("aaaa", ",", true)
print(string.format("len=%s, data=%s", #t, table.concat(t, ", ")))
t = costr.split("aaaa,", ",.", true)
print(string.format("len=%s, data=%s", #t, table.concat(t, ", ")))
t = costr.split(",,aaaa,,", ",", true)
print(string.format("len=%s, data=%s", #t, table.concat(t, ", ")))


t = costr.split("1|2|3", "|")
print(string.format("len=%s, data=%s", #t, table.concat(t, ", ")))
t = costr.split("", ",")
print(string.format("len=%s, data=%s", #t, table.concat(t, ", ")))
t = costr.split("aaaa", ",")
print(string.format("len=%s, data=%s", #t, table.concat(t, ", ")))
t = costr.split(",,aaaa,,", ",")
print(string.format("len=%s, data=%s", #t, table.concat(t, ", ")))
t = costr.split("aaaa..xx..bb", "xx")
print(string.format("len=%s, data=%s", #t, table.concat(t, ", ")))

