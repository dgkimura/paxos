#include "gtest/gtest.h"

#include "paxos/replicaset.hpp"


TEST(ReplicaSetUnittest, testReplicaSetSizeInitializedToZero)
{
    ReplicaSet set;

    ASSERT_EQ(set.GetSize(), 0);
}


TEST(ReplicaSetUnittest, testAddReplicaIncrementsReplicaSetSize)
{
    ReplicaSet set;
    Replica replica("host1");
    set.Add(replica);

    ASSERT_EQ(set.GetSize(), 1);
}


TEST(ReplicaSetUnittest, testRemoveReplicaDecrementsReplicaSetSize)
{
    ReplicaSet set;
    Replica replica("host1");

    set.Add(replica);
    ASSERT_EQ(set.GetSize(), 1);

    set.Remove(replica);
    ASSERT_EQ(set.GetSize(), 0);
}


TEST(ReplicaSetUnittest, testIterateReplicaSet)
{
    ReplicaSet set;
    Replica first("myhost");
    set.Add(first);

    Replica second("yourhost");
    set.Add(second);

    Replica third("theirhost");
    set.Add(third);

    std::set<std::string> expected = {"myhost", "yourhost", "theirhost"};

    for (Replica r : set)
    {
        ASSERT_TRUE(expected.count(r.hostname) > 0);
    }
    ASSERT_EQ(set.GetSize(), 3);
}


TEST(ReplicaSetUnittest, testClearReplicaSetRemovesAllReplicas)
{
    ReplicaSet set;

    Replica first("myhost");
    Replica second("yourhost");
    Replica third("theirhost");

    set.Add(first);
    set.Add(second);
    set.Add(third);

    set.Clear();

    ASSERT_EQ(set.GetSize(), 0);
}


TEST(ReplicaSetUnittest, testContainsOnEmptyReplicaSet)
{
    ReplicaSet set;
    Replica replica("host1");

    ASSERT_FALSE(set.Contains(replica));
}


TEST(ReplicaSetUnittest, testContainsOnReplicaSetWithReplica)
{
    ReplicaSet set;
    set.Add(Replica("host1"));

    ASSERT_TRUE(set.Contains(Replica("host1")));
}


TEST(ReplicaSetUnittest, testIntersectionBetweenReplicaSets)
{
    auto set_a = std::make_shared<ReplicaSet>();
    set_a->Add(Replica("host"));

    auto set_b = std::make_shared<ReplicaSet>();
    set_b->Add(Replica("host"));

    auto set_c = set_a->Intersection(set_b);

    ASSERT_TRUE(set_c->Contains(Replica("host")));
}


TEST(ReplicaSetUnittest, testDifferenceBetweenReplicaSets)
{
    auto set_a = std::make_shared<ReplicaSet>();
    set_a->Add(Replica("A"));
    set_a->Add(Replica("B"));
    set_a->Add(Replica("C"));

    auto set_b = std::make_shared<ReplicaSet>();
    set_b->Add(Replica("B"));
    set_b->Add(Replica("C"));

    auto set_c = set_a->Difference(set_b);

    ASSERT_TRUE(set_c->Contains(Replica("A")));
    ASSERT_FALSE(set_c->Contains(Replica("B")));
    ASSERT_FALSE(set_c->Contains(Replica("C")));
}


TEST(ReplicaTest, testReplicaFields)
{
    Replica r("host", 1234);

    ASSERT_EQ(r.hostname, "host");
    ASSERT_EQ(r.port, 1234);
}


TEST(ReplicaTest, testSaveReplicaSetWritesIntoStream)
{
    Replica r("host", 1234);
    std::stringstream replicasetstream;

    auto set = std::make_shared<ReplicaSet>();
    set->Add(Replica("host1", 80));
    set->Add(Replica("host2", 8080));

    SaveReplicaSet(set, replicasetstream);
    ASSERT_EQ("host1:80\nhost2:8080\n", replicasetstream.str());
}
