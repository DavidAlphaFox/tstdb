# Introduction #

"less" is a new command, which does not exist in memcached


# Details #
request:

```
less <key> <limit>\r\n
```

response:
```
KEYS <result amount> <total response length in bytes>\r\n
...
...
END\r\n
```

# Example #
```
telnet localhost 8402
...

less thing123 10
KEYS 10 127
thing123
thing122999
thing122998
thing122997
thing122996
thing122995
thing122994
thing122993
thing122992
thing122991
END

```