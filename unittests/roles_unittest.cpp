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

    std::shared_ptr<Ledger> ledger = std::make_shared<VolatileLedger>();
    return std::make_shared<LearnerContext>(replicaset, ledger);
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

    std::shared_ptr<AcceptorContext> context(new AcceptorContext());
    context->promised_decree = Decree("the_author", 0, "");

    std::shared_ptr<FakeSender> sender(new FakeSender());

    HandlePrepare(message, context, sender);

    ASSERT_TRUE(IsDecreeEqual(message.decree, context->promised_decree));
}


TEST_F(AcceptorTest, testHandlePrepareWithLowerDecreeDoesNotUpdatePromisedDecree)
{
    Message message(Decree("the_author", -1, ""), Replica("from"), Replica("to"), MessageType::PrepareMessage);

    std::shared_ptr<AcceptorContext> context(new AcceptorContext());
    context->promised_decree = Decree("the_author", 1, "");

    std::shared_ptr<FakeSender> sender(new FakeSender());

    HandlePrepare(message, context, sender);

    ASSERT_TRUE(IsDecreeLower(message.decree, context->promised_decree));
}


TEST_F(AcceptorTest, testHandleAcceptWithLowerDecreeDoesNotUpdateAcceptedDecree)
{
    Message message(Decree("the_author", -1, ""), Replica("from"), Replica("to"), MessageType::AcceptMessage);

    std::shared_ptr<AcceptorContext> context(new AcceptorContext());
    context->promised_decree = Decree("the_author", 1, "");
    context->accepted_decree = Decree("the_author", 1, "");

    std::shared_ptr<FakeSender> sender(new FakeSender());

    HandleAccept(message, context, sender);

    ASSERT_TRUE(IsDecreeLower(message.decree, context->accepted_decree));
}


TEST_F(AcceptorTest, testHandleAcceptWithEqualDecreeDoesNotUpdateAcceptedDecree)
{
    Message message(Decree("the_author", 1, ""), Replica("from"), Replica("to"), MessageType::AcceptMessage);

    std::shared_ptr<AcceptorContext> context(new AcceptorContext());
    context->promised_decree = Decree("the_author", 1, "");
    context->accepted_decree = Decree("the_author", 1, "");

    std::shared_ptr<FakeSender> sender(new FakeSender());

    HandleAccept(message, context, sender);

    ASSERT_TRUE(IsDecreeLowerOrEqual(message.decree, context->accepted_decree));
}


TEST_F(AcceptorTest, testHandleAcceptWithHigherDecreeDoesUpdateAcceptedDecree)
{
    Message message(Decree("the_author", 2, ""), Replica("from"), Replica("to"), MessageType::AcceptMessage);

    std::shared_ptr<AcceptorContext> context(new AcceptorContext());
    context->promised_decree = Decree("the_author", 1, "");
    context->accepted_decree = Decree("the_author", 1, "");

    std::shared_ptr<FakeSender> sender(new FakeSender());

    HandleAccept(message, context, sender);

    ASSERT_TRUE(IsDecreeEqual(message.decree, context->accepted_decree));
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

    ASSERT_TRUE(context->ledger->Contains(message.decree));
}


TEST_F(LearnerTest, testProclaimHandleIgnoresMessagesFromUnknownReplica)
{
    Message message(Decree("A", 1, ""), Replica("Unknown"), Replica("A"), MessageType::AcceptedMessage);
    std::shared_ptr<LearnerContext> context = createLearnerContext({"A"});

    HandleProclaim(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    ASSERT_FALSE(context->ledger->Contains(message.decree));
}


TEST_F(LearnerTest, testProclaimHandleReceivesOneAcceptedWithThreeReplicaSet)
{
    Message message(Decree("A", 1, ""), Replica("A"), Replica("B"), MessageType::AcceptedMessage);
    std::shared_ptr<LearnerContext> context = createLearnerContext({"A", "B", "C"});

    HandleProclaim(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    ASSERT_FALSE(context->ledger->Contains(message.decree));
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

    ASSERT_TRUE(context->ledger->Contains(Decree("A", 1, "")));
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

    ASSERT_FALSE(context->ledger->Contains(Decree("A", 1, "")));
}

