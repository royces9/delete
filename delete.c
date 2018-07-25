#include <dirent.h>
#include <fcntl.h>
#include <math.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "delete.h"

int8_t error = 0;

//separates a string by a given delimiter, the final portion is saved and given as output
//extracts file name from the absolute path given in file
char *separateString(char *input, char delimiter) {
	char *tok;
	char token[2];
  token[0] = delimiter;
  token[1] = '\0';


  uint32_t tokenCount = 0;
  uint32_t length = 0;
  for(; input[length]; length++) {
	  if(input[length] == token[0])
		  ++tokenCount;
  }

  if(!tokenCount)
    return input;

  char *separatedString = malloc((length) * sizeof(separatedString));    
  if(!separatedString){
    error = mallocError;
    return NULL;
  }

  char *input2 = malloc((length+2) * sizeof(input2));
  if(!input2){
    error = mallocError;
    return NULL;
  }

  //copy input to another string because strtok destroys the original string
  strcpy(input2,input);

  input2[length+1] = 0;

  tok = strtok(input2, token);
  ++tok;

  int i = 0;
  //we only care about the last bit, so keep rewriting that part until the end
  for(; tok != NULL; ++i) {
    strcpy(separatedString, tok);
    tok = strtok(NULL, token);
  }

  separatedString[strlen(separatedString)] = '\0';

  free(input2);

  return separatedString;
}


//given two strings, concatenate them and separate them by a '/'
//two strings are generally directories/files
char *concatDirectory(char *str1, char *str2) {
  int size = strlen(str1) + strlen(str2) + 2;

  char *out = malloc(size * sizeof(*out));

  if(!out){
    error = mallocError;
    return NULL;
  }

  strcpy(out, str1);
  strcat(out, "/");
  strcat(out, str2);

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
  unsigned int count = 0;

  struct dirent *d;

  DIR *dir = opendir(directory);

  if (dir == NULL) {
    return -1;
  }
  
  while ((d = readdir(dir)) != NULL) {
    if(++count > 2) {
      break;
    }
  }
  
  closedir(dir);

  //returns 1 if directory is empty
  //'.' and '..' are always in a directory, so there need to be
  //more than two things for a non-empty directory
  return (count <= 2);
}


//rm's everything in directory
int8_t rmDirContents(char *directory) {
  struct dirent *d;
  DIR *dir = opendir(directory);

  while((d = readdir(dir)) != NULL) {
    if((strcmp(d->d_name, ".") && strcmp(d->d_name, ".."))) {
      char *filePath = concatDirectory(directory, d->d_name);

      if(checkType(filePath)) {
	if(unlink(filePath) == -1){
	  error = deleteFileError;
	  closedir(dir);
	  free(filePath);
	  break;
	}

      } else {
	rmDirContents(filePath);
	rmdir(filePath);
      }

      free(filePath);
    }
  }

  closedir(dir);
  return error;
}


//moves the contents of directory to target
int8_t moveDirContents(char *directory, char *target) {
  struct dirent *d;
  int type = 0;

  DIR *dir = opendir(directory);

  //for every object in the directory
  while((d = readdir(dir)) != NULL) {

    if(access(directory, F_OK) == -1) {
      error = directoryExist;
      closedir(dir);
      break;

    } else {
      if((strcmp(d->d_name, ".") && strcmp(d->d_name, ".."))) {

	int8_t emptyDir = emptyDirectory(directory);
	if(emptyDir == -1){
	  error = directoryExist;
	  break;

	} else if(emptyDir) { //makes empty directory
	  mkdir(target, 0755);
	  rmdir(directory);

	} else { 

	  char *filePath = concatDirectory(directory, d->d_name);
	  type = checkType(filePath);
	  
	  char *target2 = concatDirectory(target, d->d_name);

	  //check if the object is a file or directory
	  if(type) { //file, move the file from filePath to target2
	    if(rename(filePath, target2) == -1){
	      error = removeFileError;
	      closedir(dir);
	      free(target2);
	      free(filePath);

	      break;
	    }
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
  }

  closedir(dir);
  return error;
}


char *getHome(void){
  //if sudo is used, check the user calling sudo
  char *user = getenv("SUDO_USER");

  //directory for home
  char *homeDirectory;

  //NULL if sudo was NOT used, just get home variable
  if(user == NULL){ 
    char *homeEnv = getenv("HOME");
    homeDirectory = malloc((strlen(homeEnv) + 7) * sizeof(*homeDirectory));

    if(!homeDirectory){
      error = mallocError;
      return NULL;
    }

    strcpy(homeDirectory, homeEnv);

  } else{ //otherwise concatenate "/home/" and user
    homeDirectory = malloc((strlen(user) + 7) * sizeof(*homeDirectory));

    if(!homeDirectory){
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
char *checkExistence(char *input){
  unsigned int size = strlen(input) + 1;

  if(access(input, F_OK) != -1){
    input = realloc(input, (size+1) * sizeof(*input));

    if(!input){
      error = mallocError;
      return NULL;
    }

    strcat(input, "_");
    //also checks that the renamed file
    //exists in trash
    input = checkExistence(input);

    if(!input) return NULL;
  }

  return input;
}


void error_handling(void){
  if(error < 0){
    printf("Error:\n");
    switch(error){
    case -1: printf("Could not delete file."); break;
    case -2: printf("Could not rename file."); break;
    case -3: printf("Directory does not exist."); break;
    case -4: printf("File does not exist."); break;
    case -5: printf("Malloc failure."); break;
    }

    printf("\n");
  }
}


int main(int argc, char **argv){
  if(argc == 1){
    return 0;
  }

  int type = 0;

  //directory for home
  char *homeDirectory = getHome();
  if(error) goto errorGoTo;
  
  //hardcoded directory for trash is ~/.trash
  char *trashDirectory = concatDirectory(homeDirectory, ".trash");
  if(error) goto errorGoTo;


  //if there is an argument to empty trash
  if(!strcmp("-empty", argv[1])) {
    rmDirContents(trashDirectory);
    printf("Complete.\n");

    free(trashDirectory);
    free(homeDirectory);
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
    char *fileName = separateString(argv[i], '/');
    if(error) break;

    //targetPath: the path to the file in the trash directory
    char *targetPath = concatDirectory(trashDirectory, fileName);
    if(error) break;
  
    //check if file exists in .trash
    //if it does, change the name
    targetPath = checkExistence(targetPath);
    if(error) break;

    //check object type
    type = checkType(argv[i]);

    if(type) { //file
      //move file from current location
      //to new location in trash
      if(rename(argv[i], targetPath)){
	  error = deleteFileError;
	  break;
      }

    } else { //directory
      //make directory in .trash
      if(mkdir(targetPath, 0755) == -1){
	error = makeDirectoryError;
	free(targetPath);
	break;
      }

      //if the directory given is empty
      //remove it, and continue on
      //otherwise, move the contents in the directory
      //to the new path, and then delete those
      if(emptyDirectory(argv[i])) {
	if(rmdir(argv[i]) == -1){
	  error = removeDirError;
	  break;
	}

      } else{
	moveDirContents(argv[i], targetPath);
	rmdir(argv[i]);
      }
    }
    free(targetPath);
  }

 errorGoTo:
  error_handling();
  return 0;

}
