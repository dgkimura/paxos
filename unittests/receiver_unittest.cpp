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
    paxos::NetworkReceiver<MockServer> receiver("myhost", 1111);

    ASSERT_EQ(0, receiver.GetRegisteredCallbacks(paxos::MessageType::RequestMessage).size());
}


TEST(NetworkReceiverTest, testReceiverWithRegisteredCallback)
{
    paxos::NetworkReceiver<MockServer> receiver("myhost", 1111);

    receiver.RegisterCallback(
        paxos::Callback([](paxos::Message m){}),
        paxos::MessageType::RequestMessage);

    ASSERT_EQ(1, receiver.GetRegisteredCallbacks(paxos::MessageType::RequestMessage).size());
}


TEST(NetworkReceiverTest, testReceiverWithMultipleRegisteredCallback)
{
    paxos::NetworkReceiver<MockServer> receiver("myhost", 1111);

    receiver.RegisterCallback(
        paxos::Callback([](paxos::Message m){}),
        paxos::MessageType::RequestMessage);
    receiver.RegisterCallback(
        paxos::Callback([](paxos::Message m){}),
        paxos::MessageType::RequestMessage);

    ASSERT_EQ(2, receiver.GetRegisteredCallbacks(paxos::MessageType::RequestMessage).size());
}


TEST(NetworkReceiverTest, testProcessMessageRunsCallbacksAssociatedWithRegisteredMessage)
{
    bool was_callback_called = false;

    paxos::NetworkReceiver<MockServer> receiver("myhost", 1111);
    receiver.RegisterCallback(
        paxos::Callback([&was_callback_called](paxos::Message m){was_callback_called = true;}),
        paxos::MessageType::RequestMessage);

    receiver.ProcessContent(
        Serialize(
            paxos::Message(
                paxos::Decree(),
                paxos::Replica("A"),
                paxos::Replica("B"),
                paxos::MessageType::RequestMessage
            )
        )
    );

    ASSERT_TRUE(was_callback_called);
}
