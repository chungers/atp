#!/bin/bash

DIR=$(dirname $0)

pushd $DIR
TARGETS=$(find src test -name 'CMakeLists.txt' -exec grep 'cpp_gtest' '{}' \; | sed -e 's/cpp_gtest(//g' -e 's/)//g')

echo "Clean up working directory and start a new build."

make clean
cmake .

# Build all the tests first.
for t in $TARGETS; do
	echo "Building $t"
	make $t	
done

# Run the tests.
for t in $TARGETS; do
	echo "Running $t"
	build/$t --logtostderr --v=40		
done

popd

