# Introduction #

"greater" is a new command, which does not exist in memcached


# Details #
request:

```
greater <key> <limit>\r\n
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

greater thing123 10
KEYS 10 124
thing123
thing1230
thing12300
thing123000
thing123001
thing123002
thing123003
thing123004
thing123005
thing123006
END


```