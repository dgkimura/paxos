#include <paxos.hpp>


Instance *
CreateInstance()
{
    Instance *i = new Instance();
    i->Receiver = std::shared_ptr<Receiver>(new NetworkReceiver());
    i->Sender = std::shared_ptr<Sender>(new NetworkSender());
    i->Proposer = std::shared_ptr<ProposerContext>(new ProposerContext());
    i->Acceptor = std::shared_ptr<AcceptorContext>(new AcceptorContext());
    i->Learner = std::shared_ptr<LearnerContext>(new LearnerContext());
    i->Updater = std::shared_ptr<UpdaterContext>(new UpdaterContext());
    return i;
}


void
Init()
{
    Instance *i = CreateInstance();

    RegisterProposer(
        i->Receiver,
        i->Sender,
        i->Proposer
    );
    RegisterAcceptor(
        i->Receiver,
        i->Sender,
        i->Acceptor
    );
    RegisterLearner(
        i->Receiver,
        i->Sender,
        i->Learner
    );

    _Instances.push_back(i);
}
