cmake_minimum_required(VERSION 3.5.1)
project(interpreter C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang)

include_directories(unity include)
set(UNITY_SRCS unity/unity.c)

########################################################
# Use below if you are using entirely your own code
set(SRCS src/linkedlist.c src/talloc.c src/tokenizer.c src/parser.c src/interpreter.c)
########################################################
# Use below if you are using my compiled libraries
#set(LIBS lib/linkedlist.o lib/talloc.o lib/tokenizer.o lib/parser.o)
#set(CFILES src/interpreter.c)
#set(SRCS ${LIBS} ${CFILES})
#set_source_files_properties(${LIBS} PROPERTIES EXTERNAL_OBJECT true GENERATED true)
########################################################

add_executable(interpreter ${SRCS} src/main.c)
add_executable(tests ${SRCS} ${UNITY_SRCS} tests/test.c)

add_executable(run_valgrind ${SRCS} tests/run_valgrind.c)
add_dependencies(run_valgrind interpreter)

add_executable(test_valgrind ${SRCS} ${UNITY_SRCS} tests/test_valgrind.c)
add_dependencies(test_valgrind tests)
