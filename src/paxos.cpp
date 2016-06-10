#include <paxos.hpp>


Parliament::Parliament()
    : legislators(std::make_shared<ReplicaSet>()),
      receiver(std::make_shared<NetworkReceiver>()),
      sender(std::make_shared<NetworkSender>(legislators)),
      proposer(std::make_shared<ProposerContext>()),
      acceptor(std::make_shared<AcceptorContext>()),
      learner(std::make_shared<LearnerContext>()),
      updater(std::make_shared<UpdaterContext>())
{
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
Parliament::AddLegislator(Replica replica)
{
    legislators->Add(replica);
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
            Replica(),
            Replica(r.hostname, r.port),
            MessageType::RequestMessage);

        sender->Reply(m);
        break;
    }
}
