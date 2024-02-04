#!/bin/bash
mkdir -p bin/opt
set -v
g++ -std=c++20 -O3 -I.. -c live_bench.cpp -o bin/opt/live_bench.o
g++ -std=c++20 -O3 -I.. -c live_parser2.cpp -o bin/opt/live_parser2.o
g++ bin/opt/live_bench.o bin/opt/live_parser2.o -o bin/opt/live_bench2

bin/opt/live_bench2

size bin/opt/live_parser2.o
