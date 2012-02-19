import time
import string
import random
time.time()

start = time.time()

import cmemcached as memcache
mc = memcache.Client(['localhost:8402'], debug=1)

data = 'x'*100

count = 1000000

for i in xrange(count+1):
        key = "thing" + str(i)
        rc=mc.get(key)
        #print key 
        if i%5000==0:
            print 'getting',i

end = time.time()

elapsed = end - start

print 'cost',str(elapsed)
print (count+0.0)/elapsed,' per second'
