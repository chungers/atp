#!/bin/bash

function showHelp {
    echo "$0 <options>"
    echo " -d <dir> : runtime directory."
    echo " -v <display> : vncserver display number"
    exit 0;
}

INSTALL_DIR=$(dirname $0)/../third_party/interactivebrokers/ibg
RUNTIME_DIR=$(pwd)
DISPLAY_NUM=1
IBG_DIR=$INSTALL_DIR/IBJts

while getopts "d:v:" optionName; do
case "$optionName" in
d) RUNTIME_DIR="$OPTARG";;
v) DISPLAY_NUM="$OPTARG";;
[?]) showHelp;;
esac
done

if [[ $(which vncserver) != "" ]]; then
    if [[ $DISPLAY_NUM != "" ]]; then
	echo "Start vncserver.";
	vncserver -geometry 800x600 -depth 16 :${DISPLAY_NUM}
	export DISPLAY=localhost:${DISPLAY_NUM}
    fi
fi

echo "Starting IBG with DISPLAY= ${DISPLAY} and RUNTIME_DIR=${RUNTIME_DIR}"
CLASSPATH=$IBG_DIR/jts.jar:$IBG_DIR/hsqldb.jar:$IBG_DIR/jcommon-1.0.12.jar:$IBG_DIR/jhall.jar:$IBG_DIR/other.jar:$IBG_DIR/rss.jar
nohup java -cp $CLASSPATH  -Dsun.java2d.noddraw=true -Xmx512M ibgateway.GWClient $RUNTIME_DIR &>$RUNTIME_DIR/ibg.log &

echo "IBG started."