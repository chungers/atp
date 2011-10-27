#!/bin/bash

function showHelp {
    echo "$0 <options>"
    echo " -c : clean (default = off)"
    echo " -a : build all (default = off)"
    echo " -t <target> : for target"
    exit 0;
}

# main
CLEAN=0
BUILD=0
TARGET=""

while getopts "t:v:ca" optionName; do
case "$optionName" in
c) CLEAN=1;;
t) TARGET="$OPTARG"; BUILD=1;;
a) BUILD=1;;
[?]) showHelp;;
esac
done

DIR=$(dirname $0)/..

pushd $DIR

if [[ $TARGET != "" ]]; then
    TARGETS=$TARGET
else
    TARGETS=$(find src test -name 'CMakeLists.txt' \
-exec grep '^cpp_executable' '{}' \; | sed -e 's/cpp_executable(//g' -e 's/)//g')
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
popd


