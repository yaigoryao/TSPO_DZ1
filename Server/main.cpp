#define NOMINMAX
#include <iostream>

#pragma once
#include <iostream>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <stdio.h>
#include <thread>
#include <map>
#include <mutex>
#include <set>
#include <fstream>
#include <algorithm>
#include <string>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>


#define DEFAULT_PORT "33555"

#pragma comment(lib, "Ws2_32.lib")
class NoSockServer
{
private:
	static const int MinorVersion = 2;
	static const int MajorVersion = 2;
	static const int BufferSize = 512;

	WSADATA _wsaData;

	struct addrinfo* _result = nullptr;
	struct addrinfo _hints;

	SOCKET _listenSocket = INVALID_SOCKET;

	bool _initSuccessful = false;

	std::set<SOCKET> _clientSockets;
	std::mutex _mutex;
	std::atomic<bool> _stopMainLoop = false;

	std::thread _mainThread;

	bool initFields()
	{
		this->_clientSockets = std::set<SOCKET>();
		return true;
	}

	/*bool initWinsock()
	{
		int status = WSAStartup(MAKEWORD(MajorVersion, MinorVersion), &_wsaData);
		if (status != 0)
		{
			std::cout << "WSAStartup ошибка: " << status << std::endl;
			return false;
		}
		std::cout << "WSA инициализирован" << std::endl;
		return true;
	}*/

	bool initAddrInfo()
	{
		ZeroMemory(&_hints, sizeof(_hints));
		_hints.ai_family = AF_INET;
		_hints.ai_socktype = SOCK_DGRAM;
		_hints.ai_flags = AI_PASSIVE;
		_hints.ai_protocol = 0;
		_hints.ai_canonname = NULL;
		_hints.ai_addr = NULL;
		_hints.ai_next = NULL;
		int status = getaddrinfo("0.0.0.0", DEFAULT_PORT, &_hints, &_result);
		if (status != 0)
		{
			std::cout << "getaddrinfo ошибка: " << status << std::endl;
			WSACleanup();
			return false;
		}
		std::cout << "Инициализация сервера с портом " << DEFAULT_PORT << "..." << std::endl;

		return true;
	}

	bool initSocket()
	{
		_listenSocket = socket(_result->ai_family, _result->ai_socktype, _result->ai_protocol);
		if (_listenSocket == INVALID_SOCKET)
		{
			std::cout << "Ошибка воздания сокета socket(): " << WSAGetLastError() << std::endl;
			freeaddrinfo(_result);
			WSACleanup();
			return false;
		}
		std::cout << "Серверный сокет успешно инициализирован..." << std::endl;

		return true;
	}

	bool bindSocket()
	{
		int status = bind(_listenSocket, _result->ai_addr, (int)_result->ai_addrlen);
		if (status == SOCKET_ERROR)
		{
			std::cout << "Ошибка привязки сокета: " << WSAGetLastError() << std::endl;
			freeaddrinfo(_result);
			closesocket(_listenSocket);
			WSACleanup();
			return false;
		}
		freeaddrinfo(_result);
		std::cout << "Сокет связан с портом..." << std::endl;

		return true;
	}

	/*bool startListen()
	{
		if (listen(_listenSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			std::cout << "Прослушка сокета прервалась: " << WSAGetLastError() << std::endl;
			closesocket(_listenSocket);
			WSACleanup();
			return false;
		}
		std::cout << "Сокет начал прослушивание входящих соединений" << std::endl;

		return true;
	}*/

	bool disconnectSocket()
	{
		_stopMainLoop = true;
		if (closesocket(_listenSocket) != 0)
		{
			std::cout << "Ошибка закрытия сокета closesocket(): " << WSAGetLastError() << std::endl;
			return false;
		}
		std::lock_guard<std::mutex> lock(_mutex);
		for (SOCKET clientSocket : _clientSockets)
		{
			std::string shutdownMessage = "Остановка сервера...";
			send(clientSocket, shutdownMessage.c_str(), shutdownMessage.length(), 0);
			closesocket(clientSocket);
		}
		_clientSockets.clear();
		WSACleanup();
		std::cout << "Завершение работы..." << std::endl;

		return true;
	}

public:
	NoSockServer()
	{
		init();
	}

	NoSockServer(const NoSockServer&) = delete;
	NoSockServer& operator=(const NoSockServer&) = delete;

	NoSockServer(NoSockServer&&) noexcept = default;
	NoSockServer& operator=(NoSockServer&&) noexcept = default;

	void threadLoop()
	{
		while (!_stopMainLoop)
		{
			fd_set readSet;
			FD_ZERO(&readSet);
			FD_SET(_listenSocket, &readSet);

			struct timeval timeout;
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;

			int result = select(0, &readSet, NULL, NULL, &timeout);

			if (_stopMainLoop) break;

			if (result > 0)
			{
				if (FD_ISSET(_listenSocket, &readSet))
				{
					SOCKET clientSocket = accept(_listenSocket, NULL, NULL);
					if (clientSocket == INVALID_SOCKET)
					{
						std::cout << "Ошибка принятия данных accept(): " << WSAGetLastError() << std::endl;
						continue;
					}

					std::lock_guard<std::mutex> lock(_mutex);
					_clientSockets.insert(clientSocket);
					std::cout << "Создание потока для обработки запроса..." << std::endl;
					try
					{
						std::thread([this, clientSocket]()
							{
								std::cout << "Поток для обработки запроса запущен." << std::endl;
								this->handleRequest(clientSocket);
								std::lock_guard<std::mutex> lock(_mutex);
								_clientSockets.erase(clientSocket);
								std::cout << "Запрос обработан, сокет удален." << std::endl;
							}).detach();
						std::cout << "Поток был отделен." << std::endl;
					}
					catch (const std::exception& e)
					{
						std::cerr << "Ошибка при создании потока: " << e.what() << std::endl;
					}
				}
			}
			else if (result == 0)
			{
				if (_stopMainLoop) break;
			}
			else
			{
				std::cout << "Ошибка select(): " << WSAGetLastError() << std::endl;
				break;
			}
		}
		disconnectSocket();
	}

	//int totalFileBytesSend = 0;
	//int previousPercentage = -1;
	//totalFileBytesSend += bytesRead;
//std::cout << "Считано: " << totalFileBytesSend << " байт из " << fileSize << std::endl;

/*int percentage = static_cast<int>((static_cast<double>(totalFileBytesSend) * 100.0) / static_cast<double>(fileSize));
if (percentage > previousPercentage)
{
	previousPercentage = percentage;
	std::cout << "Отправлено: " << percentage << "%" << std::endl;
}*/

	void handleRequest(SOCKET clientSocket)
	{
		std::string request;
		std::unique_ptr<char[]> requestBuffer(new char[BufferSize]);

		while (true)
		{
			request.clear();
			auto peer_addr_len = sizeof(struct sockaddr_storage);
			int status = recvfrom(clientSocket, )
				//int status = recv(clientSocket, requestBuffer.get(), BufferSize, 0);
				if (status > 0)
				{
					request.append(requestBuffer.get(), status);
					std::cout << "Принято байтов: " << request.length() << std::endl;

					std::cout << "Запрос: " << std::endl;
					std::cout << request << std::endl;

					std::string filePath;
					std::string fileData;
					int delimiterPos = request.find(";");
					filePath = request.substr(0, delimiterPos);
					fileData = request.substr(delimiterPos + 1);

					std::ofstream file("./" + filePath, std::ios_base::app);
					if (!file.is_open())
					{
						std::string errorMessage = "Ошибка открытия файла!";
						if (send(clientSocket, errorMessage.c_str(), errorMessage.size(), 0) == SOCKET_ERROR)
						{
							std::cerr << "Ошибка отправки данных: " << WSAGetLastError() << "\n";
						}
						continue;
					}

					file << fileData;

					std::string response = "Файл успешно записан!";
					if (send(clientSocket, response.c_str(), response.size(), 0) == SOCKET_ERROR)
					{
						std::cerr << "Ошибка отправки данных: " << WSAGetLastError() << "\n";
						break;
					}
					file.close();
				}

				else if (status == 0)
				{
					std::cout << "Клиент закрыл соединение." << std::endl;
					break;
				}
				else
				{
					std::cout << "Ошибка чтения данных: " << WSAGetLastError() << std::endl;
					break;
				}
		}
		closesocket(clientSocket);
	}
	/*std::string response = Router::getResponse(request, serverPrefix);
	int sendStatus = send(clientSocket, response.c_str(), response.length(), 0);

	if (sendStatus == SOCKET_ERROR)
	{
		std::cout << "Ошибка отправки: " << WSAGetLastError() << std::endl;
		break;
	}
	else
	{
		std::cout << "Байтов отправлено: " << sendStatus << std::endl;
	}

	request.clear();*/
	/*void handleRequest(SOCKET clientSocket)
	{
		std::string request;
		std::unique_ptr<char[]> requestBuffer(new char[BufferSize]);

		while (true)
		{
			int status = recv(clientSocket, requestBuffer.get(), BufferSize, 0);

			if (status > 0)
			{
				request.append(requestBuffer.get(), status);
				if (processAndReceiveRemainingBytes(clientSocket, request, status) >= 0)
				{
					std::cout << "Принято байтов: " << request.length() << std::endl;
					std::string response = Router::getResponse(request, serverPrefix);
					int sendStatus = send(clientSocket, response.c_str(), response.length(), 0);

					if (sendStatus == SOCKET_ERROR)
					{
						std::cout << "Ошибка отправки: " << WSAGetLastError() << std::endl;
						break;
					}
					else
					{
						std::cout << "Байтов отправлено: " << sendStatus << std::endl;
					}

					request.clear();
				}
				else
				{
					std::cout << "Ошибка чтения данных: " << WSAGetLastError() << std::endl;
				}
			}
			else if (status == 0)
			{
				std::cout << "Клиент закрыл соединение." << std::endl;
				break;
			}
			else
			{
				std::cout << "Ошибка чтения данных: " << WSAGetLastError() << std::endl;
				break;
			}
		}
		closesocket(clientSocket);
	}*/

	void init()
	{
		if (!_initSuccessful)
		{
			_initSuccessful =
				initFields() &&
				/*initWinsock() && */
				initAddrInfo() &&
				initSocket() &&
				bindSocket();
			/*&& startListen();*/
		}
	}

	void startMainLoop()
	{
		_stopMainLoop = false;
		_mainThread = std::thread([this]() { this->threadLoop(); });
	}

	void stopMainLoop()
	{
		_stopMainLoop = true;
		if (_mainThread.joinable()) _mainThread.join();
	}

	~NoSockServer()
	{
		//disconnectSocket();
	}
public:
	static inline const std::string serverPrefix = "server";
};

int main()
{
    setlocale(LC_ALL, "");
    NoSockServer server;
    server.startMainLoop();
    printf("Введите '0' для остановки сервера\n");
    while (true)
    {
        if ((char)getchar() == '0')
        {
            server.stopMainLoop();
            break;
        }
    }
}
