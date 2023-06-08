#set -x
set -e
mkdir -p bin

MODE="-g -O0"
#MODE="-O3"
#MODE="-Os"

echo "Building source"
g++ $MODE -std=c++20 -c cgen.cpp -o bin/cgen.o
g++ $MODE -std=c++20 -c c_lexer.cpp -o bin/c_lexer.o
g++ $MODE -std=c++20 -c c_parser.cpp -o bin/c_parser.o
g++ $MODE -std=c++20 -c c99_parser.cpp -o bin/c99_parser.o
g++ $MODE -std=c++20 -c Node.cpp -o bin/Node.o

#echo "Building cgen"
#g++ $MODE -std=c++20 bin/cgen.o -o bin/cgen

#echo "Building bin/matcheroni_test"
#g++ $MODE -std=c++20 -c test_c_lexer.cpp -o bin/test_c_lexer.o
#g++ $MODE -std=c++20 bin/c_lexer.o bin/test_c_lexer.o -o bin/matcheroni_test

#echo "Building bin/matcheroni_benchmark"
#g++ $MODE -std=c++20 benchmark.cpp -o bin/matcheroni_benchmark

echo "Building bin/matcheroni_demo"
g++ $MODE -std=c++20 -c demo.cpp -o bin/demo.o
g++ $MODE -std=c++20 bin/c_lexer.o bin/c99_parser.o bin/demo.o bin/Node.o -o bin/matcheroni_demo

#echo "Running bin/matcheroni_test"
#bin/matcheroni_test

#echo "Running bin/matcheroni_benchmark"
#bin/matcheroni_benchmark

#echo "Running bin/matcheroni_demo"
#bin/matcheroni_demo ../gcc
