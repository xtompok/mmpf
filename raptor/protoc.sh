#!/bin/sh
protoc -I=./ --python_out=./ data.proto
protoc-c --c_out=. data.proto


