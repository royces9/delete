#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

//string value is set during compilation via -D flag
char const *const trash_path = TRASH_PATH;

//extracts file name from the absolute path given in file
char *get_file_name(char *input, char delimiter, int *out_len) {
	int length = strlen(input);
	char *out = &input[length - 1];

	while((length > 0) && (*out == delimiter)) {
		--out;
		--length;
	}

	while((length > 1) && (*out != delimiter)) {
		--out;
		--length;
		++(*out_len);
	}

	return out;
}


//given two strings, concatenate them and separate them by a '/'
//two strings are generally directories/files
char *concat_dir(char const *const aa, int a_len, char const *const bb, int b_len, int *out_len) {
	*out_len = a_len + b_len + 2;

	char *out = malloc(*out_len * sizeof(*out));
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

	while((d = readdir(dir)) != NULL) {
		if((strcmp(d->d_name, ".") && strcmp(d->d_name, ".."))) {
			int d_name_len = strlen(d->d_name);
			char *filePath = concat_dir(directory, dir_len, d->d_name, d_name_len, NULL);

			if(checkType(filePath)) {
				if(unlink(filePath) == -1)
					break;
			} else {
				rmDirContents(filePath);
				rmdir(filePath);
			}

			free(filePath);
		}
	}

	closedir(dir);
}


int exists(char *input) {
	return access(input, F_OK) != -1;
}

int append(char **input, int len) {
	char *out = realloc(*input, (len + 2) * sizeof(*out));
	if(!out)
		return 1;

	out[len + 1] = 0;
	int i = len - 1;
	for(; out[i] == '/'; --i)
		out[i] = 0;

	out[i + 1] = '_';
	*input = out;
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

	int const trash_len = strlen(trash_path);

	//loop through all given arguments
	//should be names of files/directories
	for(int i = 1; argv[i]; ++i) {

		if(!exists(argv[i])) {
			printf("%s does not exist.\n", argv[i]);
			continue;
		}

		//fileName: the name of just the file that is going to be deleted
		int file_len = 0;
		char *fileName = get_file_name(argv[i], '/', &file_len);

		//targetPath: the path to the file in the trash directory
		int target_len = 0;
		char *targetPath = concat_dir(trash_path, trash_len, fileName, file_len, &target_len);
		if(!targetPath)
			break;
  
		//check if file exists in .trash
		//if it does, change the name
		while(exists(targetPath)) {
			if(append(&targetPath, target_len))
				return 1;
			++target_len;
		}

		//move file to trash
		if(rename(argv[i], targetPath))
			break;
		
		free(targetPath);
	}

	return 0;

}
