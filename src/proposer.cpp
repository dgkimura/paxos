#include <proposer.hpp>

void
RegisterProposer(Receiver receiver, std::shared_ptr<Sender> sender)
{
    using namespace std::placeholders;

    std::shared_ptr<ProposerContext> context(new ProposerContext());

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
HandleRequest(Message message, std::shared_ptr<ProposerContext> context, std::shared_ptr<Sender> sender)
{
}


void
HandlePromise(Message message, std::shared_ptr<ProposerContext> context, std::shared_ptr<Sender> sender)
{
    if (IsDecreeHigher(message.decree, context->highest_promised_decree))
    {
        context->highest_promised_decree = message.decree;
        context->promise_map[message.decree] = std::shared_ptr<ReplicaSet>(new ReplicaSet());
    }

    if (IsDecreeEqual(message.decree, context->highest_promised_decree))
    {
        context->promise_map[message.decree]->Add(message.from);

        int minimum_quorum = context->full_replicaset.GetSize() / 2 + 1;
        int received_promises = context->promise_map[message.decree]->GetSize();

        if (received_promises >= minimum_quorum)
        {
            sender->ReplyAll<AcceptedMessage>(message);
        }
    }

}


void
HandleAccepted(Message message, std::shared_ptr<ProposerContext> context, std::shared_ptr<Sender> sender)
{
    context->promise_map.erase(message.decree);
}
