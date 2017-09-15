#SRC = $(wildcard [^_]*.cpp)
SRC = parser.cpp env.cpp main.cpp
HDR = $(wildcard *.h)
TGR = ${HOME}/bin/jcc

# make sure we link tester.cpp first
SRC_TEST = test/tester.cpp $(filter-out test/tester.cpp, $(wildcard test/*.cpp))
SRC_LIB = parser.cpp env.cpp
HDR_TEST = ${HDR} $(wildcard test/*.h)
TGR_TEST = ${HOME}/bin/jcc_test

.Phony: all debug test

debug : ${SRC} ${HDR}
	c++ -std=c++11 -Wall -pedantic -Wextra -Wno-unused-parameter -g -o ${TGR} ${SRC}

all : ${TGR}

test : ${TGR_TEST}
	${TGR_TEST}

${TGR}: ${SRC} ${HDR}
	c++ -std=c++11 -Wall -pedantic -Wextra -Wno-unused-parameter -o $@ ${SRC}

${TGR_TEST}: ${SRC_TEST} ${HDR_TEST} ${SRC_LIB}
	c++ -std=c++11 -Wall -pedantic -Wextra -Wno-unused-parameter -g -o $@ ${SRC_TEST} ${SRC_LIB}

#c++ -std=c++0x -o $@ $^
