#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <string>
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include "../../src/FileManager.h"

#pragma comment(lib, "ws2_32.lib")

class ClientServer {
public:

	ClientServer(PCWSTR Ip)
		: serverIp(Ip) {}


	void connectServer() {

		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			std::cerr << "WSAStartup failed" << '\n';

			return;
		}

		clientSocket = socket(AF_INET, SOCK_STREAM, 0);

		if (clientSocket == INVALID_SOCKET)
		{
			std::cerr << "Error creating socket: " << WSAGetLastError() << '\n';
			WSACleanup();

			return;
		}

		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(port);
		InetPton(AF_INET, serverIp, &serverAddr.sin_addr);


		if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
		{
			std::cerr << "Connect failed with error: " << WSAGetLastError() << '\n';
			closesocket(clientSocket);
			WSACleanup();

			return;
		}

	}

	void connectToServer() {

		if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
		{
			std::cerr << "Connect failed with error: " << WSAGetLastError() << '\n';
			closesocket(clientSocket);
			WSACleanup();

			return;
		}
	}


	void sendArgs(const char* message) {

		send(clientSocket, message, (int)strlen(message), 0);
	}

	/*
	void putFile(FileManager& fileManager, std::string& filename) {

		const std::vector<char> fileData = fileManager.readFile(filename);

		auto bufferSize = fileManager.getBufferSize();
		send(clientSocket, (char*)&bufferSize, sizeof(std::streamsize), 0);

		send(clientSocket, fileData.data(), (int)bufferSize, 0);

		std::vector<char> confirmationBuffer(128, 0);
		int bytesReceived = recv(clientSocket, confirmationBuffer.data(), confirmationBuffer.size(), 0);
		if (bytesReceived > 0) {

			std::cout << confirmationBuffer.data();
		}
	}

	/*
	void receiveData(std::string& filename, FileManager& fileManager) {

		std::streamsize bufferSize;
		int bytesReceived = recv(clientSocket, (char*)&bufferSize, sizeof(std::streamsize), 0);
		std::cout << "\n\nBytes received during GET: " << bytesReceived << "\n\n";
		if (bytesReceived <= 8) {	//you should always recieve 8 bytes (at least). If I start getting weird errors it's probably because of this check

			std::cerr << "\nError getting file.\n";
			return;
		}

		std::vector<char> buffer(bufferSize, 0);

		bytesReceived = recv(clientSocket, buffer.data(), (int)bufferSize, 0);
		if (bytesReceived > 0)
		{

			fileManager.writeFile(filename, buffer, (int)bufferSize);

			std::cout << "File created.\n";
		}
	}*/

	//contains chunk based logic
	void getFile(std::string& filename, FileManager& fileManager) {

		std::streamsize fileSize;
		recv(clientSocket, (char*)&fileSize, sizeof(std::streamsize), 0);

		std::streamsize bufferSize;
		recv(clientSocket, (char*)&bufferSize, sizeof(std::streamsize), 0);

		std::string relativePath = "clientfiles\\" + filename;
		std::ofstream file(relativePath, std::ios::binary);

		std::vector<char> buffer(bufferSize, 0);
		std::streamsize totalReceivedSize = 0;
		while (totalReceivedSize < fileSize) {

			std::streamsize bytesReceived = recv(clientSocket, buffer.data(), buffer.size(), 0);
			if (bytesReceived <= 0) {
				std::cerr << "\n\nMajor bruh moment right here\n\n";
			}
			file.write(buffer.data(), bytesReceived);

			totalReceivedSize += bytesReceived;
		}

		file.close();
	}

	void putFile(std::string& userpath) {

		std::string relativePath = "clientfiles\\" + userpath;
		std::ifstream file(relativePath, std::ios::binary);
		if (!file.is_open()) {
			std::cerr << "\n\n\033[31m!!!Error opening file!!!\033[0m\n\n";
			return;
		}

		file.seekg(0, std::ios::end);
		std::streamsize fileSize = file.tellg();
		file.seekg(0, std::ios::beg);


		send(clientSocket, (char*)&fileSize, sizeof(std::streamsize), 0);

		std::streamsize bufferSize = 4196;
		send(clientSocket, (char*)&bufferSize, sizeof(std::streamsize), 0);

		std::streamsize lastChunk = fileSize % bufferSize;
		std::vector<char> buffer(bufferSize, 0);
		std::streamsize totalSentSize = 0;

		while (totalSentSize < fileSize)
		{
			std::streamsize remainingSize = fileSize - totalSentSize;
			std::streamsize currentChunkSize = (remainingSize < bufferSize) ? remainingSize : lastChunk;
			//std::streamsize currentChunkSize = min(remainingSize, bufferSize);

			file.read(buffer.data(), currentChunkSize);
			int bytesSent = send(clientSocket, buffer.data(), (int)currentChunkSize, 0);
			if (bytesSent == SOCKET_ERROR) {

				std::cerr << "\n\nMajor bruh moment right here\n\n";
			}

			totalSentSize += bytesSent;
		}

		file.close();
	}

	void recieveMessage() {

		std::vector<char> buffer(128, 0);
		recv(clientSocket, buffer.data(), buffer.size(), 0);

		std::cout << buffer.data();

	}

	void recieveFileInfo() {

		std::vector<char> buffer(256, 0);

		int bytesReceived = recv(clientSocket, buffer.data(), buffer.size(), 0);
		if (bytesReceived > 0) {

			std::cout << buffer.data();
		}
		else {

			std::cout << "\nFile not found.\n";
		}
	}

	void recieveListInfo() {

		std::vector<char> buffer(1024, 0);
		recv(clientSocket, buffer.data(), buffer.size(), 0);

		std::cout << '\n' << buffer.data() << "\n";
	}

	SOCKET& getSocket() {
		return clientSocket;
	}

	sockaddr_in getAddress() {
		return serverAddr;
	}

	~ClientServer() {
		closesocket(clientSocket);
		WSACleanup();
	}

private:
	WSADATA wsaData;
	PCWSTR serverIp;
	SOCKET clientSocket;
	sockaddr_in serverAddr;
	int port = 12345;

};

class UI {
public:

	void runCommLoop(ClientServer& server, FileManager& fileManager) {

		std::cout << "\nEnter username: ";
		std::cin >> name;
		server.sendArgs(name.c_str());

		while (true) {

			std::cout << "\nCMD \033[36m>>>\033[0m:";
			std::cin >> commandType;

			std::cin.ignore();
			std::cout << "\nPATH \033[36m>>>\033[0m:";
			std::cin >> filename;

			std::string userMsg = commandType + " " + filename;
			const char* response = userMsg.c_str();

			server.sendArgs(response);

			if (commandType == "PUT") {

				server.putFile(filename);
			}
			else if (commandType == "GET") {

				//server.receiveData(filename, fileManager);
				server.getFile(filename, fileManager);
			}
			else if (commandType == "DELETE") {

				server.recieveMessage();
			}
			else if (commandType == "INFO") {

				server.recieveFileInfo();
			}
			else if (commandType == "LIST") {

				server.recieveListInfo();
			}
		}
	}

	const std::string getFilename() {
		return filename;
	}

private:
	std::string commandType;
	std::string filename;
	std::string name;
};

int main()
{
	ClientServer server(L"127.0.0.1");
	UI UI;
	FileManager fileManager;

	server.connectServer();

	UI.runCommLoop(server, fileManager);

	return 0;
}