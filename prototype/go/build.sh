#!/bin/bash

export GOPATH=`pwd`:$GOPATH

echo "GOPATH=$GOPATH"

#pushd src/main
go build src/main/main.go
#popd
