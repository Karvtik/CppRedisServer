#include "../include/RedisServer.h"
#include "../include/RedisDatabase.h"

#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[])
{
    int port = 6379; // Default Redis port

    if (argc >= 2)
        port = std::stoi(argv[1]);

    // Load previously saved database
    if (RedisDatabase::getInstance().load("dump.my_rdb"))
    {
        std::cout << "Database Loaded From dump.my_rdb\n";
    }
    else
    {
        std::cout
            << "No dump found or load failed; "
            << "starting with an empty database.\n";
    }

    RedisServer server(port);

    // Background persistence:
    // Save the database every 5 minutes.
    std::thread persistenceThread([]()
    {
        while (true)
        {
            std::this_thread::sleep_for(
                std::chrono::seconds(300));

            if (!RedisDatabase::getInstance().dump("dump.my_rdb"))
            {
                std::cerr
                    << "Error Dumping Database\n";
            }
            else
            {
                std::cout
                    << "Database Dumped to dump.my_rdb\n";
            }
        }
    });

    // Run persistence in the background.
    persistenceThread.detach();

    // Start Redis server.
    server.run();

    return 0;
}