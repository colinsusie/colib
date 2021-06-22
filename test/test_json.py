import json
import time


def dotest(file, count):
	with open(file, "rb") as f:
		str = f.read()
		ti = time.time()
		for i in range(count):
			json.loads(str)
		print("{}, seconds: {}".format(file, time.time() - ti))

dotest("./test_float.json", 30)
dotest("./test_int.json", 30)
dotest("./test_string.json", 90)
dotest("./test_string2.json", 50)
dotest("./test_string3.json", 50)
dotest("./test_word.json", 200)
