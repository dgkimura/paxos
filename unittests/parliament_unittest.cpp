#include "gtest/gtest.h"

#include "paxos/customhash.hpp"
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

    std::unordered_map<MessageType, std::vector<Callback>> registered_map;
};


class ParliamentTest: public testing::Test
{
    virtual void SetUp()
    {
        replica = Replica("myhost", 111);
        legislators = std::make_shared<ReplicaSet>();
        legislators->Add(replica);

        queue = std::make_shared<RolloverQueue<Decree>>(sstream);
        ledger = std::make_shared<Ledger>(queue);
        receiver = std::make_shared<MockReceiver>();
        sender = std::make_shared<MockSender>();
        auto signal = std::make_shared<Signal>();
        auto proposer = std::make_shared<ProposerContext>(
            legislators,
            ledger,
            std::make_shared<VolatileDecree>(),
            [](std::string entry){},
            std::make_shared<NoPause>(),
            signal
        );
        auto acceptor = std::make_shared<AcceptorContext>(
            std::make_shared<VolatileDecree>(),
            std::make_shared<VolatileDecree>(),
            std::chrono::milliseconds(1000)
        );
        auto learner = std::make_shared<LearnerContext>(
            legislators,
            ledger
        );
        signal->Set(true);

        parliament = std::make_shared<Parliament>(
            replica,
            legislators,
            ledger,
            receiver,
            sender,
            acceptor,
            proposer,
            learner
        );
    }

public:

    int GetQueueSize(std::shared_ptr<RolloverQueue<Decree>> queue)
    {
        int size = 0;
        for (auto d : *queue)
        {
            size += 1;
        }
        return size;
    }

    Replica replica;

    std::shared_ptr<ReplicaSet> legislators;

    std::shared_ptr<MockSender> sender;

    std::shared_ptr<MockReceiver> receiver;

    std::stringstream sstream;

    std::shared_ptr<RolloverQueue<Decree>> queue;

    std::shared_ptr<Ledger> ledger;

    std::shared_ptr<Parliament> parliament;
};


TEST_F(ParliamentTest, testAddLegislatorSendsRequestMessage)
{
    parliament->AddLegislator("yourhost", 222);

    ASSERT_EQ(DecreeType::AddReplicaDecree,
              sender->sentMessages()[0].decree.type);
    ASSERT_EQ(MessageType::RequestMessage, sender->sentMessages()[0].type);
}


TEST_F(ParliamentTest, testRemoveLegislatorSendsRequestMessage)
{
    parliament->RemoveLegislator("yourhost", 222);

    ASSERT_EQ(DecreeType::RemoveReplicaDecree,
              sender->sentMessages()[0].decree.type);
    ASSERT_EQ(MessageType::RequestMessage, sender->sentMessages()[0].type);
}


TEST_F(ParliamentTest, testGetLegislatorsReturnsAllLegislators)
{
    ASSERT_EQ(parliament->GetLegislators()->GetSize(), 1);
}


TEST_F(ParliamentTest, testBasicSendProposalSendsProposal)
{
    parliament->SendProposal("Pinky says, 'Narf!'");

    ASSERT_EQ(MessageType::RequestMessage, sender->sentMessages()[0].type);
    ASSERT_EQ("Pinky says, 'Narf!'", sender->sentMessages()[0].decree.content);
}


TEST_F(ParliamentTest, testSetActiveEnablesAppendIntoLedger)
{
    parliament->SetActive();

    receiver->ReceiveMessage(
        Message(
            Decree(replica, 1, "my decree content", DecreeType::UserDecree),
            replica,
            replica,
            MessageType::AcceptedMessage
        )
    );

    ASSERT_EQ(1, GetQueueSize(queue));
}


TEST_F(ParliamentTest, testSetInactiveDisablesAppendIntoLedger)
{
    parliament->SetInactive();

    receiver->ReceiveMessage(
        Message(
            Decree(replica, 1, "my decree content", DecreeType::UserDecree),
            replica,
            replica,
            MessageType::AcceptedMessage
        )
    );

    ASSERT_EQ(0, GetQueueSize(queue));
}


TEST_F(ParliamentTest, testGetAbsenteeBallotWithMultipleReplicaSet)
{
    legislators->Add(Replica("yourhost", 2222));
    legislators->Add(Replica("ourhost", 1111));

    Decree decree(replica, 1, "my decree content", DecreeType::UserDecree);
    ledger->Append(decree);
    receiver->ReceiveMessage(
        Message(
            decree,
            replica,
            replica,
            MessageType::AcceptedMessage
        )
    );

    ASSERT_EQ(1, parliament->GetAbsenteeBallots(100).size());
    ASSERT_TRUE(parliament->GetAbsenteeBallots(100)[decree]->Contains(Replica("yourhost", 2222)));
    ASSERT_TRUE(parliament->GetAbsenteeBallots(100)[decree]->Contains(Replica("ourhost", 1111)));
    ASSERT_FALSE(parliament->GetAbsenteeBallots(100)[decree]->Contains(replica));
}


TEST_F(ParliamentTest, testGetAbsenteeBallotWithMultipleDecrees)
{
    ledger->Append(Decree(replica, 1, "my decree 1", DecreeType::UserDecree));
    ledger->Append(Decree(replica, 2, "my decree 2", DecreeType::UserDecree));
    ledger->Append(Decree(replica, 3, "my decree 3", DecreeType::UserDecree));
    ledger->Append(Decree(replica, 4, "my decree 4", DecreeType::UserDecree));
    ledger->Append(Decree(replica, 5, "my decree 5", DecreeType::UserDecree));
    ledger->Append(Decree(replica, 6, "my decree 6", DecreeType::UserDecree));

    ASSERT_EQ(5, parliament->GetAbsenteeBallots(5).size());
}


TEST_F(ParliamentTest, testGetAbsenteeBallotIfDecreeIsNotInPromiseMap)
{
    Decree decree1(replica, 1, "my decree 1", DecreeType::UserDecree);

    ledger->Append(decree1);

    ASSERT_EQ(1, parliament->GetAbsenteeBallots(5).size());
    ASSERT_EQ(0, parliament->GetAbsenteeBallots(5)[decree1]->GetSize());
}
