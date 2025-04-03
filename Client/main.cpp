#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_PORT "33555"

#pragma comment(lib, "Ws2_32.lib")

class NoSockClient
{
private:
	static const int MinorVersion = 2;
	static const int MajorVersion = 2;

	WSADATA _wsaData;

	struct addrinfo* _result = nullptr;
	struct addrinfo _hints;

	SOCKET _connectSocket = INVALID_SOCKET;

	bool _initSuccessful = false;

	std::string serverName;

	bool initWinsock()
	{
		int status = WSAStartup(MAKEWORD(MajorVersion, MinorVersion), &_wsaData);
		if (status != 0)
		{
			std::cout << "WSAStartup ошибка: " << status << std::endl;
			return false;
		}
		std::cout << "WSA инициализирован" << std::endl;

		return true;
	}

	bool initAddrInfo()
	{
		ZeroMemory(&_hints, sizeof(_hints));
		_hints.ai_family = AF_INET;
		_hints.ai_socktype = SOCK_STREAM;
		_hints.ai_protocol = IPPROTO_TCP;

		//int status = getaddrinfo(Config::getInstance()->getLocalAddress().c_str(), std::to_string(Config::getInstance()->getServerPort()).c_str(), &_hints, &_result);
		std::pair<std::string, int> serverAddress;
		try
		{
			//serverAddress = DNSResolver::resolve(serverName);
			//int status = getaddrinfo(serverAddress.first.c_str(), std::to_string(serverAddress.second).c_str(), &_hints, &_result);
			int status = getaddrinfo(serverName.c_str(), DEFAULT_PORT, &_hints, &_result);
			if (status != 0)
			{
				std::cout << "getaddrinfo ошибка: " << status << std::endl;
				WSACleanup();
				return false;
			}
			std::cout << "Подключение к серверу " << serverName << "..." << std::endl;
			return true;
		}
		catch (...)
		{
			std::cout << "Ошибка! Не удалось разрешить имя сервера..." << std::endl;
			WSACleanup();
			return false;
		}
	}

	bool initSocket()
	{
		_connectSocket = socket(_result->ai_family, _result->ai_socktype, _result->ai_protocol);
		if (_connectSocket == INVALID_SOCKET)
		{
			std::cout << "Ошибка создания сокета socket(): " << WSAGetLastError() << std::endl;
			freeaddrinfo(_result);
			WSACleanup();
			return false;
		}
		std::cout << "Клиентский сокет успешно инициализирован..." << std::endl;

		return true;
	}

	bool connectToSocket()
	{
		int status = connect(_connectSocket, _result->ai_addr, (int)_result->ai_addrlen);
		if (status == SOCKET_ERROR)
		{
			closesocket(_connectSocket);
			_connectSocket = INVALID_SOCKET;
		}
		freeaddrinfo(_result);

		if (_connectSocket == INVALID_SOCKET)
		{
			std::cout << "Невозможно подключиться к серверу" << std::endl;
			WSACleanup();
			return false;
		}
		std::cout << "Успешное подключение к серверу" << std::endl;

		return true;
	}

	bool disconnectSocket()
	{
		if (_connectSocket != INVALID_SOCKET)
		{
			int status = shutdown(_connectSocket, SD_SEND);
			if (status == SOCKET_ERROR)  return false;
		}
		std::cout << "Окончание работы..." << std::endl;

		return true;
	}

	bool closeConnection()
	{
		if (_connectSocket != INVALID_SOCKET) closesocket(_connectSocket);
		WSACleanup();
		std::cout << "Все ресурсы освобождены" << std::endl;
		return true;
	}

	void init()
	{
		if (!_initSuccessful)
		{
			_initSuccessful =
				initWinsock() &&
				initAddrInfo() &&
				initSocket() &&
				connectToSocket();
		}
	}

public:
	NoSockClient(const std::string& serverName)
	{
		this->serverName = serverName;
		init();
	}

	bool isInitSuccessful() { return this->_initSuccessful; }

	std::string write(const std::string& message, const std::string& filePath)
	{
		if (!_initSuccessful) throw std::runtime_error("Клиент некорректно инициализирован!");
		std::unique_ptr<char[]> responseBuffer(new char[BufferSize]);
		std::string request = filePath + ";\n" + message;
		std::string response = "";
		int status = send(_connectSocket, request.c_str(), request.length(), 0);
		if (status == SOCKET_ERROR)
		{
			std::cout << "Ошибка отправки: " << WSAGetLastError() << std::endl;
			closesocket(_connectSocket);
			WSACleanup();
			return std::string("");
		}

		std::cout << "Отправлено байтов: " << status << std::endl;
		status = recv(_connectSocket, responseBuffer.get(), BufferSize, 0);
		if (status > 0)
		{
			response.append(responseBuffer.get(), status);
			std::cout << std::endl << "Ответ: " << std::endl;
			std::cout << response << std::endl;
		}
		else if (status == 0)
		{
			std::cout << "Закрытие соединения" << std::endl;
		}
		else
		{
			std::cout << "Ошибка принятия recv(): " << WSAGetLastError() << std::endl;
		}
		return response;
	}

	~NoSockClient()
	{
		if (!disconnectSocket())
		{
			std::cout << "Ошибка завершеня работы shutdown(): " << WSAGetLastError() << std::endl;
		}
		else
		{
			closeConnection();
		}
	}
};



int main()
{
    setlocale(LC_ALL, "");

    try
    {
        std::string serverName;
        std::cout << "\nВведите название сервера для подключения: ";
        std::getline(std::cin, serverName);
        NoSockClient client(serverName);
        std::string command;
        bool isValidCommand = false;
        if (!client.isInitSuccessful()) return -1;
        while (true)
        {
            std::cout << "\nВведите команду (): ";
            std::getline(std::cin, command);

            if (command == "x")
            {
                std::cout << "Завершение программы...\n";
                break;
            }
            else if (command == "file")
            {

                isValidCommand = true;
            }
            else if (command == "manual")
            {

                isValidCommand = true;
            }
            else
            {
                std::cout << "Неизвестная команда. Попробуйте снова.\n";
            }

            if (isValidCommand)
            {
                std::string savePath;
                std::cout << "Введите конечный путь сохранения файла: ";
                std::getline(std::cin, savePath);
                std::cout << "Файл будет сохранён по пути: " << savePath << "\n";
                std::string content = "";

                client.write(savePath, content);
            }

        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Ошибка: " << e.what() << "\n";
    }

    return 0;
}


