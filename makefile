VALGRIND_FLAGS=--leak-check=full --track-origins=yes --show-reachable=yes
CC = gcc

all: cachesim

cachesim: main.c
	$(CC) main.c -o cachesim -lm