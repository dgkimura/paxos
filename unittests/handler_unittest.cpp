#include <functional>

#include "gtest/gtest.h"

#include "paxos/handler.hpp"


TEST(HandlerTest, testDecreeHandlerWithSingleHandler)
{
    bool was_called = false;
    DecreeHandler handler([&was_called](std::string entry){ was_called=true; });

    ASSERT_FALSE(was_called);

    handler("some entry");

    ASSERT_TRUE(was_called);
}


TEST(HandlerTest, testDecreeHandlerWithMultipleHandlers)
{
    bool was_called = false;
    bool was_called_too = false;

    DecreeHandler handler;
    handler.Add([&was_called](std::string entry){ was_called=true; });
    handler.Add([&was_called_too](std::string entry){ was_called_too=true; });

    ASSERT_FALSE(was_called);
    ASSERT_FALSE(was_called_too);

    handler("some entry");

    ASSERT_TRUE(was_called);
    ASSERT_TRUE(was_called_too);
}
