#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "exp.h"

int8_t error = 0;

void error_handling(void) {
	if(error) {
		switch(error) {
		case exec_err: printf("Exec error."); break;
		case fork_err: printf("Fork error."); break;
		case malloc_err: printf("Malloc error."); break;
		}
		printf("\n");
	}
}

//extracts file name from the absolute path given in file
char *separateString(char *input, char delimiter) {
	int length = strlen(input);
	int length2 = length;

	//empty for
	for(; (length2 > 0) && (input[length2] != delimiter); --length2);

	if(length != length2)
		input += length2;

	return input;
}


void clear_trash(char *trash) {
	int pid = fork();
	if( !pid ) {
		chdir(trash);

		struct dirent *d;

		DIR *dir = opendir(".");

		while( !error && (d = readdir(dir)) ) {
			if( !((strcmp(d->d_name, ".") && strcmp(d->d_name, ".."))) )
				continue;

			if( !(pid = fork()) ) {
				if(execvp("rm", (char *[]) {"rm", "-r", d->d_name, NULL}))
					error = exec_err;
			} else if( pid == -1) {
				error = fork_err;
			}
		}

	} else if( pid == -1 ) {
		error = fork_err;
	} else {
		wait(&pid);
	}
}

char *concatDirectory(char *str1, char *str2) {
	int size = strlen(str1) + strlen(str2) + 3;

	char *out = malloc(size * sizeof(*out));

	if(!out){
		error = malloc_err;
	} else {
		strcpy(out, str1);
		strcat(out, "/");
		strcat(out, str2);
	}

	return out;
}


char *checkExistence(char *input){
	int size = strlen(input) + 1;

	if(access(input, F_OK) != -1){
		if( (input = realloc(input, (size+1) * sizeof(*input))) ) {
			strcat(input, "_");
			//also checks if the renamed file
			//exists in trash
			input = checkExistence(input);
		} else {
			error = malloc_err;
		}
	}

	return input;
}



char *getHome(void){
	//if sudo is used, check the user calling sudo
	char *user = getenv("SUDO_USER");

	//directory for home
	char *homeDirectory = NULL;

	//NULL if sudo was NOT used, just get home variable
	if(!user) {
		user = getenv("HOME");
		if( !(homeDirectory = malloc((strlen(user) + 1) * sizeof(*homeDirectory))) ) {
			error = malloc_err;
		} else {
			strcpy(homeDirectory, user);
		}
	} else {
		if( !(homeDirectory = malloc((strlen(user) + 7) * sizeof(*homeDirectory))) ) {
			error = malloc_err;
		} else {
			strcpy(homeDirectory, "/home/");
			strcat(homeDirectory, user);
		}
	}

	return homeDirectory;
}


int main(int argc, char **argv) {
	if(argc == 1)
		return 0;

	//directory for home
	char *homeDirectory = getHome();
	if(error) goto errorGoTo;
	char *trashDirectory =
	#if DEBUG
	malloc(sizeof(*trashDirectory) * 9);
	strcpy(trashDirectory, "trash/");
	#else
	//hardcoded directory for trash is ~/.trash
	concatDirectory(homeDirectory, ".trash/");
	if(error) goto errorGoTo;
	#endif

	int pid = 0;

	//if there is an argument to empty trash
	if(!strcmp("-empty", argv[1])) {
		clear_trash(trashDirectory);
		goto errorGoTo;
	}

	//loop through all given arguments
	//should be names of files/directories
	for(int i = 1; argv[i] && !error; i++) {
		//fileName: the name of just the file that is going to be deleted
		char *fileName = separateString(argv[i], '/');
		if(error) break;

		//targetPath: the path to the file in the trash directory
		char *targetPath = concatDirectory(trashDirectory, fileName);
		if(error) break;

		//check if file exists in .trash
		//if it does, change the name
		targetPath = checkExistence(targetPath);
		if(error) break;

		if( !(pid = fork()) ) {
			if( !execvp("mv", (char *[]) {"mv", fileName, targetPath, NULL}) )
				error = exec_err;

			break;
		} else if(pid == -1) {
			error = fork_err;
			break;
		} else {
			wait(&pid);
		}
 
		free(targetPath);
	}

 errorGoTo:
	error_handling();

	free(trashDirectory);
	free(homeDirectory);

	return 0;

}
