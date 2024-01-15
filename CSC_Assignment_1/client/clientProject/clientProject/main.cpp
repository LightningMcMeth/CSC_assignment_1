#include <iostream>
#include <cstring>
#include <string>
#include <WinSock2.h>
#include <Ws2tcpip.h>
// Linking the library needed for network communication
#pragma comment(lib, "ws2_32.lib")

class ClientServer {
public:

	ClientServer(PCWSTR Ip) 
		: serverIp(Ip) {}

	int connectServer() {

		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			std::cerr << "WSAStartup failed" << std::endl;
			return 1;
		}

		clientSocket = socket(AF_INET, SOCK_STREAM, 0);

		if (clientSocket == INVALID_SOCKET)
		{
			std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
			WSACleanup();
			return 1;
		}

		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(port);
		InetPton(AF_INET, serverIp, &serverAddr.sin_addr);


		if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
		{
			std::cerr << "Connect failed with error: " << WSAGetLastError() << std::endl;
			closesocket(clientSocket);
			WSACleanup();

			return 1;
		}

	}

	void sendMessage(const char* message) {

		send(clientSocket, message, (int)strlen(message), 0);
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

int main()
{	
	//GAMER
	ClientServer server(L"127.0.0.1");
	server.connectServer();
	
	std::string userMessage = "cheese";

	std::cout << "Send a message: ";
	std::getline(std::cin, userMessage);

	server.sendMessage(userMessage.c_str());

	// Receive the response from the server
	char buffer[1024];
	memset(buffer, 0, 1024);
	int bytesReceived = recv(server.getSocket(), buffer, sizeof(buffer), 0);
	if (bytesReceived > 0)
	{
		std::cout << "Received from server: " << buffer << std::endl;
	}
	// Clean up
	
	return 0;
}
