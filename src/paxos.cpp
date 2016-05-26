#include <memory>

#include <paxos.hpp>


void
Init()
{
    std::shared_ptr<Receiver> receiver(new Receiver());
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
