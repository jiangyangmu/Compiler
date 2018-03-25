PROJECT_DIR = $(PWD)
BUILD_DIR = build
TEST_DIR = test
SRC_DIR = src

CC = c++
CC_FLAGS = -std=c++11 -Wall -pedantic -Wextra -Wno-unused-parameter -g -c
LD = c++

JCC = jcc
JCC_HDR = $(wildcard ${SRC_DIR}/*.h)
JCC_LIB = ${SRC_DIR}/parser.cpp \
		  ${SRC_DIR}/type.cpp \
		  ${SRC_DIR}/env.cpp \
		  ${SRC_DIR}/ir.cpp
JCC_SRC = ${JCC_LIB} ${SRC_DIR}/main.cpp
JCC_OBJ = $(patsubst ${SRC_DIR}/%.cpp, ${BUILD_DIR}/%.o, ${JCC_SRC})

JCC_TEST = jcc_test
JCC_TEST_HDR = ${HDR} $(wildcard ${TEST_DIR}/*.h)
# make sure we link tester.cpp first
JCC_TEST_SRC = ${TEST_DIR}/tester.cpp \
			   $(filter-out ${TEST_DIR}/tester.cpp, $(wildcard ${TEST_DIR}/*.cpp)) \
			   ${JCC_LIB}
JCC_TEST_OBJ = $(patsubst %.cpp, ${BUILD_DIR}/%.o, \
			   $(patsubst ${TEST_DIR}/%.cpp, ${BUILD_DIR}/%.o, \
			   ${JCC_TEST_SRC}))

IR_TEST = ir_test
IR_TEST_SRC = ${TEST_DIR}/tester.cpp \
			  ${TEST_DIR}/ir_test.cpp \
			  ${JCC_LIB}
IR_TEST_OBJ = $(patsubst %.cpp, ${BUILD_DIR}/%.o, \
			  $(patsubst ${TEST_DIR}/%.cpp, ${BUILD_DIR}/%.o, \
			  ${IR_TEST_SRC}))

.PHONY: all test clean rebuild

all : dir ${HOME}/bin/${JCC}

test : ${HOME}/bin/${JCC_TEST}
	${JCC_TEST}

ir_test : ${HOME}/bin/ir_test
	ir_test

clean :
	rm ${BUILD_DIR}/*.o

rebuild : clean all

dir :
	mkdir -p $(BUILD_DIR)

${HOME}/bin/${JCC} : ${BUILD_DIR}/${JCC}
	[ -e "$$HOME/bin/${JCC}" ] || ln -s ${PROJECT_DIR}/${BUILD_DIR}/${JCC} ${HOME}/bin/${JCC}

${HOME}/bin/${JCC_TEST} : ${BUILD_DIR}/${JCC_TEST}
	[ -e "$$HOME/bin/${JCC_TEST}" ] || ln -s ${PROJECT_DIR}/${BUILD_DIR}/${JCC_TEST} ${HOME}/bin/${JCC_TEST}

${HOME}/bin/${IR_TEST} : ${BUILD_DIR}/${IR_TEST}
	[ -e "$$HOME/bin/${IR_TEST}" ] || ln -s ${PROJECT_DIR}/${BUILD_DIR}/${IR_TEST} ${HOME}/bin/${IR_TEST}

${BUILD_DIR}/${JCC} : ${JCC_OBJ}
	${LD} $^ -o $@

${BUILD_DIR}/${JCC_TEST} : ${JCC_TEST_OBJ}
	${LD} $^ -o $@

${BUILD_DIR}/${IR_TEST} : ${IR_TEST_OBJ}
	${LD} $^ -o $@

${BUILD_DIR}/%.o : %.cpp
	${CC} ${CC_FLAGS} -o $@ $<

${BUILD_DIR}/%.o : ${TEST_DIR}/%.cpp
	${CC} ${CC_FLAGS} -o $@ $<

