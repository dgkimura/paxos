#include "gtest/gtest.h"

#include "proposer.hpp"


TEST(ProposerTest, testHandlePromiseWithLowerDecreeDoesNotUpdatesighestPromisedDecree)
{
    Message message(Decree("the_author", -1, ""), Replica("from"), Replica("to"));

    std::shared_ptr<ProposerContext> context(new ProposerContext());
    context->highest_promised_decree = Decree("the_author", 0, "");

    std::shared_ptr<FakeSender> sender(new FakeSender());

    HandlePromise(message, context, sender);

    ASSERT_TRUE(IsDecreeLower(message.decree, context->highest_promised_decree));
}


TEST(ProposerTest, testHandlePromiseWithHigherDecreeUpdatesHighestPromisedDecree)
{
    Message message(Decree("host", 1, ""), Replica("host"), Replica("host"));

    std::shared_ptr<ProposerContext> context(new ProposerContext());
    context->highest_promised_decree = Decree("host", 0, "");
    context->replicaset = std::shared_ptr<ReplicaSet>(new ReplicaSet());
    context->replicaset->Add(Replica("host"));

    std::shared_ptr<FakeSender> sender(new FakeSender(context->replicaset));

    HandlePromise(message, context, sender);

    ASSERT_TRUE(IsDecreeEqual(message.decree, context->highest_promised_decree));
    ASSERT_EQ(sender->sentMessages().size(), 1);
}
