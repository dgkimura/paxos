#include <cstdio>
#include <memory>

#include <boost/filesystem.hpp>

#include "paxos/bootstrap.hpp"
#include "paxos/handler.hpp"
#include "paxos/serialization.hpp"


namespace paxos
{


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
    std::shared_ptr<Signal> signal,
    std::function<void(
        std::shared_ptr<ReplicaSet>,
        std::ostream&)> save_replicaset)
    : location(location),
      legislator(legislator),
      legislators(legislators),
      signal(signal),
      save_replicaset(save_replicaset)
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
    save_replicaset(legislators, replicasetfile);

    // Only decree author sends bootstrap.
    if (decree.author.hostname == legislator.hostname &&
        decree.author.port == legislator.port)
    {
        std::vector<boost::filesystem::directory_entry> filepaths;
        if(boost::filesystem::is_directory(location))
        {
            std::copy(boost::filesystem::directory_iterator(location),
                      boost::filesystem::directory_iterator(),
                      std::back_inserter(filepaths));
        }
        SendBootstrap(
            location,
            decree.remote_directory,
            filepaths,
            [&](BootstrapFile file){
                NetworkFileSender<BoostTransport> sender;
                sender.SendFile(decree.replica, file);
            });
        signal->Set(true);
    }
}


HandleRemoveReplica::HandleRemoveReplica(
    std::string location,
    Replica legislator,
    std::shared_ptr<ReplicaSet>& legislators,
    std::shared_ptr<Signal> signal,
    std::function<void(
        std::shared_ptr<ReplicaSet>,
        std::ostream&)> save_replicaset)
    : location(location),
      legislator(legislator),
      legislators(legislators),
      signal(signal),
      save_replicaset(save_replicaset)
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
    save_replicaset(legislators, replicasetfile);

    signal->Set(true);
}


}
