SRC = $(wildcard [^_]*.cpp)
HDR = $(wildcard *.h)
TGR = ${HOME}/bin/jcc

.Phony: all debug svm

all : ${TGR}

${TGR}: ${SRC} ${HDR}
	c++ -std=c++11 -Wall -pedantic -Wextra -Wno-unused-parameter -o $@ ${SRC}

debug : ${SRC} ${HDR}
	c++ -std=c++11 -Wall -pedantic -Wextra -Wno-unused-parameter -g -o ${TGR} ${SRC}

svm : ${HOME}/bin/svm

${HOME}/bin/svm : _svm_main.cpp
	c++ -std=c++11 -o $@ $<

#c++ -std=c++0x -o $@ $^
