package.cpath = package.cpath .. ";../colib/?.so"
package.path = package.path ..";../colib/?.lua"

local coqueue = require "queue"

local q = coqueue.new()
print(q)

local function printq(q)
	print("printq===========")
	for i = 1, #q do
		print(q[i])
	end
end

coqueue.push(q, 1)
coqueue.push(q, true)
coqueue.push(q, nil)
coqueue.push(q, "foo")
coqueue.push(q, {x=1, y=2})
for i = 1, 20 do
	coqueue.push(q, i)
end
print("size:============")
print(#q)

for i = 1, 24 do
	coqueue.pop(q)
end
printq(q)

coqueue.push(q, 1)
coqueue.push(q, "hello")
coqueue.push(q, 100)
coqueue.push(q, "foo")
printq(q)

local t = coqueue.totable(q)
print("totable: ===============")
print(table.concat(t, ", "))

for i = 1, 3 do
	coqueue.pop(q)
end
printq(q)


for i = 1, 5 do
	coqueue.pop(q)
end
printq(q)
print("size: ", #q)

print("benchmark:==============")
local q = coqueue.new()
local t = {}
for i = 1, 10000 do
	coqueue.push(q, i)
	t[i] = i
end

local c = os.clock()
local remove = table.remove
local insert = table.insert
for i = 1, 100000 do
	remove(t, 1)
	insert(t, i)
end
print("table: ", os.clock() - c)

c = os.clock()
local pop = coqueue.pop
local push = coqueue.push
for i = 1, 100000 do
	pop(q)
	push(q, i)
end
print("queue: ", os.clock() - c)

