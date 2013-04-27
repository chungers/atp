#!/bin/bash

export GOPATH=`pwd`:$GOPATH

echo "GOPATH=$GOPATH"

# set up required packages
go get github.com/hoisie/web

go build src/main/main.go

