.PHONY: run debug

run : bnf
	./bnf

debug : bnfd
	lldb bnfd

bnf : bnf.cpp bnf_main.cpp
	clang++ -std=c++11 bnf.cpp bnf_main.cpp -o bnf

bnfd : bnf.cpp bnf_main.cpp
	clang++ -std=c++11 -g bnf.cpp bnf_main.cpp -o bnfd

bnf_test : ../test/tester.cpp bnf.cpp bnf_test.cpp
	clang++ -std=c++11 ../test/tester.cpp bnf.cpp bnf_test.cpp -o bnf_test