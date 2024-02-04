#!/bin/bash
mkdir -p bin/dbg
set -v
g++ -DDEBUG -std=c++20 -O0 -I.. -c live_demo.cpp -o bin/dbg/live_demo.o
g++ -DDEBUG -std=c++20 -O0 -I.. -c live_parser2.cpp -o bin/dbg/live_parser2.o
g++ bin/dbg/live_demo.o bin/dbg/live_parser2.o  -o bin/dbg/live_demo2

bin/dbg/live_demo2
