#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

#include "config.hpp"

int main(int argc, char **argv) {
	if(argc == 1)
		return 0;

	//if there is an argument to empty trash
	if(!strcmp("-empty", argv[1])) {
		for(auto &di : std::filesystem::directory_iterator(trash_dir)) {
			if(!std::filesystem::remove_all(di))
				std::cout << "Error removing: " << di << "\n";
		}

		return 0;
	}

	std::filesystem::path cwd = std::filesystem::current_path();

	//loop through all given arguments
	//should be names of files/directories
	for(int i = 1; argv[i]; i++) {
		std::filesystem::path file = argv[i];

		if(!std::filesystem::exists(file)) {
			std::cout << file << " does not exist.\n";
			continue;
		}


		std::filesystem::path target = trash_dir / file.filename();

		while(std::filesystem::exists(target)) {
			target += "_";
		}

		std::filesystem::rename(cwd / file, target);
	}

	return 0;

}
