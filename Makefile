#SRC = $(wildcard *.cpp)
HDR = $(wildcard *.h)
SRC = main.cpp

compiler : ${SRC} ${HDR}
	c++ -std=c++0x -o $@ ${SRC}

#c++ -std=c++0x -o $@ $^
