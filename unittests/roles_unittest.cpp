#include <initializer_list>
#include <set>
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


class FakeReceiver : public Receiver
{
public:

    void RegisterCallback(Callback&& callback, MessageType type)
    {
        registered_set.insert(type);
    }

    bool IsMessageTypeRegister(MessageType type)
    {
        return registered_set.find(type) != registered_set.end();
    }

private:

    std::set<MessageType> registered_set;
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


TEST_F(ProposerTest, testRegisterProposerWillRegistereMessageTypes)
{
    auto receiver = std::make_shared<FakeReceiver>();
    auto sender = std::make_shared<FakeSender>();
    auto replicaset = std::make_shared<ReplicaSet>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        std::make_shared<Ledger>(
            std::make_shared<VolatileQueue<Decree>>()
        ),
        std::make_shared<VolatileDecree>()
    );

    RegisterProposer(receiver, sender, context);

    ASSERT_TRUE(receiver->IsMessageTypeRegister(MessageType::RequestMessage));
    ASSERT_TRUE(receiver->IsMessageTypeRegister(MessageType::PromiseMessage));
    ASSERT_TRUE(receiver->IsMessageTypeRegister(MessageType::NackMessage));
    ASSERT_TRUE(receiver->IsMessageTypeRegister(MessageType::AcceptedMessage));

    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::PrepareMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::AcceptMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::UpdateMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::UpdatedMessage));
}


TEST_F(ProposerTest, testHandleRequestAllowsOnlyOneInProgressProposal)
{
    auto replicaset = std::make_shared<ReplicaSet>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        std::make_shared<Ledger>(
            std::make_shared<VolatileQueue<Decree>>()
        ),
        std::make_shared<VolatileDecree>()
    );
    context->replicaset->Add(Replica("host"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandleRequest(
        Message(
            Decree(Replica("host"), -1, "first", DecreeType::UserDecree),
            Replica("host"),
            Replica("B"),
            MessageType::RequestMessage
        ),
        context,
        sender
    );
    HandleRequest(
        Message(
            Decree(Replica("host"), -1, "second", DecreeType::UserDecree),
            Replica("host"),
            Replica("B"),
            MessageType::RequestMessage
        ),
        context,
        sender
    );
    HandleRequest(
        Message(
            Decree(Replica("host"), -1, "third", DecreeType::UserDecree),
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
        Decree(Replica("the_author"), -1, "", DecreeType::UserDecree),
        Replica("from"),
        Replica("to"),
        MessageType::PromiseMessage);

    auto replicaset = std::make_shared<ReplicaSet>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        std::make_shared<Ledger>(
            std::make_shared<VolatileQueue<Decree>>()
        ),
        std::make_shared<VolatileDecree>()
    );
    context->highest_proposed_decree = Decree(Replica("the_author"), 0, "", DecreeType::UserDecree);

    std::shared_ptr<FakeSender> sender(new FakeSender());

    HandlePromise(message, context, sender);

    ASSERT_TRUE(IsDecreeLower(message.decree, context->highest_proposed_decree.Value()));
    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, MessageType::AcceptMessage);
}


TEST_F(ProposerTest, testHandlePromiseWithHigherDecreeUpdatesHighestPromisedDecree)
{
    Message message(Decree(Replica("host"), 1, "", DecreeType::UserDecree), Replica("host"), Replica("host"), MessageType::PromiseMessage);

    auto replicaset = std::make_shared<ReplicaSet>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        std::make_shared<Ledger>(
            std::make_shared<VolatileQueue<Decree>>()
        ),
        std::make_shared<VolatileDecree>()
    );
    context->highest_proposed_decree = Decree(Replica("host"), 0, "", DecreeType::UserDecree);
    context->replicaset = std::make_shared<ReplicaSet>();
    context->replicaset->Add(Replica("host"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandlePromise(message, context, sender);

    ASSERT_TRUE(IsDecreeEqual(message.decree, context->highest_proposed_decree.Value()));
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::AcceptMessage);
}


TEST_F(ProposerTest, testHandlePromiseWithHigherDecreeFromUnknownReplicaDoesNotUpdateHighestPromisedDecree)
{
    Message message(Decree(Replica("host"), 1, "", DecreeType::UserDecree), Replica("unknown_host"), Replica("host"), MessageType::PromiseMessage);

    auto replicaset = std::make_shared<ReplicaSet>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        std::make_shared<Ledger>(
            std::make_shared<VolatileQueue<Decree>>()
        ),
        std::make_shared<VolatileDecree>()
    );
    context->highest_proposed_decree = Decree(Replica("host"), 0, "", DecreeType::UserDecree);
    context->replicaset = std::shared_ptr<ReplicaSet>(new ReplicaSet());
    context->replicaset->Add(Replica("host"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandlePromise(message, context, sender);

    ASSERT_FALSE(IsDecreeEqual(message.decree, context->highest_proposed_decree.Value()));
    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, MessageType::AcceptMessage);
}


TEST_F(ProposerTest, testHandlePromiseWithHigherEmptyDecreeAndExistingRequestedValueThenUpdateDecreeContents)
{
    Message message(Decree(Replica("host"), 1, "", DecreeType::UserDecree), Replica("host"), Replica("host"), MessageType::PromiseMessage);

    auto replicaset = std::make_shared<ReplicaSet>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        std::make_shared<Ledger>(
            std::make_shared<VolatileQueue<Decree>>()
        ),
        std::make_shared<VolatileDecree>()
    );

    // Highest promised decree content is "".
    context->highest_proposed_decree = Decree(Replica("host"), 0, "", DecreeType::UserDecree);
    context->replicaset = std::make_shared<ReplicaSet>();
    context->replicaset->Add(Replica("host"));

    // Requested values contains entry with "new content to be used".
    context->requested_values.push_back(std::make_tuple("new content to be used", DecreeType::UserDecree));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandlePromise(message, context, sender);

    ASSERT_EQ("new content to be used", sender->sentMessages()[0].decree.content);
    ASSERT_EQ(DecreeType::UserDecree, sender->sentMessages()[0].decree.type);
    ASSERT_EQ(0, context->requested_values.size());
}


TEST_F(ProposerTest, testHandlePromiseWillSendAcceptAgainIfDuplicatePromiseIsSent)
{
    Message message(Decree(Replica("host1"), 1, "", DecreeType::UserDecree), Replica("host1"), Replica("host1"), MessageType::PromiseMessage);

    auto replicaset = std::make_shared<ReplicaSet>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        std::make_shared<Ledger>(
            std::make_shared<VolatileQueue<Decree>>()
        ),
        std::make_shared<VolatileDecree>()
    );
    context->highest_proposed_decree = Decree(Replica("host"), 0, "", DecreeType::UserDecree);
    context->replicaset = std::make_shared<ReplicaSet>();
    context->replicaset->Add(Replica("host1"));
    context->replicaset->Add(Replica("host2"));
    context->replicaset->Add(Replica("host3"));
    context->promise_map[message.decree] = std::make_shared<ReplicaSet>();
    context->promise_map[message.decree]->Add(Replica("host1"));
    context->promise_map[message.decree]->Add(Replica("host2"));
    context->promise_map[message.decree]->Add(Replica("host3"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandlePromise(message, context, sender);

    // Accept message is sent to all 3 replicas.
    ASSERT_EQ(3, sender->sentMessages().size());
    ASSERT_EQ(MessageType::AcceptMessage, sender->sentMessages()[0].type);
    ASSERT_EQ(MessageType::AcceptMessage, sender->sentMessages()[1].type);
    ASSERT_EQ(MessageType::AcceptMessage, sender->sentMessages()[2].type);
}


TEST_F(ProposerTest, testHandlePromiseWillNotSendAcceptAgainIfPromiseIsUnique)
{
    Message message(Decree(Replica("host1"), 1, "", DecreeType::UserDecree), Replica("host1"), Replica("host1"), MessageType::PromiseMessage);

    auto replicaset = std::make_shared<ReplicaSet>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        std::make_shared<Ledger>(
            std::make_shared<VolatileQueue<Decree>>()
        ),
        std::make_shared<VolatileDecree>()
    );
    context->highest_proposed_decree = Decree(Replica("host"), 0, "", DecreeType::UserDecree);
    context->replicaset = std::make_shared<ReplicaSet>();
    context->replicaset->Add(Replica("host1"));
    context->replicaset->Add(Replica("host2"));
    context->replicaset->Add(Replica("host3"));
    context->promise_map[message.decree] = std::make_shared<ReplicaSet>();
    context->promise_map[message.decree]->Add(Replica("host2"));
    context->promise_map[message.decree]->Add(Replica("host3"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    // Sending promise from host1, which is not in our promise map yet.
    HandlePromise(message, context, sender);

    // We shouldn't send accept, since this is not a duplicate message and
    // promise_map contains more than minimum quorum (so we should have already
    // sent accept message).
    ASSERT_EQ(0, sender->sentMessages().size());
}


TEST_F(ProposerTest, testHandleRequestWithMultipleInProgressInSendsSingleProposal)
{
    auto replicaset = std::make_shared<ReplicaSet>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        std::make_shared<Ledger>(
            std::make_shared<VolatileQueue<Decree>>()
        ),
        std::make_shared<VolatileDecree>()
    );
    context->replicaset->Add(Replica("host"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    // Increments current by one.
    HandleRequest(
        Message(
            Decree(Replica("host"), -1, "first", DecreeType::UserDecree),
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
            Decree(Replica("host"), -1, "second", DecreeType::UserDecree),
            Replica("host"),
            Replica("B"),
            MessageType::RequestMessage
        ),
        context,
        sender
    );

    ASSERT_EQ(sender->sentMessages()[0].decree.number, 1);
    ASSERT_EQ(sender->sentMessages().size(), 1);
}


TEST_F(ProposerTest, testHandleNackIncrementsDecreeNumberAndResendsPrepareMessage)
{
    auto replica = Replica("host");
    auto decree = Decree(replica, -1, "first", DecreeType::UserDecree);
    auto replicaset = std::make_shared<ReplicaSet>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        std::make_shared<Ledger>(
            std::make_shared<VolatileQueue<Decree>>()
        ),
        std::make_shared<VolatileDecree>()
    );

    // Add replica to known replicas.
    context->replicaset->Add(replica);

    auto sender = std::make_shared<FakeSender>(context->replicaset);

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

    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::PrepareMessage);
    ASSERT_EQ(decree.number + 1, sender->sentMessages()[0].decree.number);
}


TEST_F(ProposerTest, testUpdatingLedgerUpdatesNextProposedDecreeNumber)
{
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<VolatileQueue<Decree>>()
    );
    auto replicaset = std::make_shared<ReplicaSet>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<VolatileDecree>()
    );
    context->replicaset->Add(Replica("host"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    // Current decree is one.
    HandleRequest(
        Message(
            Decree(Replica("host"), -1, "second", DecreeType::UserDecree),
            Replica("host"),
            Replica("B"),
            MessageType::RequestMessage
        ),
        context,
        sender
    );
    ASSERT_EQ(sender->sentMessages()[0].decree.number, 1);

    // Our ledger was updated underneath us to 5.
    context->ledger->Append(
        Decree(Replica("the_author"), 5, "", DecreeType::UserDecree));
    std::atomic_flag_clear(&context->in_progress);

    // Next round, increments current by one.
    HandleRequest(
        Message(
            Decree(Replica("host"), -1, "second", DecreeType::UserDecree),
            Replica("host"),
            Replica("B"),
            MessageType::RequestMessage
        ),
        context,
        sender
    );

    ASSERT_EQ(sender->sentMessages()[1].decree.number, 6);
}


TEST_F(ProposerTest, testHandleAcceptRemovesEntriesInThePromiseMap)
{
    Message message(
        Decree(Replica("A"), 1, "content", DecreeType::UserDecree),
        Replica("from"),
        Replica("to"),
        MessageType::AcceptMessage);

    auto replicaset = std::make_shared<ReplicaSet>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        std::make_shared<Ledger>(
            std::make_shared<VolatileQueue<Decree>>()
        ),
        std::make_shared<VolatileDecree>()
    );
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


TEST_F(AcceptorTest, testRegisterAcceptorWillRegistereMessageTypes)
{
    auto receiver = std::make_shared<FakeReceiver>();
    auto sender = std::make_shared<FakeSender>();
    auto context = createAcceptorContext();

    RegisterAcceptor(receiver, sender, context);

    ASSERT_TRUE(receiver->IsMessageTypeRegister(MessageType::PrepareMessage));
    ASSERT_TRUE(receiver->IsMessageTypeRegister(MessageType::AcceptMessage));

    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::RequestMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::PromiseMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::NackMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::AcceptedMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::UpdateMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::UpdatedMessage));
}


TEST_F(AcceptorTest, testHandlePrepareWithHigherDecreeUpdatesPromisedDecree)
{
    Message message(Decree(Replica("the_author"), 1, "", DecreeType::UserDecree), Replica("from"), Replica("to"), MessageType::PrepareMessage);

    std::shared_ptr<AcceptorContext> context = createAcceptorContext();
    context->promised_decree = Decree(Replica("the_author"), 0, "", DecreeType::UserDecree);

    auto sender = std::make_shared<FakeSender>();

    HandlePrepare(message, context, sender);

    ASSERT_TRUE(IsDecreeEqual(message.decree, context->promised_decree.Value()));
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::PromiseMessage);
}


TEST_F(AcceptorTest, testHandlePrepareWithLowerDecreeDoesNotUpdatePromisedDecree)
{
    Message message(Decree(Replica("the_author"), -1, "", DecreeType::UserDecree), Replica("from"), Replica("to"), MessageType::PrepareMessage);

    auto context = createAcceptorContext();
    context->promised_decree = Decree(Replica("the_author"), 1, "", DecreeType::UserDecree);

    auto sender = std::make_shared<FakeSender>();

    HandlePrepare(message, context, sender);

    ASSERT_TRUE(IsDecreeLower(message.decree, context->promised_decree.Value()));
}


TEST_F(AcceptorTest, testHandlePrepareWithEqualDecreeNumberFromMultipleReplicasSendsSinglePromise)
{
    Message author_a(Decree(Replica("author_a"), 1, "", DecreeType::UserDecree), Replica("from"), Replica("to"), MessageType::PrepareMessage);
    Message author_b(Decree(Replica("author_b"), 1, "", DecreeType::UserDecree), Replica("from"), Replica("to"), MessageType::PrepareMessage);

    auto context = createAcceptorContext();
    context->promised_decree = Decree(Replica("the_author"), 0, "", DecreeType::UserDecree);

    auto sender = std::make_shared<FakeSender>();

    HandlePrepare(author_a, context, sender);
    HandlePrepare(author_b, context, sender);

    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::PromiseMessage);
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::NackMessage);

    // We send 1 accept message and 1 nack message.
    ASSERT_EQ(2, sender->sentMessages().size());
}


TEST_F(AcceptorTest, testHandlePrepareWithEqualDecreeNumberFromSingleReplicasResendsPromise)
{
    Message message(Decree(Replica("the_author"), 1, "", DecreeType::UserDecree), Replica("from"), Replica("to"), MessageType::PrepareMessage);

    auto context = createAcceptorContext();
    context->promised_decree = Decree(Replica("the_author"), 0, "", DecreeType::UserDecree);

    auto sender = std::make_shared<FakeSender>();

    HandlePrepare(message, context, sender);
    HandlePrepare(message, context, sender);

    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::PromiseMessage);
    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, MessageType::NackMessage);

    // We send 2 accept messages.
    ASSERT_EQ(2, sender->sentMessages().size());
}


TEST_F(AcceptorTest, testHandleAcceptWithLowerDecreeDoesNotUpdateAcceptedDecree)
{
    Message message(Decree(Replica("the_author"), -1, "", DecreeType::UserDecree), Replica("from"), Replica("to"), MessageType::AcceptMessage);

    auto context = createAcceptorContext();
    context->promised_decree = Decree(Replica("the_author"), 1, "", DecreeType::UserDecree);
    context->accepted_decree = Decree(Replica("the_author"), 1, "", DecreeType::UserDecree);

    auto sender = std::make_shared<FakeSender>();

    HandleAccept(message, context, sender);

    ASSERT_TRUE(IsDecreeLower(message.decree, context->accepted_decree.Value()));
}


TEST_F(AcceptorTest, testHandleAcceptWithEqualDecreeDoesNotUpdateAcceptedDecree)
{
    Message message(Decree(Replica("the_author"), 1, "", DecreeType::UserDecree), Replica("from"), Replica("to"), MessageType::AcceptMessage);

    auto context = createAcceptorContext();
    context->promised_decree = Decree(Replica("the_author"), 1, "", DecreeType::UserDecree);
    context->accepted_decree = Decree(Replica("the_author"), 1, "", DecreeType::UserDecree);

    auto sender = std::make_shared<FakeSender>();

    HandleAccept(message, context, sender);

    ASSERT_TRUE(IsDecreeLowerOrEqual(message.decree, context->accepted_decree.Value()));
}


TEST_F(AcceptorTest, testHandleAcceptWithHigherDecreeDoesUpdateAcceptedDecree)
{
    Message message(Decree(Replica("the_author"), 2, "", DecreeType::UserDecree), Replica("from"), Replica("to"), MessageType::AcceptMessage);

    auto context = createAcceptorContext();
    context->promised_decree = Decree(Replica("the_author"), 1, "", DecreeType::UserDecree);
    context->accepted_decree = Decree(Replica("the_author"), 1, "", DecreeType::UserDecree);

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


TEST_F(LearnerTest, testRegisterLearnerWillRegistereMessageTypes)
{
    auto receiver = std::make_shared<FakeReceiver>();
    auto sender = std::make_shared<FakeSender>();
    auto context = createLearnerContext({});

    RegisterLearner(receiver, sender, context);

    ASSERT_TRUE(receiver->IsMessageTypeRegister(MessageType::AcceptedMessage));
    ASSERT_TRUE(receiver->IsMessageTypeRegister(MessageType::UpdatedMessage));

    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::RequestMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::PrepareMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::PromiseMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::NackMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::AcceptMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::UpdateMessage));
}


TEST_F(LearnerTest, testProclaimHandleWithSingleReplica)
{
    Message message(Decree(Replica("A"), 1, "", DecreeType::UserDecree), Replica("A"), Replica("A"), MessageType::AcceptedMessage);
    auto context = createLearnerContext({"A"});

    HandleProclaim(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    ASSERT_EQ(context->ledger->Size(), 1);
}


TEST_F(LearnerTest, testProclaimHandleIgnoresMessagesFromUnknownReplica)
{
    Message message(Decree(Replica("A"), 1, "", DecreeType::UserDecree), Replica("Unknown"), Replica("A"), MessageType::AcceptedMessage);
    auto context = createLearnerContext({"A"});

    HandleProclaim(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    ASSERT_EQ(context->ledger->Size(), 0);
}


TEST_F(LearnerTest, testProclaimHandleReceivesOneAcceptedWithThreeReplicaSet)
{
    Message message(Decree(Replica("A"), 1, "", DecreeType::UserDecree), Replica("A"), Replica("B"), MessageType::AcceptedMessage);
    auto context = createLearnerContext({"A", "B", "C"});

    HandleProclaim(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    ASSERT_EQ(context->ledger->Size(), 0);
}


TEST_F(LearnerTest, testProclaimHandleReceivesTwoAcceptedWithThreeReplicaSet)
{
    auto context = createLearnerContext({"A", "B", "C"});

    HandleProclaim(
        Message(
            Decree(Replica("A"), 1, "", DecreeType::UserDecree),
            Replica("A"),
            Replica("B"),
            MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );
    HandleProclaim(
        Message(
            Decree(Replica("A"), 1, "", DecreeType::UserDecree),
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
    Decree decree(Replica("A"), 1, "", DecreeType::UserDecree);

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
    Decree decree(Replica("A"), 1, "", DecreeType::UserDecree);

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
            Decree(Replica("A"), 1, "", DecreeType::UserDecree),
            Replica("A"),
            Replica("B"),
            MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );
    HandleProclaim(
        Message(
            Decree(Replica("A"), 1, "", DecreeType::UserDecree),
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
            Decree(Replica("A"), 1, "", DecreeType::UserDecree),
            Replica("A"), Replica("A"),
            MessageType::AcceptedMessage
        ),
        context,
        sender
    );

    // Missing decrees 2-9 so don't write to ledger yet.
    HandleProclaim(
        Message(
            Decree(Replica("A"), 10, "", DecreeType::UserDecree),
            Replica("A"), Replica("A"),
            MessageType::AcceptedMessage
        ),
        context,
        sender
    );

    ASSERT_EQ(context->ledger->Size(), 1);
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::UpdateMessage);
}


TEST_F(LearnerTest, testProclaimHandleAppendsToLedgerAfterComparingAgainstOriginalDecree)
{
    Message message(Decree(Replica("A"), 42, "", DecreeType::UserDecree), Replica("A"), Replica("A"), MessageType::AcceptedMessage);
    message.original_decree = Decree(Replica("A"), 1, "", DecreeType::UserDecree);
    auto context = createLearnerContext({"A"});

    HandleProclaim(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    // Even though actual decree 42 is out-of-order, original is 1 which is in-order.
    ASSERT_EQ(context->ledger->Size(), 1);
    ASSERT_EQ(context->tracked_future_decrees.size(), 0);

    // Actual decree should be appended
    ASSERT_TRUE(IsDecreeIdentical(context->ledger->Tail(), message.decree));
}


TEST_F(LearnerTest, testProclaimHandleTracksFutureDecreesIfReceivedOutOfOrder)
{
    Message message(Decree(Replica("A"), 42, "", DecreeType::UserDecree), Replica("A"), Replica("A"), MessageType::AcceptedMessage);
    auto context = createLearnerContext({"A"});
    context->is_observer = true;

    HandleProclaim(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    // Mode is_observer should not write to ledger.
    ASSERT_EQ(context->ledger->Size(), 0);
    ASSERT_EQ(context->tracked_future_decrees.size(), 1);
}


TEST_F(LearnerTest, testProclaimHandleDoesNotTrackPastDecrees)
{
    Decree past_decree(Replica("A"), 1, "", DecreeType::UserDecree);
    Decree future_decree(Replica("A"), 1, "", DecreeType::UserDecree);

    Message message(past_decree, Replica("A"), Replica("A"), MessageType::AcceptedMessage);
    auto context = createLearnerContext({"A"});
    context->is_observer = true;
    context->ledger->Append(future_decree);

    HandleProclaim(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    // Mode is_observer shouldn't write next passed decree to ledger.
    ASSERT_EQ(context->ledger->Size(), 1);

    // Decree is in the past so shouldn't add to tracked decrees.
    ASSERT_EQ(context->tracked_future_decrees.size(), 0);
}


TEST_F(LearnerTest, testProclaimHandleAppendsTrackedFutureDecreesToLedgerWhenTheyFillInHoles)
{
    Decree past_decree(Replica("A"), 1, "", DecreeType::UserDecree);
    Decree current_decree(Replica("A"), 2, "", DecreeType::UserDecree);

    Message message(current_decree, Replica("A"), Replica("A"), MessageType::AcceptedMessage);
    auto context = createLearnerContext({"A"});
    context->tracked_future_decrees.push(past_decree);

    HandleProclaim(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    // Past decree from tracked future decrees and current decree are both appended to ledger.
    ASSERT_EQ(context->ledger->Size(), 2);
}


TEST_F(LearnerTest, testHandleUpdatedWithEmptyLedger)
{
    auto context = createLearnerContext({"A"});
    auto sender = std::make_shared<FakeSender>();

    // Missing decrees 1-9 so don't write to ledger yet.
    HandleUpdated(
        Message(
            Decree(Replica("A"), 10, "", DecreeType::UserDecree),
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
    context->ledger->Append(Decree(Replica("A"), 9, "", DecreeType::UserDecree));

    // Receive next ordered decree 10.
    HandleUpdated(
        Message(
            Decree(Replica("A"), 10, "", DecreeType::UserDecree),
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
    context->tracked_future_decrees.push(Decree(Replica("A"), 2, "", DecreeType::UserDecree));
    context->tracked_future_decrees.push(Decree(Replica("A"), 3, "", DecreeType::UserDecree));

    // Receive next ordered decree 1.
    HandleUpdated(
        Message(
            Decree(Replica("A"), 1, "", DecreeType::UserDecree),
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
    context->tracked_future_decrees.push(Decree(Replica("A"), 3, "", DecreeType::UserDecree));
    context->tracked_future_decrees.push(Decree(Replica("A"), 4, "", DecreeType::UserDecree));

    // Receive next ordered decree 1.
    HandleUpdated(
        Message(
            Decree(Replica("A"), 1, "", DecreeType::UserDecree),
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


TEST_F(UpdaterTest, testRegisterUpdaterWillRegistereMessageTypes)
{
    auto receiver = std::make_shared<FakeReceiver>();
    auto sender = std::make_shared<FakeSender>();
    auto context = std::make_shared<UpdaterContext>(
        std::make_shared<Ledger>(
            std::make_shared<VolatileQueue<Decree>>()
        )
    );

    RegisterUpdater(receiver, sender, context);

    ASSERT_TRUE(receiver->IsMessageTypeRegister(MessageType::UpdateMessage));

    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::RequestMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::PrepareMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::PromiseMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::NackMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::AcceptMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::AcceptedMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::UpdatedMessage));
}


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
            Decree(Replica("A"), 1, "", DecreeType::UserDecree),
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

    context->ledger->Append(Decree(Replica("A"), 1, "", DecreeType::UserDecree));
    context->ledger->Append(Decree(Replica("A"), 2, "", DecreeType::UserDecree));
    context->ledger->Append(Decree(Replica("A"), 3, "", DecreeType::UserDecree));

    HandleUpdate(
        Message(
            Decree(Replica("A"), 1, "", DecreeType::UserDecree),
            Replica("A"), Replica("A"),
            MessageType::UpdateMessage
        ),
        context,
        sender
    );

    // Our ledger contained next decree so we shoul send an updated message.
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::UpdatedMessage);
}
