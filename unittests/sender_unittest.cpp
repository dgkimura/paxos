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
        void Connect(std::string hostname, short port)
        {
        }
        void Write(std::string content)
        {
            transport_writes.push_back(content);
        }
    };

    auto replicaset = std::make_shared<ReplicaSet>();
    replicaset->Add(Replica("A", 111));
    replicaset->Add(Replica("B", 222));
    replicaset->Add(Replica("C", 333));

    NetworkSender<MockTransport> sender(replicaset);

    Message m(
        Decree(),
        Replica("from", 111),
        Replica("to", 111),
        MessageType::RequestMessage
    );

    sender.ReplyAll(m);

    ASSERT_EQ("22 serialization::archive 14 0 0 0 0 4 from 111 1 A 111 0 0 0 0  0 0 0 0 ", transport_writes[0]);
    ASSERT_EQ("22 serialization::archive 14 0 0 0 0 4 from 111 1 B 222 0 0 0 0  0 0 0 0 ", transport_writes[1]);
    ASSERT_EQ("22 serialization::archive 14 0 0 0 0 4 from 111 1 C 333 0 0 0 0  0 0 0 0 ", transport_writes[2]);
}


TEST(SenderTest, testSendFileAlongTransport)
{
    static std::vector<std::string> transport_writes; // Yuck, a static...

    class MockTransport
    {
    public:
        void Connect(std::string hostname, short port)
        {
        }
        void Write(std::string content)
        {
            transport_writes.push_back(content);
        }
    };

    NetworkFileSender<MockTransport> sender;

    sender.SendFile(
        Replica("A", 111),
        BootstrapFile("filename", "the file contents")
    );

    ASSERT_EQ("filename", Deserialize<BootstrapFile>(transport_writes[0]).name);
    ASSERT_EQ("the file contents", Deserialize<BootstrapFile>(transport_writes[0]).content);
}
