#include <paxos.hpp>


void
Init()
{
    Receiver receiver;
    std::shared_ptr<NetworkSender> sender(new NetworkSender());

    RegisterProposer(
        receiver,
        sender,
        std::shared_ptr<ProposerContext>(new ProposerContext())
    );
    RegisterAcceptor(
        receiver,
        sender,
        std::shared_ptr<AcceptorContext>(new AcceptorContext())
    );
    RegisterLearner(
        receiver,
        sender,
        std::shared_ptr<LearnerContext>(new LearnerContext())
    );
}
