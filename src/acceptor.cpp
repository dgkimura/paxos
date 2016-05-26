#include <acceptor.hpp>


void
RegisterAcceptor(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<AcceptorContext> context)
{
    using namespace std::placeholders;

    receiver->RegisterCallback<PrepareMessage>(
        Callback(std::bind(HandlePrepare, _1, context, sender))
    );
    receiver->RegisterCallback<AcceptMessage>(
        Callback(std::bind(HandleAccept, _1, context, sender))
    );
}


void
HandlePrepare(
    Message message,
    std::shared_ptr<AcceptorContext> context,
    std::shared_ptr<Sender> sender)
{
    if (IsDecreeHigherOrEqual(message.decree, context->promised_decree))
    {
        context->promised_decree = message.decree;
        sender->Reply(Response<PromiseMessage>(message));
    }
}


void
HandleAccept(
    Message message,
    std::shared_ptr<AcceptorContext> context,
    std::shared_ptr<Sender> sender)
{
    if (IsDecreeHigherOrEqual(message.decree, context->promised_decree))
    {
        if (IsDecreeHigher(message.decree, context->accepted_decree))
        {
            context->accepted_decree = message.decree;
        }
        sender->ReplyAll(Response<AcceptedMessage>(message));
    }
}
