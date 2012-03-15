#!/bin/bash

function showHelp {
    echo "$0 <options>"
    echo " -c : clean (default = off)"
    echo " -a : build all (default = off)"
    echo " -t <target> : for target"
    echo " -d : for deployment to directory (hub/atp)"
    echo " -m <message> : commit message for release."
    exit 0;
}

# main
DIR=$(dirname $0)/..

CLEAN=0
BUILD=0
TARGET=""
DEPLOY_HUB="$DIR/../hub"
DEPLOY_HUB_ATP="$DEPLOY_HUB/atp"
DEPLOY=0
MESSAGE=""

while getopts "t:m:cad" optionName; do
case "$optionName" in
c) CLEAN=1;;
t) TARGET="$OPTARG"; BUILD=1;;
a) BUILD=1;;
d) DEPLOY=1;;
m) MESSAGE="$OPTARG";;
[?]) showHelp;;
esac
done

pushd $DIR

cmake .

if [[ $TARGET != "" ]]; then
    TARGETS=$TARGET
else
    TARGETS=$(find src test -name 'CMakeLists.txt' \
-exec grep '^cpp_executable' '{}' \; | sed -e 's/cpp_executable(//g' -e 's/)//g' | sort)
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

DEPLOY_TARGET=$DEPLOY_HUB_ATP/$(uname)-$(arch)
# Deploy to the hub
if [ $DEPLOY == 1 ]; then
    pushd $DEPLOY_HUB/
    git pull
    popd
    # run full installation to build/
    make install
    mkdir -p $DEPLOY_TARGET
    cp -r $DIR/install/ $DEPLOY_TARGET
    git add -v atp/
    git status
    echo "Committing with message: $MESSAGE"
    git commit -m "Build / release: $MESSAGE" -a
    git push
fi

popd


