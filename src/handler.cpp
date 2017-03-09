#include <cstdio>

#include <boost/filesystem.hpp>

#include "paxos/bootstrap.hpp"
#include "paxos/handler.hpp"
#include "paxos/serialization.hpp"


void
EmptyDecreeHandler::operator()(std::string entry)
{
}


CompositeHandler::CompositeHandler()
    : handlers()
{
}


CompositeHandler::CompositeHandler(Handler handler)
    : handlers()
{
    AddHandler(handler);
}


void
CompositeHandler::operator()(std::string entry)
{
    for (auto h : handlers)
    {
        h(entry);
    }
}


void
CompositeHandler::AddHandler(Handler handler)
{
    handlers.push_back(handler);
}


HandleAddReplica::HandleAddReplica(
    std::string location,
    Replica legislator,
    std::shared_ptr<ReplicaSet>& legislators)
    : location(location),
      legislator(legislator),
      legislators(legislators)
{
}


void
HandleAddReplica::operator()(std::string entry)
{
    UpdateReplicaSetDecree decree = Deserialize<UpdateReplicaSetDecree>(entry);
    legislators->Add(decree.replica);
    std::ofstream replicasetfile(
        (boost::filesystem::path(location) /
         boost::filesystem::path(ReplicasetFilename)).string());
    SaveReplicaSet(legislators, replicasetfile);

    // Only decree author sends bootstrap.
    if (decree.author.hostname == legislator.hostname &&
        decree.author.port == legislator.port)
    {
        SendBootstrap<BoostTransport>(
            decree.replica,
            location,
            decree.remote_directory);
    }
}


HandleRemoveReplica::HandleRemoveReplica(
    std::string location,
    std::shared_ptr<ReplicaSet>& legislators)
    : location(location),
      legislators(legislators)
{
}


void
HandleRemoveReplica::operator()(std::string entry)
{
    UpdateReplicaSetDecree decree = Deserialize<UpdateReplicaSetDecree>(entry);
    legislators->Remove(decree.replica);
    std::ofstream replicasetfile(
        (boost::filesystem::path(location) /
         boost::filesystem::path(ReplicasetFilename)).string());
    SaveReplicaSet(legislators, replicasetfile);
}


HandleDistributedLock::HandleDistributedLock(
    Replica replica,
    std::string location,
    std::string lockname,
    std::shared_ptr<Signal> signal)
    : replica(replica),
      location(location),
      lockname(lockname),
      signal(signal)
{
}


void
HandleDistributedLock::operator()(std::string entry)
{
    DistributedLockDecree decree = Deserialize<DistributedLockDecree>(entry);
    if (decree.lock && !boost::filesystem::exists(
        (boost::filesystem::path(location) /
        boost::filesystem::path(lockname)).string()))
    {
        std::ofstream lockfile(
            (boost::filesystem::path(location) /
             boost::filesystem::path(lockname)).string());
        lockfile << Serialize(replica);
    }
    else
    {
        std::remove(
            (boost::filesystem::path(location) /
             boost::filesystem::path(lockname)).string().c_str());
    }
    signal->Set();
}
