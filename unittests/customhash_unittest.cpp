#include "gtest/gtest.h"

#include "paxos/customhash.hpp"


TEST(MessageTest, testMessageTypeHashFunction)
{
    ASSERT_EQ(0, std::hash<paxos::MessageType>{}(paxos::MessageType::InvalidMessage));
    ASSERT_EQ(1, std::hash<paxos::MessageType>{}(paxos::MessageType::RequestMessage));
    ASSERT_EQ(2, std::hash<paxos::MessageType>{}(paxos::MessageType::PrepareMessage));
    ASSERT_EQ(3, std::hash<paxos::MessageType>{}(paxos::MessageType::PromiseMessage));
    ASSERT_EQ(4, std::hash<paxos::MessageType>{}(paxos::MessageType::NackTieMessage));
    ASSERT_EQ(5, std::hash<paxos::MessageType>{}(paxos::MessageType::NackPrepareMessage));
    ASSERT_EQ(6, std::hash<paxos::MessageType>{}(paxos::MessageType::AcceptMessage));
    ASSERT_EQ(7, std::hash<paxos::MessageType>{}(paxos::MessageType::NackAcceptMessage));
    ASSERT_EQ(8, std::hash<paxos::MessageType>{}(paxos::MessageType::AcceptedMessage));
    ASSERT_EQ(9, std::hash<paxos::MessageType>{}(paxos::MessageType::ResumeMessage));
    ASSERT_EQ(10, std::hash<paxos::MessageType>{}(paxos::MessageType::UpdateMessage));
    ASSERT_EQ(11, std::hash<paxos::MessageType>{}(paxos::MessageType::UpdatedMessage));
}


TEST(MessageTest, testDecreeTypeHashFunction)
{
    ASSERT_EQ(0, std::hash<paxos::DecreeType>{}(paxos::DecreeType::UserDecree));
    ASSERT_EQ(1, std::hash<paxos::DecreeType>{}(paxos::DecreeType::AddReplicaDecree));
    ASSERT_EQ(2, std::hash<paxos::DecreeType>{}(paxos::DecreeType::RemoveReplicaDecree));
}
