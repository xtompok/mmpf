CC=gcc
CFLAGS= -c -Wall -std=c99 -ggdb3 -O2 -march=native
LDFLAGS=-lprotobuf-c -lm
LDPATHS=-L ./

all: raptor libraptor.so


LIBRAPTOR_SRC=dbg-print.c data.pb-c.c util.c
LIBRAPTOR_OBJ=$(LIBRAPTOR_SRC:.c=.o)
LIBRAPTOR_LIB=libraptor.so

RAPTOR_SRC= 
RAPTOR_OBJ=$(RAPTOR_SRC:.c=.o)
RAPTOR_EXE=raptor
RAPTOR_LDFLAGS=-lraptor -L. -Wl,-rpath=./

$(LIBRAPTOR_LIB): $(LIBRAPTOR_OBJ) libraptor.c 
	echo "Libraptor"
	$(CC) $(CFLAGS) -fpic libraptor.c -o $@.o
	$(CC) -shared  $(LIBRAPTOR_OBJ) $@.o -o $@ $(LDFLAGS)

$(RAPTOR_EXE): $(RAPTOR_OBJ) $(LIBRAPTOR_LIB) $@.c
	echo "Raptor"
	$(CC) $(CFLAGS) $@.c -o $@.o
	$(CC) $(RAPTOR_OBJ) $@.o -o $@ $(LDFLAGS) $(RAPTOR_LDFLAGS)


.c.o:
	$(CC) $(CFLAGS) -fpic $< -o $@


clean:
	rm -f *.o $(LIBRAPTOR_LIB) $(RAPTOR_EXE)

.PHONY: clean all
