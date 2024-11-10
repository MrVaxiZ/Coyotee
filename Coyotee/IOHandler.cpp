#include "IOHandler.h"

void IOHandler::CreateFile(const string& filename, const string& path) {
	string fullPath = path.empty() ? filename : (fs::path(path) / filename).string();

	std::ofstream file(fullPath, std::ios::app);

	if (!file) {
		printf("Error opening a file!\n");
	}
	else {
		printf("File has been created! At '%s'", fullPath.c_str());
	}
	file.close();
}

bool IOHandler::DeleteFile(const string& filePath) {
    try {
        if (fs::exists(filePath)) {
            fs::remove(filePath);
            printf("File deleted: '%s'", filePath.c_str());
            return true;
        }
        else {
            printf("File does not exist: '%s'", filePath.c_str());
            return false;
        }
    }
    catch (const fs::filesystem_error& e) {
        printf("Filesystem error: %s", e.what());
        return false;
    }
}