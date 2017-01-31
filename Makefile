SRC = $(wildcard [^_]*.cpp)
HDR = $(wildcard *.h)
TGR = ${HOME}/bin/jcc

.Phony: all debug

all : ${TGR}

debug : ${SRC} ${HDR}
	c++ -std=c++11 -Wall -pedantic -Wextra -Wno-unused-parameter -g -o ${TGR} ${SRC}

${TGR}: ${SRC} ${HDR}
	c++ -std=c++11 -Wall -pedantic -Wextra -Wno-unused-parameter -o $@ ${SRC}

#c++ -std=c++0x -o $@ $^
