#include <proposer.hpp>

void
RegisterProposer(Receiver receiver, Sender& sender)
{
    using namespace std::placeholders;

    std::shared_ptr<Context> context(new Context());

    receiver.RegisterCallback<PrepareMessage>(
        Callback(std::bind(HandlePrepare, _1, context, sender))
    );
    receiver.RegisterCallback<AcceptMessage>(
        Callback(std::bind(HandleAccept, _1, context, sender))
    );
}


void
HandlePrepare(Message message, std::shared_ptr<Context> context, Sender& sender)
{
}


void
HandleAccept(Message message, std::shared_ptr<Context> context, Sender& sender)
{
}
