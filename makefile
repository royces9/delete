CC = gcc

TEST_PATH := "\"${PWD}/trash/\""
REL_PATH := "\"${HOME}/.trash/\""

CFLAGS = -Wall

.PHONY:

.DEFAULT_GOAL = debug

debug: CFLAGS += -g -DTRASH_PATH=$(TEST_PATH)
debug: all

release: CFLAGS += -DTRASH_PATH=$(REL_PATH) -O2
release: all

all: delete exp dcpp drs ers

delete: delete.c delete.h
	gcc $(CFLAGS) delete.c -o $@ 

exp: exp.c exp.h
	gcc $(CFLAGS) exp.c -o $@ 

dcpp: delete.cpp
	g++ -std=c++17 $(CFLAGS) delete.cpp -lstdc++fs -o $@ 

drs: delete.rs
	rustc delete.rs -o $@

ers: exp.rs
	rustc exp.rs -o $@


.PHONY:
clean:
	del delete
	del exp
	del dcpp
	del drs
	del ers

