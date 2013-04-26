#!/bin/bash

export GOPATH=`pwd`:$GOPATH

echo "GOPATH=$GOPATH"

pushd src/borg
go install
popd

pushd src/utils
go install
popd

pushd src/main
go install
popd
