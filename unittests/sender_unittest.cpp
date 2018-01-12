#include <functional>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "paxos/messages.hpp"
#include "paxos/sender.hpp"


TEST(SenderTest, testReplyAllSendsMultipleMessages)
{
    static std::vector<std::string> transport_writes; // Yuck, a static...

    class MockTransport
    {
    public:
        MockTransport(std::string hostname, short port)
        {
        }
        void Write(std::string content)
        {
            transport_writes.push_back(content);
        }
    };

    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    replicaset->Add(paxos::Replica("A", 111));
    replicaset->Add(paxos::Replica("B", 222));
    replicaset->Add(paxos::Replica("C", 333));

    paxos::NetworkSender<MockTransport> sender(replicaset);

    paxos::Message m(
        paxos::Decree(),
        paxos::Replica("from", 111),
        paxos::Replica("to", 111),
        paxos::MessageType::RequestMessage
    );

    sender.ReplyAll(m);

    ASSERT_EQ("22 serialization::archive 15 0 0 0 0 4 from 111 1 A 111 1 0 0 0  0 0 0 0 0 ", transport_writes[0]);
    ASSERT_EQ("22 serialization::archive 15 0 0 0 0 4 from 111 1 B 222 1 0 0 0  0 0 0 0 0 ", transport_writes[1]);
    ASSERT_EQ("22 serialization::archive 15 0 0 0 0 4 from 111 1 C 333 1 0 0 0  0 0 0 0 0 ", transport_writes[2]);
}


TEST(SenderTest, testSendFileAlongTransport)
{
    static std::vector<std::string> transport_writes; // Yuck, a static...

    class MockTransport
    {
    public:
        MockTransport(std::string hostname, short port)
        {
        }
        void Write(std::string content)
        {
            transport_writes.push_back(content);
        }
    };

    paxos::NetworkFileSender<MockTransport> sender;

    sender.SendFile(
        paxos::Replica("A", 111),
        paxos::BootstrapFile("filename", "the file contents")
    );

    ASSERT_EQ("filename", paxos::Deserialize<paxos::BootstrapFile>(transport_writes[0]).name);
    ASSERT_EQ("the file contents", paxos::Deserialize<paxos::BootstrapFile>(transport_writes[0]).content);
}
