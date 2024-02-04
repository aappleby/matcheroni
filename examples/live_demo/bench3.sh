#!/bin/bash
mkdir -p bin/opt
set -v
g++ -std=c++20 -O3 -I.. -c live_bench.cpp -o bin/opt/live_bench.o
g++ -std=c++20 -O3 -I.. -c live_parser3.cpp -o bin/opt/live_parser3.o
g++ bin/opt/live_bench.o bin/opt/live_parser3.o -o bin/opt/live_bench3

bin/opt/live_bench3

size bin/opt/live_parser3.o
