CC = gcc

delete: delete.c
	gcc -o delete delete.c -lm

usr: delete.c
	gcc -o /usr/local/bin/del delete.c
