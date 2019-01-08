CC = gcc

delete: delete.c delete.h
	gcc -DDEBUG -Wall -o delete delete.c

exp: exp.c exp.h
	gcc -DDEBUG -Wall -o exp exp.c

usr: delete.c delete.h
	gcc -o /usr/local/bin/del delete.c
