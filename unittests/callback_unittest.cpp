#include <functional>

#include "gtest/gtest.h"

#include "paxos/callback.hpp"


TEST(CallbackTest, testCallbackWithLambdaExpression)
{
    bool ran_callback = false;
    paxos::Message m;

    paxos::Callback callback([&](paxos::Message m) { ran_callback = true; });
    callback(m);

    ASSERT_TRUE(ran_callback);
}


TEST(CallbackTest, testCallbackWithBindExpression)
{
    bool ran_callback = false;
    paxos::Message m;

    using namespace std::placeholders;

    paxos::Callback callback(std::bind([&](paxos::Message m, int a_number) { ran_callback = true; }, std::placeholders::_1, 1234));
    callback(m);

    ASSERT_TRUE(ran_callback);
}
