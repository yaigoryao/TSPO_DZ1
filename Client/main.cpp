#pragma once
#include <iostream>
#include <sstream>
#include <string.h>
#include <stdio.h>
#include <fstream>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <memory>

#define DEFAULT_PORT 33555

#pragma comment(lib, "Ws2_32.lib")

class NoSockClient
{
private:
	static const int MinorVersion = 2;
	static const int MajorVersion = 2;
	static const int BufferSize = 1024;

	bool _initSuccessful = false;

	std::string serverAddress;
	struct sockaddr_in peer_addr;
	int udp_socket;

	bool getAddrInfo()
	{
		peer_addr.sin_family = AF_INET;
		peer_addr.sin_port = htons(DEFAULT_PORT);
		if (inet_pton(AF_INET, serverAddress.c_str(), &(peer_addr.sin_addr)) < 0)
		{
			perror("Error while receiving connection data");
			return false;
		}
		return true;
	}

	bool initSocket()
	{
		udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
		if (udp_socket < 0)
		{
			perror("Error while initializing socket");
			return false;
		}
		return true;
	}

	void init()
	{
		if (!_initSuccessful)
		{
			_initSuccessful = getAddrInfo() && initSocket();
		}
	}

public:
	NoSockClient(const std::string& serverAddress)
	{
		this->serverAddress = serverAddress;
		init();
	}

	bool isInitSuccessful() { return this->_initSuccessful; }

	std::string write(const std::string& message, const std::string& filePath)
	{
		if (!_initSuccessful) throw std::runtime_error("Error initializing client!");
		std::unique_ptr<char[]> responseBuffer(new char[BufferSize]);
		std::string request = filePath + ";\n" + message;
		std::string response = "";
		int status = sendto(udp_socket, request.length(), 0, (struct sockaddr*) & peer_addr, sizeof(peer_addr));
		if (status < 0)
		{
			std::cout << "Failed to send a message!" << std::endl;
			
			return std::string("");
		}

		std::cout << "Bytes send: " << status << std::endl;
		status = recv(_connectSocket, responseBuffer.get(), BufferSize, 0);
		if (status > 0)
		{
			response.append(responseBuffer.get(), status);
			std::cout << std::endl << "Ответ: " << std::endl;
			std::cout << response << std::endl;
		}
		else if (status == 0)
		{
			std::cout << "Closing connection..." << std::endl;
		}
		else
		{
			std::cout << "recv() error!" << std::endl;
		}
		return response;
	}

	~NoSockClient()
	{
		std::cout << "Closing socket..." << std::endl;
		close(udp_socket);
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


