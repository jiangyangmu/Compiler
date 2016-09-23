SRC = $(wildcard *.cpp)
HDR = $(wildcard *.h)
#SRC = main.cpp

.Phony: all debug

all : compiler

compiler : ${SRC} ${HDR}
	c++ -std=c++0x -o $@ ${SRC}

debug : ${SRC} ${HDR}
	c++ -std=c++0x -g -o compiler ${SRC}

#c++ -std=c++0x -o $@ $^
