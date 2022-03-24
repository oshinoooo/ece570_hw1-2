#!/bin/bash

scp -P 2500 thread.cc test1.cc group28@localhost:Desktop/.

submit570 1t src/thread.cc tests/*

g++ -m32 src/thread.cc tests/test1.cc lib/libinterrupt.a -ldl -o test