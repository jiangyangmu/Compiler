PROJECT_DIR = $(PWD)
BUILD_DIR = build
TEST_DIR = test

CC = c++
CC_FLAGS = -std=c++11 -Wall -pedantic -Wextra -Wno-unused-parameter -g -c
LD = c++

JCC = jcc
JCC_HDR = $(wildcard *.h)
JCC_LIB = parser.cpp type.cpp env.cpp
JCC_SRC = ${JCC_LIB} main.cpp
JCC_OBJ = $(patsubst %.cpp, ${BUILD_DIR}/%.o, ${JCC_SRC})

JCC_TEST = jcc_test
JCC_TEST_HDR = ${HDR} $(wildcard test/*.h)
# make sure we link tester.cpp first
JCC_TEST_SRC = ${TEST_DIR}/tester.cpp \
			   $(filter-out ${TEST_DIR}/tester.cpp, $(wildcard ${TEST_DIR}/*.cpp)) \
			   ${JCC_LIB}
JCC_TEST_OBJ = $(patsubst %.cpp, ${BUILD_DIR}/%.o, \
			   $(patsubst ${TEST_DIR}/%.cpp, ${BUILD_DIR}/%.o, \
			   ${JCC_TEST_SRC}))

.PHONY: all test clean rebuild

all : dir ${HOME}/bin/${JCC}

test : ${HOME}/bin/${JCC_TEST}
	${JCC_TEST}

clean :
	rm ${BUILD_DIR}/*.o

rebuild : clean all

dir :
	mkdir -p $(BUILD_DIR)

${HOME}/bin/${JCC} : ${BUILD_DIR}/${JCC}
	[ -e "$$HOME/bin/${JCC}" ] || ln -s ${PROJECT_DIR}/${BUILD_DIR}/${JCC} ${HOME}/bin/${JCC}

${HOME}/bin/${JCC_TEST} : ${BUILD_DIR}/${JCC_TEST}
	[ -e "$$HOME/bin/${JCC_TEST}" ] || ln -s ${PROJECT_DIR}/${BUILD_DIR}/${JCC_TEST} ${HOME}/bin/${JCC_TEST}

${BUILD_DIR}/${JCC} : ${JCC_OBJ}
	${LD} $^ -o $@

${BUILD_DIR}/${JCC_TEST} : ${JCC_TEST_OBJ}
	${LD} $^ -o $@

${BUILD_DIR}/%.o : %.cpp
	${CC} ${CC_FLAGS} -o $@ $<

${BUILD_DIR}/%.o : ${TEST_DIR}/%.cpp
	${CC} ${CC_FLAGS} -o $@ $<

