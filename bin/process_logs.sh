#!/bin/bash

LOGDIR=~david/runtime/cl-logs/*.INFO.201*
PATH=~david/github/lab616/atp/build:$PATH
PROCESS_DIR=~david/runtime/processed

pushd $PROCESS_DIR
for i in $(ls $LOGDIR); do 
    fn=$(echo $i | awk -F . '{print $7}' | awk -F - '{print $1}'); 
    if [ -e "$fn.csv" ]; then 
	echo "Skip $fn"; 
    else 
	echo "Process $fn";
	time LogReader --logfile=$i > $fn.csv
    fi; 
done
popd;
