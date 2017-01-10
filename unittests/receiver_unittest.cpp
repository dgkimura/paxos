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

    void RegisterAction(std::function<void(std::string content)> action)
    {
    }

    void Start()
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


TEST(NetworkReceiverTest, testProcessMessageRunsCallbacksAssociatedWithRegisteredMessage)
{
    bool was_callback_called = false;

    NetworkReceiver<MockServer> receiver("myhost", 1111);
    receiver.RegisterCallback(
        Callback([&was_callback_called](Message m){was_callback_called = true;}),
        MessageType::RequestMessage);

    receiver.ProcessContent(
        Serialize(
            Message(
                Decree(),
                Replica("A"),
                Replica("B"),
                MessageType::RequestMessage
            )
        )
    );

    ASSERT_TRUE(was_callback_called);
}
