#include <iostream>
#include <cstring>
#include <string>
#include <WinSock2.h>
#include <Ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

class ClientServer {
public:

	ClientServer(PCWSTR Ip) 
		: serverIp(Ip) {}


	void connectServer() {

		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			std::cerr << "WSAStartup failed" << std::endl;
			return;
		}

		clientSocket = socket(AF_INET, SOCK_STREAM, 0);

		if (clientSocket == INVALID_SOCKET)
		{
			std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
			WSACleanup();
			return;
		}

		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(port);
		InetPton(AF_INET, serverIp, &serverAddr.sin_addr);


		if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
		{
			std::cerr << "Connect failed with error: " << WSAGetLastError() << std::endl;
			closesocket(clientSocket);
			WSACleanup();

			return;
		}

	}


	void sendData(const char* message) {

		send(clientSocket, message, (int)strlen(message), 0);
	}

	void receiveData() {

		char buffer[1024];
		memset(buffer, 0, 1024);

		int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (bytesReceived > 0)
		{
			std::cout << "Received from server: " << buffer << std::endl;
		}
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

	std::string processInput() {

		std::cout << '\n' << "Enter a command: ";

		std::cin >> commandType >> filename;

		if (commandType == "MSG") {

			std::string userMessage = "";

			std::cin.ignore();
			std::cout << "Send a message: ";
			std::getline(std::cin, userMessage);

			return userMessage;
		}
		else {

			return "";
		}
	}

	std::string enterMessage() {

		std::string userMessage;

		std::cout << "\n\nEnter message: ";
		std::getline(std::cin, userMessage);

	}


private:
	std::string commandType;
	std::string filename;
};

int main()
{	
	ClientServer server(L"127.0.0.1");
	UI UI;
	std::string commandType, filename, userMessage;

	server.connectServer();

	std::string userMsg = UI.processInput();
	const char* response = userMsg.c_str();

	server.sendData(response);

	server.receiveData();
	
	return 0;
}
