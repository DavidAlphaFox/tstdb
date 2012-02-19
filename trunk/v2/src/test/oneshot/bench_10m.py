import time
import string
import random
time.time()

start = time.time()

import cmemcached as memcache
mc = memcache.Client(['localhost:8402'], debug=0)

data = 'x'*100

count = 10000000

d={}
for i in xrange(count+1):
		key = "thing" + str(i)
		d[key]=data
		if i%100==0:
				rc = mc.set_multi(d)
				if rc==0:
					print rc
				d={}
		if i%10000==0:
			print 'inserting',i

end = time.time()

elapsed = end - start

print 'cost',str(elapsed)
print (count+0.0)/elapsed,' per second'
