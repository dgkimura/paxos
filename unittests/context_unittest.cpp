#include <sstream>
#include <gtest/gtest.h>

#include "paxos/context.hpp"
#include "paxos/ledger.hpp"
#include "paxos/replicaset.hpp"


TEST(ContextUnitTest, testLearnerContextTrackedFutureDecreeStoredInAscendingOrder)
{
    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    std::stringstream ss;
    auto queue = std::make_shared<paxos::RolloverQueue<paxos::Decree>>(ss);
    auto ledger = std::make_shared<paxos::Ledger>(queue);
    paxos::LearnerContext context(
        replicaset,
        ledger
    );

    context.tracked_future_decrees.push(paxos::Decree(paxos::Replica("A"), 2, "", paxos::DecreeType::UserDecree));
    context.tracked_future_decrees.push(paxos::Decree(paxos::Replica("A"), 1, "", paxos::DecreeType::UserDecree));
    context.tracked_future_decrees.push(paxos::Decree(paxos::Replica("A"), 3, "", paxos::DecreeType::UserDecree));

    ASSERT_EQ(context.tracked_future_decrees.top().number, 1);
}
