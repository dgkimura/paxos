#include <initializer_list>
#include <set>
#include <string>
#include <thread>

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
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    auto signal = std::make_shared<Signal>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<VolatileDecree>(),
        [](std::string entry){},
        std::make_shared<NoPause>(),
        signal
    );

    RegisterProposer(receiver, sender, context);

    ASSERT_TRUE(receiver->IsMessageTypeRegister(MessageType::RequestMessage));
    ASSERT_TRUE(receiver->IsMessageTypeRegister(MessageType::PromiseMessage));
    ASSERT_TRUE(receiver->IsMessageTypeRegister(MessageType::NackTieMessage));
    ASSERT_TRUE(receiver->IsMessageTypeRegister(MessageType::ResumeMessage));

    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::PrepareMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::AcceptMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::UpdateMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::UpdatedMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::AcceptedMessage));
}


TEST_F(ProposerTest, testHandleRequestWillSendHigestProposedDecreeIfItExists)
{
    auto replicaset = std::make_shared<ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    auto signal = std::make_shared<Signal>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<VolatileDecree>(),
        [](std::string entry){},
        std::make_shared<NoPause>(),
        signal
    );
    context->replicaset->Add(Replica("host"));
    context->highest_proposed_decree = Decree(Replica("the_author"), 42, "", DecreeType::UserDecree);

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

    ASSERT_EQ(42, sender->sentMessages()[0].decree.number);
}


TEST_F(ProposerTest, testHandleRequestAllowsOnlyOneUniqueInProgressProposal)
{
    auto replicaset = std::make_shared<ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    auto signal = std::make_shared<Signal>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<VolatileDecree>(),
        [](std::string entry){},
        std::make_shared<NoPause>(),
        signal
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

    ASSERT_EQ(3, sender->sentMessages().size());
    ASSERT_EQ(1, sender->sentMessages()[0].decree.number);
    ASSERT_EQ(1, sender->sentMessages()[1].decree.number);
    ASSERT_EQ(1, sender->sentMessages()[2].decree.number);
}


TEST_F(ProposerTest, testHandlePromiseWithLowerDecreeDoesNotUpdatesighestPromisedDecree)
{
    Message message(
        Decree(Replica("the_author"), -1, "", DecreeType::UserDecree),
        Replica("from"),
        Replica("to"),
        MessageType::PromiseMessage);

    auto replicaset = std::make_shared<ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    auto signal = std::make_shared<Signal>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<VolatileDecree>(),
        [](std::string entry){},
        std::make_shared<NoPause>(),
        signal
    );
    context->highest_proposed_decree = Decree(Replica("the_author"), 0, "", DecreeType::UserDecree);

    std::shared_ptr<FakeSender> sender(new FakeSender());

    HandlePromise(message, context, sender);

    ASSERT_TRUE(IsDecreeLower(message.decree, context->highest_proposed_decree.Value()));
    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, MessageType::AcceptMessage);
}


TEST_F(ProposerTest, testHandlePromiseWithoutAnyRequestedValuesDoesNotSendAccept)
{
    Message message(Decree(Replica("host"), 1, "", DecreeType::UserDecree), Replica("host"), Replica("host"), MessageType::PromiseMessage);

    auto replicaset = std::make_shared<ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    auto signal = std::make_shared<Signal>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<VolatileDecree>(),
        [](std::string entry){},
        std::make_shared<NoPause>(),
        signal
    );
    context->highest_proposed_decree = Decree(Replica("host"), 0, "", DecreeType::UserDecree);
    context->replicaset = std::make_shared<ReplicaSet>();
    context->replicaset->Add(Replica("host"));

    // context->requested_values is EMPTY!!

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandlePromise(message, context, sender);

    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, MessageType::AcceptMessage);
}


TEST_F(ProposerTest, testHandlePromiseWithHigherDecreeUpdatesHighestPromisedDecree)
{
    Message message(Decree(Replica("host"), 1, "", DecreeType::UserDecree), Replica("host"), Replica("host"), MessageType::PromiseMessage);

    auto replicaset = std::make_shared<ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    auto signal = std::make_shared<Signal>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<VolatileDecree>(),
        [](std::string entry){},
        std::make_shared<NoPause>(),
        signal
    );
    context->highest_proposed_decree = Decree(Replica("host"), 0, "", DecreeType::UserDecree);
    context->replicaset = std::make_shared<ReplicaSet>();
    context->replicaset->Add(Replica("host"));
    context->requested_values.push_back(std::make_tuple("a_requested_value", DecreeType::UserDecree));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandlePromise(message, context, sender);

    ASSERT_TRUE(IsDecreeEqual(message.decree, context->highest_proposed_decree.Value()));
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::AcceptMessage);
}


TEST_F(ProposerTest, testHandlePromiseWithHigherDecreeFromUnknownReplicaDoesNotUpdateHighestPromisedDecree)
{
    Message message(Decree(Replica("host"), 1, "", DecreeType::UserDecree), Replica("unknown_host"), Replica("host"), MessageType::PromiseMessage);

    auto replicaset = std::make_shared<ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    auto signal = std::make_shared<Signal>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<VolatileDecree>(),
        [](std::string entry){},
        std::make_shared<NoPause>(),
        signal
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
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    auto signal = std::make_shared<Signal>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<VolatileDecree>(),
        [](std::string entry){},
        std::make_shared<NoPause>(),
        signal
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
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    auto signal = std::make_shared<Signal>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<VolatileDecree>(),
        [](std::string entry){},
        std::make_shared<NoPause>(),
        signal
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
    context->requested_values.push_back(std::make_tuple("a_requested_value",  DecreeType::UserDecree));

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
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    auto signal = std::make_shared<Signal>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<VolatileDecree>(),
        [](std::string entry){},
        std::make_shared<NoPause>(),
        signal
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


TEST_F(ProposerTest, testHandleNackTieIncrementsDecreeNumberAndResendsPrepareMessage)
{
    auto replica = Replica("host");
    auto decree = Decree(replica, 1, "first", DecreeType::UserDecree);
    auto replicaset = std::make_shared<ReplicaSet>();
    auto highest_proposed_decree = std::make_shared<VolatileDecree>();
    highest_proposed_decree->Put(decree);
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    ledger->Append(Decree(replica, 0, "previous", DecreeType::UserDecree));

    auto signal = std::make_shared<Signal>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        highest_proposed_decree,
        [](std::string entry){},
        std::make_shared<NoPause>(),
        signal
    );

    // Add replica to known replicas.
    context->replicaset->Add(replica);
    context->replicaset->Add(Replica("host-2"));

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandleNackTie(
        Message(
            decree,
            replica,
            replica,
            MessageType::NackTieMessage
        ),
        context,
        sender
    );

    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::PrepareMessage);
    ASSERT_EQ(decree.number + 1, sender->sentMessages()[0].decree.number);

    // Sends 1 message to each replica
    ASSERT_EQ(2, sender->sentMessages().size());
}


TEST_F(ProposerTest, testHandleNackTieDoesNotSendWhenDecreeIsLowerThanHighestProposedDecree)
{
    auto replica = Replica("host");
    auto decree = Decree(replica, 2, "next", DecreeType::UserDecree);
    auto replicaset = std::make_shared<ReplicaSet>();
    auto highest_proposed_decree = std::make_shared<VolatileDecree>();

    // Highest proposed decree (3) is higher than decree (2).
    highest_proposed_decree->Put(Decree(replica, 3, "first", DecreeType::UserDecree));
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    ledger->Append(Decree(replica, 1, "first", DecreeType::UserDecree));

    auto signal = std::make_shared<Signal>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        highest_proposed_decree,
        [](std::string entry){},
        std::make_shared<NoPause>(),
        signal
    );

    // Add replica to known replicas.
    context->replicaset->Add(replica);

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandleNackTie(
        Message(
            decree,
            replica,
            replica,
            MessageType::NackTieMessage
        ),
        context,
        sender
    );

    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, MessageType::PrepareMessage);
}


TEST_F(ProposerTest, testHandleNackTieDoesNotSendWhenDecreeIsHigherThanHighestProposedDecree)
{
    auto replica = Replica("host");
    auto decree = Decree(replica, 2, "next", DecreeType::UserDecree);
    auto replicaset = std::make_shared<ReplicaSet>();
    auto highest_proposed_decree = std::make_shared<VolatileDecree>();

    // Highest proposed decree (1) is lower than decree (2).
    highest_proposed_decree->Put(Decree(replica, 1, "first", DecreeType::UserDecree));
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    ledger->Append(Decree(replica, 1, "first", DecreeType::UserDecree));

    auto signal = std::make_shared<Signal>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        highest_proposed_decree,
        [](std::string entry){},
        std::make_shared<NoPause>(),
        signal
    );

    // Add replica to known replicas.
    context->replicaset->Add(replica);

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandleNackTie(
        Message(
            decree,
            replica,
            replica,
            MessageType::NackTieMessage
        ),
        context,
        sender
    );

    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, MessageType::PrepareMessage);
}


class EventfulPause : public NoPause
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
    auto replica = Replica("host");
    auto decree = Decree(replica, 1, "first", DecreeType::UserDecree);
    auto replicaset = std::make_shared<ReplicaSet>();
    auto highest_proposed_decree = std::make_shared<VolatileDecree>();
    highest_proposed_decree->Put(decree);
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    ledger->Append(Decree(replica, 0, "first", DecreeType::UserDecree));

    auto signal = std::make_shared<Signal>();
    auto pause = std::make_shared<EventfulPause>();

    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        highest_proposed_decree,
        [](std::string entry){},
        pause,
        signal
    );

    // Highest nack tied decree is incremented before our pause is run.
    pause->RunBefore([&context](){ context->highest_nacktie_decree.number += 1; });

    context->replicaset->Add(replica);

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandleNackTie(
        Message(
            decree,
            replica,
            replica,
            MessageType::NackTieMessage
        ),
        context,
        sender
    );

    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, MessageType::PrepareMessage);
}


TEST_F(ProposerTest, testHandleNackTieDoesNotSendWhenLedgerIsIncrementedBetweenPause)
{
    auto replica = Replica("host");
    auto decree = Decree(replica, 1, "first", DecreeType::UserDecree);
    auto replicaset = std::make_shared<ReplicaSet>();
    auto highest_proposed_decree = std::make_shared<VolatileDecree>();
    highest_proposed_decree->Put(decree);
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    ledger->Append(Decree(replica, 0, "first", DecreeType::UserDecree));

    auto signal = std::make_shared<Signal>();
    auto pause = std::make_shared<EventfulPause>();

    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        highest_proposed_decree,
        [](std::string entry){},
        pause,
        signal
    );

    // Ledger is updated before our pause is run.
    pause->RunBefore([&context](){
        Decree d;
        d.number = context->ledger->Tail().number + 1;
        d.root_number = context->ledger->Tail().root_number+ 1;
        context->ledger->Append(d);
    });

    context->replicaset->Add(replica);

    auto sender = std::make_shared<FakeSender>(context->replicaset);

    HandleNackTie(
        Message(
            decree,
            replica,
            replica,
            MessageType::NackTieMessage
        ),
        context,
        sender
    );

    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, MessageType::PrepareMessage);
}


TEST_F(ProposerTest, testHandleNackRunsIgnoreHandler)
{
    bool was_ignore_handler_run = false;
    auto replica = Replica("host");
    auto decree = Decree(replica, 1, "next", DecreeType::UserDecree);
    auto replicaset = std::make_shared<ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    auto signal = std::make_shared<Signal>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<VolatileDecree>(),
        [&was_ignore_handler_run](std::string entry){ was_ignore_handler_run = true; },
        std::make_shared<NoPause>(),
        signal
    );
    context->requested_values.push_back(std::make_tuple("a pending value", DecreeType::UserDecree));

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

    ASSERT_TRUE(was_ignore_handler_run);
    ASSERT_EQ(0, context->requested_values.size());
}


TEST_F(ProposerTest, testHandleNackWithAddReplicaDecreeSendsFailSignal)
{
    auto replica = Replica("host");
    auto decree = Decree(replica, 1, "next", DecreeType::AddReplicaDecree);
    auto replicaset = std::make_shared<ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    auto signal = std::make_shared<Signal>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<VolatileDecree>(),
        [](std::string entry){ },
        std::make_shared<NoPause>(),
        signal
    );
    context->requested_values.push_back(std::make_tuple("a pending value", DecreeType::UserDecree));

    auto sender = std::make_shared<FakeSender>(context->replicaset);


    bool result = true;
    std::thread t([&signal, &result](){
        result = signal->Wait();
    });
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
    t.join();

    // HandleNack sets result to false.
    ASSERT_FALSE(result);
}


TEST_F(ProposerTest, testHandleNackDoesNotRunIgnoreHandlerOnSystemDecrees)
{
    bool was_ignore_handler_run = false;
    auto replica = Replica("host");

    // DecreeType::AddReplicaDecree is a system decree
    auto decree = Decree(replica, 1, "next", DecreeType::AddReplicaDecree);
    auto replicaset = std::make_shared<ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    auto signal = std::make_shared<Signal>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<VolatileDecree>(),
        [&was_ignore_handler_run](std::string entry){ was_ignore_handler_run = true; },
        std::make_shared<NoPause>(),
        signal
    );
    context->requested_values.push_back(std::make_tuple("a pending value", DecreeType::AddReplicaDecree));

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

    ASSERT_FALSE(was_ignore_handler_run);
}


TEST_F(ProposerTest, testHandleNackRunsIgnoreHandlerOnceForEachNackedDecree)
{
    int ignore_handler_run_count = 0;
    auto replica = Replica("host");
    auto decree = Decree(replica, 1, "next", DecreeType::UserDecree);
    auto replicaset = std::make_shared<ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    auto signal = std::make_shared<Signal>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<VolatileDecree>(),
        [&ignore_handler_run_count](std::string entry){ ignore_handler_run_count += 1; },
        std::make_shared<NoPause>(),
        signal
    );
    context->requested_values.push_back(std::make_tuple("a pending value", DecreeType::UserDecree));
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

    // HandleNack run multiple times on decree, but ignore handler is run once.
    ASSERT_EQ(1, ignore_handler_run_count);
}


TEST_F(ProposerTest, testUpdatingLedgerUpdatesNextProposedDecreeNumber)
{
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    auto replicaset = std::make_shared<ReplicaSet>();
    auto signal = std::make_shared<Signal>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<VolatileDecree>(),
        [](std::string entry){},
        std::make_shared<NoPause>(),
        signal
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
    ASSERT_EQ(sender->sentMessages()[0].decree.root_number, 1);

    // Our ledger was updated underneath us to 5.
    context->ledger->Append(
        Decree(Replica("the_author"), 5, "", DecreeType::UserDecree));

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
    ASSERT_EQ(sender->sentMessages()[1].decree.root_number, 6);
    ASSERT_TRUE(IsReplicaEqual(Replica("B"), sender->sentMessages()[1].decree.author));
}


TEST_F(ProposerTest, testHandleAcceptRemovesEntriesInThePromiseMap)
{
    Message message(
        Decree(Replica("A"), 1, "content", DecreeType::UserDecree),
        Replica("from"),
        Replica("to"),
        MessageType::AcceptMessage);

    auto replicaset = std::make_shared<ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    auto signal = std::make_shared<Signal>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<VolatileDecree>(),
        [](std::string entry){},
        std::make_shared<NoPause>(),
        signal
    );
    context->promise_map[message.decree] = std::make_shared<ReplicaSet>();
    context->promise_map[message.decree]->Add(message.from);

    ASSERT_EQ(context->promise_map.size(), 1);

    HandleResume(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    ASSERT_EQ(context->promise_map.size(), 0);
}


TEST_F(ProposerTest, testHandleResumeSendsNextRequestIfThereArePendingProposals)
{
    Decree decree(Replica("A"), 1, "content", DecreeType::UserDecree);
    Message message(
        decree,
        Replica("from"),
        Replica("to"),
        MessageType::AcceptMessage);

    auto replicaset = std::make_shared<ReplicaSet>();
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    ledger->Append(decree);
    auto signal = std::make_shared<Signal>();
    auto context = std::make_shared<ProposerContext>(
        replicaset,
        ledger,
        std::make_shared<VolatileDecree>(),
        [](std::string entry){},
        std::make_shared<NoPause>(),
        signal
    );
    context->requested_values.push_back(std::make_tuple("another pending value", DecreeType::UserDecree));
    auto sender = std::shared_ptr<FakeSender>(new FakeSender());

    HandleResume(message, context, sender);

    ASSERT_EQ(sender->sentMessages()[0].type, MessageType::RequestMessage);
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
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::NackTieMessage));
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
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::NackMessage);
}


TEST_F(AcceptorTest, testHandlePrepareWithEqualRootDecreeNumberFromMultipleReplicasSendsSinglePromise)
{
    Message author_a(Decree(Replica("author_a"), 1, "", DecreeType::UserDecree), Replica("from"), Replica("to"), MessageType::PrepareMessage);
    Message author_b(Decree(Replica("author_b"), 1, "", DecreeType::UserDecree), Replica("from"), Replica("to"), MessageType::PrepareMessage);

    // The root number of author_a (1) equals root number of author_b (1).
    author_b.decree.root_number = 1;

    auto context = createAcceptorContext();
    context->promised_decree = Decree(Replica("the_author"), 0, "", DecreeType::UserDecree);

    auto sender = std::make_shared<FakeSender>();

    HandlePrepare(author_a, context, sender);
    HandlePrepare(author_b, context, sender);

    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::PromiseMessage);
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::NackTieMessage);

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
    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, MessageType::NackTieMessage);

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

        replicaset = std::make_shared<ReplicaSet>();
        queue = std::make_shared<RolloverQueue<Decree>>(sstream);
        ledger = std::make_shared<Ledger>(queue);
        context = std::make_shared<LearnerContext>(replicaset, ledger);
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

    std::shared_ptr<ReplicaSet> replicaset;
    std::stringstream sstream;
    std::shared_ptr<RolloverQueue<Decree>> queue;
    std::shared_ptr<Ledger> ledger;
    std::shared_ptr<LearnerContext> context;
};


TEST_F(LearnerTest, testRegisterLearnerWillRegistereMessageTypes)
{
    auto receiver = std::make_shared<FakeReceiver>();
    auto sender = std::make_shared<FakeSender>();

    RegisterLearner(receiver, sender, context);

    ASSERT_TRUE(receiver->IsMessageTypeRegister(MessageType::AcceptedMessage));
    ASSERT_TRUE(receiver->IsMessageTypeRegister(MessageType::UpdatedMessage));

    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::RequestMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::PrepareMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::PromiseMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::NackTieMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::AcceptMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::UpdateMessage));
}


TEST_F(LearnerTest, testAcceptedHandleWithSingleReplica)
{
    Message message(Decree(Replica("A"), 1, "", DecreeType::UserDecree), Replica("A"), Replica("A"), MessageType::AcceptedMessage);
    replicaset->Add(Replica("A"));

    HandleAccepted(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    ASSERT_EQ(GetQueueSize(queue), 1);
}


TEST_F(LearnerTest, testAcceptedHandleIgnoresMessagesFromUnknownReplica)
{
    Message message(Decree(Replica("A"), 1, "", DecreeType::UserDecree), Replica("Unknown"), Replica("A"), MessageType::AcceptedMessage);
    replicaset->Add(Replica("A"));

    HandleAccepted(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    ASSERT_EQ(GetQueueSize(queue), 0);
}


TEST_F(LearnerTest, testAcceptedHandleReceivesOneAcceptedWithThreeReplicaSet)
{
    Message message(Decree(Replica("A"), 1, "", DecreeType::UserDecree), Replica("A"), Replica("B"), MessageType::AcceptedMessage);
    replicaset->Add(Replica("A"));
    replicaset->Add(Replica("B"));
    replicaset->Add(Replica("C"));

    HandleAccepted(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    ASSERT_EQ(GetQueueSize(queue), 0);
}


TEST_F(LearnerTest, testAcceptedHandleReceivesTwoAcceptedWithThreeReplicaSet)
{
    replicaset->Add(Replica("A"));
    replicaset->Add(Replica("B"));
    replicaset->Add(Replica("C"));

    HandleAccepted(
        Message(
            Decree(Replica("A"), 1, "", DecreeType::UserDecree),
            Replica("A"),
            Replica("B"),
            MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );
    HandleAccepted(
        Message(
            Decree(Replica("A"), 1, "", DecreeType::UserDecree),
            Replica("B"),
            Replica("B"),
            MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );

    ASSERT_EQ(GetQueueSize(queue), 1);
}


TEST_F(LearnerTest, testAcceptedHandleCleansUpAcceptedMapAfterAllVotesReceived)
{
    replicaset->Add(Replica("A"));
    replicaset->Add(Replica("B"));
    replicaset->Add(Replica("C"));

    Decree decree(Replica("A"), 1, "", DecreeType::UserDecree);

    HandleAccepted(
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

    HandleAccepted(
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

    HandleAccepted(
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


TEST_F(LearnerTest, testAcceptedHandleIgnoresPreviouslyAcceptedMessagesFromReplicasRemovedFromReplicaSet)
{
    replicaset->Add(Replica("A"));
    replicaset->Add(Replica("B"));
    replicaset->Add(Replica("C"));
    Decree decree(Replica("A"), 1, "", DecreeType::UserDecree);

    // Accepted from replica A.
    HandleAccepted(
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

    HandleAccepted(
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
    ASSERT_EQ(GetQueueSize(queue), 0);
}


TEST_F(LearnerTest, testAcceptedHandleIgnoresDuplicateAcceptedMessages)
{
    replicaset->Add(Replica("A"));
    replicaset->Add(Replica("B"));
    replicaset->Add(Replica("C"));

    HandleAccepted(
        Message(
            Decree(Replica("A"), 1, "", DecreeType::UserDecree),
            Replica("A"),
            Replica("B"),
            MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );
    HandleAccepted(
        Message(
            Decree(Replica("A"), 1, "", DecreeType::UserDecree),
            Replica("A"),
            Replica("B"),
            MessageType::AcceptedMessage
        ),
        context,
        std::shared_ptr<FakeSender>(new FakeSender())
    );

    ASSERT_EQ(GetQueueSize(queue), 0);
}


TEST_F(LearnerTest, testAcceptedHandleDoesNotWriteInLedgerIfTheLastDecreeInTheLedgerIsOutOfOrder)
{
    replicaset->Add(Replica("A"));
    auto sender = std::make_shared<FakeSender>();

    HandleAccepted(
        Message(
            Decree(Replica("A"), 1, "", DecreeType::UserDecree),
            Replica("A"), Replica("A"),
            MessageType::AcceptedMessage
        ),
        context,
        sender
    );

    // Missing decrees 2-9 so don't write to ledger yet.
    HandleAccepted(
        Message(
            Decree(Replica("A"), 10, "", DecreeType::UserDecree),
            Replica("A"), Replica("A"),
            MessageType::AcceptedMessage
        ),
        context,
        sender
    );

    ASSERT_EQ(GetQueueSize(queue), 1);
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::UpdateMessage);
}


TEST_F(LearnerTest, testAcceptedHandleAppendsToLedgerAfterComparingAgainstOriginalDecree)
{
    Message message(Decree(Replica("A"), 42, "", DecreeType::UserDecree), Replica("A"), Replica("A"), MessageType::AcceptedMessage);
    message.decree.root_number = 1;
    replicaset->Add(Replica("A"));

    HandleAccepted(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    // Even though actual decree 42 is out-of-order, original is 1 which is in-order.
    ASSERT_EQ(GetQueueSize(queue), 1);
    ASSERT_EQ(context->tracked_future_decrees.size(), 0);

    // Actual decree should be appended
    ASSERT_TRUE(IsDecreeIdentical(context->ledger->Tail(), message.decree));
}


TEST_F(LearnerTest, testAcceptedHandleTracksFutureDecreesIfReceivedOutOfOrder)
{
    Message message(Decree(Replica("A"), 42, "", DecreeType::UserDecree), Replica("A"), Replica("A"), MessageType::AcceptedMessage);
    replicaset->Add(Replica("A"));
    context->is_observer = true;

    HandleAccepted(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    // Mode is_observer should not write to ledger.
    ASSERT_EQ(GetQueueSize(queue), 0);
    ASSERT_EQ(context->tracked_future_decrees.size(), 1);
}


TEST_F(LearnerTest, testAcceptedHandleDoesNotTrackPastDecrees)
{
    Decree past_decree(Replica("A"), 1, "", DecreeType::UserDecree);
    Decree future_decree(Replica("A"), 1, "", DecreeType::UserDecree);

    Message message(past_decree, Replica("A"), Replica("A"), MessageType::AcceptedMessage);
    replicaset->Add(Replica("A"));
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
    Decree past_decree(Replica("A"), 1, "", DecreeType::UserDecree);
    Decree current_decree(Replica("A"), 2, "", DecreeType::UserDecree);

    Message message(current_decree, Replica("A"), Replica("A"), MessageType::AcceptedMessage);
    replicaset->Add(Replica("A"));
    context->tracked_future_decrees.push(past_decree);

    HandleAccepted(message, context, std::shared_ptr<FakeSender>(new FakeSender()));

    // Past decree from tracked future decrees and current decree are both appended to ledger.
    ASSERT_EQ(GetQueueSize(queue), 2);
}


TEST_F(LearnerTest, testAcceptedHandleSendsResumeWhenMessagedDecreeIsEqualToLedger)
{
    Message message(Decree(Replica("A"), 1, "", DecreeType::UserDecree), Replica("A"), Replica("A"), MessageType::AcceptedMessage);
    replicaset->Add(Replica("A"));
    auto sender = std::make_shared<FakeSender>();

    HandleAccepted(message, context, sender);

    // Reset sender to erase message queue.
    sender = std::make_shared<FakeSender>();
    ASSERT_EQ(0, sender->sentMessages().size());

    HandleAccepted(message, context, sender);

    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::ResumeMessage);
    ASSERT_EQ(sender->sentMessages()[0].decree.number, 1);
}


TEST_F(LearnerTest, testAcceptedHandleSendsResumeWhenAcceptedIsGreaterThanQuorum)
{
    Message message(Decree(Replica("A"), 1, "", DecreeType::UserDecree), Replica("A"), Replica("A"), MessageType::AcceptedMessage);
    replicaset->Add(Replica("A"));
    replicaset->Add(Replica("B"));
    replicaset->Add(Replica("C"));
    auto sender = std::make_shared<FakeSender>();

    // A replica accepted.
    HandleAccepted(
        Message(
            Decree(Replica("A"), 1, "", DecreeType::UserDecree),
            Replica("A"),
            Replica("A"),
            MessageType::AcceptedMessage),
        context, sender);

    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, MessageType::ResumeMessage);

    // B replica accepted.
    HandleAccepted(
        Message(
            Decree(Replica("A"), 1, "", DecreeType::UserDecree),
            Replica("B"),
            Replica("A"),
            MessageType::AcceptedMessage),
        context, sender);

    // Have quorum, should send resume.
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::ResumeMessage);

    // Reset sender to erase message queue.
    sender = std::make_shared<FakeSender>();

    // C replica accepted.
    HandleAccepted(
        Message(
            Decree(Replica("A"), 1, "", DecreeType::UserDecree),
            Replica("C"),
            Replica("A"),
            MessageType::AcceptedMessage),
        context, sender);

    // Still have quorum, should send resume, again.
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::ResumeMessage);
    ASSERT_EQ(sender->sentMessages()[0].decree.number, 1);
}


TEST_F(LearnerTest, testHandleUpdatedWithEmptyLedger)
{
    replicaset->Add(Replica("A"));
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

    ASSERT_EQ(GetQueueSize(queue), 0);

    // We are still behind so we should not resume.
    ASSERT_MESSAGE_TYPE_NOT_SENT(sender, MessageType::ResumeMessage);
}


TEST_F(LearnerTest, testHandleUpdatedReceivedMessageWithZeroDecree)
{
    replicaset->Add(Replica("A"));
    context->ledger->Append(Decree(Replica("A"), 10, "", DecreeType::UserDecree));
    auto sender = std::make_shared<FakeSender>();

    // Sending zero decree.
    HandleUpdated(
        Message(
            Decree(Replica("A"), 0, "", DecreeType::UserDecree),
            Replica("A"), Replica("A"),
            MessageType::UpdatedMessage
        ),
        context,
        sender
    );

    // We are up to date so send resume.
    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::ResumeMessage);

    // Decree number in resume message should be last ledger decree number.
    ASSERT_EQ(sender->sentMessages()[0].decree.number, 10);
}


TEST_F(LearnerTest, testHandleUpdatedReceivesMessageWithNextOrderedDecree)
{
    replicaset->Add(Replica("A"));
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
    ASSERT_EQ(GetQueueSize(queue), 2);
}


TEST_F(LearnerTest, testHandleUpdatedReceivesMessageWithNextOrderedDecreeAndTrackedAdditionalOrderedDecrees)
{
    replicaset->Add(Replica("A"));
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
    ASSERT_EQ(GetQueueSize(queue), 3);
}


TEST_F(LearnerTest, testHandleUpdatedReceivesMessageWithNextOrderedDecreeAndTrackedAdditionalUnorderedDecrees)
{
    replicaset->Add(Replica("A"));
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
    ASSERT_EQ(GetQueueSize(queue), 1);

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
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    auto context = std::make_shared<UpdaterContext>(
        ledger
    );

    RegisterUpdater(receiver, sender, context);

    ASSERT_TRUE(receiver->IsMessageTypeRegister(MessageType::UpdateMessage));

    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::RequestMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::PrepareMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::PromiseMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::NackTieMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::AcceptMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::AcceptedMessage));
    ASSERT_FALSE(receiver->IsMessageTypeRegister(MessageType::UpdatedMessage));
}


TEST_F(UpdaterTest, testHandleUpdateReceivesMessageWithDecreeAndHasEmptyLedger)
{
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    auto context = std::make_shared<UpdaterContext>(
        ledger
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

    ASSERT_MESSAGE_TYPE_SENT(sender, MessageType::UpdatedMessage);

    // Our ledger is empty so we will send an zero decree to show we have nothing.
    ASSERT_EQ(sender->sentMessages()[0].decree.number, 0);
}


TEST_F(UpdaterTest, testHandleUpdateReceivesMessageWithDecreeAndLedgerHasNextDecree)
{
    std::stringstream ss;
    auto ledger = std::make_shared<Ledger>(
        std::make_shared<RolloverQueue<Decree>>(ss)
    );
    auto context = std::make_shared<UpdaterContext>(
        ledger
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
