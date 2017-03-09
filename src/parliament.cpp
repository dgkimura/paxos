#include "paxos/parliament.hpp"


Parliament::Parliament(
    Replica legislator,
    std::string location,
    Handler decree_handler)
    : legislator(legislator),
      legislators(LoadReplicaSet(
          std::ifstream(
              (fs::path(location) / fs::path(ReplicasetFilename)).string()))),
      receiver(std::make_shared<NetworkReceiver<BoostServer>>(
               legislator.hostname, legislator.port)),
      sender(std::make_shared<NetworkSender<BoostTransport>>(legislators)),
      bootstrap(
          std::make_shared<BootstrapListener<BoostServer>>(
              legislators,
              legislator.hostname,
              legislator.port + 1
          )
      ),
      ledger(std::make_shared<Ledger>(
          std::make_shared<PersistentQueue<Decree>>(location, "paxos.ledger"))),
      learner(std::make_shared<LearnerContext>(legislators, ledger)),
      location(location)
{
    ledger->RegisterHandler(
        DecreeType::UserDecree,
        std::make_shared<CompositeHandler>(decree_handler));
    ledger->RegisterHandler(
        DecreeType::AddReplicaDecree,
        std::make_shared<HandleAddNode>(location, legislator, legislators)
    );
    ledger->RegisterHandler(
        DecreeType::RemoveReplicaDecree,
        std::make_shared<HandleRemoveNode>(location, legislators)
    );

    auto acceptor = std::make_shared<AcceptorContext>(
        std::make_shared<PersistentDecree>(location, "paxos.promised_decree"),
        std::make_shared<PersistentDecree>(location, "paxos.accepted_decree"));
    hookup_legislator(legislator, acceptor);
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
    ledger(ledger),
    learner(std::make_shared<LearnerContext>(legislators, ledger))
{
    hookup_legislator(legislator, acceptor);
}


void
Parliament::hookup_legislator(
    Replica replica,
    std::shared_ptr<AcceptorContext> acceptor)
{
    auto proposer = std::make_shared<ProposerContext>(
        legislators,
        ledger
    );
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
    std::string remote)
{
    Decree d;
    d.type = DecreeType::AddReplicaDecree;
    d.content = Serialize(
        SystemOperation(
            legislator,
            0,
            Replica(address, port),
            Serialize(
                BootstrapMetadata
                {
                    location,
                    remote
                }
            )
        )
    );
    send_decree(d);
}


void
Parliament::RemoveLegislator(
    std::string address,
    short port,
    std::string remote)
{
    Decree d;
    d.type = DecreeType::RemoveReplicaDecree;
    d.content = Serialize(
        SystemOperation(
            legislator,
            0,
            Replica(address, port),
            Serialize(
                BootstrapMetadata
                {
                    location,
                    remote
                }
            )
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
    Message m(
        d,
        legislator,
        legislator,
        MessageType::RequestMessage);

    sender->Reply(m);
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
