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


TEST(SerializationUnitTest, testUpdateReplicaSetDecreeIsSerializableAndDeserializable)
{
    UpdateReplicaSetDecree expected
    {
        Replica("author"),
        Replica("myhost"),
        "remote/directory/path"
    }, actual;

    std::string string_obj = Serialize(expected);
    actual = Deserialize<UpdateReplicaSetDecree>(string_obj);

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.replica.hostname, actual.replica.hostname);
    ASSERT_EQ(expected.replica.port, actual.replica.port);
    ASSERT_EQ(expected.remote_directory, actual.remote_directory);
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


TEST(SerializationUnitTest, testBootstrapMetadataIsSerializableAndDeserializable)
{
    BootstrapMetadata expected
    {
        "localpath",
        "remotepath"
    },
    actual;

    std::string string_obj = Serialize(expected);
    actual = Deserialize<BootstrapMetadata>(string_obj);

    ASSERT_EQ(expected.local, actual.local);
    ASSERT_EQ(expected.remote, actual.remote);
}


TEST(SerializationUnitTest, testBootstrapFileIsSerializableAndDeserializable)
{
    BootstrapFile expected(
        "the_filename",
        "content of the bootstrap file."),
    actual;

    std::string string_obj = Serialize(expected);
    actual = Deserialize<BootstrapFile>(string_obj);

    ASSERT_EQ(expected.name, actual.name);
    ASSERT_EQ(expected.content, actual.content);
}


TEST(SerializationUnitTest, testSerializationWithPaddedFluffOnTheEndOfTheBuffer)
{
    Decree expected(Replica("an_author_1", 0), 1, "the_decree_contents", DecreeType::UserDecree), actual;

    actual = Deserialize<Decree>(
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
    Decree expected(Replica("an_author_1", 0), 1, "the_decree_contents", DecreeType::UserDecree), actual;

    std::string string_obj = Serialize(Decree(Replica("an_author_1", 0), 1, "the_decree_contents", DecreeType::UserDecree));
    actual = Deserialize<Decree>(string_obj);

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.content, actual.content);
    ASSERT_EQ(expected.number, actual.number);
}


TEST(SerializationUnitTest, testSerializationWithStreamValues)
{
    Decree expected(Replica("an_author_1", 0), 1, "the_decree_contents", DecreeType::UserDecree), actual;

    std::istringstream stream_obj(Serialize(expected));
    actual = Deserialize<Decree>(stream_obj);

    ASSERT_EQ(expected.author.hostname, actual.author.hostname);
    ASSERT_EQ(expected.author.port, actual.author.port);
    ASSERT_EQ(expected.content, actual.content);
    ASSERT_EQ(expected.number, actual.number);
}
