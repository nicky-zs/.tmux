#!/bin/sh

set -ue

cd $(dirname $0)

test -d bin || mkdir bin

gcc -O3 -Wall src/*.c -o bin/resource_usage

sudo touch /var/run/resource-usage.cpu.data
sudo chmod 666 /var/run/resource-usage.cpu.data

