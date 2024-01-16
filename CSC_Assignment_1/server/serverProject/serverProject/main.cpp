#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <string>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

class FileManager {
public:

	const char* readFile(std::string& filename) {

		std::ifstream file(filename);

		if (!file.is_open()) {

			std::cerr << "\n\n!!!Error opening file!!!\n\n";
			return "";
		}

		char buffer[1024];
		memset(buffer, 0, 1024);

		std::string line;

		int i = 0;
		while (getline(file, line)) {

			for (char& ch : line) {

				buffer[i] = ch;
				i++;
			}
		}

		file.close();

		return buffer;
	}

private:
	std::string filename;
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
		std::cout << "Server listening on port " << port << '\n';
	}


	void acceptConnection() {

		clientSocket = accept(serverSocket, nullptr, nullptr);
		if (clientSocket == INVALID_SOCKET)
		{
			std::cerr << "Accept failed with error: " << WSAGetLastError() << '\n';
			closesocket(serverSocket);
			WSACleanup();

			return;
		}
	}


	void recieveData(FileManager& fileManager) {

		char buffer[1024];
		memset(buffer, 0, 1024);
		int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (bytesReceived > 0)
		{
			std::cout << "Received data: " << buffer << '\n';

			std::stringstream arguments(buffer);
			std::string commandType, filename;	//might move these to class variables since they will constantly be in use

			arguments >> commandType >> filename;

			if (commandType == "GET") {

				const char* gamer = fileManager.readFile(filename);
				send(clientSocket, gamer, (int)strlen(gamer), 0);
			}

			const char* response = "\nRoger that\n";
			send(clientSocket, response, (int)strlen(response), 0);
		}

	}


	void sendData(const char* buffer){
		
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
};


int main()
{
	Server server;
	FileManager fileManager;
	
	server.initializeServer();
	server.bindServer();
	server.listenToConnections();
	server.acceptConnection();
	
	while (true) {

		server.recieveData(fileManager);
	}

	//server.recieveData();

	return 0;
}