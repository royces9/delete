#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

char *separateString(char *input){//extracts file name from the absolute path given in file
  char *tok, *token = "/";

  int i = 0, length = 0, tokenCount = 0;

  for(length = 0; input[length]; length++){
    if(input[length] == '/'){
      tokenCount++;
    }
  }
  if(tokenCount == 0){
    return input;
  }
    
  char *input2 = (char *) malloc((length+2) * sizeof(char));

  //copy input to another string because strtok destroys the original string
  strcpy(input2,input);

  char *separatedString = (char *) malloc((length) * sizeof(char));

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

int checkType(char *path){
  struct stat pathType;
  stat(path, &pathType);
  return S_ISREG(pathType.st_mode);
}

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

int rmDirContents(char *directory){
  struct dirent *d;
  DIR *dir = opendir(directory);
  int error, type;

  while((d = readdir(dir)) != NULL){
    if((strcmp(d->d_name, ".") && strcmp(d->d_name, ".."))){
      char *filePath = (char *) malloc((strlen(directory)+strlen(d->d_name)+3) * sizeof(char));
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
	  char *filePath = (char *) malloc((strlen(directory)+strlen(d->d_name)+3) * sizeof(char));
	  strcpy(filePath, directory);
	  strcat(filePath, "/");
	  strcat(filePath, d->d_name);

	  type = checkType(filePath);
	
	  char *target2 = (char *) malloc((strlen(target)+strlen(d->d_name)+3) * sizeof(char));
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

int main(int argc, char **argv){
  if(argc == 1){
    return 0;
  }

  int error, n, type;
  char *fileName;

  struct stat st;

  if(!strcmp("-empty", argv[1])){
    printf("Emptying ~/.trash, are you sure? (Y/N)\n");
    char prompt=getchar();
    if(prompt == 'Y' || prompt == 'y'){

      rmDirContents("/home/royce/.trash/");
      printf("Complete.\n");
      return 0;
    }
    else{
      printf("Cancelled.\n");
      return 1;
    }
  }

  for(int i = 1; argv[i]; i++){
    char *targetPath = (char *) malloc((strlen("/home/royce/.trash/")+strlen(argv[i])+2)*sizeof(char));
    char *fileName = separateString(argv[i]);

    strcpy(targetPath, "/home/royce/.trash/");
    strcat(targetPath, fileName);

    type = checkType(fileName);

    if(type){
      if(access(argv[i], F_OK) == -1){
	printf("File does not exist.\n");
	return -1;
      }

      rename(argv[i], targetPath);
    }

    else{
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
