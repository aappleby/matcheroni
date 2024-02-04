#!/bin/bash
mkdir -p bin/dbg
set -v
g++ -DDEBUG -std=c++20 -O0 -I.. -c live_demo.cpp -o bin/dbg/live_demo.o
g++ -DDEBUG -std=c++20 -O0 -I.. -c live_parser3.cpp -o bin/dbg/live_parser3.o
g++ bin/dbg/live_demo.o bin/dbg/live_parser3.o  -o bin/dbg/live_demo3

bin/dbg/live_demo3
