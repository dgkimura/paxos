#include <sstream>
#include <gtest/gtest.h>

#include "paxos/context.hpp"
#include "paxos/ledger.hpp"
#include "paxos/replicaset.hpp"


TEST(ContextUnitTest, testLearnerContextTrackedFutureDecreeStoredInAscendingOrder)
{
    auto replicaset = std::make_shared<ReplicaSet>();
    std::stringstream ss;
    auto queue = std::make_shared<RolloverQueue<Decree>>(ss);
    auto ledger = std::make_shared<Ledger>(queue);
    LearnerContext context(
        replicaset,
        ledger
    );

    context.tracked_future_decrees.push(Decree(Replica("A"), 2, "", DecreeType::UserDecree));
    context.tracked_future_decrees.push(Decree(Replica("A"), 1, "", DecreeType::UserDecree));
    context.tracked_future_decrees.push(Decree(Replica("A"), 3, "", DecreeType::UserDecree));

    ASSERT_EQ(context.tracked_future_decrees.top().number, 1);
}
