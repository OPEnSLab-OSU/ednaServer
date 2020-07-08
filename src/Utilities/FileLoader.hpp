#pragma once
#include <KPFoundation.hpp>
#include <SD.h>

class FileLoader {
public:
	bool createDirectoryIfNeeded(const char * dir) {
		File folder = SD.open(dir, FILE_READ);
		if (folder) {
			if (folder.isDirectory()) {
				folder.close();
				return true;
			}

			raise(Error("FileLoader: path already exists but "
						"not directory please remove the file"));
		}

		// folder doesn't exist
		print("FileLoader: directory doesn't exist. Creating...");
		bool success = SD.mkdir(dir);
		println(success ? "success" : "failed");
		return success;
	}
};