#include<iostream>
#include<winsock2.h>
#include<ws2tcpip.h>
#include<tchar.h>
#include<thread>
#include<cstring>
#include"json.hpp"
#include<fstream>
#include<sstream>

using namespace std;
using json = nlohmann::json;

void sendMsg(SOCKET clientSocket) {
	while (true) {
		string header = "TEXT";
		string content;
		cout << "Unesite poruku za server: " << endl;
		getline(cin, content);
		string fullMsg = header + content;
		int byteCount = send(clientSocket, fullMsg.c_str(), fullMsg.size(), 0);
		if (byteCount <= 0) {
			cout << "Slanje nije uspelo." << endl;
		}
	}
}

void sendHeartBeat(SOCKET clientSocket)
{
	json heartbeat;
	heartbeat["msgType"] = 5;
	heartbeat["sentFrom"] = "SocketClient";
	string header = "BEAT";
	string file = header + heartbeat.dump();
		int byteCount = send(clientSocket, file.c_str(), file.size(), 0);
		if (byteCount <= 0) cout << "Neuspelo slanje heartbeata" << endl;
		this_thread::sleep_for(chrono::seconds(5));

}

void recieveHeartBeat(string content, SOCKET clientSocket)
{
	try {
		json odgovorServera = json::parse(content);
		if (odgovorServera.contains("msgType") && odgovorServera["msgType"] == 5 &&
			odgovorServera.contains("sentFrom") && odgovorServera["sentFrom"] == "SocketServer") sendHeartBeat(clientSocket);
	}
	catch (...)
	{
		cout << "ne mogu da parsujem json" << endl;
	}

}

void recieveMsg(SOCKET clientSocket)
{
	char buffer[2048];
	while (true) {
		int byteCount = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
		if (byteCount > 0) {
			buffer[byteCount] = '\0';
			string msg(buffer);
			string header = msg.substr(0, 4);
			string content = msg.substr(4);

			if (header == "BEAT") {
				recieveHeartBeat(content, clientSocket);
			}
			else {
				cout << "Nepoznat tip poruke: " << header << endl;
			}
		}
	}
}



int main()
{

	SOCKET clientSocket;
	int port = 55555;
	WSADATA wsaData;
	int wsaerr;
	WORD wVersionRequested = MAKEWORD(2, 2);
	wsaerr = WSAStartup(wVersionRequested, &wsaData);
	if (wsaerr != 0) {
		cout << "The Winsock dll not found" << endl;
		return 0;
	}
	else {
		cout << "The Winsock dll found!" << endl;
		cout << "The status: " << wsaData.szSystemStatus << endl;
	}

	clientSocket = INVALID_SOCKET;
	clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET) {
		cout << "Error at socket()" << WSAGetLastError() << endl;
		WSACleanup();
		return 0;
	}
	else {
		cout << "socket() is OK" << endl;
	}

	sockaddr_in clientService;
	clientService.sin_family = AF_INET;
	InetPton(AF_INET, _TEXT("127.0.0.1"), &clientService.sin_addr.s_addr);
	clientService.sin_port = htons(port);
	if (connect(clientSocket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR) {
		cout << "Client connection() failed" << WSAGetLastError() << endl;
		WSACleanup();
		return 0;
	}
	else {
		cout << "Client connect() is OK!" << endl;
		cout << "Client can start sending and receiving data" << endl;
	}

	thread SendMsgThread(sendMsg, clientSocket);
	thread RecieveMsgThread(recieveMsg, clientSocket);
	SendMsgThread.join();
	RecieveMsgThread.join();

	system("pause");
	WSACleanup();
	return 0;
}