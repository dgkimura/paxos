#include <replicaset.hpp>


ReplicaSet::ReplicaSet()
    : replicaset()
{
}


ReplicaSet::~ReplicaSet()
{
}


void
ReplicaSet::Add(Replica replica)
{
    replicaset.insert(replica);
}


void
ReplicaSet::Remove(Replica replica)
{
    replicaset.erase(replica);
}


bool
ReplicaSet::Contains(Replica replica)
{
    return replicaset.find(replica) != replicaset.end();
}


int
ReplicaSet::GetSize()
{
    return replicaset.size();
}


void
ReplicaSet::Clear()
{
    replicaset.clear();
}


std::shared_ptr<ReplicaSet>
ReplicaSet::Intersection(std::shared_ptr<ReplicaSet> other)
{
    std::shared_ptr<ReplicaSet> intersection = std::make_shared<ReplicaSet>();
    for (auto r : *other)
    {
        if (Contains(r))
        {
            intersection->Add(r);
        }
    }
    return intersection;
}


ReplicaSet::iterator
ReplicaSet::begin() const
{
    return replicaset.begin();
}


ReplicaSet::iterator
ReplicaSet::end() const
{
    return replicaset.end();
}
