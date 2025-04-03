#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define DEFAULT_PORT 33555

class UDPClient 
{
    int client_socket = -1;
    struct sockaddr_in server_addr;
    bool is_initialized = false;

    void initializeSocket() 
    {
        client_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (client_socket < 0) 
        {
            throw std::runtime_error("Socket creation error: " + std::string(strerror(errno)));
        }
    }

    void initializeAddress(const std::string& server_ip)
    {
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(DEFAULT_PORT);

        if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0)
        {
            close(client_socket);
            throw std::runtime_error("Invalid address format: " + std::string(strerror(errno)));
        }
    }

public:
    explicit UDPClient(const std::string& server_ip)
    {
        try
        {
            initializeSocket();
            initializeAddress(server_ip);
            is_initialized = true;
        }
        catch (const std::exception& e) 
        {
            std::cerr << "Initialization failed: " << e.what() << std::endl;
            is_initialized = false;
        }
    }

    ~UDPClient() {
        if (client_socket >= 0)
        {
            if (close(client_socket))
            {
                std::cerr << "Socket closure error: " << strerror(errno) << std::endl;
            }
        }
    }

    bool isInitialized() const {
        return is_initialized;
    }

    std::string sendAndReceive(const std::string& message) 
    {
        if (!is_initialized)
        {
            throw std::runtime_error("Client not initialized");
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytes_sent, bytes_received;

        bytes_sent = sendto(
            client_socket,
            message.c_str(),
            message.size(),
            0,
            (struct sockaddr*)&server_addr,
            sizeof(server_addr)
        );

        if (bytes_sent < 0) 
        {
            throw std::runtime_error("Send error: " + std::string(strerror(errno)));
        }

        struct sockaddr_in response_addr;
        socklen_t addr_len = sizeof(response_addr);

        bytes_received = recvfrom(
            client_socket,
            buffer,
            BUFFER_SIZE,
            0,
            (struct sockaddr*)&response_addr,
            &addr_len
        );

        if (bytes_received < 0) 
        {
            throw std::runtime_error("Receive error: " + std::string(strerror(errno)));
        }

        char sender_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &response_addr.sin_addr, sender_ip, INET_ADDRSTRLEN);

        std::cout << "Received " << bytes_received
            << " bytes from " << sender_ip
            << ":" << ntohs(response_addr.sin_port)
            << std::endl;

        return std::string(buffer, bytes_received);
    }
};

int main() 
{
    try
    {
        UDPClient client("127.0.0.1");

        if (!client.isInitialized()) 
        {
            return EXIT_FAILURE;
        }

        std::string response = client.sendAndReceive("Hello, Server!");
        std::cout << "Server response: " << response << std::endl;

    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}