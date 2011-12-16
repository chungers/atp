#!/bin/bash

function showHelp {
    echo "$0 <options>"
    echo " -d <dir> : runtime directory."
    echo " -v <display> : vncserver display number"
    exit 0;
}
START_VNC=0
INSTALL_DIR=$(dirname $0)/../third_party/interactivebrokers/ibg
RUNTIME_DIR=$(pwd)
DISPLAY_NUM=1
IBG_DIR=$INSTALL_DIR/IBJts

while getopts "d:v:x" optionName; do
case "$optionName" in
x) START_VNC=1;;
d) RUNTIME_DIR="$OPTARG";;
v) DISPLAY_NUM="$OPTARG";;
[?]) showHelp;;
esac
done

if [[ $(which vncserver) != "" ]]; then
    if [[ $DISPLAY_NUM != "" ]]; then
        if [[ $START_VNC == "1" ]]; then
	    echo "Start vncserver.";
            vncserver -geometry 800x600 -depth 16 :${DISPLAY_NUM}
	fi
        export DISPLAY=localhost:${DISPLAY_NUM}
    fi
fi

VNC_PORT=$(echo "5900 + ${DISPLAY_NUM}" | bc)
LOG=$RUNTIME_DIR/ibg-${VNC_PORT}.log
PID_FILE=$RUNTIME_DIR/ibg-${VNC_PORT}.pid

echo "Starting IBG with DISPLAY= ${DISPLAY} and RUNTIME_DIR=${RUNTIME_DIR}, logfile = $LOG"
CLASSPATH=$IBG_DIR/jts.jar:$IBG_DIR/hsqldb.jar:$IBG_DIR/jcommon-1.0.12.jar:$IBG_DIR/jhall.jar:$IBG_DIR/other.jar:$IBG_DIR/rss.jar
nohup java -cp $CLASSPATH  -Dsun.java2d.noddraw=true -Xmx512M ibgateway.GWClient $RUNTIME_DIR &>$LOG &

PID=$!
echo $PID > $PID_FILE

echo "IBG started: $PID"
