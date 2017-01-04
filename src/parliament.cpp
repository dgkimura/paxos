#include "paxos/parliament.hpp"


Parliament::Parliament(std::string location, DecreeHandler decree_handler)
    : legislators(LoadReplicaSet(
          std::ifstream(
              (fs::path(location) / fs::path(ReplicasetFilename)).string()))),
      ledger(std::make_shared<Ledger>(
          std::make_shared<PersistentQueue<Decree>>(location, "paxos.ledger"),
          decree_handler,
          DecreeHandler([this, location](std::string entry){
              SystemDecree d = Deserialize<SystemDecree>(entry);
              if (d.operation == SystemOperation::AddReplica)
              {
                  Replica r = Deserialize<Replica>(d.content);
                  legislators->Add(r);
                  std::ofstream replicasetfile(
                      (fs::path(location) /
                       fs::path(ReplicasetFilename)).string());
                  SaveReplicaSet(legislators, replicasetfile);
                  SendBootstrap(r, location);
              }
              else if (d.operation == SystemOperation::RemoveReplica)
              {
                  Replica r = Deserialize<Replica>(d.content);
                  legislators->Remove(r);
                  std::ofstream replicasetfile(
                      (fs::path(location) /
                       fs::path(ReplicasetFilename)).string());
                  SaveReplicaSet(legislators, replicasetfile);
              }
          })))
{
    for (auto l : *legislators)
    {
        try
        {
            receiver = std::make_shared<NetworkReceiver>(l.hostname, l.port);
            sender = std::make_shared<NetworkSender>(legislators);

            proposer = std::make_shared<ProposerContext>(
                legislators,
                ledger->Tail().number + 1
            );
            acceptor = std::make_shared<AcceptorContext>(location);
            learner = std::make_shared<LearnerContext>(legislators, ledger);
            updater = std::make_shared<UpdaterContext>(ledger);

            RegisterProposer(
                receiver,
                sender,
                proposer
            );
            RegisterAcceptor(
                receiver,
                sender,
                acceptor
            );
            RegisterLearner(
                receiver,
                sender,
                learner
            );
            RegisterUpdater(
                receiver,
                sender,
                updater
            );

            std::make_shared<BootstrapListener>(
                l.hostname,
                l.port + 1
            );
        }
        catch (
            boost::exception_detail::clone_impl<
                boost::exception_detail::error_info_injector<
                    boost::system::system_error>>& e)
        {
        }
    }

}


void
Parliament::AddLegislator(std::string address, short port)
{
    Decree d;
    d.type = DecreeType::SystemDecree;
    d.content = Serialize(
        SystemDecree(
            SystemOperation::AddReplica,
            0,
            Serialize(Replica(address, port))
        )
    );
    send_decree(d);
}


void
Parliament::RemoveLegislator(std::string address, short port)
{
    Decree d;
    d.type = DecreeType::SystemDecree;
    d.content = Serialize(
        SystemDecree(
            SystemOperation::RemoveReplica,
            0,
            Serialize(Replica(address, port))
        )
    );
    send_decree(d);
}


void
Parliament::SendProposal(std::string entry)
{
    Decree d;
    d.content = entry;
    send_decree(d);
}


void
Parliament::send_decree(Decree d)
{
    for (Replica r : *legislators)
    {
        Message m(
            d,
            Replica(r.hostname, r.port),
            Replica(r.hostname, r.port),
            MessageType::RequestMessage);

        sender->Reply(m);
        break;
    }
}


AbsenteeBallots
Parliament::GetAbsenteeBallots(int max_ballots)
{
    std::map<Decree, std::shared_ptr<ReplicaSet>, compare_decree> ballots;

    int current = 0;
    for (auto vote : learner->accepted_map)
    {
        if (current >= learner->accepted_map.size() - max_ballots)
        {
            ballots[vote.first] = learner->replicaset->Difference(vote.second);
        }
        current += 1;
    }
    return ballots;

}


void
Parliament::SetActive()
{
    learner->is_observer = false;
}


void
Parliament::SetInactive()
{
    learner->is_observer = true;
}
