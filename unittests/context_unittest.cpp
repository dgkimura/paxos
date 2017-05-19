#include "gtest/gtest.h"

#include "paxos/context.hpp"
#include "paxos/ledger.hpp"
#include "paxos/replicaset.hpp"


TEST(ContextUnitTest, testLearnerContextTrackedFutureDecreeStoredInAscendingOrder)
{
    auto replicaset = std::make_shared<ReplicaSet>();
    LearnerContext context(
        replicaset,
        std::make_shared<Ledger>(std::make_shared<VolatileQueue<Decree>>())
    );

    context.tracked_future_decrees.push(Decree(Replica("A"), 2, "", DecreeType::UserDecree));
    context.tracked_future_decrees.push(Decree(Replica("A"), 1, "", DecreeType::UserDecree));
    context.tracked_future_decrees.push(Decree(Replica("A"), 3, "", DecreeType::UserDecree));

    ASSERT_EQ(context.tracked_future_decrees.top().number, 1);
}
