#!/usr/bin/env sh
if (( $#<2 ))
then
        echo "usage:"
        echo "./yali.sh program_name number_of_proc"
        exit
fi

prog_name=$1
num_proc=$2
pid_file="/tmp/pids.yali"

#clear
>$pid_file

trap "cat $pid_file | xargs kill -9 " EXIT
for ((i=0;i<$num_proc;i++))
do
        $prog_name 2>&1 &
        echo $! >> $pid_file
done
wait
