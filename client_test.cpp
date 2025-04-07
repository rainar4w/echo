#include <array>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <shared_mutex>
#include <thread>

#include <stdio.h>
#include <string.h>

#include <client_config.h>
#include <server_config.h>
#include <client/client.h>

#include "server_config.h"
#include <client/config.h>

namespace {
    Logger& getLogger()
    {
        static std::unique_ptr<Logger> l = LoggerFactory::getConsoleLogger();
        return *l;
    }

    bool basicTest(const char *msg)
    {
        Client cl(getLogger());
        std::string reply;

        return cl.connect(PORT) && cl.send(msg) && cl.receive(reply, strlen(msg)) && (0 == reply.compare(msg));
    }

    bool mixedOrderTest(const char *msg1, const char *msg2)
    {
        Client cl1(getLogger());
        std::string reply1;

        Client cl2(getLogger());
        std::string reply2;

        return cl1.connect(PORT) && cl1.send(msg1) &&
            cl2.connect("127.0.0.1", PORT) && cl2.send(msg2) &&
            cl1.receive(reply1, strlen(msg1)) && cl2.receive(reply2, strlen(msg2)) &&
            (0 == reply1.compare(msg1)) && (0 == reply2.compare(msg2));
    }
}

bool test1()
{
    return basicTest("");
}

bool test2()
{
    return basicTest("hello");
}

bool test3()
{
    // line is longer than both server and client read buffers
    std::string msg(1024, 'A');
    return basicTest(msg.c_str());
}

bool test4()
{
    // multiple to both buffers
    std::string msg(SERVER_BUFFEER_SIZE * CLIENT_BUFFER_SIZE, 'B');
    return basicTest(msg.c_str());
}

bool test5()
{
    // even longer
    std::string msg(1024 * 1024, 'C');
    return basicTest(msg.c_str());
}

bool test6()
{
    return mixedOrderTest("hello", "world");
}

bool test7()
{
    std::string msg1(100, 'D');
    std::string msg2(200, 'F');    
    return mixedOrderTest(msg1.c_str(), msg2.c_str());
}

bool test8()
{
    std::atomic<bool> result{true};
    std::array<std::thread, MAX_CONN> conns;

    for (size_t i = 0; i < conns.size(); ++i)
    {
        conns[i] = std::thread([i, &result]() {
            std::string msg(10, 'A' + i);
            if (!basicTest(msg.c_str()))
                result = false;
        });
    }

    for (auto &c : conns)    
        c.join();

    return result;
}

/*bool testVeryLong()
{
    // even longer
    std::string msg(50 * 1024 * 1024, 'C');
    return basicTest(msg.c_str());
}*/

#define TEST(t) do {                                        \
    bool result = t();                                      \
    printf("%s: %s\n", result ? "SUCCESS" : "FAIL", #t);    \
} while (0)

int main ()
{
    TEST(test1);
    TEST(test2);
    TEST(test3);
    TEST(test4);
    TEST(test5);
    TEST(test6);
    TEST(test7);
    TEST(test8);

    return 0;
}
