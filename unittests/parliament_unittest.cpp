#include "gtest/gtest.h"

#include "paxos/parliament.hpp"


class MockSender : public Sender
{
public:

    MockSender()
        : sent_messages(),
          replicaset(std::shared_ptr<ReplicaSet>(new ReplicaSet()))
    {
    }

    MockSender(std::shared_ptr<ReplicaSet> replicaset_)
        : sent_messages(),
          replicaset(replicaset_)
    {
    }

    ~MockSender()
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


class MockReceiver : public Receiver
{
public:

    void RegisterCallback(Callback&& callback, MessageType type)
    {
        if (registered_map.find(type) == registered_map.end())
        {
            registered_map[type] = std::vector<Callback> { std::move(callback) };
        }
        else
        {
            registered_map[type].push_back(std::move(callback));
        }
    }

    void ReceiveMessage(Message message)
    {
        for (auto callback : registered_map[message.type])
        {
            callback(message);
        }
    }

private:

    std::unordered_map<MessageType, std::vector<Callback>, MessageTypeHash> registered_map;
};


class ParliamentTest: public testing::Test
{
    virtual void SetUp()
    {
        replica = Replica("myhost", 111);
        legislators = std::make_shared<ReplicaSet>();
        legislators->Add(replica);

        ledger = std::make_shared<Ledger>(std::make_shared<VolatileQueue<Decree>>());
        receiver = std::make_shared<MockReceiver>();
        sender = std::make_shared<MockSender>();

        parliament = Parliament(replica, legislators, ledger, receiver, sender);
    }

public:

    Replica replica;

    std::shared_ptr<ReplicaSet> legislators;

    std::shared_ptr<MockSender> sender;

    std::shared_ptr<MockReceiver> receiver;

    std::shared_ptr<Ledger> ledger;

    Parliament parliament;
};


TEST_F(ParliamentTest, testAddLegislatorSendsRequestMessage)
{
    parliament.AddLegislator("yourhost", 222);

    ASSERT_EQ(SystemOperation::AddReplica,
              Deserialize<SystemDecree>(sender->sentMessages()[0].decree.content).operation);
    ASSERT_EQ(MessageType::RequestMessage, sender->sentMessages()[0].type);
}


TEST_F(ParliamentTest, testRemoveLegislatorSendsRequestMessage)
{
    parliament.RemoveLegislator("yourhost", 222);

    ASSERT_EQ(SystemOperation::RemoveReplica,
              Deserialize<SystemDecree>(sender->sentMessages()[0].decree.content).operation);
    ASSERT_EQ(MessageType::RequestMessage, sender->sentMessages()[0].type);
}


TEST_F(ParliamentTest, testBasicSendProposalSendsProposal)
{
    parliament.SendProposal("Pinky says, 'Narf!'");

    ASSERT_EQ(MessageType::RequestMessage, sender->sentMessages()[0].type);
    ASSERT_EQ("Pinky says, 'Narf!'", sender->sentMessages()[0].decree.content);
}


TEST_F(ParliamentTest, testSetActiveEnablesAppendIntoLedger)
{
    parliament.SetActive();

    receiver->ReceiveMessage(
        Message(
            Decree(replica, 1, "my decree content", DecreeType::UserDecree),
            replica,
            replica,
            MessageType::AcceptedMessage
        )
    );

    ASSERT_EQ(1, ledger->Size());
}


TEST_F(ParliamentTest, testSetInactiveDisablesAppendIntoLedger)
{
    parliament.SetInactive();

    receiver->ReceiveMessage(
        Message(
            Decree(replica, 1, "my decree content", DecreeType::UserDecree),
            replica,
            replica,
            MessageType::AcceptedMessage
        )
    );

    ASSERT_EQ(0, ledger->Size());
}
