#pragma once

#include <fstream>
#include <iostream>
#include <stdio.h>


#include <Windows.h>
#include <wincon.h>

#define LOG_PATH "log.txt"
enum LogLevel {
	Info,
	Warning,
	Error,
	Debug
};

class SLogger {
public:
	static inline SLogger* instance_ = nullptr;
	static SLogger& getInstance(const char* logPath = LOG_PATH) {
		if (!instance_) {
			instance_ = new SLogger(logPath);
		}
		return *instance_;
	}
	std::ofstream file;
	void Log(LogLevel level, const char* format, ...) {
		SLogger& logger = SLogger::getInstance();
		file_.open(LOG_PATH, std::ios_base::app);
		if (!file_.is_open()) {
			printf("Error while opening log file!");
		}
		char message[2048];
		va_list args;
		va_start(args, format);
		vsnprintf(message, sizeof(message), format, args);
		va_end(args);
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		switch (level)
		{
		case Info:
			SetConsoleTextAttribute(hConsole, 0x08);
			printf("[INFO] %s\n", message);
			file.write("[INFO] ", strlen("[INFO] ")); file.write(message, strlen(message)); file.write(message, strlen(message));
			break;

		case Warning:
			SetConsoleTextAttribute(hConsole, 0x06);
			printf("[WARNING] %s\n", message);
			file.write("[WARNING] ", strlen("[WARNING] ")); file.write(message, strlen(message)); file.write(message, strlen(message));
			break;

		case Error:
			SetConsoleTextAttribute(hConsole, 0x04);
			printf("[ERROR] %s\n", message);
			file.write("[ERROR] ", strlen("[ERROR] ")); file.write(message, strlen(message)); file.write(message, strlen(message));
			break;

		case Debug:
			SetConsoleTextAttribute(hConsole, 0x05);
			printf("[DEBUG] %s\n", message);
			file.write("[DEBUG] ", strlen("[DEBUG] ")); file.write(message, strlen(message)); file.write(message, strlen(message));
			break;
		default:
			printf("Invalid Level");
			break;
		}
		if (file_.is_open()) {
			file_.close();
		}
	}

	~SLogger() {
		if (file_.is_open()) {
			file_.close();
		}
	}

private:
	SLogger(const char* logPath) : logPath_((char*)logPath) {
		file_.open(logPath_, std::ios_base::app);
		if (!file_.is_open()) {
			printf("Error while opening log file!");
		}
	}

	SLogger(const SLogger&) = delete;
	SLogger& operator=(const SLogger&) = delete;

	char* logPath_;
	std::ofstream file_;

};