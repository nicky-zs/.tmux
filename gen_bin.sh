#!/bin/sh

set -ue

cd $(dirname $0)

test -d bin || mkdir bin

gcc -O2 src/*.c -o bin/resource_usage

