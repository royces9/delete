#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

//separates a string by a given delimiter, the final portion is saved and given as output
//extracts file name from the absolute path given in file
char *separateString(char *input, char delimiter) {
  char *tok, token[2];
  token[0] = delimiter;
  token[1] = '\0';

  int i = 0, length = 0, tokenCount = 0;

  for(length = 0; input[length]; length++) {
    tokenCount += (input[length] == token[0]);
  }

  if(tokenCount == 0){
    return input;
  }

  char *separatedString = malloc((length) * sizeof(separatedString));    
  char *input2 = malloc((length+2) * sizeof(input2));

  //copy input to another string because strtok destroys the original string
  strcpy(input2,input);

  input2[length+1] = 0;

  tok = strtok(input2, token);
  ++tok;

  //we only care about the last bit, so keep rewriting that part until the end
  for(i = 0; tok != NULL; ++i) {
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
  strcpy(out, str1);
  strcat(out, "/");
  strcat(out, str2);
  return out;
}


//checks the object type, returns 0 if directory, 1 otherwise
int checkType(char *path) {
  struct stat pathType;
  stat(path, &pathType);
  if(S_ISDIR(pathType.st_mode)) return 0;
  return 1;
}


//checks if the directory is empty
int emptyDirectory(char *directory) {
  int count = 0;
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
int rmDirContents(char *directory) {
  struct dirent *d;
  DIR *dir = opendir(directory);
  int error = 0;

  while((d = readdir(dir)) != NULL) {
    if((strcmp(d->d_name, ".") && strcmp(d->d_name, ".."))) {
      char *filePath = concatDirectory(directory, d->d_name);

      if(checkType(filePath)) {
	error = unlink(filePath);
	if(error == -1){
	  printf("Error deleting.\n");
	  closedir(dir);
	  free(filePath);
	  return -1;
	}
      } else {
	rmDirContents(filePath);
	rmdir(filePath);
      }
      free(filePath);
    }
  }
  closedir(dir);
  return 0;
}

//moves the contents of directory to target
int moveDirContents(char *directory, char *target) {
  struct dirent *d;
  int type, error;

  DIR *dir = opendir(directory);

  //for every object in the directory
  while((d = readdir(dir)) != NULL) {
    if(access(directory, F_OK) == -1) {
      printf("Directory does not exist.\n");
      closedir(dir);
      return -1;
    } else {
      if((strcmp(d->d_name, ".") && strcmp(d->d_name, ".."))) {
	if(emptyDirectory(directory)) { //makes empty directory
	  mkdir(target, 0755);
	  rmdir(directory);
	} else { 
	  char *filePath = concatDirectory(directory, d->d_name);
	  type = checkType(filePath);
	  
	  char *target2 = concatDirectory(target, d->d_name);

	  //check if the object is a file or directory
	  if(type) { //file, move the file from filePath to target2
	    error = rename(filePath, target2);
	    if(error == -1) {
	      printf("Error removing.\n");
	      closedir(dir);
	      free(target2);
	      free(filePath);
	      return -1;
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
  return 1;
}

//check that a given file exists
//used to check if duplicates are in .trash
char *checkExistence(char *input){
  int size = strlen(input) + 1;
  if(access(input, F_OK) != -1){
    input = realloc(input, (size+1) * sizeof(*input));
    if(input == NULL){
      printf("Couldn't rename file.\n");
      return NULL;
    }
    strcat(input, "_");
    input = checkExistence(input);
  }

  return input;
}


int main(int argc, char **argv){
  if(argc == 1){
    return 0;
  }

  int error, n, type;

  //if sudo is used, check the user calling sudo
  char *user = getenv("SUDO_USER");

  //directory for home
  char *homedir;

  //NULL if sudo was NOT used, just get home variable
  if(user == NULL){ 
    char *temp = getenv("HOME");
    homedir = malloc((strlen(temp) + 7) * sizeof(*homedir));
    strcpy(homedir, temp);
  } else{ //otherwise concatenate "/home/" and user
    homedir = malloc((strlen(user) + 7) * sizeof(*homedir));
    strcpy(homedir, "/home/");
    strcat(homedir, user);
  }
  
  //hardcoded directory for trash is ~/.trash
  int lengthTrashdir = strlen(homedir) + 9;

  char *trashdir = malloc(lengthTrashdir * sizeof(*trashdir));
  strcpy(trashdir, homedir);
  strcat(trashdir, "/.trash");

  //struct for file information
  struct stat st;

  //if there is an argument to empty trash
  if(!strcmp("-empty", argv[1])) {
    printf("Emptying ~/.trash, are you sure? (Y/N)\n");
    char prompt=getchar();
    
    if(prompt == 'Y' || prompt == 'y'){
      rmDirContents(trashdir);
      printf("Complete.\n");
    } else{
      printf("Cancelled.\n");
    }
    free(trashdir);
    free(homedir);
    return 0;
  }

  //loop through all given arguments
  //should be names of files/directories
  for(int i = 1; argv[i]; i++) {
    char *targetPath = malloc((strlen(trashdir) + strlen(argv[i]) + 2) * sizeof(targetPath));
    char *fileName = separateString(argv[i], '/');

    strcpy(targetPath, trashdir);
    strcat(targetPath, "/");
    strcat(targetPath, fileName);

    //check object type
    type = checkType(argv[i]);

    //check that file given as an argument exists
    if(access(argv[i], F_OK) == -1) {
      printf("File does not exist.\n");
      free(targetPath);
      free(fileName);
      return -1;
    }

    //check if file exists in .trash
    //if it does, change the name
    targetPath = checkExistence(targetPath);

    if(type) { //file
      error = rename(argv[i], targetPath);
      if(error) {
	printf("Error moving file to %s.\n", targetPath);
      }
    } else { //directory
      //prompt to delete directory
      printf("Deleting directory: %s \nAre you sure? (Y/N)\n", argv[i]);
      int prompt = getchar();

      //check that prompt is 'Y' or 'y', anything else will cancel
      if((prompt == 'Y') == ((prompt == 'y'))) {
	printf("Cancelled.\n");
	free(targetPath);
	free(fileName);
	return 0;
      }

      //make directory in .trash
      error = mkdir(targetPath, 0755);

      if(error == -1) {
	printf("Error copying directory.\n");
	free(targetPath);
	free(fileName);
	return -1;
      }

      //if the directory given is empty
      //remove it, and continue on
      //otherwise, move the contents in the directory
      //to the new path, and then delete those
      if(emptyDirectory(argv[i])) {
	error = rmdir(argv[i]);
	
	if(error == -1){
	  printf("Error removing directory.\n");
	}
      } else{
	moveDirContents(argv[i], targetPath);
	rmdir(argv[i]);
      }
    }
    free(targetPath);
  }
  free(homedir);
  free(trashdir);
  return 0;
}
