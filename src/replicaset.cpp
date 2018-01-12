#include <fstream>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "paxos/replicaset.hpp"


namespace paxos
{


bool
IsReplicaEqual(const Replica& lhs, const Replica& rhs)
{
    return lhs.hostname == rhs.hostname && lhs.port == rhs.port;
}


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
ReplicaSet::Contains(const Replica replica) const
{
    return replicaset.find(replica) != replicaset.end();
}


int
ReplicaSet::GetSize() const
{
    return replicaset.size();
}


void
ReplicaSet::Clear()
{
    replicaset.clear();
}


std::shared_ptr<ReplicaSet>
ReplicaSet::Intersection(std::shared_ptr<ReplicaSet>& other) const
{
    auto intersection = std::make_shared<ReplicaSet>();
    for (auto r : *other)
    {
        if (Contains(r))
        {
            intersection->Add(r);
        }
    }
    return intersection;
}


std::shared_ptr<ReplicaSet>
ReplicaSet::Difference(std::shared_ptr<const ReplicaSet> other) const
{
    auto difference = std::make_shared<ReplicaSet>();
    for (auto r : replicaset)
    {
        if (!other->Contains(r))
        {
            difference->Add(r);
        }
    }
    return difference;
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


std::shared_ptr<ReplicaSet>
LoadReplicaSet(std::istream&& replicasetfile)
{
    auto replicaset = std::make_shared<ReplicaSet>();

    std::string line;
    while (std::getline(replicasetfile, line))
    {
        std::vector<std::string> hostport;
        split(hostport, line, boost::is_any_of(":"));
        replicaset->Add(
            Replica(
                hostport[0],
                boost::lexical_cast<short>(hostport[1])
            )
        );
    }
    return replicaset;
}


void
SaveReplicaSet(std::shared_ptr<ReplicaSet> replicaset, std::ostream& replicasetfile)
{
    for (auto r : *replicaset)
    {
        replicasetfile << r.hostname << ":" << r.port << "\n";
    }
    replicasetfile.flush();
}


}
