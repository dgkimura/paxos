#include "paxos/parliament.hpp"


Parliament::Parliament(
    Replica legislator,
    std::string location,
    DecreeHandler decree_handler)
    : legislators(LoadReplicaSet(
          std::ifstream(
              (fs::path(location) / fs::path(ReplicasetFilename)).string()))),
      ledger(std::make_shared<Ledger>(
          std::make_shared<PersistentQueue<Decree>>(location, "paxos.ledger"),
          decree_handler,
          DecreeHandler([this, location](std::string entry){
              SystemOperation op = Deserialize<SystemOperation>(entry);
              if (op.operation == SystemOperationType::AddReplica)
              {
                  legislators->Add(op.replica);
                  std::ofstream replicasetfile(
                      (fs::path(location) /
                       fs::path(ReplicasetFilename)).string());
                  SaveReplicaSet(legislators, replicasetfile);
                  SendBootstrap<BoostTransport>(op.replica, op.content);
              }
              else if (op.operation == SystemOperationType::RemoveReplica)
              {
                  legislators->Remove(op.replica);
                  std::ofstream replicasetfile(
                      (fs::path(op.content) /
                       fs::path(ReplicasetFilename)).string());
                  SaveReplicaSet(legislators, replicasetfile);
              }
          })))
{
    receiver = std::make_shared<NetworkReceiver<BoostServer>>(
        legislator.hostname, legislator.port);
    sender = std::make_shared<NetworkSender<BoostTransport>>(legislators);
    auto acceptor = std::make_shared<AcceptorContext>(
        std::make_shared<PersistentDecree>(location, "paxos.promised_decree"),
        std::make_shared<PersistentDecree>(location, "paxos.accepted_decree"));
    hookup_legislator(legislator, location, acceptor);

    bootstrap = std::make_shared<BootstrapListener<BoostServer>>(
        legislator.hostname,
        legislator.port + 1
    );
}


Parliament::Parliament(
    Replica legislator,
    std::shared_ptr<ReplicaSet> legislators,
    std::shared_ptr<Ledger> ledger,
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<AcceptorContext> acceptor
) :
    legislators(legislators),
    receiver(receiver),
    sender(sender),
    ledger(ledger)
{
    hookup_legislator(legislator, ".", acceptor);
}


void
Parliament::hookup_legislator(
    Replica replica,
    std::string location,
    std::shared_ptr<AcceptorContext> acceptor)
{
    auto proposer = std::make_shared<ProposerContext>(
        legislators,
        ledger->Tail().number + 1
    );
    learner = std::make_shared<LearnerContext>(legislators, ledger);
    auto updater = std::make_shared<UpdaterContext>(ledger);

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
}


void
Parliament::AddLegislator(
    std::string address,
    short port,
    std::string location)
{
    Decree d;
    d.type = DecreeType::SystemDecree;
    d.content = Serialize(
        SystemOperation(
            SystemOperationType::AddReplica,
            0,
            Replica(address, port),
            location
        )
    );
    send_decree(d);
}


void
Parliament::RemoveLegislator(
    std::string address,
    short port,
    std::string location)
{
    Decree d;
    d.type = DecreeType::SystemDecree;
    d.content = Serialize(
        SystemOperation(
            SystemOperationType::RemoveReplica,
            0,
            Replica(address, port),
            location
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
        if (current <= max_ballots)
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
