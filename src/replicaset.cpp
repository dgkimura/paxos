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


ReplicaSet::iterator
ReplicaSet::begin()
{
    return replicaset.begin();
}


ReplicaSet::iterator
ReplicaSet::end()
{
    return replicaset.end();
}
