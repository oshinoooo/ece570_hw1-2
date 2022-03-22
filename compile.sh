#!/bin/bash

g++ -m32 thread.cc test1.cc lib/libinterrupt.a -ldl -o test

./disk 3 TestCases/disk.in0 TestCases/disk.in1 TestCases/disk.in2 TestCases/disk.in3 TestCases/disk.in4