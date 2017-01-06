#include <functional>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "paxos/receiver.hpp"


class MockServer
{
public:

    MockServer(std::string address, short port)
    {
    }

    void RegisterAction(std::function<void(std::string content)> action_)
    {
    }
};


TEST(NetworkReceiverTest, testReceiverWithNoRegisteredCallback)
{
    NetworkReceiver<MockServer> receiver("myhost", 1111);

    ASSERT_EQ(0, receiver.GetRegisteredCallbacks(MessageType::RequestMessage).size());
}


TEST(NetworkReceiverTest, testReceiverWithRegisteredCallback)
{
    NetworkReceiver<MockServer> receiver("myhost", 1111);

    receiver.RegisterCallback(
        Callback([](Message m){}),
        MessageType::RequestMessage);

    ASSERT_EQ(1, receiver.GetRegisteredCallbacks(MessageType::RequestMessage).size());
}


TEST(NetworkReceiverTest, testReceiverWithMultipleRegisteredCallback)
{
    NetworkReceiver<MockServer> receiver("myhost", 1111);

    receiver.RegisterCallback(
        Callback([](Message m){}),
        MessageType::RequestMessage);
    receiver.RegisterCallback(
        Callback([](Message m){}),
        MessageType::RequestMessage);

    ASSERT_EQ(2, receiver.GetRegisteredCallbacks(MessageType::RequestMessage).size());
}
