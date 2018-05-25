#include <mutex>

#include "paxos/parliament.hpp"


namespace paxos
{


Parliament::Parliament(
    Replica legislator,
    std::string location,
    Handler accept_handler,
    Handler ignore_handler)
    : legislator(legislator),
      legislators(LoadReplicaSet(
          std::ifstream(
              (boost::filesystem::path(location) /
               boost::filesystem::path(ReplicasetFilename)).string()))),
      receiver(std::make_shared<NetworkReceiver<AsynchronousServer>>(
               legislator.hostname, legislator.port, legislators)),
      sender(std::make_shared<NetworkSender<BoostTransport>>(legislators)),
      bootstrap(
          std::make_shared<BootstrapListener<SynchronousServer>>(
              legislators,
              legislator.hostname,
              legislator.port + 1
          )
      ),
      ledger(std::make_shared<Ledger>(
          std::make_shared<RolloverQueue<Decree>>(location, "paxos.ledger"))),
      learner(std::make_shared<LearnerContext>(legislators, ledger)),
      location(location),
      signal(std::make_shared<Signal>([this](){ SendProposal(""); },
             std::chrono::milliseconds(1000)))
{
    ledger->RegisterHandler(
        DecreeType::UserDecree,
        std::make_shared<CompositeHandler>(accept_handler)
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
            "paxos.highest_proposed_decree"),
        ignore_handler,
        std::make_shared<RandomPause>(std::chrono::milliseconds(100)),
        signal);
    auto acceptor = std::make_shared<AcceptorContext>(
        std::make_shared<PersistentDecree>(location, "paxos.promised_decree"),
        std::make_shared<PersistentDecree>(location, "paxos.accepted_decree"),
        std::chrono::milliseconds(1000));
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
    std::shared_ptr<LearnerContext> learner
) :
    legislators(legislators),
    receiver(receiver),
    sender(sender),
    ledger(ledger),
    learner(learner),
    signal(proposer->signal)
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


bool
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
    return signal->Wait();
}


bool
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
    return signal->Wait();
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
    d.author = legislator;
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
    //
    // Guard against changes to accepted_map while counting absentees.
    //
    std::lock_guard<std::mutex> lock(learner->mutex);

    std::map<Decree, std::shared_ptr<ReplicaSet>, compare_decree> ballots;

    auto last = learner->ledger->Tail().root_number;
    int start = last - max_ballots + 1;

    for (auto kv : learner->accepted_map)
    {
        if (start <= kv.first.root_number)
        {
            Decree d(Replica(), kv.first.number, "", DecreeType::UserDecree);
            d.root_number = kv.first.root_number;
            ballots[d] = learner->replicaset->Difference(kv.second);
        }
    }

    for (int i=start; i<=last; i++)
    {
        Decree d(Replica(), i, "", DecreeType::UserDecree);

        if (ballots.find(d) == ballots.end() && 0 < i)
        {
            ballots[d] = std::make_shared<ReplicaSet>();
        }
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


}
