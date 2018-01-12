#include "gtest/gtest.h"

#include "paxos/replicaset.hpp"


TEST(ReplicaSetUnittest, testReplicaSetSizeInitializedToZero)
{
    paxos::ReplicaSet set;

    ASSERT_EQ(set.GetSize(), 0);
}


TEST(ReplicaSetUnittest, testAddReplicaIncrementsReplicaSetSize)
{
    paxos::ReplicaSet set;
    paxos::Replica replica("host1");
    set.Add(replica);

    ASSERT_EQ(set.GetSize(), 1);
}


TEST(ReplicaSetUnittest, testRemoveReplicaDecrementsReplicaSetSize)
{
    paxos::ReplicaSet set;
    paxos::Replica replica("host1");

    set.Add(replica);
    ASSERT_EQ(set.GetSize(), 1);

    set.Remove(replica);
    ASSERT_EQ(set.GetSize(), 0);
}


TEST(ReplicaSetUnittest, testIterateReplicaSet)
{
    paxos::ReplicaSet set;
    paxos::Replica first("myhost");
    set.Add(first);

    paxos::Replica second("yourhost");
    set.Add(second);

    paxos::Replica third("theirhost");
    set.Add(third);

    std::set<std::string> expected = {"myhost", "yourhost", "theirhost"};

    for (paxos::Replica r : set)
    {
        ASSERT_TRUE(expected.count(r.hostname) > 0);
    }
    ASSERT_EQ(set.GetSize(), 3);
}


TEST(ReplicaSetUnittest, testClearReplicaSetRemovesAllReplicas)
{
    paxos::ReplicaSet set;

    paxos::Replica first("myhost");
    paxos::Replica second("yourhost");
    paxos::Replica third("theirhost");

    set.Add(first);
    set.Add(second);
    set.Add(third);

    set.Clear();

    ASSERT_EQ(set.GetSize(), 0);
}


TEST(ReplicaSetUnittest, testContainsOnEmptyReplicaSet)
{
    paxos::ReplicaSet set;
    paxos::Replica replica("host1");

    ASSERT_FALSE(set.Contains(replica));
}


TEST(ReplicaSetUnittest, testContainsOnReplicaSetWithReplica)
{
    paxos::ReplicaSet set;
    set.Add(paxos::Replica("host1"));

    ASSERT_TRUE(set.Contains(paxos::Replica("host1")));
}


TEST(ReplicaSetUnittest, testIntersectionBetweenReplicaSets)
{
    auto set_a = std::make_shared<paxos::ReplicaSet>();
    set_a->Add(paxos::Replica("host"));

    auto set_b = std::make_shared<paxos::ReplicaSet>();
    set_b->Add(paxos::Replica("host"));

    auto set_c = set_a->Intersection(set_b);

    ASSERT_TRUE(set_c->Contains(paxos::Replica("host")));
}


TEST(ReplicaSetUnittest, testDifferenceBetweenReplicaSets)
{
    auto set_a = std::make_shared<paxos::ReplicaSet>();
    set_a->Add(paxos::Replica("A"));
    set_a->Add(paxos::Replica("B"));
    set_a->Add(paxos::Replica("C"));

    auto set_b = std::make_shared<paxos::ReplicaSet>();
    set_b->Add(paxos::Replica("B"));
    set_b->Add(paxos::Replica("C"));

    auto set_c = set_a->Difference(set_b);

    ASSERT_TRUE(set_c->Contains(paxos::Replica("A")));
    ASSERT_FALSE(set_c->Contains(paxos::Replica("B")));
    ASSERT_FALSE(set_c->Contains(paxos::Replica("C")));
}


TEST(ReplicaTest, testReplicaFields)
{
    paxos::Replica r("host", 1234);

    ASSERT_EQ(r.hostname, "host");
    ASSERT_EQ(r.port, 1234);
}


TEST(ReplicaTest, testLoadReplicaSetReadsFromStream)
{
    auto set = paxos::LoadReplicaSet(std::stringstream("host1:80\nhost2:8080\n"));

    ASSERT_EQ(2, set->GetSize());
}


TEST(ReplicaTest, testLoadReplicaSetReadsFromEmptyStream)
{
    auto set = paxos::LoadReplicaSet(std::stringstream(""));

    ASSERT_EQ(0, set->GetSize());
}


TEST(ReplicaTest, testSaveReplicaSetWritesIntoStream)
{
    paxos::Replica r("host", 1234);
    std::stringstream replicasetstream;

    auto set = std::make_shared<paxos::ReplicaSet>();
    set->Add(paxos::Replica("host1", 80));
    set->Add(paxos::Replica("host2", 8080));

    paxos::SaveReplicaSet(set, replicasetstream);
    ASSERT_EQ("host1:80\nhost2:8080\n", replicasetstream.str());
}
