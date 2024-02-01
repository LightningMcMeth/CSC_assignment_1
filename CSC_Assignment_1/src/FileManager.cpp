#include "FileManager.h"
#include <fstream>
#include <iostream>

std::vector<char> FileManager::readFile(std::string& filename) {
    
	std::string relativePath = "clientfiles\\" + filename;
	std::ifstream file(relativePath, std::ios::binary);

	if (!file.is_open()) {

		std::cerr << "\n\n\033[31m!!!Error opening file!!!\033[0m\n\n";

		std::vector<char> empty;
		return empty;
	}

	file.seekg(0, std::ios::end);
	bufferSize = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(bufferSize, 0);

	file.read(buffer.data(), bufferSize);
	file.close();

	return buffer;
}

const std::streamsize FileManager::getBufferSize() {
	return bufferSize;
}

void FileManager::writeFile(std::string& filename, std::vector<char>& buffer, int bufferSize) {

	std::string relativePath = "clientfiles\\" + filename;
	std::ofstream file(relativePath, std::ios::binary);

	file.write(buffer.data(), bufferSize);
}
