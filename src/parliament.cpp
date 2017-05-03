#include <mutex>

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
      location(location),
      signal(std::make_shared<Signal>())
{
    ledger->RegisterHandler(
        DecreeType::UserDecree,
        std::make_shared<CompositeHandler>(decree_handler)
    );
    ledger->RegisterHandler(
        DecreeType::AddReplicaDecree,
        std::make_shared<HandleAddReplica>(
            location,
            legislator,
            legislators,
            signal)
    );
    ledger->RegisterHandler(
        DecreeType::RemoveReplicaDecree,
        std::make_shared<HandleRemoveReplica>(
            location,
            legislator,
            legislators,
            signal)
    );

    auto proposer = std::make_shared<ProposerContext>(
        legislators,
        ledger,
        std::make_shared<PersistentDecree>(
            location,
            "paxos.highest_proposed_decree"));
    auto acceptor = std::make_shared<AcceptorContext>(
        std::make_shared<PersistentDecree>(location, "paxos.promised_decree"),
        std::make_shared<PersistentDecree>(location, "paxos.accepted_decree"));
    hookup_legislator(legislator, proposer, acceptor);
}


Parliament::Parliament(
    Replica legislator,
    std::shared_ptr<ReplicaSet> legislators,
    std::shared_ptr<Ledger> ledger,
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<AcceptorContext> acceptor,
    std::shared_ptr<ProposerContext> proposer,
    std::shared_ptr<Signal> signal
) :
    legislators(legislators),
    receiver(receiver),
    sender(sender),
    ledger(ledger),
    learner(std::make_shared<LearnerContext>(legislators, ledger)),
    signal(signal)
{
    hookup_legislator(legislator, proposer, acceptor);
}


void
Parliament::hookup_legislator(
    Replica replica,
    std::shared_ptr<ProposerContext> proposer,
    std::shared_ptr<AcceptorContext> acceptor)
{
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
        UpdateReplicaSetDecree
        {
            legislator,
            Replica(address, port),
            remote
        }
    );
    send_decree(d);

    // Block our main thread until decree handler is finished.
    signal->Wait();
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
        UpdateReplicaSetDecree
        {
            legislator,
            Replica(address, port),
            remote
        }
    );
    send_decree(d);
    signal->Wait();
}


std::shared_ptr<ReplicaSet>
Parliament::GetLegislators()
{
    return legislators;
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

    std::lock_guard<std::mutex> lock(learner->mutex);

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
