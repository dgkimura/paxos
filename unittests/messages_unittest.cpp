#include "gtest/gtest.h"

#include "messages.hpp"


TEST(MessageTest, testMessageResponseUpdatesSenderAndReceiver)
{
    Decree the_decree("an_author", 1, "");
    Replica from("from_hostname");
    Replica to("to_hostname");
    Message m(the_decree, from, to, MessageType::RequestMessage);

    Message response = Response(m, MessageType::PromiseMessage);

    ASSERT_EQ(response.from.hostname, m.to.hostname);
    ASSERT_EQ(response.to.hostname, m.from.hostname);
    ASSERT_EQ(response.type, MessageType::PromiseMessage);
}
