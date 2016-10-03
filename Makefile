SRC = $(wildcard *.cpp)
HDR = $(wildcard *.h)
TGR = ${HOME}/bin/jcc

.Phony: all debug

all : ${TGR}

${TGR}: ${SRC} ${HDR}
	c++ -std=c++0x -o $@ ${SRC}

debug : ${SRC} ${HDR}
	c++ -std=c++0x -g -o ${TGR} ${SRC}

#c++ -std=c++0x -o $@ $^
