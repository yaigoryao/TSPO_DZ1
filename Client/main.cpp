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

void safeInput(double& value, const std::string& prompt) 
{
    while (true) 
    {
        std::cout << prompt;
        if (std::cin >> value) break;

        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid input. Please enter a number.\n";
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

QuadraticCoefficients readFromKeyboard()
{
    QuadraticCoefficients coeffs;
    safeInput(coeffs.a, "Enter coefficient a: ");
    safeInput(coeffs.b, "Enter coefficient b: ");
    safeInput(coeffs.c, "Enter coefficient c: ");
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
        std::string input_line;
        while (run)
        {
            std::cout << "\nMenu:\n"
                << "1. Input coefficients from keyboard\n"
                << "2. Read coefficients from file\n"
                << "3. Exit\n"
                << "Your choice: ";

            std::getline(std::cin, input_line);

            if (input_line.empty()) continue;

            QuadraticCoefficients coeffs;
            std::string response;
            std::string server_filename;

            switch (input_line[0]) {
            case '1': {
                coeffs = readFromKeyboard();

                std::cout << "Enter server filename: ";
                std::getline(std::cin, server_filename);

                std::string result = solveQuadratic(coeffs);
                response = client.sendAndReceive(server_filename + ";" + result);
                break;
            }
            case '2': {
                std::cout << "Enter input filename: ";
                std::string input_filename;
                std::getline(std::cin, input_filename);

                coeffs = readFromFile(input_filename);

                std::cout << "Enter server filename: ";
                std::getline(std::cin, server_filename);

                std::string result = solveQuadratic(coeffs);
                response = client.sendAndReceive(server_filename + ";" + result);
                break;
            }
            case '3':
                run = false;
                break;
            default:
                std::cout << "Invalid choice!\n";
                continue;
            }

            if (!run) break;
            std::cout << "\nServer response: " << response << "\n";
        }
        
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}