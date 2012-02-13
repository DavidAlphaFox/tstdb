import cmemcached as memcached
import os
import time
basedir="/home/sunjoy/test/tstdb/"
os.system(basedir+"/tst.sh start")

time.sleep(2)
print 'begin set'
mc=memcached.Client(['localhost:8402'])

print mc.set("abc","123\r\n")
print mc.set("xxxx","\r\n456")

print 'set over'
time.sleep(2)
print 'stop tstserver'
os.system(basedir+"/tst.sh stop")
time.sleep(2)
result=os.popen("cat '../../log/stdout.log'").read()
		


