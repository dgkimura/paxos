#include <initializer_list>
#include <set>
#include <string>
#include <thread>

#include "gtest/gtest.h"

#include "paxos/logging.hpp"
#include "paxos/roles.hpp"


class FakeSender : public paxos::Sender
{
public:

    FakeSender()
        : sent_messages(),
          replicaset(std::shared_ptr<paxos::ReplicaSet>(new paxos::ReplicaSet()))
    {
    }

    FakeSender(std::shared_ptr<paxos::ReplicaSet> replicaset_)
        : sent_messages(),
          replicaset(replicaset_)
    {
    }

    ~FakeSender()
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


class FakeReceiver : public paxos::Receiver
{
public:

    void RegisterCallback(paxos::Callback&& callback, paxos::MessageType type)
    {
        registered_set.insert(type);
    }

    bool IsMessageTypeRegister(paxos::MessageType type)
    {
        return registered_set.find(type) != registered_set.end();
    }

private:

    std::set<paxos::MessageType> registered_set;
};


std::shared_ptr<paxos::AcceptorContext> createAcceptorContext()
{
    return std::make_shared<paxos::AcceptorContext>(
        std::make_shared<paxos::VolatileDecree>(),
        std::make_shared<paxos::VolatileDecree>(),
        std::chrono::milliseconds(0)
    );
}


bool WasMessageTypeSent(std::shared_ptr<FakeSender> sender, paxos::MessageType type)
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
ASSERT_MESSAGE_TYPE_SENT(std::shared_ptr<FakeSender> sender, paxos::MessageType type)
{
    ASSERT_TRUE(WasMessageTypeSent(sender, type));
}


void
ASSERT_MESSAGE_TYPE_NOT_SENT(std::shared_ptr<FakeSender> sender, paxos::MessageType type)
{
    ASSERT_FALSE(WasMessageTypeSent(sender, type));
}


class ProposerTest: public testing::Test
{
    virtual void SetUp()
    {
        paxos::DisableLogging();
    }
};


TEST_F(ProposerTest, testRegisterProposerWillRegistereMessageTypes)
{
    auto receiver = std::make_shared<FakeReceiver>();
    auto sender = std::make_shared<FakeSender>();
    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<paxos::VolatileDecree>(),
        std::make_shared<paxos::NoPause>(),
        signal
    );

    RegisterProposer(receiver, sender, context);

    ASSERT_TRUE(receiver->IsMessageTypeRegister(paxos::MessageType::RequestMessage));
    ASSERT_TRUE(receiver->IsMessageTypeRegister(paxos::MessageType::PromiseMessage));
    ASSERT_TRUE(receiver->IsMessageTypeRegister(paxos::MessageType::NackTieMessage));
    ASSERT_TRUE(receiver->IsMessageTypeRegister(paxos::MessageType::ResumeMessage));

    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::PrepareMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::AcceptMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::UpdateMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::UpdatedMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::AcceptedMessage));
}


TEST_F(ProposerTest, testHandleRequestWillSendHigestProposedDecreeIfItExists)
{
    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<paxos::VolatileDecree>(),
        std::make_shared<paxos::NoPause>(),
        signal
    );
    context->replicaset->Add(paxos::Replica("host"));
    context->highest_proposed_decree = paxos::Decree(paxos::Replica("the_author"), 42, "", paxos::DecreeType::UserDecree);

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandleRequest(
        paxos::Message(
            paxos::Decree(paxos::Replica("host"), -1, "first", paxos::DecreeType::UserDecree),
            paxos::Replica("host"),
            paxos::Replica("B"),
            paxos::MessageType::RequestMessage
        ),
        context,
        sender
    );

    ASSERT_EQ(42, sender->sentMessages()[0].decree.number);
}


TEST_F(ProposerTest, testHandleRequestAllowsOnlyOneUniqueInProgressProposal)
{
    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<paxos::VolatileDecree>(),
        std::make_shared<paxos::NoPause>(),
        signal
    );
    context->replicaset->Add(paxos::Replica("host"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandleRequest(
        paxos::Message(
            paxos::Decree(paxos::Replica("host"), -1, "first", paxos::DecreeType::UserDecree),
            paxos::Replica("host"),
            paxos::Replica("B"),
            paxos::MessageType::RequestMessage
        ),
        context,
        sender
    );
    HandleRequest(
        paxos::Message(
            paxos::Decree(paxos::Replica("host"), -1, "second", paxos::DecreeType::UserDecree),
            paxos::Replica("host"),
            paxos::Replica("B"),
            paxos::MessageType::RequestMessage
        ),
        context,
        sender
    );
    HandleRequest(
        paxos::Message(
            paxos::Decree(paxos::Replica("host"), -1, "third", paxos::DecreeType::UserDecree),
            paxos::Replica("host"),
            paxos::Replica("B"),
            paxos::MessageType::RequestMessage
        ),
        context,
        sender
    );

    ASSERT_EQ(3, sender->sentMessages().size());
    ASSERT_EQ(1, sender->sentMessages()[0].decree.number);
    ASSERT_EQ(1, sender->sentMessages()[1].decree.number);
    ASSERT_EQ(1, sender->sentMessages()[2].decree.number);
}


TEST_F(ProposerTest, testHandlePromiseWithLowerDecreeDoesNotUpdatesighestPromisedDecree)
{
    paxos::Message message(
        paxos::Decree(paxos::Replica("the_author"), -1, "", paxos::DecreeType::UserDecree),
        paxos::Replica("from"),
        paxos::Replica("to"),
        paxos::MessageType::PromiseMessage);

    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<paxos::VolatileDecree>(),
        std::make_shared<paxos::NoPause>(),
        signal
    );
    context->highest_proposed_decree = paxos::Decree(paxos::Replica("the_author"), 0, "", paxos::DecreeType::UserDecree);

    std::shared_ptr<FakeSender> sender(new FakeSender());

    HandlePromise(message, context, sender);

    ASSERT_TRUE(IsDecreeLower(message.decree, context->highest_proposed_decree.Value()));
    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, paxos::MessageType::AcceptMessage);
}


TEST_F(ProposerTest, testHandlePromiseWithoutAnyRequestedValuesDoesNotSendAccept)
{
    paxos::Message message(paxos::Decree(paxos::Replica("host"), 1, "", paxos::DecreeType::UserDecree), paxos::Replica("host"), paxos::Replica("host"), paxos::MessageType::PromiseMessage);

    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<paxos::VolatileDecree>(),
        std::make_shared<paxos::NoPause>(),
        signal
    );
    context->highest_proposed_decree = paxos::Decree(paxos::Replica("host"), 0, "", paxos::DecreeType::UserDecree);
    context->replicaset = std::make_shared<paxos::ReplicaSet>();
    context->replicaset->Add(paxos::Replica("host"));

    // context->requested_values is EMPTY!!

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandlePromise(message, context, sender);

    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, paxos::MessageType::AcceptMessage);
}


TEST_F(ProposerTest, testHandlePromiseWithLowerDecreeAndNonemptyContentResendsAccept)
{
    paxos::Message message(paxos::Decree(paxos::Replica("host"), 1, "previous accepted decree content", paxos::DecreeType::UserDecree), paxos::Replica("host"), paxos::Replica("host"), paxos::MessageType::PromiseMessage);

    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<paxos::VolatileDecree>(),
        std::make_shared<paxos::NoPause>(),
        signal
    );
    // highest proposed decree is higher than messaged decree
    context->highest_proposed_decree = paxos::Decree(paxos::Replica("host"), 2, "", paxos::DecreeType::UserDecree);
    context->replicaset = std::make_shared<paxos::ReplicaSet>();
    context->replicaset->Add(paxos::Replica("host"));
    context->requested_values.push_back(std::make_tuple("a_requested_value", paxos::DecreeType::UserDecree));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandlePromise(message, context, sender);

    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::AcceptMessage);
    ASSERT_EQ("previous accepted decree content", sender->sentMessages()[0].decree.content);
}


TEST_F(ProposerTest, testHandlePromiseWithHigherDecreeUpdatesHighestPromisedDecree)
{
    paxos::Message message(paxos::Decree(paxos::Replica("host"), 1, "", paxos::DecreeType::UserDecree), paxos::Replica("host"), paxos::Replica("host"), paxos::MessageType::PromiseMessage);

    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<paxos::VolatileDecree>(),
        std::make_shared<paxos::NoPause>(),
        signal
    );
    context->highest_proposed_decree = paxos::Decree(paxos::Replica("host"), 0, "", paxos::DecreeType::UserDecree);
    context->replicaset = std::make_shared<paxos::ReplicaSet>();
    context->replicaset->Add(paxos::Replica("host"));
    context->requested_values.push_back(std::make_tuple("a_requested_value", paxos::DecreeType::UserDecree));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandlePromise(message, context, sender);

    ASSERT_TRUE(IsDecreeEqual(message.decree, context->highest_proposed_decree.Value()));
    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::AcceptMessage);
}


TEST_F(ProposerTest, testHandlePromiseWithHigherDecreeFromUnknownReplicaDoesNotUpdateHighestPromisedDecree)
{
    paxos::Message message(paxos::Decree(paxos::Replica("host"), 1, "", paxos::DecreeType::UserDecree), paxos::Replica("unknown_host"), paxos::Replica("host"), paxos::MessageType::PromiseMessage);

    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<paxos::VolatileDecree>(),
        std::make_shared<paxos::NoPause>(),
        signal
    );
    context->highest_proposed_decree = paxos::Decree(paxos::Replica("host"), 0, "", paxos::DecreeType::UserDecree);
    context->replicaset = std::shared_ptr<paxos::ReplicaSet>(new paxos::ReplicaSet());
    context->replicaset->Add(paxos::Replica("host"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandlePromise(message, context, sender);

    ASSERT_FALSE(IsDecreeEqual(message.decree, context->highest_proposed_decree.Value()));
    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, paxos::MessageType::AcceptMessage);
}


TEST_F(ProposerTest, testHandlePromiseWithHigherEmptyDecreeAndExistingRequestedValueThenUpdateDecreeContents)
{
    paxos::Message message(paxos::Decree(paxos::Replica("host"), 1, "", paxos::DecreeType::UserDecree), paxos::Replica("host"), paxos::Replica("host"), paxos::MessageType::PromiseMessage);

    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<paxos::VolatileDecree>(),
        std::make_shared<paxos::NoPause>(),
        signal
    );

    // Highest promised decree content is "".
    context->highest_proposed_decree = paxos::Decree(paxos::Replica("host"), 0, "", paxos::DecreeType::UserDecree);
    context->replicaset = std::make_shared<paxos::ReplicaSet>();
    context->replicaset->Add(paxos::Replica("host"));

    // Requested values contains entry with "new content to be used".
    context->requested_values.push_back(std::make_tuple("new content to be used", paxos::DecreeType::UserDecree));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandlePromise(message, context, sender);

    ASSERT_EQ("new content to be used", sender->sentMessages()[0].decree.content);
    ASSERT_EQ(paxos::DecreeType::UserDecree, sender->sentMessages()[0].decree.type);
    ASSERT_EQ(0, context->requested_values.size());
}


TEST_F(ProposerTest, testHandlePromiseRecievesPromiseContainingContentsSendsAcceptWithSameContents)
{
    paxos::Message message(paxos::Decree(paxos::Replica("host"), 1, "existing contents", paxos::DecreeType::UserDecree), paxos::Replica("host"), paxos::Replica("host"), paxos::MessageType::PromiseMessage);

    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<paxos::VolatileDecree>(),
        std::make_shared<paxos::NoPause>(),
        signal
    );

    context->highest_proposed_decree = paxos::Decree(paxos::Replica("host"), 0, "", paxos::DecreeType::UserDecree);
    context->replicaset = std::make_shared<paxos::ReplicaSet>();
    context->replicaset->Add(paxos::Replica("host"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandlePromise(message, context, sender);

    ASSERT_EQ("existing contents", sender->sentMessages()[0].decree.content);
}


TEST_F(ProposerTest, testHandlePromiseWillSendAcceptAgainIfDuplicatePromiseIsSent)
{
    paxos::Message message(paxos::Decree(paxos::Replica("host1"), 1, "", paxos::DecreeType::UserDecree), paxos::Replica("host1"), paxos::Replica("host1"), paxos::MessageType::PromiseMessage);

    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<paxos::VolatileDecree>(),
        std::make_shared<paxos::NoPause>(),
        signal
    );
    context->highest_proposed_decree = paxos::Decree(paxos::Replica("host"), 0, "", paxos::DecreeType::UserDecree);
    context->replicaset = std::make_shared<paxos::ReplicaSet>();
    context->replicaset->Add(paxos::Replica("host1"));
    context->replicaset->Add(paxos::Replica("host2"));
    context->replicaset->Add(paxos::Replica("host3"));
    context->promise_map[message.decree] = std::make_shared<paxos::ReplicaSet>();
    context->promise_map[message.decree]->Add(paxos::Replica("host1"));
    context->promise_map[message.decree]->Add(paxos::Replica("host2"));
    context->promise_map[message.decree]->Add(paxos::Replica("host3"));
    context->requested_values.push_back(std::make_tuple("a_requested_value",  paxos::DecreeType::UserDecree));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandlePromise(message, context, sender);

    // Accept message is sent to all 3 replicas.
    ASSERT_EQ(3, sender->sentMessages().size());
    ASSERT_EQ(paxos::MessageType::AcceptMessage, sender->sentMessages()[0].type);
    ASSERT_EQ(paxos::MessageType::AcceptMessage, sender->sentMessages()[1].type);
    ASSERT_EQ(paxos::MessageType::AcceptMessage, sender->sentMessages()[2].type);
}


TEST_F(ProposerTest, testHandlePromiseWillNotSendAcceptAgainIfPromiseIsUnique)
{
    paxos::Message message(paxos::Decree(paxos::Replica("host1"), 1, "", paxos::DecreeType::UserDecree), paxos::Replica("host1"), paxos::Replica("host1"), paxos::MessageType::PromiseMessage);

    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<paxos::VolatileDecree>(),
        std::make_shared<paxos::NoPause>(),
        signal
    );
    context->highest_proposed_decree = paxos::Decree(paxos::Replica("host"), 0, "", paxos::DecreeType::UserDecree);
    context->replicaset = std::make_shared<paxos::ReplicaSet>();
    context->replicaset->Add(paxos::Replica("host1"));
    context->replicaset->Add(paxos::Replica("host2"));
    context->replicaset->Add(paxos::Replica("host3"));
    context->promise_map[message.decree] = std::make_shared<paxos::ReplicaSet>();
    context->promise_map[message.decree]->Add(paxos::Replica("host2"));
    context->promise_map[message.decree]->Add(paxos::Replica("host3"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    // Sending promise from host1, which is not in our promise map yet.
    HandlePromise(message, context, sender);

    // We shouldn't send accept, since this is not a duplicate message and
    // promise_map contains more than minimum quorum (so we should have already
    // sent accept message).
    ASSERT_EQ(0, sender->sentMessages().size());
}


TEST_F(ProposerTest, testHandleNackTieIncrementsDecreeNumberAndResendsPrepareMessage)
{
    auto replica = paxos::Replica("host");
    auto decree = paxos::Decree(replica, 1, "first", paxos::DecreeType::UserDecree);
    auto current_decree = paxos::Decree(replica, 2, "current", paxos::DecreeType::UserDecree);
    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    auto highest_proposed_decree = std::make_shared<paxos::VolatileDecree>();
    highest_proposed_decree->Put(current_decree);
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    ledger->Append(decree);

    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        highest_proposed_decree,
        std::make_shared<paxos::NoPause>(),
        signal
    );
    context->interval = std::chrono::milliseconds(0);

    // Add replica to known replicas.
    context->replicaset->Add(replica);
    context->replicaset->Add(paxos::Replica("host-2"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandleNackTie(
        paxos::Message(
            current_decree,
            replica,
            replica,
            paxos::MessageType::NackTieMessage
        ),
        context,
        sender
    );

    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::PrepareMessage);
    ASSERT_EQ(current_decree.number + 1, sender->sentMessages()[0].decree.number);
    ASSERT_EQ(current_decree.number + 1, context->highest_proposed_decree.Value().number);

    // Sends 1 message to each replica
    ASSERT_EQ(2, sender->sentMessages().size());
}


TEST_F(ProposerTest, testHandleNackTieDoesNotSendWhenDecreeIsLowerThanHighestProposedDecree)
{
    auto replica = paxos::Replica("host");
    auto decree = paxos::Decree(replica, 2, "next", paxos::DecreeType::UserDecree);
    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    auto highest_proposed_decree = std::make_shared<paxos::VolatileDecree>();

    // Highest proposed decree (3) is higher than decree (2).
    highest_proposed_decree->Put(paxos::Decree(replica, 3, "first", paxos::DecreeType::UserDecree));
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    ledger->Append(paxos::Decree(replica, 1, "first", paxos::DecreeType::UserDecree));

    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        highest_proposed_decree,
        std::make_shared<paxos::NoPause>(),
        signal
    );
    context->interval = std::chrono::milliseconds(0);

    // Add replica to known replicas.
    context->replicaset->Add(replica);

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandleNackTie(
        paxos::Message(
            decree,
            replica,
            replica,
            paxos::MessageType::NackTieMessage
        ),
        context,
        sender
    );

    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, paxos::MessageType::PrepareMessage);
}


TEST_F(ProposerTest, testHandleNackTieDoesNotSendWhenDecreeIsLowerThanLedgerTail)
{
    auto replica = paxos::Replica("host");
    auto current_decree = paxos::Decree(replica, 1, "current", paxos::DecreeType::UserDecree);
    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    auto highest_proposed_decree = std::make_shared<paxos::VolatileDecree>();
    highest_proposed_decree->Put(current_decree);
    std::stringstream ss;
    auto queue = std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss);
    auto ledger = std::make_shared<paxos::Ledger>(queue);
    // Ledger tail decree (2) is higher than messaged decree (1)
    ledger->Append(paxos::Decree(replica, 1, "tail", paxos::DecreeType::UserDecree));
    ledger->Append(paxos::Decree(replica, 2, "tail", paxos::DecreeType::UserDecree));

    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        highest_proposed_decree,
        std::make_shared<paxos::NoPause>(),
        signal
    );
    context->interval = std::chrono::milliseconds(0);

    // Add replica to known replicas.
    context->replicaset->Add(replica);
    context->replicaset->Add(paxos::Replica("host-2"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandleNackTie(
        paxos::Message(
            current_decree,
            replica,
            replica,
            paxos::MessageType::NackTieMessage
        ),
        context,
        sender
    );

    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, paxos::MessageType::PrepareMessage);
}


TEST_F(ProposerTest, testHandleNackTieDoesNotSendResendWhenDecreeIsEqualToPreviousTie)
{
    auto replica = paxos::Replica("host");
    auto decree = paxos::Decree(replica, 1, "first", paxos::DecreeType::UserDecree);
    auto current_decree = paxos::Decree(replica, 2, "current", paxos::DecreeType::UserDecree);
    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    auto highest_proposed_decree = std::make_shared<paxos::VolatileDecree>();
    highest_proposed_decree->Put(current_decree);
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    ledger->Append(decree);

    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        highest_proposed_decree,
        std::make_shared<paxos::NoPause>(),
        signal
    );
    context->interval = std::chrono::milliseconds(0);
    context->replicaset->Add(replica);

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandleNackTie(
        paxos::Message(
            current_decree,
            replica,
            replica,
            paxos::MessageType::NackTieMessage
        ),
        context,
        sender
    );
    HandleNackTie(
        paxos::Message(
            current_decree,
            replica,
            replica,
            paxos::MessageType::NackTieMessage
        ),
        context,
        sender
    );

    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::PrepareMessage);
    ASSERT_EQ(1, sender->sentMessages().size());
}


class EventfulPause : public paxos::NoPause
{
public:

    void RunBefore(std::function<void(void)> callback)
    {
        before.push_back(callback);
    }

    virtual void
    Start(std::function<void(void)> callback)
    {
        for (auto& f : before)
        {
            f();
        }
        callback();
    }

private:

    std::vector<std::function<void(void)>> before;
};


TEST_F(ProposerTest, testHandleNackTieDoesNotSendWhenHighestNackTieIsIncrementedBetweenPause)
{
    auto replica = paxos::Replica("host");
    auto decree = paxos::Decree(replica, 1, "first", paxos::DecreeType::UserDecree);
    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    auto highest_proposed_decree = std::make_shared<paxos::VolatileDecree>();
    highest_proposed_decree->Put(decree);
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    ledger->Append(paxos::Decree(replica, 0, "first", paxos::DecreeType::UserDecree));

    auto signal = std::make_shared<paxos::Signal>();
    auto pause = std::make_shared<EventfulPause>();

    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        highest_proposed_decree,
        pause,
        signal
    );

    // Highest nack tied decree is incremented before our pause is run.
    pause->RunBefore([&context](){ context->highest_nacktie_decree.number += 1; });

    context->replicaset->Add(replica);

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandleNackTie(
        paxos::Message(
            decree,
            replica,
            replica,
            paxos::MessageType::NackTieMessage
        ),
        context,
        sender
    );

    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, paxos::MessageType::PrepareMessage);
}


TEST_F(ProposerTest, testHandleNackTieDoesNotSendWhenLedgerIsIncrementedBetweenPause)
{
    auto replica = paxos::Replica("host");
    auto decree = paxos::Decree(replica, 1, "first", paxos::DecreeType::UserDecree);
    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    auto highest_proposed_decree = std::make_shared<paxos::VolatileDecree>();
    highest_proposed_decree->Put(decree);
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    ledger->Append(paxos::Decree(replica, 0, "first", paxos::DecreeType::UserDecree));

    auto signal = std::make_shared<paxos::Signal>();
    auto pause = std::make_shared<EventfulPause>();

    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        highest_proposed_decree,
        pause,
        signal
    );

    // Ledger is updated before our pause is run.
    pause->RunBefore([&context](){
        paxos::Decree d;
        d.number = context->ledger->Tail().number + 1;
        d.root_number = context->ledger->Tail().root_number+ 1;
        context->ledger->Append(d);
    });

    context->replicaset->Add(replica);

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandleNackTie(
        paxos::Message(
            decree,
            replica,
            replica,
            paxos::MessageType::NackTieMessage
        ),
        context,
        sender
    );

    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, paxos::MessageType::PrepareMessage);
}


TEST_F(ProposerTest, testHandleNackPrepareSendsUpdateMessage)
{
    auto replica = paxos::Replica("host");
    auto decree = paxos::Decree(replica, 1, "next", paxos::DecreeType::UserDecree);
    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<paxos::VolatileDecree>(),
        std::make_shared<paxos::NoPause>(),
        signal
    );
    context->requested_values.push_back(std::make_tuple("a pending value", paxos::DecreeType::UserDecree));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandleNackPrepare(
        paxos::Message(
            decree,
            replica,
            replica,
            paxos::MessageType::NackPrepareMessage
        ),
        context,
        sender
    );

    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::UpdateMessage);
}


TEST_F(ProposerTest, testHandleNackPrepareDoesNotSendUpdateForEqualDecrees)
{
    auto replica = paxos::Replica("host");
    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<paxos::VolatileDecree>(),
        std::make_shared<paxos::NoPause>(),
        signal
    );

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandleNackPrepare(
        paxos::Message(
            paxos::Decree(replica, 1, "next", paxos::DecreeType::UserDecree),
            replica,
            replica,
            paxos::MessageType::NackPrepareMessage
        ),
        context,
        sender
    );
    // Equal decree (1) is sent
    HandleNackPrepare(
        paxos::Message(
            paxos::Decree(replica, 1, "next", paxos::DecreeType::UserDecree),
            replica,
            replica,
            paxos::MessageType::NackPrepareMessage
        ),
        context,
        sender
    );

    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::UpdateMessage);

    // Should only send UpdateMessage once even though 2 NackPrepareMessage sent
    ASSERT_EQ(sender->sentMessages().size(), 1);
}


TEST_F(ProposerTest, testHandleNackPrepareDoesNotSendUpdateOnLowerRootDecrees)
{
    auto replica = paxos::Replica("host");

    // Decree (1) is lower than ledger tail decree (2)
    auto decree = paxos::Decree(replica, 1, "next", paxos::DecreeType::AddReplicaDecree);
    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    ledger->Append(paxos::Decree(paxos::Replica(), 1, "", paxos::DecreeType::UserDecree));
    ledger->Append(paxos::Decree(paxos::Replica(), 2, "", paxos::DecreeType::UserDecree));
    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<paxos::VolatileDecree>(),
        std::make_shared<paxos::NoPause>(),
        signal
    );
    context->requested_values.push_back(std::make_tuple("a pending value", paxos::DecreeType::AddReplicaDecree));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandleNackPrepare(
        paxos::Message(
            decree,
            replica,
            replica,
            paxos::MessageType::NackPrepareMessage
        ),
        context,
        sender
    );

    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, paxos::MessageType::UpdateMessage);
}


TEST_F(ProposerTest, testUpdatingLedgerUpdatesNextProposedDecreeNumber)
{
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<paxos::VolatileDecree>(),
        std::make_shared<paxos::NoPause>(),
        signal
    );
    context->replicaset->Add(paxos::Replica("host"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    // Current decree is one.
    HandleRequest(
        paxos::Message(
            paxos::Decree(paxos::Replica("host"), -1, "second", paxos::DecreeType::UserDecree),
            paxos::Replica("host"),
            paxos::Replica("B"),
            paxos::MessageType::RequestMessage
        ),
        context,
        sender
    );
    ASSERT_EQ(sender->sentMessages()[0].decree.number, 1);
    ASSERT_EQ(sender->sentMessages()[0].decree.root_number, 1);

    // Our ledger was updated underneath us to 5.
    context->ledger->Append(paxos::Decree(paxos::Replica("a_author"), 1, "", paxos::DecreeType::UserDecree));
    context->ledger->Append(paxos::Decree(paxos::Replica("a_author"), 2, "", paxos::DecreeType::UserDecree));
    context->ledger->Append(paxos::Decree(paxos::Replica("a_author"), 3, "", paxos::DecreeType::UserDecree));
    context->ledger->Append(paxos::Decree(paxos::Replica("a_author"), 4, "", paxos::DecreeType::UserDecree));
    context->ledger->Append(paxos::Decree(paxos::Replica("a_author"), 5, "", paxos::DecreeType::UserDecree));

    // Next round, increments current by one.
    HandleRequest(
        paxos::Message(
            paxos::Decree(paxos::Replica("host"), -1, "second", paxos::DecreeType::UserDecree),
            paxos::Replica("host"),
            paxos::Replica("B"),
            paxos::MessageType::RequestMessage
        ),
        context,
        sender
    );

    ASSERT_EQ(sender->sentMessages()[1].decree.number, 6);
    ASSERT_EQ(sender->sentMessages()[1].decree.root_number, 6);
    ASSERT_TRUE(IsReplicaEqual(paxos::Replica("B"), sender->sentMessages()[1].decree.author));
}


TEST_F(ProposerTest, testHandleAcceptRemovesEntriesInThePromiseMap)
{
    paxos::Message message(
        paxos::Decree(paxos::Replica("A"), 1, "content", paxos::DecreeType::UserDecree),
        paxos::Replica("from"),
        paxos::Replica("to"),
        paxos::MessageType::AcceptMessage);

    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<paxos::VolatileDecree>(),
        std::make_shared<paxos::NoPause>(),
        signal
    );
    context->promise_map[message.decree] = std::make_shared<paxos::ReplicaSet>();
    context->promise_map[message.decree]->Add(message.from);

    ASSERT_EQ(context->promise_map.size(), 1);

    HandleResume(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    ASSERT_EQ(context->promise_map.size(), 0);
}


TEST_F(ProposerTest, testHandleResumeSendsNextRequestIfThereArePendingProposals)
{
    paxos::Decree decree(paxos::Replica("A"), 1, "content", paxos::DecreeType::UserDecree);
    paxos::Message message(
        decree,
        paxos::Replica("from"),
        paxos::Replica("to"),
        paxos::MessageType::AcceptMessage);

    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    ledger->Append(decree);
    auto signal = std::make_shared<paxos::Signal>();
    auto context = std::make_shared<paxos::ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<paxos::VolatileDecree>(),
        std::make_shared<paxos::NoPause>(),
        signal
    );
    context->requested_values.push_back(std::make_tuple("another pending value", paxos::DecreeType::UserDecree));
    auto sender = std::shared_ptr<FakeSender>(new FakeSender());

    HandleResume(message, context, sender);

    ASSERT_EQ(sender->sentMessages()[0].type, paxos::MessageType::RequestMessage);
}


class AcceptorTest: public testing::Test
{
    virtual void SetUp()
    {
        paxos::DisableLogging();
    }
};


TEST_F(AcceptorTest, testRegisterAcceptorWillRegistereMessageTypes)
{
    auto receiver = std::make_shared<FakeReceiver>();
    auto sender = std::make_shared<FakeSender>();
    auto context = createAcceptorContext();

    RegisterAcceptor(receiver, sender, context);

    ASSERT_TRUE(receiver->IsMessageTypeRegister(paxos::MessageType::PrepareMessage));
    ASSERT_TRUE(receiver->IsMessageTypeRegister(paxos::MessageType::AcceptMessage));

    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::RequestMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::PromiseMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::NackTieMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::AcceptedMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::UpdateMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::UpdatedMessage));
}


TEST_F(AcceptorTest, testHandlePrepareWithHigherDecreeUpdatesPromisedDecree)
{
    paxos::Message message(paxos::Decree(paxos::Replica("the_author"), 1, "", paxos::DecreeType::UserDecree), paxos::Replica("from"), paxos::Replica("to"), paxos::MessageType::PrepareMessage);

    std::shared_ptr<paxos::AcceptorContext> context = createAcceptorContext();
    context->promised_decree = paxos::Decree(paxos::Replica("the_author"), 0, "", paxos::DecreeType::UserDecree);

    auto sender = std::make_shared<FakeSender>();

    HandlePrepare(message, context, sender);

    ASSERT_TRUE(IsDecreeEqual(message.decree, context->promised_decree.Value()));
    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::PromiseMessage);
}


TEST_F(AcceptorTest, testHandlePrepareWithLowerDecreeButSameAuthorAsHighestPromsiedDecreeSendsPromiseMessage)
{
    paxos::Message message(paxos::Decree(paxos::Replica("the_author"), -1, "", paxos::DecreeType::UserDecree), paxos::Replica("from"), paxos::Replica("to"), paxos::MessageType::PrepareMessage);

    auto context = createAcceptorContext();
    context->promised_decree = paxos::Decree(paxos::Replica("the_author"), 1, "", paxos::DecreeType::UserDecree);

    auto sender = std::make_shared<FakeSender>();

    HandlePrepare(message, context, sender);

    ASSERT_TRUE(IsDecreeLower(message.decree, context->promised_decree.Value()));
    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::PromiseMessage);
}


TEST_F(AcceptorTest, testHandlePrepareWithLowerAndDifferentAuthorDecreeDoesNotUpdatePromisedDecree)
{
    paxos::Message message(paxos::Decree(paxos::Replica("the_author"), -1, "", paxos::DecreeType::UserDecree), paxos::Replica("from"), paxos::Replica("to"), paxos::MessageType::PrepareMessage);

    auto context = createAcceptorContext();
    context->promised_decree = paxos::Decree(paxos::Replica("another_author"), 1, "", paxos::DecreeType::UserDecree);

    auto sender = std::make_shared<FakeSender>();

    HandlePrepare(message, context, sender);

    ASSERT_TRUE(IsDecreeLower(message.decree, context->promised_decree.Value()));
    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::NackPrepareMessage);
}


TEST_F(AcceptorTest, testHandlePrepareWithEqualRootDecreeNumberFromMultipleReplicasSendsSinglePromise)
{
    paxos::Message author_a(paxos::Decree(paxos::Replica("author_a"), 2, "", paxos::DecreeType::UserDecree), paxos::Replica("from"), paxos::Replica("to"), paxos::MessageType::PrepareMessage);
    paxos::Message author_b(paxos::Decree(paxos::Replica("author_b"), 1, "", paxos::DecreeType::UserDecree), paxos::Replica("from"), paxos::Replica("to"), paxos::MessageType::PrepareMessage);

    // The root number of author_a (1) equals root number of author_b (1).
    author_a.decree.root_number = 1;

    auto context = createAcceptorContext();
    context->promised_decree = paxos::Decree(paxos::Replica("the_author"), 0, "", paxos::DecreeType::UserDecree);

    auto sender = std::make_shared<FakeSender>();

    HandlePrepare(author_a, context, sender);
    HandlePrepare(author_b, context, sender);

    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::PromiseMessage);
    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::NackTieMessage);

    // We send 1 accept message and 1 nack message.
    ASSERT_EQ(2, sender->sentMessages().size());
}


TEST_F(AcceptorTest, testHandlePrepareWithEqualDecreeNumberFromSingleReplicasResendsPromise)
{
    paxos::Message message(paxos::Decree(paxos::Replica("the_author"), 1, "", paxos::DecreeType::UserDecree), paxos::Replica("from"), paxos::Replica("to"), paxos::MessageType::PrepareMessage);

    auto context = createAcceptorContext();
    context->promised_decree = paxos::Decree(paxos::Replica("the_author"), 0, "", paxos::DecreeType::UserDecree);

    auto sender = std::make_shared<FakeSender>();

    HandlePrepare(message, context, sender);
    HandlePrepare(message, context, sender);

    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::PromiseMessage);
    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, paxos::MessageType::NackTieMessage);

    // We send 2 accept messages.
    ASSERT_EQ(2, sender->sentMessages().size());
}


TEST_F(AcceptorTest, testHandlePrepareWithPendingNonEmptyAcceptDecree)
{
    paxos::Message message(paxos::Decree(paxos::Replica("the_author"), 1, "", paxos::DecreeType::UserDecree), paxos::Replica("from"), paxos::Replica("to"), paxos::MessageType::PrepareMessage);

    std::shared_ptr<paxos::AcceptorContext> context = createAcceptorContext();

    // We have a pending accepted decree with non-empty content with higher value (2) than messaged decree (1)
    context->accepted_decree = paxos::Decree(paxos::Replica("the_author"), 2, "pending non-empty contents", paxos::DecreeType::UserDecree);

    auto sender = std::make_shared<FakeSender>();

    HandlePrepare(message, context, sender);

    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::PromiseMessage);
    ASSERT_EQ("pending non-empty contents", sender->sentMessages()[0].decree.content);
}


TEST_F(AcceptorTest, testHandleAcceptWithLowerDecreeDoesNotUpdateAcceptedDecree)
{
    paxos::Message message(paxos::Decree(paxos::Replica("the_author"), -1, "", paxos::DecreeType::UserDecree), paxos::Replica("from"), paxos::Replica("to"), paxos::MessageType::AcceptMessage);

    auto context = createAcceptorContext();
    context->promised_decree = paxos::Decree(paxos::Replica("the_author"), 1, "", paxos::DecreeType::UserDecree);
    context->accepted_decree = paxos::Decree(paxos::Replica("the_author"), 1, "", paxos::DecreeType::UserDecree);

    auto sender = std::make_shared<FakeSender>();

    HandleAccept(message, context, sender);

    ASSERT_TRUE(IsDecreeLower(message.decree, context->accepted_decree.Value()));
}


TEST_F(AcceptorTest, testHandleAcceptWithIdenticalPromisedDecreeSendsAcceptedDecree)
{
    paxos::Message message(paxos::Decree(paxos::Replica("the_author"), 1, "", paxos::DecreeType::UserDecree), paxos::Replica("from"), paxos::Replica("to"), paxos::MessageType::AcceptMessage);

    auto context = createAcceptorContext();
    context->interval = std::chrono::milliseconds(0);
    context->promised_decree = paxos::Decree(paxos::Replica("the_author"), 1, "", paxos::DecreeType::UserDecree);
    context->accepted_decree = paxos::Decree(paxos::Replica("the_author"), 1, "", paxos::DecreeType::UserDecree);

    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    replicaset->Add(paxos::Replica("the_author"));
    auto sender = std::make_shared<FakeSender>(replicaset);

    HandleAccept(message, context, sender);

    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::AcceptedMessage);
}


TEST_F(AcceptorTest, testHandleAcceptWithEqualDecreeDoesNotUpdateAcceptedDecree)
{
    paxos::Message message(paxos::Decree(paxos::Replica("the_author"), 1, "", paxos::DecreeType::UserDecree), paxos::Replica("from"), paxos::Replica("to"), paxos::MessageType::AcceptMessage);

    auto context = createAcceptorContext();
    context->promised_decree = paxos::Decree(paxos::Replica("the_author"), 1, "", paxos::DecreeType::UserDecree);
    context->accepted_decree = paxos::Decree(paxos::Replica("the_author"), 1, "", paxos::DecreeType::UserDecree);

    auto sender = std::make_shared<FakeSender>();

    HandleAccept(message, context, sender);

    ASSERT_TRUE(IsDecreeLowerOrEqual(message.decree, context->accepted_decree.Value()));
}


TEST_F(AcceptorTest, testHandleAcceptWithHigherOrEqualAcceptedDecreeResendsAcceptedMessage)
{
    paxos::Message message(paxos::Decree(paxos::Replica("the_author"), 1, "", paxos::DecreeType::UserDecree), paxos::Replica("from"), paxos::Replica("to"), paxos::MessageType::AcceptMessage);

    auto context = createAcceptorContext();
    context->promised_decree = paxos::Decree(paxos::Replica("the_author"), 1, "", paxos::DecreeType::UserDecree);
    context->accepted_decree = paxos::Decree(paxos::Replica("the_author"), 1, "", paxos::DecreeType::UserDecree);

    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    replicaset->Add(paxos::Replica("the_author"));
    auto sender = std::make_shared<FakeSender>(replicaset);

    HandleAccept(message, context, sender);

    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::AcceptedMessage);
}


TEST_F(AcceptorTest, testHandleAcceptWithHigherDecreeDoesUpdateAcceptedDecree)
{
    paxos::Message message(paxos::Decree(paxos::Replica("the_author"), 2, "", paxos::DecreeType::UserDecree), paxos::Replica("from"), paxos::Replica("to"), paxos::MessageType::AcceptMessage);

    auto context = createAcceptorContext();
    context->promised_decree = paxos::Decree(paxos::Replica("the_author"), 1, "", paxos::DecreeType::UserDecree);
    context->accepted_decree = paxos::Decree(paxos::Replica("the_author"), 1, "", paxos::DecreeType::UserDecree);

    auto sender = std::make_shared<FakeSender>();

    HandleAccept(message, context, sender);

    ASSERT_TRUE(IsDecreeEqual(message.decree, context->accepted_decree.Value()));
}


TEST_F(AcceptorTest, testHandleCleanupResetsAcceptDecreeContentsWhenDecreeIsEqual)
{
    paxos::Message message(paxos::Decree(paxos::Replica("the_author"), 2, "accepted decree content", paxos::DecreeType::UserDecree), paxos::Replica("from"), paxos::Replica("to"), paxos::MessageType::ResumeMessage);

    auto context = createAcceptorContext();
    context->accepted_decree = paxos::Decree(paxos::Replica("the_author"), 2, "accepted decree content", paxos::DecreeType::UserDecree);

    auto sender = std::make_shared<FakeSender>();

    HandleCleanup(message, context, sender);

    ASSERT_TRUE(IsDecreeEqual(message.decree, context->accepted_decree.Value()));
    ASSERT_EQ("", context->accepted_decree.Value().content);
}


TEST_F(AcceptorTest, testHandleCleanupDoesNotResetAcceptDecreeContentsWhenDecreeIsLower)
{
    paxos::Message message(paxos::Decree(paxos::Replica("the_author"), 1, "", paxos::DecreeType::UserDecree), paxos::Replica("from"), paxos::Replica("to"), paxos::MessageType::ResumeMessage);

    auto context = createAcceptorContext();
    context->accepted_decree = paxos::Decree(paxos::Replica("the_author"), 2, "accepted decree content", paxos::DecreeType::UserDecree);

    auto sender = std::make_shared<FakeSender>();

    HandleCleanup(message, context, sender);

    // Messaged decree (1) is lower than the accepted decree (2) so accepted
    // content should not be erased.
    ASSERT_EQ("accepted decree content", context->accepted_decree.Value().content);
}


class LearnerTest: public testing::Test
{
    virtual void SetUp()
    {
        paxos::DisableLogging();

        replicaset = std::make_shared<paxos::ReplicaSet>();
        queue = std::make_shared<paxos::RolloverQueue<paxos::Decree>>(sstream);
        ledger = std::make_shared<paxos::Ledger>(queue);
        context = std::make_shared<paxos::LearnerContext>(replicaset, ledger);
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

    std::shared_ptr<paxos::ReplicaSet> replicaset;
    std::stringstream sstream;
    std::shared_ptr<paxos::RolloverQueue<paxos::Decree>> queue;
    std::shared_ptr<paxos::Ledger> ledger;
    std::shared_ptr<paxos::LearnerContext> context;
};


TEST_F(LearnerTest, testRegisterLearnerWillRegistereMessageTypes)
{
    auto receiver = std::make_shared<FakeReceiver>();
    auto sender = std::make_shared<FakeSender>();

    RegisterLearner(receiver, sender, context);

    ASSERT_TRUE(receiver->IsMessageTypeRegister(paxos::MessageType::AcceptedMessage));
    ASSERT_TRUE(receiver->IsMessageTypeRegister(paxos::MessageType::UpdatedMessage));

    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::RequestMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::PrepareMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::PromiseMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::NackTieMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::AcceptMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::UpdateMessage));
}


TEST_F(LearnerTest, testAcceptedHandleWithSingleReplica)
{
    paxos::Message message(paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree), paxos::Replica("A"), paxos::Replica("A"), paxos::MessageType::AcceptedMessage);
    replicaset->Add(paxos::Replica("A"));

    HandleAccepted(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    ASSERT_EQ(GetQueueSize(queue), 1);
}


TEST_F(LearnerTest, testAcceptedHandleIgnoresMessagesFromUnknownReplica)
{
    paxos::Message message(paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree), paxos::Replica("Unknown"), paxos::Replica("A"), paxos::MessageType::AcceptedMessage);
    replicaset->Add(paxos::Replica("A"));

    HandleAccepted(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    ASSERT_EQ(GetQueueSize(queue), 0);
}


TEST_F(LearnerTest, testAcceptedHandleReceivesOneAcceptedWithThreeReplicaSet)
{
    paxos::Message message(paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree), paxos::Replica("A"), paxos::Replica("B"), paxos::MessageType::AcceptedMessage);
    replicaset->Add(paxos::Replica("A"));
    replicaset->Add(paxos::Replica("B"));
    replicaset->Add(paxos::Replica("C"));

    HandleAccepted(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    ASSERT_EQ(GetQueueSize(queue), 0);
}


TEST_F(LearnerTest, testAcceptedHandleReceivesTwoAcceptedWithThreeReplicaSet)
{
    replicaset->Add(paxos::Replica("A"));
    replicaset->Add(paxos::Replica("B"));
    replicaset->Add(paxos::Replica("C"));

    HandleAccepted(
        paxos::Message(
            paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree),
            paxos::Replica("A"),
            paxos::Replica("B"),
            paxos::MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );
    HandleAccepted(
        paxos::Message(
            paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree),
            paxos::Replica("B"),
            paxos::Replica("B"),
            paxos::MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );

    ASSERT_EQ(GetQueueSize(queue), 1);
}


TEST_F(LearnerTest, testAcceptedHandleCleansUpAcceptedMapAfterAllVotesReceived)
{
    replicaset->Add(paxos::Replica("A"));
    replicaset->Add(paxos::Replica("B"));
    replicaset->Add(paxos::Replica("C"));

    paxos::Decree decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree);

    HandleAccepted(
        paxos::Message(
            decree,
            paxos::Replica("A"),
            paxos::Replica("B"),
            paxos::MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );

    ASSERT_FALSE(context->accepted_map.find(decree) == context->accepted_map.end());

    HandleAccepted(
        paxos::Message(
            decree,
            paxos::Replica("B"),
            paxos::Replica("B"),
            paxos::MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );

    ASSERT_FALSE(context->accepted_map.find(decree) == context->accepted_map.end());

    HandleAccepted(
        paxos::Message(
            decree,
            paxos::Replica("C"),
            paxos::Replica("B"),
            paxos::MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );

    ASSERT_TRUE(context->accepted_map.find(decree) == context->accepted_map.end());
}


TEST_F(LearnerTest, testAcceptedHandleIgnoresPreviouslyAcceptedMessagesFromReplicasRemovedFromReplicaSet)
{
    replicaset->Add(paxos::Replica("A"));
    replicaset->Add(paxos::Replica("B"));
    replicaset->Add(paxos::Replica("C"));
    paxos::Decree decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree);

    // Accepted from replica A.
    HandleAccepted(
        paxos::Message(
            decree,
            paxos::Replica("A"),
            paxos::Replica("B"),
            paxos::MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );

    // Remove replica A from replicaset.
    context->replicaset->Remove(paxos::Replica("A"));

    HandleAccepted(
        paxos::Message(
            decree,
            paxos::Replica("B"),
            paxos::Replica("B"),
            paxos::MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );

    // We should not have written to ledger since replica A was removed from replicaset.
    ASSERT_EQ(GetQueueSize(queue), 0);
}


TEST_F(LearnerTest, testAcceptedHandleIgnoresDuplicateAcceptedMessages)
{
    replicaset->Add(paxos::Replica("A"));
    replicaset->Add(paxos::Replica("B"));
    replicaset->Add(paxos::Replica("C"));

    HandleAccepted(
        paxos::Message(
            paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree),
            paxos::Replica("A"),
            paxos::Replica("B"),
            paxos::MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );
    HandleAccepted(
        paxos::Message(
            paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree),
            paxos::Replica("A"),
            paxos::Replica("B"),
            paxos::MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );

    ASSERT_EQ(GetQueueSize(queue), 0);
}


TEST_F(LearnerTest, testAcceptedHandleDoesNotWriteInLedgerIfTheLastDecreeInTheLedgerIsOutOfOrder)
{
    replicaset->Add(paxos::Replica("A"));
    auto sender = std::make_shared<FakeSender>();

    HandleAccepted(
        paxos::Message(
            paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree),
            paxos::Replica("A"), paxos::Replica("A"),
            paxos::MessageType::AcceptedMessage
        ),
        context,
        sender
    );

    // Missing decrees 2-9 so don't write to ledger yet.
    HandleAccepted(
        paxos::Message(
            paxos::Decree(paxos::Replica("A"), 10, "", paxos::DecreeType::UserDecree),
            paxos::Replica("A"), paxos::Replica("A"),
            paxos::MessageType::AcceptedMessage
        ),
        context,
        sender
    );

    ASSERT_EQ(GetQueueSize(queue), 1);
    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::UpdateMessage);
}


TEST_F(LearnerTest, testAcceptedHandleAppendsToLedgerAfterComparingAgainstOriginalDecree)
{
    paxos::Message message(paxos::Decree(paxos::Replica("A"), 42, "", paxos::DecreeType::UserDecree), paxos::Replica("A"), paxos::Replica("A"), paxos::MessageType::AcceptedMessage);
    message.decree.root_number = 1;
    replicaset->Add(paxos::Replica("A"));

    HandleAccepted(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    // Even though actual decree 42 is out-of-order, original is 1 which is in-order.
    ASSERT_EQ(GetQueueSize(queue), 1);
    ASSERT_EQ(context->tracked_future_decrees.size(), 0);

    // Actual decree should be appended
    ASSERT_TRUE(IsDecreeIdentical(context->ledger->Tail(), message.decree));
}


TEST_F(LearnerTest, testAcceptedHandleTracksFutureDecreesIfReceivedOutOfOrder)
{
    paxos::Message message(paxos::Decree(paxos::Replica("A"), 42, "", paxos::DecreeType::UserDecree), paxos::Replica("A"), paxos::Replica("A"), paxos::MessageType::AcceptedMessage);
    replicaset->Add(paxos::Replica("A"));
    context->is_observer = true;

    HandleAccepted(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    // Mode is_observer should not write to ledger.
    ASSERT_EQ(GetQueueSize(queue), 0);
    ASSERT_EQ(context->tracked_future_decrees.size(), 1);
}


TEST_F(LearnerTest, testAcceptedHandleDoesNotTrackPastDecrees)
{
    paxos::Decree past_decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree future_decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree);

    paxos::Message message(past_decree, paxos::Replica("A"), paxos::Replica("A"), paxos::MessageType::AcceptedMessage);
    replicaset->Add(paxos::Replica("A"));
    context->is_observer = true;
    context->ledger->Append(future_decree);

    HandleAccepted(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    // Mode is_observer shouldn't write next passed decree to ledger.
    ASSERT_EQ(GetQueueSize(queue), 1);

    // Decree is in the past so shouldn't add to tracked decrees.
    ASSERT_EQ(context->tracked_future_decrees.size(), 0);
}


TEST_F(LearnerTest, testAcceptedHandleAppendsTrackedFutureDecreesToLedgerWhenTheyFillInHoles)
{
    paxos::Decree past_decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree);
    paxos::Decree current_decree(paxos::Replica("A"), 2, "", paxos::DecreeType::UserDecree);

    paxos::Message message(current_decree, paxos::Replica("A"), paxos::Replica("A"), paxos::MessageType::AcceptedMessage);
    replicaset->Add(paxos::Replica("A"));
    context->tracked_future_decrees.push(past_decree);

    HandleAccepted(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    // Past decree from tracked future decrees and current decree are both appended to ledger.
    ASSERT_EQ(GetQueueSize(queue), 2);
}


TEST_F(LearnerTest, testAcceptedHandleSendsResumeWhenMessagedDecreeIsEqualToLedger)
{
    paxos::Message message(paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree), paxos::Replica("A"), paxos::Replica("A"), paxos::MessageType::AcceptedMessage);
    replicaset->Add(paxos::Replica("A"));
    auto sender = std::make_shared<FakeSender>();

    HandleAccepted(message, context, sender);

    // Reset sender to erase message queue.
    sender = std::make_shared<FakeSender>();
    ASSERT_EQ(0, sender->sentMessages().size());

    HandleAccepted(message, context, sender);

    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::ResumeMessage);
    ASSERT_EQ(sender->sentMessages()[0].decree.number, 1);
}


TEST_F(LearnerTest, testAcceptedHandleSendsResumeWhenAcceptedIsGreaterThanQuorum)
{
    paxos::Message message(paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree), paxos::Replica("A"), paxos::Replica("A"), paxos::MessageType::AcceptedMessage);
    replicaset->Add(paxos::Replica("A"));
    replicaset->Add(paxos::Replica("B"));
    replicaset->Add(paxos::Replica("C"));
    auto sender = std::make_shared<FakeSender>();

    // A replica accepted.
    HandleAccepted(
        paxos::Message(
            paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree),
            paxos::Replica("A"),
            paxos::Replica("A"),
            paxos::MessageType::AcceptedMessage),
        context, sender);

    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, paxos::MessageType::ResumeMessage);

    // B replica accepted.
    HandleAccepted(
        paxos::Message(
            paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree),
            paxos::Replica("B"),
            paxos::Replica("A"),
            paxos::MessageType::AcceptedMessage),
        context, sender);

    // Have quorum, should send resume.
    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::ResumeMessage);

    // Reset sender to erase message queue.
    sender = std::make_shared<FakeSender>();

    // C replica accepted.
    HandleAccepted(
        paxos::Message(
            paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree),
            paxos::Replica("C"),
            paxos::Replica("A"),
            paxos::MessageType::AcceptedMessage),
        context, sender);

    // Still have quorum, should send resume, again.
    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::ResumeMessage);
    ASSERT_EQ(sender->sentMessages()[0].decree.number, 1);
}


TEST_F(LearnerTest, testHandleUpdatedWithEmptyLedger)
{
    replicaset->Add(paxos::Replica("A"));
    auto sender = std::make_shared<FakeSender>();

    // Missing decrees 1-9 so don't write to ledger yet.
    HandleUpdated(
        paxos::Message(
            paxos::Decree(paxos::Replica("A"), 10, "", paxos::DecreeType::UserDecree),
            paxos::Replica("A"), paxos::Replica("A"),
            paxos::MessageType::UpdatedMessage
        ),
        context,
        sender
    );

    ASSERT_EQ(GetQueueSize(queue), 0);

    // We are still behind so we should not resume.
    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, paxos::MessageType::ResumeMessage);
}


TEST_F(LearnerTest, testHandleUpdatedReceivedMessageWithZeroDecree)
{
    replicaset->Add(paxos::Replica("A"));
    context->ledger->Append(paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree));
    context->ledger->Append(paxos::Decree(paxos::Replica("A"), 2, "", paxos::DecreeType::UserDecree));
    context->ledger->Append(paxos::Decree(paxos::Replica("A"), 3, "", paxos::DecreeType::UserDecree));
    auto sender = std::make_shared<FakeSender>();

    // Sending zero decree.
    HandleUpdated(
        paxos::Message(
            paxos::Decree(paxos::Replica("A"), 0, "", paxos::DecreeType::UserDecree),
            paxos::Replica("A"), paxos::Replica("A"),
            paxos::MessageType::UpdatedMessage
        ),
        context,
        sender
    );

    // We are up to date so send resume.
    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::ResumeMessage);

    // Decree number in resume message should be last ledger decree number.
    ASSERT_EQ(sender->sentMessages()[0].decree.number, 3);
}


TEST_F(LearnerTest, testHandleUpdatedReceivesMessageWithNextOrderedDecree)
{
    replicaset->Add(paxos::Replica("A"));
    auto sender = std::make_shared<FakeSender>();

    // Last decree in ledger is 1.
    context->ledger->Append(paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree));

    // Receive next ordered decree 2.
    HandleUpdated(
        paxos::Message(
            paxos::Decree(paxos::Replica("A"), 2, "", paxos::DecreeType::UserDecree),
            paxos::Replica("A"), paxos::Replica("A"),
            paxos::MessageType::UpdatedMessage
        ),
        context,
        sender
    );

    // We should have appended 1 and 2 to our ledger.
    ASSERT_EQ(GetQueueSize(queue), 2);
}


TEST_F(LearnerTest, testHandleUpdatedReceivesMessageWithNextOrderedDecreeAndTrackedAdditionalOrderedDecrees)
{
    replicaset->Add(paxos::Replica("A"));
    auto sender = std::make_shared<FakeSender>();

    // We have tracked decrees 2, 3.
    context->tracked_future_decrees.push(paxos::Decree(paxos::Replica("A"), 2, "", paxos::DecreeType::UserDecree));
    context->tracked_future_decrees.push(paxos::Decree(paxos::Replica("A"), 3, "", paxos::DecreeType::UserDecree));

    // Receive next ordered decree 1.
    HandleUpdated(
        paxos::Message(
            paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree),
            paxos::Replica("A"), paxos::Replica("A"),
            paxos::MessageType::UpdatedMessage
        ),
        context,
        sender
    );

    // We should have appended 1, 2, and 3 to our ledger.
    ASSERT_EQ(GetQueueSize(queue), 3);
}


TEST_F(LearnerTest, testHandleUpdatedReceivesMessageWithNextOrderedDecreeAndTrackedAdditionalUnorderedDecrees)
{
    replicaset->Add(paxos::Replica("A"));
    auto sender = std::make_shared<FakeSender>();

    // We have tracked decrees 3, 4.
    context->tracked_future_decrees.push(paxos::Decree(paxos::Replica("A"), 3, "", paxos::DecreeType::UserDecree));
    context->tracked_future_decrees.push(paxos::Decree(paxos::Replica("A"), 4, "", paxos::DecreeType::UserDecree));

    // Receive next ordered decree 1.
    HandleUpdated(
        paxos::Message(
            paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree),
            paxos::Replica("A"), paxos::Replica("A"),
            paxos::MessageType::UpdatedMessage
        ),
        context,
        sender
    );

    // We appended 1 to our ledger, but are missing 2 so we can't add 3 and 4.
    ASSERT_EQ(GetQueueSize(queue), 1);

    // Since we still have a hole we should send an update message.
    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::UpdateMessage);
}


class UpdaterTest: public testing::Test
{
    virtual void SetUp()
    {
        paxos::DisableLogging();
    }
};


TEST_F(UpdaterTest, testRegisterUpdaterWillRegistereMessageTypes)
{
    auto receiver = std::make_shared<FakeReceiver>();
    auto sender = std::make_shared<FakeSender>();
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    auto context = std::make_shared<paxos::UpdaterContext>(
        ledger
    );

    RegisterUpdater(receiver, sender, context);

    ASSERT_TRUE(receiver->IsMessageTypeRegister(paxos::MessageType::UpdateMessage));

    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::RequestMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::PrepareMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::PromiseMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::NackTieMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::AcceptMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::AcceptedMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(paxos::MessageType::UpdatedMessage));
}


TEST_F(UpdaterTest, testHandleUpdateReceivesMessageWithDecreeAndHasEmptyLedger)
{
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    auto context = std::make_shared<paxos::UpdaterContext>(
        ledger
    );
    auto sender = std::make_shared<FakeSender>();

    HandleUpdate(
        paxos::Message(
            paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree),
            paxos::Replica("A"), paxos::Replica("A"),
            paxos::MessageType::UpdateMessage
        ),
        context,
        sender
    );

    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::UpdatedMessage);

    // Our ledger is empty so we will send an zero decree to show we have nothing.
    ASSERT_EQ(sender->sentMessages()[0].decree.number, 0);
}


TEST_F(UpdaterTest, testHandleUpdateReceivesMessageWithDecreeAndLedgerHasNextDecree)
{
    std::stringstream ss;
    auto ledger = std::make_shared<paxos::Ledger>(
        std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss)
    );
    auto context = std::make_shared<paxos::UpdaterContext>(
        ledger
    );
    auto sender = std::make_shared<FakeSender>();

    context->ledger->Append(paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree));
    context->ledger->Append(paxos::Decree(paxos::Replica("A"), 2, "", paxos::DecreeType::UserDecree));
    context->ledger->Append(paxos::Decree(paxos::Replica("A"), 3, "", paxos::DecreeType::UserDecree));

    HandleUpdate(
        paxos::Message(
            paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree),
            paxos::Replica("A"), paxos::Replica("A"),
            paxos::MessageType::UpdateMessage
        ),
        context,
        sender
    );

    // Our ledger contained next decree so we shoul send an updated message.
    ASSERT_MESSAGE_TYPE_SENT(sender, paxos::MessageType::UpdatedMessage);
}
