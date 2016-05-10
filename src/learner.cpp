#include <learner.hpp>


void
RegisterLearner(Receiver receiver, Sender& sender)
{
    using namespace std::placeholders;

    std::shared_ptr<Context> context(new Context());

    receiver.RegisterCallback<AcceptedMessage>(
        Callback(std::bind(HandleProclaim, _1, context, sender))
    );
}


void
HandleProclaim(Message message, std::shared_ptr<Context> context, Sender& sender)
{
}
