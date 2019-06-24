#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "delete.h"

char const *const trash_path = "/home/royce/Documents/program/delete/trash/";

char error = 0;

//extracts file name from the absolute path given in file
char *get_file_name(char *input, char delimiter) {
	int length = strlen(input);
	char *out = input + length - 1;

	while((length > 0) && (*out == delimiter)) {
		--out;
		--length;
	}

	for(; (length > 1) && (*out != delimiter); --out, --length);

	return out;
}


//given two strings, concatenate them and separate them by a '/'
//two strings are generally directories/files
char *concat_dir(char const *const aa, int a_len ,char const *const bb, int b_len) {
	int size = a_len + b_len + 2;

	char *out = malloc(size * sizeof(*out));

	if(!out)
		return out;

	strcpy(out, aa);
	strcat(out, "/");
	strcat(out, bb);
	
	return out;
}


//checks the object type, returns 0 if directory, 1 otherwise
int checkType(char *path) {
	struct stat pathType;

	stat(path, &pathType);

	return !(S_ISDIR(pathType.st_mode));
}


//rm's everything in directory
void rmDirContents(char const *const directory) {
	struct dirent *d;
	DIR *dir = opendir(directory);
	int dir_len = strlen(directory);

	while( (!error) && ((d = readdir(dir)) != NULL)) {
		if((strcmp(d->d_name, ".") && strcmp(d->d_name, ".."))) {
			int d_name_len = strlen(d->d_name);
			char *filePath = concat_dir(directory, dir_len, d->d_name, d_name_len);

			if(checkType(filePath)) {
				if(unlink(filePath) == -1)
					error = deleteFileError;
			} else {
				rmDirContents(filePath);
				rmdir(filePath);
			}

			free(filePath);
		}
	}

	closedir(dir);
}


//check that a given file exists
//used to check if duplicates are in .trash
//returns the new string
int checkExistence(char **input){
	int len = strlen(*input);

	while(access(*input, F_OK) != -1) {
		char *out = realloc(*input, (len + 2) * sizeof(*out));
		if(!out)
			return 1;

		strcpy(out, *input);
		out[len + 1] = 0;
		out[len] = 0;
		int i = len - 1;

		for(; out[i] == '/'; --i)
			out[i] = 0;

		out[i + 1] = '_';
		free(*input);

		*input = out;

		++len;
	}

	return 0;
}


int main(int argc, char **argv) {
	if(argc == 1)
		return 0;

	//if there is an argument to empty trash
	if(!strcmp("-empty", argv[1])) {
		rmDirContents(trash_path);
		return 0;
	}

	//loop through all given arguments
	//should be names of files/directories
	for(int i = 1; argv[i] && !error; i++) {

		if(access(argv[i], F_OK) == -1) {
			printf("%s does not exist.\n", argv[i]);
			continue;
		}

		//fileName: the name of just the file that is going to be deleted
		char *fileName = get_file_name(argv[i], '/');
		//targetPath: the path to the file in the trash directory
		char *targetPath = concat_dir(trash_path, strlen(trash_path), fileName, strlen(fileName));
		if(!targetPath)
			break;
  
		//check if file exists in .trash
		//if it does, change the name
		if(checkExistence(&targetPath))
			break;

		//move file to trash
		if(rename(argv[i], targetPath))
			error = deleteFileError;
		
		free(targetPath);
	}

	return 0;

}
