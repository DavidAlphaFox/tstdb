Introduction to tstdb v2.
= Feature =
 # memcached protocol compatible (get/set/delete/incr/decr/cas/gets)
 # no third-party library dependencies 
 # pipeline requests support
 # high performance & data persistence ability
 # [http://code.google.com/p/tstdb/wiki/PrefixCommand prefix searching]
 # range searching: [http://code.google.com/p/tstdb/wiki/LessCommand less] and [http://code.google.com/p/tstdb/wiki/GreaterCommand greater]

= Build =

== check out source ==

 * svn co http://tstdb.googlecode.com/svn/trunk/v2/ tstdb
 * or go to [http://code.google.com/p/tstdb/downloads/list] to download the source package, and then unpack it with 'tar -xzf'.

== compile ==

 * cd src
 * make

= Start Service=
 * ./tst.sh start
 * tail -f log/`*`   (check the log)
 * default listening port: 8402 (can be changed in conf/tstserver.conf)

= Stop Service=
 * ./tst.sh stop

= Configuration =
 * vim conf/tstserver.conf
 * when conf file changed, run './tst.sh stop' and then './tst.sh start'

= Client =
 * tstdb python client:[http://code.google.com/p/tstdb/source/browse/trunk/v2/src/client/pytst.py]
 * any other memcached client library can be used as a tstdb client(except 'prefix' operation)
 * some example: [http://code.google.com/p/tstdb/source/browse/#svn%2Ftrunk%2Fv2%2Fsrc%2Ftest%2Foneshot%253Fstate%253Dclosed python client]