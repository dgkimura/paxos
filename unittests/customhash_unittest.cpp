#include "gtest/gtest.h"

#include "paxos/customhash.hpp"


TEST(MessageTest, testMessageTypeHashFunction)
{
    ASSERT_EQ(0, std::hash<MessageType>{}(MessageType::RequestMessage));
    ASSERT_EQ(1, std::hash<MessageType>{}(MessageType::RetryRequestMessage));
    ASSERT_EQ(2, std::hash<MessageType>{}(MessageType::PrepareMessage));
    ASSERT_EQ(3, std::hash<MessageType>{}(MessageType::RetryPrepareMessage));
    ASSERT_EQ(4, std::hash<MessageType>{}(MessageType::PromiseMessage));
    ASSERT_EQ(5, std::hash<MessageType>{}(MessageType::NackTieMessage));
    ASSERT_EQ(6, std::hash<MessageType>{}(MessageType::NackMessage));
    ASSERT_EQ(7, std::hash<MessageType>{}(MessageType::AcceptMessage));
    ASSERT_EQ(8, std::hash<MessageType>{}(MessageType::AcceptedMessage));
    ASSERT_EQ(9, std::hash<MessageType>{}(MessageType::ResumeMessage));
    ASSERT_EQ(10, std::hash<MessageType>{}(MessageType::UpdateMessage));
    ASSERT_EQ(11, std::hash<MessageType>{}(MessageType::UpdatedMessage));
}


TEST(MessageTest, testDecreeTypeHashFunction)
{
    ASSERT_EQ(0, std::hash<DecreeType>{}(DecreeType::UserDecree));
    ASSERT_EQ(1, std::hash<DecreeType>{}(DecreeType::AddReplicaDecree));
    ASSERT_EQ(2, std::hash<DecreeType>{}(DecreeType::RemoveReplicaDecree));
}
