#ifndef IOHandler_H
#define IOHandler_H

#include <fstream>
#include <filesystem>
#include "UsingAliases.h"
#include "NamespacesAliases.h"

struct IOHandler {
	void CreateFile(const string& filename, const string& path);
	bool DeleteFile(const string& filePath);
};

#endif // IOHandler_H

