CFLAGS ?= -g -O2 -std=gnu11 -Wall -Wextra -Wvla
LDLIBS := -lm

cachesim: cachesim.o

.PHONY: clean
