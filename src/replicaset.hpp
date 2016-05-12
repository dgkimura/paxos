#include <set>
#include <string>


struct Replica
{
    std::string hostname;
};


struct compare_replica
{
    bool operator()(const Replica& lhs, const Replica& rhs) const
    {
        return lhs.hostname < rhs.hostname;
    }
};


class ReplicaSet
{
public:

    ReplicaSet();

    ~ReplicaSet();

    void Add(Replica replica);

    void Remove(Replica replica);

    int GetSize();

private:

    std::set<Replica, compare_replica> replicaset;
};
