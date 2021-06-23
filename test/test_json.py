import json
import time


print("load============================================================")
def doload(file, count):
	with open(file, "rb") as f:
		str = f.read()
		ti = time.time()
		for i in range(count):
			json.loads(str)
		print("{}, seconds: {}".format(file, time.time() - ti))

doload("./test_float.json", 30)
doload("./test_int.json", 30)
doload("./test_string.json", 90)
doload("./test_string2.json", 50)
doload("./test_word.json", 200)
doload("./twitter.json", 60)
doload("./citm_catalog.json", 30)
doload("./player.json", 200)


print("dump============================================================")
def dodump(file, count):
	with open(file, "rb") as f:
		obj = json.loads(f.read())
		ti = time.time()
		for i in range(count):
			json.dumps(obj)
		print("{}, seconds: {}".format(file, time.time() - ti))

dodump("./test_float.json", 30)
dodump("./test_int.json", 30)
dodump("./test_string.json", 90)
dodump("./test_string2.json", 50)
dodump("./test_word.json", 200)
dodump("./twitter.json", 60)
dodump("./citm_catalog.json", 30)
dodump("./player.json", 200)


