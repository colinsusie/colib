-- 先这样判断是不是Windows
if package.config:sub(1, 1) == "\\" then
	package.cpath = package.cpath .. ";../colib/?.dll"
else
	package.cpath = package.cpath .. ";../colib/?.so"
end
package.path = package.path ..";../colib/?.lua"
local codec = require "codec"

do 	-- base64
	local ds = "S"
	local es = codec.b64encode(ds)
	print("b64encode", ds, es)
	ds = codec.b64decode(es)
	print("b64decode", es, ds)

	ds = "Su"
	es = codec.b64encode(ds)
	print("b64encode", ds, es)
	ds = codec.b64decode(es)
	print("b64decode", es, ds)

	ds = "Suv"
	es = codec.b64encode(ds)
	print("b64encode", ds, es)
	ds = codec.b64decode(es)
	print("b64decode", es, ds)

	ds = ""
	es = codec.b64encode(ds)
	print("b64encode", ds, es)
	ds = codec.b64decode(es)
	print("b64decode", es, ds)

	ds = "This software"
	es = codec.b64encode(ds)
	print("b64encode", ds, es)
	ds = codec.b64decode(es)
	print("b64decode", es, ds)

	ds = "This software"
	es = codec.b64encode(ds)
	print("b64encode", ds, es)
	ds = codec.b64decode(es.."!!==")
	print("b64decode", es, ds)

	ds = "Thissoftware"
	es = codec.b64encode(ds)
	print("b64encode", ds, es)
	es = string.sub(es, 1, 8).."==="
	ds = codec.b64decode(es)
	print("b64decode", es, ds)
end