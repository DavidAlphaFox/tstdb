import time
import string
import random
time.time()

start = time.time()

import cmemcached as memcache
mc = memcache.Client(['localhost:8402'], debug=0)

data = 'x'*100

count = 1000000

for i in xrange(count+1):
        key = "thing" + str(random.randint(0,count))
        rc = mc.set(key,data)
        if rc==0:
            mc = memcache.Client(['localhost:8402'], debug=0) #re connect
            print 'reconn'
        if i%10000==0:
            print 'inserting',key,i

end = time.time()

elapsed = end - start

print 'cost',str(elapsed)
print (count+0.0)/elapsed,' per second'
