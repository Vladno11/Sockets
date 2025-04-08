	#include<iostream>
	#include<winsock2.h>
	#include<ws2tcpip.h>
	#include<tchar.h>
	#include<cstring>
	#include<string>
	#include<thread>
	#include<chrono>
	#include"json.hpp"
	#include<fstream>
	#include<sstream>
	#include<mutex>

	using namespace std;
	using json = nlohmann::json;

	void sendHeartBeat(SOCKET acceptSocket)
	{
		json heartbeat;
		heartbeat["msgType"] = 5;
		heartbeat["sentFrom"] = "SocketServer";
		string header = "BEAT";
		string file = header + heartbeat.dump();
		while (TRUE) {
			int byteCount = send(acceptSocket, file.c_str(), file.size(), 0);
			if (byteCount <= 0) cout << "Neuspelo slanje heartbeata" << endl;
			this_thread::sleep_for(chrono::seconds(30));
		}

	}

	void recieveHeartBeat(string content, SOCKET acceptSocket)
	{
		static int brojPokusaja = 0;
			try {
				json odgovorKlijenta = json::parse(content);
				if (odgovorKlijenta.contains("msgType") && odgovorKlijenta["msgType"] == 5 &&
					odgovorKlijenta.contains("sentFrom") && odgovorKlijenta["sentFrom"] == "SocketClient") brojPokusaja = 0;
				else brojPokusaja++;
			}
			catch (...)
			{
				cout << "ne mogu da parsujem json" << endl;
				brojPokusaja++;
			}
			if (brojPokusaja == 3) closesocket(acceptSocket);

	}

	void sendMsg(SOCKET acceptSocket, const string& header, const string& content) {
		string fullMsg = header + content;
		int byteCount = send(acceptSocket, fullMsg.c_str(), fullMsg.size(), 0);
		if (byteCount <= 0) {
			cout << "Slanje nije uspelo." << endl;
		}
	}

	void recieveMsg(SOCKET acceptSocket) {
		char buffer[2048];
		while (true) {
			int byteCount = recv(acceptSocket, buffer, sizeof(buffer) - 1, 0);
			if (byteCount > 0) {
				buffer[byteCount] = '\0';
				string msg(buffer);
				string header = msg.substr(0, 4);
				string content = msg.substr(4);

				if (header == "TEXT") {
					cout << "Primljen tekst: " << content << endl;
				}
				else if (header == "BEAT")
				{
					recieveHeartBeat(content, acceptSocket);
				}
				else {
					cout << "Nepoznat tip poruke: " << header << endl;
				}
			}
		}
	}



	int main()
	{
		SOCKET serverSocket, acceptSocket;
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
			cout << "Winsock dll found" << endl;
			cout << "The status:" << wsaData.szSystemStatus << endl;
		}

		serverSocket = INVALID_SOCKET;
		serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (serverSocket == INVALID_SOCKET) {
			cout << "Error at socket()" << endl;
			WSACleanup();
			return 0;
		}
		else {
			cout << "socket() is OK!" << endl;
		}

		sockaddr_in service;
		service.sin_family = AF_INET;
		InetPton(AF_INET, _T("127.0.0.1"), &service.sin_addr.s_addr);
		service.sin_port = htons(port);
		if (::bind(serverSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
			cout << "bind() failed: " << WSAGetLastError() << endl;
			closesocket(serverSocket);
			WSACleanup();
			return 0;
		}
		else {
			cout << "bind() is OK!" << endl;
		}

		if (listen(serverSocket, 2) == SOCKET_ERROR)
			cout << "listen(): Error listening on socket" << WSAGetLastError() << endl;
		else
			cout << "listen() is OK, I'm waiting for connections..." << endl;

		while (true)
		{
			acceptSocket = accept(serverSocket, NULL, NULL);
			if (acceptSocket == INVALID_SOCKET) {
				cout << "accept() failed" << WSAGetLastError() << endl;
				WSACleanup();
				return -1;
			}
			else {
					thread HeartBeatThread(sendHeartBeat, acceptSocket);
					thread RecieveMsgThread(recieveMsg, acceptSocket);
					HeartBeatThread.detach();
					RecieveMsgThread.detach();
					
			}
		}
		system("pause");
		WSACleanup();
		return 0;
	}
