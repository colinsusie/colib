package.cpath = package.cpath .. ";../colib/?.so"
package.path = package.path ..";../colib/?.lua"

local bitarray = require "bitarray"

local a = bitarray.new(2, 64)

print("len", #a)


local t = {}
for i, v in ipairs(a) do
	t[i] = tostring(v)
end
print("array: ", #t, table.concat(t, ","))

t = bitarray.tointegers(a)
print("tointegers: ", table.concat(t, ","))

for i = 1, 40 do
	a[i] = true
end
t = bitarray.tointegers(a)
print("tointegers: ", table.concat(t, ","))

print("concat", #a, bitarray.concat(a, ","))

bitarray.fromintegers(a, {1023})
print("fromintegers: ", table.concat(bitarray.tointegers(a), ","))
bitarray.fromintegers(a, {1023, 511, 511})
print("fromintegers: ", table.concat(bitarray.tointegers(a), ","))
bitarray.frombooleans(a, {true, true, false, false})
print("frombooleans: ", table.concat(bitarray.tointegers(a), ","))

bitarray.clear(a)
print("clear: ", table.concat(bitarray.tointegers(a), ","))

bitarray.resize(a, 2)
a[1] = true
bitarray.exchange(a, 1, 32)
print("exchange", a[1], a[32])
print("exchange: ", table.concat(bitarray.tointegers(a), ","))

bitarray.set(a, 49, true)
print("set: ", table.concat(bitarray.tointegers(a), ","))
bitarray.set(a, 128, true)
print("set: ", table.concat(bitarray.tointegers(a), ","))
bitarray.set(a, 1, true)
print("set: ", table.concat(bitarray.tointegers(a), ","))

print(a[49], a[128], a[48])
print(a[32], a[1], a[200], a[2])
print(bitarray.wordsize(a))
print(bitarray.wordbits(a))

bitarray.resize(a, 3)
print("resize: ", table.concat(bitarray.tointegers(a), ","))

