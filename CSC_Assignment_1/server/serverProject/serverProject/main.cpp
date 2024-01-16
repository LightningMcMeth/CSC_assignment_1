#include <iostream>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

class Server {
public:

	void initializeServer() {

		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			std::cerr << "WSAStartup failed" << std::endl;

			return;
		}

		serverSocket = socket(AF_INET, SOCK_STREAM, 0);

		if (serverSocket == INVALID_SOCKET)
		{
			std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
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
			std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
			closesocket(serverSocket);
			WSACleanup();

			return;
		}

		std::cout << "\nServer binded\n";
	}


	void listenToConnections() {

		if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			std::cerr << "Listen failed with error: " << WSAGetLastError() << std::endl;
			closesocket(serverSocket);
			WSACleanup();

			return;
		}
		std::cout << "Server listening on port " << port << std::endl;
	}


	void acceptConnection() {

		clientSocket = accept(serverSocket, nullptr, nullptr);
		if (clientSocket == INVALID_SOCKET)
		{
			std::cerr << "Accept failed with error: " << WSAGetLastError() << std::endl;
			closesocket(serverSocket);
			WSACleanup();

			return;
		}
	}


	void recieveData() {

		char buffer[1024];
		memset(buffer, 0, 1024);
		int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (bytesReceived > 0)
		{
			std::cout << "Received data: " << buffer << std::endl;

			const char* response = "Hello, client! This is the server.";
			send(clientSocket, response, (int)strlen(response), 0);
		}

	}


	void sendData(){
	
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
	
	server.initializeServer();
	server.bindServer();
	
	server.listenToConnections();
	server.acceptConnection();

	server.recieveData();

	return 0;
}