#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <string>
#include <WinSock2.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

class FileManager {
public:

	std::vector<char> readFile(std::string& filename) {

		std::string relativePath = "serverstorage\\" + filename;
		std::ifstream file(relativePath, std::ios::binary);

		if (!file.is_open()) {

			std::cerr << "\n\n!!!Error opening file!!!\n\n";

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

	const std::streamsize getBufferSize() {
		return bufferSize;
	}

	void writeFile(std::string& filename, std::vector<char>& buffer, int bufferSize) {

		std::string relativePath = "serverstorage\\" + filename;
		std::ofstream file(relativePath, std::ios::binary);

		file.write(buffer.data(), bufferSize);
	}

private:
	std::streamsize bufferSize;
};


class Server {
public:

	void initializeServer() {

		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			std::cerr << "WSAStartup failed" << '\n';

			return;
		}

		serverSocket = socket(AF_INET, SOCK_STREAM, 0);

		if (serverSocket == INVALID_SOCKET)
		{
			std::cerr << "Error creating socket: " << WSAGetLastError() << '\n';
			WSACleanup();

			return;
		}

		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = INADDR_ANY;
		serverAddr.sin_port = htons(port);

		std::cout << "\nServer intialized\n";
	}


	void bindServer() {

		if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
		{
			std::cerr << "Bind failed with error: " << WSAGetLastError() << '\n';
			closesocket(serverSocket);
			WSACleanup();

			return;
		}

		std::cout << "\nServer binded\n";
	}


	void listenToConnections() {

		if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			std::cerr << "Listen failed with error: " << WSAGetLastError() << '\n';
			closesocket(serverSocket);
			WSACleanup();

			return;
		}
		std::cout << "\nServer listening on port " << port << '\n';
	}


	SOCKET acceptConnection() {

		clientSocket = accept(serverSocket, nullptr, nullptr);
		if (clientSocket == INVALID_SOCKET)
		{
			std::cerr << "Accept failed with error: " << WSAGetLastError() << '\n';
			closesocket(serverSocket);
			WSACleanup();

			return INVALID_SOCKET;
		}

		return clientSocket;
	}


	void setupAndConnect() {

		initializeServer();
		bindServer();
		listenToConnections();
		//acceptConnection();
	}


	void getFile(FileManager& fileManager, std::string& filename, SOCKET clientSocket, std::string& userpath) {

		const std::vector<char> fileData = fileManager.readFile(userpath);

		std::streamsize bufferSize = fileManager.getBufferSize();
		int bytesSent = send(clientSocket, (char*)&bufferSize, sizeof(std::streamsize), 0);

		if (WSAGetLastError() != 0) {

			std::cerr << "\n\033[31mError getting file.\033[0m\n";
			return;
		}

		send(clientSocket, fileData.data(), (int)bufferSize, 0);
	}


	void putFile(FileManager& fileManager, std::string& filename, SOCKET clientSocket, std::string& userpath) {

		std::streamsize bufferSize;
		int bytesReceived = recv(clientSocket, (char*)&bufferSize, sizeof(std::streamsize), 0);
		if (bytesReceived == 0) {

			const char* response = "\n\033[31mError creating file.\033[0m\n";
			send(clientSocket, response, (int)strlen(response), 0);

			return;
		}

		std::vector<char> buffer(bufferSize, 0);
		bytesReceived = recv(clientSocket, buffer.data(), (int)bufferSize, 0);
		if (bytesReceived > 0)
		{

			fileManager.writeFile(userpath, buffer, (int)bufferSize);

			std::string strResponse = "\n\nFile " + filename + " created.\n\n";
			auto response = strResponse.c_str();

			send(clientSocket, response, (int)strlen(response), 0);
		}
		else {

			const char* response = "\n\033[31mError creating file.\033[0m\n";
			send(clientSocket, response, (int)strlen(response), 0);
		}
	}


	void deleteFile(std::string& filename, SOCKET clientSocket, std::string& userpath) {

		std::string relativePath = "serverstorage\\" + userpath;
		std::wstring wstrRelativePath = toWideString(relativePath);

		bool isDeleted = DeleteFile(wstrRelativePath.c_str());
		if (isDeleted) {

			std::cout << "\nFile deleted successfully.\n";

			std::string strResponse = "\nFile " + filename + " deleted.\n";
			auto response = strResponse.c_str();

			send(clientSocket, response, (int)strlen(response), 0);
		}
		else {

			const char* response = "\n\033[31mError deleting file.\033[0m\n";

			send(clientSocket, response, (int)strlen(response), 0);
		}
	}


	void viewFileInfo(std::string& filename, SOCKET clientSocket, std::string& userpath) {

		BY_HANDLE_FILE_INFORMATION fileInfo = {};
		const char* errorMsg = "\nError handling file.\n";
		int errCode = 0;

		std::string relativePath = "serverstorage\\" + userpath;
		std::wstring wstrRelativePath = toWideString(relativePath);

		HANDLE fileHandle = CreateFile(wstrRelativePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (fileHandle == INVALID_HANDLE_VALUE) {

			errCode = GetLastError();
			std::cerr << "\033[31mFile handle err code: " << errCode << "\033[0m\n";
		}

		if (!GetFileInformationByHandle(fileHandle, &fileInfo)) {

			errCode = GetLastError();
			std::cerr << "\033[31mRetrieving file info err code: " << errCode << "\033[0m\n";
		}

		if (errCode != 0) {

			send(clientSocket, errorMsg, (int)strlen(errorMsg), 0);
			return;
		}

		CloseHandle(fileHandle);

		std::stringstream fileInfoStream;
		SYSTEMTIME lastAccessTime, lastWriteTime, creationTime;
		FileTimeToSystemTime(&fileInfo.ftLastAccessTime, &lastAccessTime);
		FileTimeToSystemTime(&fileInfo.ftLastWriteTime, &lastWriteTime);
		FileTimeToSystemTime(&fileInfo.ftCreationTime, &creationTime);

		fileInfoStream << "\n\nLast accessed (d/m/y): " << lastAccessTime.wDay << '/'
			<< lastAccessTime.wMonth << '/' << lastAccessTime.wYear;

		fileInfoStream << "\nLast modified (d/m/y): " << lastWriteTime.wDay << '/'
			<< lastWriteTime.wMonth << '/' << lastWriteTime.wYear;

		fileInfoStream << "\nCreated on (d/m/y): " << creationTime.wDay << '/'
			<< creationTime.wMonth << '/' << creationTime.wYear;

		LARGE_INTEGER fileSize;
		fileSize.LowPart = fileInfo.nFileSizeLow;
		fileSize.HighPart = fileInfo.nFileSizeHigh;

		float filesize = fileSize.QuadPart / 1024;

		if (filesize < 1) {

			fileInfoStream << "\nFile size: " << fileSize.QuadPart << " bytes\n";
		}
		else {

			fileInfoStream << "\nFile size: " << filesize << " kilobytes\n";
		}

		std::string fileInfoString = fileInfoStream.str();
		std::vector<char> buffer(fileInfoString.begin(), fileInfoString.end());

		send(clientSocket, buffer.data(), buffer.size(), 0);
	}


	void viewListInfo(std::string& dirname, SOCKET clientSocket, std::string& username) {

		WIN32_FIND_DATA fileData;
		std::string relativePath;

		if (dirname == "-" || dirname == "root") {

			relativePath = "serverstorage\\*";
		}
		else {
			relativePath = "serverstorage\\" + username + "\\*";
		}

		std::wstring wstrRelativePath = toWideString(relativePath);

		HANDLE hFind = FindFirstFile(wstrRelativePath.c_str(), &fileData);

		std::stringstream filenames;

		if (hFind != INVALID_HANDLE_VALUE) {

			while (FindNextFile(hFind, &fileData) != 0)
			{
				std::wstring wstrFilename(fileData.cFileName);
				std::string filename(wstrFilename.begin(), wstrFilename.end());

				filenames << filename << '\n';
			}
		}

		std::string concatenatedNames = filenames.str();
		std::vector<char> buffer(concatenatedNames.begin(), concatenatedNames.end());

		send(clientSocket, buffer.data(), buffer.size(), 0);
	}


	bool recieveData(FileManager& fileManager, SOCKET clientSocket, std::string username) {

		std::vector<char> buffer(1024, 0);
		int bytesReceived = recv(clientSocket, buffer.data(), buffer.size(), 0);
		if (bytesReceived > 0)
		{
			std::cout << '\n' << username << "\033[36m >>> \033[0m" << buffer.data() << '\n';

			std::stringstream arguments(buffer.data());
			std::string commandType, filename;
			arguments >> commandType >> filename;

			std::string userpath;
			if (filename == "root") {
				userpath = filename;
			}
			else {
				userpath = username + "\\" + filename;
			}

			if (commandType == "GET") {

				getFile(fileManager, filename, clientSocket, userpath);
				return true;
			}
			else if (commandType == "PUT") {

				putFile(fileManager, filename, clientSocket, userpath);
				return true;
			}
			else if (commandType == "DELETE") {

				deleteFile(filename, clientSocket, userpath);
				return true;
			}
			else if (commandType == "INFO") {

				viewFileInfo(filename, clientSocket, userpath);
				return true;
			}
			else if (commandType == "LIST") {

				viewListInfo(filename, clientSocket, username);
				return true;
			}
			else if (commandType == "NO") {

				return false;
			}
			else {

				std::cerr << "\nCommand not recognised.\n";
			}
		}

	}


	std::string processUser(SOCKET clientSocket) {

		std::vector<char> username(32, 0);
		recv(clientSocket, username.data(), username.size(), 0);	//no error checking btw
		username.shrink_to_fit();

		return username.data();
	}


	void createUserDirectory(std::string& username) {

		std::string userDirPath = "serverstorage\\" + username;
		std::wstring wstrUserDirPath = toWideString(userDirPath);

		auto result = CreateDirectory(wstrUserDirPath.c_str(), NULL);

		if (result == ERROR_ALREADY_EXISTS) {

			std::cerr << "\n\n\033[31mError: the directory already exists.\033[0m\n\n";
			return;
		}
		else if (result == ERROR_PATH_NOT_FOUND) {

			std::cerr << "\n\n\033[31mError: specified path not found.\033[0m\n\n";
			return;
		}

		std::cout << "\nUser folder created for: " << username << '\n';
	}


	void sendData(const char* buffer) {

		send(clientSocket, buffer, (int)strlen(buffer), 0);
	}


	~Server() {
		closesocket(clientSocket);
		closesocket(serverSocket);
		WSACleanup();
	}

private:
	WSADATA wsaData;
	int port = 12345;
	SOCKET serverSocket;
	SOCKET clientSocket;
	sockaddr_in serverAddr;

	std::wstring toWideString(std::string& string) {

		int length = MultiByteToWideChar(CP_ACP, 0, string.c_str(), -1, NULL, 0);
		std::wstring wideString(length, L'\0');
		MultiByteToWideChar(CP_ACP, 0, string.c_str(), -1, &wideString[0], length);

		return wideString;
	}
};


class Program {
public:

	void runCommLoop(Server& server, FileManager& fileManager) {

		while (true) {

			SOCKET clientSocket = server.acceptConnection();

			if (clientSocket != INVALID_SOCKET) {

				std::cout << "\n\033[32m>>> New connection accepted <<<\033[0m\n";
				threads.emplace_back(Program::handleClient, clientSocket, std::ref(fileManager), &server);
			}
		}
	}

	static void handleClient(SOCKET clientSocket, FileManager fileManager, Server* server) {

		std::string username = server->processUser(clientSocket);
		server->createUserDirectory(username);

		while (true) {

			bool result = server->recieveData(fileManager, clientSocket, username);
			if (!result) {

				break;
			}
		}
	}

private:
	std::vector<std::thread> threads;
};


int main()
{
	Server server;
	FileManager fileManager;
	Program program;

	server.setupAndConnect();

	program.runCommLoop(server, fileManager);

	return 0;
}