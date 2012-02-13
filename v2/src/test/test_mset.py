import cmemcached as memcached
import os
import time

basedir="/home/sunjoy/test/tstdb/"
os.system(basedir+"/tst.sh start")

time.sleep(2)
print 'begin set'

mc=memcached.Client(['localhost:8402'])
d={}
for i in xrange(1000):
	d['thing'+str(i)]='xxxxx'+str(i)
print mc.set_multi(d)
print mc.set('fxsjy','sunjoy')

print 'set over'

time.sleep(2)
print 'stop tstserver'
os.system(basedir+"/tst.sh stop")
time.sleep(2)

result=os.popen("wc -l '../../log/stdout.log' | cut -d ' ' -f 1").read()
print result
assert(int(result)==2004)


