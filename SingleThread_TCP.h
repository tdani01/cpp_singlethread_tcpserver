#include <WinSock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <vector>
#include <chrono>
#include <map>
#pragma comment(lib, "Ws2_32.lib")

#include "Logger.cpp"

#define PORT 8080
#define BUFFER_SIZE 1024
#define unix_timestamp std::chrono::seconds(std::time(NULL))
#define WM_SOCKET (WM_USER+1)

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;

static int _connection_id = 0;
class Client {
public:
	int connection_id;
	const char* ip;
	SOCKET socket;
	sockaddr_in clientAddr;
	WSAEVENT event;
	//int playerID;
	TimePoint connectTime;
	TimePoint lastPing;

	Client(SOCKET sock, WSAEVENT evt, sockaddr_in address) : socket(sock), clientAddr(address), event(evt), connection_id(_connection_id), ip(nullptr), connectTime(Clock::now()), lastPing(connectTime) {
		_connection_id++;
		connectTime = Clock::now();
		lastPing = connectTime;
	}


	double getDuration(TimePoint from) const {
		auto now = Clock::now();
		return std::chrono::duration_cast<std::chrono::seconds>(now - from).count();
	}

	void updatePingTime() { lastPing = Clock::now(); }

	const char* getIp() const {
		char str[INET_ADDRSTRLEN];
		return inet_ntop(AF_INET, &this->clientAddr.sin_addr, str, INET_ADDRSTRLEN);
	}
};

std::map<SOCKET, Client*> clients;
SOCKET serverSocket;
WSAEVENT serverEvent;