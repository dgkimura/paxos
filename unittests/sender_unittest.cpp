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

        void Read(){}
    };

    paxos::NetworkFileSender<MockTransport> sender;

    sender.SendFile(
        paxos::Replica("A", 111),
        paxos::BootstrapFile("filename", "the file contents")
    );

    ASSERT_EQ("filename", paxos::Deserialize<paxos::BootstrapFile>(transport_writes[0]).name);
    ASSERT_EQ("the file contents", paxos::Deserialize<paxos::BootstrapFile>(transport_writes[0]).content);
}


TEST(SenderTest, testCreateHeader)
{
    ASSERT_THAT(paxos::CreateHeader(0), testing::ElementsAre('\0', '\0', '\0', '\0'));
    ASSERT_THAT(paxos::CreateHeader(1), testing::ElementsAre('\0', '\0', '\0', '\x01'));
    ASSERT_THAT(paxos::CreateHeader(2), testing::ElementsAre('\0', '\0', '\0', '\x02'));
    ASSERT_THAT(paxos::CreateHeader(4), testing::ElementsAre('\0', '\0', '\0', '\x04'));
    ASSERT_THAT(paxos::CreateHeader(8), testing::ElementsAre('\0', '\0', '\0', '\x08'));
    ASSERT_THAT(paxos::CreateHeader(16), testing::ElementsAre('\0', '\0', '\0', '\x10'));
    ASSERT_THAT(paxos::CreateHeader(32), testing::ElementsAre('\0', '\0', '\0', '\x20'));
    ASSERT_THAT(paxos::CreateHeader(64), testing::ElementsAre('\0', '\0', '\0', '\x40'));
    ASSERT_THAT(paxos::CreateHeader(128), testing::ElementsAre('\0', '\0', '\0', '\x80'));
    ASSERT_THAT(paxos::CreateHeader(256), testing::ElementsAre('\0', '\0', '\x01', '\0'));
    ASSERT_THAT(paxos::CreateHeader(512), testing::ElementsAre('\0', '\0', '\x02', '\0'));
    ASSERT_THAT(paxos::CreateHeader(1024), testing::ElementsAre('\0', '\0', '\x04', '\0'));
    ASSERT_THAT(paxos::CreateHeader(2048), testing::ElementsAre('\0', '\0', '\x08', '\0'));
    ASSERT_THAT(paxos::CreateHeader(4096), testing::ElementsAre('\0', '\0', '\x10', '\0'));
    ASSERT_THAT(paxos::CreateHeader(8192), testing::ElementsAre('\0', '\0', '\x20', '\0'));
    ASSERT_THAT(paxos::CreateHeader(16384), testing::ElementsAre('\0', '\0', '\x40', '\0'));
    ASSERT_THAT(paxos::CreateHeader(32768), testing::ElementsAre('\0', '\0', '\x80', '\0'));
    ASSERT_THAT(paxos::CreateHeader(65536), testing::ElementsAre('\0', '\x01', '\0', '\0'));
    ASSERT_THAT(paxos::CreateHeader(65536), testing::ElementsAre('\0', '\x01', '\0', '\0'));
    ASSERT_THAT(paxos::CreateHeader(131072), testing::ElementsAre('\0', '\x02', '\0', '\0'));
    ASSERT_THAT(paxos::CreateHeader(262144), testing::ElementsAre('\0', '\x04', '\0', '\0'));
    ASSERT_THAT(paxos::CreateHeader(524288), testing::ElementsAre('\0', '\x08', '\0', '\0'));
    ASSERT_THAT(paxos::CreateHeader(1048576), testing::ElementsAre('\0', '\x10', '\0', '\0'));
    ASSERT_THAT(paxos::CreateHeader(2097152), testing::ElementsAre('\0', '\x20', '\0', '\0'));
    ASSERT_THAT(paxos::CreateHeader(4194304), testing::ElementsAre('\0', '\x40', '\0', '\0'));
    ASSERT_THAT(paxos::CreateHeader(8388608), testing::ElementsAre('\0', '\x80', '\0', '\0'));
    ASSERT_THAT(paxos::CreateHeader(16777216), testing::ElementsAre('\x01', '\0', '\0', '\0'));
}
