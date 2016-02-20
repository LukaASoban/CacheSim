# Makefile

SUBMIT = cachesim.h cachesim.c cachesim_driver.c Makefile
CFLAGS := -g -Wall -std=c99 -lm -pedantic-errors
CC=gcc

all: cachesim

cachesim: cachesim.o cachesim_driver.o
	$(CC) -o cachesim cachesim.o cachesim_driver.o $(CFLAGS)

clean:
	rm -f cachesim *.o

submit: clean
	tar zcvf cachesim.tar.gz $(SUBMIT)
