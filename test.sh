g++ -g -MMD -std=c++20 -DCONFIG_RELEASE -O3 -I. -Isymlinks/MetroLib  -MF obj/parseroni/ParseroniApp.o.d -c parseroni/ParseroniApp.cpp -o obj/parseroni/ParseroniApp.o
g++ -g -MMD -std=c++20 -DCONFIG_RELEASE -O3 -I. -Isymlinks/MetroLib  -MF obj/parseroni/Parser.o.d -c parseroni/Parser.cpp -o obj/parseroni/Parser.o
g++ -g -MMD -std=c++20 -DCONFIG_RELEASE -O3 -I. -Isymlinks/MetroLib  -MF obj/parseroni/Combinators.o.d -c parseroni/Combinators.cpp -o obj/parseroni/Combinators.o
 g++ -g -MMD -std=c++20 -DCONFIG_RELEASE -O3 -I. -Isymlinks/MetroLib  -MF obj/parseroni/NewThingy.o.d -c parseroni/NewThingy.cpp -o obj/parseroni/NewThingy.o
10] g++ -g -MMD -std=c++20 -DCONFIG_RELEASE -O3 -I. -Isymlinks/MetroLib  -MF obj/parseroni/Matcheroni.o.d -c parseroni/Matcheroni.cpp -o obj/parseroni/Matcheroni.o
[6/10] g++ -g -MMD -std=c++20 -DCONFIG_RELEASE -O3 -I. -Isymlinks/MetroLib  -MF obj/tests/ParseroniTest.o.d -c tests/ParseroniTest.cpp -o obj/tests/ParseroniTest.o
[7/10] ninja -C symlinks/MetroLib
[8/10] g++ -g -MMD -std=c++20 -DCONFIG_RELEASE -O3 obj/parseroni/Parser.o obj/parseroni/Combinators.o obj/parseroni/NewThingy.o obj/parseroni/Matcheroni.o obj/parseroni/ParseroniApp.o symlinks/MetroLib/bin/metrolib/libcore.a  -o bin/parseroni
[9/10] g++ -g -MMD -std=c++20 -DCONFIG_RELEASE -O3 obj/parseroni/Parser.o obj/parseroni/Combinators.o obj/parseroni/NewThingy.o obj/parseroni/Matcheroni.o obj/tests/ParseroniTest.o symlinks/MetroLib/bin/metrolib/libcore.a  -o bin/parseroni_test
[10/10] bin/parseroni_test
