import pytst
tst = pytst.TSTClient(host='localhost',port=8402)

tst.set("some_key", "Some value")
value = tst.get("some_key")
print value

tst.set("another_key", 3)
tst.delete("another_key")

tst.set("key", "1")   # note that the key used for incr/decr must be a string.
tst.incr("key")
tst.decr("key")

tst.set('haha/1',123)
tst.set('haha/2',456)
tst.set('haha/5','xyz')
print tst.prefix('haha')

