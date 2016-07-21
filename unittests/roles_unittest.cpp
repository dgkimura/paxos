#include <initializer_list>
#include <string>

#include "gtest/gtest.h"

#include "logging.hpp"
#include "roles.hpp"


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


std::shared_ptr<LearnerContext> createLearnerContext(std::initializer_list<std::string> authors)
{
    std::shared_ptr<ReplicaSet> replicaset(new ReplicaSet());
    for (auto a : authors)
    {
        Replica r(a);
        replicaset->Add(r);
    }

    std::shared_ptr<VolatileLedger> ledger = std::make_shared<VolatileLedger>();
    return std::make_shared<LearnerContext>(replicaset, ledger);
}


std::shared_ptr<AcceptorContext> createAcceptorContext()
{
    return std::make_shared<AcceptorContext>(
        std::make_shared<VolatileDecree>(),
        std::make_shared<VolatileDecree>()
    );
}


bool WasMessageTypeSent(std::shared_ptr<FakeSender> sender, MessageType type)
{
    bool was_sent = false;
    for (auto m : sender->sentMessages())
    {
        if (m.type == type)
        {
            was_sent = true;
            break;
        }
    }
    return was_sent;
}


void
ASSERT_MESSAGE_TYPE_SENT(std::shared_ptr<FakeSender> sender, MessageType type)
{
    ASSERT_TRUE(WasMessageTypeSent(sender, type));
}


void
ASSERT_MESSAGE_TYPE_NOT_SENT(std::shared_ptr<FakeSender> sender, MessageType type)
{
    ASSERT_FALSE(WasMessageTypeSent(sender, type));
}


class ProposerTest: public testing::Test
{
    virtual void SetUp()
    {
        DisableLogging();
    }
};


TEST_F(ProposerTest, testHandlePromiseWithLowerDecreeDoesNotUpdatesighestPromisedDecree)
{
    Message message(
        Decree("the_author", -1, ""),
        Replica("from"),
        Replica("to"),
        MessageType::PromiseMessage);

    std::shared_ptr<ProposerContext> context(new ProposerContext(
        std::make_shared<ReplicaSet>(), 0));
    context->highest_proposed_decree = Decree("the_author", 0, "");

    std::shared_ptr<FakeSender> sender(new FakeSender());

    HandlePromise(message, context, sender);

    ASSERT_TRUE(IsDecreeLower(message.decree, context->highest_proposed_decree));
    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, MessageType::AcceptMessage);
}


TEST_F(ProposerTest, testHandlePromiseWithHigherDecreeUpdatesHighestPromisedDecree)
{
    Message message(Decree("host", 1, ""), Replica("host"), Replica("host"), MessageType::PromiseMessage);

    std::shared_ptr<ProposerContext> context(new ProposerContext(
        std::make_shared<ReplicaSet>(), 0));
    context->highest_proposed_decree = Decree("host", 0, "");
    context->replicaset = std::shared_ptr<ReplicaSet>(new ReplicaSet());
    context->replicaset->Add(Replica("host"));

    std::shared_ptr<FakeSender> sender(new FakeSender(context->replicaset));

    HandlePromise(message, context, sender);

    ASSERT_TRUE(IsDecreeEqual(message.decree, context->highest_proposed_decree));
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::AcceptMessage);
}


TEST_F(ProposerTest, testHandlePromiseWithHigherDecreeFromUnknownReplicaDoesNotUpdateHighestPromisedDecree)
{
    Message message(Decree("host", 1, ""), Replica("unknown_host"), Replica("host"), MessageType::PromiseMessage);

    auto context = std::make_shared<ProposerContext>(
        std::make_shared<ReplicaSet>(), 0);
    context->highest_proposed_decree = Decree("host", 0, "");
    context->replicaset = std::shared_ptr<ReplicaSet>(new ReplicaSet());
    context->replicaset->Add(Replica("host"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandlePromise(message, context, sender);

    ASSERT_FALSE(IsDecreeEqual(message.decree, context->highest_proposed_decree));
    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, MessageType::AcceptMessage);
}


class AcceptorTest: public testing::Test
{
    virtual void SetUp()
    {
        DisableLogging();
    }
};


TEST_F(AcceptorTest, testHandlePrepareWithHigherDecreeUpdatesPromisedDecree)
{
    Message message(Decree("the_author", 1, ""), Replica("from"), Replica("to"), MessageType::PrepareMessage);

    std::shared_ptr<AcceptorContext> context = createAcceptorContext();
    context->promised_decree = Decree("the_author", 0, "");

    std::shared_ptr<FakeSender> sender(new FakeSender());

    HandlePrepare(message, context, sender);

    ASSERT_TRUE(IsDecreeEqual(message.decree, context->promised_decree.Value()));
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::PromiseMessage);
}


TEST_F(AcceptorTest, testHandlePrepareWithLowerDecreeDoesNotUpdatePromisedDecree)
{
    Message message(Decree("the_author", -1, ""), Replica("from"), Replica("to"), MessageType::PrepareMessage);

    std::shared_ptr<AcceptorContext> context = createAcceptorContext();
    context->promised_decree = Decree("the_author", 1, "");

    std::shared_ptr<FakeSender> sender(new FakeSender());

    HandlePrepare(message, context, sender);

    ASSERT_TRUE(IsDecreeLower(message.decree, context->promised_decree.Value()));
}


TEST_F(AcceptorTest, testHandlePrepareWithReplayedPrepareDoesNotSendAnotherPromise)
{
    Message message(Decree("the_author", 1, ""), Replica("from"), Replica("to"), MessageType::PrepareMessage);

    std::shared_ptr<AcceptorContext> context = createAcceptorContext();
    context->promised_decree = Decree("the_author", 1, "");

    std::shared_ptr<FakeSender> sender(new FakeSender());

    HandlePrepare(message, context, sender);

    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, MessageType::PromiseMessage);
}


TEST_F(AcceptorTest, testHandleAcceptWithLowerDecreeDoesNotUpdateAcceptedDecree)
{
    Message message(Decree("the_author", -1, ""), Replica("from"), Replica("to"), MessageType::AcceptMessage);

    std::shared_ptr<AcceptorContext> context = createAcceptorContext();
    context->promised_decree = Decree("the_author", 1, "");
    context->accepted_decree = Decree("the_author", 1, "");

    std::shared_ptr<FakeSender> sender(new FakeSender());

    HandleAccept(message, context, sender);

    ASSERT_TRUE(IsDecreeLower(message.decree, context->accepted_decree.Value()));
}


TEST_F(AcceptorTest, testHandleAcceptWithEqualDecreeDoesNotUpdateAcceptedDecree)
{
    Message message(Decree("the_author", 1, ""), Replica("from"), Replica("to"), MessageType::AcceptMessage);

    std::shared_ptr<AcceptorContext> context = createAcceptorContext();
    context->promised_decree = Decree("the_author", 1, "");
    context->accepted_decree = Decree("the_author", 1, "");

    std::shared_ptr<FakeSender> sender(new FakeSender());

    HandleAccept(message, context, sender);

    ASSERT_TRUE(IsDecreeLowerOrEqual(message.decree, context->accepted_decree.Value()));
}


TEST_F(AcceptorTest, testHandleAcceptWithHigherDecreeDoesUpdateAcceptedDecree)
{
    Message message(Decree("the_author", 2, ""), Replica("from"), Replica("to"), MessageType::AcceptMessage);

    std::shared_ptr<AcceptorContext> context = createAcceptorContext();
    context->promised_decree = Decree("the_author", 1, "");
    context->accepted_decree = Decree("the_author", 1, "");

    std::shared_ptr<FakeSender> sender(new FakeSender());

    HandleAccept(message, context, sender);

    ASSERT_TRUE(IsDecreeEqual(message.decree, context->accepted_decree.Value()));
}


class LearnerTest: public testing::Test
{
    virtual void SetUp()
    {
        DisableLogging();
    }
};


TEST_F(LearnerTest, testProclaimHandleWithSingleReplica)
{
    Message message(Decree("A", 1, ""), Replica("A"), Replica("A"), MessageType::AcceptedMessage);
    std::shared_ptr<LearnerContext> context = createLearnerContext({"A"});

    HandleProclaim(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    ASSERT_EQ(context->ledger->Size(), 1);
}


TEST_F(LearnerTest, testProclaimHandleIgnoresMessagesFromUnknownReplica)
{
    Message message(Decree("A", 1, ""), Replica("Unknown"), Replica("A"), MessageType::AcceptedMessage);
    std::shared_ptr<LearnerContext> context = createLearnerContext({"A"});

    HandleProclaim(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    ASSERT_EQ(context->ledger->Size(), 0);
}


TEST_F(LearnerTest, testProclaimHandleReceivesOneAcceptedWithThreeReplicaSet)
{
    Message message(Decree("A", 1, ""), Replica("A"), Replica("B"), MessageType::AcceptedMessage);
    std::shared_ptr<LearnerContext> context = createLearnerContext({"A", "B", "C"});

    HandleProclaim(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    ASSERT_EQ(context->ledger->Size(), 0);
}


TEST_F(LearnerTest, testProclaimHandleReceivesTwoAcceptedWithThreeReplicaSet)
{
    std::shared_ptr<LearnerContext> context = createLearnerContext({"A", "B", "C"});

    HandleProclaim(
        Message(
            Decree("A", 1, ""),
            Replica("A"),
            Replica("B"),
            MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );
    HandleProclaim(
        Message(
            Decree("A", 1, ""),
            Replica("B"),
            Replica("B"),
            MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );

    ASSERT_EQ(context->ledger->Size(), 1);
}


TEST_F(LearnerTest, testProclaimHandleIgnoresDuplicateAcceptedMessages)
{
    std::shared_ptr<LearnerContext> context = createLearnerContext({"A", "B", "C"});

    HandleProclaim(
        Message(
            Decree("A", 1, ""),
            Replica("A"),
            Replica("B"),
            MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );
    HandleProclaim(
        Message(
            Decree("A", 1, ""),
            Replica("A"),
            Replica("B"),
            MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );

    ASSERT_EQ(context->ledger->Size(), 0);
}


TEST_F(LearnerTest, testProclaimHandleDoesNotWriteInLedgerIfTheLastDecreeInTheLedgerIsOutOfOrder)
{
    auto context = createLearnerContext({"A"});
    auto sender = std::make_shared<FakeSender>();

    HandleProclaim(
        Message(
            Decree("A", 1, ""),
            Replica("A"), Replica("A"),
            MessageType::AcceptedMessage
        ),
        context,
        sender
    );

    // Missing decrees 2-9 so don't write to ledger yet.
    HandleProclaim(
        Message(
            Decree("A", 10, ""),
            Replica("A"), Replica("A"),
            MessageType::AcceptedMessage
        ),
        context,
        sender
    );

    ASSERT_EQ(context->ledger->Size(), 1);
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::UpdateMessage);
}


TEST_F(LearnerTest, testHandleUpdatedWithEmptyLedger)
{
    auto context = createLearnerContext({"A"});
    auto sender = std::make_shared<FakeSender>();

    // Missing decrees 1-9 so don't write to ledger yet.
    HandleUpdated(
        Message(
            Decree("A", 10, ""),
            Replica("A"), Replica("A"),
            MessageType::UpdatedMessage
        ),
        context,
        sender
    );

    ASSERT_EQ(context->ledger->Size(), 0);
}


TEST_F(LearnerTest, testHandleUpdatedReceivesMessageWithNextOrderedDecree)
{
    auto context = createLearnerContext({"A"});
    auto sender = std::make_shared<FakeSender>();

    // Last decree in ledger is 9.
    context->ledger->Append(Decree("A", 9, ""));

    // Receive next ordered decree 10.
    HandleUpdated(
        Message(
            Decree("A", 10, ""),
            Replica("A"), Replica("A"),
            MessageType::UpdatedMessage
        ),
        context,
        sender
    );

    // We should have appended 9 and 10 to our ledger.
    ASSERT_EQ(context->ledger->Size(), 2);
}


TEST_F(LearnerTest, testHandleUpdatedReceivesMessageWithNextOrderedDecreeAndTrackedAdditionalOrderedDecrees)
{
    auto context = createLearnerContext({"A"});
    auto sender = std::make_shared<FakeSender>();

    // We have tracked decrees 2, 3.
    context->tracked_future_decrees.push_back(Decree("A", 2, ""));
    context->tracked_future_decrees.push_back(Decree("A", 3, ""));

    // Receive next ordered decree 1.
    HandleUpdated(
        Message(
            Decree("A", 1, ""),
            Replica("A"), Replica("A"),
            MessageType::UpdatedMessage
        ),
        context,
        sender
    );

    // We should have appended 1, 2, and 3 to our ledger.
    ASSERT_EQ(context->ledger->Size(), 3);
}


TEST_F(LearnerTest, testHandleUpdatedReceivesMessageWithNextOrderedDecreeAndTrackedAdditionalUnorderedDecrees)
{
    auto context = createLearnerContext({"A"});
    auto sender = std::make_shared<FakeSender>();

    // We have tracked decrees 2, 3.
    context->tracked_future_decrees.push_back(Decree("A", 3, ""));
    context->tracked_future_decrees.push_back(Decree("A", 4, ""));

    // Receive next ordered decree 1.
    HandleUpdated(
        Message(
            Decree("A", 1, ""),
            Replica("A"), Replica("A"),
            MessageType::UpdatedMessage
        ),
        context,
        sender
    );

    // We appended 1 to our ledger, but are missing 2 so we can't add 3 and 4.
    ASSERT_EQ(context->ledger->Size(), 1);

    // Since we still have a hole we should send an update message.
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::UpdateMessage);
}
