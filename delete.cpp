#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
static const fs::path trash_dir = TRASH_PATH;

int main(int argc, char **argv) {
	if(argc == 1)
		return 0;

	//if there is an argument to empty trash
	if(!strcmp("-empty", argv[1])) {
		for(auto &di : fs::directory_iterator(trash_dir))
			if(!fs::remove_all(di))
				std::cout << "Error removing: " << di << "\n";

		return 0;
	}

	auto cwd = fs::current_path();

	//loop through all given arguments
	//should be names of files/directories
	for(int i = 1; argv[i]; ++i) {
		fs::path file = argv[i];

		if(!fs::exists(file)) {
			std::cout << file << " does not exist.\n";
			continue;
		}

		auto target = trash_dir / file.filename();
		
		while(fs::exists(target))
			target += "_";

		fs::rename(cwd / file, target);
	}

	return 0;

}
