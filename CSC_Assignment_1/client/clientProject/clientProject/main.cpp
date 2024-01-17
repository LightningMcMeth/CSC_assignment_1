#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <string>
#include <WinSock2.h>
#include <Ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

class FileManager {
public:

	std::vector<char> readFile(std::string& filename) {

		std::ifstream file("clientfiles\\" + filename, std::ios::binary);

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

		std::ofstream file("clientfiles\\" + filename, std::ios::binary);

		file.write(buffer.data(), bufferSize);
	}

private:
	std::streamsize bufferSize;
};


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


	void sendData(const char* message) {

		send(clientSocket, message, (int)strlen(message), 0);
	}


	void putFile(FileManager& fileManager, std::string& filename) {

		const std::vector<char> fileData = fileManager.readFile(filename);

		auto bufferSize = fileManager.getBufferSize();
		send(clientSocket, (char*)&bufferSize, sizeof(std::streamsize), 0);

		send(clientSocket, fileData.data(), (int)bufferSize, 0);
	}


	void receiveData(std::string& filename, FileManager& fileManager) {

		std::streamsize bufferSize;
		recv(clientSocket, (char*)&bufferSize, sizeof(std::streamsize), 0);

		std::vector<char> buffer(bufferSize, 0);

		int bytesReceived = recv(clientSocket, buffer.data(), (int)bufferSize, 0);
		if (bytesReceived > 0)
		{

			fileManager.writeFile(filename, buffer, (int)bufferSize);

			std::cout << "response recieved!\n";
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

	void runCommLoop(ClientServer& server, FileManager& fileManager) {

		while (true) {

			std::string userMsg = processInput();
			const char* response = userMsg.c_str();

			server.sendData(response);

			if (commandType == "PUT") {		//refactor this mess
				
				server.putFile(fileManager, filename);
			}
			else {
				server.receiveData(filename, fileManager);
			}
		}
	}

	std::string processInput() {	//refactor this entire mess

		std::cout << "\nEnter a command: ";
		std::cin >> commandType;

		if (commandType == "MSG") {

			std::string userMessage = "";

			std::cin.ignore();
			std::cout << "Send a message: ";
			std::getline(std::cin, userMessage);

			return userMessage;
		}
		else if (commandType == "GET") {

			std::cin.ignore();
			std::cout << "\nEnter filepath: ";
			std::cin >> filename;

			return commandType + " " + filename;

		}
		else if (commandType == "PUT") {

			std::cin.ignore();
			std::cout << "\nEnter filepath: ";
			std::cin >> filename;

			return commandType + " " + filename;
		}
		else {

			std::cerr << "\n\n!!!Command not recognised!!!\n\n";

			return "";
		}
	}


	std::string enterMessage() {

		std::string userMessage;

		std::cout << "\n\nEnter message: ";
		std::getline(std::cin, userMessage);

	}

	const std::string getFilename() {
		return filename;
	}


private:
	std::string commandType;
	std::string filename;
};

int main()	//clientfiles
{	
	ClientServer server(L"127.0.0.1");
	UI UI;
	FileManager fileManager;

	server.connectServer();

	UI.runCommLoop(server, fileManager);
	
	return 0;
}

//const char* response = UI.processInput().c_str();   <--- this line creates a dangling pointer error. Why???