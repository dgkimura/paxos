#include <functional>
#include <thread>

#include "gtest/gtest.h"

#include "paxos/signal.hpp"


TEST(SignalTest, x)
{
    Signal s;
    bool is_run = false;

    std::thread t([&s, &is_run](){
        s.Wait();
        is_run = true;
    });

    ASSERT_FALSE(is_run);

    s.Set();
    t.join();

    ASSERT_TRUE(is_run);
}

