#include <chrono>
#include <functional>

#include "gtest/gtest.h"

#include "paxos/pause.hpp"


TEST(PauseTest, testRnadomPauseStartRunsCallbackHandler)
{
    bool is_handler_called = false;

    paxos::RandomPause pause(std::chrono::milliseconds(0));
    pause.Start(
        [&is_handler_called]()
        {
            is_handler_called = true;
        });
    ASSERT_TRUE(is_handler_called);
}

