#include "gtest/gtest.h"

#include "paxos/customhash.hpp"
#include "paxos/parliament.hpp"


class MockSender : public paxos::Sender
{
public:

    MockSender()
        : sent_messages(),
          replicaset(std::shared_ptr<paxos::ReplicaSet>(new paxos::ReplicaSet()))
    {
    }

    MockSender(std::shared_ptr<paxos::ReplicaSet> replicaset_)
        : sent_messages(),
          replicaset(replicaset_)
    {
    }

    ~MockSender()
    {
    }

    void Reply(paxos::Message message)
    {
        sent_messages.push_back(message);
    }

    void ReplyAll(paxos::Message message)
    {
        for (auto r : *replicaset)
        {
            sent_messages.push_back(message);
        }
    }

    std::vector<paxos::Message> sentMessages()
    {
        return sent_messages;
    }

private:

    std::vector<paxos::Message> sent_messages;

    std::shared_ptr<paxos::ReplicaSet> replicaset;
};


class MockReceiver : public paxos::Receiver
{
public:

    void RegisterCallback(paxos::Callback&& callback, paxos::MessageType type)
    {
        if (registered_map.find(type) == registered_map.end())
        {
            registered_map[type] = std::vector<paxos::Callback> { std::move(callback) };
        }
        else
        {
            registered_map[type].push_back(std::move(callback));
        }
    }

    void ReceiveMessage(paxos::Message message)
    {
        for (auto callback : registered_map[message.type])
        {
            callback(message);
        }
    }

private:

    std::unordered_map<paxos::MessageType, std::vector<paxos::Callback>> registered_map;
};


class ParliamentTest: public testing::Test
{
    virtual void SetUp()
    {
        replica = paxos::Replica("myhost", 111);
        legislators = std::make_shared<paxos::ReplicaSet>();
        legislators->Add(replica);

        queue = std::make_shared<paxos::RolloverQueue<paxos::Decree>>(sstream);
        ledger = std::make_shared<paxos::Ledger>(queue);
        receiver = std::make_shared<MockReceiver>();
        sender = std::make_shared<MockSender>();
        auto signal = std::make_shared<paxos::Signal>();
        auto proposer = std::make_shared<paxos::ProposerContext>(
            legislators,
            ledger,
            std::make_shared<paxos::VolatileDecree>(),
            [](std::string entry){},
            std::make_shared<paxos::NoPause>(),
            signal
        );
        auto acceptor = std::make_shared<paxos::AcceptorContext>(
            std::make_shared<paxos::VolatileDecree>(),
            std::make_shared<paxos::VolatileDecree>(),
            std::chrono::milliseconds(1000)
        );
        auto learner = std::make_shared<paxos::LearnerContext>(
            legislators,
            ledger
        );
        signal->Set(true);

        parliament = std::make_shared<paxos::Parliament>(
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

    int GetQueueSize(std::shared_ptr<paxos::RolloverQueue<paxos::Decree>> queue)
    {
        int size = 0;
        for (auto d : *queue)
        {
            size += 1;
        }
        return size;
    }

    paxos::Replica replica;

    std::shared_ptr<paxos::ReplicaSet> legislators;

    std::shared_ptr<MockSender> sender;

    std::shared_ptr<MockReceiver> receiver;

    std::stringstream sstream;

    std::shared_ptr<paxos::RolloverQueue<paxos::Decree>> queue;

    std::shared_ptr<paxos::Ledger> ledger;

    std::shared_ptr<paxos::Parliament> parliament;
};


TEST_F(ParliamentTest, testAddLegislatorSendsRequestMessage)
{
    parliament->AddLegislator("yourhost", 222);

    ASSERT_EQ(paxos::DecreeType::AddReplicaDecree,
              sender->sentMessages()[0].decree.type);
    ASSERT_EQ(paxos::MessageType::RequestMessage, sender->sentMessages()[0].type);
}


TEST_F(ParliamentTest, testRemoveLegislatorSendsRequestMessage)
{
    parliament->RemoveLegislator("yourhost", 222);

    ASSERT_EQ(paxos::DecreeType::RemoveReplicaDecree,
              sender->sentMessages()[0].decree.type);
    ASSERT_EQ(paxos::MessageType::RequestMessage, sender->sentMessages()[0].type);
}


TEST_F(ParliamentTest, testGetLegislatorsReturnsAllLegislators)
{
    ASSERT_EQ(parliament->GetLegislators()->GetSize(), 1);
}


TEST_F(ParliamentTest, testBasicSendProposalSendsProposal)
{
    parliament->SendProposal("Pinky says, 'Narf!'");

    ASSERT_EQ(paxos::MessageType::RequestMessage, sender->sentMessages()[0].type);
    ASSERT_EQ("Pinky says, 'Narf!'", sender->sentMessages()[0].decree.content);
}


TEST_F(ParliamentTest, testSetActiveEnablesAppendIntoLedger)
{
    parliament->SetActive();

    receiver->ReceiveMessage(
        paxos::Message(
            paxos::Decree(replica, 1, "my decree content", paxos::DecreeType::UserDecree),
            replica,
            replica,
            paxos::MessageType::AcceptedMessage
        )
    );

    ASSERT_EQ(1, GetQueueSize(queue));
}


TEST_F(ParliamentTest, testSetInactiveDisablesAppendIntoLedger)
{
    parliament->SetInactive();

    receiver->ReceiveMessage(
        paxos::Message(
            paxos::Decree(replica, 1, "my decree content", paxos::DecreeType::UserDecree),
            replica,
            replica,
            paxos::MessageType::AcceptedMessage
        )
    );

    ASSERT_EQ(0, GetQueueSize(queue));
}


TEST_F(ParliamentTest, testGetAbsenteeBallotWithMultipleReplicaSet)
{
    legislators->Add(paxos::Replica("yourhost", 2222));
    legislators->Add(paxos::Replica("ourhost", 1111));

    paxos::Decree decree(paxos::Replica("myhost"), 111, "my decree content", paxos::DecreeType::UserDecree);
    ledger->Append(decree);
    receiver->ReceiveMessage(
        paxos::Message(
            decree,
            replica,
            replica,
            paxos::MessageType::AcceptedMessage
        )
    );

    ASSERT_EQ(1, parliament->GetAbsenteeBallots(100).size());
    ASSERT_TRUE(parliament->GetAbsenteeBallots(100)[decree]->Contains(paxos::Replica("yourhost", 2222)));
    ASSERT_TRUE(parliament->GetAbsenteeBallots(100)[decree]->Contains(paxos::Replica("ourhost", 1111)));
    ASSERT_FALSE(parliament->GetAbsenteeBallots(100)[decree]->Contains(replica));
}


TEST_F(ParliamentTest, testGetAbsenteeBallotWithMultipleDecrees)
{
    ledger->Append(paxos::Decree(replica, 1, "my decree 1", paxos::DecreeType::UserDecree));
    ledger->Append(paxos::Decree(replica, 2, "my decree 2", paxos::DecreeType::UserDecree));
    ledger->Append(paxos::Decree(replica, 3, "my decree 3", paxos::DecreeType::UserDecree));
    ledger->Append(paxos::Decree(replica, 4, "my decree 4", paxos::DecreeType::UserDecree));
    ledger->Append(paxos::Decree(replica, 5, "my decree 5", paxos::DecreeType::UserDecree));
    ledger->Append(paxos::Decree(replica, 6, "my decree 6", paxos::DecreeType::UserDecree));

    ASSERT_EQ(5, parliament->GetAbsenteeBallots(5).size());
}


TEST_F(ParliamentTest, testGetAbsenteeBallotIfDecreeIsNotInPromiseMap)
{
    paxos::Decree decree1(replica, 1, "my decree 1", paxos::DecreeType::UserDecree);

    ledger->Append(decree1);

    ASSERT_EQ(1, parliament->GetAbsenteeBallots(5).size());
    ASSERT_EQ(0, parliament->GetAbsenteeBallots(5)[decree1]->GetSize());
}


TEST_F(ParliamentTest, testAbsenteeBallotsIsReplicaInsensitive)
{
    paxos::AbsenteeBallots ballots;

    paxos::Decree my_decree(paxos::Replica("my_replica", 1), 1, "my decree 1", paxos::DecreeType::UserDecree);
    paxos::Decree your_decree(paxos::Replica("your_replica", 2), 1, "your decree 1", paxos::DecreeType::UserDecree);
    paxos::Decree their_decree(paxos::Replica("their_replica", 3), 2, "their decree 2", paxos::DecreeType::UserDecree);

    ballots[my_decree] = std::make_shared<paxos::ReplicaSet>();
    ballots[your_decree] = std::make_shared<paxos::ReplicaSet>();

    // my_decree (1) and your_decree(1) are the same decree number so we expect
    // that the first insert is overwritten by the second
    ASSERT_EQ(1, ballots.size());

    ballots[their_decree] = std::make_shared<paxos::ReplicaSet>();

    // their_decree (2) is a different decree number than my_decree (1) and
    // your_decree (1) so we expect a new entry in the ballots map
    ASSERT_EQ(2, ballots.size());
}


TEST_F(ParliamentTest, testGetAbsenteeBallotsOf1WithAcceptedDecreeComparesAgainstAcceptedMap)
{
    legislators->Add(paxos::Replica("yourhost", 2222));

    paxos::Decree decree(paxos::Replica("myhost"), 1, "my decree content", paxos::DecreeType::UserDecree);
    ledger->Append(decree);
    receiver->ReceiveMessage(
        paxos::Message(
            decree,
            replica,
            replica,
            paxos::MessageType::AcceptedMessage
        )
    );

    // Get 1 ballot compares against accepted map
    ASSERT_TRUE(parliament->GetAbsenteeBallots(1)[decree]->Contains(paxos::Replica("yourhost", 2222)));
    ASSERT_FALSE(parliament->GetAbsenteeBallots(1)[decree]->Contains(replica));
}
