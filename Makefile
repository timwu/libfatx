CFLAGS=-g -Wall
LDFLAGS=
LDLIBS=
SRCS=libfatx.c libfatx_internal.c
OBJS=${SRCS:.c=.o}
TEST_EXECUTABLE=libfattest
TEST_OBJECT=libfattest.o
TEST_TARGET=/dev/disk4s3

.SUFFIXES: .c .o

test: ${TEST_EXECUTABLE}
	./${TEST_EXECUTABLE} ${TEST_TARGET}

${TEST_EXECUTABLE}: ${OBJS} ${TEST_OBJECT}

clean:
	-rm ${OBJS}
	-rm ${TEST_OBJECT}
	-rm ${TEST_EXECUTABLE}
	-rm -rf html

docs:
	doxygen Doxyfile

