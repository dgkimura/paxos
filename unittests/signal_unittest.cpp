#include <functional>
#include <thread>

#include "gtest/gtest.h"

#include "paxos/signal.hpp"


TEST(SignalTest, x)
{
    paxos::Signal s;
    bool result = false;

    std::thread t([&s, &result](){
        result = s.Wait();
    });

    ASSERT_FALSE(result);

    s.Set(true);
    t.join();

    ASSERT_TRUE(result);
}

