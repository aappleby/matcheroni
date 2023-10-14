#!/bin/bash
set -v
g++ -std=c++20 -Os -I.. -c json_parser.cpp -o obj/json_parser.o
g++ -std=c++20 -Os -I.. -c json_demo.cpp -o obj/json_demo.o
g++ -std=c++20 -Os -I.. obj/json_parser.o obj/json_demo.o  -o bin/json_demo
