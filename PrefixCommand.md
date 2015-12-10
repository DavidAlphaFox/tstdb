# Introduction #

"prefix" is a new command, which does not exist in memcached


# Details #
request:

```
prefix <key prefix> <limit> <sorting order>\r\n
```

response:
```
KEYS <result amount> <total len in bytes>\r\n
...
...
END\r\n
```

# Example #
```
telnet localhost 8402
...

prefix haha 10 desc
KEYS 3 24
haha/5
haha/2
haha/1
END


```