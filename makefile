CC = gcc

.PHONY:
all: delete exp dcpp drs ers

delete: delete.c delete.h
	gcc -DDEBUG -g -Wall -o $@ delete.c

exp: exp.c exp.h
	gcc -DDEBUG -Wall -o $@ exp.c

dcpp: delete.cpp
	g++ -std=c++17 -DDEBUG -Wall -o $@ delete.cpp -lstdc++fs

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

