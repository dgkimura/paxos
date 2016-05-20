#include "gtest/gtest.h"

#include "acceptor.hpp"


TEST(AcceptorTest, testHandlePrepareWithHigherDecreeUpdatesPromisedDecree)
{
    Message message(Decree("the_author", 1, ""), Replica("from"), Replica("to"));

    std::shared_ptr<AcceptorContext> context(new AcceptorContext());
    context->promised_decree = Decree("the_author", 0, "");

    std::shared_ptr<Sender> sender(new Sender());

    HandlePrepare(message, context, sender);

    ASSERT_TRUE(IsDecreeEqual(message.decree, context->promised_decree));
}


TEST(AcceptorTest, testHandlePrepareWithLowerDecreeDoesNotUpdatePromisedDecree)
{
    Message message(Decree("the_author", -1, ""), Replica("from"), Replica("to"));

    std::shared_ptr<AcceptorContext> context(new AcceptorContext());
    context->promised_decree = Decree("the_author", 1, "");

    std::shared_ptr<Sender> sender(new Sender());

    HandlePrepare(message, context, sender);

    ASSERT_TRUE(IsDecreeLower(message.decree, context->promised_decree));
}


TEST(AcceptorTest, testHandleAcceptWithLowerDecreeDoesNotUpdateAcceptedDecree)
{
    Message message(Decree("the_author", -1, ""), Replica("from"), Replica("to"));

    std::shared_ptr<AcceptorContext> context(new AcceptorContext());
    context->promised_decree = Decree("the_author", 1, "");
    context->accepted_decree = Decree("the_author", 1, "");

    std::shared_ptr<Sender> sender(new Sender());

    HandleAccept(message, context, sender);

    ASSERT_TRUE(IsDecreeLower(message.decree, context->accepted_decree));
}


TEST(AcceptorTest, testHandleAcceptWithEqualDecreeDoesNotUpdateAcceptedDecree)
{
    Message message(Decree("the_author", 1, ""), Replica("from"), Replica("to"));

    std::shared_ptr<AcceptorContext> context(new AcceptorContext());
    context->promised_decree = Decree("the_author", 1, "");
    context->accepted_decree = Decree("the_author", 1, "");

    std::shared_ptr<Sender> sender(new Sender());

    HandleAccept(message, context, sender);

    ASSERT_TRUE(IsDecreeLowerOrEqual(message.decree, context->accepted_decree));
}


TEST(AcceptorTest, testHandleAcceptWithHigherDecreeDoesUpdateAcceptedDecree)
{
    Message message(Decree("the_author", 2, ""), Replica("from"), Replica("to"));

    std::shared_ptr<AcceptorContext> context(new AcceptorContext());
    context->promised_decree = Decree("the_author", 1, "");
    context->accepted_decree = Decree("the_author", 1, "");

    std::shared_ptr<Sender> sender(new Sender());

    HandleAccept(message, context, sender);

    ASSERT_TRUE(IsDecreeEqual(message.decree, context->accepted_decree));
}
