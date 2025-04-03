#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define DEFAULT_PORT 33555

class UDPServer {
    int server_socket = -1;
    struct sockaddr_in server_addr;
    bool is_initialized = false;

    void initializeSocket()
    {
        server_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (server_socket < 0) 
        {
            throw std::runtime_error("Socket creation error: " + std::string(strerror(errno)));
        }

        int optval = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)))
        {
            close(server_socket);
            throw std::runtime_error("Socket option error: " + std::string(strerror(errno)));
        }
    }

    void bindSocket() 
    {
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(DEFAULT_PORT);

        if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) 
        {
            close(server_socket);
            throw std::runtime_error("Bind error: " + std::string(strerror(errno)));
        }
    }

public:
    UDPServer() 
    {
        try 
        {
            initializeSocket();
            bindSocket();
            is_initialized = true;
            std::cout << "Server initialized on port " << DEFAULT_PORT << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Server initialization failed: " << e.what() << std::endl;
            is_initialized = false;
        }
    }

    ~UDPServer() 
    {
        if (server_socket >= 0) 
        {
            if (close(server_socket))
            {
                std::cerr << "Socket closure error: " << strerror(errno) << std::endl;
            }
        }
    }

    bool isInitialized() const 
    {
        return is_initialized;
    }

    void handle() 
    {
        if (!is_initialized) 
        {
            throw std::runtime_error("Server not initialized");
        }

        char buffer[BUFFER_SIZE];
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        while (true) 
        {
            ssize_t bytes_received = recvfrom(
                server_socket,
                buffer,
                BUFFER_SIZE,
                0,
                (struct sockaddr*)&client_addr,
                &client_len
            );

            if (bytes_received < 0) 
            {
                std::cerr << "Receive error: " << strerror(errno) << std::endl;
                continue;
            }

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
            std::cout << "Received " << bytes_received << " bytes from " << client_ip << ":" << ntohs(client_addr.sin_port) << std::endl;

            std::string request(buffer, bytes_received);
            size_t delimiter_pos = request.find(';');
            if (delimiter_pos == std::string::npos)
            {
                sendErrorResponse(client_addr, "Invalid message format: missing delimiter");
                continue;
            }

            std::string file_path = request.substr(0, delimiter_pos);
            std::string file_content = request.substr(delimiter_pos + 1);

            if (saveToFile(file_path, file_content, client_addr)) 
            {
                sendResponse(client_addr, "File saved successfully");
            }
        }
    }

private:
    bool saveToFile(const std::string& path, const std::string& content, const sockaddr_in& client_addr) 
    {
        std::ofstream file("./" + path, std::ios::app);
        if (!file.is_open()) 
        {
            sendErrorResponse(client_addr, "Failed to open file: " + path);
            return false;
        }

        file << content;
        file.close();
        return true;
    }

    void sendResponse(const sockaddr_in& client_addr, const std::string& message) 
    {
        sendToClient(client_addr, message);
    }

    void sendErrorResponse(const sockaddr_in& client_addr, const std::string& error)
    {
        std::cerr << "Error: " << error << std::endl;
        sendToClient(client_addr, "[ERROR] " + error);
    }

    void sendToClient(const sockaddr_in& client_addr, const std::string& message)
    {
        ssize_t bytes_sent = sendto(
            server_socket,
            message.c_str(),
            message.size(),
            0,
            (struct sockaddr*)&client_addr,
            sizeof(client_addr)
        );

        if (bytes_sent < 0) 
        {
            std::cerr << "Send error: " << strerror(errno) << std::endl;
        }
    }
};

int main()
{
    try 
    {
        UDPServer server;

        if (!server.isInitialized()) 
        {
            return EXIT_FAILURE;
        }

        server.handle();

    }
    catch (const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}