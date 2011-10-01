#!/bin/bash

function showHelp {
    echo "$0 <options>"
    echo " -b : build (default = off)"
    echo " -r : run (default = off)"
    echo " -c : clean (default = off)"
    echo " -q : quiet (default = off)"
    echo " -t <test> : for specified test."
    echo " -v level : log level (default = 40)"
    exit 0;
}

# main
CLEAN=0
RUN=0
BUILD=0
TEST=""
VLEVEL=40
LOG="--logtostderr"

while getopts "t:v:cbrq" optionName; do
case "$optionName" in
t) TEST="$OPTARG";;
v) VLEVEL="$OPTARG";;
c) CLEAN=1;;
b) BUILD=1;;
r) RUN=1;;
q) LOG="";;
[?]) showHelp;;
esac
done

DIR=$(dirname $0)

pushd $DIR

if [[ $TEST != "" ]]; then
    TARGETS=$TEST
else
    TARGETS=$(find src test -name 'CMakeLists.txt' \
-exec grep '^cpp_gtest' '{}' \; | sed -e 's/cpp_gtest(//g' -e 's/)//g')
fi

echo "Targets are:"
for t in $TARGETS; do
        echo "$t"
done

if [ $CLEAN == 1 ]; then
    echo "Clean up working directory and start a new build."
    make clean
    cmake .
fi


if [ $BUILD == 1 ]; then
    for t in $TARGETS; do
        echo "****************************************************************"
        echo "BUILDING $t"
        make $t
        if [ $? != 0 ]; then
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!  Build failed. Exiting.";
            exit -1;
        fi;
    done
fi

if [ $RUN == 1 ]; then
    for t in $TARGETS; do
        echo "****************************************************************"
        echo "RUNNING $t"
        ./build/$t $LOG --v=$VLEVEL
    done
fi

popd


