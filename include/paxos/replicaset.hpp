#ifndef __REPLICASET_HPP_INCLUDED__
#define __REPLICASET_HPP_INCLUDED__

#include <iterator>
#include <memory>
#include <set>
#include <string>


struct Replica
{
    std::string hostname;

    short port;

    Replica()
        : hostname(),
          port()
    {
    }

    Replica(std::string h, short p=8081)
        : hostname(h),
          port(p)
    {
    }
};


bool IsReplicaEqual(const Replica& lhs, const Replica& rhs);


struct compare_replica
{
    bool operator()(const Replica& lhs, const Replica& rhs) const
    {
        return lhs.hostname != rhs.hostname ?
               lhs.hostname < rhs.hostname : lhs.port < rhs.port;
    }
};


class ReplicaSet
{
public:

    ReplicaSet();

    ~ReplicaSet();

    void Add(Replica replica);

    void Remove(Replica replica);

    bool Contains(Replica replica);

    int GetSize();

    void Clear();

    std::shared_ptr<ReplicaSet> Intersection(std::shared_ptr<ReplicaSet> other);

    std::shared_ptr<ReplicaSet> Difference(std::shared_ptr<ReplicaSet> other);

    using iterator = std::set<Replica, compare_replica>::iterator;

    using const_iterator = std::set<Replica, compare_replica>::const_iterator;

    ReplicaSet::iterator begin() const;

    ReplicaSet::iterator end() const;

private:

    std::set<Replica, compare_replica> replicaset;
};


std::shared_ptr<ReplicaSet>
LoadReplicaSet(std::string directory);


#endif
