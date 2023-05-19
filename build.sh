set -x
mkdir -p bin

#MODE="-g -O0"
MODE="-O3 -flto"

echo "Building examples"
g++ $MODE -std=c++20 -c examples.cpp -o bin/examples.o

#echo "Building bin/matcheroni_test"
#g++ $MODE -std=c++20 -c test.cpp -o bin/test.o
#g++ $MODE -std=c++20 bin/examples.o bin/test.o -o bin/matcheroni_test

#echo "Building bin/matcheroni_benchmark"
#g++ $MODE -std=c++20 benchmark.cpp -o bin/matcheroni_benchmark

echo "Building bin/matcheroni_demo"
g++ $MODE -std=c++20 -c demo.cpp -o bin/demo.o
g++ $MODE -std=c++20 bin/examples.o bin/demo.o -o bin/matcheroni_demo

#echo "Running bin/matcheroni_test"
#bin/matcheroni_test

#echo "Running bin/matcheroni_benchmark"
#bin/matcheroni_benchmark
