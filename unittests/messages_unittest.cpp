#include "gtest/gtest.h"

#include "paxos/messages.hpp"


TEST(MessageTest, testMessageResponseUpdatesSenderAndReceiver)
{
    paxos::Replica from("from_hostname");
    paxos::Replica to("to_hostname");

    paxos::Decree the_decree(from, 1, "", paxos::DecreeType::UserDecree);
    paxos::Message m(the_decree, from, to, paxos::MessageType::RequestMessage);

    paxos::Message response = paxos::Response(m, paxos::MessageType::PromiseMessage);

    ASSERT_EQ(response.from.hostname, m.to.hostname);
    ASSERT_EQ(response.to.hostname, m.from.hostname);
    ASSERT_EQ(response.type, paxos::MessageType::PromiseMessage);
}
