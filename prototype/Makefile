.PHONY: run debug

run : bnf
	./bnf

debug : bnfd
	lldb bnfd

bnf : bnf.cpp
	clang++ -std=c++11 bnf.cpp -o bnf

bnfd : bnf.cpp
	clang++ -std=c++11 -g bnf.cpp -o bnfd
