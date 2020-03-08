#!/bin/sh

set -ue

cd $(dirname $0)

test -d bin || mkdir bin

gcc -O3 -Wall src/*.c -o bin/resource_usage

