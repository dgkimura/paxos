#include <logging.hpp>
#include <roles.hpp>


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
RegisterLearner(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<LearnerContext> context)
{
    using namespace std::placeholders;

    receiver->RegisterCallback(
        Callback(std::bind(HandleProclaim, _1, context, sender)),
        MessageType::AcceptedMessage
    );
    receiver->RegisterCallback(
        Callback(std::bind(HandleUpdated, _1, context, sender)),
        MessageType::UpdatedMessage
    );
}


void
RegisterUpdater(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<UpdaterContext> context)
{
    using namespace std::placeholders;

    receiver->RegisterCallback(
        Callback(std::bind(HandleUpdate, _1, context, sender)),
        MessageType::UpdateMessage
    );
}


void
HandleRequest(
    Message message,
    std::shared_ptr<ProposerContext> context,
    std::shared_ptr<Sender> sender)
{
    context->requested_values.push_back(message.decree.content);

    Message response = Response(message, MessageType::PrepareMessage);
    response.decree.number = context->current_decree_number;
    response.decree.content = "";
    sender->ReplyAll(response);
}


void
HandlePromise(
    Message message,
    std::shared_ptr<ProposerContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandlePromise | " << Serialize(message);
    if (IsDecreeHigher(message.decree, context->highest_proposed_decree) &&
        context->replicaset->Contains(message.from))
    {
        context->highest_proposed_decree = message.decree;
    }

    if (context->promise_map.find(message.decree) == context->promise_map.end())
    {
        context->promise_map[message.decree] = std::shared_ptr<ReplicaSet>(new ReplicaSet());
    }

    if (IsDecreeEqual(message.decree, context->highest_proposed_decree))
    {
        context->promise_map[message.decree]->Add(message.from);

        int minimum_quorum = context->replicaset->GetSize() / 2 + 1;
        int received_promises = context->promise_map[message.decree]
                                       ->Intersection(context->replicaset)
                                       ->GetSize();

        if (received_promises >= minimum_quorum)
        {
            if (message.decree.content.empty() && !context->requested_values.empty())
            {
                message.decree.content = context->requested_values[0];
                context->requested_values.erase(context->requested_values.begin());
            }
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
    LOG(LogLevel::Info) << "HandleAccepted| " << Serialize(message);
    context->promise_map.erase(message.decree);
    context->current_decree_number++;
}


void
HandlePrepare(
    Message message,
    std::shared_ptr<AcceptorContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandlePrepare | " << Serialize(message);
    if (IsDecreeHigher(message.decree, context->state.promised_decree))
    {
        context->state.promised_decree = message.decree;
        sender->Reply(Response(message, MessageType::PromiseMessage));
    }
}


void
HandleAccept(
    Message message,
    std::shared_ptr<AcceptorContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandleAccept  | " << Serialize(message);
    if (IsDecreeHigherOrEqual(message.decree, context->state.promised_decree))
    {
        if (IsDecreeHigher(message.decree, context->state.accepted_decree))
        {
            context->state.accepted_decree = message.decree;
        }
        sender->ReplyAll(Response(message, MessageType::AcceptedMessage));
    }
}


void
HandleProclaim(
    Message message,
    std::shared_ptr<LearnerContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandleProclaim| " << Serialize(message);
    if (context->accepted_map.find(message.decree) == context->accepted_map.end())
    {
        context->accepted_map[message.decree] = std::shared_ptr<ReplicaSet>(
                                                    new ReplicaSet());
    }
    if (context->replicaset->Contains(message.from))
    {
        context->accepted_map[message.decree]->Add(message.from);
    }

    int minimum_quorum = context->replicaset->GetSize() / 2 + 1;
    int accepted_quorum = context->accepted_map[message.decree]->GetSize();

    if (accepted_quorum >= minimum_quorum)
    {
        context->ledger->Append(message.decree);
    }
}


void
HandleUpdated(
    Message message,
    std::shared_ptr<LearnerContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandleUpdated| " << Serialize(message);

    // 1. Check if message.decree == highest ledger entry + 1
    // 2. If correct entry, then write to ledger
    // 3. Reply with another UpdateMessage
}


void
HandleUpdate(
    Message message,
    std::shared_ptr<UpdaterContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandleUpdate| " << Serialize(message);

    // 1. Check message.decree and see if ledger contains message.decree + 1
    // 2. Reply UpdatedMessage with message.decree + 1 or highest known decree
}
