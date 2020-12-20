import time

# xrange = range

def test1():
	c = 1000000
	a = []
	tm = time.time()
	for i in xrange(c):
		a.append(i)
	print "python-append(1000000)",  time.time() - tm

def test2():
	c = 100000
	a = []
	tm = time.time()
	for i in xrange(c):
		a.insert(0, i)
	print "python-insert(100000)",  time.time() - tm

def test3():
	c = 1000000
	a = []
	for i in xrange(c):
		a.append(i)

	tm = time.time()
	for i in xrange(c):
		a.pop()
	print "python-pop-after(1000000)",  time.time() - tm

def test4():
	c = 100000
	a = []
	for i in xrange(c):
		a.append(i)

	tm = time.time()
	for i in xrange(c):
		a.pop(0)
	print "python-pop-before(100000)",  time.time() - tm

def test5():
	c = 1000000
	a = []
	for i in xrange(c):
		a.append(i)

	tm = time.time()
	for i in xrange(c):
		a[i] = a[i] - a[i]
	print "python-get/set(1000000)",  time.time() - tm


seed = 123456789
def srandom():
	global seed
	seed = (1103515245 * seed + 12345) % 65536
	return seed

def test6():
	global seed
	seed = 123456789
	c = 1000000
	a = []
	for i in xrange(c):
		a.append(srandom())

	def comp(a, b):
		return a - b
	tm = time.time()
	# a.sort(cmp=comp)
	a.sort(key=lambda a: a)
	print "python-sort(1000000)",  time.time() - tm

test1()
test2()
test3()
test4()
test5()
test6()
