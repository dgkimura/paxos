#include <initializer_list>
#include <string>

#include "gtest/gtest.h"

#include "learner.hpp"


std::shared_ptr<LearnerContext> createLearnerContext(std::initializer_list<std::string> authors)
{
    std::shared_ptr<LearnerContext> context(new LearnerContext());

    ReplicaSet replicaset;
    for (auto a : authors)
    {
        Replica r(a);
        replicaset.Add(r);
    }

    context->ledger = std::shared_ptr<VolatileLedger>(new VolatileLedger());
    context->full_replicaset = replicaset;

    return context;
}


TEST(LearnerTest, testProclaimHandleWithSingleReplica)
{
    Message message(Decree("A", 1, ""), Replica("A"), Replica("A"));
    std::shared_ptr<LearnerContext> context = createLearnerContext({"A"});

    HandleProclaim(message, context, std::shared_ptr<Sender>(new Sender()));

    ASSERT_TRUE(context->ledger->Contains(message.decree));
}


TEST(LearnerTest, testProclaimHandleIgnoresMessagesFromUnknownReplica)
{
    Message message(Decree("A", 1, ""), Replica("Unknown"), Replica("A"));
    std::shared_ptr<LearnerContext> context = createLearnerContext({"A"});

    HandleProclaim(message, context, std::shared_ptr<Sender>(new Sender()));

    ASSERT_FALSE(context->ledger->Contains(message.decree));
}


TEST(LearnerTest, testProclaimHandleReceivesOneAcceptedWithThreeReplicaSet)
{
    Message message(Decree("A", 1, ""), Replica("A"), Replica("B"));
    std::shared_ptr<LearnerContext> context = createLearnerContext({"A", "B", "C"});

    HandleProclaim(message, context, std::shared_ptr<Sender>(new Sender()));

    ASSERT_FALSE(context->ledger->Contains(message.decree));
}


TEST(LearnerTest, testProclaimHandleReceivesTwoAcceptedWithThreeReplicaSet)
{
    std::shared_ptr<LearnerContext> context = createLearnerContext({"A", "B", "C"});

    HandleProclaim(
        Message(
            Decree("A", 1, ""),
            Replica("A"),
            Replica("B")
        ),
        context,
        std::shared_ptr<Sender>(new Sender())
    );
    HandleProclaim(
        Message(
            Decree("A", 1, ""),
            Replica("B"),
            Replica("B")
        ),
        context,
        std::shared_ptr<Sender>(new Sender())
    );

    ASSERT_TRUE(context->ledger->Contains(Decree("A", 1, "")));
}


TEST(LearnerTest, testProclaimHandleIgnoresDuplicateAcceptedMessages)
{
    std::shared_ptr<LearnerContext> context = createLearnerContext({"A", "B", "C"});

    HandleProclaim(
        Message(
            Decree("A", 1, ""),
            Replica("A"),
            Replica("B")
        ),
        context,
        std::shared_ptr<Sender>(new Sender())
    );
    HandleProclaim(
        Message(
            Decree("A", 1, ""),
            Replica("A"),
            Replica("B")
        ),
        context,
        std::shared_ptr<Sender>(new Sender())
    );

    ASSERT_FALSE(context->ledger->Contains(Decree("A", 1, "")));
}
