#include "gtest/gtest.h"

#include "proposer.hpp"


TEST(ProposerTest, testHandlePromiseWithLowerDecreeDoesNotUpdatesighestPromisedDecree)
{
    Message message(Decree("the_author", -1, ""), Replica("from"), Replica("to"));

    std::shared_ptr<ProposerContext> context(new ProposerContext());
    context->highest_promised_decree = Decree("the_author", 0, "");

    std::shared_ptr<Sender> sender(new Sender());

    HandlePromise(message, context, sender);

    ASSERT_TRUE(IsDecreeLower(message.decree, context->highest_promised_decree));
}


TEST(ProposerTest, testHandlePromiseWithHigherDecreeUpdatesHighestPromisedDecree)
{
    Message message(Decree("the_author", 1, ""), Replica("from"), Replica("to"));

    std::shared_ptr<ProposerContext> context(new ProposerContext());
    context->highest_promised_decree = Decree("the_author", 0, "");

    std::shared_ptr<Sender> sender(new Sender());

    HandlePromise(message, context, sender);

    ASSERT_TRUE(IsDecreeEqual(message.decree, context->highest_promised_decree));
}
