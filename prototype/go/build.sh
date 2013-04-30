#!/bin/bash


ATP_DIR=$(pwd)/../..
PROTOTYPE_DIR=$ATP_DIR/prototype
PROTO_DIR=$PROTOTYPE_DIR/proto

export GOROOT=/usr/local/go
export GOPATH=`pwd`:$GOPATH
echo "GOPATH=$GOPATH"

export PATH=$(pwd)/bin:$GOROOT/bin:$PATH
echo "PATH=$PATH"

export CGO_CFLAGS="-I/usr/include -I/usr/local/include -I$ATP_DIR/build/third_party/include" 
export CGO_LDFLAGS="-L/usr/lib -L/usr/local/lib -L$ATP_DIR/build/third_party/lib" 

# set up required packages
go get -u github.com/hoisie/web
go get -u code.google.com/p/goprotobuf/...
go get github.com/jmhodges/levigo

# build the protocol buffers
protoc --go_out=src --proto_path=$PROTO_DIR $PROTO_DIR/*.proto

# build the main
go build src/main/main.go

