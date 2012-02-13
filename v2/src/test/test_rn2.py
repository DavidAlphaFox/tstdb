import cmemcached as memcached
import os
import time
basedir="/home/sunjoy/test/tstdb/"
os.system(basedir+"/tst.sh start")

time.sleep(2)
print 'begin set'
mc=memcached.Client(['localhost:8402'])
for i in xrange(1,10001):
	mc.set('thing'+str(i),'\r\nasdf\rsf\nsfsfd'*i)
print 'set over'

time.sleep(2)
print 'stop tstserver'
os.system(basedir+"/tst.sh stop")
time.sleep(2)
result=os.popen("grep header '../../log/stdout.log' | awk -F' ' '{print $5}'").read().split("\n")
#print result

for i in xrange(1,10001):
	#print i
	assert(int(result[i-1]),i)
print result[:10]
		


