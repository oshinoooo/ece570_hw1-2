#!/bin/bash

submit570 1t src/thread.cc tests/*

g++ -m32 src/thread.cc tests/test1.cc lib/libinterrupt.a -ldl -o test