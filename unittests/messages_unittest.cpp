#include "gtest/gtest.h"

#include "paxos/messages.hpp"


TEST(MessageTest, testMessageResponseUpdatesSenderAndReceiver)
{
    Replica from("from_hostname");
    Replica to("to_hostname");

    Decree the_decree(from, 1, "");
    Message m(the_decree, from, to, MessageType::RequestMessage);

    Message response = Response(m, MessageType::PromiseMessage);

    ASSERT_EQ(response.from.hostname, m.to.hostname);
    ASSERT_EQ(response.to.hostname, m.from.hostname);
    ASSERT_EQ(response.type, MessageType::PromiseMessage);
}
