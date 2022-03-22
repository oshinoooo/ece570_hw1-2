#!/bin/bash

scp -P 2500 thread.cc test1.cc group28@localhost:Desktop/.

submit570 1t Desktop/thread.cc Desktop/test1.cc

g++ -m32 thread.cc test1.cc lib/libinterrupt.a -ldl -o test

./disk 3 TestCases/disk.in0 TestCases/disk.in1 TestCases/disk.in2 TestCases/disk.in3 TestCases/disk.in4