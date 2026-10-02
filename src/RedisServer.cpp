#include "../include/RedisServer.h"
#include "../include/RedisCommandHandler.h"
#include "../include/RedisDatabase.h"

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <thread>
#include <cstring>
#include <csignal>
#include <cstdlib>

// Global pointer for signal handling
static RedisServer* globalServer = nullptr;

void signalHandler(int signum)
{
    if (globalServer)
    {
        std::cout << "\nCaught signal " << signum
                  << ", shutting down...\n";

        globalServer->shutdown();
    }

    std::exit(signum);
}

void RedisServer::setupSignalHandler()
{
    std::signal(SIGINT, signalHandler);
}

RedisServer::RedisServer(int port)
    : port(port),
      server_socket(INVALID_SOCKET),
      running(true)
{
    globalServer = this;
    setupSignalHandler();
}

RedisServer::~RedisServer()
{
    shutdown();
}

void RedisServer::shutdown()
{
    if (!running)
        return;

    running = false;

    if (server_socket != INVALID_SOCKET)
    {
        if (RedisDatabase::getInstance().dump("dump.my_rdb"))
            std::cout << "Database Dumped to dump.my_rdb\n";
        else
            std::cerr << "Error dumping database\n";

        closesocket(server_socket);
        server_socket = INVALID_SOCKET;
    }

    std::cout << "Server Shutdown Complete!\n";
}

void RedisServer::run()
{
    // Initialize Winsock
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed\n";
        return;
    }

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == INVALID_SOCKET)
    {
        std::cerr << "Error Creating Server Socket\n";
        WSACleanup();
        return;
    }

    int opt = 1;

    if (setsockopt(
            server_socket,
            SOL_SOCKET,
            SO_REUSEADDR,
            reinterpret_cast<const char*>(&opt),
            sizeof(opt)) == SOCKET_ERROR)
    {
        std::cerr << "Error Setting Socket Options\n";
        closesocket(server_socket);
        server_socket = INVALID_SOCKET;
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr{};

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(
            server_socket,
            reinterpret_cast<sockaddr*>(&serverAddr),
            sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Error Binding Server Socket\n";
        closesocket(server_socket);
        server_socket = INVALID_SOCKET;
        WSACleanup();
        return;
    }

    if (listen(server_socket, 10) == SOCKET_ERROR)
    {
        std::cerr << "Error Listening On Server Socket\n";
        closesocket(server_socket);
        server_socket = INVALID_SOCKET;
        WSACleanup();
        return;
    }

    std::cout << "Redis Server Listening On Port "
              << port << "\n";

    std::vector<std::thread> threads;

    RedisCommandHandler cmdHandler;

    while (running)
    {
        SOCKET client_socket = accept(
            server_socket,
            nullptr,
            nullptr);

        if (client_socket == INVALID_SOCKET)
        {
            if (running)
                std::cerr << "Error Accepting Client Connection\n";

            break;
        }

        threads.emplace_back(
            [client_socket, &cmdHandler]()
            {
                char buffer[1024];

                while (true)
                {
                    std::memset(buffer, 0, sizeof(buffer));

                    int bytes = recv(
                        client_socket,
                        buffer,
                        sizeof(buffer) - 1,
                        0);

                    if (bytes <= 0)
                        break;

                    std::string request(buffer, bytes);

                    std::string response =
                        cmdHandler.processCommand(request);

                    send(
                        client_socket,
                        response.c_str(),
                        static_cast<int>(response.size()),
                        0);
                }

                closesocket(client_socket);
            });
    }

    for (auto& t : threads)
    {
        if (t.joinable())
            t.join();
    }

    if (server_socket != INVALID_SOCKET)
    {
        closesocket(server_socket);
        server_socket = INVALID_SOCKET;
    }

    WSACleanup();
}