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
    std::shared_ptr<ReplicaSet>& legislators,
    std::shared_ptr<Signal> signal)
    : location(location),
      legislator(legislator),
      legislators(legislators),
      signal(signal)
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
        signal->Set();
    }
}


HandleRemoveReplica::HandleRemoveReplica(
    std::string location,
    Replica legislator,
    std::shared_ptr<ReplicaSet>& legislators,
    std::shared_ptr<Signal> signal)
    : location(location),
      legislator(legislator),
      legislators(legislators),
      signal(signal)
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

    if (decree.author.hostname == legislator.hostname &&
        decree.author.port == legislator.port)
    {
        signal->Set();
    }
}
