#include <memory>
#include <string>

#include <stdio.h>
#include <string.h>

#include <client/client.h>
#include <logger/logger.h>

#include "client_config.h"

void printUsage()
{
    printf("client message [server ip]\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2 || argc > 3)
    {
        printUsage();
        return -1;
    }

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")))
    {
        printUsage();
        return 0;
    }

    std::unique_ptr<Logger> logger = LoggerFactory::getConsoleLogger(Logger::Level::INFO);
    Client client(*logger);

    if ((argc == 2 && !client.connect(PORT)) ||
        (argc == 3 && !client.connect(argv[2], PORT)))
    {
        return -1;
    }

    const char *data = argv[1];
    std::string reply;
    if (!client.send(data) || !client.receive(reply, strlen(data)))
        return -1;

    printf("sent: %s\n", data);
    printf("received: %s\n", reply.c_str());

    return 0;
}