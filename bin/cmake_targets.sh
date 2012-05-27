#!/bin/bash

# main
DIR=$(dirname $0)/..
cd $DIR
DIR=$(pwd)

echo "Running from $DIR"

pushd $DIR

EXECUTABLES=$(find src test R-packages -name 'CMakeLists.txt' \
-exec grep '^cpp_executable' '{}' \; | \
sed -e 's/cpp_executable(//g' -e 's/)//g' | sort)

LIBS=$(find src test R-packages -name 'CMakeLists.txt' \
-exec grep '^cpp_library' '{}' \; | \
sed -e 's/cpp_library(//g' -e 's/)//g' | sort)

CUSTOM=$(find src test R-packages -name 'CMakeLists.txt' \
-exec grep '^add_custom_target' '{}' \; | \
sed -e 's/add_custom_target(//g' -e 's/)//g' | sort)

echo "LIBS -----------------------------------"
for t in $LIBS; do
        echo "        $t"
done

echo "EXECUTABLES ----------------------------"
for t in $EXECUTABLES; do
        echo "        $t"
done

echo "CUSTOM ---------------------------------"
for t in $CUSTOM; do
        echo "        $t"
done


popd


