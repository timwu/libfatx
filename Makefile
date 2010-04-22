CFLAGS=-c -Wall
LDFLAGS=
SRCS=libfatx.c
OBJS=${SRCS:.c=.o}
OUTPUT=libfatx.so

all: ${OBJS}
	gcc -shared ${LDFLAGS} -o ${OUTPUT} ${OBJS}

.c.o:
	gcc ${CFLAGS} $<

clean:
	rm ${OBJS}
	rm ${OUTPUT}
	rm -rf html

docs:
	doxygen Doxyfile

