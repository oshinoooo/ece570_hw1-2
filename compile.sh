#!/bin/bash

g++ disk.cc lib/libinterrupt.a -ldl -o disk

./disk 3 TestCases/disk.in0 TestCases/disk.in1 TestCases/disk.in2 TestCases/disk.in3 TestCases/disk.in4