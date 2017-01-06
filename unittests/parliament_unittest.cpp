#include "gtest/gtest.h"

#include "paxos/parliament.hpp"


class FakeSender : public Sender
{
public:

    FakeSender()
        : sent_messages(),
          replicaset(std::shared_ptr<ReplicaSet>(new ReplicaSet()))
    {
    }

    FakeSender(std::shared_ptr<ReplicaSet> replicaset_)
        : sent_messages(),
          replicaset(replicaset_)
    {
    }

    ~FakeSender()
    {
    }

    void Reply(Message message)
    {
        sent_messages.push_back(message);
    }

    void ReplyAll(Message message)
    {
        for (auto r : *replicaset)
        {
            sent_messages.push_back(message);
        }
    }

    std::vector<Message> sentMessages()
    {
        return sent_messages;
    }

private:

    std::vector<Message> sent_messages;

    std::shared_ptr<ReplicaSet> replicaset;
};


class FakeReceiver : public Receiver
{
public:

    void RegisterCallback(Callback&& callback, MessageType type)
    {
        registered_set.insert(type);
    }

private:

    std::set<MessageType> registered_set;
};


TEST(ParliamentTest, testAddLegislatorSendsRequestMessage)
{
    Replica replica("myhost", 111);
    auto legislators = std::make_shared<ReplicaSet>();
    legislators->Add(replica);
    auto ledger = std::make_shared<Ledger>(std::make_shared<VolatileQueue<Decree>>());
    auto receiver = std::make_shared<FakeReceiver>();
    auto sender = std::make_shared<FakeSender>();

    Parliament parliament(replica, legislators, ledger, receiver, sender);

    parliament.AddLegislator("yourhost", 222);

    ASSERT_EQ(SystemOperation::AddReplica,
              Deserialize<SystemDecree>(sender->sentMessages()[0].decree.content).operation);
    ASSERT_EQ(MessageType::RequestMessage, sender->sentMessages()[0].type);
}


TEST(ParliamentTest, testRemoveLegislatorSendsRequestMessage)
{
    Replica replica("myhost", 111);
    auto legislators = std::make_shared<ReplicaSet>();
    legislators->Add(replica);
    auto ledger = std::make_shared<Ledger>(std::make_shared<VolatileQueue<Decree>>());
    auto receiver = std::make_shared<FakeReceiver>();
    auto sender = std::make_shared<FakeSender>();

    Parliament parliament(replica, legislators, ledger, receiver, sender);

    parliament.RemoveLegislator("yourhost", 222);

    ASSERT_EQ(SystemOperation::RemoveReplica,
              Deserialize<SystemDecree>(sender->sentMessages()[0].decree.content).operation);
    ASSERT_EQ(MessageType::RequestMessage, sender->sentMessages()[0].type);
}


TEST(ParliamentTest, testBasicSendProposalSendsProposal)
{
    Replica replica("myhost", 111);
    auto legislators = std::make_shared<ReplicaSet>();
    legislators->Add(replica);
    auto ledger = std::make_shared<Ledger>(std::make_shared<VolatileQueue<Decree>>());
    auto receiver = std::make_shared<FakeReceiver>();
    auto sender = std::make_shared<FakeSender>();

    Parliament parliament(replica, legislators, ledger, receiver, sender);

    parliament.SendProposal("Pinky says, 'Narf!'");

    ASSERT_EQ(MessageType::RequestMessage, sender->sentMessages()[0].type);
    ASSERT_EQ("Pinky says, 'Narf!'", sender->sentMessages()[0].decree.content);
}
