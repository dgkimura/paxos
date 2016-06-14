#include <proposer.hpp>


void
RegisterProposer(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<ProposerContext> context)
{
    using namespace std::placeholders;

    receiver->RegisterCallback(
        Callback(std::bind(HandleRequest, _1, context, sender)),
        MessageType::RequestMessage
    );
    receiver->RegisterCallback(
        Callback(std::bind(HandlePromise, _1, context, sender)),
        MessageType::PromiseMessage
    );
    receiver->RegisterCallback(
        Callback(std::bind(HandleAccepted, _1, context, sender)),
        MessageType::AcceptedMessage
    );
}


void
HandleRequest(
    Message message,
    std::shared_ptr<ProposerContext> context,
    std::shared_ptr<Sender> sender)
{
    Message response = Response(message, MessageType::PrepareMessage);
    response.decree.number = context->highest_promised_decree.number;
    sender->ReplyAll(response);
}


void
HandlePromise(
    Message message,
    std::shared_ptr<ProposerContext> context,
    std::shared_ptr<Sender> sender)
{
    if (IsDecreeHigher(message.decree, context->highest_promised_decree))
    {
        context->highest_promised_decree = message.decree;
    }

    if (context->promise_map.find(message.decree) == context->promise_map.end())
    {
        context->promise_map[message.decree] = std::shared_ptr<ReplicaSet>(new ReplicaSet());
    }

    if (IsDecreeEqual(message.decree, context->highest_promised_decree))
    {
        context->promise_map[message.decree]->Add(message.from);

        int minimum_quorum = context->replicaset->GetSize() / 2 + 1;
        int received_promises = context->promise_map[message.decree]->GetSize();

        if (received_promises >= minimum_quorum)
        {
            sender->ReplyAll(Response(message, MessageType::AcceptMessage));
        }
    }

}


void
HandleAccepted(
    Message message,
    std::shared_ptr<ProposerContext> context,
    std::shared_ptr<Sender> sender)
{
    context->promise_map.erase(message.decree);
}
