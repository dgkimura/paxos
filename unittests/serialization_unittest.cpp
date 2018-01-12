#include "gtest/gtest.h"

#include "paxos/serialization.hpp"


TEST(SerializationUnitTest, testDecreeIsSerializableAndDeserializable)
{
    paxos::Decree expected(paxos::Replica("an_author_1"), 1, "the_decree_contents", paxos::DecreeType::UserDecree), actual;

    std::string string_obj = paxos::Serialize(expected);
    actual = paxos::Deserialize<paxos::Decree>(string_obj);

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.content, actual.content);
    ASSERT_EQ(expected.number, actual.number);
}


TEST(SerializationUnitTest, testUpdateReplicaSetDecreeIsSerializableAndDeserializable)
{
    paxos::UpdateReplicaSetDecree expected
    {
        paxos::Replica("author"),
        paxos::Replica("myhost"),
        "remote/directory/path"
    }, actual;

    std::string string_obj = paxos::Serialize(expected);
    actual = paxos::Deserialize<paxos::UpdateReplicaSetDecree>(string_obj);

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.replica.hostname, actual.replica.hostname);
    ASSERT_EQ(expected.replica.port, actual.replica.port);
    ASSERT_EQ(expected.remote_directory, actual.remote_directory);
}


TEST(SerializationUnitTest, testReplicaIsSerializableAndDeserializable)
{
    paxos::Replica expected("hostname", 123), actual;

    std::string string_obj = paxos::Serialize(expected);
    actual = paxos::Deserialize<paxos::Replica>(string_obj);

    ASSERT_EQ(expected.hostname, actual.hostname);
    ASSERT_EQ(expected.port, actual.port);
}


TEST(SerializationUnitTest, testMessageIsSerializableAndDeserializable)
{
    paxos::Message expected(
        paxos::Decree(paxos::Replica("author-hostname", 0), 1, "the_decree_contents", paxos::DecreeType::UserDecree),
        paxos::Replica("hostname-A", 111),
        paxos::Replica("hostname-B", 111),
        paxos::MessageType::PrepareMessage),
    actual;

    std::string string_obj = paxos::Serialize(expected);
    actual = paxos::Deserialize<paxos::Message>(string_obj);

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


TEST(SerializationUnitTest, testBootstrapMetadataIsSerializableAndDeserializable)
{
    paxos::BootstrapMetadata expected
    {
        "localpath",
        "remotepath"
    },
    actual;

    std::string string_obj = paxos::Serialize(expected);
    actual = paxos::Deserialize<paxos::BootstrapMetadata>(string_obj);

    ASSERT_EQ(expected.local, actual.local);
    ASSERT_EQ(expected.remote, actual.remote);
}


TEST(SerializationUnitTest, testBootstrapFileIsSerializableAndDeserializable)
{
    paxos::BootstrapFile expected(
        "the_filename",
        "content of the bootstrap file."),
    actual;

    std::string string_obj = paxos::Serialize(expected);
    actual = paxos::Deserialize<paxos::BootstrapFile>(string_obj);

    ASSERT_EQ(expected.name, actual.name);
    ASSERT_EQ(expected.content, actual.content);
}


TEST(SerializationUnitTest, testSerializationWithPaddedFluffOnTheEndOfTheBuffer)
{
    paxos::Decree expected(paxos::Replica("an_author_1", 0), 1, "the_decree_contents", paxos::DecreeType::UserDecree), actual;

    actual = paxos::Deserialize<paxos::Decree>(
        "22 serialization::archive 14 0 0 0 0 11 an_author_1 "
        "0 1 1 0 19 the_decree_contents THIS IS FLUFF!!!"
    );

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.content, actual.content);
    ASSERT_EQ(expected.number, actual.number);
}


TEST(SerializationUnitTest, testSerializationWithUniversalReferenceValues)
{
    paxos::Decree expected(paxos::Replica("an_author_1", 0), 1, "the_decree_contents", paxos::DecreeType::UserDecree), actual;

    std::string string_obj = paxos::Serialize(paxos::Decree(paxos::Replica("an_author_1", 0), 1, "the_decree_contents", paxos::DecreeType::UserDecree));
    actual = paxos::Deserialize<paxos::Decree>(string_obj);

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.content, actual.content);
    ASSERT_EQ(expected.number, actual.number);
}


TEST(SerializationUnitTest, testSerializationWithStreamValues)
{
    paxos::Decree expected(paxos::Replica("an_author_1", 0), 1, "the_decree_contents", paxos::DecreeType::UserDecree), actual;

    std::istringstream stream_obj(paxos::Serialize(expected));
    actual = paxos::Deserialize<paxos::Decree>(stream_obj);

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.content, actual.content);
    ASSERT_EQ(expected.number, actual.number);
}


TEST(SerializationUnitTest, testMessageDeserializionOfCorruptedStringReturnsInvalidMessage)
{
    std::string string_obj = "22 serialization::archiv\x01!14 0 0 0 0 0  0 9 "
                             "127.0.0.1 8094 9 0 0 9 127.0.0.1 8094 678 678 0 0 ";

    paxos::Message message = paxos::Deserialize<paxos::Message>(string_obj);
    ASSERT_EQ(paxos::MessageType::InvalidMessage, message.type);
}
