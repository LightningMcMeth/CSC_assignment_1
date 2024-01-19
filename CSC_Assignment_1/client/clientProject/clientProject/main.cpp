#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <string>
#include <WinSock2.h>
#include <Ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

class FileManager {	//C:\\Users\\markh\\OneDrive\\Documents\Gamer repositories\CSC_ASSIGNMENTS\CSC_assignment_1\CSC_Assignment_1\client\clientProject\clientProject\clientfiles
public:

	std::vector<char> readFile(std::string& filename) {

		std::string relativePath = "clientfiles\\" + filename;
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

		std::string relativePath = "clientfiles\\" + filename;
		std::ofstream file(relativePath, std::ios::binary);

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


	void sendArgs(const char* message) {

		send(clientSocket, message, (int)strlen(message), 0);
	}


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


	void receiveData(std::string& filename, FileManager& fileManager) {

		std::streamsize bufferSize;
		int bytesReceived = recv(clientSocket, (char*)&bufferSize, sizeof(std::streamsize), 0);
		if (bytesReceived == 0) {	//it always sends 8 bytes. I haven't figured out why yet.

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
	}

	void recieveMessage() {

		std::vector<char> buffer(128, 0);
		recv(clientSocket, buffer.data(), buffer.size(), 0);

		std::cout << buffer.data();

	}

	void recieveFileInfo() {

		std::vector<char> buffer(256, 0); //really don't think I need a bigger buffer here, it's just info about a file.

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

		while (true) {

			std::cout << "\nCMD >>>: ";
			std::cin >> commandType;

			std::cin.ignore();
			std::cout << "\nPATH >>>: ";
			std::cin >> filename;

			std::string userMsg = commandType + " " + filename;
			const char* response = userMsg.c_str();

			server.sendArgs(response);

			if (commandType == "PUT") {

				server.putFile(fileManager, filename);
			}
			else if (commandType == "GET") {

				server.receiveData(filename, fileManager);
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

//const char* response = UI.processInput().c_str();   <--- this line creates a dangling pointer error. Why???