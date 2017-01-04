#include "gtest/gtest.h"

#include "paxos/messages.hpp"


TEST(MessageTest, testMessageResponseUpdatesSenderAndReceiver)
{
    Replica from("from_hostname");
    Replica to("to_hostname");

    Decree the_decree(from, 1, "", DecreeType::UserDecree);
    Message m(the_decree, from, to, MessageType::RequestMessage);

    Message response = Response(m, MessageType::PromiseMessage);

    ASSERT_EQ(response.from.hostname, m.to.hostname);
    ASSERT_EQ(response.to.hostname, m.from.hostname);
    ASSERT_EQ(response.type, MessageType::PromiseMessage);
}


TEST(MessageTest, testMessageTypeHashFunction)
{
    ASSERT_EQ(0, MessageTypeHash()(MessageType::RequestMessage));
    ASSERT_EQ(1, MessageTypeHash()(MessageType::RetryRequestMessage));
    ASSERT_EQ(2, MessageTypeHash()(MessageType::PrepareMessage));
    ASSERT_EQ(3, MessageTypeHash()(MessageType::RetryPrepareMessage));
    ASSERT_EQ(4, MessageTypeHash()(MessageType::PromiseMessage));
    ASSERT_EQ(5, MessageTypeHash()(MessageType::NackMessage));
    ASSERT_EQ(6, MessageTypeHash()(MessageType::AcceptMessage));
    ASSERT_EQ(7, MessageTypeHash()(MessageType::AcceptedMessage));
    ASSERT_EQ(8, MessageTypeHash()(MessageType::UpdateMessage));
    ASSERT_EQ(9, MessageTypeHash()(MessageType::UpdatedMessage));
}
