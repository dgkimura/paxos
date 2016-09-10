#include "paxos/logging.hpp"
#include "paxos/roles.hpp"


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
        //
        // If the messaged decree is higher than any previoiusly promised
        // decree and the sender is a known sender in our replica set then
        // remember the sent decree as the highest promised decree.
        //
        context->highest_proposed_decree = message.decree;
    }

    if (context->promise_map.find(message.decree) == context->promise_map.end())
    {
        //
        // If there is no entry for the messaged decree then make an entry.
        //
        context->promise_map[message.decree] = std::make_shared<ReplicaSet>();
    }

    if (IsDecreeEqual(message.decree, context->highest_proposed_decree))
    {
        //
        // If the messaged decree is the highest promised decree then update
        // our promised decree map and calculate if majority of replicas have
        // sent promises for the decree.
        //
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

    if (context->promise_map.find(message.decree) != context->promise_map.end()
        && context->promise_map[message.decree]
                  ->Intersection(context->replicaset)
                  ->GetSize() >= context->replicaset->GetSize())
    {
        //
        // If we have received an accepted message from every replica in the
        // replicaset for the given decree, then we are unlikely to receive
        // them again. Therefore we will reclaim memory.
        //
        context->promise_map.erase(message.decree);
    }
    context->current_decree_number = message.decree.number + 1;
}


void
HandlePrepare(
    Message message,
    std::shared_ptr<AcceptorContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandlePrepare | " << Serialize(message);

    if (IsDecreeHigher(message.decree, context->promised_decree.Value()))
    {
        //
        // If the messaged decree is higher than any decree seen before then
        // save it on persistent storage and then send a promised message.
        //
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
    LOG(LogLevel::Info) << "HandleAccept  | " << Serialize(message);

    if (IsDecreeHigherOrEqual(message.decree, context->promised_decree.Value()))
    {
        if (IsDecreeHigher(message.decree, context->accepted_decree.Value()))
        {
            context->accepted_decree = message.decree;
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
        if (IsDecreeOrdered(context->ledger->Tail(), message.decree))
        {
            //
            // Append the decree iff the decree is in order with the last
            // decree recorded in our ledger.
            //
            context->ledger->Append(message.decree);
        }
        else
        {
            //
            // If the decree is not in order with the last decree recorded in
            // our ledger then there must be holes in our ledger.
            //
            context->tracked_future_decrees.push_back(message.decree);
            Message response = Response(message, MessageType::UpdateMessage);
            response.decree = context->ledger->Tail();
            sender->Reply(response);
        }
    }
}


void
HandleUpdated(
    Message message,
    std::shared_ptr<LearnerContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandleUpdated| " << Serialize(message);

    if (IsDecreeOrdered(context->ledger->Tail(), message.decree))
    {
        //
        // Append the decree iff the decree is in order with the last decree
        // recorded in our ledger.
        //
        context->ledger->Append(message.decree);

        Decree previous_decree = message.decree;
        for (Decree current_decree : context->tracked_future_decrees)
        {
            //
            // If there are tracked_future_decrees then we should check if they
            // are now the next ordered decrees and append to the ledger
            // accordingly.
            //
            if (IsDecreeOrdered(previous_decree, current_decree))
            {
                context->ledger->Append(current_decree);
                previous_decree = current_decree;
            }
        }

        if (IsDecreeEqual(message.decree, previous_decree))
        {
            //
            // If the tracked_future_decrees did not contain the next ordered
            // decree then ask the message sender to send us more updates.
            //
            sender->Reply(Response(message, MessageType::UpdateMessage));
        }
    }
}


void
HandleUpdate(
    Message message,
    std::shared_ptr<UpdaterContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandleUpdate| " << Serialize(message);

    //
    // Check if our ledger has the next logical ordered decree.
    //
    Decree next = context->ledger->Next(message.decree);

    if (IsDecreeOrdered(message.decree, next))
    {
        sender->Reply(Response(message, MessageType::UpdatedMessage));
    }
}
