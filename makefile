CC = gcc

delete: delete.c delete.h
	gcc -DDEBUG -g -Wall -o delete delete.c

exp: exp.c exp.h
	gcc -DDEBUG -Wall -o exp exp.c

usr: delete.c delete.h
	gcc -o /usr/local/bin/del delete.c


dcpp: delete.cpp config.hpp
	g++ -std=c++17 -DDEBUG -Wall -o dcpp delete.cpp -lstdc++fs

drs: delete.rs
	rustc delete.rs -o drs
