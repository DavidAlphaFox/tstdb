这个版本的主要改进包括：

  1. 加入了prefix search和range query的支持
  1. 更好低兼容memcached，加入了incr/derc/gets/cas操作
  1. 改进了服务器的buffer管理，更加稳定
  1. 改进了数据持久化机制，除了日志重放外还加上了磁盘镜像reload功能，使得服务重启时数据加载更快
  1. 支持pipeline请求，支持noreply的异步set操作

目前，您可以使用任意的memcached客户端来访问tstdb。
如果您需要使用prefix search等功能的话，目前只有一个python客户端pytst可用。不过，很容易扩展到其他语言。

安装说明：http://code.google.com/p/tstdb/wiki/TSTDBV2

Usage demo

```
import pytst  
tst = pytst.TSTClient(host='localhost',port=8402)  
  
tst.set("some_key", "Some value")  
value = tst.get("some_key")  
print value  
  
tst.set("another_key", 3)  
tst.delete("another_key")  
  
tst.set("key", "1")   
tst.incr("key")  
tst.decr("key")  
  
tst.set('haha/1',123)  
tst.set('haha/2',456)  
tst.set('haha/5','xyz')  
print tst.prefix('haha')  
print tst.less('haha/2')  
print tst.greater('haha/2')  
output will be  
  
Some value  
['haha/1', 'haha/2', 'haha/5']  
['haha/2', 'haha/1']  
['haha/2', 'haha/5', 'key', 'some_key']  

```