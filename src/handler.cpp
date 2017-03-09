#include <boost/filesystem.hpp>

#include "paxos/bootstrap.hpp"
#include "paxos/handler.hpp"
#include "paxos/replicaset.hpp"
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


HandleAddNode::HandleAddNode(
    std::string location,
    Replica legislator,
    std::shared_ptr<ReplicaSet>& legislators)
    : location(location),
      legislator(legislator),
      legislators(legislators)
{
}


void
HandleAddNode::operator()(std::string entry)
{
    SystemOperation op = Deserialize<SystemOperation>(entry);
    legislators->Add(op.replica);
    std::ofstream replicasetfile(
        (boost::filesystem::path(location) /
         boost::filesystem::path(ReplicasetFilename)).string());
    SaveReplicaSet(legislators, replicasetfile);

    // Only decree author sends bootstrap.
    if (op.author.hostname == legislator.hostname &&
        op.author.port == legislator.port)
    {
        SendBootstrap<BoostTransport>(
            op.replica,
            Deserialize<BootstrapMetadata>(op.content));
    }
}


HandleRemoveNode::HandleRemoveNode(
    std::string location,
    std::shared_ptr<ReplicaSet>& legislators)
    : location(location),
      legislators(legislators)
{
}


void
HandleRemoveNode::operator()(std::string entry)
{
    SystemOperation op = Deserialize<SystemOperation>(entry);
    legislators->Remove(op.replica);
    std::ofstream replicasetfile(
        (boost::filesystem::path(location) /
         boost::filesystem::path(ReplicasetFilename)).string());
    SaveReplicaSet(legislators, replicasetfile);
}
