#SRC = $(wildcard [^_]*.cpp)
SRC = env.cpp parser.cpp main.cpp
HDR = $(wildcard *.h)
TGR = ${HOME}/bin/jcc

.Phony: all debug

debug : ${SRC} ${HDR}
	c++ -std=c++11 -Wall -pedantic -Wextra -Wno-unused-parameter -g -o ${TGR} ${SRC}

all : ${TGR}

${TGR}: ${SRC} ${HDR}
	c++ -std=c++11 -Wall -pedantic -Wextra -Wno-unused-parameter -o $@ ${SRC}

#c++ -std=c++0x -o $@ $^
