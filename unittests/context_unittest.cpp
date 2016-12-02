#include "gtest/gtest.h"

#include "paxos/context.hpp"
#include "paxos/ledger.hpp"
#include "paxos/replicaset.hpp"


TEST(ContextUnitTest, testLearnerContextTrackedFutureDecreeStoredInAscendingOrder)
{
    LearnerContext context(
        std::make_shared<ReplicaSet>(),
        std::make_shared<Ledger>(std::make_shared<VolatileQueue<Decree>>())
    );

    context.tracked_future_decrees.push(Decree(Replica("A"), 2, ""));
    context.tracked_future_decrees.push(Decree(Replica("A"), 1, ""));
    context.tracked_future_decrees.push(Decree(Replica("A"), 3, ""));

    ASSERT_EQ(context.tracked_future_decrees.top().number, 1);
}
