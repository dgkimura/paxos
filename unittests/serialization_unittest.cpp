#include "gtest/gtest.h"

#include "paxos/serialization.hpp"


TEST(SerializationUnitTest, testDecreeIsSerializableAndDeserializable)
{
    Decree expected(Replica("an_author_1"), 1, "the_decree_contents", DecreeType::UserDecree), actual;

    std::string string_obj = Serialize(expected);
    actual = Deserialize<Decree>(string_obj);

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.content, actual.content);
    ASSERT_EQ(expected.number, actual.number);
}


TEST(SerializationUnitTest, testReplicaIsSerializableAndDeserializable)
{
    Replica expected("hostname", 123), actual;

    std::string string_obj = Serialize(expected);
    actual = Deserialize<Replica>(string_obj);

    ASSERT_EQ(expected.hostname, actual.hostname);
    ASSERT_EQ(expected.port, actual.port);
}


TEST(SerializationUnitTest, testMessageIsSerializableAndDeserializable)
{
    Message expected(
        Decree(Replica("author-hostname", 0), 1, "the_decree_contents", DecreeType::UserDecree),
        Replica("hostname-A", 111),
        Replica("hostname-B", 111),
        MessageType::PrepareMessage),
    actual;

    std::string string_obj = Serialize(expected);
    actual = Deserialize<Message>(string_obj);

    ASSERT_EQ(expected.decree.author.hostname, actual.decree.author.hostname);
    ASSERT_EQ(expected.decree.author.port, actual.decree.author.port);
    ASSERT_EQ(expected.decree.number, actual.decree.number);
    ASSERT_EQ(expected.decree.content, actual.decree.content);
    ASSERT_EQ(expected.from.hostname, actual.from.hostname);
    ASSERT_EQ(expected.from.port, actual.from.port);
    ASSERT_EQ(expected.to.hostname, actual.to.hostname);
    ASSERT_EQ(expected.to.port, actual.to.port);
    ASSERT_EQ(expected.type, actual.type);
}


TEST(SerializationUnitTest, testSerializationWithPaddedFluffOnTheEndOfTheBuffer)
{
    Decree expected(Replica("an_author_1", 0), 1, "the_decree_contents", DecreeType::UserDecree), actual;

    actual = Deserialize<Decree>(
        "22 serialization::archive 11 0 0 0 0 11 an_author_1 "
        "0 1 0 19 the_decree_contents THIS IS FLUFF!!!"
    );

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.content, actual.content);
    ASSERT_EQ(expected.number, actual.number);
}


TEST(SerializationUnitTest, testSerializationWithUniversalReferenceValues)
{
    Decree expected(Replica("an_author_1", 0), 1, "the_decree_contents", DecreeType::UserDecree), actual;

    std::string string_obj = Serialize(Decree(Replica("an_author_1", 0), 1, "the_decree_contents", DecreeType::UserDecree));
    actual = Deserialize<Decree>(string_obj);

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.content, actual.content);
    ASSERT_EQ(expected.number, actual.number);
}
