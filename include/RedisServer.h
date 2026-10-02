#ifndef REDIS_SERVER_H
#define REDIS_SERVER_H

#include <atomic>
#include <winsock2.h>

class RedisServer
{
public:
    RedisServer(int port);
    ~RedisServer();

    void run();
    void shutdown();

private:
    int port;
    SOCKET server_socket;
    std::atomic<bool> running;

    void setupSignalHandler();
};

#endif