#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"

#include "delete.h"

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

	if(!out){
		error = mallocError;
		return out;
	}

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


//checks if the directory is empty
int8_t emptyDirectory(char *directory) {
	int count = 0;

	struct dirent *d;

	DIR *dir = opendir(directory);

	if(!dir)
		return -1;
  
	while ( (d = readdir(dir)) ) {
		if(++count > 2)
			break;
	}
  
	closedir(dir);

	//returns 1 if directory is empty
	//'.' and '..' are always in a directory, so there need to be
	//more than two things for a non-empty directory
	return (count <= 2);
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


//moves the contents of directory to target
int8_t moveDirContents(char *directory, char *target) {
	struct dirent *d;
	int type = 0;

	DIR *dir = opendir(directory);

	int dir_len = strlen(directory);
	int target_len = strlen(target);
	//for every object in the directory
	while( (!error) && (d = readdir(dir)) ) {

		if(access(directory, F_OK) == -1) {
			error = directoryExist;

		} else if((strcmp(d->d_name, ".") && strcmp(d->d_name, ".."))) {
			int8_t emptyDir = emptyDirectory(directory);
			if(emptyDir == -1){
				error = directoryExist;

			} else if(emptyDir) { //makes empty directory
				mkdir(target, 0755);
				rmdir(directory);

			} else {
				int d_name_len = strlen(d->d_name);
				char *filePath = concat_dir(directory, dir_len, d->d_name, d_name_len);
				type = checkType(filePath);
	  
				char *target2 = concat_dir(target, target_len, d->d_name, d_name_len);

				//check if the object is a file or directory
				if(type) { //file, move the file from filePath to target2
					if(rename(filePath, target2) == -1)
						error = removeFileError;

				} else { //directory, make a new directory, and move all the contents
					//in the directory, and then remove the old directory
					mkdir(target2, 0755);
					moveDirContents(filePath, target2);
					rmdir(filePath);
				}

				free(target2);
				free(filePath);
			}
		}
	}

	closedir(dir);
	return error;
}


char *getHome(void){
	//if sudo is used, check the user calling sudo
	char *user = getenv("SUDO_USER");

	//directory for home
	char *homeDirectory = NULL;

	//NULL if sudo was NOT used, just get home variable
	if(!user) { 
		user = getenv("HOME");
		homeDirectory = malloc((strlen(user) + 1) * sizeof(*homeDirectory));
		if(!homeDirectory) {
			error = mallocError;
			return NULL;
		}

		strcpy(homeDirectory, user);

	} else {
		homeDirectory = malloc((strlen(user) + 7) * sizeof(*homeDirectory));
		if(!homeDirectory) {
			error = mallocError;
			return NULL;
		}

		strcpy(homeDirectory, "/home/");
		strcat(homeDirectory, user);
	}

	return homeDirectory;
}


//check that a given file exists
//used to check if duplicates are in .trash
//returns the new string
void checkExistence(char **input){
	int size = strlen(*input) + 1;

	if(access(*input, F_OK) == -1)
		return;

	char *out = malloc((size + 1) * sizeof(*out));
	if(!out) {
		error = mallocError;
		return;
	}
	strcpy(out, *input);
	strcat(out, "_");
	free(*input);

	//also checks if the renamed file
	//exists in trash
	checkExistence(&out);

	if(!input)
		return;

	*input = out;
}


void error_handling(void) {
	if(error < 0) {
		puts("Error:");
		switch(error) {
		case -1: puts("Could not delete file."); break;
		case -2: puts("Could not rename file."); break;
		case -3: puts("Directory does not exist."); break;
		case -4: puts("File does not exist."); break;
		case -5: puts("Malloc failure."); break;
		}
	}
}


void move_file(int type, char *src, char *dest) {
	if(type) { //file
		//move file from current location
		//to new location in trash
		if(rename(src, dest)) 
			error = deleteFileError;

	} else { //directory
		//make directory in .trash
		if(mkdir(dest, 0755) == -1) {
			error = makeDirectoryError;
			return;
		}

		//if the directory given is empty
		//remove it, and continue on
		//otherwise, move the contents in the directory
		//to the new path, and then delete those
		if(!emptyDirectory(src))
			moveDirContents(src, dest);

		if(rmdir(src) == -1)
			error = removeDirError;
	}

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
			error = fileExist;
			break; 
		}

		//fileName: the name of just the file that is going to be deleted
		char *fileName = get_file_name(argv[i], '/');

		//targetPath: the path to the file in the trash directory
		char *targetPath = concat_dir(trash_path, strlen(trash_path), fileName, strlen(fileName));
		if(error) break;
  
		//check if file exists in .trash
		//if it does, change the name
		checkExistence(&targetPath);
		if(error) break;

		//check object type
		int type = checkType(argv[i]);

		move_file(type, argv[i], targetPath);
		if(error) break;
		
		free(targetPath);
	}

	error_handling();
	return 0;

}
