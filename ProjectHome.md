[Welcome to use TSTDB V2](http://code.google.com/p/tstdb/wiki/TSTDBV2)

**tstdb is based on a data structure called "Ternary Search Tree",and it is compatible with memcached. Moreover, it supports prefix searching and range searching.**


[what's "Ternary Search Tree"](http://en.wikipedia.org/wiki/Ternary_search_tree)

# Feature #
  1. memcached protocol compatible (get/set/delete/incr/decr/cas/gets)
  1. no third-party library dependencies
  1. pipeline requests support
  1. high performance & data persistence ability
  1. [prefix searching](http://code.google.com/p/tstdb/wiki/PrefixCommand)
  1. range searching: [less](http://code.google.com/p/tstdb/wiki/LessCommand) and [greater](http://code.google.com/p/tstdb/wiki/GreaterCommand)


## Usage demo ##


This should give you a feel for how this module operates::
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

```

output will be

```

Some value
['haha/1', 'haha/2', 'haha/5']
['haha/2', 'haha/1']
['haha/2', 'haha/5', 'key', 'some_key']

```


Welcome your valuable suggestions. My microblog: http://www.weibo.com/treapdb


