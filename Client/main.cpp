#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>
#include <fstream>
#include <cmath>
#include <sstream>
#include <limits>
#include <iomanip>

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


struct QuadraticCoefficients 
{
    double a = 0.0;
    double b = 0.0;
    double c = 0.0;
};

QuadraticCoefficients readFromKeyboard() 
{
    QuadraticCoefficients coeffs;
    std::cout << "Enter coefficients a, b, c: ";
    while (!(std::cin >> coeffs.a >> coeffs.b >> coeffs.c))
    {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid input. Please enter numbers: ";
    }
    return coeffs;
}

QuadraticCoefficients readFromFile(const std::string& filename) 
{
    QuadraticCoefficients coeffs;
    std::ifstream file(filename);
    if (file >> coeffs.a >> coeffs.b >> coeffs.c) 
    {
        return coeffs;
    }
    throw std::runtime_error("Error reading coefficients from file");
}

struct Solution
{
    bool is_linear = false;
    bool no_solution = false;
    bool infinite_solutions = false;
    double roots[2];
    int num_roots = 0;
};

Solution solveQuadraticImpl(const QuadraticCoefficients& coeffs)
{
    Solution sol;
    const double eps = 1e-9;
    double a = coeffs.a;
    double b = coeffs.b;
    double c = coeffs.c;

    if (fabs(a) < eps) 
    {
        sol.is_linear = true;
        if (fabs(b) < eps)
        {
            if (fabs(c) < eps) 
            {
                sol.infinite_solutions = true;
            }
            else 
            {
                sol.no_solution = true;
            }
        }
        else 
        {
            sol.roots[0] = -c / b;
            sol.num_roots = 1;
        }
        return sol;
    }

    double d = b * b - 4 * a * c;
    if (d < -eps) 
    {
        sol.no_solution = true;
    }
    else 
    {
        double sqrt_d = sqrt(fabs(d));
        if (d < eps) 
        { 
            sol.roots[0] = -b / (2 * a);
            sol.num_roots = 1;
        }
        else 
        { 
            sol.roots[0] = (-b - sqrt_d) / (2 * a);
            sol.roots[1] = (-b + sqrt_d) / (2 * a);
            sol.num_roots = 2;
        }
    }
    return sol;
}

std::string solveQuadratic(QuadraticCoefficients coeffs) 
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3);

    Solution sol = solveQuadraticImpl(coeffs);

    if (sol.infinite_solutions) return "Infinite solutions";
    if (sol.no_solution) return "No real solutions";

    if (sol.is_linear) 
    {
        oss << "x = " << sol.roots[0];
        return oss.str();
    }

    switch (sol.num_roots) 
    {
    case 1:
        oss << "x = " << sol.roots[0] << " (double root)";
        break;
    case 2:
        oss << "x1 = " << sol.roots[0] << "; x2 = " << sol.roots[1];
        break;
    }
    return oss.str();
}


int main(int argc, char* argv[]) 
{
    try 
    {
        if (argc < 2)
        {
            std::cerr << "Usage: " << argv[0] << " <server_ip>" << std::endl;
            std::cerr << "Example: " << argv[0] << " 192.168.1.100" << std::endl;
            return EXIT_FAILURE;
        }

        const std::string server_ip(argv[1]);

        UDPClient client(server_ip);

        if (!client.isInitialized())
        {
            return EXIT_FAILURE;
        }
        
        bool run = true;
        while (true)
        {
            std::cout << "[1] Input coefficients from keyboard and send solution\n[2] Read coefficients and send solution\n[3] Exit" << std::endl;
            char input = getchar();
            const std::string message = "";
            QuadraticCoefficients coeffs;

            switch (input)
            {
                case '1':
                    coeffs = readFromKeyboard();
                    break;
                case '2':
                    coeffs = readFromFile("equation.txt");
                    break;
                case '3':
                    run = false;
                    break;
                default:
                    std::cout << "Wrong command!" << std::endl;
            }
            if (!run) break;
            std::string result = solveQuadratic(coeffs);
            std::string response = client.sendAndReceive(message);
            std::cout << "Server response: " << response << std::endl;
        }
        
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}