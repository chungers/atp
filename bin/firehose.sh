#!/bin/bash

function showHelp {
    echo "$0 <options>"
    echo " -r : request market data"
    echo " -c : cancel market data"
    exit 0;
}

R_DIR=$(dirname $0)/../R

REQUEST_MKTDATA=0
CANCEL_MKTDATA=0

while getopts "rc" optionName; do
case "$optionName" in
r) REQUEST_MKTDATA=1;;
c) CANCEL_MKTDATA=1;;
[?]) showHelp;;
esac
done

if [ $REQUEST_MKTDATA == 1 ]; then
    cd $R_DIR;
    R --vanilla --slave < firehose_start.R
fi

if [ $CANCEL_MKTDATA == 1 ]; then
    cd $R_DIR;
    R --vanilla --slave < firehose_stop.R
fi
