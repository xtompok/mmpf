CFLAGS= -Wall -std=c99 -ggdb3
LDFLAGS=-lprotobuf-c

all: raptor

raptor: raptor.c data.pb-c.c
	$(CC) $(CFLAGS) raptor.c data.pb-c.c -o raptor $(LDFLAGS)


