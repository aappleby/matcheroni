#!/bin/bash
set -v
rm -rf bin
mkdir -p bin

#g++ -std=c++20 -O3 -I.. -c live_parser0.cpp -o bin/live_parser.o
g++ -std=c++20 -O3 -I.. -c live_parser1.cpp -o bin/live_parser.o
#g++ -std=c++20 -O3 -I.. -c live_parser2.cpp -o bin/live_parser.o
#g++ -std=c++20 -O3 -I.. -c live_parser3.cpp -o bin/live_parser.o
g++ -std=c++20 -O3 -I.. -c live_demo.cpp -o bin/live_demo.o
g++ -std=c++20 -O3 bin/live_parser.o bin/live_demo.o  -o bin/live_demo

bin/live_demo
