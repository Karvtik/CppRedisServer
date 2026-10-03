#ifndef REDIS_COMMAND_HANDLER_H
#define REDIS_COMMAND_HANDLER_H

#include <string>

class RedisCommandHandler
{
public:
    RedisCommandHandler();

    // Process a client command and return a RESP-formatted response
    std::string processCommand(const std::string& commandLine);
};

#endif