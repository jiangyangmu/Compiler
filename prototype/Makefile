.PHONY: run debug

run : parser
	./parser

test : parser_test
	./parser_test

debug : parser_debug
	lldb parser_debug

parser : parser.cpp parser_main.cpp
	clang++ -std=c++11 parser.cpp parser_main.cpp -o parser

parser_debug : parser.cpp parser_main.cpp
	clang++ -std=c++11 -g parser.cpp parser_main.cpp -o parser_debug

parser_test : ../testing/tester.cpp parser.cpp parser_test.cpp
	clang++ -std=c++11 ../testing/tester.cpp parser.cpp parser_test.cpp -o parser_test
