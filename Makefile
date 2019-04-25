CC = gcc
CFLAGS = -g3 -Ofast -march=native -std=c11 -D_GNU_SOURCE -pthread -Iinc/ -Llib/ -lcollectc

all: lib/libcollectc.a
	$(CC) $(CFLAGS) -o bin/max_factors src/max_factors.c lib/*.a

lib/libcollectc.a:
	cd libraries/Collections-C/build/ && cmake .. && make && cp src/libcollectc.a ../../../lib/

.PHONY: clean all

clean:
	rm bin/*
