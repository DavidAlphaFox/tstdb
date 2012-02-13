import cmemcached as memcached
import os
import time
basedir="/home/sunjoy/test/tstdb/"
os.system(basedir+"/tst.sh start")

time.sleep(2)
print 'begin set'
mc=memcached.Client(['localhost:8402'])
for i in xrange(1,10001):
	mc.set('thing'+str(i),'x'*i)
print 'set over'

time.sleep(2)
print 'stop tstserver'
os.system(basedir+"/tst.sh stop")
time.sleep(2)
result=os.popen("grep body '../../log/stdout.log' | awk -F: '{print length($2)}'").read().split("\n")
#print result

for i in xrange(1,10001):
	#print i
	assert(int(result[i-1]),i)
print result[:10]
		


