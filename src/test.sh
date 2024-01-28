#!/bin/bash
#

if [ $# == 1 ]; then
	i=$1
else
	i=1
fi

cd ..
exe="./build/src/src"
arg1="TestCase/testcase${i}/design.fpga.die"
arg2="TestCase/testcase${i}/design.die.position"
arg3="TestCase/testcase${i}/design.net"
arg4="TestCase/testcase${i}/design.die.network"

$exe $arg1 $arg2 $arg3 $arg4

cd ./src


