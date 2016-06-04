#include "gtest/gtest.h"

#include "serialization.hpp"


TEST(SerializationUnitTest, testDecreeIsSerializableAndDeserializable)
{
    Decree expected("an_author_1", 1, "the_decree_contents"), actual;

    std::stringstream stream = Serialize(expected);
    actual = Deserialize<Decree>(std::move(stream));

    ASSERT_EQ(expected.author, actual.author);
    ASSERT_EQ(expected.content, actual.content);
    ASSERT_EQ(expected.number, actual.number);
}


TEST(SerializationUnitTest, testReplicaIsSerializableAndDeserializable)
{
    Replica expected("hostname", 123), actual;

    std::stringstream stream = Serialize(expected);
    actual = Deserialize<Replica>(std::move(stream));

    ASSERT_EQ(expected.hostname, actual.hostname);
    ASSERT_EQ(expected.port, actual.port);
}


TEST(SerializationUnitTest, testMessageIsSerializableAndDeserializable)
{
    Message expected(
        Decree("author", 1, "the_decree_contents"),
        Replica("hostname-A", 111),
        Replica("hostname-B", 111),
        MessageType::PrepareMessage),
    actual;

    std::stringstream stream = Serialize(expected);
    actual = Deserialize<Message>(std::move(stream));

    ASSERT_EQ(expected.decree.author, actual.decree.author);
    ASSERT_EQ(expected.decree.number, actual.decree.number);
    ASSERT_EQ(expected.decree.content, actual.decree.content);
    ASSERT_EQ(expected.from.hostname, actual.from.hostname);
    ASSERT_EQ(expected.from.port, actual.from.port);
    ASSERT_EQ(expected.to.hostname, actual.to.hostname);
    ASSERT_EQ(expected.to.port, actual.to.port);
    ASSERT_EQ(expected.type, actual.type);
}
