#!/usr/bin/env bash
BASEDIR=$(dirname $0)

if [ ! -e $BASEDIR"/conf/tstserver.conf" ]
then
	echo "[FAILED] '$BASEDIR/conf/tstserver.conf'" does not exist, please check it. 
	exit
fi

source $BASEDIR"/conf/tstserver.conf"


cd $BASEDIR

DATA_DIR=$(dirname $DATA_FILE)

if [ ! -e $DATA_DIR ]
then
	echo "[FAILED] data directory '$DATA_DIR' does not exist, please check it."
	exit
fi

tst_start(){
	cmd="$BASEDIR/bin/tstserver -f$DATA_FILE -l$IP_ADDR -p$PORT -g$LOG_FILE $NOLOG"
	$cmd >$BASEDIR"/log/stdout.log" 2>&1 & 
}

tst_debug(){
	cmd="gdb -x $BASEDIR/src/gdb.cmd --args $BASEDIR/bin/tstserver -f$DATA_FILE -l$IP_ADDR -p$PORT -g$LOG_FILE $NOLOG"
	echo $cmd
	$cmd 2>&1 
}

tst_stop(){
	pid=$(get_pid)
	while [[ $pid != '' ]] 
	do
		kill $pid
		sleep 1
		echo -ne "."
		pid=$(get_pid)
	done
}

get_pid(){
	pid=$(ps aux | grep tstserver | grep -v grep |  tr -s ' ' | cut -d ' ' -f 2)
	echo -ne $pid
}

case "$1" in
	"start"|"")
		echo "start..."	
		[ -z $(get_pid) ] || echo "please stop the running instance first."  
		[ -z $(get_pid) ] || exit
		tst_start
		echo $! > /tmp/tstserver.pid
		if [[ $(get_pid)==$(cat /tmp/tstserver.pid) ]]
		then
			echo "[OK]"
		else
			echo "[FAILED]"
		fi
	;;

	"stop")
		[ -z $(get_pid) ] && echo "no running instance exists"
		[ -z $(get_pid) ] && exit
		echo "stop..."	
		tst_stop
		if [ -z $(get_pid) ]
		then
			echo "[OK]"
		else
			echo "[FAILED]"
		fi
	;;

	"debug")
		echo "start[debug mode]..."	
		[ -z $(get_pid) ] || echo "please stop the running instance first."  
		[ -z $(get_pid) ] || exit
		tst_debug
		echo "[OK]"
	;;

	"help"|"-h"|*)
		echo "tst.sh start|stop|help"
	;;
esac



