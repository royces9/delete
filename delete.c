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
char *separateString(char *input, char delimiter){
  char *tok, token[2];
  token[0] = delimiter;
  token[1] = '\0';

  int i = 0, length = 0, tokenCount = 0;

  for(length = 0; input[length]; length++){
    if(input[length] == token[0]){
      tokenCount++;
    }
  }

  char *separatedString = malloc((length) * sizeof(separatedString));
  if(tokenCount == 0){
    strcpy(separatedString, input);
    return separatedString;
  }
    
  char *input2 = malloc((length+2) * sizeof(input2));

  //copy input to another string because strtok destroys the original string
  strcpy(input2,input);



  input2[length+1] = 0;

  tok = strtok(input2, token);
  ++tok;

  for(i = 0; tok != NULL; i++){
    strcpy(separatedString, tok);
    tok = strtok(NULL, token);
  }

  separatedString[strlen(separatedString)] = '\0';

  free(input2);

  return separatedString;
}

//checks the object type, returns 1 if it's a file, 0 otherwise
int checkType(char *path){
  struct stat pathType;
  stat(path, &pathType);
  return S_ISREG(pathType.st_mode);
}

//checks if the directory is empty
int emptyDirectory(char *directory){
  int count = 0;
  struct dirent *d;
  DIR *dir = opendir(directory);
  if (dir == NULL){
    return -1;
  }
  
  while ((d = readdir(dir)) != NULL){
    if(++count > 2){
      break;
    }
  }
  
  closedir(dir);
  
  if (count <= 2){
    return 1; //returns 1 if directory is empty
  }
  else{
    return 0;
  }  
}

//rm's everything in directory
int rmDirContents(char *directory){
  struct dirent *d;
  DIR *dir = opendir(directory);
  int error, type;

  while((d = readdir(dir)) != NULL){
    if((strcmp(d->d_name, ".") && strcmp(d->d_name, ".."))){
      char *filePath = malloc((strlen(directory)+strlen(d->d_name)+3) * sizeof(*filePath));
      strcpy(filePath, directory);
      strcat(filePath, "/");
      strcat(filePath, d->d_name);

      type = checkType(filePath);

      if(type){
	error = unlink(filePath);
	if(error == -1){
	  printf("Error deleting.\n");
	  return -1;
	}
      }

      else{
	rmDirContents(filePath);
	rmdir(filePath);
      }
    }
  }
  return 0;
}

//moves the contents of directory to target
int moveDirContents(char *directory, char *target){
  struct dirent *d;
  int type, error;

  DIR *dir = opendir(directory);

  while((d = readdir(dir)) != NULL){
    if(access(directory, F_OK) == -1){
      printf("Directory does not exist.\n");
      return -1;
    }

    else{
      if((strcmp(d->d_name, ".") && strcmp(d->d_name, ".."))){
	if(emptyDirectory(directory)){
	  mkdir(target, 0755);
	  rmdir(directory);
	}

	else{
	  char *filePath = malloc((strlen(directory) + strlen(d->d_name) + 3) * sizeof(*filePath));
	  strcpy(filePath, directory);
	  strcat(filePath, "/");
	  strcat(filePath, d->d_name);

	  type = checkType(filePath);
	
	  char *target2 = malloc((strlen(target) + strlen(d->d_name) + 3) * sizeof(*target2));
	  strcpy(target2, target);
	  strcat(target2, "/");
	  strcat(target2, d->d_name);

	  if(type){
	    error = rename(filePath, target2);
	    if(error == -1){
	      printf("Error removing.\n");
	      return -1;
	    }
	  }

	  else{
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
  return 1;
}

char *checkExistence(char *input){
  int check = 1;
  char *buffer;
  char *input2 = malloc((strlen(input) + 1) * sizeof(*input2));
  strcpy(input2, input);

  if(access(input, F_OK) != -1){
    input = calloc(*input, (strlen(input) + 1) * sizeof(*input));
    strcpy(input, input2);
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
  char *fileName, *homedir;

  char *user = getenv("SUDO_USER");

  if(user == NULL){
    homedir = getenv("HOME");
  }
  else{
    homedir = malloc((strlen(user) + 7) * sizeof(*homedir));
    strcpy(homedir, "/home/");
    strcat(homedir, user);
  }
  
  int lengthTrashdir = strlen(homedir) + 8;

  char *trashdir = malloc(lengthTrashdir * sizeof(*trashdir));
  strcpy(trashdir, homedir);
  strcat(trashdir, "/.trash/");

  struct stat st;

  if(!strcmp("-empty", argv[1])){
    printf("Emptying ~/.trash, are you sure? (Y/N)\n");
    char prompt=getchar();

    if(prompt == 'Y' || prompt == 'y'){
      rmDirContents(trashdir);
      printf("Complete.\n");
      return 0;
    }
    else{
      printf("Cancelled.\n");
      return 1;
    }
  }

  for(int i = 1; argv[i]; i++){
    char *targetPath = malloc((strlen(trashdir) + strlen(argv[i]) + 2) * sizeof(targetPath));
    char *fileName = separateString(argv[i], '/');

    strcpy(targetPath, trashdir);
    strcat(targetPath, fileName);

    type = checkType(argv[i]);
    //    printf("%s\n%s\n%s\n", argv[i], targetPath, fileName);

    targetPath = checkExistence(targetPath);

    if(type){
      if(access(argv[i], F_OK) == -1){
	printf("File does not exist.\n");
	return -1;
      }
      error = rename(argv[i], targetPath);

      if(error){
	printf("Error moving file to %s.\n", targetPath);
      }
    }
    else{
      if(access(argv[i], F_OK) == -1){
	printf("File does not exist.\n");
	return -1;
      }

      printf("Deleting directory: %s \nAre you sure? (Y/N)\n", argv[i]);
      int prompt = getchar();
      while(getchar() != '\n');

      if(prompt == 'Y' || prompt == 'y');
      else{
	printf("Cancelled.\n");
	return 0;
      }
      error = mkdir(targetPath, 0755);

      if(error == -1){
	printf("Error copying directory.\n");
	return -1;
      }

      if(emptyDirectory(argv[i])){
	error = rmdir(argv[i]);
	
	if(error == -1){
	  printf("Error removing directory.\n");
	}
      }
      else{
	moveDirContents(argv[i], targetPath);
	rmdir(argv[i]);
      }
    }
    free(targetPath);
  }
  return 0;
}
