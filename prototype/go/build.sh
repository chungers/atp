#!/bin/bash


ATP_DIR=$(pwd)/../..
PROTOTYPE_DIR=$ATP_DIR/prototype
PROTO_DIR=$PROTOTYPE_DIR/proto


export GOPATH=`pwd`:$GOPATH
echo "GOPATH=$GOPATH"

export PATH=$(pwd)/bin:$PATH
echo "PATH=$PATH"

# set up required packages
go get -u github.com/hoisie/web
go get -u code.google.com/p/goprotobuf/...

# build the protocol buffers
protoc --go_out=src --proto_path=$PROTO_DIR $PROTO_DIR/*.proto

# build the main
go build src/main/main.go

