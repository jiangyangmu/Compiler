.PHONY: run debug

run : bnf
	./bnf

debug : bnfd
	lldb bnfd

bnf : bnf.cpp
	clang++ -std=c++11 bnf.cpp bnf_main.cpp -o bnf

bnfd : bnf.cpp
	clang++ -std=c++11 -g bnf.cpp bnf_main.cpp -o bnfd
