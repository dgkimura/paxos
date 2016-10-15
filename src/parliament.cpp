#include "paxos/parliament.hpp"


Parliament::Parliament(std::string location_, DecreeHandler decree_handler)
    : location(location_),
      legislators(std::make_shared<ReplicaSet>()),
      ledger(std::make_shared<Ledger>(
          std::make_shared<PersistentQueue<Decree>>(location, "paxos.ledger"),
          decree_handler))
{
}


void
Parliament::AddLegislator(std::string address, short port)
{
    legislators->Add(Replica(address, port));
}


void
Parliament::SetLegislator(std::string address, short port)
{
    receiver = std::make_shared<NetworkReceiver>(address, port);
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

    legislators->Add(Replica(address, port));
}


void
Parliament::RemoveLegislator(Replica replica)
{
    legislators->Remove(replica);
}


void
Parliament::CreateProposal(std::string entry)
{
    Decree d;
    d.content = entry;
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
