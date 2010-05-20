CFLAGS=-Wall
LDFLAGS=
LDLIBS=
SRCS=libfatx.c libfatxutils.c
OBJS=${SRCS:.c=.o}
OUTPUT=libfatx.o
TEST_EXECUTABLE=libfattest
TEST_OBJECT=libfattest.o

.SUFFIXES: .c .o

test: ${TEST_EXECUTABLE}
	./${TEST_EXECUTABLE} /Volumes/DATA2/fatx_data

${TEST_EXECUTABLE}: ${OBJS} ${TEST_OBJECT}

clean:
	rm ${OBJS}
	rm ${OUTPUT}
	rm -rf html

docs:
	doxygen Doxyfile

