#include "SingleThread_TCP.h"

SLogger& logger = SLogger::getInstance();

void InitSockets() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		logger.Log(Error, "WSAStartup failed with error: %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
}

void createServerSocket() {
	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET) {
		logger.Log(Error, "Socket creation failed with error: %d", WSAGetLastError());
		WSACleanup();
		exit(EXIT_FAILURE);
	}
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(PORT);

	if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		logger.Log(Error, "Socket binding failed with error: %d", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		exit(EXIT_FAILURE);
	}
	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
		logger.Log(Error, "Socket listening failed with error: %d", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		exit(EXIT_FAILURE);
	}
	serverEvent = WSACreateEvent();
	if (serverEvent == WSA_INVALID_EVENT) {
		logger.Log(Error, "WSACreateEvent failed with error: %d", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		exit(EXIT_FAILURE);
	}
	if (WSAEventSelect(serverSocket, serverEvent, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR) {
		logger.Log(Error, "WSAEventSelect failed with error: %d", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	logger.Log(Info, "Server is running on port %d ...", PORT);
}

void aceeptClient() {
	sockaddr_in clientAddr;
	int clientAddrSize = sizeof(clientAddr);
	SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);

	if (clientSocket == INVALID_SOCKET) {
		logger.Log(Error, "[CLIENT] Socket accept failed with error: %d\tIP: %s", WSAGetLastError());
		return;
	}
	WSAEVENT clientEvent = WSACreateEvent();
	if (clientEvent == WSA_INVALID_EVENT) {
		logger.Log(Error, "[CLIENT] WSACreateEvent failed with error: %d", WSAGetLastError());
		closesocket(clientSocket);
		WSACleanup();
		return;
	}
	if (WSAEventSelect(clientSocket, clientEvent, FD_READ | FD_CLOSE) == SOCKET_ERROR) {
		logger.Log(Error, "[CLIENT] WSAEventSelect failed with error: %d", WSAGetLastError());
		closesocket(clientSocket);
		WSACleanup();
		return;
	}
	clients[clientSocket] = new Client(clientSocket, clientEvent, clientAddr);

	logger.Log(Info, "[CLIENT] New client connected from: %s", clients[clientSocket]->getIp());
}

void receiveData(Client* client) {
	char buffer[BUFFER_SIZE];
	int bytesReceived = recv(client->socket, buffer, BUFFER_SIZE, 0);  //+1?

	if (bytesReceived > 0) {
		buffer[bytesReceived] = '\0';
		std::string message(buffer);
		if (message == "ping") {
			client->updatePingTime();
			logger.Log(Info, "[CLIENT] Ping received from: %s", client->getIp());
		}
		else if (message == "exit") {
			logger.Log(Info, "[CLIENT] Client disconnected: %s", client->getIp());
			closesocket(client->socket);
			WSACloseEvent(client->event);
			delete clients[client->socket];
			clients.erase(client->socket);
		}
		else {
			logger.Log(Info, "[CLIENT] Message received from: %s: %s", client->getIp(), message.c_str());
		}
	}
	else if (bytesReceived <= 0 || WSAGetLastError() == WSAECONNRESET) {
		logger.Log(Info, "[CLIENT] Client unexpectedly disconnected: %s", client->getIp());
		closesocket(client->socket);
		WSACloseEvent(client->event);
		delete clients[client->socket];
		clients.erase(client->socket);
	}
}

void closeClient(Client* client) {
	logger.Log(Info, "[CLIENT] Client disconnected: %s", client->getIp());
	closesocket(client->socket);
	WSACloseEvent(client->event);
	delete clients[client->socket];
	clients.erase(client->socket);
}

int main() {
	InitSockets();
	createServerSocket();

	std::vector<WSAEVENT> events = { serverEvent };

	while (true) {
		int index = WSAWaitForMultipleEvents(events.size(), events.data(), FALSE, WSA_INFINITE, FALSE);
		if (index == WSA_WAIT_FAILED) {
			logger.Log(Error, "WSAWaitForMultipleEvents failed with error: %d", WSAGetLastError());
			break;
		}

		int eventIndex = index - WSA_WAIT_EVENT_0;
		if (eventIndex == 0) {
			WSANETWORKEVENTS networkEvents;
			WSAEnumNetworkEvents(serverSocket, serverEvent, &networkEvents);

			if (networkEvents.lNetworkEvents & FD_ACCEPT) {
				aceeptClient();
				events.push_back(clients.rbegin()->second->event);
			}
			if (networkEvents.lNetworkEvents & FD_CLOSE) {
				logger.Log(Error, "Server socket closed unexpectedly");
				break;
			}
		}
		else {
			SOCKET clientSocket = INVALID_SOCKET;
			for (auto& [socket, client] : clients)
			{
				if (client->event == events[eventIndex]) {
					clientSocket = socket;
					break;
				}
			}

			if (clientSocket != INVALID_SOCKET) {
				Client* client = clients[clientSocket];
				WSANETWORKEVENTS networkEvents;
				WSAEnumNetworkEvents(clientSocket, client->event, &networkEvents);
				if (networkEvents.lNetworkEvents & FD_READ) {
					receiveData(client);
				}
				if (networkEvents.lNetworkEvents & FD_CLOSE) {
					closeClient(client);
					events.erase(events.begin() + eventIndex);
				}
			}
		}
	}

	closesocket(serverSocket);
	WSACloseEvent(serverEvent);
	WSACleanup();

	return 0;
}