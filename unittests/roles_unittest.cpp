#include <initializer_list>
#include <string>

#include "gtest/gtest.h"

#include "paxos/logging.hpp"
#include "paxos/roles.hpp"


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
    auto replicaset = std::make_shared<ReplicaSet>();
    for (auto a : authors)
    {
        Replica r(a);
        replicaset->Add(r);
    }

    auto ledger = std::make_shared<Ledger>(std::make_shared<VolatileQueue<Decree>>());
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


TEST_F(ProposerTest, testHandleRequestAllowsOnlyOneInProgressProposal)
{
    auto context = std::make_shared<ProposerContext>(std::make_shared<ReplicaSet>(), 0);
    context->replicaset->Add(Replica("host"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandleRequest(
        Message(
            Decree(Replica("host"), -1, "first"),
            Replica("host"),
            Replica("B"),
            MessageType::RequestMessage
        ),
        context,
        sender
    );
    HandleRequest(
        Message(
            Decree(Replica("host"), -1, "second"),
            Replica("host"),
            Replica("B"),
            MessageType::RequestMessage
        ),
        context,
        sender
    );
    HandleRequest(
        Message(
            Decree(Replica("host"), -1, "third"),
            Replica("host"),
            Replica("B"),
            MessageType::RequestMessage
        ),
        context,
        sender
    );

    ASSERT_EQ(1, sender->sentMessages().size());
}


TEST_F(ProposerTest, testHandlePromiseWithLowerDecreeDoesNotUpdatesighestPromisedDecree)
{
    Message message(
        Decree(Replica("the_author"), -1, ""),
        Replica("from"),
        Replica("to"),
        MessageType::PromiseMessage);

    auto context = std::make_shared<ProposerContext>(std::make_shared<ReplicaSet>(), 0);
    context->highest_proposed_decree = Decree(Replica("the_author"), 0, "");

    std::shared_ptr<FakeSender> sender(new FakeSender());

    HandlePromise(message, context, sender);

    ASSERT_TRUE(IsDecreeLower(message.decree, context->highest_proposed_decree));
    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, MessageType::AcceptMessage);
}


TEST_F(ProposerTest, testHandlePromiseWithHigherDecreeUpdatesHighestPromisedDecree)
{
    Message message(Decree(Replica("host"), 1, ""), Replica("host"), Replica("host"), MessageType::PromiseMessage);

    auto context = std::make_shared<ProposerContext>(std::make_shared<ReplicaSet>(), 0);
    context->highest_proposed_decree = Decree(Replica("host"), 0, "");
    context->replicaset = std::make_shared<ReplicaSet>();
    context->replicaset->Add(Replica("host"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandlePromise(message, context, sender);

    ASSERT_TRUE(IsDecreeEqual(message.decree, context->highest_proposed_decree));
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::AcceptMessage);
}


TEST_F(ProposerTest, testHandlePromiseWithHigherDecreeFromUnknownReplicaDoesNotUpdateHighestPromisedDecree)
{
    Message message(Decree(Replica("host"), 1, ""), Replica("unknown_host"), Replica("host"), MessageType::PromiseMessage);

    auto context = std::make_shared<ProposerContext>(
        std::make_shared<ReplicaSet>(), 0);
    context->highest_proposed_decree = Decree(Replica("host"), 0, "");
    context->replicaset = std::shared_ptr<ReplicaSet>(new ReplicaSet());
    context->replicaset->Add(Replica("host"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandlePromise(message, context, sender);

    ASSERT_FALSE(IsDecreeEqual(message.decree, context->highest_proposed_decree));
    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, MessageType::AcceptMessage);
}


TEST_F(ProposerTest, testHandleRequestWithMultipleInProgressIncrementsCurrentDecreeNumberOnce)
{
    auto context = std::make_shared<ProposerContext>(std::make_shared<ReplicaSet>(), 0);
    context->replicaset->Add(Replica("host"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    // Increments current by one.
    HandleRequest(
        Message(
            Decree(Replica("host"), -1, "first"),
            Replica("host"),
            Replica("B"),
            MessageType::RequestMessage
        ),
        context,
        sender
    );
    // Same round, do not increment.
    HandleRequest(
        Message(
            Decree(Replica("host"), -1, "second"),
            Replica("host"),
            Replica("B"),
            MessageType::RequestMessage
        ),
        context,
        sender
    );

    ASSERT_EQ(context->current_decree_number, 1);
}


TEST_F(ProposerTest, testHandleNackRemovesReplicaFromPromisedMapAndSendsRetryPrepareMessage)
{
    auto replica = Replica("host");
    auto decree = Decree(replica, -1, "first");
    auto context = std::make_shared<ProposerContext>(std::make_shared<ReplicaSet>(), 0);

    // Add replica to known replicas.
    context->replicaset->Add(replica);
    context->promise_map[decree] = std::make_shared<ReplicaSet>();

    // Set previously promised replica.
    context->promise_map[decree]->Add(replica);

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    ASSERT_TRUE(context->promise_map[decree]->Contains(replica));

    HandleNack(
        Message(
            decree,
            replica,
            replica,
            MessageType::NackMessage
        ),
        context,
        sender
    );

    ASSERT_FALSE(context->promise_map[decree]->Contains(replica));
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::RetryPrepareMessage);
}


TEST_F(ProposerTest, testHandleAcceptedThenHandleRequestIncrementsCurrentDecreeNumber)
{
    auto context = std::make_shared<ProposerContext>(std::make_shared<ReplicaSet>(), 0);
    context->replicaset->Add(Replica("host"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    // Increments current by one.
    HandleRequest(
        Message(
            Decree(Replica("host"), -1, "second"),
            Replica("host"),
            Replica("B"),
            MessageType::RequestMessage
        ),
        context,
        sender
    );
    // Signals round finished.
    HandleAccepted(
        Message(
            Decree(Replica("host"), -1, "first"),
            Replica("host"),
            Replica("B"),
            MessageType::AcceptedMessage
        ),
        context,
        sender
    );
    // Next round, increments current by one.
    HandleRequest(
        Message(
            Decree(Replica("host"), -1, "second"),
            Replica("host"),
            Replica("B"),
            MessageType::RequestMessage
        ),
        context,
        sender
    );

    ASSERT_EQ(context->current_decree_number, 2);
}


TEST_F(ProposerTest, testHandleRetryRequestNeverIncrementsCurrentDecreeNumber)
{
    auto context = std::make_shared<ProposerContext>(std::make_shared<ReplicaSet>(), 0);
    context->replicaset->Add(Replica("host"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    // Retry doesn't increment.
    HandleRetryRequest(
        Message(
            Decree(Replica("host"), -1, "second"),
            Replica("host"),
            Replica("B"),
            MessageType::RequestMessage
        ),
        context,
        sender
    );
    // Signals round finished.
    HandleAccepted(
        Message(
            Decree(Replica("host"), -1, "first"),
            Replica("host"),
            Replica("B"),
            MessageType::AcceptedMessage
        ),
        context,
        sender
    );
    // Next round, retry still doesn't increment.
    HandleRetryRequest(
        Message(
            Decree(Replica("host"), -1, "second"),
            Replica("host"),
            Replica("B"),
            MessageType::RequestMessage
        ),
        context,
        sender
    );

    ASSERT_EQ(context->current_decree_number, 0);
}


TEST_F(ProposerTest, testHandleAcceptRemovesEntriesInThePromiseMap)
{
    Message message(
        Decree(Replica("A"), 1, "content"),
        Replica("from"),
        Replica("to"),
        MessageType::AcceptMessage);

    auto context = std::make_shared<ProposerContext>(std::make_shared<ReplicaSet>(), 0);
    context->promise_map[message.decree] = std::make_shared<ReplicaSet>();
    context->promise_map[message.decree]->Add(message.from);

    ASSERT_EQ(context->promise_map.size(), 1);

    HandleAccepted(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    ASSERT_EQ(context->promise_map.size(), 0);
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
    Message message(Decree(Replica("the_author"), 1, ""), Replica("from"), Replica("to"), MessageType::PrepareMessage);

    std::shared_ptr<AcceptorContext> context = createAcceptorContext();
    context->promised_decree = Decree(Replica("the_author"), 0, "");

    auto sender = std::make_shared<FakeSender>();

    HandlePrepare(message, context, sender);

    ASSERT_TRUE(IsDecreeEqual(message.decree, context->promised_decree.Value()));
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::PromiseMessage);
}


TEST_F(AcceptorTest, testHandlePrepareWithLowerDecreeDoesNotUpdatePromisedDecree)
{
    Message message(Decree(Replica("the_author"), -1, ""), Replica("from"), Replica("to"), MessageType::PrepareMessage);

    auto context = createAcceptorContext();
    context->promised_decree = Decree(Replica("the_author"), 1, "");

    auto sender = std::make_shared<FakeSender>();

    HandlePrepare(message, context, sender);

    ASSERT_TRUE(IsDecreeLower(message.decree, context->promised_decree.Value()));
}


TEST_F(AcceptorTest, testHandlePrepareWithReplayedPrepareDoesNotSendAnotherPromise)
{
    Message message(Decree(Replica("the_author"), 1, ""), Replica("from"), Replica("to"), MessageType::PrepareMessage);

    auto context = createAcceptorContext();
    context->promised_decree = Decree(Replica("the_author"), 1, "");

    auto sender = std::make_shared<FakeSender>();

    HandlePrepare(message, context, sender);

    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, MessageType::PromiseMessage);
}


TEST_F(AcceptorTest, testHandlePrepareWithEqualDecreeNumberFromMultipleReplicasSendsSinglePromise)
{
    Message author_a(Decree(Replica("author_a"), 1, ""), Replica("from"), Replica("to"), MessageType::PrepareMessage);
    Message author_b(Decree(Replica("author_b"), 1, ""), Replica("from"), Replica("to"), MessageType::PrepareMessage);

    auto context = createAcceptorContext();
    context->promised_decree = Decree(Replica("the_author"), 0, "");

    auto sender = std::make_shared<FakeSender>();

    HandlePrepare(author_a, context, sender);
    HandlePrepare(author_b, context, sender);

    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::PromiseMessage);
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::NackMessage);

    // We send 1 accept message and 1 nack message.
    ASSERT_EQ(2, sender->sentMessages().size());
}


TEST_F(AcceptorTest, testHandleRetryPrepareLastSelfishPromises)
{
    auto replica = Replica("the_author");
    auto current_decree = Decree(replica, 1, "");
    auto past_decree = Decree(replica, 2, "");

    auto context = createAcceptorContext();
    context->promised_decree = current_decree;
    context->accepted_decree = past_decree;

    auto sender = std::make_shared<FakeSender>();

    HandleRetryPrepare(
        Message(
            Decree(),
            replica,
            replica,
            MessageType::RetryPrepareMessage
        ),
        context,
        sender
    );

    // Undo promise to self.
    ASSERT_TRUE(IsDecreeEqual(context->promised_decree.Value(), past_decree));
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::RetryRequestMessage);
}


TEST_F(AcceptorTest, testHandleRetryPrepareHasNoEffectOnLastUnselfishPromise)
{
    auto replica = Replica("the_author");
    auto current_decree = Decree(Replica("another_author"), 1, "");
    auto past_decree = Decree(replica, 2, "");

    auto context = createAcceptorContext();
    context->promised_decree = current_decree;
    context->accepted_decree = past_decree;

    auto sender = std::make_shared<FakeSender>();

    HandleRetryPrepare(
        Message(
            Decree(),
            replica,
            replica,
            MessageType::RetryPrepareMessage
        ),
        context,
        sender
    );

    // No promise to self, therefore nothing to undo.
    ASSERT_FALSE(IsDecreeEqual(context->promised_decree.Value(), past_decree));
    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, MessageType::RetryRequestMessage);
}


TEST_F(AcceptorTest, testHandleAcceptWithLowerDecreeDoesNotUpdateAcceptedDecree)
{
    Message message(Decree(Replica("the_author"), -1, ""), Replica("from"), Replica("to"), MessageType::AcceptMessage);

    auto context = createAcceptorContext();
    context->promised_decree = Decree(Replica("the_author"), 1, "");
    context->accepted_decree = Decree(Replica("the_author"), 1, "");

    auto sender = std::make_shared<FakeSender>();

    HandleAccept(message, context, sender);

    ASSERT_TRUE(IsDecreeLower(message.decree, context->accepted_decree.Value()));
}


TEST_F(AcceptorTest, testHandleAcceptWithEqualDecreeDoesNotUpdateAcceptedDecree)
{
    Message message(Decree(Replica("the_author"), 1, ""), Replica("from"), Replica("to"), MessageType::AcceptMessage);

    auto context = createAcceptorContext();
    context->promised_decree = Decree(Replica("the_author"), 1, "");
    context->accepted_decree = Decree(Replica("the_author"), 1, "");

    auto sender = std::make_shared<FakeSender>();

    HandleAccept(message, context, sender);

    ASSERT_TRUE(IsDecreeLowerOrEqual(message.decree, context->accepted_decree.Value()));
}


TEST_F(AcceptorTest, testHandleAcceptWithHigherDecreeDoesUpdateAcceptedDecree)
{
    Message message(Decree(Replica("the_author"), 2, ""), Replica("from"), Replica("to"), MessageType::AcceptMessage);

    auto context = createAcceptorContext();
    context->promised_decree = Decree(Replica("the_author"), 1, "");
    context->accepted_decree = Decree(Replica("the_author"), 1, "");

    auto sender = std::make_shared<FakeSender>();

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
    Message message(Decree(Replica("A"), 1, ""), Replica("A"), Replica("A"), MessageType::AcceptedMessage);
    auto context = createLearnerContext({"A"});

    HandleProclaim(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    ASSERT_EQ(context->ledger->Size(), 1);
}


TEST_F(LearnerTest, testProclaimHandleIgnoresMessagesFromUnknownReplica)
{
    Message message(Decree(Replica("A"), 1, ""), Replica("Unknown"), Replica("A"), MessageType::AcceptedMessage);
    auto context = createLearnerContext({"A"});

    HandleProclaim(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    ASSERT_EQ(context->ledger->Size(), 0);
}


TEST_F(LearnerTest, testProclaimHandleReceivesOneAcceptedWithThreeReplicaSet)
{
    Message message(Decree(Replica("A"), 1, ""), Replica("A"), Replica("B"), MessageType::AcceptedMessage);
    auto context = createLearnerContext({"A", "B", "C"});

    HandleProclaim(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    ASSERT_EQ(context->ledger->Size(), 0);
}


TEST_F(LearnerTest, testProclaimHandleReceivesTwoAcceptedWithThreeReplicaSet)
{
    auto context = createLearnerContext({"A", "B", "C"});

    HandleProclaim(
        Message(
            Decree(Replica("A"), 1, ""),
            Replica("A"),
            Replica("B"),
            MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );
    HandleProclaim(
        Message(
            Decree(Replica("A"), 1, ""),
            Replica("B"),
            Replica("B"),
            MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );

    ASSERT_EQ(context->ledger->Size(), 1);
}


TEST_F(LearnerTest, testProclaimHandleCleansUpAcceptedMapAfterAllVotesReceived)
{
    auto context = createLearnerContext({"A", "B", "C"});
    Decree decree(Replica("A"), 1, "");

    HandleProclaim(
        Message(
            decree,
            Replica("A"),
            Replica("B"),
            MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );

    ASSERT_FALSE(context->accepted_map.find(decree) == context->accepted_map.end());

    HandleProclaim(
        Message(
            decree,
            Replica("B"),
            Replica("B"),
            MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );

    ASSERT_FALSE(context->accepted_map.find(decree) == context->accepted_map.end());

    HandleProclaim(
        Message(
            decree,
            Replica("C"),
            Replica("B"),
            MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );

    ASSERT_TRUE(context->accepted_map.find(decree) == context->accepted_map.end());
}


TEST_F(LearnerTest, testProclaimHandleIgnoresPreviouslyAcceptedMessagesFromReplicasRemovedFromReplicaSet)
{
    auto context = createLearnerContext({"A", "B", "C"});
    Decree decree(Replica("A"), 1, "");

    // Accepted from replica A.
    HandleProclaim(
        Message(
            decree,
            Replica("A"),
            Replica("B"),
            MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );

    // Remove replica A from replicaset.
    context->replicaset->Remove(Replica("A"));

    HandleProclaim(
        Message(
            decree,
            Replica("B"),
            Replica("B"),
            MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );

    // We should not have written to ledger since replica A was removed from replicaset.
    ASSERT_EQ(context->ledger->Size(), 0);
}


TEST_F(LearnerTest, testProclaimHandleIgnoresDuplicateAcceptedMessages)
{
    auto context = createLearnerContext({"A", "B", "C"});

    HandleProclaim(
        Message(
            Decree(Replica("A"), 1, ""),
            Replica("A"),
            Replica("B"),
            MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );
    HandleProclaim(
        Message(
            Decree(Replica("A"), 1, ""),
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
            Decree(Replica("A"), 1, ""),
            Replica("A"), Replica("A"),
            MessageType::AcceptedMessage
        ),
        context,
        sender
    );

    // Missing decrees 2-9 so don't write to ledger yet.
    HandleProclaim(
        Message(
            Decree(Replica("A"), 10, ""),
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
            Decree(Replica("A"), 10, ""),
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
    context->ledger->Append(Decree(Replica("A"), 9, ""));

    // Receive next ordered decree 10.
    HandleUpdated(
        Message(
            Decree(Replica("A"), 10, ""),
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
    context->tracked_future_decrees.push_back(Decree(Replica("A"), 2, ""));
    context->tracked_future_decrees.push_back(Decree(Replica("A"), 3, ""));

    // Receive next ordered decree 1.
    HandleUpdated(
        Message(
            Decree(Replica("A"), 1, ""),
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

    // We have tracked decrees 3, 4.
    context->tracked_future_decrees.push_back(Decree(Replica("A"), 3, ""));
    context->tracked_future_decrees.push_back(Decree(Replica("A"), 4, ""));

    // Receive next ordered decree 1.
    HandleUpdated(
        Message(
            Decree(Replica("A"), 1, ""),
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


class UpdaterTest: public testing::Test
{
    virtual void SetUp()
    {
        DisableLogging();
    }
};


TEST_F(UpdaterTest, testHandleUpdateReceivesMessageWithDecreeAndHasEmptyLedger)
{
    auto context = std::make_shared<UpdaterContext>(
        std::make_shared<Ledger>(
            std::make_shared<VolatileQueue<Decree>>()
        )
    );
    auto sender = std::make_shared<FakeSender>();

    HandleUpdate(
        Message(
            Decree(Replica("A"), 1, ""),
            Replica("A"), Replica("A"),
            MessageType::UpdateMessage
        ),
        context,
        sender
    );

    // Our ledger is empty so we shouldn't send an updated message.
    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, MessageType::UpdatedMessage);
}


TEST_F(UpdaterTest, testHandleUpdateReceivesMessageWithDecreeAndLedgerHasNextDecree)
{
    auto context = std::make_shared<UpdaterContext>(
        std::make_shared<Ledger>(
            std::make_shared<VolatileQueue<Decree>>()
        )
    );
    auto sender = std::make_shared<FakeSender>();

    context->ledger->Append(Decree(Replica("A"), 1, ""));
    context->ledger->Append(Decree(Replica("A"), 2, ""));
    context->ledger->Append(Decree(Replica("A"), 3, ""));

    HandleUpdate(
        Message(
            Decree(Replica("A"), 1, ""),
            Replica("A"), Replica("A"),
            MessageType::UpdateMessage
        ),
        context,
        sender
    );

    // Our ledger contained next decree so we shoul send an updated message.
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::UpdatedMessage);
}
