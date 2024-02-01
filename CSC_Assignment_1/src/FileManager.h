#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>
#include <vector>

class FileManager {
public:
    std::vector<char> readFile(std::string& filename);
    const std::streamsize getBufferSize();
    void writeFile(std::string& filename, std::vector<char>& buffer, int bufferSize);

private:
    std::streamsize bufferSize;
};

#endif