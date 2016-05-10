#include <proposer.hpp>

void
RegisterProposer(Receiver receiver, Sender& sender)
{
    using namespace std::placeholders;

    std::shared_ptr<Context> context(new Context());

    receiver.RegisterCallback<RequestMessage>(
        Callback(std::bind(HandleRequest, _1, context, sender))
    );
    receiver.RegisterCallback<PromiseMessage>(
        Callback(std::bind(HandlePromise, _1, context, sender))
    );
    receiver.RegisterCallback<AcceptedMessage>(
        Callback(std::bind(HandleAccepted, _1, context, sender))
    );
}


void
HandleRequest(Message message, std::shared_ptr<Context> context, Sender& sender)
{
}


void
HandlePromise(Message message, std::shared_ptr<Context> context, Sender& sender)
{
}


void
HandleAccepted(Message message, std::shared_ptr<Context> context, Sender& sender)
{
}
