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


function processLdd {
    target=$1
    deps_dir=$2

    echo "Getting dependencies of $target"

    platform=$(uname)
    case "$platform" in
    Darwin) deps=$(otool -L $target | cut -f1 -d' ' | grep ".dylib");;
    Linux) deps=$(ldd $target | cut -f3 -d' ' | grep ".so");;
    esac

    #copy each shared library over to deps_dir
    for i in $deps; do
        if [ ! -f $i ]; then
            i=$(find /usr/local/lib /usr/lib -name $i)
        fi
        if [ -f $i ]; then
            echo "Copying $i to $deps_dir"
            cp -f $i $deps_dir
        fi
    done
}

# main
DIR=$(dirname $0)/..
cd $DIR
DIR=$(pwd)
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

echo "Running from $DIR"

CLEAN=0
BUILD=0
TARGET=""
DEPLOY_HUB="$DIR/../hub"
DEPLOY_HUB_ATP="$DEPLOY_HUB/atp"
DEPLOY=0
MESSAGE=""
DO_COMMIT=0

while getopts "t:m:cad" optionName; do
case "$optionName" in
c) CLEAN=1;;
t) TARGET="$OPTARG"; BUILD=1;;
a) BUILD=1;;
d) DEPLOY=1;;
m) MESSAGE="$OPTARG";DO_COMMIT=1;;
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
DEPLOY_OPS=$DEPLOY_HUB_ATP/ops
DEPLOY_R_PKGS=$DEPLOY_TARGET/R-packages
# Deploy to the hub
if [ $DEPLOY == 1 ]; then
    pushd $DEPLOY_HUB/
    git pull
    popd
    # run full installation to build/
    rm -rf $DIR/install/
    make install
    mkdir -p $DEPLOY_TARGET
    mkdir -p $DEPLOY_OPS
    mkdir -p $DEPLOY_R_PKGS
    mkdir -p $DEPLOY_TARGET/deps
    cp -r -f $DIR/install/* $DEPLOY_TARGET
    cp -r -f $DIR/ops/munin $DEPLOY_OPS
    cp -r -f $(find $DIR/R-packages -name *.tgz) $DEPLOY_R_PKGS
    cp -r -f $(find $DIR/R-packages -name *.gz) $DEPLOY_R_PKGS
    # determine the shared library dependencies
    l=$(ls $DEPLOY_TARGET/bin)
    for i in $l; do
        processLdd $DEPLOY_TARGET/bin/$i $DEPLOY_TARGET/deps
    done

    if [ $DO_COMMIT == 1 ]; then
        pushd $DEPLOY_HUB/
        git add -v atp/
        git status
        echo "Committing with message: $MESSAGE"
        git commit -m "Build / release: $MESSAGE" -a
        git push
        popd
    fi
fi

popd


