#include <functional>

#include "gtest/gtest.h"

#include "paxos/handler.hpp"


TEST(HandlerTest, testCompositeDecreeHandlerWithSingleHandler)
{
    bool was_called = false;
    paxos::CompositeHandler handler;
    handler.AddHandler([&was_called](std::string entry){ was_called=true; });

    ASSERT_FALSE(was_called);

    handler("some entry");

    ASSERT_TRUE(was_called);
}


TEST(HandlerTest, testCompositeDecreeHandlerWithMultipleHandlers)
{
    bool was_called = false;
    bool was_called_too = false;

    paxos::CompositeHandler handler;
    handler.AddHandler([&was_called](std::string entry){ was_called=true; });
    handler.AddHandler([&was_called_too](std::string entry){ was_called_too=true; });

    ASSERT_FALSE(was_called);
    ASSERT_FALSE(was_called_too);

    handler("some entry");

    ASSERT_TRUE(was_called);
    ASSERT_TRUE(was_called_too);
}


TEST(HandlerTest, testHandleAddReplicaSavesNewReplicaSet)
{
    auto replica = paxos::Replica("host", 8080);
    auto replicaset = std::make_shared<paxos::ReplicaSet>();
    replicaset->Add(replica);

    bool called_saved_replicaset = false;

    paxos::HandleAddReplica handler(
        "mylocation",
        replica,
        replicaset,
        std::make_shared<paxos::Signal>(),
        [&called_saved_replicaset](
            std::shared_ptr<paxos::ReplicaSet>,
            std::ostream&)
        {
            called_saved_replicaset = true;
        }
    );
    handler("a replica");

    ASSERT_TRUE(called_saved_replicaset);
}
