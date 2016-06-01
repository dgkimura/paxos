#include <acceptor.hpp>


void
RegisterAcceptor(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<AcceptorContext> context)
{
    using namespace std::placeholders;

    receiver->RegisterCallback(
        Callback(std::bind(HandlePrepare, _1, context, sender)),
        MessageType::PrepareMessage
    );
    receiver->RegisterCallback(
        Callback(std::bind(HandleAccept, _1, context, sender)),
        MessageType::AcceptMessage
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
        sender->Reply(Response(message, MessageType::PromiseMessage));
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
        sender->ReplyAll(Response(message, MessageType::AcceptedMessage));
    }
}
